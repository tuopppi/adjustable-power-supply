/*
 * peripherals.c
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

#include "peripherals.h"
#include "mode.h"
#include "clock-display.h"
#include <avr/interrupt.h>
#include <inttypes.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/eeprom.h>
#include "eventqueue.h"

/* PWM */
volatile uint16_t voltage;

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
    //set_voltage(read_eeprom_voltage());
    set_voltage(500);

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

uint16_t get_voltage() {
    return voltage;
}

/* TIMER2 */

typedef struct job {
    void (*callback)(void);
    uint16_t timeout;
    void* next;
    void* prev;
} job;

job* first_job_p = NULL;

void init_delay_timer(void) {
    TCCR2A |= _BV(WGM21); // CTC
    OCR2A = 8; // ~1ms
    TCCR2B |= _BV(CS22) | _BV(CS21) | _BV(CS20); // clk/1024
}

uint8_t add_job(uint16_t ms, void (*callback)(void), uint8_t replace) {
    // check if timer is initialized
    if(OCR2A == 0) {
        init_delay_timer();
    }

    job* loop_p;
    if(replace) {
        loop_p = first_job_p;
        while(loop_p != NULL) {
            if(loop_p->callback == callback && loop_p->timeout > 0) {
                loop_p->timeout = ms;
                return 1;
            }
            loop_p = (job*)loop_p->next;
        }
    }

    job* new_job_p = malloc(sizeof(job));
    if(new_job_p != NULL) {
        new_job_p->callback = callback;
        new_job_p->timeout = ms;
        new_job_p->next = NULL;
    } else {
        return 0;
    }

    if(first_job_p == NULL) {
        new_job_p->prev = NULL;
        first_job_p = new_job_p;
    } else {
        loop_p = first_job_p;
        while(loop_p->next != NULL) {
            loop_p = loop_p->next;
        }
        loop_p->next = new_job_p;
        new_job_p->prev = loop_p;
    }

    // enable interrupts
    TIMSK2 |= _BV(OCIE2A);

    return 1;
}

ISR(TIMER2_COMPA_vect) {
    job* loop_p = first_job_p;
    while(loop_p != NULL) {
        // time for callback
        if(loop_p->timeout == 0 && loop_p->callback != NULL) {
            (*loop_p->callback)();
            loop_p->callback = NULL;
            loop_p = (job*)loop_p->next;
        }
        // decrement timeout every millisecond
        else if(loop_p->timeout > 0 && loop_p->callback != NULL) {
            loop_p->timeout--;
            loop_p = (job*)loop_p->next;
        }
        else {
            job* tmp = loop_p;
            // free first
            if(tmp == first_job_p) {
                first_job_p = tmp->next;
                if(tmp->next != NULL) {
                    ((job*)tmp->next)->prev = NULL;
                }
            }
            // free from middle
            else if(tmp->next != NULL && tmp->prev != NULL) {
                ((job*)tmp->prev)->next = tmp->next;
                ((job*)tmp->next)->prev = tmp->prev;
            }
            // free from end
            else {
                ((job*)tmp->prev)->next = NULL;
            }
            loop_p = (job*)tmp->next;
            free(tmp);
        }
    }

    if(first_job_p == NULL) {
        // no interrupts needed
        TIMSK2 &= ~(_BV(OCIE2A));
    }

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

    measured_current = 0;
    adc_reference = ADCREFINIT;
}

/*
 * Gain = 13
 * Rsense = 0.22ohm
 * Vin = ADC * 1024 / Vref
 * current = Vin / Gain / Rsense
 */
void measure_current(void) {
    uint32_t scale = (uint32_t)current * adc_reference * 100 / 1024 / 13 / 22;
    measured_current = (uint16_t)scale;

    // determines how often display is updated
    // smaller value means faster update
    static uint16_t display_update = 0;
    if(display_update > 10000) {
        display_current = measured_current;
        display_update = 0;
    }
    display_update++;

    // select Vref of ADC
    // 1.1V gives better resolution in lower currents (1mA)
    // 5V reference gives resolution of 4mA
    if(adc_reference < ADCREFVCC && measured_current > 490) {
        adc_reference = ADCREFVCC;
        ADMUX &= ~(_BV(REFS1));
    } else if ( adc_reference > ADCREF11  && measured_current < 470) {
        adc_reference = ADCREF11;
        ADMUX |= _BV(REFS1);
    }

}

ISR(ADC_vect) {
    static float adc_reference_prev = ADCREFINIT;
    // if reference changes, discard adc result
    if(adc_reference_prev == adc_reference) {
        current = ADC;
    } else {
        adc_reference_prev = adc_reference;
    }

    ADCSRA |= _BV(ADSC); // start new conversion
}

