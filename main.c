/*
 * main.c
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply-oshw-project.
 *
 * This program free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <avr/interrupt.h>
#include "peripherals.h"
#include "clock-display.h"
#include "mode.h"

int main(void) {
    init_io();

    set_mode(DISP_MODE_VOLTAGE);
    set_current_limit(read_eeprom_current_limit());

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
