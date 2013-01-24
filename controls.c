/*
 * controls.c
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply project.
 */

#include "peripherals.h"
#include "display.h"
#include "eventqueue.h"
#include "controls.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>

enum usr_input_events {
    VOLTAGE_LEFT,
    VOLTAGE_RIGHT,
    CURRENT_LEFT,
    CURRENT_RIGHT,
    VOLTAGE_BTN,
    CURRENT_BTN,
    TOP_BTN
};

void voltage_knob_handler(uint16_t usr_input) {
    switch(usr_input) {
        case VOLTAGE_LEFT:
            evq_push(set_voltage, *get_voltage() - 5);
            set_display_readout(get_voltage());
            evq_push(save_eeprom_voltage, *get_voltage());
            break;

        case VOLTAGE_RIGHT:
            evq_push(set_voltage, *get_voltage() + 5);
            set_display_readout(get_voltage());
            evq_push(save_eeprom_voltage, *get_voltage());
            break;

        case VOLTAGE_BTN:
            set_display_readout(get_voltage());
            break;
    }
}

void current_knob_handler(uint16_t usr_input) {
    switch(usr_input) {
    case CURRENT_LEFT:
        evq_push(set_current_limit, *get_current_limit() - 10);
        set_display_readout(get_current_limit());
        evq_push(save_eeprom_current_limit, *get_current_limit());
        break;

    case CURRENT_RIGHT:
        evq_push(set_current_limit, *get_current_limit() + 10);
        set_display_readout(get_current_limit());
        evq_push(save_eeprom_current_limit, *get_current_limit());
        break;

    case CURRENT_BTN:
        set_display_readout(&display_current);
        break;
    }
}

void button_handler(uint16_t usr_input) {
    switch(usr_input) {
    case TOP_BTN:
        break;
    }
}

/*
 * ENCODERs
 * PB6 - ENC1 A
 * PB7 - ENC2 A
 * PD5 - ENC1 B
 * PD6 - ENC2 B

 *
 * PD4 - TACTILE SW
 * PD3 - ENC SW
 * PD2 - ENC SW
 *
 */
void init_controls(void) {
    // pull-ups
    PORTB |= _BV(PORTB6) | _BV(PORTB7);
    PORTD |= _BV(PORTD2) | _BV(PORTD3) |_BV(PORTD4) | _BV(PORTD5) | _BV(PORTD6);
    // Pin change interrupts
    PCMSK0 |= _BV(PCINT6) | _BV(PCINT7);
    PCMSK2 |= _BV(PCINT20);
    PCICR |= _BV(PCIE0) | _BV(PCIE2);
    // ext interrupts (INT0, INT1), falling edge
    EICRA |= _BV(ISC11) | _BV(ISC01);
    EIMSK |= _BV(INT0) | _BV(INT1);
}

enum encoderid {
    ENC_CURRENT = 1,
    ENC_VOLTAGE = 2
};

uint16_t encoder_direction(int id) {
    uint8_t direction = (PIND & (_BV(PIND5) | _BV(PIND6))) >> 5;

    if(id == ENC_CURRENT) {
        if(direction == 1) { return CURRENT_LEFT; }
        else if(direction == 3) { return CURRENT_RIGHT; }
    }
    else if(id == ENC_VOLTAGE) {
        if(direction == 3) { return VOLTAGE_LEFT; }
        else if(direction == 2) { return VOLTAGE_RIGHT; }
    }

    return 0;
}

ISR(PCINT0_vect) {
    // tarkistetaanko tuliko keskeytys ENCA vai ENCB
    int8_t trigger = (PINB & (_BV(PINB7) | _BV(PINB6))) >> 6;

    if(trigger == ENC_VOLTAGE) {
        evq_push(voltage_knob_handler, encoder_direction(ENC_VOLTAGE));
    }

    if(trigger == ENC_CURRENT) {
        evq_push(current_knob_handler, encoder_direction(ENC_CURRENT));
    }
}

ISR(INT0_vect) {
    evq_push(voltage_knob_handler, VOLTAGE_BTN);
}

ISR(INT1_vect) {
    evq_push(current_knob_handler, CURRENT_BTN);
}

ISR(PCINT2_vect) {
    if(!((PIND & _BV(PIND4)) >> 4)) {
        evq_push(button_handler, TOP_BTN);
    }
}
