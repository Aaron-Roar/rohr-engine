/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_SOFT_AREA_EDITOR_H
#define ROHR_EDITOR_SOFT_AREA_EDITOR_H
#include "editors/editor_mode_context.h"
#include "editors/editor_mode_controls.h"
typedef struct EditorSoftAreaEditor {
    FontAsset *font;
    TextAsset add_area, add_hole, name_label, name_field, visible, hidden, visibility;
    TextAsset select_nodes, picking, inactive, up, down, remove, color;
    TextAsset delete_area, delete_hole, status;
    TextAsset area_names[SOFT_BODY_MAX_AREAS];
    TextAsset hole_names[SOFT_BODY_MAX_AREA_HOLES];
    TextAsset node_names[SOFT_BODY_MAX_NODES];
    char area_cache[SOFT_BODY_MAX_AREAS][EDITOR_OBJECT_NAME_MAX];
    char hole_cache[SOFT_BODY_MAX_AREA_HOLES][EDITOR_OBJECT_NAME_MAX];
    char node_cache[SOFT_BODY_MAX_NODES][EDITOR_OBJECT_NAME_MAX + 16];
    EditorModeAccordionSection areas_section;
    EditorModeAccordionSection appearance_section;
} EditorSoftAreaEditor;
bool editor_soft_area_editor_create(EditorSoftAreaEditor *editor, FontAsset *font);
void editor_soft_area_editor_destroy(EditorSoftAreaEditor *editor);
float editor_soft_area_list_draw(EditorSoftAreaEditor *editor,
    const EditorModeContext *context, EditorSoftBody *body, float y);
bool editor_soft_area_editor_draw(EditorSoftAreaEditor *editor,
    const EditorModeContext *context);
#endif
