/*
 * mode.c
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

#include "clock-display.h"
#include "mode.h"
#include <avr/io.h>

char display_mode;
char prev_display_mode;

/* display */
void set_mode(char new_mode) {
    if(new_mode >= 1 && new_mode <= 4 && new_mode != display_mode) {
        prev_display_mode = display_mode;
        display_mode = new_mode;
    }
}

char get_mode() {
    return display_mode;
}

void return_previous_mode(void) {
    set_mode(prev_display_mode);
}

/* limits */
volatile unsigned int current_limit; // mA

void set_current_limit(unsigned int limit) {
    if(limit >= 10 && limit < 3000) {
        current_limit = limit;
    } else if (limit < 10) {
        current_limit = 10;
    } else {
        current_limit = 2999;
    }
}

unsigned int get_current_limit(void) {
    return current_limit;
}

static unsigned int current_limit_status = 0;
unsigned int in_current_limit_mode() {
    if(OCR1A > 0) {
        current_limit_status <<= 1;
    }
    return current_limit_status;
}

void limit_current(void) {
    current_limit_status = (current_limit_status << 1) + 1;
    OCR1A = 0;
}
