/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef EDITOR_SOFT_NODE_H
#define EDITOR_SOFT_NODE_H

#include "editors/editor_mode_context.h"
#include "editors/editor_collision_controls.h"
#include "editors/editor_mode_controls.h"

typedef struct EditorSoftNodeEditor {
    FontAsset *font;
    TextAsset name_label, x_label, y_label, mass_label, radius_label;
    TextAsset friction_label, restitution_label, gravity_label;
    TextAsset color_label, inherit_label, visibility_label;
    TextAsset velocity_x_label, velocity_y_label, acceleration_x_label;
    TextAsset acceleration_y_label, motion_inherit_label;
    TextAsset visible_label, hidden_label, delete_label;
    TextAsset x_field, y_field, mass_field, radius_field;
    TextAsset friction_field, restitution_field;
    TextAsset velocity_x_field, velocity_y_field, acceleration_x_field;
    TextAsset acceleration_y_field;
    TextAsset node_names[EDITOR_SOFT_NODE_MAX];
    char node_cache[EDITOR_SOFT_NODE_MAX][EDITOR_OBJECT_NAME_MAX];
    EditorCollisionControls collision;
    EditorModeAccordionSection transform_section;
    EditorModeAccordionSection initial_motion_section;
    EditorModeAccordionSection physics_section;
    EditorModeAccordionSection material_section;
    EditorModeAccordionSection collision_section;
    EditorModeAccordionSection appearance_section;
} EditorSoftNodeEditor;

bool editor_soft_node_editor_create(EditorSoftNodeEditor *editor,
    FontAsset *font);
void editor_soft_node_editor_destroy(EditorSoftNodeEditor *editor);
bool editor_soft_node_editor_draw(EditorSoftNodeEditor *editor,
    const EditorModeContext *context);

#endif
