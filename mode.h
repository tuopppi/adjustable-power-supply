/*
 * mode.h
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply-oshw-project.
 *
 * This program free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MODE_H_
#define MODE_H_

/* display modes */
typedef enum {
    DISP_MODE_VOLTAGE,
    DISP_MODE_CURRENT,
    DISP_MODE_CURRENT_SET,
    DISP_MODE_POWER,
} State;

void return_previous_mode(void);
void set_mode(State mode);
State get_mode(void);

/* limits */
void set_current_limit(unsigned int limit);
unsigned int get_current_limit(void);

void limit_current(void);
unsigned int in_current_limit_mode();

#endif /* MODE_H_ */
