/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef EDITOR_CENTER_OF_MASS_H
#define EDITOR_CENTER_OF_MASS_H

#include "editors/editor_mode_context.h"

enum { EDITOR_COM_ROW_COUNT = 5, EDITOR_COM_ROW_HEIGHT = 32 };
typedef struct EditorCenterOfMassEditor {
    TextAsset mode_label, automatic_label, explicit_label, centroid_label;
    TextAsset x_label, y_label, inertia_label, response_label;
    TextAsset x_field, y_field, inertia_field;
} EditorCenterOfMassEditor;

bool editor_center_of_mass_editor_create(EditorCenterOfMassEditor *editor,
    FontAsset *font);
void editor_center_of_mass_editor_destroy(EditorCenterOfMassEditor *editor);
bool editor_center_of_mass_editor_draw(EditorCenterOfMassEditor *editor,
    const EditorModeContext *context, EditorObject *object,
    EditorRigidBody *body, float y);

#endif
