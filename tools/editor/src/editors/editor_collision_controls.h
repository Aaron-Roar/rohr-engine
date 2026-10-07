/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_COLLISION_CONTROLS_H
#define ROHR_EDITOR_COLLISION_CONTROLS_H
#include "editor_history.h"
#include "editor_viewport.h"

typedef struct EditorCollisionValues {
    bool enabled_all, enabled_any;
    uint64_t category_all, category_any, with_all, with_any;
} EditorCollisionValues;
typedef struct EditorCollisionControls {
    FontAsset *font; /* Borrowed; all text assets below are owned. */
    TextAsset toggle, headers[2], add, fields[2], labels[EDITOR_COLLISION_MASK_MAX], error;
    char names[2][EDITOR_OBJECT_NAME_MAX];
    char caches[EDITOR_COLLISION_MASK_MAX][EDITOR_OBJECT_NAME_MAX];
    char error_message[512];
    bool open[2];
    uint64_t selection_key;
    UIRect header_bounds[2], list_bounds[2];
} EditorCollisionControls;
typedef struct EditorCollisionDrawResult { bool active, changed; float bottom; } EditorCollisionDrawResult;
bool editor_collision_values_get(EditorProject *project, const EditorSelectionRef *selections,
    size_t count, EditorCollisionValues *values);
/* filter == -1 toggles collision. Otherwise edit one named bit; create adds a
 * project category and assigns it in the same transaction. */
EditorResult editor_collision_edit(EditorProject *project, EditorHistory *history,
    const EditorSelectionRef *selections, size_t count, int filter,
    const char *mask, bool enabled, bool create);
bool editor_collision_controls_create(EditorCollisionControls *controls, FontAsset *font);
void editor_collision_controls_destroy(EditorCollisionControls *controls);
void editor_collision_controls_selection_set(EditorCollisionControls *controls,
    const EditorSelectionRef *selections, size_t count);
float editor_collision_controls_height_get(const EditorCollisionControls *controls,
    const EditorProject *project, bool toggle);
EditorCollisionDrawResult editor_collision_controls_draw(EditorCollisionControls *controls,
    const char *id, EditorProject *project, EditorHistory *history,
    const EditorSelectionRef *selections, size_t count, float x, float y, float width, bool toggle);
#endif
