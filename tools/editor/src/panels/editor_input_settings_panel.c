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
        char *value, size_t capacity, UIRect bounds) {
    return editor_mode_field(id,
        (UIFieldBinding){.kind = UI_FIELD_STRING, .string = value,
            .string_capacity = capacity}, display, bounds, NULL);
}

static UIFieldResult input_number_field(const char *id, TextAsset *display,
        float *value, UIRect bounds) {
    return editor_mode_field(id,
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = value}, display,
        bounds, NULL);
}

static void input_unique_action_name(const EditorInputController *controller,
        char output[ROHR_INPUT_NAME_MAX]) {
    for(uint32_t number = 1; number < UINT32_MAX; number += 1) {
        bool used = false;
        snprintf(output, ROHR_INPUT_NAME_MAX, "action_%u", number);
        for(size_t i = 0; controller != NULL && i < controller->action_count; i += 1)
            if(strcmp(controller->actions[i].name, output) == 0) used = true;
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
    static const char *button_modes[] = {"Momentary", "Persistent"};
    static const char *sources[] = {
        "Key", "Mouse Button", "Mouse Motion", "Mouse Wheel"};
    static const char *mouse_buttons[] = {"Left", "Middle", "Right", "X1", "X2"};
    static const char *axis_components[] = {"X", "Y", "X + Y"};
    char label[32];
    if(panel == NULL || font == NULL) return false;
    *panel = (EditorInputSettingsPanel){.font = font};
#define CREATE(value, member) \
    if(!input_text_create(font, value, &panel->member)) goto fail
    CREATE("Input", menu_label); CREATE("Controller", controller_title);
    CREATE("Action", action_title); CREATE("Close", close_label);
    CREATE("Add Action", add_action_label); CREATE("Add Binding", add_binding_label);
    CREATE("Delete Binding", delete_binding_label);
    CREATE("Delete Action", delete_action_label); CREATE("Binding", binding_label);
    CREATE("Name", name_label); CREATE("Enabled", enabled_label);
    CREATE("Type", type_label); CREATE("Mode", button_mode_label);
    CREATE("Initial state", initial_state_label); CREATE("Source", source_label);
    CREATE("Input / Axis", input_label); CREATE("Modifiers", modifiers_label);
    CREATE("Scale", scale_label); CREATE("Inverted", inverted_label);
    CREATE("Direction X", direction_x_label);
    CREATE("Direction Y", direction_y_label); CREATE("", name_field);
    CREATE("", input_field); CREATE("", modifiers_field); CREATE("", scale_field);
    CREATE("", direction_x_field); CREATE("", direction_y_field);
#undef CREATE
    for(size_t i = 0; i < 3; i += 1)
        if(!input_text_create(font, types[i], &panel->action_type_options[i]))
            goto fail;
    for(size_t i = 0; i < 2; i += 1)
        if(!input_text_create(font, button_modes[i],
                &panel->button_mode_options[i])) goto fail;
    for(size_t i = 0; i < 4; i += 1)
        if(!input_text_create(font, sources[i], &panel->binding_source_options[i]))
            goto fail;
    for(size_t i = 0; i < ROHR_INPUT_BINDING_LIMIT; i += 1) {
        snprintf(label, sizeof(label), "Binding %u", (unsigned)(i + 1));
        if(!input_text_create(font, label, &panel->binding_options[i])) goto fail;
    }
    for(size_t i = 0; i < 5; i += 1)
        if(!input_text_create(font, mouse_buttons[i],
                &panel->mouse_button_options[i])) goto fail;
    for(size_t i = 0; i < 3; i += 1)
        if(!input_text_create(font, axis_components[i],
                &panel->axis_component_options[i])) goto fail;
    return true;
fail:
    editor_input_settings_panel_destroy(panel);
    return false;
}

void editor_input_settings_panel_open(EditorInputSettingsPanel *panel) {
    if(panel != NULL) panel->open = true;
}

static void input_panel_begin(TextAsset *title, UIRect bounds) {
    rohr_ui_surface(bounds, (Color){37, 42, 52, 255});
    rohr_ui_border(bounds, 2.0f, (Color){5, 6, 8, 255});
    rohr_ui_label(title, (UIRect){bounds.x + 20.0f, bounds.y + 14.0f,
        bounds.width - 40.0f, 32.0f});
}

static void input_panel_close_draw(EditorInputSettingsPanel *panel,
        UIRect bounds) {
    if(rohr_ui_button("editor.input.close", &panel->close_label,
            (UIRect){bounds.x + bounds.width - 92.0f,
                bounds.y + 12.0f, 72.0f, 30.0f}, NULL).clicked)
        panel->open = false;
}

void editor_input_controller_editor_draw(EditorInputSettingsPanel *panel,
        const EditorModeContext *context, UIRect bounds) {
    EditorInputController *controller;
    float x, width, y;
    if(panel == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL || !panel->open) return;
    controller = editor_project_input_controller_get(context->project,
        context->viewport->selected_input_controller);
    if(controller == NULL) return;
    input_panel_begin(&panel->controller_title, bounds);
    x = bounds.x + 18.0f;
    width = bounds.width - 36.0f;
    y = bounds.y + 58.0f;
    {
        char name[ROHR_INPUT_NAME_MAX];
        bool enabled = controller->enabled;
        bool changed;
        snprintf(name, sizeof(name), "%s", controller->name);
        rohr_ui_label(&panel->name_label, (UIRect){x, y, 80.0f, 28.0f});
        changed = input_text_field("editor.input.controller.name",
            &panel->name_field, name, sizeof(name),
            (UIRect){x + 88.0f, y, width - 88.0f, 28.0f}).changed;
        y += 36.0f;
        if(editor_mode_checkbox_left("editor.input.controller.enabled",
                &panel->enabled_label, (UIRect){x, y, width, 28.0f}, &enabled))
            changed = true;
        if(changed) {
            EditorCommand command = {.type = EDITOR_COMMAND_INPUT_CONTROLLER_SET,
                .data.input_controller = {.controller = controller->id,
                    .enabled = enabled}};
            snprintf(command.data.input_controller.name,
                sizeof(command.data.input_controller.name), "%s", name);
            (void)editor_command_execute(context->project, &command);
        }
    }
    y += 40.0f;
    if(rohr_ui_button("editor.input.action.add", &panel->add_action_label,
            (UIRect){x, y, width, 32.0f}, NULL).clicked) {
        EditorCommand command = {.type = EDITOR_COMMAND_INPUT_ACTION_ADD,
            .data.input_action = {.controller = controller->id,
                .type = INPUT_ACTION_BUTTON,
                .button_mode = INPUT_BUTTON_MOMENTARY}};
        input_unique_action_name(controller, command.data.input_action.name);
        EditorCommandResult result = editor_command_execute(context->project, &command);
        if(result.kind == ERROR_RESULT_VALUE) {
            context->viewport->selected_input_action = result.result.object;
            context->viewport->selection = EDITOR_SELECTION_INPUT_ACTION;
            panel->selected_binding = 0;
        }
    }
    y += 42.0f;
    for(size_t i = 0; i < controller->action_count &&
            i < ROHR_INPUT_ACTION_LIMIT; i += 1) {
        EditorInputAction *action = &controller->actions[i];
        UIButtonStyle style = rohr_ui_button_style_default_get();
        UIButtonResult result;
        UIRect row_bounds = {x, y, width, 30.0f};
        char id[96];
        if(!editor_mode_named_text_sync(panel->font, action->name,
                &panel->action_names[i], panel->action_cache[i],
                ROHR_INPUT_NAME_MAX)) continue;
        style.idle = (Color){55, 63, 76, 255};
        style.hovered = (Color){73, 84, 101, 255};
        snprintf(id, sizeof(id), "editor.input.action.%u", action->id);
        result = rohr_ui_button(id, &panel->action_names[i], row_bounds,
            context->viewport->selection == EDITOR_SELECTION_INPUT_ACTION &&
                context->viewport->selected_input_controller == controller->id &&
                context->viewport->selected_input_action == action->id ?
                    &style : NULL);
        if(context->hierarchy_row != NULL)
            context->hierarchy_row(context->hierarchy_context, context->viewport,
                (EditorSelectionRef){EDITOR_SELECTION_INPUT_ACTION,
                    0, controller->id, 0, action->id}, row_bounds, result,
                i + 1 == controller->action_count);
        if(result.clicked || result.focus_changed) {
            editor_viewport_selection_clear(context->viewport);
            context->viewport->selected_input_controller = controller->id;
            context->viewport->selected_input_action = action->id;
            context->viewport->selection = EDITOR_SELECTION_INPUT_ACTION;
            panel->selected_binding = 0;
            editor_project_selection_clear(context->project);
            if(result.double_clicked)
                context->viewport->mode = EDITOR_VIEWPORT_INPUT_ACTION;
        }
        y += 34.0f;
    }
    input_panel_close_draw(panel, bounds);
}

static void input_action_properties_draw(EditorInputSettingsPanel *panel,
        EditorProject *project, EditorInputController *controller,
        EditorInputAction *action, float x, float width, float *y) {
    char name[ROHR_INPUT_NAME_MAX];
    size_t type = action->type;
    size_t button_mode = action->button_mode;
    bool button_initial_state = action->button_initial_state;
    bool changed;
    const TextAsset *options[] = {&panel->action_type_options[0],
        &panel->action_type_options[1], &panel->action_type_options[2]};
    UIDropdownResult selected;
    snprintf(name, sizeof(name), "%s", action->name);
    rohr_ui_label(&panel->name_label, (UIRect){x, *y, 80.0f, 28.0f});
    changed = input_text_field("editor.input.action.name", &panel->name_field,
        name, sizeof(name), (UIRect){x + 88.0f, *y, width - 88.0f, 28.0f}).changed;
    *y += 36.0f;
    rohr_ui_label(&panel->type_label, (UIRect){x, *y, 80.0f, 28.0f});
    selected = editor_mode_dropdown("editor.input.action.type", options, 3, type,
        (UIRect){x + 88.0f, *y, width - 88.0f, 28.0f}, NULL);
    if(selected.changed) { type = selected.selected_index; changed = true; }
    if((InputActionType)type == INPUT_ACTION_BUTTON) {
        const TextAsset *button_options[] = {
            &panel->button_mode_options[0], &panel->button_mode_options[1]};
        *y += 36.0f;
        rohr_ui_label(&panel->button_mode_label,
            (UIRect){x, *y, 80.0f, 28.0f});
        selected = editor_mode_dropdown("editor.input.action.button_mode",
            button_options, 2, button_mode,
            (UIRect){x + 88.0f, *y, width - 88.0f, 28.0f}, NULL);
        if(selected.changed) { button_mode = selected.selected_index; changed = true; }
        *y += 36.0f;
        if((InputButtonMode)button_mode == INPUT_BUTTON_PERSISTENT) {
            if(editor_mode_checkbox_left("editor.input.action.initial_state",
                    &panel->initial_state_label, (UIRect){x, *y, width, 28.0f},
                    &button_initial_state)) changed = true;
        } else if(button_initial_state) {
            button_initial_state = false;
            changed = true;
        }
    } else {
        button_mode = INPUT_BUTTON_MOMENTARY;
        button_initial_state = false;
    }
    if(changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_INPUT_ACTION_SET,
            .data.input_action = {.controller = controller->id,
                .action = action->id, .type = (InputActionType)type,
                .button_mode = (InputButtonMode)button_mode,
                .button_initial_state = button_initial_state}};
        snprintf(command.data.input_action.name,
            sizeof(command.data.input_action.name), "%s", name);
        (void)editor_command_execute(project, &command);
    }
}

