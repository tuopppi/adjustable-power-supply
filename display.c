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

volatile char cycle_threshold;
volatile unsigned int seq_nbr;
volatile uint16_t* dynamic_readout_p_;
volatile uint16_t static_readout_;
volatile char show_dots;
volatile char blink;

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
    TCCR0B |= _BV(CS01); // Timer 0 | clk_IO/8 (From prescaler)
    TIMSK0 |= _BV(TOIE0); // interrupt on timer 0 overflow
    seq_nbr = 0;
    cycle_threshold = 0;
    dynamic_readout_p_ = 0;
    show_dots = 0;
    blink = 0;

    DDRD |= _BV(PD0) + _BV(PD1); // LED_VOLTAGE, LED_CURRENT outputs
}

void display_blink(char bool) {
    blink = bool;
}

void set_dynamic_readout(uint16_t* readout) {
    dynamic_readout_p_ = readout;
}

void set_static_readout(uint16_t readout) {
    static_readout_ = readout;
    dynamic_readout_p_ = &static_readout_;
}

void display_dots(void) {
    show_dots ^= 1;
}

uint16_t get_readout_segments(unsigned int seq) {
    /* Display can show numerical values between 0 - 2999 */
    if (*dynamic_readout_p_ < 3000 && *dynamic_readout_p_ >= 0) {
        uint16_t segments = 0;
        int thousands = THOUSAND_OFFSET + (*dynamic_readout_p_ / 1000);
        int hundreds = HUNDRED_OFFSET + (*dynamic_readout_p_ % 1000) / 100;
        int tens = TENS_OFFSET + (*dynamic_readout_p_ % 100) / 10;
        int ones = (*dynamic_readout_p_ % 10);
        segments = display_data[thousands][seq] | display_data[hundreds][seq]
            | display_data[tens][seq] | display_data[ones][seq];
        if(show_dots) {
            segments |= display_data[DOTS][seq];
        }
        return segments;
    } else {
        /* Other values correspond to some special text string */
        switch(*dynamic_readout_p_) {
        case DISPLAY_CUR:
            return display_data[CUR][seq];
        default:
            return display_data[OL][seq];
        }
    }
}


/*                                      IO_clk
 * treshold_limit =  -------------------------------------------- - 1
 *                    refresh_freq * sequences * prescaler * 255
 */
#define THRESHOLD_LIMIT 10


void display_handler(uint16_t null) {
    if (cycle_threshold > THRESHOLD_LIMIT) {
        // Jatketaan vasta kun spi moduuli on vapaa
        loop_until_bit_is_clear(SPSR, SPIF);
        static unsigned int blink_counter = 0;

        // valitaan piirrettävät segmentit
        if(blink && blink_counter % 500 < 100) {
            spi_send_word(0);
        } else {
            spi_send_word(get_readout_segments(seq_nbr % 2));
        }

        blink_counter++;
        seq_nbr++;
        cycle_threshold = 0;
    }
    cycle_threshold++;

    //evq_timed_push(display_handler, 0, 8);
}

ISR(TIMER0_OVF_vect) {
    evq_push(display_handler, 0);
}


/* LEDs --------------------------------------------------------------------- */
#define LEDPORT (PORTD)

void status_led_on(uint8_t led) {
    LEDPORT |= led;
}

void status_led_off(uint8_t led) {
    LEDPORT &= ~(led);
}
