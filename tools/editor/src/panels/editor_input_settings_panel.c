/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_input_settings_panel.h"
#include "editor_command.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static bool input_text_create(FontAsset *font, const char *value,
        TextAsset *text) {
    return editor_mode_text_create(font, value, text);
}

static UIFieldResult input_text_field(const char *id, TextAsset *display,
        char *value, UIRect bounds) {
    return editor_mode_field(id,
        (UIFieldBinding){.kind = UI_FIELD_STRING, .string = value}, display,
        bounds, NULL);
}

static UIFieldResult input_number_field(const char *id, TextAsset *display,
        float *value, UIRect bounds) {
    return editor_mode_field(id,
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = value}, display,
        bounds, NULL);
}

static EditorInputActionMap *input_selected_map(EditorInputSettingsPanel *panel,
        EditorProject *project) {
    EditorInputActionMap *map = editor_project_input_action_map_get(project,
        panel->selected_map);
    if(map == NULL && project->input_action_map_count > 0) {
        map = &project->input_action_maps[0];
        panel->selected_map = map->id;
        panel->selected_action = 0;
        panel->selected_binding = 0;
    }
    if(project->input_action_map_count == 0) panel->selected_map = 0;
    return map;
}

static EditorInputAction *input_selected_action(EditorInputSettingsPanel *panel,
        EditorProject *project, EditorInputActionMap *map) {
    EditorInputAction *action = map == NULL ? NULL :
        editor_project_input_action_get(project, map->id,
            panel->selected_action);
    if(action == NULL && map != NULL && map->action_count > 0) {
        action = &map->actions[0];
        panel->selected_action = action->id;
        panel->selected_binding = 0;
    }
    if(map == NULL || map->action_count == 0) panel->selected_action = 0;
    if(action == NULL || panel->selected_binding >= action->binding_count)
        panel->selected_binding = 0;
    return action;
}

static void input_map_cycle(EditorInputSettingsPanel *panel,
        EditorProject *project, int direction) {
    if(project->input_action_map_count == 0) return;
    size_t index = 0;
    for(size_t i = 0; i < project->input_action_map_count; i += 1)
        if(project->input_action_maps[i].id == panel->selected_map) index = i;
    index = direction < 0 ?
        (index + project->input_action_map_count - 1) %
            project->input_action_map_count :
        (index + 1) % project->input_action_map_count;
    panel->selected_map = project->input_action_maps[index].id;
    panel->selected_action = 0;
    panel->selected_binding = 0;
}

static void input_action_cycle(EditorInputSettingsPanel *panel,
        EditorInputActionMap *map, int direction) {
    if(map == NULL || map->action_count == 0) return;
    size_t index = 0;
    for(size_t i = 0; i < map->action_count; i += 1)
        if(map->actions[i].id == panel->selected_action) index = i;
    index = direction < 0 ? (index + map->action_count - 1) % map->action_count :
        (index + 1) % map->action_count;
    panel->selected_action = map->actions[index].id;
    panel->selected_binding = 0;
}

static void input_unique_map_name(const EditorProject *project,
        char output[ROHR_INPUT_NAME_MAX]) {
    for(uint32_t number = 1; number < UINT32_MAX; number += 1) {
        bool used = false;
        snprintf(output, ROHR_INPUT_NAME_MAX, "input_map_%u", number);
        for(size_t i = 0; i < project->input_action_map_count; i += 1)
            if(strcmp(project->input_action_maps[i].name, output) == 0)
                used = true;
        if(!used) return;
    }
}

static void input_unique_action_name(const EditorInputActionMap *map,
        char output[ROHR_INPUT_NAME_MAX]) {
    for(uint32_t number = 1; number < UINT32_MAX; number += 1) {
        bool used = false;
        snprintf(output, ROHR_INPUT_NAME_MAX, "action_%u", number);
        for(size_t i = 0; map != NULL && i < map->action_count; i += 1)
            if(strcmp(map->actions[i].name, output) == 0) used = true;
        if(!used) return;
    }
}

static InputBinding input_binding_default_get(InputActionType type) {
    InputBinding binding = {.source = INPUT_BINDING_KEY,
        .input.key = type == INPUT_ACTION_BUTTON ? SDL_SCANCODE_SPACE :
            type == INPUT_ACTION_AXIS_1D ? SDL_SCANCODE_A : SDL_SCANCODE_W,
        .scale = 1.0f};
    if(type == INPUT_ACTION_AXIS_2D) binding.direction.y = -1.0f;
    return binding;
}

