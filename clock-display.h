/*
 * clock-display.h
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
 * Numerot esitetään muodossa, jossa
 * 1234 tarkoittaa 12.34
 *
 */

#ifndef CLOCK_DISPLAY_H_
#define CLOCK_DISPLAY_H_

#include "mode.h"

/* This function should be called at program startup */
void display_init(void);

unsigned int get_display_readout(void);
void toggle_dots(void);
void display_blink(char bool);

#endif /* CLOCK_DISPLAY_H_ */
