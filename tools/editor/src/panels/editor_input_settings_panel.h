/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EDITOR_INPUT_SETTINGS_PANEL_H
#define ROHR_EDITOR_INPUT_SETTINGS_PANEL_H

#include "editor_project.h"
#include "editors/editor_mode_context.h"
#include "editors/editor_mode_controls.h"

typedef struct EditorInputSettingsPanel {
    bool open;
    size_t selected_binding;
    FontAsset *font;
    TextAsset menu_label, controller_title, action_title, close_label;
    TextAsset add_action_label, add_binding_label, delete_binding_label;
    TextAsset delete_action_label;
    TextAsset binding_label, name_label, enabled_label;
    TextAsset type_label, button_mode_label, initial_state_label;
    TextAsset source_label, input_label, modifiers_label;
    TextAsset scale_label, inverted_label, direction_x_label, direction_y_label;
    TextAsset name_field, input_field, modifiers_field, scale_field;
    TextAsset direction_x_field, direction_y_field;
    TextAsset action_names[ROHR_INPUT_ACTION_LIMIT];
    char action_cache[ROHR_INPUT_ACTION_LIMIT][ROHR_INPUT_NAME_MAX];
    TextAsset action_type_options[3];
    TextAsset button_mode_options[2];
    TextAsset binding_source_options[4];
    TextAsset binding_options[ROHR_INPUT_BINDING_LIMIT];
    TextAsset mouse_button_options[5];
    TextAsset axis_component_options[3];
} EditorInputSettingsPanel;

bool editor_input_settings_panel_create(EditorInputSettingsPanel *panel,
    FontAsset *font);
void editor_input_settings_panel_open(EditorInputSettingsPanel *panel);
void editor_input_controller_editor_draw(EditorInputSettingsPanel *panel,
    const EditorModeContext *context, UIRect bounds);
void editor_input_action_editor_draw(EditorInputSettingsPanel *panel,
    EditorProject *project, EditorViewportState *viewport, UIRect bounds);
void editor_input_settings_panel_destroy(EditorInputSettingsPanel *panel);

#endif
