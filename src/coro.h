/** \file coro.h
 *
 * Interrupt-safe stackless coroutines
 */
/* Copyright 2018 Gaurav Juvekar */
#ifndef CORO_H
#define CORO_H 1

#include <stddef.h>
#include <stdbool.h>
#include "../lib/aint-safe/src/nested_queue.h"
#include "timer_interface.h"
#include "resource.h"

_Static_assert(
        __GNUC__,
        "gcc is required with support for computed goto's and labels as values");

typedef enum {
    CORO_STATUS_FINALIZE,
    CORO_STATUS_SUSPENDED,
    CORO_STATUS_WAIT_TIMED,
    CORO_STATUS_WAIT_CONDITION,
    CORO_STATUS_WAIT_RESOURCE,
    CORO_STATUS_WAIT_SUBCORO,
} CoroStatus;


/* Forward declaration */
typedef struct CoroState CoroState;


/** \brief Function type for a coroutine */
typedef void coroutine(CoroState *state, void *vars);


/** \brief Internal state of each coroutine
 *
 * \note All instances of this struct should be treated as private. Use only
 * the functions exposed in this library to access this struct.
 */
struct CoroState {
    /** Pointer to the next label to resume from (computed goto) or NULL */
    void *label;
    /** Pointer to the function specific variables */
    void *vars;
    /** The actual function to call */
    coroutine *const func;
    /** The process state */
    CoroStatus status;
    /** Whether a timeout is set */
    bool timed_wait;
    /** The timer instance if a timeout is set */
    Timer timeout;
    union {
        Condition *condition;
        struct {
            Resource *     resource;
            ResourceOwner *owner;
        } resource;
        CoroState *sub_coroutine;
    } wait;
};


/** \brief A queue at a single priority
 *
 * \note Use #CORO_QUEUE_STATIC_INIT to initialize
 */
typedef NestedQueue CoroScheduleQueue;

/** \brief Statically initialize a #CoroScheduleQueue
 *
 * Use this macro to initialize a #CoroScheduleQueue at definition.
 *
 * \note You need to \e tentatively \e define (search what a C tentative
 * definition is) the #CoroScheduleQueue first.
 *
 * \param p_queue the \e tentatively \e defined #CoroScheduleQueue to
 *                       initialize
 * \param p_n_elems      number of elements in \p data
 * \param p_data_array   data array (of #CoroState) of length \p p_n_elems
 *
 * \return A #CoroScheduleQueue static initialiizer
 *
 * \code{.c}
 * static CoroState priority_1[10];
 * static CoroScheduleQueue the_queue;
 * static CoroScheduleQueue the_queue = CORO_QUEUE_STATIC_INIT(
 *     the_queue, 10, priority_1);
 * \endcode
 */
#define CORO_QUEUE_STATIC_INIT(p_queue, p_n_elems, p_data_array) \
    NESTED_QUEUE_STATIC_INIT(p_queue,                            \
                             sizeof(CoroState),                         \
                             p_n_elems,                                 \
                             p_data_array,                              \
                             NESTED_QUEUE_OPERATION_ORDER_NESTED,       \
                             NESTED_QUEUE_OPERATION_ORDER_FCFS)

/** \brief Collection of priority queues to schedule tasks from
 */
typedef struct {
    CoroScheduleQueue *const *const queues;
    const size_t n_priorities;
} CoroSchedule;



/** \brief Start the main loop that executes coroutines
 *
 * \param schedule A pre-initialized #CoroSchedule
 *
 * \note \p schedule may have existing entries
 *
 * \return This function does not return
 */
void __attribute__((noreturn)) schedule_mainloop(CoroSchedule *schedule);


/** \brief Add a new coroutine to the schedule queue
 *
 * \param schedule the schedule to add the task to
 * \param function the coroutine to execute
 * \param vars     function specific structure to variables
 * \param priority the queue priority (0 to \p schedule->n_priorities -1)
 *
 * \return Pointer to internal state of the created coroutine
 * \retval NULL if creating the coroutine failed
 */
CoroState *Coro_add_new(CoroSchedule *schedule,
                        coroutine *   function,
                        void *        vars,
                        int           priority);

#define JOIN(a, b) a##b
#define JOIN1(a, b) JOIN(a, b)
#define JOIN2(a, b) JOIN1(a, b)
#define CORO_LINE_LABEL() JOIN1(coroutine_state_, __LINE__)
#define CORO_INLINE_SAVE_STATE_EXPLICIT(state) \
    { state->label = &&CORO_LINE_LABEL(); }

#define CORO_SAVE_STATE_EXPLICIT(state)         \
    {                                           \
        CORO_INLINE_SAVE_STATE_EXPLICIT(state); \
        CORO_LINE_LABEL() :;                    \
    }

#define CORO_INIT_EXPLICIT(state)                          \
    {                                                      \
        if (state->label != NULL) { goto * state->label; } \
    }


#define CORO_YIELD_EXPLICIT(state)              \
    {                                           \
        CORO_INLINE_SAVE_STATE_EXPLICIT(state); \
        state->status = CORO_SUSPENDED;         \
        return;                                 \
        CORO_LINE_LABEL() :;                    \
    }


#define CORO_AWAIT_CONDITION_EXPLICIT(state, condition_ptr)
#define CORO_AWAIT_CONDITION_TIMED_EXPLICIT(state, condition, milliseconds)


void func(CoroState *state, void *vars) {
    CORO_INIT_EXPLICIT(state);

    CORO_SAVE_STATE_EXPLICIT(state);

    int  a  = 1;
    CORO_SAVE_STATE_EXPLICIT(state);
    CORO_YIELD_EXPLICIT(state);


    int b = 2;
    CORO_SAVE_STATE_EXPLICIT(state);
}


#endif /* ifndef CORO_H */
