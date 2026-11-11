/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef ROHR_TEST_ALLOCATION_WRAP
static bool allocation_forbidden;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t size) {
    assert(!allocation_forbidden);
    return __real_malloc(size);
}
void *__wrap_calloc(size_t count, size_t size) {
    assert(!allocation_forbidden);
    return __real_calloc(count, size);
}
void *__wrap_realloc(void *data, size_t size) {
    assert(!allocation_forbidden);
    return __real_realloc(data, size);
}
#endif

static void success(EngineResult result) {
    assert(result.kind == ERROR_RESULT_VALUE && result.result.value);
}

static void failure(EngineResult result, EngineError expected) {
    assert(result.kind == ERROR_RESULT_ERROR && result.result.error == expected);
}

static void verify(EngineResult (*ids_get)(EntityList *),
        EngineResult (*names_get)(EntityNameList *)) {
    enum { CAPACITY = 4 };
    Entity ids[CAPACITY], saved_ids[CAPACITY];
    EntityName names[CAPACITY], saved_names[CAPACITY];
    memset(ids, 0xa5, sizeof(ids));
    memset(names, 0xa5, sizeof(names));
    memcpy(saved_ids, ids, sizeof(ids));
    memcpy(saved_names, names, sizeof(names));
    EntityList list = {.data = ids, .capacity = CAPACITY, .count = SIZE_MAX};
    EntityNameList name_list = {.data = names, .capacity = CAPACITY, .count = SIZE_MAX};
    EntityList empty = {0};
    EntityNameList empty_names = {0};
    EntityList invalid = {.capacity = CAPACITY};
    EntityNameList invalid_names = {.capacity = CAPACITY};

    assert(rohr_entity_alive_count_get() == 0);
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden = true;
#endif
    success(ids_get(&empty));
    success(names_get(&empty_names));
    assert(empty.count == 0 && empty_names.count == 0);
    success(ids_get(&list));
    success(names_get(&name_list));
    assert(list.count == 0 && name_list.count == 0);
    assert(memcmp(ids, saved_ids, sizeof(ids)) == 0);
    assert(memcmp(names, saved_names, sizeof(names)) == 0);
    failure(ids_get(NULL), ERROR_ENGINE_ENTITY_LIST_INVALID);
    failure(names_get(NULL), ERROR_ENGINE_ENTITY_LIST_INVALID);
    failure(ids_get(&invalid), ERROR_ENGINE_ENTITY_LIST_INVALID);
    failure(names_get(&invalid_names), ERROR_ENGINE_ENTITY_LIST_INVALID);
    assert(invalid.count == 0 && invalid_names.count == 0);
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden = false;
#endif

    Entity entities[3];
    for(size_t i = 0; i < 3; ++i) {
        EntityResult created = rohr_entity_add();
        assert(created.kind == ERROR_RESULT_VALUE);
        entities[i] = created.result.value;
    }
    success(rohr_entity_name_set(entities[0], "z_last_alphabetically"));
    success(rohr_entity_name_set(entities[2], "a_first_alphabetically"));
    /* Ordinary creation assigns a default name; remove it to exercise unnamed entities. */
    success(entity_components_delete(entities[1], ROHR_ENTITY_NAME));
    size_t required = rohr_entity_alive_count_get();
    assert(required == 3);

#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden = true;
#endif
    failure(ids_get(&empty), ERROR_ENGINE_ENTITY_LIST_CAPACITY_EXCEEDED);
    failure(names_get(&empty_names), ERROR_ENGINE_ENTITY_LIST_CAPACITY_EXCEEDED);
    assert(empty.count == required && empty_names.count == required);
    failure(ids_get(&invalid), ERROR_ENGINE_ENTITY_LIST_INVALID);
    failure(names_get(&invalid_names), ERROR_ENGINE_ENTITY_LIST_INVALID);
    assert(invalid.count == required && invalid_names.count == required);
    for(size_t capacity = 0; capacity < required; ++capacity) {
        list.capacity = name_list.capacity = capacity;
        list.count = name_list.count = SIZE_MAX;
        failure(ids_get(&list), ERROR_ENGINE_ENTITY_LIST_CAPACITY_EXCEEDED);
        failure(names_get(&name_list), ERROR_ENGINE_ENTITY_LIST_CAPACITY_EXCEEDED);
        assert(list.count == required && name_list.count == required);
        assert(memcmp(ids, saved_ids, sizeof(ids)) == 0);
        assert(memcmp(names, saved_names, sizeof(names)) == 0);
    }
    for(size_t capacity = required; capacity <= CAPACITY; ++capacity) {
        list.capacity = name_list.capacity = capacity;
        success(ids_get(&list));
        success(names_get(&name_list));
        assert(list.data == ids && name_list.data == names);
        assert(list.capacity == capacity && name_list.capacity == capacity);
        assert(list.count == required && name_list.count == required);
        assert(ids[3] == saved_ids[3]);
        assert(memcmp(&names[3], &saved_names[3], sizeof(EntityName)) == 0);
        for(size_t i = 0; i < required; ++i) {
            EntityResult discovered = rohr_entity_alive_at_get((uint32_t)i);
            assert(discovered.kind == ERROR_RESULT_VALUE && discovered.result.value == ids[i]);
            assert(ids[i] == entities[i]);
        }
        assert(strcmp(names[0].value, "z_last_alphabetically") == 0);
        EntityName unnamed = {0};
        assert(memcmp(&names[1], &unnamed, sizeof(unnamed)) == 0);
        assert(strcmp(names[2].value, "a_first_alphabetically") == 0);
    }
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden = false;
#endif
    memcpy(saved_ids, ids, sizeof(ids));
    memcpy(saved_names, names, sizeof(names));
    success(rohr_entity_name_set(entities[0], "renamed"));
    assert(memcmp(names, saved_names, sizeof(names)) == 0);
    EntityIndexResult old_index = rohr_entity_index_get(entities[0]);
    assert(old_index.kind == ERROR_RESULT_VALUE);
    success(rohr_entity_delete(entities[0]));
    assert(!rohr_entity_alive_check(ids[0]));
    EntityResult replacement = rohr_entity_add();
    assert(replacement.kind == ERROR_RESULT_VALUE);
    assert(replacement.result.value != ids[0]);
    EntityIndexResult new_index = rohr_entity_index_get(replacement.result.value);
    assert(new_index.kind == ERROR_RESULT_VALUE && new_index.result.value == old_index.result.value);
    assert(!rohr_entity_alive_check(ids[0]));
    success(rohr_entity_name_set(replacement.result.value, "replacement"));
    assert(memcmp(ids, saved_ids, sizeof(ids)) == 0);
    assert(memcmp(names, saved_names, sizeof(names)) == 0);

    /* Fresh copies follow current discovery order, including after swap-removal. */
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden = true;
#endif
    success(ids_get(&list));
    success(names_get(&name_list));
    assert(list.count == required && name_list.count == required);
    for(size_t i = 0; i < required; ++i) {
        EntityResult discovered = entity_alive_at_get((uint32_t)i);
        assert(discovered.kind == ERROR_RESULT_VALUE && discovered.result.value == ids[i]);
        assert(rohr_entity_alive_check(ids[i]));
        EntityNameResult current = rohr_entity_name_get(ids[i]);
        if(current.kind == ERROR_RESULT_VALUE)
            assert(memcmp(&names[i], &current.result.value, sizeof(EntityName)) == 0);
        else
            assert(ids[i] == entities[1] && names[i].value[0] == '\0');
    }
#ifdef ROHR_TEST_ALLOCATION_WRAP
    allocation_forbidden = false;
#endif
    /* Deletion through captured IDs demonstrates that the snapshot retains nothing. */
    for(size_t i = 0; i < list.count; ++i) success(rohr_entity_delete(ids[i]));
    success(ids_get(&list));
    success(names_get(&name_list));
    assert(list.count == 0 && name_list.count == 0);
}

int main(void) {
    success(rohr_engine_start());
    Report report = report_create();
    success(report_header_set(&report,
        (ReportHeader){.id = ROHR_ENGINE_REPORT_ID, .status = ROHR_REPORT_COMPLETE}));
    EngineError codes[] = {ERROR_ENGINE_ENTITY_LIST_INVALID,
        ERROR_ENGINE_ENTITY_LIST_CAPACITY_EXCEEDED};
    for(size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); ++i) {
        success(report_diagnostic_add(&report, (ReportDiagnostic){
            .report_id = ROHR_ENGINE_REPORT_ID, .severity = ROHR_REPORT_SEVERITY_WARNING,
            .code = codes[i], .occurrence_count = 1}));
        assert(strstr(error_code_message_get(codes[i]), "list") != NULL);
    }
    assert(report_check(report));
    verify(entity_list_get, entity_name_list_get);
    verify(rohr_entity_list_get, rohr_entity_name_list_get);
    /* Cross-facade pairs share the same discovery order and error contract. */
    verify(entity_list_get, rohr_entity_name_list_get);
    verify(rohr_entity_list_get, entity_name_list_get);
    rohr_engine_stop();
    return 0;
}
