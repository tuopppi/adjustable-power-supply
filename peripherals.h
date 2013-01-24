/*
 * peripherals.h
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

#ifndef PERIPHERALS_H_
#define PERIPHERALS_H_

#include <inttypes.h>

/* ADC  --------------------------------------------------------------------- */
uint16_t display_current;
void init_adc();
void current_handeler(uint16_t current);

/* EEPROM ------------------------------------------------------------------- */
void save_eeprom_current_limit(uint16_t current);
void save_eeprom_voltage(uint16_t voltage);
uint16_t read_eeprom_current_limit(void);
uint16_t read_eeprom_voltage(void);

/* Voltage PWM  ------------------------------------------------------------- */
void init_voltage_pwm(void);
void set_voltage(uint16_t set_voltage);
uint16_t* get_voltage();


/* SPI  ------------------------------------------------------------------------
 * used to communicate with two 74HC595 sift registers
 */
void init_spi(void);
void spi_send(uint8_t cData);
void spi_send_word(uint16_t word);

#endif /* PERIPHERALS_H_ */
