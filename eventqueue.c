/*
 * eventqueue.c
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply project.
 */

#include <stdint.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "eventqueue.h"
#include "display.h"

// FIFO - ring buffer
#define BUFMAX 64
uint8_t events_ = 0;
event ebuf_[BUFMAX];
event *first_ = ebuf_;
event *last_ = ebuf_;

uint8_t evq_push(void (*callback)(uint16_t), uint16_t data) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if(events_ >= BUFMAX) {
            // buffer is full
            return 0;
        }

        if(last_ >= ebuf_ + BUFMAX) {
            // jumb back to start if over the end of the ring
            last_ = ebuf_;
        }

        event new_event = {callback, data};
        *last_ = new_event;
        last_++;
        events_++;
    }
    return events_;
}

void evq_pop() {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if(events_ > 0) {
            // buffer is not empty
            events_--;
            first_++;
        }

        if(first_ >= ebuf_ + BUFMAX) {
            // jump back to start if over the end of the ring
            first_ = ebuf_;
        }
    }
}

event* evq_front() {
    if(events_ == 0) {
        // buffer is empty
        return 0;
    }
    return first_;
}

/* TIMED EVENTS ------------------------------------------------------------- */

/* Timer will give interrupt every (1) millisecond */
void init_evq_timer(void) {
    TCCR2A |= _BV(WGM21); // CTC
    OCR2A = 8; // ~1ms
    TCCR2B |= _BV(CS22) | _BV(CS21) | _BV(CS20); // clk/1024
    TIMSK2 |= _BV(OCIE2A);
}

typedef struct {
    event data;
    uint16_t timer;
} timed_event;

#define TIMED_BUFMAX 32
timed_event timed_ebuf_[TIMED_BUFMAX];
uint8_t timed_events_ = 0;

/* Implements small hash table for timed callback events
 * waitms is time in milliseconds after callback function is called.
 *
 * If user pushes event with same callback and data values the old one is
 * overwritten.
 */
void evq_timed_push(void (*callback)(uint16_t),
                    uint16_t data,
                    uint16_t waitms)
{
    timed_event timed_ev = {{callback, data}, waitms};

    uint8_t hash = ((uint16_t)callback+data) % TIMED_BUFMAX;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        timed_ebuf_[hash] = timed_ev;
        // TODO: etsi vapaa kohta
        // timed_ebuf_[hash].data.callback == callback
    }
}

/* Loops through hash table and pushes events which timer has reached zero
 * to event queue.
 *
 * TIMER2 ISR calls this function every 1 millisecond
 */
void evq_timer_tick() {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    for(int idx = 0; idx < TIMED_BUFMAX; idx++) {
        if(--timed_ebuf_[idx].timer == 0 &&
             timed_ebuf_[idx].data.callback != 0) {
            if(evq_push(timed_ebuf_[idx].data.callback,
                        timed_ebuf_[idx].data.data)) {
                timed_ebuf_[idx].data.callback = 0; // mark as done
            } else {
                // there is no space in evq, try again next round
                timed_ebuf_[idx].timer++;
            }
        }
    }
    }
}

ISR(TIMER2_COMPA_vect) {
    // tick 1ms intervals
    evq_timer_tick();
}
