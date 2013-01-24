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

// FIFO - ring buffer
#define BUFMAX 64
uint8_t events_ = 0;
event ebuf_[BUFMAX];
event *first_ = ebuf_;
event *last_ = ebuf_;

int8_t evq_push(void (*callback)(uint16_t), uint16_t data) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if(events_ >= BUFMAX) {
            // buffer is full
            return -1;
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
    return 1;
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

timed_event timed_ebuf_[BUFMAX];
uint8_t timed_events_ = 0;

void evq_timed_push(void (*callback)(uint16_t),
                    uint16_t data,
                    uint16_t waitms) {
    timed_event timed_ev;
    event new_event = {callback, data};
    timed_ev.data = new_event; timed_ev.timer = waitms;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        timed_ebuf_[timed_events_] = timed_ev;
        timed_events_++;
    }
}

void evq_timer_tick() {
    int idx;
    for(idx = 0; idx < timed_events_; idx++) {
        if(--timed_ebuf_[idx].timer == 0) {
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
                evq_push(timed_ebuf_[idx].data.callback,
                         timed_ebuf_[idx].data.data);

                // move last event in buffer over idx
                timed_ebuf_[idx] = timed_ebuf_[timed_events_ - 1];
                timed_events_--;
                idx = 0; // start loop from beginning
            }
        };
    }
}

ISR(TIMER2_COMPA_vect) {
    // tick 1ms intervals
    evq_timer_tick();
}
