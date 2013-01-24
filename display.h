/*
 * display.h
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply project.
 */

#ifndef CLOCK_DISPLAY_H_
#define CLOCK_DISPLAY_H_

#include <inttypes.h>
#include "mode.h"

/* This function should be called at program startup */
void init_display(void);

uint16_t DISPLAY_CUR;

void set_display_readout(uint16_t* readout);

void display_blink(char bool);
void display_dots(void);

#endif /* CLOCK_DISPLAY_H_ */
