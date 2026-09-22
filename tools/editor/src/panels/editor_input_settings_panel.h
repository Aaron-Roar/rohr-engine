/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EDITOR_INPUT_SETTINGS_PANEL_H
#define ROHR_EDITOR_INPUT_SETTINGS_PANEL_H

#include "editor_project.h"
#include "editors/editor_mode_controls.h"

typedef struct EditorInputSettingsPanel {
    bool open;
    EditorInputActionMapId selected_map;
    EditorInputActionId selected_action;
    size_t selected_binding;
    TextAsset menu_label, title, close_label;
    TextAsset map_label, action_label, binding_label, name_label, enabled_label;
    TextAsset type_label, source_label, input_label, modifiers_label;
    TextAsset scale_label, inverted_label, direction_x_label, direction_y_label;
    TextAsset previous_label, next_label, add_label, delete_label;
    TextAsset selected_map_label, selected_action_label, selected_binding_label;
    TextAsset name_field, input_field, modifiers_field, scale_field;
    TextAsset direction_x_field, direction_y_field;
    TextAsset action_type_options[3];
    TextAsset binding_source_options[4];
} EditorInputSettingsPanel;

bool editor_input_settings_panel_create(EditorInputSettingsPanel *panel,
    FontAsset *font);
void editor_input_settings_panel_open(EditorInputSettingsPanel *panel);
void editor_input_settings_panel_draw(EditorInputSettingsPanel *panel,
    EditorProject *project, UIRect bounds);
void editor_input_settings_panel_destroy(EditorInputSettingsPanel *panel);

#endif
