/*
 * eventqueue.h
 *
 * Author: Tuomas Vaherkoski <tuomasvaherkoski@gmail.com>
 *
 * This file is part of variable-power-supply project.
 */

#ifndef EVENTQUEUE
#define EVENTQUEUE

typedef struct event {
    void (*callback)(uint16_t data);
    uint16_t data;
} event;

/* Adds a new element at the end of the queue 
 * Returns 1 on success and -1 on failure 
 */
int8_t evq_push(void (*callback)(uint16_t), uint16_t data);

/* Returns a pointer to the next event in the queue */
event* evq_front();

/* Removes the next element in the queue */
void evq_pop();

void init_evq_timer(void);

void evq_timed_push(void (*callback)(uint16_t),
                    uint16_t data,
                    uint16_t waitms);

#endif
