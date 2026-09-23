/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "input.h"
#include "input/input_internal.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct InputSnapshot {
    bool keys_down[SDL_SCANCODE_COUNT];
    bool keys_pressed[SDL_SCANCODE_COUNT];
    bool keys_released[SDL_SCANCODE_COUNT];
    bool mouse_down[INPUT_MOUSE_BUTTON_COUNT];
    bool mouse_pressed[INPUT_MOUSE_BUTTON_COUNT];
    bool mouse_released[INPUT_MOUSE_BUTTON_COUNT];
    Vec2D mouse_position;
    Vec2D mouse_delta;
    Vec2D mouse_wheel;
    SDL_Keymod modifiers;
    InputTextState text;
    SDL_WindowID mouse_window;
    SDL_WindowID keyboard_window;
} InputSnapshot;

typedef struct InputController {
    char name[ROHR_INPUT_NAME_MAX];
    InputControllerId id;
    bool enabled;
    bool used;
} InputController;

typedef struct InputAction {
    char name[ROHR_INPUT_NAME_MAX];
    InputActionId id;
    InputControllerId controller;
    InputActionType type;
    InputBinding defaults[ROHR_INPUT_BINDING_LIMIT];
    size_t default_count;
    InputBinding overrides[ROHR_INPUT_BINDING_LIMIT];
    size_t override_count;
    bool override_active;
    InputButtonMode button_mode;
    bool button_initial_state;
    bool button_physical_down;
    bool button_down;
    bool button_pressed;
    bool button_released;
    bool used;
} InputAction;

static InputSnapshot input_snapshot;
static InputController input_controllers[ROHR_INPUT_CONTROLLER_LIMIT];
static uint32_t input_controller_generations[ROHR_INPUT_CONTROLLER_LIMIT];
static InputAction input_actions[ROHR_INPUT_ACTION_LIMIT];
static uint32_t input_action_generations[ROHR_INPUT_ACTION_LIMIT];

static uint32_t input_id_create(uint32_t generation, size_t slot) {
    return (generation << 16) | (uint32_t)(slot + 1);
}

static bool input_id_slot_get(uint32_t id, size_t capacity, size_t *slot) {
    uint32_t value = id & UINT32_C(0xffff);
    if(value == 0 || value > capacity || slot == NULL) return false;
    *slot = (size_t)(value - 1);
    return true;
}

static InputController *input_controller_get(InputControllerId id) {
    size_t slot;
    if(!input_id_slot_get(id, ROHR_INPUT_CONTROLLER_LIMIT, &slot) ||
            !input_controllers[slot].used || input_controllers[slot].id != id)
        return NULL;
    return &input_controllers[slot];
}

static InputAction *input_action_get(InputActionId id) {
    size_t slot;
    if(!input_id_slot_get(id, ROHR_INPUT_ACTION_LIMIT, &slot) ||
            !input_actions[slot].used || input_actions[slot].id != id)
        return NULL;
    return &input_actions[slot];
}

static bool input_name_check(const char *name) {
    return name != NULL && name[0] != '\0' && strlen(name) < ROHR_INPUT_NAME_MAX;
}

static bool input_scancode_check(SDL_Scancode key) {
    return key > SDL_SCANCODE_UNKNOWN && key < SDL_SCANCODE_COUNT;
}

static bool input_mouse_button_check(InputMouseButton button) {
    return button > INPUT_MOUSE_BUTTON_NONE && button < INPUT_MOUSE_BUTTON_COUNT;
}

static bool input_axis_component_check(InputAxisComponent component) {
    return component >= INPUT_AXIS_COMPONENT_X && component <= INPUT_AXIS_COMPONENT_XY;
}

static bool input_modifiers_check(SDL_Keymod required) {
    return (input_snapshot.modifiers & required) == required;
}

static const InputBinding *input_action_bindings_get(const InputAction *action,
        size_t *count) {
    if(action == NULL || count == NULL) return NULL;
    if(action->override_active) {
        *count = action->override_count;
        return action->overrides;
    }
    *count = action->default_count;
    return action->defaults;
}

static bool input_binding_digital_down_check(const InputBinding *binding) {
    if(binding == NULL || !input_modifiers_check(binding->modifiers)) return false;
    if(binding->source == INPUT_BINDING_KEY)
        return input_key_down_check(binding->input.key);
    if(binding->source == INPUT_BINDING_MOUSE_BUTTON)
        return input_mouse_button_down_check(binding->input.mouse_button);
    return false;
}

static bool input_action_button_value_get(const InputAction *action) {
    size_t count = 0;
    const InputBinding *bindings;
    if(action == NULL || action->type != INPUT_ACTION_BUTTON) return false;
    bindings = input_action_bindings_get(action, &count);
    for(size_t i = 0; i < count; i += 1)
        if(input_binding_digital_down_check(&bindings[i])) return true;
    return false;
}

