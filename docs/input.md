# Input

Rohr owns one keyboard, pointer, text-input, and action snapshot. Call
`rohr_input_frame_begin()` once at the beginning of each frame, then drain
events through `rohr_engine_event_poll()`. The event poller automatically adds
every returned SDL event to the snapshot, so application code must not forward
events to the input system itself.

```c
while(running) {
    SDL_Event event;
    rohr_input_frame_begin();
    while((event = rohr_engine_event_poll()).type != 0) {
        if(event.type == SDL_EVENT_QUIT) running = false;
    }

    if(rohr_input_action_button_pressed_check(exit_action)) running = false;
}
```

Calling `rohr_input_frame_begin()` clears only transient state: pressed and
released transitions, pointer movement, wheel movement, and committed text.
Held keys and buttons, absolute pointer position, IME composition, and IME
candidates persist until an event changes them. Losing keyboard focus releases
all held keys and pointer buttons to prevent stuck input.

## Raw input

Raw queries use SDL scancodes so keyboard controls describe physical keys
independently of the active keyboard layout. Pointer state includes left,
middle, right, X1, and X2 buttons, absolute window position, accumulated
per-frame movement, and accumulated wheel movement. SDL trackpad pointer and
scroll events use these same mouse APIs. Rohr does not define nonportable
trackpad gesture actions.

Relative pointer mode is applied to the window associated with the latest
pointer event, then the focused pointer or keyboard window. It returns an error
when no suitable window exists.

Text entry is separate from key actions. Start and stop it with
`rohr_input_text_start()` and `rohr_input_text_stop()`, and use
`rohr_input_text_area_set()` to tell the platform where to place an IME. The
copied `InputTextState` contains per-frame committed UTF-8, persistent
composition text and selection, and a bounded candidate list. Truncation flags
make fixed-capacity loss explicit.

## Actions and maps

An action map is an engine-owned named context such as `gameplay`, `menu`, or
`console`. Disabling a map makes every action in it neutral without changing
its bindings. Enabling, disabling, or rebinding a map does not synthesize
pressed or released transitions.

Actions have stable generation-checked identities and one logical type:

- `INPUT_ACTION_BUTTON` produces down, pressed, and released state.
- `INPUT_ACTION_AXIS_1D` sums its bindings and clamps the result to `[-1, 1]`.
- `INPUT_ACTION_AXIS_2D` sums vectors and normalizes results whose magnitude is
  greater than one, preventing faster diagonal movement.

Bindings are evaluated in stored order and combined deterministically. Button
bindings are active when any valid binding is down. Every bit in a binding's
modifier mask must be active. Axis scale is applied before clamping;
`inverted` negates the scaled contribution. Digital Axis 2D bindings also use
their authored direction vector.

The tagged `InputBinding` sources currently support keys, pointer buttons,
pointer movement, and pointer wheels. The representation can gain SDL gamepad
buttons, triggers, and sticks without changing action types or the
gameplay-facing action queries. Gamepad discovery and lifetime management are
not implemented yet.

```c
InputActionMapIdResult map_result =
    rohr_input_action_map_create("gameplay");
InputActionIdResult jump_result = rohr_input_action_create(
    map_result.result.value, "jump", INPUT_ACTION_BUTTON);
InputBinding defaults[] = {
    {.source = INPUT_BINDING_KEY, .input.key = SDL_SCANCODE_SPACE},
    {.source = INPUT_BINDING_MOUSE_BUTTON,
        .input.mouse_button = INPUT_MOUSE_BUTTON_LEFT},
};
rohr_input_action_bindings_default_set(
    jump_result.result.value, defaults, 2);
```

Maps own their actions. Destroying a map destroys all of its action handles.
Bindings are copied into engine storage, and returned binding lists are copies,
so callers retain no borrowed allocation. Limits are declared in `input.h` as
`ROHR_INPUT_ACTION_MAP_LIMIT`, `ROHR_INPUT_ACTION_LIMIT`, and
`ROHR_INPUT_BINDING_LIMIT`.

## Defaults and runtime rebinding

Editor-authored and generated bindings are action defaults. Runtime user
rebinding uses the separate override APIs. An active override fully replaces
the default list, including when the override list is empty. Clearing the
override restores the defaults.

Rohr deliberately does not write runtime overrides into project JSON. A game
that persists player preferences owns that save file and reapplies its copied
override bindings after generated project input is created.

## Editor, JSON, generated C, and CLI

The editor's **Settings > Input** panel creates maps, typed actions, and default
bindings. These edits use normal editor commands and participate in undo/redo.
Project JSON stores stable editor IDs and readable action/source names.
Generated `ProjectInput` state creates the runtime maps before generated scene
objects and destroys it with `ProjectObjects`.

The selector-first CLI supports the same authored state. A binding command has
seven values after its operation: source, physical input, modifier bit mask,
scale, inversion, direction X, and direction Y.

```sh
rohr-cli --project objects/project.rohr.json \
  --input-map gameplay add true

rohr-cli --project objects/project.rohr.json \
  --input-map gameplay --input-action move add axis-2d

rohr-cli --project objects/project.rohr.json \
  --input-map gameplay --input-action move \
  binding-add key W 0 1 false 0 -1
```

Sources are `key`, `mouse-button`, `mouse-motion`, and `mouse-wheel`. Keys
accept an SDL scancode number or SDL scancode name. Pointer buttons accept a
number or `left`, `middle`, `right`, `x1`, or `x2`. Motion and wheel inputs use
`x`, `y`, or `xy`; Axis 1D actions require a single component.

