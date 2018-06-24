/** \file coro.c
 *
 * Interrupt-safe stackless coroutines
 */
/* Copyright 2018 Gaurav Juvekar */

#include "coro.h"
#include <assert.h>
#include <stdbool.h>


static void execute(CoroState *state);
static void execute_once(CoroState *state);


typedef int funcVars;
void func(CoroState *state, void *vars) {
    CORO_INIT(func);

    CORO_YIELD();

    int  a  = 1;
    CORO_YIELD();
    CORO_YIELD();


    int b = 2;
    CORO_YIELD();
}



static void execute(CoroState *state) {
    if (state->timed_wait) {
        Timer_cancel(&state->timeout);
        state->timed_wait = false;
    }
    state->status = CORO_STATUS_FINALIZE;
    state->func(state, state->vars);
}


static void execute_once(CoroState *state) {
    if (state->timed_wait && Condition_get(&state->timeout.timed_out)) {
        execute(state);
    } else {
        switch (state->status) {
        case CORO_STATUS_FINALIZE: break;
        case CORO_STATUS_SUSPENDED: execute(state); break;
        case CORO_STATUS_WAIT_TIMED: break;
        case CORO_STATUS_WAIT_CONDITION:
            if (Condition_get(state->wait.condition)) { execute(state); };
            break;
        case CORO_STATUS_WAIT_RESOURCE:
            if ((state->wait.resource.retval =
                         Resource_acquire(state->wait.resource.resource,
                                          state->wait.resource.owner))
                != RESOURCE_ACQUIRE_FAILED) {
                execute(state);
            }
            break;
        case CORO_STATUS_WAIT_SUBCORO:
            if (state->wait.sub_coroutine->status == CORO_STATUS_FINALIZE) {
                execute(state);
            } else {
                execute_once(state->wait.sub_coroutine);
            }
            break;
        }
    }
}


void __attribute__((noreturn)) schedule_mainloop(CoroSchedule *schedule) {
    do {
        for (size_t i = 0; i < schedule->n_priorities; i++) {
            /* For every priority queue */
            CoroScheduleQueue *queue = schedule->queues[i];
            /* Acquire all readable elements */
            while (NestedQueue_read_acquire(queue) != NULL)
                ;

            /* And iterate over them all */
            NestedQueueIterator iter  = NestedQueueIterator_init_read(queue);
            CoroState *         state = NULL;
            for (state = NestedQueueIterator_next(&iter); state != NULL;
                 state = NestedQueueIterator_next(&iter)) {
                execute_once(state);
                if (state->status == CORO_STATUS_FINALIZE) {
                    /* will silently fail if not the oldest inserted
                     * state */
                    NestedQueue_read_release(queue, state);
                }
            }
        }
    } while (true);
}
