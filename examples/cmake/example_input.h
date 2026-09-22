/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EXAMPLE_INPUT_H
#define ROHR_EXAMPLE_INPUT_H

#include "rohr.h"

static inline bool example_input_map_create(const char *name,
        InputActionMapId *map) {
    InputActionMapIdResult result = rohr_input_action_map_create(name);
    if(rohr_error_check(result) || map == NULL) return false;
    *map = result.result.value;
    return true;
}

static inline bool example_input_action_create(InputActionMapId map,
        const char *name, InputActionType type, const InputBinding *bindings,
        size_t binding_count, InputActionId *action) {
    InputActionIdResult result = rohr_input_action_create(map, name, type);
    EngineResult binding_result;
    if(rohr_error_check(result) || action == NULL) return false;
    *action = result.result.value;
    binding_result = rohr_input_action_bindings_default_set(*action, bindings,
        binding_count);
    if(!rohr_error_check(binding_result)) return true;
    (void)rohr_input_action_destroy(*action);
    *action = INPUT_ACTION_INVALID;
    return false;
}

static inline InputBinding example_input_key_binding(SDL_Scancode key,
        float scale, Vec2D direction) {
    return (InputBinding){.source = INPUT_BINDING_KEY, .input.key = key,
        .scale = scale, .direction = direction};
}

static inline InputBinding example_input_mouse_button_binding(
        InputMouseButton button) {
    return (InputBinding){.source = INPUT_BINDING_MOUSE_BUTTON,
        .input.mouse_button = button, .scale = 1.0f};
}

static inline MouseButtonState example_input_mouse_button_state_get(
        InputMouseButton button) {
    if(rohr_input_mouse_button_pressed_check(button))
        return MOUSE_BUTTON_STATE_PRESSED;
    if(rohr_input_mouse_button_released_check(button))
        return MOUSE_BUTTON_STATE_RELEASED;
    return rohr_input_mouse_button_down_check(button) ?
        MOUSE_BUTTON_STATE_DOWN : MOUSE_BUTTON_STATE_UP;
}

#endif
