/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static bool close_float(float a, float b) {
    return fabsf(a - b) < 0.001f;
}

static bool event_push(SDL_Event event) {
    return SDL_PushEvent(&event);
}

static void events_drain(void) {
    while(rohr_engine_event_poll().type != 0) {}
}

static bool key_event_push(Uint32 type, SDL_Scancode key, SDL_Keymod modifiers) {
    return event_push((SDL_Event){
        .key = {
            .type = type,
            .scancode = key,
            .key = SDL_GetKeyFromScancode(key, modifiers, false),
            .mod = modifiers,
            .down = type == SDL_EVENT_KEY_DOWN,
        },
    });
}

static bool mouse_button_event_push(Uint32 type, InputMouseButton button,
        float x, float y) {
    return event_push((SDL_Event){
        .button = {
            .type = type,
            .button = (Uint8)button,
            .down = type == SDL_EVENT_MOUSE_BUTTON_DOWN,
            .x = x,
            .y = y,
        },
    });
}

static bool input_raw_snapshot_test(void) {
    const char *candidates[] = {"candidate one", "candidate two"};
    InputTextState text;
    Vec2D value;

    rohr_input_frame_begin();
    if(!key_event_push(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_A, SDL_KMOD_SHIFT) ||
            !event_push((SDL_Event){
                .motion = {.type = SDL_EVENT_MOUSE_MOTION,
                    .x = 30.0f, .y = 40.0f, .xrel = 3.0f, .yrel = -2.0f},
            }) ||
            !event_push((SDL_Event){
                .motion = {.type = SDL_EVENT_MOUSE_MOTION,
                    .x = 35.0f, .y = 44.0f, .xrel = 5.0f, .yrel = 4.0f},
            }) ||
            !mouse_button_event_push(SDL_EVENT_MOUSE_BUTTON_DOWN,
                INPUT_MOUSE_BUTTON_X1, 35.0f, 44.0f) ||
            !event_push((SDL_Event){
                .wheel = {.type = SDL_EVENT_MOUSE_WHEEL,
                    .x = 2.0f, .y = -3.0f,
                    .direction = SDL_MOUSEWHEEL_FLIPPED,
                    .mouse_x = 35.0f, .mouse_y = 44.0f},
            }) ||
            !event_push((SDL_Event){
                .text = {.type = SDL_EVENT_TEXT_INPUT, .text = "hÃ©"},
            }) ||
            !event_push((SDL_Event){
                .text = {.type = SDL_EVENT_TEXT_INPUT, .text = "llo"},
            }) ||
            !event_push((SDL_Event){
                .edit = {.type = SDL_EVENT_TEXT_EDITING,
                    .text = "composition", .start = 2, .length = 3},
            }) ||
            !event_push((SDL_Event){
                .edit_candidates = {
                    .type = SDL_EVENT_TEXT_EDITING_CANDIDATES,
                    .candidates = candidates, .num_candidates = 2,
                    .selected_candidate = 1, .horizontal = true},
            })) return false;
    events_drain();

    value = rohr_input_mouse_position_get();
    if(!rohr_input_key_down_check(SDL_SCANCODE_A) ||
            !rohr_input_key_pressed_check(SDL_SCANCODE_A) ||
            rohr_input_key_released_check(SDL_SCANCODE_A) ||
            rohr_input_modifiers_get() != SDL_KMOD_SHIFT ||
            !rohr_input_mouse_button_down_check(INPUT_MOUSE_BUTTON_X1) ||
            !rohr_input_mouse_button_pressed_check(INPUT_MOUSE_BUTTON_X1) ||
            !close_float(value.x, 35.0f) || !close_float(value.y, 44.0f))
        return false;
    value = rohr_input_mouse_delta_get();
    if(!close_float(value.x, 8.0f) || !close_float(value.y, 2.0f)) return false;
    value = rohr_input_mouse_wheel_get();
    if(!close_float(value.x, -2.0f) || !close_float(value.y, 3.0f)) return false;
    text = rohr_input_text_state_get();
    if(strcmp(text.committed, "hÃ©llo") != 0 ||
            strcmp(text.composition, "composition") != 0 ||
            text.composition_start != 2 || text.composition_length != 3 ||
            text.committed_truncated || text.composition_truncated ||
            text.candidate_count != 2 || text.selected_candidate != 1 ||
            !text.candidates_horizontal || text.candidates_truncated ||
            strcmp(text.candidates[0], "candidate one") != 0 ||
            strcmp(text.candidates[1], "candidate two") != 0) return false;

    rohr_input_frame_begin();
    text = rohr_input_text_state_get();
    value = rohr_input_mouse_delta_get();
    if(rohr_input_key_pressed_check(SDL_SCANCODE_A) ||
            !rohr_input_key_down_check(SDL_SCANCODE_A) ||
            rohr_input_mouse_button_pressed_check(INPUT_MOUSE_BUTTON_X1) ||
            !close_float(value.x, 0.0f) || !close_float(value.y, 0.0f) ||
            text.committed[0] != '\0' || strcmp(text.composition, "composition") != 0 ||
            text.candidate_count != 2)
        return false;

    if(!key_event_push(SDL_EVENT_KEY_UP, SDL_SCANCODE_A, SDL_KMOD_NONE) ||
            !mouse_button_event_push(SDL_EVENT_MOUSE_BUTTON_UP,
                INPUT_MOUSE_BUTTON_X1, 35.0f, 44.0f)) return false;
    events_drain();
    if(rohr_input_key_down_check(SDL_SCANCODE_A) ||
            !rohr_input_key_released_check(SDL_SCANCODE_A) ||
            !rohr_input_mouse_button_released_check(INPUT_MOUSE_BUTTON_X1))
        return false;

    rohr_input_frame_begin();
    for(size_t i = 0; i < 20; i += 1)
        if(!event_push((SDL_Event){
            .text = {.type = SDL_EVENT_TEXT_INPUT,
                    .text = "0123456789abcdef"},
            })) return false;
    events_drain();
    text = rohr_input_text_state_get();
    return strlen(text.committed) == ROHR_INPUT_TEXT_CAPACITY - 1 &&
        text.committed_truncated;
}

