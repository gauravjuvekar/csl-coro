/** \file timer_interface.h
 *
 * Timer interface required by coroutine scheduler. This should be implemeted
 * specific to each hardware
 */
/* Copyright 2018 Gaurav Juvekar */
#ifndef TIMER_INTERFACE_H
#define TIMER_INTERFACE_H 1

#include <inttypes.h>
#include "condition.h"


/** Number of milliseconds */
typedef uint32_t timer_ms_t;

/** Forward declaration for internal implementation of the timer */
typedef struct TimerInternal TimerInternal;


/** A timer instance with a condition set at timeout
 * \warning The instance must be passed to only one runnnig timer at a time,
 * and must be passed to #Timer_cancel before it is deallocated.
 */
typedef struct {
    /** A condition set when the timer value has expired */
    Condition timed_out;
    /** Pointer to any required internal representation. This will not be
     * modified by any user of the implementation. */
    TimerInternal *internal;
} Timer;

/** \brief Start a new timer
 * \param instance The timer instance and condition variable that wil be set
 *                 when milliseconds have elapsed.
 * \param milliseconds the number of milliseconds after which
 *                     \p instance->timed_out will be set
 * \return A pointer that can be later passed to
 *
 * \post \p instance must be cancelled with #Timer_cancel before it is
 * deallocated, otherwise it will be asynchronously overwritten when the timer
 * eventually expires. This will lead to \b undefined \b behaviour.
 */
extern void Timer_start_new(Timer *instance, timer_ms_t milliseconds);

/** \brief Cancel a started (or expired timer)
 * \param instance The timer instance to cancel
 * \pre The instance must be initialized with #Timer_start_new, otherwise it
 * will lead to \b undefined \b behaviour.
 * \note This function may be called more than once on the same \p instance
 * consecutively \b without re-initialization with #Timer_start_new as long as
 * \p instance->internal is not externally modified
 */
extern void Timer_cancel(Timer *instance);

#endif /* ifndef TIMER_INTERFACE_H */
