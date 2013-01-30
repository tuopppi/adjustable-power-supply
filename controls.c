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
    UNKNOWN_INPUT,
    VOLTAGE_LEFT,
    VOLTAGE_RIGHT,
    CURRENT_LEFT,
    CURRENT_RIGHT,
    VOLTAGE_BTN,
    CURRENT_BTN,
    TOP_BTN
};

void set_and_save_voltage(int8_t diff) {
    set_voltage(*get_voltage() + diff);
    set_dynamic_readout(get_voltage());
    evq_timed_push(save_eeprom_voltage, 0, 3000);
}

#define VOLTAGE_CHANGE_PER_NOTCH 5
void voltage_knob_handler(uint16_t usr_input) {
    status_led_on(LED_VOLTAGE);
    status_led_off(LED_CURRENT);
    switch(usr_input) {
        case VOLTAGE_LEFT:
            set_and_save_voltage(-VOLTAGE_CHANGE_PER_NOTCH);
            break;

        case VOLTAGE_RIGHT:
            set_and_save_voltage(VOLTAGE_CHANGE_PER_NOTCH);
            break;

        case VOLTAGE_BTN:
            set_dynamic_readout(get_voltage());
            break;
    }
}

void set_and_save_current(int8_t diff) {
    set_current_limit(*get_current_limit() + diff);
    set_dynamic_readout(get_current_limit());
    evq_timed_push(save_eeprom_current_limit, 0, 3000);
}

#define CURRENT_CHANGE_PER_NOTCH 10
void current_knob_handler(uint16_t usr_input) {
    status_led_on(LED_CURRENT);
    status_led_off(LED_VOLTAGE);
    switch(usr_input) {
    case CURRENT_LEFT:
        set_and_save_current(-CURRENT_CHANGE_PER_NOTCH);
        break;

    case CURRENT_RIGHT:
        set_and_save_current(CURRENT_CHANGE_PER_NOTCH);
        break;

    case CURRENT_BTN:
        set_dynamic_readout(get_current());
        break;
    }
}

void button_handler(uint16_t usr_input) {
    if(usr_input == TOP_BTN) {
        set_static_readout(DISPLAY_CUR);
    }
}

/* VOLTAGE
 * =======
 * ENC1 A - PD5 - PCINT21 - PCMSK2 -(sininen)
 * ENC1 B - PB6 - (punainen)
 * ENC1 S - PD2 - INT0
 *
 * CURRENT
 * =======
 * ENC2 A - PB7 - PCINT7 - PCMSK0 - (keltainen)
 * ENC2 B - PD6 - (valkoinen)
 * ENC2 S - PD3 - INT1

 * TACTILE SW
 * ==========
 * PD4 - PCINT20 - PCMSK2
 *
 * TOGGLE SW
 * PC4 - PCINT12 - PCMSK1
 */

void init_controls(void) {
    // pull-ups
    PORTB |= _BV(PORTB6) | _BV(PORTB7);
    PORTD |= _BV(PORTD2) | _BV(PORTD3) |_BV(PORTD4) | _BV(PORTD5) | _BV(PORTD6);
    // Pin change interrupts
    PCMSK0 |= _BV(PCINT7);
    PCMSK1 |= _BV(PCINT12);
    PCMSK2 |= _BV(PCINT21) | _BV(PCINT20);
    PCICR |= _BV(PCIE0) | _BV(PCIE1) | _BV(PCIE2);
    // ext interrupts (INT0, INT1), falling edge
    EICRA |= _BV(ISC11) | _BV(ISC01);
    EIMSK |= _BV(INT0) | _BV(INT1);
}

enum encoderid {
    ENC_CURRENT = 0b01,
    ENC_VOLTAGE = 0b10
};

uint16_t encoder_direction(uint8_t id) {
    switch(id) {
    case ENC_CURRENT:
        if(PIND & _BV(PIND6)) { return CURRENT_LEFT; }
        else { return CURRENT_RIGHT; }

    case ENC_VOLTAGE:
        if(PINB & _BV(PINB6)) { return VOLTAGE_LEFT; }
        else { return VOLTAGE_RIGHT; }
    }

    return UNKNOWN_INPUT;
}

ISR(PCINT0_vect) {
    // ENC2 A
    if(PINB & _BV(PINB7)) {
        evq_push(current_knob_handler, encoder_direction(ENC_CURRENT));
    }
}

ISR(PCINT1_vect) {
    status_led_toggle(LED_VOLTAGE);
    /*if(PINC & _BV(PINC4)) {
        status_led_off(LED_VOLTAGE);
    } else {
        status_led_on(LED_VOLTAGE);
    }*/
}

ISR(PCINT2_vect) {
    // TODO: Cant react to VOLTAGE knob while switch is pressed
    static uint8_t wait_for_btn_release = 0;

    // TACTILE SW RELEASE
    if(wait_for_btn_release) {
        wait_for_btn_release = 0;
        return;
    }

    // TACTILE SW PRESS
    if((PIND & _BV(PIND4)) == 0) {
        evq_push(button_handler, TOP_BTN);
        wait_for_btn_release = 1;
        return;
    }

    // ENC1 A
    if(PIND & _BV(PIND5) ) {
        evq_push(voltage_knob_handler, encoder_direction(ENC_VOLTAGE));
    }

}

ISR(INT0_vect) {
    evq_push(voltage_knob_handler, VOLTAGE_BTN);
}

ISR(INT1_vect) {
    evq_push(current_knob_handler, CURRENT_BTN);
}


