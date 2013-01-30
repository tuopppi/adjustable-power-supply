/*
 * peripherals.h
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply-oshw-project.
 */

#ifndef PERIPHERALS_H_
#define PERIPHERALS_H_

#include <inttypes.h>

/* ADC  --------------------------------------------------------------------- */
void init_adc();
void current_handeler(uint16_t current);
uint16_t* get_current();

/* EEPROM ------------------------------------------------------------------- */
void save_eeprom_current_limit(uint16_t current);
void save_eeprom_voltage(uint16_t voltage);
uint16_t read_eeprom_current_limit(void);
uint16_t read_eeprom_voltage(void);

/* Voltage PWM  ------------------------------------------------------------- */
void init_voltage_pwm(void);
void set_voltage(uint16_t set_voltage);
uint16_t* get_voltage();

/* limits */
void set_current_limit(uint16_t limit);
uint16_t* get_current_limit(void);
void limit_current(void);

/* SPI  ------------------------------------------------------------------------
 * used to communicate with two 74HC595 sift registers
 */
void init_spi(void);
void spi_send_word(uint16_t word);

#endif /* PERIPHERALS_H_ */
