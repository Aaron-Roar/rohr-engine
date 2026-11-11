/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_TEST_RUNTIME_FIXTURE_H
#define ROHR_TEST_RUNTIME_FIXTURE_H
#include "rohr.h"
#include <string.h>

/* Test fixtures deliberately repeat recipes in one world. Inspect public ID/name
 * snapshots and lifetime ownership to locate the fixture's expected suffixed
 * runtime name. This is test inspection, not a generated runtime access API.
 * A fixture with two matching bases in one subtree must retain explicit IDs. */
static inline EntityResult fixture_entity_get(Entity owner, const char *base) {
    Entity storage[MAX_ENTITIES];
    EntityList list = {.data = storage, .capacity = rohr_entity_alive_count_get()};
    if(!base || !base[0] || !rohr_entity_alive_check(owner))
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_ENTITY_NOT_FOUND);
    EngineResult copied = rohr_entity_list_get(&list);
    if(rohr_error_check(copied)) return ERROR_RESULT_MAKE_ERROR(EntityResult, copied.result.error);
    size_t length = strlen(base);
    Entity found = ENTITY_INVALID;
    for(size_t i = 0; i < list.count; ++i) {
        EntityNameResult name = rohr_entity_name_get(list.data[i]);
        if(rohr_error_check(name)) continue;
        const char *value = name.result.value.value;
        bool match = !strcmp(value, base);
        if(!match && !strncmp(value, base, length) && value[length] == '_') {
            const char *suffix = value + length + 1;
            match = *suffix >= '1' && *suffix <= '9';
            while(*suffix >= '0' && *suffix <= '9') ++suffix;
            match = match && !*suffix;
        }
        if(!match || list.data[i] == owner) continue;
        Entity ancestor = list.data[i];
        while(ancestor && ancestor != owner) {
            EntityResult parent = rohr_entity_owner_get(ancestor);
            if(rohr_error_check(parent)) break;
            ancestor = parent.result.value;
        }
        if(ancestor != owner) continue;
        if(found) return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
        found = list.data[i];
    }
    return found ? ERROR_RESULT_MAKE_VALUE(EntityResult, found) :
        ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_ENTITY_NOT_FOUND);
}
#endif
