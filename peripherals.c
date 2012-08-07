/*
 * peripherals.c
 *
 *  Created on: 25.6.2012
 *      Author: Tuomas
 */

#include "peripherals.h"
#include "mode.h"
#include "clock-display.h"
#include <avr/interrupt.h>
#include <inttypes.h>
#include <avr/io.h>

/* IO */

// TODO: TARKISTA...
void init_io(void) {
   // DDRC |= _BV(DDC2); // OUTPUT ENABLE (alhaalla aktiivinen)
   // enable_output();
    DDRC |= _BV(DDC1); // mini load output
}

/* PWM */
void init_16_bit_pwm(void) {
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
    set_voltage(STARTUP_VOLTAGE);

    // start
    TCCR1B |= _BV(CS10);
}

void set_voltage(unsigned int set_voltage) {
    unsigned int uplimit = 1060;
    unsigned int downlimit = 125;

    if(set_voltage > downlimit && set_voltage <= uplimit) {
        OCR1A = set_voltage - 125 + 4;
        voltage = set_voltage;
    } else if (set_voltage > uplimit) {
        OCR1A = uplimit - 125;
        voltage = uplimit;
    } else {
        OCR1A = 0;
        voltage = downlimit;
    }
}

unsigned int get_voltage() {
    return voltage;
}

/* ADC */

#define AVERAGES 10
volatile unsigned int current_in; // adc value (0-1024)
volatile unsigned int cur_avg[AVERAGES]; // adc values (0-1024)


void init_adc(void) {
    // AVCC with external capacitor at AREF pin
    // select ADC3
    ADMUX |= _BV(REFS0) | _BV(MUX0) | _BV(MUX1);
    // ADC-clk = 1MHz / 8 = 125kHz
    ADCSRA |= _BV(ADEN) | _BV(ADSC) | _BV(ADIE) | _BV(ADPS0) | _BV(ADPS1) | _BV(ADPS2);
    current_in = 0;

    int i = 0;
    for(; i<AVERAGES;i++) {
        cur_avg[i] = 0;
    }
    cur_avg_calculated = 0;
}

/*
 * Vref = 5V
 * Gain = 10
 * Sense resistor = 0.22ohm
 */
void calc_current_average(void) {
    unsigned long avg = 0;
    unsigned int i = 0;
    for(;i<AVERAGES;i++) {
        avg += (cur_avg[i]);
    }
    // tweak last multiplier to show correct current...
    cur_avg_calculated = (((avg/AVERAGES) * 500) / 1024) * 4.1;
}

ISR(ADC_vect) {
    static unsigned int curIndx = 0;
    current_in = ADC;
    cur_avg[curIndx] = current_in;
    if(curIndx < AVERAGES - 1) {
        curIndx++;
    } else {
        curIndx = 0;
    }

    ADCSRA |= _BV(ADSC); // start new conversion
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
    set_voltage(voltage + read_encoder(0));
}

// ENCODER 1 SWITCH
ISR(INT0_vect) {
    set_mode(DISP_MODE_VOLTAGE);
}

// ENCODER 2
ISR(PCINT2_vect) {
    set_current_limit(get_current_limit() + 5 * read_encoder(1));
    set_mode_timeout(2000);
    set_mode(DISP_MODE_CURRENT_SET);
}

// ENCODER 2 SWITCH
ISR(INT1_vect) {
    set_mode(DISP_MODE_CURRENT);
}

/* SPI */

void spi_send(char cData) {
    spi_begin();
    SPDR = cData;
}

void spi_send_word(uint16_t word) {
    spi_begin();
    sending_word = 1;
    // LSB first
    SPDR = (0x00FF & word);
    while (sending_word); // keskeytys nollaa
    SPDR = (word >> 8);
}

/*
 * SS   PB2
 * MOSI PB3
 * SCK  PB5
 */
void spi_init(void) {
    DDRB |= _BV(DDB2) | _BV(DDB3) | _BV(DDB5); // SS, MOSI, SCK | output
    SPCR |= _BV(SPE) | _BV(SPIE) | _BV(MSTR) | _BV(CPOL) | _BV(DORD);
    sending_word = 0;
}

ISR(SPI_STC_vect) {
    if (sending_word > 0) {
        sending_word--;
    } else {
        spi_end();
    }
}