bool editor_input_settings_panel_create(EditorInputSettingsPanel *panel,
        FontAsset *font) {
    static const char *types[] = {"Button", "Axis 1D", "Axis 2D"};
    static const char *sources[] = {
        "Key", "Mouse Button", "Mouse Motion", "Mouse Wheel"};
    if(panel == NULL || font == NULL) return false;
    *panel = (EditorInputSettingsPanel){0};
#define CREATE(value, member) \
    if(!input_text_create(font, value, &panel->member)) goto fail
    CREATE("Input", menu_label); CREATE("Input Settings", title);
    CREATE("Close", close_label); CREATE("Action Map", map_label);
    CREATE("Action", action_label); CREATE("Binding", binding_label);
    CREATE("Name", name_label); CREATE("Enabled", enabled_label);
    CREATE("Type", type_label); CREATE("Source", source_label);
    CREATE("Input / Axis", input_label); CREATE("Modifiers", modifiers_label);
    CREATE("Scale", scale_label); CREATE("Inverted", inverted_label);
    CREATE("Direction X", direction_x_label);
    CREATE("Direction Y", direction_y_label); CREATE("<", previous_label);
    CREATE(">", next_label); CREATE("Add", add_label); CREATE("Delete", delete_label);
    CREATE("None", selected_map_label); CREATE("None", selected_action_label);
    CREATE("None", selected_binding_label); CREATE("", name_field);
    CREATE("", input_field); CREATE("", modifiers_field); CREATE("", scale_field);
    CREATE("", direction_x_field); CREATE("", direction_y_field);
#undef CREATE
    for(size_t i = 0; i < 3; i += 1)
        if(!input_text_create(font, types[i], &panel->action_type_options[i]))
            goto fail;
    for(size_t i = 0; i < 4; i += 1)
        if(!input_text_create(font, sources[i], &panel->binding_source_options[i]))
            goto fail;
    return true;
fail:
    editor_input_settings_panel_destroy(panel);
    return false;
}

void editor_input_settings_panel_open(EditorInputSettingsPanel *panel) {
    if(panel != NULL) panel->open = true;
}

static void input_selector_draw(const char *prefix, TextAsset *title,
        TextAsset *selection, EditorInputSettingsPanel *panel, float x, float y,
        float width, bool *previous, bool *next, bool *add, bool *remove) {
    char id[96];
    rohr_ui_label(title, (UIRect){x, y, 90.0f, 28.0f});
#define BUTTON(suffix, label, bx, bw, target) do { \
    snprintf(id, sizeof(id), "%s.%s", prefix, suffix); \
    *(target) = rohr_ui_button(id, (label), \
        (UIRect){(bx), y, (bw), 28.0f}, NULL).clicked; \
} while(0)
    BUTTON("previous", &panel->previous_label, x + 94.0f, 30.0f, previous);
    rohr_ui_surface((UIRect){x + 128.0f, y, width - 278.0f, 28.0f},
        (Color){25, 29, 36, 255});
    rohr_ui_label(selection,
        (UIRect){x + 134.0f, y, width - 290.0f, 28.0f});
    BUTTON("next", &panel->next_label, x + width - 146.0f, 30.0f, next);
    BUTTON("add", &panel->add_label, x + width - 112.0f, 52.0f, add);
    BUTTON("delete", &panel->delete_label, x + width - 56.0f, 56.0f, remove);
#undef BUTTON
}