static bool input_action_test(void) {
    InputControllerIdResult gameplay_result =
        rohr_input_controller_create((InputControllerDescription){.name = "gameplay", .enabled = true});
    InputControllerId gameplay;
    InputActionIdResult jump_result;
    InputActionIdResult throttle_result;
    InputActionIdResult move_result;
    InputActionIdResult look_result;
    InputActionIdResult zoom_result;
    InputActionIdResult shortcut_result;
    InputBindingListResult binding_list;
    InputAxis1DResult axis_1d;
    InputAxis2DResult axis_2d;
    const InputBinding jump_defaults[] = {
        {.name = "keyboard", .source = INPUT_BINDING_KEY,
            .input.key = SDL_SCANCODE_SPACE,
            .scale = {0.0f, 0.0f},
            .inverted_x = true, .inverted_y = true},
        {.name = "pointer", .source = INPUT_BINDING_MOUSE_BUTTON,
            .input.mouse_button = INPUT_MOUSE_BUTTON_LEFT},
    };
    const InputBinding throttle_bindings[] = {
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_D,
            .scale = {1.0f, 1.0f}},
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_A,
            .scale = {1.0f, 1.0f}, .inverted_x = true},
    };
    const InputBinding move_bindings[] = {
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_W,
            .affects_y = true, .scale = {1.0f, 1.0f},
            .direction = {0.5f, 1.0f}},
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_S,
            .affects_y = true, .scale = {1.0f, 1.0f},
            .direction = {0.0f, -1.0f}},
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_A,
            .affects_x = true, .scale = {1.0f, 1.0f},
            .direction = {-1.0f, 0.0f}},
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_D,
            .affects_x = true, .scale = {1.0f, 1.0f},
            .direction = {1.0f, 0.0f}},
    };
    const InputBinding look_bindings[] = {
        {.source = INPUT_BINDING_MOUSE_MOTION,
            .input.axis_component = INPUT_AXIS_COMPONENT_XY,
            .affects_x = true, .affects_y = true,
            .scale = {0.1f, 0.05f}, .inverted_y = true},
        {.source = INPUT_BINDING_MOUSE_MOTION,
            .input.axis_component = INPUT_AXIS_COMPONENT_Y,
            .affects_x = true, .scale = {0.05f, 1.0f}},
    };
    const InputBinding zoom_bindings[] = {
        {.source = INPUT_BINDING_MOUSE_WHEEL,
            .input.axis_component = INPUT_AXIS_COMPONENT_Y,
            .scale = {0.25f, 1.0f}, .inverted_x = true},
    };
    const InputBinding shortcut_bindings[] = {
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_Q,
            .modifiers = SDL_KMOD_CTRL},
    };
    InputBinding letter_binding = rohr_input_binding_key_create(SDLK_A);
    InputBinding number_binding = rohr_input_binding_key_create(SDLK_7);
    InputBinding physical_binding = rohr_input_binding_scancode_create(
        SDL_SCANCODE_W);

    if(rohr_error_check(gameplay_result) ||
            letter_binding.input.key != SDL_SCANCODE_A ||
            number_binding.input.key != SDL_SCANCODE_7 ||
            physical_binding.input.key != SDL_SCANCODE_W ||
            letter_binding.source != INPUT_BINDING_KEY ||
            letter_binding.scale.x != 1.0f ||
            letter_binding.scale.y != 1.0f ||
            !letter_binding.affects_x || !letter_binding.affects_y ||
            physical_binding.modifiers != SDL_KMOD_NONE) return false;
    gameplay = gameplay_result.result.value;
    if(!rohr_error_check(rohr_input_controller_create((InputControllerDescription){.name = "gameplay", .enabled = true})) ||
            !rohr_input_controller_enabled_check(gameplay)) return false;
    jump_result = rohr_input_action_create((InputActionDescription){.controller = gameplay, .name = "jump", .type = INPUT_ACTION_BUTTON});
    throttle_result = rohr_input_action_create((InputActionDescription){.controller = gameplay, .name = "throttle", .type = INPUT_ACTION_AXIS_1D});
    move_result = rohr_input_action_create((InputActionDescription){.controller = gameplay, .name = "move", .type = INPUT_ACTION_AXIS_2D});
    look_result = rohr_input_action_create((InputActionDescription){.controller = gameplay, .name = "look", .type = INPUT_ACTION_AXIS_2D});
    zoom_result = rohr_input_action_create((InputActionDescription){.controller = gameplay, .name = "zoom", .type = INPUT_ACTION_AXIS_1D});
    shortcut_result = rohr_input_action_create((InputActionDescription){.controller = gameplay, .name = "shortcut", .type = INPUT_ACTION_BUTTON});
    if(rohr_error_check(jump_result) || rohr_error_check(throttle_result) ||
            rohr_error_check(move_result) || rohr_error_check(look_result) ||
            rohr_error_check(zoom_result) || rohr_error_check(shortcut_result) ||
            rohr_error_check(rohr_input_action_bindings_default_set(
                jump_result.result.value, jump_defaults, 2)) ||
            rohr_error_check(rohr_input_action_bindings_default_set(
                throttle_result.result.value, throttle_bindings, 2)) ||
            rohr_error_check(rohr_input_action_bindings_default_set(
                move_result.result.value, move_bindings, 4)) ||
            rohr_error_check(rohr_input_action_bindings_default_set(
                look_result.result.value, look_bindings, 2)) ||
            rohr_error_check(rohr_input_action_bindings_default_set(
                zoom_result.result.value, zoom_bindings, 1)) ||
            rohr_error_check(rohr_input_action_bindings_default_set(
                shortcut_result.result.value, shortcut_bindings, 1))) return false;
    binding_list = rohr_input_action_bindings_default_get(
        jump_result.result.value);
    if(rohr_error_check(binding_list) || binding_list.result.value.count != 2 ||
            strcmp(binding_list.result.value.values[0].name, "keyboard") != 0 ||
            strcmp(binding_list.result.value.values[1].name, "pointer") != 0)
        return false;
    {
        const InputBinding duplicates[] = {
            {.name = "duplicate", .source = INPUT_BINDING_KEY,
                .input.key = SDL_SCANCODE_SPACE},
            {.name = "duplicate", .source = INPUT_BINDING_KEY,
                .input.key = SDL_SCANCODE_RETURN},
        };
        EngineResult duplicate_result = rohr_input_action_bindings_default_set(
            jump_result.result.value, duplicates, 2);
        if(!rohr_error_check(duplicate_result) ||
                duplicate_result.result.error != ERROR_ENGINE_DUPLICATE_INPUT_NAME)
            return false;
    }
    {
        InputControllerIdResult controller_lookup =
            rohr_input_controller_by_name_get("gameplay");
        InputActionIdResult action_lookup =
            rohr_input_action_by_name_get(gameplay, "move");
        if(rohr_error_check(controller_lookup) ||
                controller_lookup.result.value != gameplay ||
                rohr_error_check(action_lookup) ||
                action_lookup.result.value != move_result.result.value)
            return false;
    }
    {
        InputBinding invalid = {.source = INPUT_BINDING_MOUSE_MOTION,
            .input.axis_component = INPUT_AXIS_COMPONENT_X};
        InputActionTypeResult type = rohr_input_action_type_get(
            move_result.result.value);
        if(rohr_error_check(type) || type.result.value != INPUT_ACTION_AXIS_2D ||
                !rohr_error_check(rohr_input_action_bindings_default_set(
                    jump_result.result.value, &invalid, 1)) ||
                !rohr_error_check(rohr_input_action_bindings_default_set(
                    move_result.result.value, move_bindings,
                    ROHR_INPUT_BINDING_LIMIT + 1)) ||
                !rohr_error_check(rohr_input_action_axis_1d_get(
                    move_result.result.value))) return false;
    }

    rohr_input_frame_begin();
    if(!key_event_push(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_SPACE, SDL_KMOD_NONE))
        return false;
    events_drain();
    if(!rohr_input_action_button_down_check(jump_result.result.value) ||
            !rohr_input_action_button_pressed_check(jump_result.result.value) ||
            rohr_input_action_button_released_check(jump_result.result.value))
        return false;
    if(!mouse_button_event_push(SDL_EVENT_MOUSE_BUTTON_DOWN,
            INPUT_MOUSE_BUTTON_LEFT, 0.0f, 0.0f) ||
            !key_event_push(SDL_EVENT_KEY_UP, SDL_SCANCODE_SPACE, SDL_KMOD_NONE))
        return false;
    events_drain();
    if(!rohr_input_action_button_down_check(jump_result.result.value) ||
            rohr_input_action_button_released_check(jump_result.result.value))
        return false;
    if(!mouse_button_event_push(SDL_EVENT_MOUSE_BUTTON_UP,
            INPUT_MOUSE_BUTTON_LEFT, 0.0f, 0.0f)) return false;
    events_drain();
    if(rohr_input_action_button_down_check(jump_result.result.value) ||
            !rohr_input_action_button_pressed_check(jump_result.result.value) ||
            !rohr_input_action_button_released_check(jump_result.result.value))
        return false;

    rohr_input_frame_begin();
    if(!key_event_push(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, SDL_KMOD_NONE) ||
            !key_event_push(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_D, SDL_KMOD_NONE) ||
            !event_push((SDL_Event){
                .motion = {.type = SDL_EVENT_MOUSE_MOTION,
                    .xrel = 8.0f, .yrel = -6.0f},
            }) ||
            !event_push((SDL_Event){
                .wheel = {.type = SDL_EVENT_MOUSE_WHEEL,
                    .y = 2.0f, .direction = SDL_MOUSEWHEEL_NORMAL},
            })) return false;
    events_drain();
    axis_1d = rohr_input_action_axis_1d_get(throttle_result.result.value);
    axis_2d = rohr_input_action_axis_2d_get(move_result.result.value);
    if(rohr_error_check(axis_1d) || !close_float(axis_1d.result.value, 1.0f) ||
            rohr_error_check(axis_2d) ||
            !close_float(axis_2d.result.value.x, 0.7071067f) ||
            !close_float(axis_2d.result.value.y, 0.7071067f)) return false;
    axis_2d = rohr_input_action_axis_2d_get(look_result.result.value);
    if(rohr_error_check(axis_2d) ||
            !close_float(axis_2d.result.value.x, 0.5f) ||
            !close_float(axis_2d.result.value.y, 0.3f)) return false;
    axis_1d = rohr_input_action_axis_1d_get(zoom_result.result.value);
    if(rohr_error_check(axis_1d) || !close_float(axis_1d.result.value, -0.5f))
        return false;
    if(!key_event_push(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_A, SDL_KMOD_NONE))
        return false;
    events_drain();
    axis_1d = rohr_input_action_axis_1d_get(throttle_result.result.value);
    axis_2d = rohr_input_action_axis_2d_get(move_result.result.value);
    if(rohr_error_check(axis_1d) || !close_float(axis_1d.result.value, 0.0f) ||
            rohr_error_check(axis_2d) || !close_float(axis_2d.result.value.x, 0.0f) ||
            !close_float(axis_2d.result.value.y, 1.0f)) return false;

    rohr_input_frame_begin();
    if(!key_event_push(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_Q, SDL_KMOD_NONE))
        return false;
    events_drain();
    if(rohr_input_action_button_down_check(shortcut_result.result.value) ||
            !key_event_push(SDL_EVENT_KEY_UP, SDL_SCANCODE_Q, SDL_KMOD_NONE) ||
            !key_event_push(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_Q, SDL_KMOD_CTRL))
        return false;
    events_drain();
    if(!rohr_input_action_button_down_check(shortcut_result.result.value) ||
            !rohr_input_action_button_pressed_check(shortcut_result.result.value))
        return false;

    if(rohr_error_check(rohr_input_controller_enabled_set(gameplay, false)))
        return false;
    axis_2d = rohr_input_action_axis_2d_get(move_result.result.value);
    if(rohr_input_action_button_down_check(jump_result.result.value) ||
            rohr_error_check(axis_2d) || axis_2d.result.value.x != 0.0f ||
            axis_2d.result.value.y != 0.0f) return false;
    if(rohr_error_check(rohr_input_controller_enabled_set(gameplay, true)))
        return false;

    {
        const InputBinding override[] = {{.name = "confirm",
            .source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_RETURN}};
        if(rohr_error_check(rohr_input_action_bindings_override_set(
                    jump_result.result.value, override, 1)) ||
                !rohr_input_action_bindings_override_check(jump_result.result.value))
            return false;
        binding_list = rohr_input_action_bindings_effective_get(
            jump_result.result.value);
        if(rohr_error_check(binding_list) || binding_list.result.value.count != 1 ||
                binding_list.result.value.values[0].input.key != SDL_SCANCODE_RETURN ||
                strcmp(binding_list.result.value.values[0].name, "confirm") != 0 ||
                rohr_error_check(rohr_input_action_bindings_override_clear(
                    jump_result.result.value))) return false;
        binding_list = rohr_input_action_bindings_effective_get(
            jump_result.result.value);
        if(rohr_error_check(binding_list) || binding_list.result.value.count != 2 ||
                rohr_input_action_bindings_override_check(jump_result.result.value))
            return false;
    }

    {
        InputActionId stale = jump_result.result.value;
        if(rohr_error_check(rohr_input_action_destroy(stale)) ||
                !rohr_error_check(rohr_input_action_axis_1d_get(stale))) return false;
        jump_result = rohr_input_action_create((InputActionDescription){.controller = gameplay, .name = "jump", .type = INPUT_ACTION_BUTTON});
        if(rohr_error_check(jump_result) || jump_result.result.value == stale ||
                rohr_input_action_button_down_check(stale)) return false;
    }
    return !rohr_error_check(rohr_input_controller_destroy(gameplay)) &&
        rohr_error_check(rohr_input_action_by_name_get(gameplay, "move"));
}

