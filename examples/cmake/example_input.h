/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EXAMPLE_INPUT_H
#define ROHR_EXAMPLE_INPUT_H

#include "rohr.h"
#include <string.h>

static inline bool example_input_controller_create(const char *name,
        InputControllerId *controller) {
    if(controller == NULL || name == NULL || strlen(name) >= ROHR_INPUT_NAME_MAX)
        return false;
    InputControllerDescription description = {.enabled = true};
    memcpy(description.name, name, strlen(name) + 1);
    InputControllerIdResult result = rohr_input_controller_create(description);
    if(rohr_error_check(result)) return false;
    *controller = result.result.value;
    return true;
}

static inline bool example_input_action_create(InputControllerId controller,
        const char *name, InputActionType type, const InputBinding *bindings,
        size_t binding_count, InputActionId *action) {
    if(action == NULL || name == NULL || strlen(name) >= ROHR_INPUT_NAME_MAX ||
            binding_count > ROHR_INPUT_BINDING_LIMIT ||
            (binding_count > 0 && bindings == NULL)) return false;
    InputActionDescription description = {.controller = controller, .type = type,
        .bindings_default.count = binding_count};
    memcpy(description.name, name, strlen(name) + 1);
    if(binding_count > 0) memcpy(description.bindings_default.values, bindings,
        binding_count * sizeof(*bindings));
    InputActionIdResult result = rohr_input_action_create(description);
    if(rohr_error_check(result)) return false;
    *action = result.result.value;
    return true;
}

static inline InputBinding example_input_key_binding(SDL_Scancode key,
        float scale, Vec2D direction) {
    return (InputBinding){.source = INPUT_BINDING_KEY, .input.key = key,
        .affects_x = direction.x != 0.0f,
        .affects_y = direction.y != 0.0f,
        .scale = {scale, scale}, .direction = direction};
}

static inline InputBinding example_input_mouse_button_binding(
        InputMouseButton button) {
    return (InputBinding){.source = INPUT_BINDING_MOUSE_BUTTON,
        .input.mouse_button = button, .scale = {1.0f, 1.0f}};
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