void editor_input_settings_panel_draw(EditorInputSettingsPanel *panel,
        EditorProject *project, UIRect bounds) {
    EditorInputActionMap *map;
    EditorInputAction *action;
    bool previous, next, add, remove;
    float x, width, y;
    if(panel == NULL || project == NULL || !panel->open) return;
    map = input_selected_map(panel, project);
    action = input_selected_action(panel, project, map);
    (void)rohr_graphics_text_value_set(&panel->selected_map_label,
        map == NULL ? "None" : map->name);
    (void)rohr_graphics_text_value_set(&panel->selected_action_label,
        action == NULL ? "None" : action->name);
    if(action == NULL || action->binding_count == 0)
        (void)rohr_graphics_text_value_set(&panel->selected_binding_label, "None");
    else {
        char value[32];
        snprintf(value, sizeof(value), "Binding %u of %u",
            (unsigned)(panel->selected_binding + 1),
            (unsigned)action->binding_count);
        (void)rohr_graphics_text_value_set(&panel->selected_binding_label, value);
    }
    rohr_ui_modal_controls_begin();
    rohr_ui_surface(bounds, (Color){37, 42, 52, 255});
    rohr_ui_border(bounds, 2.0f, (Color){5, 6, 8, 255});
    rohr_ui_label(&panel->title,
        (UIRect){bounds.x + 20.0f, bounds.y + 14.0f,
            bounds.width - 40.0f, 32.0f});
    x = bounds.x + 18.0f; width = bounds.width - 36.0f; y = bounds.y + 58.0f;
    input_selector_draw("editor.settings.input.map", &panel->map_label,
        &panel->selected_map_label, panel, x, y, width,
        &previous, &next, &add, &remove);
    if(previous) input_map_cycle(panel, project, -1);
    if(next) input_map_cycle(panel, project, 1);
    if(add) {
        EditorCommand command = {.type = EDITOR_COMMAND_INPUT_MAP_ADD};
        input_unique_map_name(project, command.data.input_map.name);
        command.data.input_map.enabled = true;
        EditorCommandResult result = editor_command_execute(project, &command);
        if(result.kind == ERROR_RESULT_VALUE) {
            panel->selected_map = result.result.object;
            panel->selected_action = 0;
        }
    }
    if(remove && map != NULL) {
        EditorCommand command = {.type = EDITOR_COMMAND_INPUT_MAP_REMOVE,
            .data.input_map = {.map = map->id, .enabled = map->enabled}};
        snprintf(command.data.input_map.name,
            sizeof(command.data.input_map.name), "%s", map->name);
        (void)editor_command_execute(project, &command);
        panel->selected_map = 0; panel->selected_action = 0;
    }
    map = input_selected_map(panel, project);
    y += 36.0f;
    if(map != NULL) {
        char name[ROHR_INPUT_NAME_MAX];
        bool enabled = map->enabled;
        bool changed;
        snprintf(name, sizeof(name), "%s", map->name);
        rohr_ui_label(&panel->name_label, (UIRect){x + 10.0f, y, 80.0f, 28.0f});
        changed = input_text_field("editor.settings.input.map.name",
            &panel->name_field, name,
            (UIRect){x + 98.0f, y, width - 230.0f, 28.0f}).changed;
        if(editor_mode_checkbox_left("editor.settings.input.map.enabled",
                &panel->enabled_label,
                (UIRect){x + width - 122.0f, y, 122.0f, 28.0f}, &enabled))
            changed = true;
        if(changed) {
            EditorCommand command = {.type = EDITOR_COMMAND_INPUT_MAP_SET,
                .data.input_map = {.map = map->id, .enabled = enabled}};
            snprintf(command.data.input_map.name,
                sizeof(command.data.input_map.name), "%s", name);
            (void)editor_command_execute(project, &command);
        }
    }
    y += 42.0f;
    map = input_selected_map(panel, project);
    action = input_selected_action(panel, project, map);
    input_selector_draw("editor.settings.input.action", &panel->action_label,
        &panel->selected_action_label, panel, x, y, width,
        &previous, &next, &add, &remove);
    if(previous) input_action_cycle(panel, map, -1);
    if(next) input_action_cycle(panel, map, 1);
    if(add && map != NULL) {
        EditorCommand command = {.type = EDITOR_COMMAND_INPUT_ACTION_ADD,
            .data.input_action = {.map = map->id, .type = INPUT_ACTION_BUTTON}};
        input_unique_action_name(map, command.data.input_action.name);
        EditorCommandResult result = editor_command_execute(project, &command);
        if(result.kind == ERROR_RESULT_VALUE)
            panel->selected_action = result.result.object;
    }
    if(remove && map != NULL && action != NULL) {
        EditorCommand command = {.type = EDITOR_COMMAND_INPUT_ACTION_REMOVE,
            .data.input_action = {.map = map->id, .action = action->id,
                .type = action->type}};
        snprintf(command.data.input_action.name,
            sizeof(command.data.input_action.name), "%s", action->name);
        (void)editor_command_execute(project, &command);
        panel->selected_action = 0; panel->selected_binding = 0;
    }
    map = input_selected_map(panel, project);
    action = input_selected_action(panel, project, map);
    y += 36.0f;
    if(action != NULL) {
        char name[ROHR_INPUT_NAME_MAX];
        size_t type = action->type;
        bool changed;
        const TextAsset *options[] = {&panel->action_type_options[0],
            &panel->action_type_options[1], &panel->action_type_options[2]};
        snprintf(name, sizeof(name), "%s", action->name);
        rohr_ui_label(&panel->name_label, (UIRect){x + 10.0f, y, 80.0f, 28.0f});
        changed = input_text_field("editor.settings.input.action.name",
            &panel->name_field, name,
            (UIRect){x + 98.0f, y, width * 0.42f, 28.0f}).changed;
        rohr_ui_label(&panel->type_label,
            (UIRect){x + width * 0.56f, y, 56.0f, 28.0f});
        UIDropdownResult selected = rohr_ui_dropdown(
            "editor.settings.input.action.type", options, 3, type,
            (UIRect){x + width * 0.68f, y, width * 0.32f, 28.0f}, NULL);
        if(selected.changed) { type = selected.selected_index; changed = true; }
        if(changed) {
            EditorCommand command = {.type = EDITOR_COMMAND_INPUT_ACTION_SET,
                .data.input_action = {.map = map->id, .action = action->id,
                    .type = (InputActionType)type}};
            snprintf(command.data.input_action.name,
                sizeof(command.data.input_action.name), "%s", name);
            (void)editor_command_execute(project, &command);
        }
    }
    y += 42.0f;
    action = input_selected_action(panel, project, map);
    input_selector_draw("editor.settings.input.binding", &panel->binding_label,
        &panel->selected_binding_label, panel, x, y, width,
        &previous, &next, &add, &remove);
    if(action != NULL && action->binding_count > 0) {
        if(previous) panel->selected_binding =
            (panel->selected_binding + action->binding_count - 1) %
                action->binding_count;
        if(next) panel->selected_binding =
            (panel->selected_binding + 1) % action->binding_count;
    }
    if(add && map != NULL && action != NULL) {
        EditorCommand command = {.type = EDITOR_COMMAND_INPUT_BINDING_ADD,
            .data.input_binding = {.map = map->id, .action = action->id,
                .binding = input_binding_default_get(action->type)}};
        EditorCommandResult result = editor_command_execute(project, &command);
        if(result.kind == ERROR_RESULT_VALUE)
            panel->selected_binding = result.result.object;
    }
    if(remove && map != NULL && action != NULL &&
            panel->selected_binding < action->binding_count) {
        EditorCommand command = {.type = EDITOR_COMMAND_INPUT_BINDING_REMOVE,
            .data.input_binding = {.map = map->id, .action = action->id,
                .index = panel->selected_binding,
                .binding = action->bindings[panel->selected_binding]}};
        (void)editor_command_execute(project, &command);
        panel->selected_binding = 0;
    }
    action = input_selected_action(panel, project, map);
    y += 38.0f;
    if(action != NULL && panel->selected_binding < action->binding_count) {
        InputBinding binding = action->bindings[panel->selected_binding];
        const TextAsset *options[] = {&panel->binding_source_options[0],
            &panel->binding_source_options[1], &panel->binding_source_options[2],
            &panel->binding_source_options[3]};
        size_t source = binding.source;
        float input = binding.source == INPUT_BINDING_KEY ? binding.input.key :
            binding.source == INPUT_BINDING_MOUSE_BUTTON ?
                binding.input.mouse_button : binding.input.axis_component;
        float modifiers = binding.modifiers, scale = binding.scale;
        float direction_x = binding.direction.x, direction_y = binding.direction.y;
        bool inverted = binding.inverted;
        bool changed = false;
        rohr_ui_label(&panel->source_label, (UIRect){x + 10.0f, y, 88.0f, 28.0f});
        UIDropdownResult selected = rohr_ui_dropdown(
            "editor.settings.input.binding.source", options,
            action->type == INPUT_ACTION_BUTTON ? 2 : 4, source,
            (UIRect){x + 102.0f, y, width * 0.34f, 28.0f}, NULL);
        if(selected.changed) {
            source = selected.selected_index; changed = true;
            input = source == INPUT_BINDING_KEY ? SDL_SCANCODE_SPACE :
                source == INPUT_BINDING_MOUSE_BUTTON ? INPUT_MOUSE_BUTTON_LEFT :
                    INPUT_AXIS_COMPONENT_X;
        }
        rohr_ui_label(&panel->input_label,
            (UIRect){x + width * 0.54f, y, 90.0f, 28.0f});
        if(input_number_field("editor.settings.input.binding.input",
                &panel->input_field, &input,
                (UIRect){x + width * 0.75f, y, width * 0.25f, 28.0f}).changed)
            changed = true;
        y += 34.0f;
        rohr_ui_label(&panel->modifiers_label,
            (UIRect){x + 10.0f, y, 88.0f, 28.0f});
        if(input_number_field("editor.settings.input.binding.modifiers",
                &panel->modifiers_field, &modifiers,
                (UIRect){x + 102.0f, y, width * 0.22f, 28.0f}).changed)
            changed = true;
        rohr_ui_label(&panel->scale_label,
            (UIRect){x + width * 0.38f, y, 56.0f, 28.0f});
        if(input_number_field("editor.settings.input.binding.scale",
                &panel->scale_field, &scale,
                (UIRect){x + width * 0.50f, y, width * 0.18f, 28.0f}).changed)
            changed = true;
        if(editor_mode_checkbox_left("editor.settings.input.binding.inverted",
                &panel->inverted_label,
                (UIRect){x + width * 0.72f, y, width * 0.28f, 28.0f},
                &inverted)) changed = true;
        y += 34.0f;
        rohr_ui_label(&panel->direction_x_label,
            (UIRect){x + 10.0f, y, 100.0f, 28.0f});
        if(input_number_field("editor.settings.input.binding.direction_x",
                &panel->direction_x_field, &direction_x,
                (UIRect){x + 114.0f, y, width * 0.26f, 28.0f}).changed)
            changed = true;
        rohr_ui_label(&panel->direction_y_label,
            (UIRect){x + width * 0.52f, y, 100.0f, 28.0f});
        if(input_number_field("editor.settings.input.binding.direction_y",
                &panel->direction_y_field, &direction_y,
                (UIRect){x + width * 0.76f, y, width * 0.24f, 28.0f}).changed)
            changed = true;
        if(changed) {
            binding = (InputBinding){.source = (InputBindingSource)source,
                .modifiers = (SDL_Keymod)(uint32_t)fmaxf(0.0f, roundf(modifiers)),
                .scale = scale, .inverted = inverted,
                .direction = {direction_x, direction_y}};
            if(binding.source == INPUT_BINDING_KEY)
                binding.input.key = (SDL_Scancode)(uint32_t)fmaxf(0.0f,
                    roundf(input));
            else if(binding.source == INPUT_BINDING_MOUSE_BUTTON)
                binding.input.mouse_button = (InputMouseButton)(uint32_t)fmaxf(
                    0.0f, roundf(input));
            else binding.input.axis_component = (InputAxisComponent)(uint32_t)
                fmaxf(0.0f, roundf(input));
            EditorCommand command = {.type = EDITOR_COMMAND_INPUT_BINDING_SET,
                .data.input_binding = {.map = map->id, .action = action->id,
                    .index = panel->selected_binding, .binding = binding}};
            (void)editor_command_execute(project, &command);
        }
    }
    if(rohr_ui_button("editor.settings.input.close", &panel->close_label,
            (UIRect){bounds.x + bounds.width - 124.0f,
                bounds.y + bounds.height - 48.0f, 100.0f, 32.0f}, NULL).clicked)
        panel->open = false;
    rohr_ui_modal_controls_end();
}

