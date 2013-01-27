/*
 * display.h
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply project.
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <inttypes.h>
#include <avr/io.h>
#include "mode.h"

enum special_display {
    DISPLAY_CUR = 3000
};

/* This function should be called at program startup */
void init_display(void);

void set_dynamic_readout(uint16_t* readout);
void set_static_readout(uint16_t readout);

void display_dots(void);

#define LED_VOLTAGE (_BV(PD0))
#define LED_CURRENT (_BV(PD1))

void status_led_on(uint16_t led);
void status_led_off(uint16_t led);
void status_led_toggle(uint16_t led);
void blink_led(uint16_t led, uint16_t time);

#endif /* DISPLAY_H_ */
