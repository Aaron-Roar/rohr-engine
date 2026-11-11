/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <SDL3/SDL.h>
#include "error.h"
#include "math2d.h"

#define ROHR_INPUT_CONTROLLER_LIMIT 16
#define ROHR_INPUT_ACTION_LIMIT 256
#define ROHR_INPUT_BINDING_LIMIT 16
#define ROHR_INPUT_NAME_MAX 64
#define ROHR_INPUT_TEXT_CAPACITY 256
#define ROHR_INPUT_TEXT_CANDIDATE_LIMIT 8
#define ROHR_INPUT_TEXT_CANDIDATE_CAPACITY 64

typedef uint32_t InputControllerId;
typedef uint32_t InputActionId;

#define INPUT_CONTROLLER_INVALID UINT32_C(0)
#define INPUT_ACTION_INVALID UINT32_C(0)

ERROR_DECLARE_RESULT_TYPE(InputControllerIdResult, InputControllerId);
ERROR_DECLARE_RESULT_TYPE(InputActionIdResult, InputActionId);
ERROR_DECLARE_RESULT_TYPE(InputAxis1DResult, float);
ERROR_DECLARE_RESULT_TYPE(InputAxis2DResult, Vec2D);

/** Device-independent value produced by one input action. */
typedef enum InputActionType {
    INPUT_ACTION_BUTTON,
    INPUT_ACTION_AXIS_1D,
    INPUT_ACTION_AXIS_2D,
} InputActionType;

ERROR_DECLARE_RESULT_TYPE(InputActionTypeResult, InputActionType);

/** Logical behavior applied to a Button action's combined physical input. */
typedef enum InputButtonMode {
    /** The logical state follows whether any binding is currently active. */
    INPUT_BUTTON_MOMENTARY,
    /** Each inactive-to-active binding transition toggles the logical state. */
    INPUT_BUTTON_PERSISTENT,
} InputButtonMode;

ERROR_DECLARE_RESULT_TYPE(InputButtonModeResult, InputButtonMode);

/** Mouse buttons supported by the engine-owned snapshot. */
typedef enum InputMouseButton {
    INPUT_MOUSE_BUTTON_NONE = 0,
    INPUT_MOUSE_BUTTON_LEFT = SDL_BUTTON_LEFT,
    INPUT_MOUSE_BUTTON_MIDDLE = SDL_BUTTON_MIDDLE,
    INPUT_MOUSE_BUTTON_RIGHT = SDL_BUTTON_RIGHT,
    INPUT_MOUSE_BUTTON_X1 = SDL_BUTTON_X1,
    INPUT_MOUSE_BUTTON_X2 = SDL_BUTTON_X2,
    INPUT_MOUSE_BUTTON_COUNT,
} InputMouseButton;

/** Physical source read by an action binding. */
typedef enum InputBindingSource {
    INPUT_BINDING_KEY,
    INPUT_BINDING_MOUSE_BUTTON,
    INPUT_BINDING_MOUSE_MOTION,
    INPUT_BINDING_MOUSE_WHEEL,
} InputBindingSource;

/** Component selected from a one- or two-dimensional physical source. */
typedef enum InputAxisComponent {
    INPUT_AXIS_COMPONENT_X,
    INPUT_AXIS_COMPONENT_Y,
    INPUT_AXIS_COMPONENT_XY,
} InputAxisComponent;

/** One named, copied, allocation-free physical-source binding. */
typedef struct InputBinding {
    /** Optional name used by authored defaults and runtime rebinding tools. */
    char name[ROHR_INPUT_NAME_MAX];
    InputBindingSource source;
    union {
        SDL_Scancode key;
        InputMouseButton mouse_button;
        InputAxisComponent axis_component;
    } input;
    /** Required modifier mask. Every bit in the mask must be active. */
    SDL_Keymod modifiers;
    /** Whether this binding contributes to an Axis 2D action's X axis. */
    bool affects_x;
    /** Whether this binding contributes to an Axis 2D action's Y axis. */
    bool affects_y;
    /** Per-axis multipliers. Axis 1D uses x. Ignored by buttons. */
    Vec2D scale;
    /** Flip the scaled X contribution. Ignored by buttons. */
    bool inverted_x;
    /** Flip the scaled Y contribution. Used only by Axis 2D. */
    bool inverted_y;
    /** Direction contributed by a digital source to an Axis 2D action. */
    Vec2D direction;
} InputBinding;

