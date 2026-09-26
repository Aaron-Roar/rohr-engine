/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EDITOR_PARTICLE_H
#define ROHR_EDITOR_PARTICLE_H

#include "editors/editor_mode_context.h"
#include "editor_center_of_mass.h"

typedef struct EditorParticleEditor {
    EditorCenterOfMassEditor center_of_mass;
    TextAsset title;
    TextAsset visibility_label;
    TextAsset visible_label;
    TextAsset hidden_label;
    TextAsset delete_label;
    TextAsset radius_label;
    TextAsset rigid_vertices_label;
    TextAsset origin_x_label;
    TextAsset origin_y_label;
    TextAsset auto_fit_label;
    TextAsset ring_color_label;
    TextAsset fill_color_label;
    TextAsset radius_field;
    TextAsset rigid_vertices_field;
    TextAsset origin_x_field;
    TextAsset origin_y_field;
} EditorParticleEditor;

bool editor_particle_editor_create(EditorParticleEditor *editor,
    FontAsset *font);
void editor_particle_editor_destroy(EditorParticleEditor *editor);
bool editor_particle_editor_draw(EditorParticleEditor *editor,
    const EditorModeContext *context);
bool editor_particle_radius_editor_draw(EditorParticleEditor *editor,
    const EditorModeContext *context);

#endif