/* EEPROM */
uint16_t EEMEM eeprom_voltage = 125;
uint16_t EEMEM eeprom_current_limit = 200;
void save_eeprom_current_limit(void) {
    eeprom_update_word(&eeprom_current_limit, get_current_limit());
}
void save_eeprom_voltage(void) {
    eeprom_update_word(&eeprom_voltage, get_voltage());
}
uint16_t read_eeprom_current_limit(void) {
    return eeprom_read_word(&eeprom_current_limit);
}
uint16_t read_eeprom_voltage(void) {
    return eeprom_read_word(&eeprom_voltage);
}

/* ENCODERS */

enum usr_input_events {
    VOLTAGE_LEFT,
    VOLTAGE_RIGHT,
    CURRENT_LEFT,
    CURRENT_RIGHT,
    VOLTAGE_BTN,
    CURRENT_BTN,
    TOP_BTN
};

void usr_input_handler(uint16_t usr_input) {
    switch(usr_input) {
    case VOLTAGE_LEFT:
        break;
    case VOLTAGE_RIGHT:
        break;
    case CURRENT_LEFT:
        break;
    case CURRENT_RIGHT:
        break;
    case VOLTAGE_BTN:
        set_mode(DISP_MODE_VOLTAGE);
        break;
    case CURRENT_BTN:
        set_mode(DISP_MODE_CURRENT);
        break;
    case TOP_BTN:
        set_mode(DISP_MODE_CURRENT);
        set_mode(DISP_MODE_POWER);
        add_job(1000, &return_previous_mode, 1);
        break;
    }
}

/*
 * ENCODERs
 * PB6 - ENC1 A
 * PB7 - ENC2 A
 * PD5 - ENC1 B
 * PD6 - ENC2 B

 *
 * PD4 - TACTILE SW
 * PD3 - ENC SW
 * PD2 - ENC SW
 *
 */
void init_encoders(void) {
    // pull-ups
    PORTB |= _BV(PORTB6) | _BV(PORTB7);
    PORTD |= _BV(PORTD2) | _BV(PORTD3) |_BV(PORTD4) | _BV(PORTD5) | _BV(PORTD6);
    // Pin change interrupts
    PCMSK0 |= _BV(PCINT6) | _BV(PCINT7);
    PCMSK2 |= _BV(PCINT20);
    PCICR |= _BV(PCIE0) | _BV(PCIE2);
    // ext interrupts (INT0, INT1), falling edge
    EICRA |= _BV(ISC11) | _BV(ISC01);
    EIMSK |= _BV(INT0) | _BV(INT1);
}

enum encoderid {
    ENC_CURRENT = 1,
    ENC_VOLTAGE = 2
};

int8_t read_encoder(int id) {
    uint8_t direction = (PIND & (_BV(PIND5) | _BV(PIND6))) >> 5;

    if(id == ENC_CURRENT) {
        if(direction == 1) { return -1; }
        else if(direction == 3) { return 1; }
    }
    else if(id == ENC_VOLTAGE) {
        if(direction == 3) { return -1; }
        else if(direction == 2) { return 1; }
    }

    return 0;
}

ISR(PCINT0_vect) {
    // tarkistetaanko tuliko keskeytys ENCA vai ENCB
    int8_t trigger = (PINB & (_BV(PINB7) | _BV(PINB6))) >> 6;

    if( trigger == ENC_VOLTAGE) {
        set_voltage(get_voltage() + read_encoder(ENC_VOLTAGE) * 5);
        add_job(10000, &save_eeprom_voltage, 1);
    }

    if( trigger == ENC_CURRENT) {
        set_mode(DISP_MODE_CURRENT_SET);
        set_current_limit(get_current_limit() + read_encoder(ENC_CURRENT) * 10);
        add_job(2000, &return_previous_mode, 1);
        add_job(10000, &save_eeprom_current_limit, 1);
    }
}

// ENC_VOLTAGE SWITCH
ISR(INT0_vect) {
    evq_push(usr_input_handler, VOLTAGE_BTN);
}

// ENC_CURRENT SWITCH
ISR(INT1_vect) {
    evq_push(usr_input_handler, CURRENT_BTN);
}

// TACTILE SWITCH
ISR(PCINT2_vect) {
    if(!((PIND & _BV(PIND4)) >> 4)) {
        evq_push(usr_input_handler, TOP_BTN);
    }
}

/* SPI */

#define spi_begin() PORTC &= ~(_BV(PC5));
#define spi_end() PORTC |= (_BV(PC5));

volatile uint16_t spi_data_word;

/*
 * RCK   PC5
 * MOSI PB3
 * SCK  PB5
 */
void init_spi(void) {
    // SS (PB2) pitää olla output SPI väylän oikean toiminnan varmistamiseksi
    DDRB |= _BV(DDB2) | _BV(DDB3) | _BV(DDB5); // SS, MOSI, SCK | output
    DDRC |= _BV(DDC5);
    SPCR |= _BV(SPE) | _BV(SPIE) | _BV(MSTR) | _BV(CPOL) | _BV(DORD);
    SPSR |= _BV(SPI2X);
    spi_data_word = 0;
}

void spi_send(uint8_t cData) {
    spi_begin();
    SPDR = cData;
}

void spi_send_word(uint16_t word) {
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