static bool input_capacity_test(void) {
    InputControllerId controllers[ROHR_INPUT_CONTROLLER_LIMIT];
    InputControllerId action_controller;
    char name[32];
    for(size_t i = 0; i < ROHR_INPUT_CONTROLLER_LIMIT; i += 1) {
        InputControllerIdResult result;
        snprintf(name, sizeof(name), "controller_%zu", i);
        InputControllerDescription description = {.enabled = true};
        snprintf(description.name, sizeof(description.name), "%s", name);
        result = rohr_input_controller_create(description);
        if(rohr_error_check(result)) return false;
        controllers[i] = result.result.value;
    }
    if(!rohr_error_check(rohr_input_controller_create((InputControllerDescription){.name = "overflow", .enabled = true}))) return false;
    InputControllerDescriptionResult full_controller =
        rohr_input_controller_description_get(controllers[0]);
    if(rohr_error_check(full_controller) || rohr_error_check(
            rohr_input_controller_description_set(controllers[0], full_controller.result.value))) return false;
    for(size_t i = 0; i < ROHR_INPUT_CONTROLLER_LIMIT; i += 1)
        if(rohr_error_check(rohr_input_controller_destroy(controllers[i]))) return false;

    {
        InputControllerIdResult result =
            rohr_input_controller_create((InputControllerDescription){.name = "action_capacity", .enabled = true});
        if(rohr_error_check(result)) return false;
        action_controller = result.result.value;
    }
    for(size_t i = 0; i < ROHR_INPUT_ACTION_LIMIT; i += 1) {
        snprintf(name, sizeof(name), "action_%zu", i);
        InputActionDescription description = {.controller = action_controller, .type = INPUT_ACTION_BUTTON};
        snprintf(description.name, sizeof(description.name), "%s", name);
        if(rohr_error_check(rohr_input_action_create(description))) return false;
    }
    if(!rohr_error_check(rohr_input_action_create((InputActionDescription){.controller = action_controller, .name = "overflow", .type = INPUT_ACTION_BUTTON}))) return false;
    InputActionIdResult existing = rohr_input_action_by_name_get(action_controller, "action_0");
    if(rohr_error_check(existing)) return false;
    InputActionDescriptionResult full_action = rohr_input_action_description_get(existing.result.value);
    InputControllerIdResult destination = rohr_input_controller_create(
        (InputControllerDescription){.name = "capacity_destination", .enabled = true});
    if(rohr_error_check(full_action) || rohr_error_check(destination)) return false;
    full_action.result.value.controller = destination.result.value;
    if(rohr_error_check(rohr_input_action_description_set(existing.result.value, full_action.result.value)) ||
            rohr_error_check(rohr_input_controller_destroy(action_controller)) ||
            rohr_error_check(rohr_input_action_description_get(existing.result.value))) return false;
    return !rohr_error_check(rohr_input_controller_destroy(destination.result.value));
}