/** Fixed-capacity binding value returned without exposing internal storage. */
typedef struct InputBindingList {
    InputBinding values[ROHR_INPUT_BINDING_LIMIT];
    size_t count;
} InputBindingList;

ERROR_DECLARE_RESULT_TYPE(InputBindingListResult, InputBindingList);

/** Complete copied configuration; names must be nonempty and terminated. */
typedef struct InputControllerDescription {
    char name[ROHR_INPUT_NAME_MAX];
    bool enabled;
} InputControllerDescription;
ERROR_DECLARE_RESULT_TYPE(InputControllerDescriptionResult, InputControllerDescription);

typedef struct InputControllerName { char value[ROHR_INPUT_NAME_MAX]; } InputControllerName;
ERROR_DECLARE_RESULT_TYPE(InputControllerNameResult, InputControllerName);

/** Authored action configuration. Runtime overrides and button state are separate.
 * All names and bindings are copied; no borrowed pointers or allocation.
 * Axis actions normalize unused button settings to momentary/false. */
typedef struct InputActionDescription {
    InputControllerId controller;
    char name[ROHR_INPUT_NAME_MAX];
    InputActionType type;
    InputButtonMode button_mode;
    bool button_initial_state;
    InputBindingList bindings_default;
} InputActionDescription;
ERROR_DECLARE_RESULT_TYPE(InputActionDescriptionResult, InputActionDescription);

/** Immutable action recipe without a live owner. Controller creation supplies
 * the owner and copies names, bindings, and initial state. */
typedef struct ControllerActionDescription {
    char name[ROHR_INPUT_NAME_MAX];
    InputActionType type;
    InputButtonMode button_mode;
    bool button_initial_state;
    InputBindingList bindings_default;
} ControllerActionDescription;
#define ROHR_CONTROLLER_DESCRIPTION_FIELDS \
    InputControllerDescription controller; \
    const ControllerActionDescription *actions; \
    size_t action_count
typedef struct ControllerDescription {
    ROHR_CONTROLLER_DESCRIPTION_FIELDS;
} ControllerDescription;

/** Enabled unnamed controller, no actions. Names remain globally unique.
 * Creation owns the returned controller and every action; destroy with
 * input_controller_destroy. Failure removes this attempt's controller/actions
 * without changing existing input state. No entity owner or heap allocation. */
ControllerDescription controller_description_default_get(void);
InputControllerIdResult rohr_internal_controller_create(ControllerDescription description);
static inline ControllerDescription rohr_internal_controller_recipe_get(
        const ControllerDescription *description) { return *description; }
static inline ControllerDescription rohr_internal_controller_empty_recipe_get(
        const InputControllerDescription *description) {
    return (ControllerDescription){.controller = *description};
}
/* Generated views borrow their original storage; only this small recipe header
 * is copied. As with entity/object creation, the argument executes once. */
#define ROHR_INTERNAL_CONTROLLER_VIEW_TYPES(apply)
#define ROHR_INTERNAL_CONTROLLER_TYPE(type, adapter) \
    , type *: adapter, const type *: adapter
#define controller_create(...) \
    rohr_internal_controller_create(_Generic(&(__VA_ARGS__), \
        ControllerDescription *: rohr_internal_controller_recipe_get, \
        const ControllerDescription *: rohr_internal_controller_recipe_get, \
        InputControllerDescription *: rohr_internal_controller_empty_recipe_get, \
        const InputControllerDescription *: rohr_internal_controller_empty_recipe_get \
        ROHR_INTERNAL_CONTROLLER_VIEW_TYPES(ROHR_INTERNAL_CONTROLLER_TYPE))(&(__VA_ARGS__)))

/** Per-frame committed and in-progress UTF-8 text input. */
typedef struct InputTextState {
    char committed[ROHR_INPUT_TEXT_CAPACITY];
    char composition[ROHR_INPUT_TEXT_CAPACITY];
    int composition_start;
    int composition_length;
    bool committed_truncated;
    bool composition_truncated;
    char candidates[ROHR_INPUT_TEXT_CANDIDATE_LIMIT]
        [ROHR_INPUT_TEXT_CANDIDATE_CAPACITY];
    size_t candidate_count;
    int selected_candidate;
    bool candidates_horizontal;
    bool candidates_truncated;
    bool active;
} InputTextState;

/** Clear transient snapshot values before polling the next frame's events. */
void input_frame_begin(void);

