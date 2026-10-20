/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef EDITOR_FORCE_H
#define EDITOR_FORCE_H
#include "editors/editor_mode_context.h"
#include "editors/editor_mode_controls.h"
#define EDITOR_FORCE_FIELD_MAX 40
#define EDITOR_FORCE_REFERENCE_MAX (1 + EDITOR_RIGID_BODY_MAX + EDITOR_SOFT_BODY_MAX * (1 + EDITOR_SOFT_NODE_MAX + EDITOR_SOFT_BEAM_MAX))
typedef struct EditorForceReferenceLabel {
    ObjectSourceReference reference;
    TextAsset text;
    char name[EDITOR_OBJECT_NAME_MAX * 2 + 4];
} EditorForceReferenceLabel;
typedef struct EditorForceEditor {
    FontAsset *font;
    EditorVisualControls visuals;
    TextAsset labels[EDITOR_FORCE_FIELD_MAX], fields[EDITOR_FORCE_FIELD_MAX];
    char label_cache[EDITOR_FORCE_FIELD_MAX][EDITOR_OBJECT_NAME_MAX];
    TextAsset choices[16], name, delete_label;
    char name_cache[EDITOR_OBJECT_NAME_MAX];
    EditorForceReferenceLabel *references;
    const TextAsset **options;
    size_t *indices;
    EditorSelectionRef *pick_references;
    size_t reference_count;
    size_t row;
    const char *scope;
    bool editing;
    bool torque;
    TextAsset torque_delete_label;
    float content_height;
    EditorModeAccordionSection source_section, location_section, orientation_section;
} EditorForceEditor;
bool editor_force_editor_create(EditorForceEditor *editor, FontAsset *font);
void editor_force_editor_destroy(EditorForceEditor *editor);
bool editor_force_editor_draw(EditorForceEditor *editor, const EditorModeContext *context);
typedef EditorForceEditor EditorTorqueEditor;
bool editor_torque_editor_create(EditorTorqueEditor *editor, FontAsset *font);
bool editor_torque_editor_draw(EditorTorqueEditor *editor, const EditorModeContext *context);
#endif
