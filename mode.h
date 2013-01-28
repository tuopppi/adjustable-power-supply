/*
 * mode.h
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply project.
 */

#ifndef MODE_H_
#define MODE_H_

/* limits */
void set_current_limit(uint16_t limit);
uint16_t* get_current_limit(void);
void limit_current(void);

#endif /* MODE_H_ */
