/*
 * mode.h
 *
 *  Created on: 3.8.2012
 *      Author: Tuomas
 */

#ifndef MODE_H_
#define MODE_H_

/* display */
#define DISP_MODE_VOLTAGE 1
#define DISP_MODE_CURRENT 2
#define DISP_MODE_CURRENT_SET 3
#define DISP_MODE_POWER 4

void return_previous_mode(void);
void set_mode(char mode);
char get_mode(void);

/* limits */
void set_current_limit(unsigned int limit);
unsigned int get_current_limit(void);

void zero_output_over_current(void);
unsigned int in_current_limit_mode();

#endif /* MODE_H_ */