static void input_binding_selector_draw(EditorInputSettingsPanel *panel,
        EditorProject *project, EditorInputController *controller,
        EditorInputAction *action, float x, float width, float y) {
    bool add;
    bool remove;
    rohr_ui_label(&panel->binding_label, (UIRect){x, y, 76.0f, 28.0f});
    if(action->binding_count > 0) {
        const TextAsset *options[ROHR_INPUT_BINDING_LIMIT];
        UIDropdownResult selected;
        if(panel->selected_binding >= action->binding_count)
            panel->selected_binding = action->binding_count - 1;
        for(size_t i = 0; i < action->binding_count; i += 1)
            options[i] = &panel->binding_options[i];
        selected = editor_mode_dropdown("editor.input.binding.select", options,
            action->binding_count, panel->selected_binding,
            (UIRect){x + 80.0f, y, width - 80.0f, 28.0f}, NULL);
        if(selected.changed) panel->selected_binding = selected.selected_index;
    } else {
        rohr_ui_button_disabled((UIRect){x + 80.0f, y, width - 80.0f, 28.0f},
            NULL);
    }
    y += 36.0f;
    add = rohr_ui_button("editor.input.binding.add", &panel->add_binding_label,
        (UIRect){x, y, (width - 4.0f) * 0.5f, 28.0f}, NULL).clicked;
    remove = rohr_ui_button("editor.input.binding.delete",
        &panel->delete_binding_label,
        (UIRect){x + (width + 4.0f) * 0.5f, y,
            (width - 4.0f) * 0.5f, 28.0f}, NULL).clicked;
    if(add) {
        EditorCommand command = {.type = EDITOR_COMMAND_INPUT_BINDING_ADD,
            .data.input_binding = {.controller = controller->id,
                .action = action->id,
                .binding = input_binding_default_get(action->type)}};
        EditorCommandResult result = editor_command_execute(project, &command);
        if(result.kind == ERROR_RESULT_VALUE)
            panel->selected_binding = result.result.object;
    }
    if(remove && panel->selected_binding < action->binding_count) {
        EditorCommand command = {.type = EDITOR_COMMAND_INPUT_BINDING_REMOVE,
            .data.input_binding = {.controller = controller->id,
                .action = action->id, .index = panel->selected_binding,
                .binding = action->bindings[panel->selected_binding]}};
        (void)editor_command_execute(project, &command);
        if(panel->selected_binding > 0 &&
                panel->selected_binding >= action->binding_count - 1)
            panel->selected_binding -= 1;
    }
}

