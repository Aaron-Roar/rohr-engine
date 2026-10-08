/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EDITOR_PARTICLE_H
#define ROHR_EDITOR_PARTICLE_H

#include "editors/editor_mode_context.h"
#include "editor_center_of_mass.h"
#include "editors/editor_collision_controls.h"
#include "editors/editor_mode_controls.h"

typedef struct EditorParticleEditor {
    FontAsset *font; /* Borrowed for label refreshes. */
    TextAsset name_label;
    TextAsset x_label;
    TextAsset y_label;
    TextAsset velocity_x_label;
    TextAsset velocity_y_label;
    TextAsset acceleration_x_label;
    TextAsset acceleration_y_label;
    TextAsset mass_label;
    TextAsset friction_label;
    TextAsset restitution_label;
    TextAsset parent_label;
    TextAsset none_label;
    TextAsset gravity_label;
    TextAsset dynamic_label;
    TextAsset static_label;
    TextAsset x_field;
    TextAsset y_field;
    TextAsset velocity_x_field;
    TextAsset velocity_y_field;
    TextAsset acceleration_x_field;
    TextAsset acceleration_y_field;
    TextAsset mass_field;
    TextAsset friction_field;
    TextAsset restitution_field;
    TextAsset name_field;
    char name_cache[EDITOR_OBJECT_NAME_MAX];
    TextAsset parent_names[EDITOR_RIGID_BODY_MAX];
    char parent_cache[EDITOR_RIGID_BODY_MAX][EDITOR_OBJECT_NAME_MAX];
    EditorModeAccordionSection transform_section;
    EditorModeAccordionSection initial_motion_section;
    EditorModeAccordionSection physics_section;
    EditorModeAccordionSection material_section;
    EditorModeAccordionSection collision_section;
    EditorModeAccordionSection parenting_section;
    EditorModeAccordionSection appearance_section;
    EditorModeAccordionSection geometry_section;
    EditorCenterOfMassEditor center_of_mass;
    EditorCollisionControls collision;
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
