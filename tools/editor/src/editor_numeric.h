/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only */
#ifndef ROHR_EDITOR_NUMERIC_H
#define ROHR_EDITOR_NUMERIC_H
#include "editor_project.h"

typedef enum EditorNumericType {
    EDITOR_NUMERIC_FLOAT, EDITOR_NUMERIC_DOUBLE, EDITOR_NUMERIC_UINT,
    EDITOR_NUMERIC_INT, EDITOR_NUMERIC_UINT16, EDITOR_NUMERIC_UINT64, EDITOR_NUMERIC_AXIS_ANGLE,
    EDITOR_NUMERIC_LENGTH, EDITOR_NUMERIC_UI_LENGTH, EDITOR_NUMERIC_MASS, EDITOR_NUMERIC_COM_X, EDITOR_NUMERIC_COM_Y, EDITOR_NUMERIC_ALPHA
} EditorNumericType;
typedef struct EditorNumericProperty {
    const char *key, *section;
    uint32_t kind;
    EditorNumericType type;
    size_t offset;
} EditorNumericProperty;
typedef union EditorNumericValue {
    float number;
    double real;
    uint32_t integer;
    int signed_integer;
    uint64_t wide;
} EditorNumericValue;

bool editor_numeric_target_check(EditorNumericTarget a, EditorNumericTarget b);
const EditorNumericProperty *editor_numeric_properties_get(size_t *count);
const EditorNumericProperty *editor_numeric_property_get(EditorNumericTarget target, const char *key);
const char *editor_numeric_compatible_key_get(EditorNumericTarget target, const char *key);
void *editor_numeric_target_get(EditorProject *project, EditorNumericTarget target);
bool editor_numeric_value_get(EditorProject *project, EditorNumericTarget target,
    const char *key, EditorNumericValue *value);
bool editor_numeric_value_equal_check(const EditorNumericProperty *property,
    EditorNumericValue a, EditorNumericValue b);
bool editor_numeric_value_restore(EditorProject *project, EditorNumericTarget target,
    const char *key, EditorNumericValue value);
bool editor_numeric_lock_check(const EditorProject *project, EditorNumericTarget target, const char *key);
bool editor_numeric_lock_set(EditorProject *project, EditorNumericTarget target, const char *key, bool locked);
bool editor_numeric_locks_copy(EditorProject *destination, const EditorProject *source);
void editor_numeric_locks_prune(EditorProject *project);
void editor_numeric_ui_vertices_remap(EditorProject *project,uint32_t definition,
    size_t old_count,const bool *removed,size_t insertion);
const char *editor_numeric_kind_name_get(uint32_t kind);
bool editor_numeric_kind_parse(const char *name, uint32_t *kind);
bool editor_numeric_section_check(const EditorNumericProperty *property, const char *section);
typedef struct EditorNumericGuardEntry {
    EditorNumericLock lock;
    EditorNumericValue value;
    bool slider_range;
    float minimum, maximum, slider_value;
    bool ui_line;
    Position endpoints[2];
} EditorNumericGuardEntry;
typedef struct EditorNumericGuard {
    EditorNumericGuardEntry *entries;
    size_t count, capacity;
    uint64_t revision;
} EditorNumericGuard;
bool editor_numeric_guard_begin(EditorNumericGuard *guard, EditorProject *project);
size_t editor_numeric_guard_finish(EditorNumericGuard *guard, EditorProject *project, char *reason, size_t capacity);
void editor_numeric_guard_destroy(EditorNumericGuard *guard);
#endif
