# Building a game with authored descriptions

Include `src/generated/project.h` and compile `project_objects.c`. The header
exposes immutable object, member and viewport descriptions. Create mutable
instances through the engine, update their state, and show a viewport; application
camera callbacks and per-object draw calls are unnecessary.

The [player controller](player_controller/src/main.c) is a small complete game
loop with authored content and developer-owned movement/jump logic. The
[soft-body car](soft-body/src/main.c) combines one authored object with terrain
and particles constructed in C. All graphical examples use automatic viewports;
`game-state` remains a headless serialization demonstration.

## Create, bind, update, show

Using the player controller project's generated names, initialization is:

```c
EntityResult created = rohr_object_create(project.player_world);
if (rohr_error_check(created)) goto fail;
Entity world = created.result.value;

EntityResult member = rohr_object_member_get(world, project.player_world.player.name);
if (rohr_error_check(member)) goto fail;
Entity player = member.result.value;

ViewportObjectBinding binding = { &project.player_world._description, world };
ViewportCreateDescription description = {
    .template = &project.game_viewport,
    .objects = &binding,
    .object_count = 1,
};
ViewportIdResult view = rohr_viewport_create(description);
if (rohr_error_check(view)) goto fail;
ViewportId viewport = view.result.value;
```

Start the engine and graphics before this sequence, apply `project_settings_apply()`
and create any authored input controllers. Track each successful creation for
cleanup on both failure and normal exit, as the complete example does.
The binding chooses the camera belonging to this instance. Creating another
`player_world` yields independent entities, cameras and mutable state. Each
viewport can bind to the intended instance explicitly.

In each frame, begin/poll input, update game state and the engine clock, advance
physics using the selected tick settings, update sprite animations when present,
and call `rohr_graphics_viewport_show(viewport)`. Check each update's report for
failure. Use `rohr_graphics_viewports_show(ids, count)` to compose several
viewports and present once per frame. Rendering does not advance physics, audio,
or animations for you. The example's movement and jump code stays in `main.c`.

The scene is discovered from live appearance, visibility, sprites and soft-body
visuals. Newly created entities appear without draw-list registration. A raw
physics hitbox needs explicit appearance to become visible. Named layers retain
identity independently of their numeric draw order; equal numbers use discovery
order. Camera layer allowlists/blocklists remain future work.

For a standalone member, use `rohr_entity_create(project.player_world.player)`.
Members referring to other entities need their documented creation bindings;
see [shared creation](../docs/shared_creation.md). Descriptions stay immutable.
Destroy the viewport before its borrowed camera/object, then delete the object
root with `rohr_entity_delete(world)`. That deletes its owned members. Destroy
input controllers and remaining caller-owned resources before engine shutdown.

## Extend an existing viewport

Look up an authored mount with `rohr_viewport_item_by_name_get`. Change its
placement/visibility through item get/set APIs, or add screens, shapes, text and
sliders using the viewport APIs. New controls can be added entirely in C.
Recipe-created resources and UI created by the viewport convenience functions
are owned by the viewport. Separately created resources mounted by handle remain
caller-owned; cameras are borrowed. Keep caller-owned text alive until its UI has
been destroyed. See [automatic viewports](../docs/automatic_viewports.md) for
binding, shared-screen, and teardown details.

The [audio](audio/src/main.c) and [UI](user-interface/src/main.c) examples create
persistent controls without a camera. The [extreme-mass diagnostic](extreme_mass_drop/src/main.c)
uses persistent UI rectangles for its last finite physics observations, preserving
halted snapshots without creating extra simulated entities.

## Generate and build on Linux

For a standalone game, extract the matching generic Linux SDK into a versioned
folder outside the game's distribution output, for example
`~/sdk/rohr/<version>/`. Your game's `dist/` can then hold its runnable output.
Generate the starter from the editor or SDK CLI. Keep gameplay in the generated
starter's `src/main.c`; subsequent generation preserves developer-edited entry
points and build files. Editor-owned files under `src/generated/` are replaceable.

For a bundled example, from this checkout after installing the SDK:

```sh
cmake -S examples/player_controller -B build/player_game \
  -DCMAKE_PREFIX_PATH="$HOME/sdk/rohr/<version>"
cmake --build build/player_game
./build/player_game/PlayerController
```

Bundled examples use the adjacent `examples/cmake` helpers; copy the examples
layout together if building away from this checkout. Generated standalone game
projects have their own build files. Assets are staged beside each executable,
and examples switch to that directory on startup, so launching from another
working directory works.

Regenerate edited authored data with `rohr-cli --project <project-dir> generate-c`
or the editor, then rebuild against the same SDK version. Legacy generated
`project_viewports.h/.c` support has been removed. Customized older entry points
must adopt `project.h` and explicit viewport creation; regeneration deliberately
does not replace developer gameplay. The [migration guide](../docs/shared_creation.md#unified-generated-project)
explains that transition.

The [integrated SDK verification](../docs/creation_rendering_verification.md)
records generic Linux packaging, Windows cross-builds, lifecycle checks and
measured rendering costs. Its generated workflow consumer demonstrates two
independent instances and application-created inset screens and controls.
