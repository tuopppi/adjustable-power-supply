/*
 * display.c
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply project.
 */

#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include "display.h"
#include "peripherals.h"
#include "eventqueue.h"

volatile uint8_t seq_nbr;
volatile uint16_t* readout_p_;
volatile uint16_t static_readout_;
volatile char show_dots;

uint16_t get_readout_segments(uint8_t);
void display_handler(uint16_t);

#define TENS_OFFSET 10
#define HUNDRED_OFFSET 20
#define THOUSAND_OFFSET 30
#define OL 33
#define DOTS 34
#define CUR 35
uint16_t display_data[36][2] = {
    { 0b0000000000111001, 0b0000000001011010 }, // xxx0
    { 0b0000000000110001, 0b0000000000000000 }, // xxx1
    { 0b0000000000101001, 0b0000000001110010 }, // xxx2
    { 0b0000000000111001, 0b0000000000110010 }, // xxx3
    { 0b0000000000110001, 0b0000000000101010 }, // xxx4
    { 0b0000000000011001, 0b0000000000111010 }, // xxx5
    { 0b0000000000011001, 0b0000000001111010 }, // xxx6
    { 0b0000000000111001, 0b0000000000000000 }, // xxx7
    { 0b0000000000111001, 0b0000000001111010 }, // xxx8
    { 0b0000000000111001, 0b0000000000111010 }, // xxx9
    { 0b0000001011000001, 0b0000001110000010 }, // xx0x
    { 0b0000000000000000, 0b0000000110000010 }, // xx1x
    { 0b0000000111000001, 0b0000001100000010 }, // xx2x
    { 0b0000000110000001, 0b0000001110000010 }, // xx3x
    { 0b0000001100000001, 0b0000000110000010 }, // xx4x
    { 0b0000001110000001, 0b0000001010000010 }, // xx5x
    { 0b0000001111000001, 0b0000001010000010 }, // xx6x
    { 0b0000000000000000, 0b0000001110000010 }, // xx7x
    { 0b0000001111000001, 0b0000001110000010 }, // xx8x
    { 0b0000001110000001, 0b0000001110000010 }, // xx9x
    { 0b0001110000000001, 0b0010110000000010 }, // x0xx
    { 0b0001100000000001, 0b0000000000000000 }, // x1xx
    { 0b0001010000000001, 0b0011100000000010 }, // x2xx
    { 0b0001110000000001, 0b0001100000000010 }, // x3xx
    { 0b0001100000000001, 0b0001010000000010 }, // x4xx
    { 0b0000110000000001, 0b0001110000000010 }, // x5xx
    { 0b0000110000000001, 0b0011110000000010 }, // x6xx
    { 0b0001110000000001, 0b0000000000000000 }, // x7xx
    { 0b0001110000000001, 0b0011110000000010 }, // x8xx
    { 0b0001110000000001, 0b0001110000000010 }, // x9xx
    { 0b0000000000000000, 0b0000000000000000 }, // xxxx
    { 0b0110000000000001, 0b0000000000000000 }, // 1xxx
    { 0b1100000000000001, 0b1000000000000010 }, // 2xxx
    { 0b0000001011000001, 0b0000001111011010 }, // OL
    { 0b0000000000000000, 0b0000000000000110 }, // DOTS
    { 0b0000000011000001, 0b0011100011100010 }, // cur
};

void init_display(void) {
    init_spi();
    DDRD |= _BV(PD0) + _BV(PD1); // LED_VOLTAGE, LED_CURRENT outputs

    seq_nbr = 0;
    show_dots = 0;

    set_static_readout(0);
    evq_push(display_handler, 0); // start display
}

void set_dynamic_readout(uint16_t* readout) {
    readout_p_ = readout;
}

void set_static_readout(uint16_t readout) {
    static_readout_ = readout;
    readout_p_ = &static_readout_;
}

void display_dots(void) {
    show_dots ^= 1;
}

uint16_t get_readout_segments(uint8_t seq) {
    /* Display can show numerical values between 0 - 2999 */
    if (*readout_p_ < 3000 && *readout_p_ >= 0) {
        uint16_t segments = 0;
        int thousands = THOUSAND_OFFSET + (*readout_p_ / 1000);
        int hundreds = HUNDRED_OFFSET + (*readout_p_ % 1000) / 100;
        int tens = TENS_OFFSET + (*readout_p_ % 100) / 10;
        int ones = (*readout_p_ % 10);
        segments = display_data[thousands][seq] |
                   display_data[hundreds][seq]  |
                   display_data[tens][seq]      |
                   display_data[ones][seq];

        if(show_dots) {
            segments |= display_data[DOTS][seq];
        }
        return segments;
    } else {
        /* Other values mapped to special text strings */
        switch(*readout_p_) {
        case DISPLAY_CUR:
            return display_data[CUR][seq];
        default:
            return 0;
        }
    }
}

void display_handler(uint16_t null) {
    spi_send_word(get_readout_segments(seq_nbr % 2));
    seq_nbr++;

    // 100Hz refresh-rate
    evq_timed_push(display_handler, 0, 10);
}


/* LEDs --------------------------------------------------------------------- */

#define LEDPORT (PORTD)

uint8_t status_led_status(uint8_t led);

void status_led_on(uint16_t led) {
    LEDPORT |= led;
}

void status_led_off(uint16_t led) {
    LEDPORT &= ~(led);
}

void status_led_toggle(uint16_t led) {
    LEDPORT ^= (led);
}

void blink_led(uint16_t led, uint16_t time) {
    if(status_led_status(led)) {
        status_led_off(led);
        evq_timed_push(status_led_on, led, time);
    } else {
        status_led_on(led);
        evq_timed_push(status_led_off, led, time);
    }
}

uint8_t status_led_status(uint8_t led) {
    return LEDPORT & (uint8_t)(led);
}
