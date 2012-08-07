/*
 * mode.c
 *
 *  Created on: 3.8.2012
 *      Author: Tuomas
 */

#include "clock-display.h"
#include "mode.h"
#include <avr/io.h>

char display_mode;
volatile unsigned int mode_timeout;

void set_mode_timeout(unsigned int ms) {
    mode_timeout = ms / 5;
}

unsigned int get_mode_timeout(void) {
    unsigned int save = mode_timeout;
    mode_timeout = 0;
    return save;
}

/* display */
void set_mode(char mode) {
    if(mode >= 1 && mode <= 4) {
        display_mode = mode;
    } else {
        display_mode = DISP_MODE_VOLTAGE;
    }
}

char get_mode() {
    return display_mode;
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

void zero_output_over_current(void) {
    current_limit_status = (current_limit_status << 1) + 1;
    OCR1A = 0;
}