static float input_binding_scalar_get(const InputBinding *binding) {
    float value = 0.0f;
    if(binding == NULL || !input_modifiers_check(binding->modifiers)) return 0.0f;
    if(binding->source == INPUT_BINDING_KEY ||
            binding->source == INPUT_BINDING_MOUSE_BUTTON) {
        value = input_binding_digital_down_check(binding) ? 1.0f : 0.0f;
    } else if(binding->source == INPUT_BINDING_MOUSE_MOTION) {
        value = binding->input.axis_component == INPUT_AXIS_COMPONENT_X ?
            input_snapshot.mouse_delta.x : input_snapshot.mouse_delta.y;
    } else if(binding->source == INPUT_BINDING_MOUSE_WHEEL) {
        value = binding->input.axis_component == INPUT_AXIS_COMPONENT_X ?
            input_snapshot.mouse_wheel.x : input_snapshot.mouse_wheel.y;
    }
    value *= binding->scale.x;
    return binding->inverted_x ? -value : value;
}

static Vec2D input_binding_vector_get(const InputBinding *binding) {
    Vec2D value = {0};
    if(binding == NULL || !input_modifiers_check(binding->modifiers)) return value;
    if(binding->source == INPUT_BINDING_KEY ||
            binding->source == INPUT_BINDING_MOUSE_BUTTON) {
        if(input_binding_digital_down_check(binding)) value = binding->direction;
    } else {
        Vec2D source = binding->source == INPUT_BINDING_MOUSE_MOTION ?
            input_snapshot.mouse_delta : input_snapshot.mouse_wheel;
        if(binding->input.axis_component == INPUT_AXIS_COMPONENT_X) {
            value.x = source.x;
            value.y = source.x;
        } else if(binding->input.axis_component == INPUT_AXIS_COMPONENT_Y) {
            value.x = source.y;
            value.y = source.y;
        } else value = source;
    }
    if(!binding->affects_x) value.x = 0.0f;
    if(!binding->affects_y) value.y = 0.0f;
    value.x *= binding->scale.x;
    value.y *= binding->scale.y;
    if(binding->inverted_x) value.x = -value.x;
    if(binding->inverted_y) value.y = -value.y;
    return value;
}

static void input_button_actions_refresh(bool transitions) {
    for(size_t i = 0; i < ROHR_INPUT_ACTION_LIMIT; i += 1) {
        InputAction *action = &input_actions[i];
        InputController *controller;
        bool physical_down;
        if(!action->used || action->type != INPUT_ACTION_BUTTON) continue;
        controller = input_controller_get(action->controller);
        physical_down = input_action_button_value_get(action);
        if(action->button_mode == INPUT_BUTTON_PERSISTENT) {
            if(transitions && controller != NULL && controller->enabled &&
                    !action->button_physical_down && physical_down) {
                action->button_down = !action->button_down;
                if(action->button_down) action->button_pressed = true;
                else action->button_released = true;
            }
        } else {
            if(transitions && controller != NULL && controller->enabled &&
                    !action->button_down && physical_down)
                action->button_pressed = true;
            if(transitions && controller != NULL && controller->enabled &&
                    action->button_down && !physical_down)
                action->button_released = true;
            action->button_down = physical_down;
        }
        action->button_physical_down = physical_down;
    }
}

bool input_binding_valid_check(InputActionType action_type,
        const InputBinding *binding) {
    if(action_type < INPUT_ACTION_BUTTON || action_type > INPUT_ACTION_AXIS_2D ||
            binding == NULL || binding->source < INPUT_BINDING_KEY ||
            binding->source > INPUT_BINDING_MOUSE_WHEEL ||
            memchr(binding->name, '\0', sizeof(binding->name)) == NULL ||
            !isfinite(binding->scale.x) || !isfinite(binding->scale.y) ||
            !isfinite(binding->direction.x) || !isfinite(binding->direction.y))
        return false;
    if(binding->source == INPUT_BINDING_KEY &&
            !input_scancode_check(binding->input.key)) return false;
    if(binding->source == INPUT_BINDING_MOUSE_BUTTON &&
            !input_mouse_button_check(binding->input.mouse_button)) return false;
    if((binding->source == INPUT_BINDING_MOUSE_MOTION ||
            binding->source == INPUT_BINDING_MOUSE_WHEEL) &&
            !input_axis_component_check(binding->input.axis_component)) return false;
    if(action_type == INPUT_ACTION_BUTTON)
        return binding->source == INPUT_BINDING_KEY ||
            binding->source == INPUT_BINDING_MOUSE_BUTTON;
    if(action_type == INPUT_ACTION_AXIS_1D &&
            (binding->source == INPUT_BINDING_MOUSE_MOTION ||
             binding->source == INPUT_BINDING_MOUSE_WHEEL))
        return binding->input.axis_component != INPUT_AXIS_COMPONENT_XY;
    return true;
}

InputBinding input_binding_key_create(SDL_Keycode key) {
    SDL_Keymod modifiers = SDL_KMOD_NONE;
    SDL_Scancode scancode = SDL_GetScancodeFromKey(key, &modifiers);
    return (InputBinding){.source = INPUT_BINDING_KEY,
        .input.key = scancode, .modifiers = modifiers,
        .affects_x = true, .affects_y = true, .scale = {1.0f, 1.0f}};
}

InputBinding input_binding_scancode_create(SDL_Scancode key) {
    return (InputBinding){.source = INPUT_BINDING_KEY,
        .input.key = key, .affects_x = true, .affects_y = true,
        .scale = {1.0f, 1.0f}};
}