static bool input_persistent_button_test(void) {
    InputControllerIdResult controller_result =
        rohr_input_controller_create((InputControllerDescription){.name = "persistent_buttons", .enabled = true});
    InputActionIdResult toggle_result;
    InputActionIdResult axis_result;
    InputButtonModeResult mode_result;
    const InputBinding bindings[] = {
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_E},
        {.source = INPUT_BINDING_MOUSE_BUTTON,
            .input.mouse_button = INPUT_MOUSE_BUTTON_RIGHT},
    };
    const InputBinding override[] = {
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_R},
    };
    InputControllerId controller;
    InputActionId toggle;

    if(rohr_error_check(controller_result)) return false;
    controller = controller_result.result.value;
    toggle_result = rohr_input_action_create((InputActionDescription){.controller = controller, .name = "console", .type = INPUT_ACTION_BUTTON});
    axis_result = rohr_input_action_create((InputActionDescription){.controller = controller, .name = "axis", .type = INPUT_ACTION_AXIS_1D});
    if(rohr_error_check(toggle_result) || rohr_error_check(axis_result)) return false;
    toggle = toggle_result.result.value;
    mode_result = rohr_input_action_button_mode_get(toggle);
    if(rohr_error_check(mode_result) ||
            mode_result.result.value != INPUT_BUTTON_MOMENTARY ||
            !rohr_error_check(rohr_input_action_button_mode_set(
                toggle, (InputButtonMode)99)) ||
            !rohr_error_check(rohr_input_action_button_mode_set(
                axis_result.result.value, INPUT_BUTTON_PERSISTENT)) ||
            rohr_error_check(rohr_input_action_button_mode_set(
                toggle, INPUT_BUTTON_PERSISTENT)) ||
            rohr_error_check(rohr_input_action_button_initial_state_set(
                toggle, true)) ||
            !rohr_input_action_button_initial_state_check(toggle) ||
            rohr_error_check(rohr_input_action_bindings_default_set(
                toggle, bindings, 2)) ||
            !rohr_input_action_button_down_check(toggle) ||
            rohr_input_action_button_pressed_check(toggle) ||
            rohr_input_action_button_released_check(toggle)) return false;

    rohr_input_frame_begin();
    if(!key_event_push(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_E, SDL_KMOD_NONE))
        return false;
    events_drain();
    if(rohr_input_action_button_down_check(toggle) ||
            !rohr_input_action_button_released_check(toggle)) return false;
    if(!mouse_button_event_push(SDL_EVENT_MOUSE_BUTTON_DOWN,
            INPUT_MOUSE_BUTTON_RIGHT, 0.0f, 0.0f) ||
            !key_event_push(SDL_EVENT_KEY_UP, SDL_SCANCODE_E, SDL_KMOD_NONE))
        return false;
    events_drain();
    if(rohr_input_action_button_down_check(toggle) ||
            rohr_input_action_button_pressed_check(toggle)) return false;
    if(!mouse_button_event_push(SDL_EVENT_MOUSE_BUTTON_UP,
            INPUT_MOUSE_BUTTON_RIGHT, 0.0f, 0.0f)) return false;
    events_drain();
    if(rohr_input_action_button_down_check(toggle)) return false;

    rohr_input_frame_begin();
    if(!mouse_button_event_push(SDL_EVENT_MOUSE_BUTTON_DOWN,
            INPUT_MOUSE_BUTTON_RIGHT, 0.0f, 0.0f)) return false;
    events_drain();
    if(!rohr_input_action_button_down_check(toggle) ||
            !rohr_input_action_button_pressed_check(toggle)) return false;
    rohr_input_frame_begin();
    if(rohr_error_check(rohr_input_controller_enabled_set(controller, false)) ||
            rohr_input_action_button_down_check(toggle) ||
            !mouse_button_event_push(SDL_EVENT_MOUSE_BUTTON_UP,
                INPUT_MOUSE_BUTTON_RIGHT, 0.0f, 0.0f) ||
            !key_event_push(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_E, SDL_KMOD_NONE))
        return false;
    events_drain();
    if(rohr_input_action_button_down_check(toggle) ||
            rohr_input_action_button_pressed_check(toggle) ||
            rohr_input_action_button_released_check(toggle) ||
            rohr_error_check(rohr_input_controller_enabled_set(controller, true)) ||
            !rohr_input_action_button_down_check(toggle) ||
            rohr_input_action_button_pressed_check(toggle) ||
            rohr_input_action_button_released_check(toggle)) return false;
    if(!key_event_push(SDL_EVENT_KEY_UP, SDL_SCANCODE_E, SDL_KMOD_NONE))
        return false;
    events_drain();
    rohr_input_frame_begin();
    if(!key_event_push(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_E, SDL_KMOD_NONE))
        return false;
    events_drain();
    if(rohr_input_action_button_down_check(toggle) ||
            !rohr_input_action_button_released_check(toggle)) return false;

    rohr_input_frame_begin();
    if(rohr_error_check(rohr_input_action_bindings_override_set(
                toggle, override, 1)) ||
            rohr_input_action_button_pressed_check(toggle) ||
            rohr_input_action_button_released_check(toggle) ||
            rohr_error_check(rohr_input_action_bindings_override_clear(toggle)) ||
            rohr_input_action_button_pressed_check(toggle) ||
            rohr_input_action_button_released_check(toggle)) return false;
    if(!key_event_push(SDL_EVENT_KEY_UP, SDL_SCANCODE_E, SDL_KMOD_NONE))
        return false;
    events_drain();

    rohr_input_frame_begin();
    if(rohr_error_check(rohr_input_action_button_state_set(toggle, true)) ||
            !rohr_input_action_button_down_check(toggle) ||
            !rohr_input_action_button_pressed_check(toggle)) return false;
    rohr_input_frame_begin();
    if(rohr_error_check(rohr_input_action_button_state_set(toggle, false)) ||
            rohr_input_action_button_down_check(toggle) ||
            !rohr_input_action_button_released_check(toggle)) return false;
    rohr_input_frame_begin();
    if(rohr_error_check(rohr_input_action_button_state_reset(toggle)) ||
            !rohr_input_action_button_down_check(toggle) ||
            !rohr_input_action_button_pressed_check(toggle) ||
            rohr_error_check(rohr_input_action_button_mode_set(
                toggle, INPUT_BUTTON_MOMENTARY)) ||
            !rohr_error_check(rohr_input_action_button_state_set(toggle, true)))
        return false;

    return !rohr_error_check(rohr_input_controller_destroy(controller));
}

