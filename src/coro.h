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
    /** Wait type specific data */
    union {
        /** The condition to wait for */
        Condition *condition;
        /** Data specific to wait on a resource */
        struct {
            /** The resource to acquire */
            Resource *     resource;
            /** The owner instance acquiring the resource */
            ResourceOwner *owner;
            /** The return value if the resource is acquired passed to the
             * coroutine */
            RetResource_acquire retval;
        } resource;
        /** The coroutine to wait for
         *
         * This may or may not be in the schedule and have any priority.
         */
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

/** Collection of priority queues to schedule tasks from */
typedef struct {
    /** An array of #CoroScheduleQueue's, one for each priority */
    CoroScheduleQueue *const *const queues;
    /** Total number of priority levels */
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


#define CORO_IMPLICIT_TIMED                             \
    {                                                   \
        state->timed_wait = true;                       \
        Timer_start_new(&state->timeout, milliseconds); \
    }


#define CORO_IMPLICIT_NOT_TIMED \
    { state->timed_wait = false; }


#define CORO_IMPLICIT_RETURN_AND_LABEL \
    {                                  \
        return;                        \
        CORO_LINE_LABEL() :;           \
    }


#define CORO_YIELD_EXPLICIT(state)              \
    {                                           \
        CORO_INLINE_SAVE_STATE_EXPLICIT(state); \
        state->status = CORO_STATUS_SUSPENDED;  \
        CORO_IMPLICIT_NOT_TIMED;                \
        CORO_IMPLICIT_RETURN_AND_LABEL;         \
    }


#define CORO_AWAIT_TIMED_EXPLICIT(state, milliseconds) \
    {                                                  \
        CORO_INLINE_SAVE_STATE_EXPLICIT(state);        \
        state->status = CORO_STATUS_WAIT_TIMED;        \
        CORO_IMPLICIT_TIMED;                           \
        CORO_IMPLICIT_RETURN_AND_LABEL;                \
    }

#define CORO_AWAIT_CONDITION_EXPLICIT(state, condition_ptr) \
    {                                                       \
        CORO_INLINE_SAVE_STATE_EXPLICIT(state);             \
        state->status         = CORO_STATUS_WAIT_CONDITION; \
        state->wait.condition = condition_ptr;              \
        CORO_IMPLICIT_NOT_TIMED;                            \
        CORO_IMPLICIT_RETURN_AND_LABEL;                     \
    }


#define CORO_AWAIT_CONDITION_TIMED_EXPLICIT(                \
        state, condition_ptr, milliseconds)                 \
    {                                                       \
        CORO_INLINE_SAVE_STATE_EXPLICIT(state);             \
        state->status         = CORO_STATUS_WAIT_CONDITION; \
        state->wait.condition = condition_ptr;              \
        CORO_IMPLICIT_TIMED;                                \
        CORO_IMPLICIT_RETURN_AND_LABEL;                     \
    }


#define CORO_AWAIT_RESOURCE_EXPLICIT(state, resource_ptr, owner_ptr)       \
    (                                                                      \
            {                                                              \
                CORO_INLINE_SAVE_STATE_EXPLICIT(state);                    \
                state->status                 = CORO_STATUS_WAIT_RESOURCE; \
                state->wait.resource.resource = resource_ptr;              \
                state->wait.resource.owner    = owner_ptr;                 \
                CORO_IMPLICIT_NOT_TIMED;                                   \
                CORO_IMPLICIT_RETURN_AND_LABEL;                            \
            },                                                             \
            state->wait.resource.retval)


#define CORO_AWAIT_RESOURCE_TIMED_EXPLICIT(                                \
        state, resource_ptr, owner_ptr, milliseconds)                      \
    (                                                                      \
            {                                                              \
                CORO_INLINE_SAVE_STATE_EXPLICIT(state);                    \
                state->status                 = CORO_STATUS_WAIT_RESOURCE; \
                state->wait.resource.resource = resource_ptr;              \
                state->wait.resource.owner    = owner_ptr;                 \
                CORO_IMPLICIT_TIMED;                                       \
                CORO_IMPLICIT_RETURN_AND_LABEL;                            \
            },                                                             \
            state->wait.resource.retval)


#define CORO_AWAIT_SUB_COROUTINE_EXPLICIT(state, sub_state_ptr) \
    {                                                           \
        state->status             = CORO_STATUS_WAIT_SUBCORO;   \
        state->wait.sub_coroutine = sub_state_ptr;              \
        CORO_IMPLICIT_NOT_TIMED;                                \
        CORO_IMPLICIT_RETURN_AND_LABEL;                         \
    }


#define CORO_AWAIT_SUB_COROUTINE_TIMED_EXPLICIT(              \
        state, sub_state_ptr, milliseconds)                   \
    {                                                         \
        state->status             = CORO_STATUS_WAIT_SUBCORO; \
        state->wait.sub_coroutine = sub_state_ptr;            \
        CORO_IMPLICIT_TIMED;                                  \
        CORO_IMPLICIT_RETURN_AND_LABEL;                       \
    }


#define CONF_CORO_ENABLE_IMPLICIT_MACROS 1

#if CONF_CORO_ENABLE_IMPLICIT_MACROS
#define CORO_INIT() CORO_INIT_EXPLICIT(state)
#define CORO_YIELD() CORO_YIELD_EXPLICIT(state)

#define CORO_AWAIT(on, ...)                                             \
    _Generic(on, /* -----------------------------------------------*/   \
             Condition *                                                \
             : CORO_AWAIT_CONDITION_EXPLICIT(state, on, ##__VA_ARGS__), \
               Resource *                                               \
             : CORO_AWAIT_RESOURCE_EXPLICIT(state, on, ##__VA_ARGS__),  \
               CoroState *                                              \
             : CORO_AWAIT_SUB_COROUTINE_EXPLICIT(state, on, ##__VA_ARGS__))


#define CORO_AWAIT_ATMOST(milliseconds, on, ...)                \
    _Generic(on, /* ----------------------------------------*/  \
             Condition *                                        \
             : CORO_AWAIT_CONDITION_TIMED_EXPLICIT(             \
                       state, on, ##__VA_ARGS__, milliseconds), \
               Resource *                                       \
             : CORO_AWAIT_RESOURCE_TIMED_EXPLICIT(              \
                       state, on, ##__VA_ARGS__, milliseconds), \
               CoroState *                                      \
             : CORO_AWAIT_SUB_COROUTINE_TIMED_EXPLICIT(         \
                     state, on, ##__VA_ARGS__, milliseconds))


#endif


#endif /* ifndef CORO_H */