static EngineResult input_bindings_set(InputAction *action,
        InputBinding *destination, size_t *destination_count,
        const InputBinding *bindings, size_t count) {
    if(action == NULL) return error_result_error(ERROR_ENGINE_INPUT_NOT_FOUND);
    if(count > ROHR_INPUT_BINDING_LIMIT || (count > 0 && bindings == NULL))
        return error_result_error(ERROR_ENGINE_INPUT_CAPACITY_EXCEEDED);
    for(size_t i = 0; i < count; i += 1) {
        if(!input_binding_valid_check(action->type, &bindings[i]))
            return error_result_error(ERROR_ENGINE_INPUT_BINDING_INVALID);
        if(bindings[i].name[0] == '\0') continue;
        for(size_t previous = 0; previous < i; previous += 1)
            if(strcmp(bindings[previous].name, bindings[i].name) == 0)
                return error_result_error(ERROR_ENGINE_DUPLICATE_INPUT_NAME);
    }
    if(count > 0) memcpy(destination, bindings, count * sizeof(*bindings));
    if(count < ROHR_INPUT_BINDING_LIMIT)
        memset(destination + count, 0,
            (ROHR_INPUT_BINDING_LIMIT - count) * sizeof(*destination));
    *destination_count = count;
    input_button_actions_refresh(false);
    return error_result_value(true);
}

static InputBindingListResult input_binding_list_result_get(
        const InputBinding *bindings, size_t count) {
    InputBindingList list = {.count = count};
    if(count > 0) memcpy(list.values, bindings, count * sizeof(*bindings));
    return ERROR_RESULT_MAKE_VALUE(InputBindingListResult, list);
}

static void input_utf8_copy(char *destination, size_t capacity,
        bool *truncated, const char *source) {
    size_t source_length;
    size_t copy_length;
    if(destination == NULL || capacity == 0 || truncated == NULL) return;
    if(source == NULL) source = "";
    source_length = strlen(source);
    copy_length = source_length < capacity ? source_length : capacity - 1;
    while(copy_length > 0 && ((unsigned char)source[copy_length] & 0xc0) == 0x80)
        copy_length -= 1;
    memcpy(destination, source, copy_length);
    destination[copy_length] = '\0';
    *truncated = source_length >= capacity;
}

static void input_text_copy(char destination[ROHR_INPUT_TEXT_CAPACITY],
        bool *truncated, const char *source) {
    input_utf8_copy(destination, ROHR_INPUT_TEXT_CAPACITY, truncated, source);
}

static void input_text_candidates_clear(void) {
    memset(input_snapshot.text.candidates, 0,
        sizeof(input_snapshot.text.candidates));
    input_snapshot.text.candidate_count = 0;
    input_snapshot.text.selected_candidate = -1;
    input_snapshot.text.candidates_horizontal = false;
    input_snapshot.text.candidates_truncated = false;
}

static void input_text_append(const char *source) {
    size_t current;
    size_t available;
    size_t source_length;
    size_t copy_length;
    if(source == NULL) return;
    current = strlen(input_snapshot.text.committed);
    available = ROHR_INPUT_TEXT_CAPACITY - current - 1;
    source_length = strlen(source);
    copy_length = source_length < available ? source_length : available;
    while(copy_length > 0 && ((unsigned char)source[copy_length] & 0xc0) == 0x80)
        copy_length -= 1;
    memcpy(input_snapshot.text.committed + current, source, copy_length);
    input_snapshot.text.committed[current + copy_length] = '\0';
    if(copy_length < source_length) input_snapshot.text.committed_truncated = true;
}

void input_start(void) {
    memset(&input_snapshot, 0, sizeof(input_snapshot));
    memset(input_controllers, 0, sizeof(input_controllers));
    memset(input_controller_generations, 0, sizeof(input_controller_generations));
    memset(input_actions, 0, sizeof(input_actions));
    memset(input_action_generations, 0, sizeof(input_action_generations));
    input_snapshot.text.selected_candidate = -1;
}

void input_stop(void) {
    SDL_Window *window = SDL_GetKeyboardFocus();
    if(window != NULL && SDL_TextInputActive(window)) (void)SDL_StopTextInput(window);
    input_start();
}

void input_frame_begin(void) {
    memset(input_snapshot.keys_pressed, 0, sizeof(input_snapshot.keys_pressed));
    memset(input_snapshot.keys_released, 0, sizeof(input_snapshot.keys_released));
    memset(input_snapshot.mouse_pressed, 0, sizeof(input_snapshot.mouse_pressed));
    memset(input_snapshot.mouse_released, 0, sizeof(input_snapshot.mouse_released));
    input_snapshot.mouse_delta = (Vec2D){0};
    input_snapshot.mouse_wheel = (Vec2D){0};
    input_snapshot.text.committed[0] = '\0';
    input_snapshot.text.committed_truncated = false;
    for(size_t i = 0; i < ROHR_INPUT_ACTION_LIMIT; i += 1) {
        input_actions[i].button_pressed = false;
        input_actions[i].button_released = false;
    }
    input_button_actions_refresh(false);
}

bool input_key_down_check(SDL_Scancode key) {
    return input_scancode_check(key) && input_snapshot.keys_down[key];
}

bool input_key_pressed_check(SDL_Scancode key) {
    return input_scancode_check(key) && input_snapshot.keys_pressed[key];
}