static bool input_window_mode_test(void) {
    EngineResult result;
    InputTextState text;
    if(rohr_error_check(rohr_graphics_start())) return false;
    result = rohr_input_text_start();
    if(rohr_error_check(result)) goto fail;
    text = rohr_input_text_state_get();
    if(!text.active) goto fail;
    result = rohr_input_text_area_set((SDL_Rect){12, 20, 160, 28}, 4);
    if(rohr_error_check(result)) goto fail;
    result = rohr_input_text_stop();
    if(rohr_error_check(result) || rohr_input_text_state_get().active) goto fail;
    result = rohr_input_mouse_relative_mode_set(true);
    if(rohr_error_check(result) || !rohr_input_mouse_relative_mode_check()) goto fail;
    result = rohr_input_mouse_relative_mode_set(false);
    if(rohr_error_check(result) || rohr_input_mouse_relative_mode_check()) goto fail;
    rohr_graphics_recording_stop();
    rohr_graphics_stop();
    return true;
fail:
    rohr_graphics_recording_stop();
    rohr_graphics_stop();
    return false;
}

static bool input_focus_release_test(void) {
    rohr_input_frame_begin();
    if(!key_event_push(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_Q, SDL_KMOD_NONE) ||
            !mouse_button_event_push(SDL_EVENT_MOUSE_BUTTON_DOWN,
                INPUT_MOUSE_BUTTON_RIGHT, 0.0f, 0.0f)) return false;
    events_drain();
    if(!event_push((SDL_Event){
        .window = {.type = SDL_EVENT_WINDOW_FOCUS_LOST}})) return false;
    events_drain();
    return !rohr_input_key_down_check(SDL_SCANCODE_Q) &&
        rohr_input_key_released_check(SDL_SCANCODE_Q) &&
        !rohr_input_mouse_button_down_check(INPUT_MOUSE_BUTTON_RIGHT) &&
        rohr_input_mouse_button_released_check(INPUT_MOUSE_BUTTON_RIGHT);
}

