# Editor guide

The Rohr editor is a visual authoring tool for engine objects. It stores editable
project data as JSON and generates C source that is compiled with the game. Game
runtime behavior remains developer-owned C code.

## Build and run

From the engine repository:

```sh
./dev.sh build
./build/tools/rohr-gui/rohr-gui
```

For an installed SDK, launch `bin/rohr-gui` or place `bin/` on `PATH` and run
`rohr-gui`.

On startup, choose **New Project** or **Load Project**. The directory browser
starts in the current working directory. Single-click a directory to select it
and preview its contents; double-click it to enter it. `..` behaves the same way.

For a new project, navigate to the parent directory, enter the new directory
name, and choose **Create Project**. The editor creates a working starter scene
containing a static floor and a gravity-enabled box.

## Generated project layout

The default workspace is:

```text
my-game/
├── CMakeLists.txt
├── project.rohr.json
├── assets/
├── objects/
│   └── project.rohr.json
└── src/
    ├── main.c
    └── generated/
        ├── project_objects.c
        └── project_objects.h
```

`project.rohr.json` at the root is the workspace manifest. The JSON under
`objects/` is editor-owned scene data. Files under `src/generated/` are
replaceable generated output. `src/main.c` is created with a new project but is
not overwritten by **Generate C**.

Saving and generating are separate operations:

- **File > Save** writes the workspace manifest and editor JSON.
- **Build > Generate C** replaces only the generated object source and header.
- **File > Close** closes the project and asks about unsaved changes.
- **File > Exit** safely exits and asks about unsaved changes.

Configure and build a generated game from its project directory:

```sh
rohr-cli --project . build
./build/MyGame
```

