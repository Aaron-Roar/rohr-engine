# Application and scene entry points

New projects have two developer-owned entry points:

- `src/main.c` demonstrates creating starter bodies through `objects.starter.*`,
  spawning a whole soft body, and explicitly connecting separately spawned joints.
  Attached visuals and anchors are created with their bodies. It also spawns
  `objects.tug` as a complete instance: `box_1`, `box_2`, `tug_spring`, and two
  opposing persistent forces.
- `src/scene.c` creates the entire current authored scene through
  `project_objects_create_all`, then creates the authored viewports and runs the
  scene runner's engine, physics, audio, animation, and presentation loop.

The application target is `${PROJECT_NAME}`; the independent scene target is
`${PROJECT_NAME}_scene`. The `${PROJECT_NAME}_project` aggregate builds both,
as does a default build. Each target stages assets
beside its executable and sets its working directory there before loading them.
They work with both a source-tree engine and the installed SDK. For example:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/rohr-sdk
cmake --build build --target MyGame
cmake --build build --target MyGame_scene
cmake --build build --target MyGame_project
```

The scene executable opens an ordinary window when run directly. It also supports
isolated hosting through the SDK scene runner and editor-core session API described
below. The Build and Run menus are available. Viewport tabs and playback controls
remain goal 4; **Simulate Scene** currently resolves and prepares the built scene
in a stopped state, reporting its executable path without launching it.

## Build and Run menus

| Action | Generates C | Build target |
| --- | --- | --- |
| Build Application | Yes | Application only |
| Build Scene | Yes | Scene only |
| Build Project | Yes | Application and scene |
| Generate App C | Yes | None |
| Compile App | No | Application only |

Build actions configure before compiling. **Run Application** starts the existing
application; **Simulate Scene** prepares the existing scene for playback. Neither
run action generates, configures, or compiles. Missing executables produce a
notification directing you to build first. Changes become available after an
explicit successful build and a new run request. A failed build never triggers
an implicit repair or rebuild.

Generated CMake writes target names and per-configuration executable paths under
`build/`. Run resolves the actual output path, including custom output names and
multi-configuration directories, choosing the newest existing executable.
Projects configured before this metadata existed fall back to the conventional
build directories. A failed linker that removes its old executable makes that
output unavailable; the editor does not retain separate binary snapshots.

Scene/project builds request scene shutdown and wait for the child to exit before
generation or configuration. Application/project builds and Compile App also stop
the editor-owned application before replacing it. Only one build and one
application run can be active. Opening, closing, or exiting a project cancels its
build and stops owned children, including when opening a replacement project
fails. Asset imports do not cancel builds.

Shown and hidden build output use the same argument-array process runner.
**Terminal > Show build operations** controls display only. Ctrl+C in the focused
terminal cancels an active build; a console interrupt is forwarded before the
editor or CLI exits. POSIX builds own a process group, with a two-second interrupt
grace period before forced termination. Windows builds own a kill-on-close job;
a GUI process without a console terminates the job directly. Child processes are
reaped before their owner is destroyed.

Custom GUI compile commands must include `{target}`, for example
`{"cmake", "--build", "{build}", "--target", "{target}"}`. A plain CMake build
command without an explicit target automatically receives the selected target.
Other commands and fixed target overrides are rejected instead of ignoring the
menu selection. When the CLI evaluates shared or CLI commands, `{target}` names the project aggregate.
Empty command arrays remain valid disabled stages; disabled compilation reports
SKIPPED rather than claiming a new executable was built.

`editor_build_test` covers argument boundaries, target selection, configuration
failure, disabled stages, output lookup, scene shutdown ordering, and cancellation
of descendants that ignore interrupts. The CLI override test checks shared target
expansion. Installed-SDK verification builds the targets independently and checks
that a broken application entry point does not block the scene target (and vice
versa).

## Source ownership and regeneration

`src/generated/` remains editor-owned. Generating C refreshes descriptions,
settings, binding helpers, viewport composition, and `project_scene.cmake`.
Customized `main.c` and `scene.c` are preserved. An exact copy of the preceding
whole-scene loop in `scene.c` is upgraded to lifecycle callbacks. A missing
`scene.c` is recreated from the callback template, including for existing projects. Deleting the
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

## Scene runner and lifecycle

`rohr_scene_run(argc, argv, callbacks)` (direct API: `scene_run`) is a blocking
entry point for the scene process. Include `rohr.h` or `scene.h`. The runner owns
engine/graphics startup and teardown; do not call it inside an already running
engine. `SceneCallbacks` borrows a context pointer for the duration of the call:

- `initialize(context)` returns `EngineResult` and creates entities, assets,
  controllers, and viewports. It is required.
- Optional `update(context, tick_delta, time)` returns a `Report`, runs after the
  engine clock/lifecycle update and before physics, and supplies gameplay logic.
- Required `shutdown(context)` releases callback-owned resources while engine
  services still exist. It runs once after every attempted initialization,
  including partial initialization failure. It must tolerate that partial state.

With no hosting arguments the scene uses a visible window. The host supplies
private arguments for a loopback port and random session token; entry points must
pass their original `argc` and `argv` to the runner. Customized old entry points
are never silently rewritten. Move their creation/destruction into callbacks and
replace their loop with `rohr_scene_run`; generation logs this guidance. Build
again after migration. A scene without the runner times out during hosted startup.

Hosted scenes own a hidden SDL window and renderer. Authored fullscreen modes do
not change the desktop. Capture happens after camera, screen, and UI composition
and before presentation, excluding native-window letterboxing. The transmitted
RGBA32 image has the scene's logical dimensions. Logical and physical dimensions
are limited to 4096 on each side. Frame capture waits for the previous frame to
leave the send queue; there is no growing frame backlog. Simulation continues
under frame backpressure. The hosted loop is capped at 60 frames per second;
readback, conversion, copying, and software/GPU performance may lower that rate.

Pause suspends gameplay, engine updates, physics, entity animation updates,
rendering, and the audio device without destroying their state. Pending audio
commands and queued samples remain intact. Resume resets the engine's real-time
baseline so pause duration does not become physics catch-up. Focus loss instead
releases held keys/buttons and suppresses new input while simulation and audio
continue. The host explicitly manages focus; a hidden SDL window's native focus
is not used. No physics stage or timestep policy changes.

## Editor-core host

`editor_scene_session.h` defines an explicitly allocated, single-threaded session.
`launch` accepts an executable path and starts a child; `update` polls startup,
frames, controls, logs, process exit, and timeouts without waiting on sockets or
process completion. Request acceptance is distinct from a later acknowledged
RUNNING/PAUSED state. The API does not generate code or compile applications.

`pause_set`, `focus_set`, and `input_add` queue controls. `stop` is asynchronous:
continue updating until `active_check` is false before relaunching. Stop discards
pending input, requests graceful cleanup, then force-terminates an unresponsive
child after two seconds and reports the failure. Startup times out after ten
seconds. Exited processes are reaped; each launch resets the previous frame/log.
`destroy` also stops and reaps any owned process, allowing up to two seconds for
graceful cleanup. Keep normal UI teardown asynchronous by stopping first.

A frame getter returns borrowed pixels valid until the next update, launch, or
destruction. Never pass an entity, camera, texture, SDL resource, or callback
pointer across this boundary. `pointer_map` converts an editor point through a
centered aspect-preserving destination rectangle into logical scene coordinates;
points outside return false but retain coordinates for pointer capture. Scale
relative motion by the same ratio. Send keyboard, pointer button/motion/wheel,
UTF-8 committed text, and IME composition events through `input_add`. Text packets
hold at most 255 bytes plus the terminator; oversize/unsupported events are
rejected explicitly. Native IME candidate-list events are not transported.
The host input-mode getter exposes text activation/area/cursor and relative-mouse
requests for the later viewport integration. The visible editor must own native
text input, IME placement, cursor capture, and focus routing.

The private protocol uses versioned, length-checked, little-endian packets over a
nonblocking IPv4 loopback socket. A random 128-bit token authenticates startup
before frame allocation; this isolates sessions, not code running under the same
user account. There is no third-party networking dependency. Each wire retains
one send packet and one receive packet, each capped at a 4096² RGBA frame plus
headers. The host additionally retains one displayed frame. Input/control queues
hold 256 packets; overflow fails and stops the session instead of dropping a
release event. A poll reads at most 8 MiB/64 packets, writes at most 8 MiB, and
reads at most 64 KiB of child logs. Logs retain the latest 16 KiB, with dropped-byte
counts and warning diagnostics. The child also has bounded control polling.

The session's latest error and bounded stdout/stderr tail remain available after
failure. `editor_scene_session_update` returns a `ROHR_SCENE_SESSION_REPORT_ID`
report owning evidence observed since its preceding call; failure diagnostics
are delivered once, and an idle session returns NOT_RUN. It does not claim child
simulation time. `scene_run` returns a `ROHR_SCENE_REPORT_ID` aggregate that merges
executed engine/gameplay/physics/audio/animation/graphics reports and final audio
shutdown once, retaining shared overflow and first-fatal semantics. Its elapsed
time covers the entire call, including paused time, callbacks, and cleanup.
Scene reports own their evidence after teardown and are available through the
usual `ROHR_SCENE_REPORT` and `ROHR_SCENE_SESSION_REPORT` selection flags.

## Hosting verification

`scene_runner_test` checks public/direct behavior, invalid arguments, existing
engine preservation, partial initialization cleanup, gameplay failure, and report
ownership after teardown. `editor_scene_session_test` launches `scene_child_test`
and checks transport, two composed screens and layered UI, forwarded interaction,
text/relative input requests, focus release, pause timing, startup failure,
malformed/version/token/length rejection, bounded logs, queue overflow, timeouts,
restarts, and graceful/forced cleanup. It measures 1080p throughput and host poll
cost with a software renderer and exercises backpressure. `audio_reports_test`
also verifies suspension of callback output and command execution until resume.

The host test accepts a generated scene executable plus any extra argument to
run a smaller launch/frame/pause/resume/stop check against an installed-SDK
project. These checks do not substitute for the real editor focus/IME and visual
review planned with the viewport tabs, or for native Windows runtime testing.
