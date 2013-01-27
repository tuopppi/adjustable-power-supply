/*
 * peripherals.c
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply project.
 */

#include "peripherals.h"
#include "mode.h"
#include "display.h"
#include <avr/interrupt.h>
#include <inttypes.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/eeprom.h>
#include "eventqueue.h"

/* PWM */
uint16_t voltage;

void init_voltage_pwm(void) {
    // Waveform outputs
    DDRB |= _BV(PB1);

    /*
     * WGM10 0 (TCCR1A)
     * WGM11 1 (TCCR1A)
     * WGM12 1 (TCCR1B)
     * WGM13 1 (TCCR1B)

     * Timer/Counter Mode of Operation = Fast PWM
     * TOP = ICR1
     * Update of OCR1x at BOTTOM
     * TOV1 Flag Set on TOP
     */
    TCCR1A |= _BV(COM1A1) | _BV(WGM11);
    TCCR1B |= _BV(WGM12) | _BV(WGM13);

    ICR1 = 1000;
    set_voltage(read_eeprom_voltage());

    // start
    TCCR1B |= _BV(CS10);
}

void set_voltage(uint16_t set_voltage) {
    unsigned int uplimit = 1060;
    unsigned int downlimit = 125;

    if(set_voltage > downlimit && set_voltage <= uplimit) {
        OCR1A = set_voltage - 125;
        voltage = set_voltage;
    } else if (set_voltage > uplimit) {
        OCR1A = uplimit - 125;
        voltage = uplimit;
    } else {
        OCR1A = 0;
        voltage = downlimit;
    }
}

uint16_t* get_voltage() {
    return &voltage;
}

/* ADC */

#define ADCREF11 1100
#define ADCREFVCC 5000
#define ADCREFINIT ADCREF11

volatile unsigned int current; // adc values (0-1023)
volatile unsigned int adc_reference;

void init_adc(void) {
    // 1.1V with external capacitor at AREF pin
    // select ADC0
    ADMUX |= _BV(REFS0) | _BV(REFS1);
    // ADC-clk = 1MHz / 128 = 7812Hz
    ADCSRA |= _BV(ADEN) | _BV(ADSC) | _BV(ADIE) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);

    adc_reference = ADCREFINIT;
    current = 0;

    ADCSRA |= _BV(ADSC); // start new conversion
}

/*
 * Gain = 13
 * Rsense = 0.22ohm
 * Vin = ADC * 1024 / Vref
 * current = Vin / Gain / Rsense
 */
void current_handeler(uint16_t current) {
    uint32_t scale = (uint32_t)current * adc_reference * 100 / 1024 / 13 / 22;
    current = (uint16_t)scale;

    if(current > *get_current_limit()) {
        limit_current();
    } else {
        set_voltage(*get_voltage());
    }

    // determines how often display is updated
    // smaller value means faster update
    static uint16_t display_update = 0;
    if(display_update > 5000) {
        display_current = current;
        display_update = 0;
    }
    display_update++;

    // select Vref of ADC
    // 1.1V gives better resolution in lower currents (1mA)
    // 5V reference gives resolution of 4mA
    if(adc_reference < ADCREFVCC && current > 490) {
        adc_reference = ADCREFVCC;
        ADMUX &= ~(_BV(REFS1));
    } else if ( adc_reference > ADCREF11  && current < 470) {
        adc_reference = ADCREF11;
        ADMUX |= _BV(REFS1);
    }

}

ISR(ADC_vect) {
    /* ADC Conversion finished
     * Result is saved to ADC register
     */
    static float adc_reference_prev = ADCREFINIT;
    // if reference changes, discard adc result
    if(adc_reference_prev == adc_reference) {
        current = ADC;
    } else {
        adc_reference_prev = adc_reference;
    }

    evq_push(current_handeler, current);

    ADCSRA |= _BV(ADSC); // start new conversion
}

/* EEPROM */
uint16_t EEMEM eeprom_voltage = 125;
uint16_t EEMEM eeprom_current_limit = 200;

void save_eeprom_current_limit(uint16_t current) {
    eeprom_update_word(&eeprom_current_limit, *get_current_limit());
    blink_led(LED_CURRENT, 200);
}

void save_eeprom_voltage(uint16_t voltage) {
    eeprom_update_word(&eeprom_voltage, *get_voltage());
    blink_led(LED_VOLTAGE, 200);
}

uint16_t read_eeprom_current_limit(void) {
    return eeprom_read_word(&eeprom_current_limit);
}

uint16_t read_eeprom_voltage(void) {
    return eeprom_read_word(&eeprom_voltage);
}

/* SPI */

#define spi_begin() PORTC &= ~(_BV(PC5));
#define spi_end() PORTC |= (_BV(PC5));

volatile uint16_t spi_data_word;

/*
 * MOSI - PB3
 * SCK  - PB5
 * RCK  - PC5
 */
void init_spi(void) {
    // SS (PB2) pitää olla output SPI väylän oikean toiminnan varmistamiseksi
    DDRB |= _BV(DDB2) | _BV(DDB3) | _BV(DDB5); // SS, MOSI, SCK | output
    DDRC |= _BV(DDC5);
    SPCR |= _BV(SPE) | _BV(SPIE) | _BV(MSTR) | _BV(CPOL) | _BV(DORD);
    SPSR |= _BV(SPI2X);
    spi_data_word = 0;
}

void spi_send_word(uint16_t word) {
    loop_until_bit_is_clear(SPSR, SPIF);
    spi_begin();
    spi_data_word = word;
    // LSB first
    SPDR = (0x00FF & spi_data_word);
}

ISR(SPI_STC_vect) {
    if (spi_data_word > 0) {
        // there's still data, send MSB
        SPDR = (spi_data_word >> 8);
        spi_data_word = 0;
    } else {
        spi_end(); // done
    }
}