void editor_input_settings_panel_destroy(EditorInputSettingsPanel *panel) {
    if(panel == NULL) return;
#define DESTROY(member) rohr_graphics_text_destroy(&panel->member)
    DESTROY(menu_label); DESTROY(title); DESTROY(close_label); DESTROY(map_label);
    DESTROY(action_label); DESTROY(binding_label); DESTROY(name_label);
    DESTROY(enabled_label); DESTROY(type_label); DESTROY(source_label);
    DESTROY(input_label); DESTROY(modifiers_label); DESTROY(scale_label);
    DESTROY(inverted_label); DESTROY(direction_x_label); DESTROY(direction_y_label);
    DESTROY(previous_label); DESTROY(next_label); DESTROY(add_label);
    DESTROY(delete_label); DESTROY(selected_map_label);
    DESTROY(selected_action_label); DESTROY(selected_binding_label);
    DESTROY(name_field); DESTROY(input_field); DESTROY(modifiers_field);
    DESTROY(scale_field); DESTROY(direction_x_field); DESTROY(direction_y_field);
#undef DESTROY
    for(size_t i = 0; i < 3; i += 1)
        rohr_graphics_text_destroy(&panel->action_type_options[i]);
    for(size_t i = 0; i < 4; i += 1)
        rohr_graphics_text_destroy(&panel->binding_source_options[i]);
    *panel = (EditorInputSettingsPanel){0};
}
