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
    bool key_capture_active;
    bool input_numeric;
    bool modifiers_numeric;
    SDL_Scancode pending_modifier;
    FontAsset *font;
    EditorModeAccordionSection input_section;
    EditorModeAccordionSection axis_section;
    EditorModeAccordionSection x_axis_section;
    EditorModeAccordionSection y_axis_section;
    TextAsset menu_label, controller_title, action_title, binding_title;
    TextAsset add_action_label, add_binding_label, delete_binding_label;
    TextAsset delete_action_label;
    TextAsset binding_label, name_label, enabled_label;
    TextAsset type_label, button_mode_label, initial_state_label;
    TextAsset source_label, input_label, modifiers_label;
    TextAsset affects_x_label, affects_y_label;
    TextAsset scale_label, scale_x_label, scale_y_label;
    TextAsset inverted_label, inverted_x_label, inverted_y_label;
    TextAsset direction_x_label, direction_y_label;
    TextAsset ascii_value_label, letter_symbols_label;
    TextAsset name_field, input_field, modifiers_field;
    TextAsset scale_x_field, scale_y_field;
    TextAsset direction_x_field, direction_y_field;
    TextAsset action_names[ROHR_INPUT_ACTION_LIMIT];
    char action_cache[ROHR_INPUT_ACTION_LIMIT][ROHR_INPUT_NAME_MAX];
    TextAsset binding_names[ROHR_INPUT_BINDING_LIMIT];
    char binding_cache[ROHR_INPUT_BINDING_LIMIT][ROHR_INPUT_NAME_MAX];
    TextAsset action_type_options[3];
    TextAsset button_mode_options[2];
    TextAsset binding_source_options[4];
    TextAsset mouse_button_options[5];
    TextAsset axis_component_options[3];
} EditorInputSettingsPanel;

bool editor_input_settings_panel_create(EditorInputSettingsPanel *panel,
    FontAsset *font);
void editor_input_settings_panel_open(EditorInputSettingsPanel *panel);
void editor_input_controller_editor_draw(EditorInputSettingsPanel *panel,
    const EditorModeContext *context, UIRect bounds);
void editor_input_action_editor_draw(EditorInputSettingsPanel *panel,
    const EditorModeContext *context, UIRect bounds);
void editor_input_binding_editor_draw(EditorInputSettingsPanel *panel,
    EditorProject *project, EditorViewportState *viewport, UIRect bounds);
bool editor_input_key_capture_apply(SDL_Scancode pressed,
    SDL_Scancode released, SDL_Keymod modifiers, SDL_Scancode *pending_modifier,
    InputBinding *binding);
void editor_input_settings_panel_destroy(EditorInputSettingsPanel *panel);

#endif
