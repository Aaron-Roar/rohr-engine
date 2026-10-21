# Application and scene entry points

Projects separate developer-owned application code from disposable simulation output:

- `src/main.c` demonstrates creating starter bodies through `objects.starter.*`,
  spawning a whole soft body, and explicitly connecting separately spawned joints.
  Attached visuals and anchors are created with their bodies. It also spawns
  `objects.tug` as a complete instance: `box_1`, `box_2`, `tug_spring`, and two
  opposing persistent forces.
- `src/generated/project_scene.c` creates the selected objects and one authored
  viewport, then runs the scene runner's engine, physics, audio, animation, and
  presentation loop. Every generation overwrites it. Its private generated object
  and viewport modules live in `src/generated/scene/`; the application catalog
  remains complete and independent.

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
below. **Run > Simulate Scene** regenerates and builds the current editor
selection, then opens the **Scene Simulation** viewport tab stopped. The CLI
workflow below builds and launches a standalone window.

## Editor simulation tab

Both **Editor** and **Scene Simulation** tabs appear above the viewport when a
project opens, initially with Editor active. Drag the compact tab headers to
reorder them. Each tab's **x** and its checked **View** menu entry control the
same visibility state. Re-enabling a tab selects it. Closing the active tab
selects the remaining visible tab; closing both shows an empty-viewport hint.
Visibility and order survive project transitions within a session, but reset
when the editor restarts. Opening a project prefers Editor when it is visible;
the strip is hidden without a project.

**Run** in
the simulation tab regenerates and builds the current in-memory scene before
launching it in an isolated child process. **Pause** freezes that instance;
**Resume** continues it without rebuilding. **Stop**, or selecting **Editor**,
stops the child, releases its input, and clears the image. The simulation tab
can be hidden through its close button or View entry, which also stops the child
and clears its image. Reopening a hidden simulation tab leaves it stopped.
**Run > Simulate Scene** reveals and selects the tab. Stop, Editor selection, and
hiding the simulation tab also cancel a pending simulation build; they do not
cancel an unrelated application build.

The properties column remains editable during simulation. Changes take effect
on the next fresh Run. Scene/project builds stop the current simulation before
replacing its executable. Failed, cancelled, or disabled builds never launch
previous output automatically.

The child supplies its fully composed logical frame, fitted inside the viewport
without stretching. Launch, Resume, and clicking the image focus scene input.
Clicking another editor control, opening a modal, losing window focus, or pressing
**Escape** releases it while simulation continues. Escape provides an exit from
relative mouse capture. Keyboard shortcuts, pointer input, text, and IME
composition reach the scene only while focused; pointer releases remain captured
outside the image after a press inside. The editor owns native relative mouse and
text-input mode and maps the child's IME rectangle into window coordinates.

Combined stdout/stderr streams into the terminal, including cleanup output,
even while the terminal is hidden. Partial UTF-8 codepoints survive
pipe-read boundaries; invalid bytes display as `?`. Plain log text does not create
warning/error notifications. Structured report diagnostics create notifications
with report/stage IDs, severity, code, recovery, source/target IDs, substeps,
details, truncation, and occurrence counts. Identical issues within one run update
the retained entry without restarting its toast. Grouping holds 100 identities,
reports omitted occurrences, and retains the first fatal when full. Counts
saturate explicitly. The notification log and terminal scrollback remain bounded.

## Editor scene settings