bool input_key_released_check(SDL_Scancode key) {
    return input_scancode_check(key) && input_snapshot.keys_released[key];
}

SDL_Keymod input_modifiers_get(void) { return input_snapshot.modifiers; }

bool input_mouse_button_down_check(InputMouseButton button) {
    return input_mouse_button_check(button) && input_snapshot.mouse_down[button];
}

bool input_mouse_button_pressed_check(InputMouseButton button) {
    return input_mouse_button_check(button) && input_snapshot.mouse_pressed[button];
}

bool input_mouse_button_released_check(InputMouseButton button) {
    return input_mouse_button_check(button) && input_snapshot.mouse_released[button];
}

Vec2D input_mouse_position_get(void) { return input_snapshot.mouse_position; }
Vec2D input_mouse_delta_get(void) { return input_snapshot.mouse_delta; }
Vec2D input_mouse_wheel_get(void) { return input_snapshot.mouse_wheel; }

static SDL_Window *input_mouse_window_get(void) {
    SDL_Window *window = input_snapshot.mouse_window == 0 ? NULL :
        SDL_GetWindowFromID(input_snapshot.mouse_window);
    if(window == NULL) window = SDL_GetMouseFocus();
    if(window == NULL) window = SDL_GetKeyboardFocus();
    return window;
}

EngineResult input_mouse_relative_mode_set(bool enabled) {
    SDL_Window *window = input_mouse_window_get();
    if(window == NULL) return error_result_error(ERROR_ENGINE_INPUT_WINDOW_NOT_FOUND);
    if(!SDL_SetWindowRelativeMouseMode(window, enabled))
        return error_result_error_detail(ERROR_ENGINE_INPUT_OPERATION_FAILED,
            SDL_GetError());
    return error_result_value(true);
}

bool input_mouse_relative_mode_check(void) {
    SDL_Window *window = input_mouse_window_get();
    return window != NULL && SDL_GetWindowRelativeMouseMode(window);
}

static SDL_Window *input_keyboard_window_get(void) {
    SDL_Window *window = input_snapshot.keyboard_window == 0 ? NULL :
        SDL_GetWindowFromID(input_snapshot.keyboard_window);
    if(window == NULL) window = SDL_GetKeyboardFocus();
    return window;
}

EngineResult input_text_start(void) {
    SDL_Window *window = input_keyboard_window_get();
    if(window == NULL) return error_result_error(ERROR_ENGINE_INPUT_WINDOW_NOT_FOUND);
    if(!SDL_StartTextInput(window)) return error_result_error_detail(
        ERROR_ENGINE_INPUT_OPERATION_FAILED, SDL_GetError());
    input_snapshot.text.active = true;
    return error_result_value(true);
}

EngineResult input_text_stop(void) {
    SDL_Window *window = input_keyboard_window_get();
    if(window == NULL) return error_result_error(ERROR_ENGINE_INPUT_WINDOW_NOT_FOUND);
    if(!SDL_StopTextInput(window)) return error_result_error_detail(
        ERROR_ENGINE_INPUT_OPERATION_FAILED, SDL_GetError());
    input_snapshot.text.active = false;
    input_snapshot.text.composition[0] = '\0';
    input_snapshot.text.composition_start = 0;
    input_snapshot.text.composition_length = 0;
    input_text_candidates_clear();
    return error_result_value(true);
}

EngineResult input_text_area_set(SDL_Rect area, int cursor) {
    SDL_Window *window = input_keyboard_window_get();
    if(window == NULL) return error_result_error(ERROR_ENGINE_INPUT_WINDOW_NOT_FOUND);
    if(!SDL_SetTextInputArea(window, &area, cursor))
        return error_result_error_detail(ERROR_ENGINE_INPUT_OPERATION_FAILED,
            SDL_GetError());
    return error_result_value(true);
}

InputTextState input_text_state_get(void) { return input_snapshot.text; }

InputControllerIdResult input_controller_create(const char *name) {
    size_t slot;
    if(name == NULL || name[0] == '\0') return ERROR_RESULT_MAKE_ERROR(
        InputControllerIdResult, ERROR_ENGINE_INVALID_INPUT_NAME);
    if(strlen(name) >= ROHR_INPUT_NAME_MAX) return ERROR_RESULT_MAKE_ERROR(
        InputControllerIdResult, ERROR_ENGINE_INPUT_NAME_TOO_LONG);
    for(size_t i = 0; i < ROHR_INPUT_CONTROLLER_LIMIT; i += 1)
        if(input_controllers[i].used && strcmp(input_controllers[i].name, name) == 0)
            return ERROR_RESULT_MAKE_ERROR(InputControllerIdResult,
                ERROR_ENGINE_DUPLICATE_INPUT_NAME);
    for(slot = 0; slot < ROHR_INPUT_CONTROLLER_LIMIT; slot += 1)
        if(!input_controllers[slot].used) break;
    if(slot == ROHR_INPUT_CONTROLLER_LIMIT) return ERROR_RESULT_MAKE_ERROR(
        InputControllerIdResult, ERROR_ENGINE_INPUT_CAPACITY_EXCEEDED);
    input_controller_generations[slot] += 1;
    if(input_controller_generations[slot] == 0)
        input_controller_generations[slot] = 1;
    input_controllers[slot] = (InputController){
        .id = input_id_create(input_controller_generations[slot], slot),
        .enabled = true,
        .used = true,
    };
    snprintf(input_controllers[slot].name, sizeof(input_controllers[slot].name),
        "%s", name);
    return ERROR_RESULT_MAKE_VALUE(InputControllerIdResult,
        input_controllers[slot].id);
}

