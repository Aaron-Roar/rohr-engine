/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EDITOR_PHYSICS_SETTINGS_PANEL_H
#define ROHR_EDITOR_PHYSICS_SETTINGS_PANEL_H

#include "editor_project.h"
#include "editors/editor_mode_controls.h"

typedef struct EditorPhysicsSettingsPanel {
    bool open;
    TextAsset menu_label, title, close_label;
    TextAsset engine_timestep_label, override_label, physics_timestep_label;
    TextAsset substeps_label, gravity_x_label, gravity_y_label;
    TextAsset solver_iterations_label;
    TextAsset engine_timestep_field, physics_timestep_field, substeps_field;
    TextAsset gravity_x_field, gravity_y_field, solver_iterations_field;
    EditorModeAccordionSection timing_section, world_section, solver_section;
} EditorPhysicsSettingsPanel;

bool editor_physics_settings_panel_create(EditorPhysicsSettingsPanel *panel,
    FontAsset *font);
void editor_physics_settings_panel_open(EditorPhysicsSettingsPanel *panel);
void editor_physics_settings_apply(const EditorProject *project);
void editor_physics_settings_panel_draw(EditorPhysicsSettingsPanel *panel,
    EditorProject *project, UIRect bounds);
void editor_physics_settings_panel_destroy(EditorPhysicsSettingsPanel *panel);

#endif
