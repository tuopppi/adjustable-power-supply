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
#include "eventqueue.h"

void initialize(void) {
    set_mode(DISP_MODE_VOLTAGE);

    set_current_limit(read_eeprom_current_limit());

    init_voltage_pwm();
    init_display();
    init_encoders();
    init_adc();
}

int main(void) {
    initialize();
    sei();

    while (1) {

        event* ep = evq_front();
        if(ep != 0) {
            if(ep->callback) {
                ep->callback(ep->data);
            }
            evq_pop();
        }

        // result is saved to (unsigned int) measured_current
        // measure_current();
        // if(get_current_limit() > measured_current) {
        //    set_voltage(get_voltage());
        // } else {
        //     limit_current();
        // }

    }

    return 1;
}