static void input_binding_properties_draw(EditorInputSettingsPanel *panel,
        EditorProject *project, EditorInputController *controller,
        EditorInputAction *action, float x, float width, float y) {
    InputBinding binding;
    const TextAsset *source_options[] = {&panel->binding_source_options[0],
        &panel->binding_source_options[1], &panel->binding_source_options[2],
        &panel->binding_source_options[3]};
    size_t source;
    float input, modifiers, scale, direction_x, direction_y;
    bool inverted, changed = false;
    UIDropdownResult selected;
    if(action->binding_count == 0 ||
            panel->selected_binding >= action->binding_count) return;
    binding = action->bindings[panel->selected_binding];
    source = binding.source;
    input = binding.source == INPUT_BINDING_KEY ? binding.input.key :
        binding.source == INPUT_BINDING_MOUSE_BUTTON ?
            binding.input.mouse_button : binding.input.axis_component;
    modifiers = binding.modifiers;
    scale = binding.scale;
    direction_x = binding.direction.x;
    direction_y = binding.direction.y;
    inverted = binding.inverted;
    rohr_ui_label(&panel->source_label, (UIRect){x, y, 80.0f, 28.0f});
    selected = editor_mode_dropdown("editor.input.binding.source", source_options,
        action->type == INPUT_ACTION_BUTTON ? 2 : 4, source,
        (UIRect){x + 88.0f, y, width - 88.0f, 28.0f}, NULL);
    if(selected.changed) {
        source = selected.selected_index;
        changed = true;
        input = source == INPUT_BINDING_KEY ? SDL_SCANCODE_SPACE :
            source == INPUT_BINDING_MOUSE_BUTTON ? INPUT_MOUSE_BUTTON_LEFT :
                INPUT_AXIS_COMPONENT_X;
    }
    y += 36.0f;
    rohr_ui_label(&panel->input_label, (UIRect){x, y, 80.0f, 28.0f});
    if(source == INPUT_BINDING_MOUSE_BUTTON) {
        const TextAsset *options[5];
        size_t selected_button = input >= INPUT_MOUSE_BUTTON_LEFT &&
                input <= INPUT_MOUSE_BUTTON_X2 ? (size_t)input - 1 : 0;
        for(size_t i = 0; i < 5; i += 1) options[i] = &panel->mouse_button_options[i];
        selected = editor_mode_dropdown("editor.input.binding.mouse_button",
            options, 5, selected_button,
            (UIRect){x + 88.0f, y, width - 88.0f, 28.0f}, NULL);
        if(selected.changed) { input = selected.selected_index + 1; changed = true; }
    } else if(source == INPUT_BINDING_MOUSE_MOTION ||
            source == INPUT_BINDING_MOUSE_WHEEL) {
        const TextAsset *options[3];
        size_t count = action->type == INPUT_ACTION_AXIS_1D ? 2 : 3;
        size_t component = input >= INPUT_AXIS_COMPONENT_X &&
                input <= INPUT_AXIS_COMPONENT_XY ? (size_t)input : 0;
        if(component >= count) component = 0;
        for(size_t i = 0; i < count; i += 1)
            options[i] = &panel->axis_component_options[i];
        selected = editor_mode_dropdown("editor.input.binding.axis", options,
            count, component, (UIRect){x + 88.0f, y, width - 88.0f, 28.0f}, NULL);
        if(selected.changed) { input = selected.selected_index; changed = true; }
    } else if(input_number_field("editor.input.binding.key", &panel->input_field,
            &input, (UIRect){x + 88.0f, y, width - 88.0f, 28.0f}).changed) {
        changed = true;
    }
    y += 36.0f;
    rohr_ui_label(&panel->modifiers_label, (UIRect){x, y, 80.0f, 28.0f});
    if(input_number_field("editor.input.binding.modifiers", &panel->modifiers_field,
            &modifiers, (UIRect){x + 88.0f, y, width * 0.25f, 28.0f}).changed)
        changed = true;
    rohr_ui_label(&panel->scale_label,
        (UIRect){x + width * 0.48f, y, 48.0f, 28.0f});
    if(input_number_field("editor.input.binding.scale", &panel->scale_field, &scale,
            (UIRect){x + width * 0.62f, y, width * 0.18f, 28.0f}).changed)
        changed = true;
    if(editor_mode_checkbox_left("editor.input.binding.inverted",
            &panel->inverted_label,
            (UIRect){x + width * 0.82f, y, width * 0.18f, 28.0f}, &inverted))
        changed = true;
    y += 36.0f;
    rohr_ui_label(&panel->direction_x_label, (UIRect){x, y, 90.0f, 28.0f});
    if(input_number_field("editor.input.binding.direction_x",
            &panel->direction_x_field, &direction_x,
            (UIRect){x + 94.0f, y, width * 0.24f, 28.0f}).changed)
        changed = true;
    rohr_ui_label(&panel->direction_y_label,
        (UIRect){x + width * 0.52f, y, 90.0f, 28.0f});
    if(input_number_field("editor.input.binding.direction_y",
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
            binding.input.mouse_button = (InputMouseButton)(uint32_t)input;
        else binding.input.axis_component = (InputAxisComponent)(uint32_t)input;
        EditorCommand command = {.type = EDITOR_COMMAND_INPUT_BINDING_SET,
            .data.input_binding = {.controller = controller->id,
                .action = action->id, .index = panel->selected_binding,
                .binding = binding}};
        (void)editor_command_execute(project, &command);
    }
}

void editor_input_action_editor_draw(EditorInputSettingsPanel *panel,
        EditorProject *project, EditorViewportState *viewport, UIRect bounds) {
    EditorInputController *controller;
    EditorInputAction *action;
    float x, width, y;
    if(panel == NULL || project == NULL || viewport == NULL || !panel->open) return;
    controller = editor_project_input_controller_get(project,
        viewport->selected_input_controller);
    action = controller == NULL ? NULL : editor_project_input_action_get(project,
        controller->id, viewport->selected_input_action);
    if(action == NULL) return;
    input_panel_begin(&panel->action_title, bounds);
    x = bounds.x + 18.0f;
    width = bounds.width - 36.0f;
    y = bounds.y + 58.0f;
    input_action_properties_draw(panel, project, controller, action, x, width, &y);
    y += 42.0f;
    input_binding_selector_draw(panel, project, controller, action, x, width, y);
    action = editor_project_input_action_get(project, controller->id,
        viewport->selected_input_action);
    y += 74.0f;
    if(action != NULL)
        input_binding_properties_draw(panel, project, controller, action,
            x, width, y);
    input_panel_close_draw(panel, bounds);
}

void editor_input_settings_panel_destroy(EditorInputSettingsPanel *panel) {
    if(panel == NULL) return;
#define DESTROY(member) rohr_graphics_text_destroy(&panel->member)
    DESTROY(menu_label); DESTROY(controller_title); DESTROY(action_title);
    DESTROY(close_label); DESTROY(add_action_label); DESTROY(add_binding_label);
    DESTROY(delete_binding_label); DESTROY(delete_action_label);
    DESTROY(binding_label); DESTROY(name_label); DESTROY(enabled_label);
    DESTROY(type_label); DESTROY(button_mode_label); DESTROY(initial_state_label);
    DESTROY(source_label); DESTROY(input_label); DESTROY(modifiers_label);
    DESTROY(scale_label); DESTROY(inverted_label); DESTROY(direction_x_label);
    DESTROY(direction_y_label); DESTROY(name_field); DESTROY(input_field);
    DESTROY(modifiers_field); DESTROY(scale_field); DESTROY(direction_x_field);
    DESTROY(direction_y_field);
#undef DESTROY
    for(size_t i = 0; i < ROHR_INPUT_ACTION_LIMIT; i += 1)
        rohr_graphics_text_destroy(&panel->action_names[i]);
    for(size_t i = 0; i < 3; i += 1) {
        rohr_graphics_text_destroy(&panel->action_type_options[i]);
        rohr_graphics_text_destroy(&panel->axis_component_options[i]);
    }
    for(size_t i = 0; i < 2; i += 1)
        rohr_graphics_text_destroy(&panel->button_mode_options[i]);
    for(size_t i = 0; i < 4; i += 1)
        rohr_graphics_text_destroy(&panel->binding_source_options[i]);
    for(size_t i = 0; i < ROHR_INPUT_BINDING_LIMIT; i += 1)
        rohr_graphics_text_destroy(&panel->binding_options[i]);
    for(size_t i = 0; i < 5; i += 1)
        rohr_graphics_text_destroy(&panel->mouse_button_options[i]);
    *panel = (EditorInputSettingsPanel){0};
}