The installed CLI locates the Rohr SDK beside itself. When configuring manually,
provide the unpacked SDK to CMake:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/rohr
cmake --build build
```

## Lua configuration

The SDK provides `share/rohr/editor.lua`. New projects receive a portable
`editor.lua` in the project root. The CLI resolves build settings in this order:

1. Project `cli` override.
2. Project shared `project` setting.
3. SDK `cli` override.
4. SDK shared `project` setting.

The SDK shared setting is the final configured default.

Commands are argument arrays rather than shell strings. This preserves paths
containing spaces and does not invoke a shell:

```lua
return {
    editor = {
        config_path_override = nil,
        gui_state_path = "config/editor_gui_settings.lua",
        gui_state_path_override = nil,
    },

    build = {
        project = {
            configure = {
                "cmake", "-S", "{project}", "-B", "{build}",
                "-DROHR_ENGINE_SOURCE_ROOT={sdk}",
            },
            compile = { "cmake", "--build", "{build}" },
        },

        cli = {
            configure = nil,
            compile = nil,
        },

        gui = {
            configure = nil,
            compile = nil,
        },
    },
}
```

For each build operation, `nil` inherits the next configuration layer, `{}`
explicitly disables the operation, and a non-empty string array executes that
command.

Relative paths declared by the SDK `editor.lua` resolve from the SDK root.
Absolute paths remain unchanged, and paths beginning with `~/` resolve from the
user home directory. `editor.config_path_override` remains opt-in.

The GUI loads and atomically updates `editor.gui_state_path`. When
`editor.gui_state_path_override` is set, that path becomes the managed state
file instead. The initial flat state stores `logical_width`, `logical_height`,
`aspect_ratio`, and `window_mode`; missing override files begin with the SDK
state values.

`{project}` and `{build}` expand to absolute paths. `{sdk}` expands to the SDK
root when the CLI runs from an installed SDK. Missing fields inherit the next
configuration layer. Malformed fields stop the build with the Lua filename and
an error message.

Configuration files execute as real Lua, but the editor does not expose Lua's
OS, I/O, package, or debug libraries while loading them. The CLI reads these
files and never rewrites them. The GUI consumes its own layered overrides.

The GUI's **Settings > Build** menu shows the effective configure and compile
commands as Lua arrays. Applying an edited field validates that it is a
sequential array of strings using only `{project}`, `{build}`, and `{sdk}`
placeholders. GUI-owned overrides are written atomically to
`.rohr/gui-overrides.lua`; `editor.lua` remains untouched. Each field can be
reset independently to its inherited value. The command fields show three
lines at once, wrap long Lua expressions, and scroll vertically when focused.
Validation runs while editing and reports success or the specific field error.
Apply saves only when both Lua command arrays are valid and their first
arguments resolve to executable files through explicit paths or `PATH`. Windows
lookup also checks the standard executable extensions without running the file.
An Apply failure creates a bottom-left notification reading
`Build configuration (GUI) - FAIL`. Clicking it opens a detailed report with
the editor parser error followed by Lua's raw error; invalid configuration is
never written. A successful Apply saves the overrides and immediately starts
the configured configure-and-compile sequence, so a separate test-command
action is unnecessary.

The **Settings > Visuals** menu controls window mode, logical aspect ratio, and
resolution. Windowed is the default; borderless fullscreen and display-mode
fullscreen are available from the Window Mode dropdown.
The editor defaults to **Auto**, matching the current window or fullscreen
output without letterboxing. Fixed 16:9, 16:10, 4:3, and 21:9 options are also
available at 720p, 1080p, and 1440p logical heights.

Editor dropdowns show at most three option rows. Shorter lists use only the
rows they need, while longer lists provide wheel and scrollbar navigation.

Controllers are top-level resources beside Objects and Viewports. **Add
Controller** creates a named logical controller. Opening that element edits only
that controller and presents **Add Action** plus one selectable element per
Button, Axis 1D, or Axis 2D action. Double-clicking an action opens its own
editor. Action type, Button mode, binding selection, binding source, mouse
button, and pointer-axis choices use dropdowns. Persistent actions can author
their initial state, and every action can own multiple keyboard or pointer
bindings. Binding properties follow the action type: Buttons omit scale and
inversion, Axis 1D shows one scale and inversion pair, and Axis 2D shows
independent X/Y scale and inversion. Digital Axis 2D bindings additionally show
X/Y direction; pointer movement and wheel bindings use scale as sensitivity.
The binding name remains visible above the accordions. **Input** provides
**Affects Action Axis X** and **Affects Action Axis Y** controls for Axis 2D;
only enabled **Action Axis X Effect** and **Action Axis Y Effect** accordions
appear. Disabling an effect hides it without discarding its direction, scale,
or inversion. For pointer bindings, the physical axis dropdown and Action Axis
effect controls express the source-to-output relationship separately. Axis 1D
uses one **Axis Effect** accordion.
All changes use normal undo/redo history. Key inputs use SDL scancode
values; the numeric representation toggle is labelled **SDL Scancode**.
Generated C preserves binding names and exposes stable controller and action
handles through `ProjectControllers`. Gameplay reads those handles and owns
responses; callbacks are not serialized. See [Input](input.md) for runtime
overrides and CLI syntax.

Add buttons for named project, object, UI, and nested elements immediately
open the created element and select its generated name in the name field.
Typing replaces the default name without an extra click or manual select-all
step. Elements without names keep their existing creation flow.

The bottom-left **Notification Log** button is always available. Up to three
new notifications appear above it, with a fourth replacing the oldest visible
notification. The log retains the latest 100 notifications independently of
that visible stack. Its entries are shown newest first in a scrollable menu;
clicking either a visible notification or a log entry opens the same detailed
report. A visible notification expires after 10 seconds but remains available
in the log. Its top-right **x** removes it from the visible stack immediately
without deleting its log entry. The log closes through its Close button, Escape,
or by clicking **Notification Log** again. Opening a report from the log keeps
the log underneath it, so closing the report returns to the same log view.
Detailed reports scroll when their wrapped text is taller than the
report area. Asynchronous configure or compile failures also create a build
failure notification; visible builds retain their complete output in the
terminal. Successful C generation reports the generated files, object count,
and whether the object tree was written to the terminal. Successful compilation
is reported only after configure and compile both finish, with the project path
and terminal-output state in its detailed report.

The executable name is the PascalCase project directory name (`my-game`
becomes `MyGame`).

## Editing model

An object is an authored collection, similar to a prefab definition. It may own:

- rigid bodies and their hitboxes;
- particles represented by particle-enabled rigid bodies;
- anchors and joints;
- soft bodies, nodes, beams, and generated triangular areas.

Object names are formatted as PascalCase because they become generated C type
names. Child property names are formatted as snake_case because they become
generated fields and identifiers.

The right column shows the hierarchy or the editor for the current item:

- Single-click selects an item.
- `Ctrl` + click toggles items of the same type in a multi-selection. Selecting
  another type replaces the current selection.
- Double-click opens its editor.
- `Enter` opens the selected item.
- `Escape` returns to the parent editor. At the root it deselects the object.
- `Delete` removes a deletable selected item.
- Arrow keys navigate the column while the pointer is over it.
- The red button at the bottom deletes the item whose editor is open.
- Eye controls change editor visibility only; hidden items remain project data.

Selection is shared between the hierarchy and viewport. Selecting an authored
item highlights its viewport representation. Clicking an empty viewport or
column background clears the current selection without jumping to the root.

When multiple items are selected, the column shows their shared property menu.
Its fields start empty: leaving a field empty preserves each item's current
value, while submitting a value applies it to every selected item. Bulk edits
and bulk deletion are atomic, so one `Ctrl+Z` or `Ctrl+Y` restores the complete
operation. Boolean bulk fields accept `true`, `false`, `1`, or `0`; color fields
accept six- or eight-digit hexadecimal colors.

## Viewport controls

- Left drag moves the selected draggable item.
- Right drag pans the camera.
- `Ctrl` + left drag beginning on empty viewport space also pans, for trackpads
  and one-button pointing devices. `Ctrl` + click on an item toggles selection.
- Mouse wheel zooms around the pointer.
- Arrow keys nudge the selected viewport item when the pointer is in the viewport.
- **World/Local** changes only the camera reference; it does not modify project data.
- Drag the divider between the viewport and tools column to resize the column.

Viewport clicks and context menus target the frontmost visible item at the
pointer, following the editor's draw layers and ordering. Selecting a body or
opening its child editor does not let clicks pass through a covering item.
Screens and UI follow their composition layers, including their order within
the same layer. Hide a covering item to select what is behind it. A drag already
in progress keeps its target until release or cancellation.

Rigid-body and soft-body editors show an origin and rotation handle. Dragging a
body interior translates it. Dragging the rotation handle rotates it. Opening an
origin editor moves the body while preserving authored local offsets. Vertices,
attached anchors, particle centers, and following visuals move with the new
origin; soft-body nodes retain their local offsets as well.

## Hitboxes and rigid bodies

A new rigid body starts with a triangular hitbox. In the hitbox editor, vertices
and lines are selectable and editable. Adding a vertex splits the selected line
at its midpoint without moving neighboring vertices. Locked vertices cannot be
moved; line-length edits distribute movement only to unlocked endpoints.

While a hitbox editor is open, clicks on its owning body or particle do not leave
the editor or drag the body. Exposed vertex and line handles retain priority
within that hitbox; covering items and sibling hitboxes still govern picking. Clicking
outside deselects the hitbox while leaving its editor open.

**Auto Shape** opens a three-column triangle, square, and circle picker. The
square option requires at least four vertices; triangle and circle require at
least three. Choosing a shape opens its parameter editor. Polygon corners are
always retained and additional vertices are distributed deterministically over
the perimeter. Circles distribute every vertex at an equal angle. Existing
vertex IDs and names are preserved.

Choosing a shape applies it immediately using the displayed defaults. Editing a
parameter reapplies the shape when that field is submitted with Enter, exited
with Escape, or loses focus.

While its shape editor remains open, the geometry stays constrained. Dragging
a circle point changes its radius. Dragging a square corner changes its width
and height. Triangle corners change the dimensions allowed by the selected
triangle type; a scalene apex also changes its apex offset. Additional points
distributed along polygon edges are derived points and cannot be dragged
independently.

Triangle modes are equilateral, isosceles, and scalene. Equilateral height is
derived from width, isosceles centers its apex using editable width and height,
and scalene adds an editable horizontal apex offset. Width and length
independently control the square tool, so unequal values produce a rectangle.

Rigid bodies expose mass, friction, restitution, gravity, motion type, rotation
locking, colors, and collision filtering. Collision filtering has two sets:

- **Collision Category** describes what the body is.
- **Collide With** describes categories it accepts.

A pair responds only when both directional filters accept one another.

## Center of mass persistence and CLI

Project data uses format version 3; older project data must be rewritten.
The workspace manifest version is unchanged. Rigid-body records store
`center_of_mass` as `{"mode": "automatic"}` or
`{"mode": "explicit", "offset": {"x": 3, "y": -2}}`.
Omitting it selects automatic mode. Invalid COM input returns an error and
preserves the previous successful project description.

Use the existing body/property CLI selectors:

```sh
rohr-cli --project ./objects/project.rohr.json --object car --body chassis --property center-of-mass explicit 3 -2
rohr-cli --project ./objects/project.rohr.json --object car --body chassis --property center-of-mass automatic
```

Selecting automatic discards the explicit offset. Generated C applies this
configuration before simulation, including bodies without geometry. Inertia
is always derived. Standalone particles reject explicit COM; their COM stays
at the particle centroid, even when collision response is disabled. Editor COM controls and visualization follow in
the next milestone goal.

## Particles

**Add Particle** creates a standalone translation-only particle. Its authored
radius, local origin, and **Rigid Vertices** produce one synchronized circular
particle shape and polygon hitbox. Generated C exposes a reusable
`<object>_<particle>_config` constant and creates the placed instance through
`rohr_physics_particle_create()`, so game code can copy the same config to
create additional particles.

Enable **Collision**, then enable **Particle** on a rigid body. The viewport
shows a dotted particle ring with its fill behind rigid bodies. Single-clicking
the ring selects it, double-clicking opens the particle editor, and dragging the
ring translates its rigid body.

Ordinary rigid bodies with Particle enabled retain their rotation handle in
both the rigid-body and particle views. A rotation drag creates one undo/redo
entry. Standalone particles do not expose this rotation handle.

The particle editor provides local origin X/Y, radius, ring color, fill color,
and **Auto Fit**. The origin is relative to the rigid-body origin and follows
its rotation. Auto-fit is enabled by default and sets the radius to the largest
particle-origin-to-vertex distance. Moving a vertex updates the fitted radius.
Disabling auto-fit preserves its last value, after which radius can be edited
manually.

For ordinary rigid bodies with Particle enabled, particle geometry remains
independent from the polygon hitbox. Generated C retains the authored polygon
and separately calls the particle-origin and particle-radius APIs. This path
does not turn the body into a standalone particle configuration.
Particle/particle pairs use circles; particle/ordinary rigid-body pairs use
their polygon hitboxes.

## Anchors and joints

An anchor refers to at most one rigid body. Its position and orientation may
follow that body independently, or remain in object/world space. Anchors can be
reused by multiple joints and are not automatically deleted when a joint changes
or is removed.

Joints select two anchors. Springs expose rest length, stiffness, and damping.
Revolute joints support damping. Pin and weld constraints update connected
bodies during editing so authored relationships remain visible.

## Soft bodies

Soft bodies own nodes and beams. Nodes provide mass, radius, friction,
restitution, gravity, particle collision filters, and color overrides. Beams
provide stiffness, damping, endpoints, color overrides, and independent
thick-segment collision settings. A newly authored beam copies its initial
settings from its endpoint nodes: collision is enabled only when both nodes
enable collision, its category and collide-with masks are the unions of the
node masks, and its thickness is the smaller endpoint diameter. These values do
not continue to inherit. Later node-radius changes only cap an oversized beam
or disable it when no thickness is valid; growing a node does not grow or
re-enable an existing beam. Collision, thickness, and both filter masks remain
individually editable in the beam panel.

Closed node-and-beam loops generate colorable areas automatically. A hexagonal
loop becomes one area; cross-section beams divide an enclosure into multiple
independently selectable areas. Beam crossings without nodes are not topology
and do not divide an area. Concave areas are triangulated internally for drawing,
while the editor preserves one boundary and one color setting for the complete
area. Triangles never define collision. Disable an area's **Surface** option to
leave that area visually open while keeping its authored beams and their
independent collision settings. Physical holes are the open spaces left by the
beam layout; disabling a visual surface does not remove boundary collision.

Areas may override the parent area color and their boundary beams may override
the parent beam color. When **Inherit** is selected, the local color control is
disabled and follows the soft-body color.

Dragging any node, beam, or filled area while in the soft-body editor translates
the whole soft body. Double-click nodes, beams, or areas to open their individual
editors.

Soft bodies provide the same **Auto Shape** tool. It repositions existing nodes
without replacing node IDs, names, beams, or area relationships.

## Current boundary

The editor authors structural data and generates object creation/destruction and
drawing support. Controls, gameplay rules, torque, timed spawning, scoring,
recording, and other runtime behavior belong in developer-owned C source.