EngineResult input_controller_destroy(InputControllerId controller) {
    InputController *value = input_controller_get(controller);
    if(value == NULL) return error_result_error(ERROR_ENGINE_INPUT_NOT_FOUND);
    for(size_t i = 0; i < ROHR_INPUT_ACTION_LIMIT; i += 1)
        if(input_actions[i].used && input_actions[i].controller == controller)
            input_actions[i] = (InputAction){0};
    *value = (InputController){0};
    return error_result_value(true);
}

InputControllerIdResult input_controller_by_name_get(const char *name) {
    if(!input_name_check(name)) return ERROR_RESULT_MAKE_ERROR(
        InputControllerIdResult, ERROR_ENGINE_INVALID_INPUT_NAME);
    for(size_t i = 0; i < ROHR_INPUT_CONTROLLER_LIMIT; i += 1)
        if(input_controllers[i].used && strcmp(input_controllers[i].name, name) == 0)
            return ERROR_RESULT_MAKE_VALUE(InputControllerIdResult,
                input_controllers[i].id);
    return ERROR_RESULT_MAKE_ERROR(InputControllerIdResult,
        ERROR_ENGINE_INPUT_NOT_FOUND);
}

EngineResult input_controller_enabled_set(InputControllerId controller,
        bool enabled) {
    InputController *value = input_controller_get(controller);
    if(value == NULL) return error_result_error(ERROR_ENGINE_INPUT_NOT_FOUND);
    value->enabled = enabled;
    for(size_t i = 0; i < ROHR_INPUT_ACTION_LIMIT; i += 1) {
        if(!input_actions[i].used ||
                input_actions[i].controller != controller) continue;
        input_actions[i].button_pressed = false;
        input_actions[i].button_released = false;
    }
    return error_result_value(true);
}

bool input_controller_enabled_check(InputControllerId controller) {
    InputController *value = input_controller_get(controller);
    return value != NULL && value->enabled;
}

InputActionIdResult input_action_create(InputControllerId controller,
        const char *name, InputActionType type) {
    size_t slot;
    if(input_controller_get(controller) == NULL) return ERROR_RESULT_MAKE_ERROR(
        InputActionIdResult, ERROR_ENGINE_INPUT_NOT_FOUND);
    if(name == NULL || name[0] == '\0') return ERROR_RESULT_MAKE_ERROR(
        InputActionIdResult, ERROR_ENGINE_INVALID_INPUT_NAME);
    if(strlen(name) >= ROHR_INPUT_NAME_MAX) return ERROR_RESULT_MAKE_ERROR(
        InputActionIdResult, ERROR_ENGINE_INPUT_NAME_TOO_LONG);
    if(type < INPUT_ACTION_BUTTON || type > INPUT_ACTION_AXIS_2D)
        return ERROR_RESULT_MAKE_ERROR(InputActionIdResult,
            ERROR_ENGINE_INPUT_TYPE_MISMATCH);
    for(size_t i = 0; i < ROHR_INPUT_ACTION_LIMIT; i += 1)
        if(input_actions[i].used && input_actions[i].controller == controller &&
                strcmp(input_actions[i].name, name) == 0)
            return ERROR_RESULT_MAKE_ERROR(InputActionIdResult,
                ERROR_ENGINE_DUPLICATE_INPUT_NAME);
    for(slot = 0; slot < ROHR_INPUT_ACTION_LIMIT; slot += 1)
        if(!input_actions[slot].used) break;
    if(slot == ROHR_INPUT_ACTION_LIMIT) return ERROR_RESULT_MAKE_ERROR(
        InputActionIdResult, ERROR_ENGINE_INPUT_CAPACITY_EXCEEDED);
    input_action_generations[slot] += 1;
    if(input_action_generations[slot] == 0) input_action_generations[slot] = 1;
    input_actions[slot] = (InputAction){
        .id = input_id_create(input_action_generations[slot], slot),
        .controller = controller,
        .type = type,
        .button_mode = INPUT_BUTTON_MOMENTARY,
        .used = true,
    };
    snprintf(input_actions[slot].name, sizeof(input_actions[slot].name), "%s", name);
    input_actions[slot].button_down = input_action_button_value_get(
        &input_actions[slot]);
    return ERROR_RESULT_MAKE_VALUE(InputActionIdResult, input_actions[slot].id);
}

EngineResult input_action_destroy(InputActionId action) {
    InputAction *value = input_action_get(action);
    if(value == NULL) return error_result_error(ERROR_ENGINE_INPUT_NOT_FOUND);
    *value = (InputAction){0};
    return error_result_value(true);
}

