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

/* IO */
void init_io(void) {
    DDRC |= _BV(DDC1); // constant current sink control signal, output
}

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

#define AVERAGES 10
volatile unsigned int cur_avg[AVERAGES]; // adc values (0-1023)
volatile unsigned int adc_reference;

void init_adc(void) {
    // 1.1V with external capacitor at AREF pin
    // select ADC3
    ADMUX |= _BV(REFS0) | _BV(REFS1) | _BV(MUX0) | _BV(MUX1);
    // ADC-clk = 1MHz / 128 = 7812Hz
    ADCSRA |= _BV(ADEN) | _BV(ADSC) | _BV(ADIE) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);

    int i = 0;
    for(; i<AVERAGES;i++) {
        cur_avg[i] = 0;
    }
    measured_current = 0;
    adc_reference = ADCREFINIT;
}

/*
 * Gain = 10
 * Sense resistor = 0.22ohm
 */
void measure_current(void) {
    unsigned long avg = 0;
    unsigned int i = 0;

    for(;i<AVERAGES;i++) {
        avg += (cur_avg[i]);
    }

    // tweak last multiplier to show correct current...
    // ((x*1100)/1024))/(0.25*10.08)
    avg = ((avg/AVERAGES) * adc_reference) / 1024;
    measured_current = (421277 * avg) / 1000000;

    // determines how often display is updated
    // smaller value means faster update
    static uint16_t display_update = 0;
    if(display_update > 600) {
        display_current = measured_current;
        display_update = 0;
    }
    display_update++;

    // select Vref of ADC
    // 1.1V gives better resolution in lower currents (1mA)
    // 5V reference gives resolution of 4mA
    if(adc_reference < ADCREFVCC && measured_current > 450) {
        adc_reference = ADCREFVCC;
        ADMUX &= ~(_BV(REFS1));
    } else if ( adc_reference > ADCREF11  && measured_current < 430) {
        adc_reference = ADCREF11;
        ADMUX |= _BV(REFS1);
    }

}

ISR(ADC_vect) {
    static unsigned int curIndx = 0;
    static float adc_reference_prev = ADCREFINIT;
    // if reference changes, discard adc result
    if(adc_reference_prev == adc_reference) {
        cur_avg[curIndx] = ADC;
        if(curIndx < AVERAGES - 1) {
            curIndx++;
        } else {
            curIndx = 0;
        }
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

/*
 * ENCODER 1 (oikea)
 * PB6 - ENC1 B
 * PB7 - ENC1 A
 * PD2 - ENC1 SW
 *
 * ENCODER 2 (vasen)
 *
 * PD5 - ENC2 B
 * PD6 - ENC2 A
 * PD3 - ENC2 SW
 *
 */
void init_encoders(void) {
    // pull-ups
    PORTB |= _BV(PORTB6) | _BV(PORTB7);
    PORTD |= _BV(PORTD2) | _BV(PORTD3) | _BV(PORTD5) | _BV(PORTD6);
    // Pin change interrupts
    PCMSK0 |= _BV(PCINT6) | _BV(PCINT7);
    PCMSK2 |= _BV(PCINT21) | _BV(PCINT22);
    PCICR |= _BV(PCIE0) | _BV(PCIE2);
    // ext interrupts (INT0, INT1), falling edge
    EICRA |= _BV(ISC11) | _BV(ISC01);
    EIMSK |= _BV(INT0) | _BV(INT1);
}

#define enc1_phase() ((PINB & (_BV(PINB7) | _BV(PINB6))) >> 6)
#define enc1_SW() (PIND & _BV(PIND2))
#define enc2_phase() ((PIND & (_BV(PIND5) | _BV(PIND6))) >> 5)
#define enc2_SW() (PIND & _BV(PIND3))

/* http://hifiduino.wordpress.com/2010/10/20/rotaryencoder-hw-sw-no-debounce */
int8_t read_encoder(int id) {
    // tarkista validi id
    if (id >= 2) {
        return 0;
    }
    int8_t enc_states[] = { 0, 1, -1, 0, -1, 0, 0, 1, 1, 0, 0, -1, 0, -1, 1, 0 };
    static uint8_t old_AB[2] = { 0, 0 };
    /**/
    old_AB[id] <<= 2; //remember previous state
    if (id == 0) {
        old_AB[id] |= enc1_phase(); //add current state
    } else if (id == 1) {
        old_AB[id] |= enc2_phase(); //add current state
    }
    return (enc_states[(old_AB[id] & 0x0f)]);
}

// ENCODER 1, voltage
ISR(PCINT0_vect) {
    set_voltage(get_voltage() + read_encoder(0));
    add_job(10000, &save_eeprom_voltage, 1);
}

// ENCODER 1 SWITCH
ISR(INT0_vect) {
    set_mode(DISP_MODE_VOLTAGE);
}

// ENCODER 2, current limit
ISR(PCINT2_vect) {
    set_current_limit(get_current_limit() + (5 * read_encoder(1)) / 2);
    add_job(2000, &return_previous_mode, 1);
    add_job(10000, &save_eeprom_current_limit, 1);
    set_mode(DISP_MODE_CURRENT_SET);
}

// ENCODER 2 SWITCH
ISR(INT1_vect) {
    set_mode(DISP_MODE_CURRENT);
}

/* SPI */

#define spi_begin() PORTB &= ~(_BV(PB2));
#define spi_end() PORTB |= (_BV(PB2));

volatile char sending_word;

/*
 * SS   PB2
 * MOSI PB3
 * SCK  PB5
 */
void init_spi(void) {
    DDRB |= _BV(DDB2) | _BV(DDB3) | _BV(DDB5); // SS, MOSI, SCK | output
    SPCR |= _BV(SPE) | _BV(SPIE) | _BV(MSTR) | _BV(CPOL) | _BV(DORD);
    SPSR |= _BV(SPI2X);
    sending_word = 0;
}

void spi_send(uint8_t cData) {
    spi_begin();
    SPDR = cData;
}

void spi_send_word(uint16_t word) {
    spi_begin();
    sending_word = 1;
    // LSB first
    SPDR = (0x00FF & word);
    while (sending_word); // SPI interrupt decrements
    SPDR = (word >> 8);
}

ISR(SPI_STC_vect) {
    if (sending_word > 0) {
        sending_word--;
    } else {
        spi_end();
    }
}