Open **Build > Scene Settings** for an object checklist, **All**/**None** buttons,
and one viewport dropdown with a **None** option. Changes apply immediately,
participate in undo/redo, and are saved with the project. All/None affects current
objects; newly created or duplicated objects start included. These choices are
independent of the editor eye and Visible in App.

Older projects start with all objects included and their first viewport selected.
Deleting the selected viewport clears the choice; undo restores it. Choosing None
is valid and remains None when viewports are added. Simulation can run without
viewport visuals, with no special warning or substitute camera.

Every editor generation uses these settings for simulation output, including
Build Scene and Build Project, while the application catalog stays complete.
Simulate Scene uses current in-memory changes without requiring a save. It stops
an existing simulation before generation and compilation, and prepares output
only after successful compilation. Failure, cancellation, disabled compilation,
and project transitions cannot prepare an older executable. Build diagnostics
use the existing terminal and notifications, as do simulation output and
structured report diagnostics.

## CLI simulation selection

```sh
rohr-cli --project ./game build-scene
rohr-cli --project ./game --object tug --viewport simulation_viewport simulate-scene
rohr-cli --project ./game --object-id 2 --viewport-id 1 build-scene
```

Both scene operations regenerate disposable sources, configure, and compile only
`${PROJECT_NAME}_scene`. `simulate-scene` then launches the standalone scene and
waits for it to exit. Interrupts reach the owned build or simulation process tree.
Generation/build failures stop the request without launching older output; a
disabled compile command is an error for these operations.

Without an object selector all objects are included. One `--object` or
`--object-id` includes only that object. Without a viewport selector the first
viewport in stored project order is used; `--viewport` or `--viewport-id` selects
exactly one. Names follow the existing object naming convention, with exact
viewport names also accepted. Unknown, ambiguous, repeated, or invalid selectors
fail before generation. CLI defaults ignore the saved editor selections. CLI requests neither save selections nor alter authored
project data, application `main.c`, or the complete application catalog.

A selected viewport retains its authored placement, clipping, UI, and visibility.
Scene selection omits screens owned by explicitly excluded objects.
No object is automatically included and no replacement camera is created. A
project with no viewport has no viewport visuals. The generation API also allows
an explicit empty object list.

Generation rejects screen references to objects or cameras missing from the
authored project before writing generated files. Diagnostics identify the
viewport, screen, and missing reference. This differs from choosing not to spawn
an existing authored object in developer-owned `main.c`: generated layer setup
and drawing skip zero entity handles, while nonzero stale entities remain errors.

A screen whose camera is unavailable at runtime stays in its viewport and shows
the editor-style dark construction-stripe pattern. Placement, rotation, layers,
visibility, and viewport clipping still apply; stale camera imagery is never
displayed. Composition reports one warning per visible screen per invocation,
including the screen and camera IDs, while other screens and UI continue.
Camera destruction preserves screens and their references. Creating another
camera does not rebind them automatically; assign it explicitly. For generated
presentation, recreating presentation after spawning the missing object also
installs its generated rendering callback.

`editor_workspace_scene_generate` takes a borrowed `EditorSceneSelection`.
Zero initialization means all objects/first viewport; `objects_selected=true`
uses the supplied stable-ID list, including an empty list. A nonzero viewport ID
selects that viewport; `viewport_selected=true` with ID zero selects none.
`editor_workspace_editor_generate` uses saved editor choices without modifying
them. Selection validation precedes file writes. The generator
uses a temporary filtered view of authored data and the existing catalog/viewport
generators; it does not modify the project or change physics pipeline behavior.

## Build and Run menus

| Action | Generates C | Build target |
| --- | --- | --- |
| Build Application | Yes | Application only |
| Build Scene | Yes | Scene only |
| Build Project | Yes | Application and scene |
| Generate App C | Yes | None |
| Compile App | No | Application only |
| Simulate Scene | Yes | Scene only, then prepare stopped |

Build actions configure before compiling. **Run Application** starts the existing
application without generation or compilation; missing output produces a build
notification. **Simulate Scene** always regenerates, configures, and compiles
before preparing the resulting scene stopped.

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

Normal editor and CLI builds reuse a valid CMake cache. Moving or copying a built
project is detected from CMake's recorded source/build directories. Before
reconfiguration, the tools set aside `CMakeCache.txt`, `CMakeFiles`, and generated
target/executable records under `build/rohr_relocated_cache_*`; unrelated build
files, source, assets, and developer configuration remain in place. These
archives are not reused automatically. Saved project/editor configuration is
reapplied, but options stored only in the old cache are not migrated. Use
`{project}` and `{build}` in saved commands for project-local paths; explicit
external engine/SDK paths remain external dependencies.

Run refuses stale metadata even if the original project still exists. An
interrupted or failed relocation recovery stays blocked until configuration
creates a valid cache for the new location. If configuration is disabled, the
tools explain that it must be enabled before recovery. A linked default `build`
directory is rejected rather than modifying another project's files.

Before configuration, all editor and CLI builds restore a missing
`CMakeLists.txt` or `src/generated/project_scene.cmake` from the engine templates,
including builds with custom commands. Existing CMake files are preserved;
unreadable files and invalid path types produce diagnostics rather than being
replaced. A deleted customized file can only be restored as the standard
template. Recovery leaves `.gitignore`, developer source, assets, and settings
alone. Compile App and CLI `compile` do not regenerate C sources.

The configured CMake command regenerates missing caches and build metadata;
valid caches remain reusable. An explicitly disabled configure command stays
disabled. Default-directory CMake builds with missing metadata explain that
configuration must be enabled; custom builders remain responsible for their own
metadata and any alternate build directory. Standalone Generate C does not
restore a missing root CMake file, preserving externally managed workflows.

Generation, configuration, compilation, and launch-preparation diagnostics always
reach the embedded terminal, including failures before a subprocess starts.
Stage messages identify starts, completion, failures, and cancellation; subprocess
stdout/stderr is retained in the terminal's bounded scrollback. The optional
generated-object tree is independent of these diagnostics. Terminal visibility
and focus change only through user interaction, including when a build fails;
failure notifications direct users to the terminal. Ctrl+C in the focused
terminal cancels an active build; a console interrupt is forwarded before the
editor or CLI exits. POSIX builds own a process group, with a two-second interrupt
grace period before forced termination. Windows builds own a kill-on-close job;
a GUI process without a console terminates the job directly. Child processes are
reaped before their owner is destroyed.

Custom GUI compile commands must include `{target}`, for example
`{"cmake", "--build", "{build}", "--target", "{target}"}`. A plain CMake build
command without an explicit target automatically receives the selected target.
Other commands and fixed target overrides are rejected instead of ignoring the
menu selection. The same target requirement applies to CLI scene compilation.
`{target}` names the scene target for scene operations and the project aggregate
for the existing CLI build/compile operations.
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
settings, binding helpers, and viewport composition. The
`project_scene.cmake` integration file is created when missing and preserved
when present, including during generation.
The simulation entry point and its private modules are always overwritten.
Missing generated files are recreated. Exact legacy `src/scene.c` scaffolds are
removed; customized legacy files remain untouched but are unused by the scene
target, with generation output explaining the change. Application `main.c`
remains developer-owned after creation.

New tutorial content is added only when creating a new project. Existing scenes
are not given a `Tug` object automatically. The tutorial main is an editable
example, not a synchronized inventory: after renaming or removing its referenced
objects, update its explicit calls. The generic scene entry point continues to
follow the requested simulation selection when C is regenerated. Editor
generation uses Scene Settings; CLI generation defaults to all objects and the
first viewport, independently of those saved settings.

An exact match of the preceding generated CMake scaffold receives this line:

```cmake
include(src/generated/project_scene.cmake)
```

Customized CMake files are preserved. Generation logs the integration step when
that include is absent. Define `ROHR_ENGINE_TARGET` and `ROHR_GENERATED_SOURCES`
before including the module, as in the default scaffold. Projects with an
external build system can compile `src/generated/project_scene.c` together with
`src/generated/scene/project_objects.c` and `project_viewports.c`, include the
private scene directory, and link the engine. The generated CMake module excludes
`project_scene.c` from application sources collected by older wildcard scaffolds. Only an exact recognized old tick-loop template
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

`editor_project_test` covers scaffold creation, source preservation, regeneration
of missing scene output, exact legacy removal, customized-build preservation, and
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
pass their original `argc` and `argv` to the runner. Generated simulations already
use this runner. A custom application can also use it by moving its creation and
destruction into callbacks and replacing its loop with `rohr_scene_run`. A custom
executable without the runner times out during hosted startup.

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
requests used by the simulation panel. The visible editor owns native
text input, IME placement, cursor capture, and focus routing.

The private protocol uses versioned, length-checked, little-endian packets over a
nonblocking IPv4 loopback socket. A random 128-bit token authenticates startup
before frame allocation; this isolates sessions, not code running under the same
user account. There is no third-party networking dependency. Each wire retains
one send packet and one receive packet, each capped at a 4096² RGBA frame plus
headers. The host additionally retains one displayed frame. Input/control queues
hold 256 packets; overflow fails and stops the session instead of dropping a
release event. A poll reads at most 8 MiB/64 packets, writes at most 8 MiB, and
reads at most 64 KiB of child logs. Logs retain the latest 16 KiB, with discarded-tail
byte counts. Without an output sink, discarded bytes also produce warnings.
`output_set` installs a borrowed byte sink receiving every read once, so tail
rotation does not imply lost delivery. `report_output_set` observes each update's
borrowed report, including updates during destruction. Keep sink contexts alive
through destruction. Both sinks run synchronously on the calling thread.

Protocol version 2 adds structured diagnostic packets and an acknowledged orderly
exit. The child drains in-flight controls until the host confirms receipt of the
final marker, preventing a socket reset from discarding unread diagnostics.
The child keeps a bounded pending report, sends first-fatal evidence ahead
of ordinary diagnostics, reports overflow, and drains pending evidence during
cleanup. It streams each executed child report once, rather than replaying its
cumulative aggregate. The host drains socket/log tails before reaping an exited
child. Forced destruction has a final bounded drain and reports any omitted
tail. The child also has bounded control polling.

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
restarts, exact incremental log delivery, live/repeated diagnostics, fatal
preservation under overflow, and graceful/forced cleanup. It measures 1080p throughput and host poll
cost with a software renderer and exercises backpressure. `audio_reports_test`
also verifies suspension of callback output and command execution until resume.

The host test accepts a generated scene executable plus any extra argument to
run a smaller launch/frame/pause/resume/stop check against an installed-SDK
project. `editor_scene_panel_test` covers tab/control actions, isolated focused
input, outside-image releases, modal focus loss, pause/resume, image clearing,
grouped notifications, UTF-8 boundaries, and queued image-resource lifetime.
These checks do not substitute for real editor focus/IME and visual review or
native Windows runtime testing.

`editor_scene_generation_test` and the CLI scene-selection test cover defaults,
explicit selections, invalid requests, application independence, and unchanged
authored data. `editor_scene_settings_test` covers panel input, scrolling, modal
capture, shared history, persistence, and viewport deletion. Build controller
checks reject preparation after active, failed, cancelled, and skipped builds.
`tools/editor_core/tests/generated_scene_selection` compiles the
actual generated entry point against the installed SDK and verifies object,
viewport, screen, and UI counts, rendering/physics updates, and complete teardown.
Its probe accepts expected object, viewport, screen, and UI counts as four arguments.
