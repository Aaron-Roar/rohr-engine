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

## Logical controllers and actions

A logical input controller is an engine-owned named context such as `gameplay`,
`menu`, or `console`. It is not a physical gamepad. Disabling a controller makes
every action in it query as neutral without changing bindings or persistent
Button state. Enabling, disabling, reconfiguring, or rebinding a controller does
not synthesize pressed or released transitions.

Actions have stable generation-checked identities and one logical type:

- `INPUT_ACTION_BUTTON` produces down, pressed, and released state. Momentary
  Buttons follow their combined bindings. Persistent Buttons toggle once when
  the combined binding state changes from inactive to active.
- `INPUT_ACTION_AXIS_1D` sums its bindings and clamps the result to `[-1, 1]`.
- `INPUT_ACTION_AXIS_2D` sums vectors and normalizes results whose magnitude is
  greater than one, preventing faster diagonal movement.

Bindings are evaluated in stored order and combined deterministically. Button
bindings are active when any valid binding is down, and do not use scale or
inversion. Every bit in a binding's modifier mask must be active. Axis 1D uses
`scale.x` and `inverted_x`. Axis 2D applies `scale.x` and `scale.y` independently,
then flips the sign of each component selected by `inverted_x` or `inverted_y`.
For pointer movement and wheel sources, those scales act as per-axis
sensitivity. Digital Axis 2D bindings also use their authored direction vector.
Binding names are optional in direct C, must
be unique within an action when present, and are retained by copied binding
lists. Key bindings use SDL scancodes; prefer SDL's
named `SDL_SCANCODE_*` constants in C so bindings remain readable and tied to
physical keys. Numeric scancode values remain valid for serialization and
tooling.

`rohr_input_binding_key_create(SDLK_*)` is the preferred authoring helper for
letters, number keys, and other logical SDL keys. It converts the keycode to
the current SDL scancode and carries any required Shift modifier. Use
`rohr_input_binding_scancode_create(SDL_SCANCODE_*)` when the physical key
position is intentional.

The tagged `InputBinding` sources currently support keys, pointer buttons,
pointer movement, and pointer wheels. The representation can gain SDL gamepad
buttons, triggers, and sticks without changing action types or the
gameplay-facing action queries. Gamepad discovery and lifetime management are
not implemented yet.

```c
InputControllerIdResult controller_result =
    rohr_input_controller_create("gameplay");
InputActionIdResult jump_result = rohr_input_action_create(
    controller_result.result.value, "jump", INPUT_ACTION_BUTTON);
InputBinding defaults[] = {
    {.name = "keyboard", .source = INPUT_BINDING_KEY,
        .input.key = SDL_SCANCODE_SPACE},
    {.name = "pointer", .source = INPUT_BINDING_MOUSE_BUTTON,
        .input.mouse_button = INPUT_MOUSE_BUTTON_LEFT},
};
rohr_input_action_bindings_default_set(
    jump_result.result.value, defaults, 2);
```

Controllers own their actions. Destroying a controller invalidates all of its
action handles. Bindings are copied into engine storage, and returned binding
lists are copies, so callers retain no borrowed allocation. Limits are declared
in `input.h` as
`ROHR_INPUT_CONTROLLER_LIMIT`, `ROHR_INPUT_ACTION_LIMIT`, and
`ROHR_INPUT_BINDING_LIMIT`.

## Persistent Buttons

Configure Button behavior with `rohr_input_action_button_mode_set()`. A
persistent Button starts at its authored initial state, then toggles only on the
combined bindings' inactive-to-active transition. Pressing a second binding
while another remains active does not toggle it again.

`rohr_input_action_button_state_set()` changes the runtime state explicitly, and
`rohr_input_action_button_state_reset()` restores the authored initial state.
Those operations report logical pressed or released transitions when the
controller is enabled. Configuration changes and controller enable changes
clear pending transitions instead of inventing new ones.

## Defaults and runtime rebinding

Editor-authored and generated bindings are action defaults. Runtime user
rebinding uses the separate override APIs. An active override fully replaces
the default list, including when the override list is empty. Clearing the
override restores the defaults.

Rohr deliberately does not write runtime overrides into project JSON. A game
that persists player preferences owns that save file and reapplies its copied
override bindings after generated project input is created.

## Editor, JSON, generated C, and CLI

Controllers are top-level editor resources beside Objects and Viewports. Each
controller element opens directly into that controller instead of selecting
from a second controller list. **Add Action** creates a selectable child element;
opening it provides the action editor for type, momentary or persistent Button
behavior through a Mode dropdown, persistent initial state, and named
default-binding children. Each
binding opens its own editor and has a stable editor ID. Clicking a key field
captures one key and then ends capture; Ctrl, Shift, Alt, and GUI can be
captured alone or held while another key is pressed, in which case the modifier
field is populated automatically. The modifier mask can also be edited
directly. Input and modifier fields can toggle between letter/symbol display
and their numeric SDL values; numeric fields accept Enter or focus loss as
submission. These edits use normal editor commands and participate in undo/redo.
Project JSON stores stable controller, action, and binding IDs plus readable
names. Generated `InputBinding` initializers retain those binding names.
Generated `ProjectControllers` state creates runtime controllers before generated
scene objects and is destroyed with `ProjectObjects`.

The selector-first CLI supports the same authored state. Bindings can be
selected by `--binding`, `--binding-id`, or the legacy `--binding-index`.
Every binding starts with source, physical input, and modifier bit mask. Button
bindings stop there. Axis 1D adds scale and inversion. Axis 2D adds scale X,
scale Y, inversion X, inversion Y, direction X, and direction Y. The old
seven-value uniform-scale form remains readable for compatibility.

```sh
rohr-cli --project objects/project.rohr.json \
  --controller gameplay add true

rohr-cli --project objects/project.rohr.json \
  --controller gameplay --action move add axis-2d

rohr-cli --project objects/project.rohr.json \
  --controller gameplay --action console add button persistent false

rohr-cli --project objects/project.rohr.json \
  --controller gameplay --action move --binding move_up \
  add key W 0 1 1 false false 0 -1

rohr-cli --project objects/project.rohr.json \
  --controller gameplay --action move --binding move_up \
  binding-set move_forward key W 0 1 1 false false 0 -1

rohr-cli --project objects/project.rohr.json \
  --controller gameplay --action move --binding move_forward delete
```

Sources are `key`, `mouse-button`, `mouse-motion`, and `mouse-wheel`. Keys
accept an SDL scancode number or SDL scancode name. Pointer buttons accept a
number or `left`, `middle`, `right`, `x1`, or `x2`. Motion and wheel inputs use
`x`, `y`, or `xy`; Axis 1D actions require a single component.