bool input_key_down_check(SDL_Scancode key);
bool input_key_pressed_check(SDL_Scancode key);
bool input_key_released_check(SDL_Scancode key);
SDL_Keymod input_modifiers_get(void);

bool input_mouse_button_down_check(InputMouseButton button);
bool input_mouse_button_pressed_check(InputMouseButton button);
bool input_mouse_button_released_check(InputMouseButton button);
Vec2D input_mouse_position_get(void);
Vec2D input_mouse_delta_get(void);
Vec2D input_mouse_wheel_get(void);
EngineResult input_mouse_relative_mode_set(bool enabled);
bool input_mouse_relative_mode_check(void);

EngineResult input_text_start(void);
EngineResult input_text_stop(void);
EngineResult input_text_area_set(SDL_Rect area, int cursor);
InputTextState input_text_state_get(void);

/** Validate one physical binding for the requested logical action type. */
bool input_binding_valid_check(InputActionType type,
    const InputBinding *binding);
/** Create a key binding from an SDL keycode, including required Shift state. */
InputBinding input_binding_key_create(SDL_Keycode key);
/** Create a physical-layout key binding directly from an SDL scancode. */
InputBinding input_binding_scancode_create(SDL_Scancode key);

InputControllerIdResult input_controller_create(InputControllerDescription description);
/** Enabled, with an empty name to fill before creation. */
InputControllerDescription input_controller_description_default_get(void);
/** Replace name/enabled atomically without rebuilding child actions.
 * Renaming and unchanged configuration preserve pending input transitions. */
EngineResult input_controller_description_set(InputControllerId controller,
    InputControllerDescription description);
/** Return an independent copy of current configuration. */
InputControllerDescriptionResult input_controller_description_get(InputControllerId controller);
EngineResult input_controller_destroy(InputControllerId controller);
InputControllerIdResult input_controller_by_name_get(const char *name);
/** Copied runtime name; stale controller IDs fail. */
InputControllerNameResult input_controller_name_get(InputControllerId controller);
EngineResult input_controller_enabled_set(InputControllerId controller,
    bool enabled);
bool input_controller_enabled_check(InputControllerId controller);

InputActionIdResult input_action_create(InputActionDescription description);
/** Momentary Button, false initial state, no bindings, invalid owner, empty name. */
InputActionDescription input_action_description_default_get(void);
/** Atomic full replacement, including controller reassignment with a stable handle.
 * Compatible overrides survive; incompatible overrides reject the entire edit.
 * Name-only/unchanged edits preserve state and transitions. Rebinding preserves
 * persistent state and resamples physical input without inventing transitions.
 * Changed type/mode/initial state reinitializes only this action. */
EngineResult input_action_description_set(InputActionId action,
    InputActionDescription description);
/** Copy authored configuration only, excluding runtime overrides and current state. */
InputActionDescriptionResult input_action_description_get(InputActionId action);
EngineResult input_action_destroy(InputActionId action);
InputActionIdResult input_action_by_name_get(InputControllerId controller,
    const char *name);
InputActionTypeResult input_action_type_get(InputActionId action);

EngineResult input_action_button_mode_set(InputActionId action,
    InputButtonMode mode);
InputButtonModeResult input_action_button_mode_get(InputActionId action);
EngineResult input_action_button_initial_state_set(InputActionId action,
    bool state);
bool input_action_button_initial_state_check(InputActionId action);
EngineResult input_action_button_state_set(InputActionId action, bool state);
EngineResult input_action_button_state_reset(InputActionId action);

/** Replace copied defaults; non-empty names must be unique within the list. */
EngineResult input_action_bindings_default_set(InputActionId action,
    const InputBinding *bindings, size_t count);
InputBindingListResult input_action_bindings_default_get(InputActionId action);
/** Replace copied overrides; non-empty names must be unique within the list. */
EngineResult input_action_bindings_override_set(InputActionId action,
    const InputBinding *bindings, size_t count);
InputBindingListResult input_action_bindings_override_get(InputActionId action);
InputBindingListResult input_action_bindings_effective_get(InputActionId action);
EngineResult input_action_bindings_override_clear(InputActionId action);
bool input_action_bindings_override_check(InputActionId action);

bool input_action_button_down_check(InputActionId action);
bool input_action_button_pressed_check(InputActionId action);
bool input_action_button_released_check(InputActionId action);
InputAxis1DResult input_action_axis_1d_get(InputActionId action);
InputAxis2DResult input_action_axis_2d_get(InputActionId action);

#endif
