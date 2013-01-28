/*
 * controls.h
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply project.
 */

#ifndef CONTROLS_H_
#define CONTROLS_H_

/* CONTROLS  ---------------------------------------------------------------- */
void init_controls(void);

void voltage_knob_handler(uint16_t);
void current_knob_handler(uint16_t);
void button_handler(uint16_t);

#endif /* CONTROLS_H_ */
