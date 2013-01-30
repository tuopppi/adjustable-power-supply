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
#include "eventqueue.h"

void initialize(void) {
    init_evq_timer();

    set_current_limit(read_eeprom_current_limit());
    init_voltage_pwm();
    init_display();
    init_controls();
    init_adc();

}

int main(void) {
    initialize();

    set_dynamic_readout(get_voltage());
    status_led_on(LED_VOLTAGE);

    sei(); // enable interrupts
    while (1) {

        event* ep = evq_front();
        if(ep != 0) {
            if(ep->callback) {
                ep->callback(ep->data);
                evq_pop();
            }
        }

    }

    return 1;
}
