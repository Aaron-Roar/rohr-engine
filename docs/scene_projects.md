# Application and scene entry points

New projects have two developer-owned entry points:

- `src/main.c` demonstrates creating starter bodies through `objects.starter.*`,
  spawning a whole soft body, and explicitly connecting separately spawned joints.
  Attached visuals and anchors are created with their bodies. It also spawns
  `objects.tug` as a complete instance: `box_1`, `box_2`, `tug_spring`, and two
  opposing persistent forces.
- `src/scene.c` creates the entire current authored scene through
  `project_objects_create_all`, then creates the authored viewports and runs the
  ordinary engine, physics, animation, and presentation loop.

The application target is `${PROJECT_NAME}`; the independent scene target is
`${PROJECT_NAME}_scene`. A default build builds both. Each target stages assets
beside its executable and sets its working directory there before loading them.
They work with both a source-tree engine and the installed SDK. For example:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/rohr-sdk
cmake --build build --target MyGame
cmake --build build --target MyGame_scene
```

At this milestone's first goal, both executables run in ordinary application
windows. Embedded simulation hosting, editor Build/Run commands, tabs, and
Run/Pause/Stop controls are subsequent goals; they are not implemented yet.

## Source ownership and regeneration

`src/generated/` remains editor-owned. Generating C refreshes descriptions,
settings, binding helpers, viewport composition, and `project_scene.cmake`.
Existing `main.c` and `scene.c` are preserved. A missing `scene.c` is recreated
from the generic scene template, including for existing projects. Deleting the
file intentionally restores that default on the next generation.

New tutorial content is added only when creating a new project. Existing scenes
are not given a `Tug` object automatically. The tutorial main is an editable
example, not a synchronized inventory: after renaming or removing its referenced
objects, update its explicit calls. The generic scene entry point continues to
follow all authored objects when C is regenerated.

An exact match of the preceding generated CMake scaffold receives this line:

```cmake
include(src/generated/project_scene.cmake)
```

Customized CMake files are preserved. Generation logs the integration step when
that include is absent. Define `ROHR_ENGINE_TARGET` and `ROHR_GENERATED_SOURCES`
before including the module, as in the default scaffold. Projects with an
external build system can compile `src/scene.c` with the generated C modules
and link the engine themselves. Only an exact recognized old tick-loop template
is automatically migrated in `main.c`; marker comments alone are insufficient.

## Generated bindings and lifetime

`project_settings_apply()` applies the generated timestep, gravity, substeps,
and solver settings without spawning objects or controllers.

Each generated object type exposes `<object>_instance_bind(output, instance)`.
It resolves typed aliases within an already-created ownership tree; it does not
spawn entities, connect separate bodies, or search unrelated entities by name.
Failure leaves the output unchanged and the input instance alive. On success,
the generated `<object>_destroy` helper owns destruction of that instance. Bind
each instance to one owning output, and destroy the previous output before
replacing it. Keep it alive while generated viewports borrow its camera and
entity handles.

The tutorial records ownership immediately after every creation so partial
initialization is safe to destroy. Soft-body topology remains inside its spawn.
Whole-object spawning resolves internal connections automatically; separately
spawned joints are connected explicitly using the returned body-anchor handles.
`project_objects_create_all` uses the same catalog path, including authored
appearance and placement. Existing individual generated constructors remain
available. Drawing visits each object's own visuals without an additional global
sprite pass.

## Verification

`editor_project_test` covers scaffold creation, source preservation, missing
scene regeneration, exact CMake migration, customized-build preservation, and
starter persistence. `tools/editor_core/tests/generated_scene` is an independent
installed-SDK check that compiles the actual new tutorial main and generated
modules. Create a fresh project first, then configure it with
`SCENE_PROJECT_DIR` and `CMAKE_PREFIX_PATH`. It checks both tutorial and generic
scene creation, joint/force bindings, independent instances, viewport setup,
force-driven spring motion, failed binding/asset-load cleanup, and teardown.