InputActionIdResult input_action_by_name_get(InputControllerId controller,
        const char *name) {
    if(input_controller_get(controller) == NULL || !input_name_check(name))
        return ERROR_RESULT_MAKE_ERROR(InputActionIdResult,
            ERROR_ENGINE_INPUT_NOT_FOUND);
    for(size_t i = 0; i < ROHR_INPUT_ACTION_LIMIT; i += 1)
        if(input_actions[i].used &&
                input_actions[i].controller == controller &&
                strcmp(input_actions[i].name, name) == 0)
            return ERROR_RESULT_MAKE_VALUE(InputActionIdResult, input_actions[i].id);
    return ERROR_RESULT_MAKE_ERROR(InputActionIdResult,
        ERROR_ENGINE_INPUT_NOT_FOUND);
}

InputActionTypeResult input_action_type_get(InputActionId action) {
    InputAction *value = input_action_get(action);
    if(value == NULL) return ERROR_RESULT_MAKE_ERROR(InputActionTypeResult,
        ERROR_ENGINE_INPUT_NOT_FOUND);
    return ERROR_RESULT_MAKE_VALUE(InputActionTypeResult, value->type);
}

EngineResult input_action_button_mode_set(InputActionId action,
        InputButtonMode mode) {
    InputAction *value = input_action_get(action);
    if(value == NULL) return error_result_error(ERROR_ENGINE_INPUT_NOT_FOUND);
    if(value->type != INPUT_ACTION_BUTTON)
        return error_result_error(ERROR_ENGINE_INPUT_TYPE_MISMATCH);
    if(mode < INPUT_BUTTON_MOMENTARY || mode > INPUT_BUTTON_PERSISTENT)
        return error_result_error(ERROR_ENGINE_INPUT_CONFIGURATION_INVALID);
    value->button_mode = mode;
    value->button_physical_down = input_action_button_value_get(value);
    value->button_down = mode == INPUT_BUTTON_PERSISTENT ?
        value->button_initial_state : value->button_physical_down;
    value->button_pressed = false;
    value->button_released = false;
    return error_result_value(true);
}

InputButtonModeResult input_action_button_mode_get(InputActionId action) {
    InputAction *value = input_action_get(action);
    if(value == NULL) return ERROR_RESULT_MAKE_ERROR(InputButtonModeResult,
        ERROR_ENGINE_INPUT_NOT_FOUND);
    if(value->type != INPUT_ACTION_BUTTON)
        return ERROR_RESULT_MAKE_ERROR(InputButtonModeResult,
            ERROR_ENGINE_INPUT_TYPE_MISMATCH);
    return ERROR_RESULT_MAKE_VALUE(InputButtonModeResult, value->button_mode);
}

EngineResult input_action_button_initial_state_set(InputActionId action,
        bool state) {
    InputAction *value = input_action_get(action);
    if(value == NULL) return error_result_error(ERROR_ENGINE_INPUT_NOT_FOUND);
    if(value->type != INPUT_ACTION_BUTTON)
        return error_result_error(ERROR_ENGINE_INPUT_TYPE_MISMATCH);
    value->button_initial_state = state;
    if(value->button_mode == INPUT_BUTTON_PERSISTENT) value->button_down = state;
    value->button_pressed = false;
    value->button_released = false;
    return error_result_value(true);
}

bool input_action_button_initial_state_check(InputActionId action) {
    InputAction *value = input_action_get(action);
    return value != NULL && value->type == INPUT_ACTION_BUTTON &&
        value->button_initial_state;
}

EngineResult input_action_button_state_set(InputActionId action, bool state) {
    InputAction *value = input_action_get(action);
    InputController *controller;
    if(value == NULL) return error_result_error(ERROR_ENGINE_INPUT_NOT_FOUND);
    if(value->type != INPUT_ACTION_BUTTON ||
            value->button_mode != INPUT_BUTTON_PERSISTENT)
        return error_result_error(ERROR_ENGINE_INPUT_TYPE_MISMATCH);
    controller = input_controller_get(value->controller);
    if(value->button_down != state && controller != NULL && controller->enabled) {
        if(state) value->button_pressed = true;
        else value->button_released = true;
    }
    value->button_down = state;
    return error_result_value(true);
}

EngineResult input_action_button_state_reset(InputActionId action) {
    InputAction *value = input_action_get(action);
    if(value == NULL) return error_result_error(ERROR_ENGINE_INPUT_NOT_FOUND);
    if(value->type != INPUT_ACTION_BUTTON ||
            value->button_mode != INPUT_BUTTON_PERSISTENT)
        return error_result_error(ERROR_ENGINE_INPUT_TYPE_MISMATCH);
    return input_action_button_state_set(action, value->button_initial_state);
}

EngineResult input_action_bindings_default_set(InputActionId action,
        const InputBinding *bindings, size_t count) {
    InputAction *value = input_action_get(action);
    return input_bindings_set(value, value == NULL ? NULL : value->defaults,
        value == NULL ? NULL : &value->default_count, bindings, count);
}

InputBindingListResult input_action_bindings_default_get(InputActionId action) {
    InputAction *value = input_action_get(action);
    if(value == NULL) return ERROR_RESULT_MAKE_ERROR(InputBindingListResult,
        ERROR_ENGINE_INPUT_NOT_FOUND);
    return input_binding_list_result_get(value->defaults, value->default_count);
}

