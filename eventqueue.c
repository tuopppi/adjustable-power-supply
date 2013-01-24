#include <stdint.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "eventqueue.h"

// FIFO - ring buffer
#define BUFMAX 128
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

