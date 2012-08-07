/*
 * main.c
 *
 *  Created on: 7.6.2012
 *      Author: Tuomas
 */

#include <avr/interrupt.h>
#include "peripherals.h"
#include "clock-display.h"
#include "mode.h"

#define TRUE 1
#define FALSE 0

int main(void) {
    init_io();

    set_mode(DISP_MODE_VOLTAGE);
    set_current_limit(200);

    init_16_bit_pwm();

    init_delay_timer();

    spi_init();
    display_init();
    init_encoders();

    init_adc();

    sei();

    while (1) {
        calc_current_average();

        if(cur_avg_calculated < 20) {
            enable_miniload();
        } else {
            disable_miniload();
        }

        if(get_current_limit() > cur_avg_calculated) {
            set_voltage(voltage);
            if(in_current_limit_mode() == 0) {
                display_blink(FALSE);
            }
        } else {
            zero_output_over_current();
            display_blink(TRUE);
        }
    }

    return 1;
}
