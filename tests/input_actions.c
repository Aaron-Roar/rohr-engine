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
        .type = type,
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
        .type = type,
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
                .type = SDL_EVENT_MOUSE_MOTION,
                .motion = {.type = SDL_EVENT_MOUSE_MOTION,
                    .x = 30.0f, .y = 40.0f, .xrel = 3.0f, .yrel = -2.0f},
            }) ||
            !event_push((SDL_Event){
                .type = SDL_EVENT_MOUSE_MOTION,
                .motion = {.type = SDL_EVENT_MOUSE_MOTION,
                    .x = 35.0f, .y = 44.0f, .xrel = 5.0f, .yrel = 4.0f},
            }) ||
            !mouse_button_event_push(SDL_EVENT_MOUSE_BUTTON_DOWN,
                INPUT_MOUSE_BUTTON_X1, 35.0f, 44.0f) ||
            !event_push((SDL_Event){
                .type = SDL_EVENT_MOUSE_WHEEL,
                .wheel = {.type = SDL_EVENT_MOUSE_WHEEL,
                    .x = 2.0f, .y = -3.0f,
                    .direction = SDL_MOUSEWHEEL_FLIPPED,
                    .mouse_x = 35.0f, .mouse_y = 44.0f},
            }) ||
            !event_push((SDL_Event){
                .type = SDL_EVENT_TEXT_INPUT,
                .text = {.type = SDL_EVENT_TEXT_INPUT, .text = "hÃ©"},
            }) ||
            !event_push((SDL_Event){
                .type = SDL_EVENT_TEXT_INPUT,
                .text = {.type = SDL_EVENT_TEXT_INPUT, .text = "llo"},
            }) ||
            !event_push((SDL_Event){
                .type = SDL_EVENT_TEXT_EDITING,
                .edit = {.type = SDL_EVENT_TEXT_EDITING,
                    .text = "composition", .start = 2, .length = 3},
            }) ||
            !event_push((SDL_Event){
                .type = SDL_EVENT_TEXT_EDITING_CANDIDATES,
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
                .type = SDL_EVENT_TEXT_INPUT,
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
        rohr_input_controller_create("gameplay");
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
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_SPACE},
        {.source = INPUT_BINDING_MOUSE_BUTTON,
            .input.mouse_button = INPUT_MOUSE_BUTTON_LEFT},
    };
    const InputBinding throttle_bindings[] = {
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_D,
            .scale = 1.0f},
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_A,
            .scale = 1.0f, .inverted = true},
    };
    const InputBinding move_bindings[] = {
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_W,
            .scale = 1.0f, .direction = {0.0f, 1.0f}},
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_S,
            .scale = 1.0f, .direction = {0.0f, -1.0f}},
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_A,
            .scale = 1.0f, .direction = {-1.0f, 0.0f}},
        {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_D,
            .scale = 1.0f, .direction = {1.0f, 0.0f}},
    };
    const InputBinding look_bindings[] = {
        {.source = INPUT_BINDING_MOUSE_MOTION,
            .input.axis_component = INPUT_AXIS_COMPONENT_XY, .scale = 0.1f},
    };
    const InputBinding zoom_bindings[] = {
        {.source = INPUT_BINDING_MOUSE_WHEEL,
            .input.axis_component = INPUT_AXIS_COMPONENT_Y,
            .scale = 0.25f, .inverted = true},
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
            letter_binding.scale != 1.0f ||
            physical_binding.modifiers != SDL_KMOD_NONE) return false;
    gameplay = gameplay_result.result.value;
    if(!rohr_error_check(rohr_input_controller_create("gameplay")) ||
            !rohr_input_controller_enabled_check(gameplay)) return false;
    jump_result = rohr_input_action_create(gameplay, "jump", INPUT_ACTION_BUTTON);
    throttle_result = rohr_input_action_create(
        gameplay, "throttle", INPUT_ACTION_AXIS_1D);
    move_result = rohr_input_action_create(gameplay, "move", INPUT_ACTION_AXIS_2D);
    look_result = rohr_input_action_create(gameplay, "look", INPUT_ACTION_AXIS_2D);
    zoom_result = rohr_input_action_create(gameplay, "zoom", INPUT_ACTION_AXIS_1D);
    shortcut_result = rohr_input_action_create(
        gameplay, "shortcut", INPUT_ACTION_BUTTON);
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
                look_result.result.value, look_bindings, 1)) ||
            rohr_error_check(rohr_input_action_bindings_default_set(
                zoom_result.result.value, zoom_bindings, 1)) ||
            rohr_error_check(rohr_input_action_bindings_default_set(
                shortcut_result.result.value, shortcut_bindings, 1))) return false;
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
                .type = SDL_EVENT_MOUSE_MOTION,
                .motion = {.type = SDL_EVENT_MOUSE_MOTION,
                    .xrel = 8.0f, .yrel = -6.0f},
            }) ||
            !event_push((SDL_Event){
                .type = SDL_EVENT_MOUSE_WHEEL,
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
            !close_float(axis_2d.result.value.x, 0.8f) ||
            !close_float(axis_2d.result.value.y, -0.6f)) return false;
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
        const InputBinding override[] = {{.source = INPUT_BINDING_KEY,
            .input.key = SDL_SCANCODE_RETURN}};
        if(rohr_error_check(rohr_input_action_bindings_override_set(
                    jump_result.result.value, override, 1)) ||
                !rohr_input_action_bindings_override_check(jump_result.result.value))
            return false;
        binding_list = rohr_input_action_bindings_effective_get(
            jump_result.result.value);
        if(rohr_error_check(binding_list) || binding_list.result.value.count != 1 ||
                binding_list.result.value.values[0].input.key != SDL_SCANCODE_RETURN ||
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
        jump_result = rohr_input_action_create(gameplay, "jump", INPUT_ACTION_BUTTON);
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
        result = rohr_input_controller_create(name);
        if(rohr_error_check(result)) return false;
        controllers[i] = result.result.value;
    }
    if(!rohr_error_check(rohr_input_controller_create("overflow"))) return false;
    for(size_t i = 0; i < ROHR_INPUT_CONTROLLER_LIMIT; i += 1)
        if(rohr_error_check(rohr_input_controller_destroy(controllers[i]))) return false;

    {
        InputControllerIdResult result =
            rohr_input_controller_create("action_capacity");
        if(rohr_error_check(result)) return false;
        action_controller = result.result.value;
    }
    for(size_t i = 0; i < ROHR_INPUT_ACTION_LIMIT; i += 1) {
        snprintf(name, sizeof(name), "action_%zu", i);
        if(rohr_error_check(rohr_input_action_create(
                action_controller, name, INPUT_ACTION_BUTTON))) return false;
    }
    if(!rohr_error_check(rohr_input_action_create(
            action_controller, "overflow", INPUT_ACTION_BUTTON))) return false;
    return !rohr_error_check(rohr_input_controller_destroy(action_controller));
}

