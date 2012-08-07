#include <avr/interrupt.h>
#include <avr/io.h>
#include "clock-display.h"
#include "peripherals.h"

volatile char cycle_threshold;
volatile unsigned int seq_nbr;
volatile unsigned int readout;
volatile char show_dots;
volatile char blink;

#define TENS_OFFSET 10
#define HUNDRED_OFFSET 20
#define THOUSAND_OFFSET 30
#define OL 33
#define DOTS 34
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
};

void display_init(void) {
    TCCR0B |= _BV(CS01); // Timer 0 | clk_IO/8 (From prescaler)
    TIMSK0 |= _BV(TOIE0); // interrupt on timer 0 overflow
    seq_nbr = 0;
    cycle_threshold = 0;
    readout = 0;
    show_dots = 0;
    blink = 0;
}

void display_blink(char bool) {
    if(bool > 0) {
        blink = 1;
    }
    else {
        blink = 0;
    }
}

void set_display_readout(unsigned int data) {
    readout = data;
}

unsigned int get_display_readout(void) {
    return readout;
}

void test_function(void) {
    show_dots ^= 1;
}

uint16_t get_readout_segments(unsigned int seq) {
    if (readout < 3000 && readout >= 0) {
        uint16_t segments = 0;
        int thousands = THOUSAND_OFFSET + (readout / 1000);
        int hundreds = HUNDRED_OFFSET + (readout % 1000) / 100;
        int tens = TENS_OFFSET + (readout % 100) / 10;
        int ones = (readout % 10);
        segments = display_data[thousands][seq] | display_data[hundreds][seq]
            | display_data[tens][seq] | display_data[ones][seq];
        if(show_dots) {
            segments |= display_data[DOTS][seq];
        }
        return segments;
    } else {
        return display_data[OL][seq];
    }
}

/*                                      IO_clk
 * treshold_limit =  -------------------------------------------- - 1
 *                    refresh_freq * sequences * prescaler * 255
 */
#define THRESHOLD_LIMIT 10

ISR(TIMER0_OVF_vect, ISR_NOBLOCK) {
    switch(get_mode()) {
    case DISP_MODE_VOLTAGE:
        set_display_readout(voltage);
        break;
    case DISP_MODE_CURRENT:
        set_display_readout(cur_avg_calculated);
        break;
    case DISP_MODE_CURRENT_SET:
        set_display_readout(get_current_limit());
        break;
    default:
        break;
    }

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
}
