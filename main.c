/*
 * main.c
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply project.
 */

#include <avr/interrupt.h>
#include "peripherals.h"
#include "controls.h"
#include "display.h"
#include "mode.h"
#include "eventqueue.h"

void initialize(void) {
    set_current_limit(read_eeprom_current_limit());
    init_voltage_pwm();
    init_display();
    init_controls();
    init_adc();
    init_evq_timer();
}

int main(void) {
    initialize();
    sei();

    while (1) {

        event* ep = evq_front();
        if(ep != 0) {
            ep->callback(ep->data);
            evq_pop();
        }

    }

    return 1;
}
