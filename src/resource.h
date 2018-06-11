/** \file resource.h
 *
 * Priority based "partial" lock for controlling access to resources
 */
/* Copyright 2018 Gaurav Juvekar */
#ifndef RESOURCE_H
#define RESOURCE_H 1

#include <stdatomic.h>
#include <inttypes.h>
#include <stdbool.h>


/** A object that is referred to as the current owner for each #Resource
 *
 * Each user must create (and ensure the lifetime of) a single #ResourceOwner
 * object as long as the #Resource is held.
 */
typedef struct {
    /** The target priority of the current owner that determines preemption of
     * the #Resource.
     */
    int32_t priority;
} ResourceOwner;


/** A resource object
 *
 * A single object must be allocated for each physical resource. Access to the
 * resource must be made by acquring this object first.
 */
typedef ResourceOwner *_Atomic Resource;


/** Return values of #Resource_acquire */
typedef enum {
    /** Acquiring the resource failed as it was already acquired at a stronger
     * priority */
    RESOURCE_ACQUIRE_FAILED = 0,
    /** The resource was un-owned and acquired successfully */
    RESOURCE_ACQUIRE_SUCCESS,
    /** The resource was acquried successfully, but was pre-empted from a
     * weaker prioirty owner
     *
     * \warning You may need to ensure safety (for example by disabling the
     * resource) in case the previous owner blind-uses the resource if it is
     * unaware of the preemption.
     */
    RESOURCE_ACQUIRE_PREEMPTED,
} RetResource_acquire;


/** Acquire a resource
 * \param resource the resource to acquire
 * \param owner    a #ResourceOwner object with desired priority set
 *
 * \note \p owner must be valid (not de-allocated) while \p resource is held
 * and until #Resource_release is called.
 *
 * \post #Resource_release must be called if \p resource is acquired.
 *
 * \return See #RetResource_acquire for possible return values.
 */
RetResource_acquire Resource_acquire(Resource *resource, ResourceOwner *owner);


/** \brief Release a held resource
 * \param resource the resource to release
 * \param owner    the owner (self) of the resource
 */
void Resource_release(Resource *resource, ResourceOwner *owner);

/** \brief Check if the resource is currently owned by the specified owner
 * \param resource the resource
 * \param owner    the owner (self) to test for
 * \return If \p resource is currently held by \p owner
 * \warning This value, if \c true is useless if the \p resource can be
 * preempted immediately after checking for ownership. You need to ensure that
 */
bool Resource_is_owned(Resource *resource, ResourceOwner *owner);

#endif /* ifndef RESOURCE_H */
