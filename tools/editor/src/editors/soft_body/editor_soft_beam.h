/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef EDITOR_SOFT_BEAM_H
#define EDITOR_SOFT_BEAM_H

#include "editors/editor_mode_context.h"

typedef bool (*EditorSoftBeamCollisionMenuFunction)(void *context,
    const char *id_prefix, EditorProject *project, uint64_t *active_masks,
    EditorObjectId object, EditorSoftBodyId body, EditorSoftBeamId beam,
    EditorCollisionFilterKind filter, float x, float y, float width,
    bool *field_active, size_t *row_count);

typedef struct EditorSoftBeamEditor {
    FontAsset *font;
    TextAsset name_label, node_a_label, node_b_label;
    TextAsset stiffness_label, damping_label, collision_label, thickness_label;
    TextAsset collision_category_label, collide_with_label, color_label, inherit_label;
    TextAsset none_label, visibility_label, visible_label, hidden_label, delete_label;
    TextAsset stiffness_field, damping_field, thickness_field;
    TextAsset beam_names[EDITOR_SOFT_BEAM_MAX];
    TextAsset node_names[EDITOR_SOFT_NODE_MAX];
    char beam_cache[EDITOR_SOFT_BEAM_MAX][EDITOR_OBJECT_NAME_MAX];
    char node_cache[EDITOR_SOFT_NODE_MAX][EDITOR_OBJECT_NAME_MAX];
    bool collision_category_open;
    bool collide_with_open;
} EditorSoftBeamEditor;

bool editor_soft_beam_editor_create(EditorSoftBeamEditor *editor,
    FontAsset *font);
void editor_soft_beam_editor_destroy(EditorSoftBeamEditor *editor);
bool editor_soft_beam_editor_draw(EditorSoftBeamEditor *editor,
    const EditorModeContext *context,
    EditorSoftBeamCollisionMenuFunction collision_menu,
    void *collision_menu_context);

#endif
