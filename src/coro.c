/** \file coro.c
 *
 * Interrupt-safe stackless coroutines
 */
/* Copyright 2018 Gaurav Juvekar */

#include "coro.h"
#include <assert.h>
#include <stdbool.h>



void func(CoroState *state, void *vars) {
    int *v = vars;
    CORO_INIT();

    CORO_YIELD();

    int  a  = 1;
    CORO_YIELD();
    CORO_YIELD();


    int b = 2;
    CORO_YIELD();
}





void __attribute__((noreturn)) schedule_mainloop(CoroSchedule *schedule) {
    for (size_t i = 0; i < schedule->n_priorities; i++) {
        CoroScheduleQueue *queue = schedule->queues[i];
        while (NestedQueue_read_acquire(queue) != NULL) ;

        NestedQueueIterator iter = NestedQueueIterator_init_read(queue);
        CoroState *state = NULL;
        for (state = NestedQueueIterator_next(&iter); state != NULL;
             state = NestedQueueIterator_next(&iter)) {

            // todo check more enum cases
            if (state->status == CORO_STATUS_SUSPENDED) {
                state->status = CORO_STATUS_FINALIZE;
                state->func(state, state->vars);
            }

            if (state->status == CORO_STATUS_FINALIZE) {
                /* will silently fail if not the oldest inserted state */
                NestedQueue_read_release(queue, state);
            }


        }
    }
}
