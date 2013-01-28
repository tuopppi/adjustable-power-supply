/*
 * mode.c
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply project.
 */

#include "display.h"
#include "mode.h"
#include <avr/io.h>
#include <inttypes.h>

/* limits */
uint16_t current_limit; // mA

void set_current_limit(uint16_t limit) {
    if(limit >= 10 && limit < 3000) {
        current_limit = limit;
    } else if (limit < 10) {
        current_limit = 10;
    } else {
        current_limit = 2999;
    }
}

uint16_t* get_current_limit(void) {
    return &current_limit;
}

void limit_current(void) {
    current_limit_status = (current_limit_status << 1) + 1;

    // set PWM output => 0
    OCR1A = 0;
}
