/*
 * mode.h
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply project.
 */

#ifndef MODE_H_
#define MODE_H_

/* display modes */
enum {
    DISP_MODE_VOLTAGE,
    DISP_MODE_CURRENT,
    DISP_MODE_CURRENT_SET,
    DISP_MODE_POWER,
};

void return_previous_mode(void);
void set_mode(uint16_t mode);
uint16_t get_mode(void);

/* limits */
void set_current_limit(uint16_t limit);
uint16_t* get_current_limit(void);

void limit_current(void);
unsigned int in_current_limit_mode();

#endif /* MODE_H_ */
