/** \file resource.h
 *
 * Priority based "partial" lock for controlling access to resources
 */
/* Copyright 2018 Gaurav Juvekar */

#include "resource.h"

RetResource_acquire Resource_acquire(Resource *     resource,
                                     ResourceOwner *owner) {
    bool           cmp_xchg_done;
    ResourceOwner *current_owner = atomic_load(resource);
    do {
        if (current_owner == NULL
            || current_owner->priority < owner->priority) {
            cmp_xchg_done = atomic_compare_exchange_strong(
                    resource, &current_owner, owner);
        } else {
            return RESOURCE_ACQUIRE_FAILED;
        }
    } while (!cmp_xchg_done);
    if (current_owner == NULL) {
        return RESOURCE_ACQUIRE_SUCCESS;
    } else {
        return RESOURCE_ACQUIRE_PREEMPTED;
    }
}


void Resource_release(Resource *resource, ResourceOwner *owner) {
    bool           cmp_xchg_done;
    ResourceOwner *current_owner = atomic_load(resource);
    do {
        if (current_owner == owner) {
            cmp_xchg_done = atomic_compare_exchange_strong(
                    resource, &current_owner, NULL);
        }
        else {
            return;
        }
    } while (!cmp_xchg_done);
}


bool Resource_is_owned(Resource *resource, ResourceOwner *owner) {
    return owner == atomic_load(resource);
}