EngineResult input_action_bindings_override_set(InputActionId action,
        const InputBinding *bindings, size_t count) {
    InputAction *value = input_action_get(action);
    EngineResult result = input_bindings_set(value,
        value == NULL ? NULL : value->overrides,
        value == NULL ? NULL : &value->override_count, bindings, count);
    if(result.kind != ERROR_RESULT_ERROR) {
        value->override_active = true;
        input_button_actions_refresh(false);
    }
    return result;
}

InputBindingListResult input_action_bindings_override_get(InputActionId action) {
    InputAction *value = input_action_get(action);
    if(value == NULL) return ERROR_RESULT_MAKE_ERROR(InputBindingListResult,
        ERROR_ENGINE_INPUT_NOT_FOUND);
    return input_binding_list_result_get(value->overrides, value->override_count);
}

InputBindingListResult input_action_bindings_effective_get(InputActionId action) {
    InputAction *value = input_action_get(action);
    size_t count = 0;
    const InputBinding *bindings;
    if(value == NULL) return ERROR_RESULT_MAKE_ERROR(InputBindingListResult,
        ERROR_ENGINE_INPUT_NOT_FOUND);
    bindings = input_action_bindings_get(value, &count);
    return input_binding_list_result_get(bindings, count);
}

EngineResult input_action_bindings_override_clear(InputActionId action) {
    InputAction *value = input_action_get(action);
    if(value == NULL) return error_result_error(ERROR_ENGINE_INPUT_NOT_FOUND);
    memset(value->overrides, 0, sizeof(value->overrides));
    value->override_count = 0;
    value->override_active = false;
    input_button_actions_refresh(false);
    return error_result_value(true);
}

bool input_action_bindings_override_check(InputActionId action) {
    InputAction *value = input_action_get(action);
    return value != NULL && value->override_active;
}

bool input_action_button_down_check(InputActionId action) {
    InputAction *value = input_action_get(action);
    InputController *controller = value == NULL ? NULL :
        input_controller_get(value->controller);
    return value != NULL && value->type == INPUT_ACTION_BUTTON &&
        controller != NULL && controller->enabled && value->button_down;
}

bool input_action_button_pressed_check(InputActionId action) {
    InputAction *value = input_action_get(action);
    InputController *controller = value == NULL ? NULL :
        input_controller_get(value->controller);
    return value != NULL && value->type == INPUT_ACTION_BUTTON &&
        controller != NULL && controller->enabled && value->button_pressed;
}

bool input_action_button_released_check(InputActionId action) {
    InputAction *value = input_action_get(action);
    InputController *controller = value == NULL ? NULL :
        input_controller_get(value->controller);
    return value != NULL && value->type == INPUT_ACTION_BUTTON &&
        controller != NULL && controller->enabled && value->button_released;
}

InputAxis1DResult input_action_axis_1d_get(InputActionId action) {
    InputAction *value = input_action_get(action);
    InputController *controller;
    const InputBinding *bindings;
    size_t count = 0;
    float axis = 0.0f;
    if(value == NULL) return ERROR_RESULT_MAKE_ERROR(InputAxis1DResult,
        ERROR_ENGINE_INPUT_NOT_FOUND);
    if(value->type != INPUT_ACTION_AXIS_1D) return ERROR_RESULT_MAKE_ERROR(
        InputAxis1DResult, ERROR_ENGINE_INPUT_TYPE_MISMATCH);
    controller = input_controller_get(value->controller);
    if(controller == NULL || !controller->enabled)
        return ERROR_RESULT_MAKE_VALUE(InputAxis1DResult, 0.0f);
    bindings = input_action_bindings_get(value, &count);
    for(size_t i = 0; i < count; i += 1)
        axis += input_binding_scalar_get(&bindings[i]);
    if(axis < -1.0f) axis = -1.0f;
    if(axis > 1.0f) axis = 1.0f;
    return ERROR_RESULT_MAKE_VALUE(InputAxis1DResult, axis);
}

InputAxis2DResult input_action_axis_2d_get(InputActionId action) {
    InputAction *value = input_action_get(action);
    InputController *controller;
    const InputBinding *bindings;
    size_t count = 0;
    Vec2D axis = {0};
    float magnitude_squared;
    if(value == NULL) return ERROR_RESULT_MAKE_ERROR(InputAxis2DResult,
        ERROR_ENGINE_INPUT_NOT_FOUND);
    if(value->type != INPUT_ACTION_AXIS_2D) return ERROR_RESULT_MAKE_ERROR(
        InputAxis2DResult, ERROR_ENGINE_INPUT_TYPE_MISMATCH);
    controller = input_controller_get(value->controller);
    if(controller == NULL || !controller->enabled)
        return ERROR_RESULT_MAKE_VALUE(InputAxis2DResult, axis);
    bindings = input_action_bindings_get(value, &count);
    for(size_t i = 0; i < count; i += 1) {
        Vec2D contribution = input_binding_vector_get(&bindings[i]);
        axis.x += contribution.x;
        axis.y += contribution.y;
    }
    magnitude_squared = axis.x * axis.x + axis.y * axis.y;
    if(magnitude_squared > 1.0f) {
        float inverse = 1.0f / sqrtf(magnitude_squared);
        axis.x *= inverse;
        axis.y *= inverse;
    }
    return ERROR_RESULT_MAKE_VALUE(InputAxis2DResult, axis);
}