static bool input_persistent_button_test(void) {
    InputControllerIdResult controller_result =
        rohr_input_controller_create("persistent_buttons");
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
    toggle_result = rohr_input_action_create(
        controller, "console", INPUT_ACTION_BUTTON);
    axis_result = rohr_input_action_create(
        controller, "axis", INPUT_ACTION_AXIS_1D);
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
    if(!event_push((SDL_Event){.type = SDL_EVENT_WINDOW_FOCUS_LOST,
            .window = {.type = SDL_EVENT_WINDOW_FOCUS_LOST}})) return false;
    events_drain();
    return !rohr_input_key_down_check(SDL_SCANCODE_Q) &&
        rohr_input_key_released_check(SDL_SCANCODE_Q) &&
        !rohr_input_mouse_button_down_check(INPUT_MOUSE_BUTTON_RIGHT) &&
        rohr_input_mouse_button_released_check(INPUT_MOUSE_BUTTON_RIGHT);
}

int main(void) {
    if(rohr_error_check(rohr_engine_start())) return 1;
    if(!input_raw_snapshot_test() || !input_action_test() ||
            !input_persistent_button_test() || !input_focus_release_test() ||
            !input_capacity_test() ||
            !input_window_mode_test()) {
        fprintf(stderr, "input action test failed\n");
        rohr_engine_stop();
        return 1;
    }
    rohr_engine_stop();
    return 0;
}
