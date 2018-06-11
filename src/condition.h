/** \file condition.h
 *
 * A synchronized condition variable that coroutines can await on.
 */
/* Copyright 2018 Gaurav Juvekar */
#ifndef CONDITION_H
#define CONDITION_H 1


#include <stdatomic.h>
#include <stdbool.h>

/** A condition can be set to true or cleared to false */
typedef _Atomic bool Condition;

/** \brief Atomically gets the current value of the condition */
static inline bool Condition_get(Condition *condition) {
    return atomic_load(condition);
}

/** Atomically sets the condition to true */
static inline void Condition_set(Condition *condition) {
    atomic_store(condition, true);
}

/** Atomically clears the condition to false */
static inline void Condition_clear(Condition *condition) {
    atomic_store(condition, false);
}

#endif /* ifndef CONDITION_H */