void input_event_add(const SDL_Event *event) {
    bool button_source_changed = false;
    if(event == NULL) return;
    switch(event->type) {
        case SDL_EVENT_KEY_DOWN:
            input_snapshot.keyboard_window = event->key.windowID;
            input_snapshot.modifiers = event->key.mod;
            if(input_scancode_check(event->key.scancode) && !event->key.repeat &&
                    !input_snapshot.keys_down[event->key.scancode]) {
                input_snapshot.keys_down[event->key.scancode] = true;
                input_snapshot.keys_pressed[event->key.scancode] = true;
                button_source_changed = true;
            }
            break;
        case SDL_EVENT_KEY_UP:
            input_snapshot.keyboard_window = event->key.windowID;
            input_snapshot.modifiers = event->key.mod;
            if(input_scancode_check(event->key.scancode) &&
                    input_snapshot.keys_down[event->key.scancode]) {
                input_snapshot.keys_down[event->key.scancode] = false;
                input_snapshot.keys_released[event->key.scancode] = true;
                button_source_changed = true;
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            InputMouseButton button = (InputMouseButton)event->button.button;
            bool down = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            input_snapshot.mouse_window = event->button.windowID;
            input_snapshot.mouse_position = (Vec2D){event->button.x, event->button.y};
            if(input_mouse_button_check(button) &&
                    input_snapshot.mouse_down[button] != down) {
                input_snapshot.mouse_down[button] = down;
                input_snapshot.mouse_pressed[button] |= down;
                input_snapshot.mouse_released[button] |= !down;
                button_source_changed = true;
            }
            break;
        }
        case SDL_EVENT_MOUSE_MOTION:
            input_snapshot.mouse_window = event->motion.windowID;
            input_snapshot.mouse_position = (Vec2D){event->motion.x, event->motion.y};
            input_snapshot.mouse_delta.x += event->motion.xrel;
            input_snapshot.mouse_delta.y += event->motion.yrel;
            break;
        case SDL_EVENT_MOUSE_WHEEL: {
            float direction = event->wheel.direction == SDL_MOUSEWHEEL_FLIPPED ?
                -1.0f : 1.0f;
            input_snapshot.mouse_window = event->wheel.windowID;
            input_snapshot.mouse_position = (Vec2D){
                event->wheel.mouse_x, event->wheel.mouse_y};
            input_snapshot.mouse_wheel.x += event->wheel.x * direction;
            input_snapshot.mouse_wheel.y += event->wheel.y * direction;
            break;
        }
        case SDL_EVENT_TEXT_INPUT:
            input_snapshot.keyboard_window = event->text.windowID;
            input_text_append(event->text.text);
            input_snapshot.text.composition[0] = '\0';
            input_snapshot.text.composition_start = 0;
            input_snapshot.text.composition_length = 0;
            input_snapshot.text.composition_truncated = false;
            input_text_candidates_clear();
            break;
        case SDL_EVENT_TEXT_EDITING:
            input_snapshot.keyboard_window = event->edit.windowID;
            input_text_copy(input_snapshot.text.composition,
                &input_snapshot.text.composition_truncated, event->edit.text);
            input_snapshot.text.composition_start = event->edit.start;
            input_snapshot.text.composition_length = event->edit.length;
            break;
        case SDL_EVENT_TEXT_EDITING_CANDIDATES: {
            size_t count = event->edit_candidates.num_candidates > 0 ?
                (size_t)event->edit_candidates.num_candidates : 0;
            input_snapshot.keyboard_window = event->edit_candidates.windowID;
            input_text_candidates_clear();
            if(count > ROHR_INPUT_TEXT_CANDIDATE_LIMIT) {
                count = ROHR_INPUT_TEXT_CANDIDATE_LIMIT;
                input_snapshot.text.candidates_truncated = true;
            }
            input_snapshot.text.candidate_count = count;
            input_snapshot.text.selected_candidate =
                event->edit_candidates.selected_candidate;
            input_snapshot.text.candidates_horizontal =
                event->edit_candidates.horizontal;
            for(size_t i = 0; i < count; i += 1) {
                bool truncated = false;
                input_utf8_copy(input_snapshot.text.candidates[i],
                    ROHR_INPUT_TEXT_CANDIDATE_CAPACITY, &truncated,
                    event->edit_candidates.candidates == NULL ? NULL :
                        event->edit_candidates.candidates[i]);
                input_snapshot.text.candidates_truncated |= truncated;
            }
            break;
        }
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            for(size_t i = 0; i < SDL_SCANCODE_COUNT; i += 1)
                input_snapshot.keys_released[i] |= input_snapshot.keys_down[i];
            for(size_t i = 0; i < INPUT_MOUSE_BUTTON_COUNT; i += 1)
                input_snapshot.mouse_released[i] |= input_snapshot.mouse_down[i];
            memset(input_snapshot.keys_down, 0, sizeof(input_snapshot.keys_down));
            memset(input_snapshot.mouse_down, 0, sizeof(input_snapshot.mouse_down));
            input_snapshot.modifiers = SDL_KMOD_NONE;
            button_source_changed = true;
            break;
        default:
            break;
    }
    if(button_source_changed) input_button_actions_refresh(true);
}
