/*
 * peripherals.h
 *
 *  Created on: 25.6.2012
 *      Author: Tuomas
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
volatile float adc_reference;
volatile unsigned int cur_avg_calculated; // mA
void calc_current_average(void);
void init_adc();

/* ENCODERS */
#define STARTUP_VOLTAGE 150

void init_encoders(void);

/* TIMERS */

    /* voltage */
    volatile unsigned int voltage;
    void init_16_bit_pwm(void);
    void set_voltage(unsigned int set_voltage);
    unsigned int get_voltage();

    /* delay timer, TIMER0B */
    void init_delay_timer(void);

/* SPI */
#define spi_begin() PORTB &= ~(_BV(PB2));
#define spi_end() PORTB |= (_BV(PB2));

volatile char sending_word;
void spi_send(char cData);
void spi_send_word(uint16_t word);
void spi_init(void);

#endif /* PERIPHERALS_H_ */
