/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "contact_constraint.h"

#include <stdlib.h>
#include <string.h>

static size_t rigid_pair_slot_get(const ContactConstraintList *list,
        Entity first, Entity second) {
    uint64_t hash = ((uint64_t)first << 32) | second;
    hash = (hash ^ (hash >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    hash = (hash ^ (hash >> 27)) * UINT64_C(0x94d049bb133111eb);
    hash ^= hash >> 31;
    size_t slot = (size_t)hash & (list->rigid_pair_capacity - 1);
    while(list->rigid_pair_slots[slot]) {
        const SystemContactConstraint *constraint =
            &list->values[list->rigid_pair_slots[slot] - 1];
        if(constraint->value.rigid.first == first &&
                constraint->value.rigid.second == second) break;
        slot = (slot + 1) & (list->rigid_pair_capacity - 1);
    }
    return slot;
}

static bool rigid_pair_reserve_check(ContactConstraintList *list, size_t requested) {
    if(requested <= list->rigid_pair_capacity / 2) return true;
    size_t capacity = list->rigid_pair_capacity ? list->rigid_pair_capacity : 64;
    while(capacity / 2 < requested) {
        if(capacity > SIZE_MAX / sizeof(size_t) / 2) return false;
        capacity *= 2;
    }
    size_t *slots = calloc(capacity, sizeof(*slots));
    if(slots == NULL) return false;
    free(list->rigid_pair_slots);
    list->rigid_pair_slots = slots;
    list->rigid_pair_capacity = capacity;
    for(size_t i = 0; i < list->count; ++i) {
        const SystemContactConstraint *constraint = &list->values[i];
        if(constraint->type != SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR) continue;
        size_t slot = rigid_pair_slot_get(list, constraint->value.rigid.first,
            constraint->value.rigid.second);
        list->rigid_pair_slots[slot] = i + 1;
    }
    return true;
}

static bool contact_constraint_list_reserve(
    ContactConstraintList *list,
    size_t requested
) {
    SystemContactConstraint *values;
    size_t capacity;

    if(list == NULL) return false;
    if(requested > SIZE_MAX / sizeof(*values)) return false;
    if(requested <= list->capacity) return true;
    capacity = list->capacity == 0 ? 64 : list->capacity;
    while(capacity < requested) {
        if(capacity > SIZE_MAX / sizeof(*values) / 2) {
            capacity = requested;
            break;
        }
        capacity *= 2;
    }
    values = realloc(list->values, capacity * sizeof(*values));
    if(values == NULL) return false;
    list->values = values;
    list->capacity = capacity;
    return true;
}

EngineResult contact_constraint_list_init(
    ContactConstraintList *list,
    size_t initial_capacity
) {
    if(list == NULL) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    *list = (ContactConstraintList){0};
    if(initial_capacity > 0 &&
            !contact_constraint_list_reserve(list, initial_capacity)) {
        return error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    return error_result_value(true);
}

void contact_constraint_list_destroy(ContactConstraintList *list) {
    if(list == NULL) return;
    free(list->values);
    free(list->rigid_pair_slots);
    *list = (ContactConstraintList){0};
}

void contact_constraint_list_clear(ContactConstraintList *list) {
    if(list != NULL) {
        list->count = 0;
        list->rigid_pair_count = 0;
        if(list->rigid_pair_slots != NULL)
            memset(list->rigid_pair_slots, 0,
                list->rigid_pair_capacity * sizeof(*list->rigid_pair_slots));
        list->rigid_gathered = false;
        list->rigid_dirty = false;
        list->solver_converged = false;
        list->solving = false;
    }
}

bool contact_constraint_list_append(
    ContactConstraintList *list,
    SystemContactConstraint constraint
) {
    if(list == NULL || list->count == SIZE_MAX ||
            !contact_constraint_list_reserve(list, list->count + 1)) {
        return false;
    }
    if(constraint.type == SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR) {
        if(!rigid_pair_reserve_check(list, list->rigid_pair_count + 1)) return false;
        size_t slot = rigid_pair_slot_get(list, constraint.value.rigid.first,
            constraint.value.rigid.second);
        list->rigid_pair_slots[slot] = list->count + 1;
        ++list->rigid_pair_count;
    }
    list->values[list->count++] = constraint;
    return true;
}

bool contact_constraint_list_rigid_pair_check(const ContactConstraintList *list,
        Entity first, Entity second) {
    if(list == NULL || list->rigid_pair_count == 0) return false;
    return list->rigid_pair_slots[rigid_pair_slot_get(list, first, second)] != 0;
}

SystemContactConstraint *contact_constraint_list_at(
    ContactConstraintList *list,
    size_t index
) {
    if(list == NULL || index >= list->count) return NULL;
    return &list->values[index];
}
