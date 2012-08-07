/*
 * clokc-display.h
 *
 * Numerot esitetään fixed point muodossa, jossa
 * 1234 tarkoittaa 12.34
 *
 */

#ifndef CLOCK_DISPLAY_H_
#define CLOCK_DISPLAY_H_

#include "mode.h"


void test_function(void);

void display_blink(char bool);
void display_init(void);
unsigned int get_display_readout(void);

#endif /* CLOKC_DISPLAY_H_ */
