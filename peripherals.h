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

/* IO */
#define enable_output() PORTC &= ~(_BV(PORTC2));
#define disable_output() PORTC |= _BV(PORTC2);
#define enable_miniload() PORTC |= _BV(PORTC1);
#define disable_miniload() PORTC &= ~(_BV(PORTC1));
void init_io(void);

/* ADC */
volatile unsigned int adc_reference;
volatile unsigned int cur_avg_calculated; // mA
void calc_current_average(void);
void init_adc();

/* EEPROM */
void save_eeprom_current_limit(void);
void save_eeprom_voltage(void);
unsigned int read_eeprom_current_limit(void);
unsigned int read_eeprom_voltage(void);

/* ENCODERS */
void init_encoders(void);

/* Voltage PWM, TIMER1 */
volatile unsigned int voltage;
void init_16_bit_pwm(void);
void set_voltage(unsigned int set_voltage);
unsigned int get_voltage();

/* Timer with call back function, TIMER2 */
void init_delay_timer(void);

/* Set timer for callback function
 * returns 1 on success, otherwise 0 */
uint8_t add_timer_callback(uint16_t ms, void (*callback)(void), uint8_t replace);

/*
 * SPI
 *
 * SPI module is used to communicate with two 74HC595 sift registers
 *
 */

void spi_init(void);
void spi_send(char cData);
void spi_send_word(uint16_t word);

#endif /* PERIPHERALS_H_ */