#define REQUIRE(condition) do { if(!(condition)) { \
    fprintf(stderr, "input description check failed at %d: %s\n", __LINE__, #condition); \
    return false; } } while(0)

static bool input_description_test(void) {
    InputControllerDescription cd = rohr_input_controller_description_default_get();
    InputActionDescription ad = rohr_input_action_description_default_get();
    REQUIRE(cd.enabled && cd.name[0] == '\0');
    REQUIRE(ad.type == INPUT_ACTION_BUTTON && ad.button_mode == INPUT_BUTTON_MOMENTARY &&
        !ad.button_initial_state && ad.bindings_default.count == 0);
    REQUIRE(rohr_error_check(rohr_input_controller_create((InputControllerDescription){0})));
    InputControllerIdResult cr = rohr_input_controller_create((InputControllerDescription){.name = "desc_disabled"});
    REQUIRE(!rohr_error_check(cr));
    InputControllerId disabled = cr.result.value;
    REQUIRE(!rohr_input_controller_enabled_check(disabled));
    cr = rohr_input_controller_create((InputControllerDescription){.name = "desc_source", .enabled = true});
    REQUIRE(!rohr_error_check(cr));
    InputControllerId source = cr.result.value;
    cr = rohr_input_controller_create((InputControllerDescription){.name = "desc_dest", .enabled = true});
    REQUIRE(!rohr_error_check(cr));
    InputControllerId dest = cr.result.value;
    InputActionDescription config = {.controller = source, .name = "toggle",
        .button_mode = INPUT_BUTTON_PERSISTENT,
        .bindings_default = {.count = 1, .values = {{.name = "key", .source = INPUT_BINDING_KEY,
            .input.key = SDL_SCANCODE_F8}}}};
    InputActionIdResult ar = rohr_input_action_create(config);
    REQUIRE(!rohr_error_check(ar));
    InputActionId action = ar.result.value;
    ar = rohr_input_action_create((InputActionDescription){.controller = source, .name = "other",
        .button_mode = INPUT_BUTTON_PERSISTENT});
    REQUIRE(!rohr_error_check(ar));
    InputActionId other = ar.result.value;
    rohr_input_frame_begin();
    REQUIRE(key_event_push(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_F8, SDL_KMOD_NONE));
    events_drain();
    REQUIRE(rohr_input_action_button_down_check(action) && rohr_input_action_button_pressed_check(action));
    REQUIRE(!rohr_error_check(rohr_input_action_button_state_set(other, true)));
    InputActionDescriptionResult got = rohr_input_action_description_get(action);
    REQUIRE(!rohr_error_check(got));
    REQUIRE(!rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    REQUIRE(rohr_input_action_button_pressed_check(action));
    // Invalid complete edits leave both configuration and pending events intact.
    for(int invalid_case = 0; invalid_case < 6; invalid_case += 1) {
        InputActionDescription invalid = got.result.value;
        switch(invalid_case) {
            case 0: invalid.controller = INPUT_CONTROLLER_INVALID; break;
            case 1: invalid.type = (InputActionType)99; break;
            case 2: invalid.button_mode = (InputButtonMode)99; break;
            case 3: memset(invalid.name, 'x', sizeof(invalid.name)); break;
            case 4: invalid.bindings_default.values[0].scale.x = NAN; break;
            case 5: invalid.name[0] = '\0'; break;
        }
        REQUIRE(rohr_error_check(rohr_input_action_description_set(action, invalid)));
        REQUIRE(rohr_input_action_by_name_get(source, "toggle").result.value == action);
        REQUIRE(rohr_input_action_button_pressed_check(action));
    }
    strcpy(got.result.value.name, "renamed");
    strcpy(got.result.value.bindings_default.values[0].name, "renamed_key");
    REQUIRE(!rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    REQUIRE(rohr_input_action_button_pressed_check(action) && rohr_input_action_button_down_check(action));
    REQUIRE(rohr_error_check(rohr_input_action_by_name_get(source, "toggle")));
    REQUIRE(rohr_input_action_by_name_get(source, "renamed").result.value == action);
    InputControllerDescriptionResult cg = rohr_input_controller_description_get(source);
    REQUIRE(!rohr_error_check(cg));
    REQUIRE(!rohr_error_check(rohr_input_controller_description_set(source, cg.result.value)));
    strcpy(cg.result.value.name, "source_renamed");
    REQUIRE(!rohr_error_check(rohr_input_controller_description_set(source, cg.result.value)));
    REQUIRE(rohr_input_action_button_pressed_check(action) && rohr_input_action_button_pressed_check(other));
    strcpy(cg.result.value.name, "desc_dest");
    REQUIRE(rohr_error_check(rohr_input_controller_description_set(source, cg.result.value)));
    REQUIRE(rohr_input_controller_by_name_get("source_renamed").result.value == source);
    memset(cg.result.value.name, 'x', sizeof(cg.result.value.name));
    REQUIRE(rohr_error_check(rohr_input_controller_description_set(source, cg.result.value)));
    got.result.value.bindings_default.values[0].input.key = SDL_SCANCODE_F9;
    REQUIRE(!rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    REQUIRE(rohr_input_action_button_down_check(action) && !rohr_input_action_button_pressed_check(action));
    REQUIRE(rohr_input_action_button_pressed_check(other));
    // Destination collisions reject the whole move without losing the old owner.
    ar = rohr_input_action_create((InputActionDescription){.controller = dest, .name = "renamed"});
    REQUIRE(!rohr_error_check(ar));
    got.result.value.controller = dest;
    REQUIRE(rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    REQUIRE(rohr_input_action_description_get(action).result.value.controller == source);
    REQUIRE(!rohr_error_check(rohr_input_action_destroy(ar.result.value)));
    REQUIRE(!rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    REQUIRE(rohr_input_action_by_name_get(dest, "renamed").result.value == action);
    REQUIRE(rohr_input_action_button_down_check(action) && !rohr_input_action_button_pressed_check(action));
    REQUIRE(!rohr_error_check(rohr_input_controller_destroy(source)));
    REQUIRE(!rohr_error_check(rohr_input_action_description_get(action)));
    REQUIRE(rohr_error_check(rohr_input_action_description_get(other)));
    // Explicit empty overrides stay active through full replacements.
    REQUIRE(!rohr_error_check(rohr_input_action_bindings_override_set(action, NULL, 0)));
    got = rohr_input_action_description_get(action);
    got.result.value.bindings_default.values[0].input.key = SDL_SCANCODE_F8;
    REQUIRE(!rohr_error_check(rohr_input_action_button_state_set(action, false)));
    REQUIRE(rohr_input_action_button_released_check(action));
    REQUIRE(!rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    REQUIRE(rohr_input_action_bindings_override_check(action));
    REQUIRE(rohr_input_action_bindings_effective_get(action).result.value.count == 0);
    REQUIRE(rohr_input_action_button_released_check(action));
    // Resetting configured initial state clears transitions, even while the key is held.
    got.result.value.button_initial_state = true;
    REQUIRE(!rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    REQUIRE(rohr_input_action_button_down_check(action) && !rohr_input_action_button_released_check(action));
    REQUIRE(!rohr_error_check(rohr_input_action_bindings_override_clear(action)));
    got.result.value.type = INPUT_ACTION_AXIS_1D;
    got.result.value.bindings_default.count = 0;
    REQUIRE(!rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    got = rohr_input_action_description_get(action);
    REQUIRE(got.result.value.button_mode == INPUT_BUTTON_MOMENTARY && !got.result.value.button_initial_state);
    InputBinding motion = {.name = "motion", .source = INPUT_BINDING_MOUSE_MOTION,
        .affects_x = true, .scale = {1, 1}};
    REQUIRE(!rohr_error_check(rohr_input_action_bindings_override_set(action, &motion, 1)));
    got.result.value.type = INPUT_ACTION_BUTTON;
    strcpy(got.result.value.name, "rejected");
    REQUIRE(rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    REQUIRE(rohr_input_action_description_get(action).result.value.type == INPUT_ACTION_AXIS_1D);
    REQUIRE(rohr_input_action_by_name_get(dest, "renamed").result.value == action);
    REQUIRE(rohr_input_action_bindings_effective_get(action).result.value.values[0].source == INPUT_BINDING_MOUSE_MOTION);
    REQUIRE(!rohr_error_check(rohr_input_action_bindings_override_clear(action)));
    REQUIRE(!rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    got = rohr_input_action_description_get(action);
    got.result.value.bindings_default.count = ROHR_INPUT_BINDING_LIMIT + 1;
    REQUIRE(rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    REQUIRE(rohr_input_action_description_get(action).result.value.bindings_default.count == 0);
    got.result.value.bindings_default = config.bindings_default;
    got.result.value.bindings_default.count = 2;
    got.result.value.bindings_default.values[1] = config.bindings_default.values[0];
    REQUIRE(rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    got.result.value.bindings_default.count = 1;
    got.result.value.bindings_default.values[0].input.key = SDL_SCANCODE_F8;
    REQUIRE(!rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    REQUIRE(rohr_input_action_button_down_check(action) && !rohr_input_action_button_pressed_check(action));
    got.result.value.button_mode = INPUT_BUTTON_PERSISTENT;
    REQUIRE(!rohr_error_check(rohr_input_action_description_set(action, got.result.value)));
    REQUIRE(!rohr_input_action_button_down_check(action) && !rohr_input_action_button_released_check(action));
    // Held-key creation seeds the baseline without toggling on an unrelated input.
    config.controller = dest;
    ar = rohr_input_action_create(config);
    REQUIRE(!rohr_error_check(ar));
    REQUIRE(key_event_push(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_F10, SDL_KMOD_NONE));
    events_drain();
    REQUIRE(!rohr_input_action_button_down_check(ar.result.value));
    REQUIRE(!rohr_input_action_button_down_check(action));
    REQUIRE(key_event_push(SDL_EVENT_KEY_UP, SDL_SCANCODE_F8, SDL_KMOD_NONE));
    REQUIRE(key_event_push(SDL_EVENT_KEY_UP, SDL_SCANCODE_F10, SDL_KMOD_NONE));
    events_drain();
    REQUIRE(!rohr_error_check(rohr_input_controller_destroy(dest)));
    REQUIRE(rohr_error_check(rohr_input_action_description_get(action)));
    REQUIRE(!rohr_error_check(rohr_input_controller_destroy(disabled)));
    return true;
}
#undef REQUIRE

int main(void) {
    if(rohr_error_check(rohr_engine_start())) return 1;
    if(!input_raw_snapshot_test() || !input_action_test() ||
            !input_persistent_button_test() || !input_focus_release_test() ||
            !input_description_test() || !input_capacity_test() ||
            !input_window_mode_test()) {
        fprintf(stderr, "input action test failed\n");
        rohr_engine_stop();
        return 1;
    }
    rohr_engine_stop();
    return 0;
}
