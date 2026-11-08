# Scoped creation and the teaching example

Milestone 16 Goal 1 design contract, 2026-11-07. This document defines the target
behavior approved for the new milestone. Goal 2 implements optional external-link
omission and background-only unbound viewport screens. Goal 3 implements the
project instance API below. Goal 4 implements generated project descriptions and
the three-path example main. Goal 5 integrates the wider SDK/game workflow
verification recorded below. The current API is documented in [shared creation](shared_creation.md)
and [automatic viewports](automatic_viewports.md).

This contract supersedes the milestone 15 rules that require bindings for every
nonzero optional reference during individual creation and for authored camera
references during standalone viewport creation. It preserves immutable authored
descriptions, explicit ownership, shared creation machinery, and automatic
rendering. No physics algorithm, solver setting, timestep, global world isolation,
or camera layer filtering is added here.

## Creation scope

Automatically resolve valid authored references only within the current creation
attempt. Explicit runtime bindings may supply dependencies outside that scope.
Never select an existing instance by a global authored name, last-created object,
or a matching description found elsewhere in the engine.

| Operation | Created contents | Automatic connections |
| --- | --- | --- |
| `rohr_entity_create(&project.example.body)` | One member and its configured components; no separately authored children or siblings. | Only references to entities created in this attempt; optional references outside it remain unbound. |
| `rohr_object_create(&project.example)` | The selected hierarchy, including its nested objects and aggregates. | References anywhere within that hierarchy, including across nested children. |
| `rohr_viewport_create(&project.initial_viewport)` | Its authored screens, UI, layout, and owned display resources. | No scene objects or cameras are created or discovered; screens without explicit runtime cameras remain empty. |
| `rohr_project_create(&project)` | One project entity hierarchy, its authored viewports, input controllers/actions, and sound/music resources. | References across all created project objects and authored screen-to-camera connections within this new instance. |

Object creation of the project root remains an entity-hierarchy-only operation;
it does not become an alias for project creation. Keep individual and object
creation available without creating windows, viewports, or input controllers.
Project creation is composition of the existing creators, not a second spawner.

An optional authored reference may be omitted only when its valid target is
outside the selected creation scope and no explicit binding was supplied.
Malformed or dangling catalog identities, wrong kinds, invalid topology, cycles
in relationships that forbid them, duplicate bindings, and stale explicit handles
remain errors. An explicit invalid binding must never be treated as absent.
Existing explicit binding rules continue to reject attempts to replace a member
being created in the same attempt. Required dependencies excluded by category
selection still fail. Category selection must not manufacture placeholder bodies.

## Optional relationships and required structure

| Relationship | Outside scope without a binding | When created together or explicitly bound |
| --- | --- | --- |
| Gameplay target | No runtime target. | Resolve the chosen entity. |
| Transform driver | Do not install the driver lock or inherit its motion. | Apply authored lock flags and offsets. |
| Sprite/animation attachment | Create an independent visual entity with its authored appearance and initial pose. | Preserve existing attachment and safe visual-alias behavior. |
| Camera follow target | Create the camera at its authored initial pose without following an entity. | Apply authored follow settings to the resolved entity. |
| Lifetime owner | No implicit owner outside the scope. | The created hierarchy supplies ownership, or the caller supplies an explicit owner. |
| Viewport screen camera | Keep screen background and UI; no scene drawing. | Use the supplied live camera, or the new project instance's authored camera. |
| Attached physics anchor | A nonzero attachment requires its body; fail if unavailable. | Resolve the body and its anchor resource. |
| Joint endpoints | Required nonzero references must resolve; fail otherwise. | Preserve existing anchor/world semantics. |
| Soft-body nodes, beams, and areas | Require the valid soft-body owner and all required topology. | Resolve owned nodes, endpoints, and loops within that aggregate. |
| Force/torque sources | Require their owner, target, and other nonzero source references. | Preserve current initial/persistent source behavior. |

A missing optional link is a normal creation outcome, not a warning. Required
physics references never become silently inactive constraints or world anchors.
Authored zero references retain their existing explicit disconnected/world
meaning; this does not make every zero-filled physics description valid.

Lifetime **duration/deadline** is independent of lifetime **ownership**. Standalone
creation preserves configured deadlines, material, collision filters, appearance,
visibility, and other independent settings. It does not spawn an ancestor to
provide ownership. Nonrotating standalone-particle restrictions remain intact.

Preserve the current authored initial world-pose calculation, including authored
ancestor transforms and the explicit placement offset. Reading immutable parent
transforms does not bind to or spawn a live parent. An unbound visual or camera
must not jump to the origin or follow a previously spawned sibling. Apply follow
offsets only when installing the relevant relationship; do not apply them twice.
Owner selection continues to control lifetime independently of transforms.

Do not rewrite or clear references in the input catalog. Resolve an effective
creation state for the current attempt. Creating the same description alone,
then within an object, must leave the object creation's wiring intact.

Keep the existing entity/object creation options and explicit binding entry
points for this milestone. Replacing all `_with_options()` APIs is not necessary
to implement scope semantics and is not part of this contract.

## Viewport binding

Keep authored screen camera assignments in project persistence and generated
immutable data. The editor uses them for previews, and project creation uses them
for automatic wiring. Standalone viewport creation does not require or infer
their runtime instances.

The existing single `ViewportCreateDescription` supports explicit cameras through
its `.template`, `.cameras`, and `.camera_count` fields. Existing object-instance
bindings remain available. A screen-specific explicit camera takes precedence
over an object-instance binding. Duplicate/conflicting or unused binding keys
remain errors. Binding arrays are borrowed for the call only.

Unbound screens render their configured background, with no fallback camera,
scene discovery, missing-camera warning, or runtime placeholder graphic. UI still
draws normally. Later `rohr_screen_camera_set()` can bind, replace, or clear a
camera through the existing runtime API. A formerly valid bound camera that
becomes stale remains a diagnostic condition and must never attach a reused slot.

Project creation resolves every nonempty authored screen assignment using its own
new entity hierarchy and supplies those bindings to the same viewport creator.
An unresolved nonempty assignment at project scope fails project creation;
deliberately unassigned screens are valid. The engine does not spawn a second
copy of an object merely because a screen refers to its camera.

Runtime screen/UI extension, clipping, named layers, visibility, resource
ownership, and selected/batched presentation retain their existing semantics.
This work does not limit a runtime camera to entities belonging to its project.
Multiple project instances share the current world, renderer, clocks, physics,
input devices, and audio service; they are not isolated worlds.

## Project description and instance API

Introduce an engine-owned immutable `ProjectDescription` containing the common
object-root description, ordered viewport recipes, input controller/action recipes,
and named sound/music recipes. Use pointer/count pairs for variable authored
collections. Generated `project.h` continues to expose named fields such as
`project.example.body` and
`project.initial_viewport`; their data and the traversable project description
refer to the same immutable records. Do not duplicate mutable state or add
project-specific runtime constructors or callback tables for the new pathway.

The generated typed view must support `rohr_project_create(&project)` and retain
entity/object access through the existing named fields. Preserve root-only
`rohr_object_create(&project)` with a valid common-prefix layout or narrow typed
adapter. Any adapter evaluates arguments once and obeys C layout/aliasing rules.
Raw engine `ProjectDescription` inputs use the same implementation. Generator and
SDK must be rebuilt together if public description layout changes.

The target public API is:

| Call | Result and responsibility |
| --- | --- |
| `rohr_project_create(const ProjectDescription *description)` | `ProjectIdResult`: a generation-checked owner of one newly created project instance. May allocate and load assets; never advances or presents a frame. |
| `rohr_project_root_get(ProjectId instance)` | `EntityResult`: borrowed entity-hierarchy root; use existing `rohr_object_member_get()` for members. |
| `rohr_project_viewport_by_name_get(ProjectId instance, const char *name)` | `ViewportIdResult`: borrowed viewport selected by its authored name within this instance. |
| `rohr_project_viewports_get(ProjectId instance)` | `ViewportListResult`: copied bounded array of borrowed viewport IDs, in authored order, with a count. No allocation or hidden presentation. |
| `rohr_project_controller_by_name_get(ProjectId instance, const char *name)` | `InputControllerIdResult`: borrowed controller; existing action lookup takes this controller and the authored action name. |
| `rohr_project_sound_by_name_get(ProjectId instance, const char *name)` | `SoundResult`: borrowed sound player; use existing audio controls for playback and changes. |
| `rohr_project_music_by_name_get(ProjectId instance, const char *name)` | `MusicResult`: borrowed music resource; playback remains explicit. |
| `rohr_project_destroy(ProjectId instance)` | `EngineResult`: release this instance's owned resources in dependency order. |

Provide equivalent direct engine functions without the `rohr_` prefix. Use
`ROHR_PROJECT_INVALID` for the zero project ID and a named bounded project-instance
capacity. Size the returned viewport list using the existing viewport limit;
exhaustion is a reported error, not silent truncation. Reject invalid/null create
inputs and stale lookup/destroy handles. Destroying the zero project ID is an
idempotent success. Copying an ID or result never adds ownership.

No `project_show()` or automatic update loop is needed. The game obtains the
viewport list and passes its values/count to `rohr_graphics_viewports_show()`.
This preserves the existing owned `Report` contract and exactly one presentation
per selected batch. Getters and creation return typed results, not pipeline
reports. Project creation does not poll input, run physics, update animation,
start playback, or apply initial forces immediately.

Input controller/action descriptions contain authored names and settings without
live controller IDs. Creation fills runtime action owners internally. Each project
instance owns distinct controller/action state. Because controller names are
currently globally unique, use collision-checked, bounded internal runtime names
while retaining authored names for project-local lookup. Never reuse another
instance's controller or silently truncate names into collisions. Actions can
retain their existing controller-local names. Physical input still reaches enabled
controllers through the current input system; this adds no focus routing.

Engine, graphics, and other required services must already be started. Project
creation validates prerequisites before acquiring resources; an empty/headless
description need not start an unused graphics service. The application owns
service startup/shutdown and calls project destruction before service teardown.
After shutdown, old project handles cannot access new-generation resources.

### Global settings and audio

By user decision, apply authored engine/window settings explicitly once during
application startup. Keep `project_settings_apply()` available to the generated
example. `rohr_project_create()` and destruction neither apply nor restore global
gravity, timestep, solver, window, pacing, or audio-mixer settings. A second
project creation cannot reset the running game's global settings.

By user decision, project creation loads every authored sound and music resource,
with independent mutable player state and shared immutable sound data through the
existing audio registry. Copy volume, pan where applicable, loop, and playback-rate
settings; create players stopped. No autoplay or implicit selection of the active
music track is introduced. The existing one-active-music-stream limitation remains.

Projects containing audio require the caller to start the audio service before
creation. Empty audio catalogs do not require that service. Loading all resources
can decode WAV files, open Ogg streams, allocate, and fail on missing files or
existing sound/music capacity limits; these costs occur during creation only.
Any audio failure rolls back the entire project attempt. The example starts audio
when required, calls the existing audio update path when used, and destroys the
project before stopping the service. Standalone entity/object/viewport creation
does not load the project's unrelated audio. Generated audio recipes remain
available for applications choosing explicit resource creation.

## Ownership, failure, and access

The project instance owns the entity root, its created viewports, input
controllers/actions, and sound/music resources. Viewports own their recipe-created
display resources and borrow camera entities from the project hierarchy. Existing shared asset
registries retain immutable resources per established rules. Named graphics
layers remain globally shared resources and are not destroyed with one project.

Lookup results are borrowed handles, not new owners. Game code may mutate members,
extend viewports, or deliberately delete a member using existing APIs. Getters and
cleanup must validate generations and tolerate resources already removed by
member lifetime expiry or deliberate deletion. A reused slot never becomes owned
by the older project. Keep the project root and project-owned viewport/controller
owners under project destruction; normal callers must not independently destroy
those borrowed owners, including project-owned sound/music handles. Existing
visual aliases may identify the same body; deleting an alias deletes that body,
not only its visual.

Destroy project-owned viewports before the entity hierarchy and its cameras;
release controller-owned actions through controller teardown and stop/release the
project's sound/music resources before audio service shutdown. Respect UI/text/font
dependency order and queued-render asset references. Destroying one project must
not invalidate another instance or a separately owned resource. Externally mounted
screens/UI remain borrowed under the existing viewport rules. A new entity belongs
to the project only when explicitly attached to its lifetime hierarchy; a new
separately created viewport is not implicitly adopted by a project.

Project creation borrows input storage only for the call. Copy required names and
runtime mappings and acquire asset references; releasing a loaded description or
temporary arrays afterward must be safe. No long-lived pointer into a caller's
creation descriptor is allowed.

Validate descriptions and references, then create resources in dependency order,
and publish the project ID only after success. On failure, return the original
typed error, destroy partial viewports/controllers/entities/audio players, release
assets, and undo newly introduced unused layer registrations. Preserve previously live
instances and shared resources. No partial project escapes to the caller, and no
failed attempt leaves force effects or input actions active. Reuse existing
creator rollback rather than bypassing it with a separate construction path.

## Generated teaching example

The initial `main.c` shows three complete alternatives separated by conspicuous
comment dividers. Each section contains a complete alternative `main()` so setup,
the loop, and cleanup can be selected together:

1. **Whole project — active by default.** Create through
   `rohr_project_create(&project)`, retrieve the viewport batch, use the common
   game loop, and destroy the project instance. Authored cameras are wired by
   creation. Show a brief member lookup where it teaches gameplay access.
2. **Whole object — commented out.** Create the authored object, obtain its camera
   member, create the viewport with an explicit camera binding, and destroy the
   viewport before the object. Demonstrate that standalone viewport creation can
   precede a later screen-camera binding without the discovery helper.
3. **Individual entities — commented out.** Create a small representative selection
   of independent authored entities and a camera, bind a screen explicitly, and
   destroy owned resources in order. Explain that optional links are absent when
   their targets were not created together. Do not present a beam without its
   required topology as a valid standalone example.

The object and entity sections must be actual commented C, not just prose,
preprocessor-disabled code, or snippets requiring invented variables. Use line
comments around alternatives to avoid nested block-comment hazards. Give a short
instruction to disable path 1 and uncomment exactly one alternative. Arrange
startup/setup and cleanup so selecting a path is mechanical; do not require
editing multiple scattered blocks or leave duplicate declarations after a switch.
Keep shared input/event polling, engine/physics updates, animation updates,
presentation, and shutdown visible. Check every creation/getter result before use.
Do not conceal initialization in generated bespoke callbacks.

Each path must run independently. They need not instantiate identical content:
the individual path intentionally teaches partial creation. The active project
path shows the authored scene; the previous extra direct-API demonstration bodies
must not be silently added alongside it. Avoid a long low-level topology tutorial
in the initial main. Detailed physics examples remain available separately.

Regeneration replaces editor-owned generated modules. Preserve developer-edited
`main.c` and build files; migrate only strictly recognized untouched example/scene
templates. Keep scene simulation's selection and host lifecycle intact; it may
compose a selected project description rather than spawning excluded objects.
Keep persisted authored camera choices for previews and project creation. No
`project_viewports.h/.c` compatibility layer is reintroduced.

## Implementation boundaries and acceptance

Goal 1 changes documentation only. Source review identifies existing dependency
resolution in [object_spawn.c](../src/entity/object_spawn.c), binding validation in
[viewport_creation.c](../src/graphics/viewport_creation.c), typed catalog generation
in [editor_object_catalog.c](../tools/editor_core/src/editor_object_catalog.c),
controller/settings generation in
[editor_workspace.c](../tools/editor/src/editor_workspace.c), and example setup in
[editor_example_main.h](../tools/editor/src/editor_example_main.h).

Each implementation goal verifies its own changes before stopping. Goal 5
integrates the completed paths; it does not defer focused checks until the end.

- Verify optional-reference omission, explicit binding, full-object resolution,
  cross-object project wiring, and unchanged immutable templates across repeated
  creations. Cover malformed/required references, excluded categories, and cycle
  validation. Check initial pose, visibility, particle restrictions, and deadlines.
- Verify unbound screens show backgrounds and UI only, then bind/clear/rebind them.
  Cover stale cameras separately, two differently placed/rotated/clipped screens,
  named-layer identity, runtime extension, and one presentation per batch.
- Create two instances with matching authored names. Check independent entity,
  viewport, controller, audio-player, and UI state, project-local lookup, unchanged
  global settings, and safe destruction while the other instance remains alive. Do not
  claim isolated simulation worlds or project-specific camera filtering.
- Verify sound/music loading, initially stopped playback, authored audio settings,
  missing-file rollback, capacity exhaustion, shared sound-data ownership, named
  lookup, and destruction while another instance's audio remains valid. Confirm
  explicit audio updates and unchanged single-active-music behavior.
- Inject allocation/resource failures at construction stages. Verify original
  errors, no partially published handles or surviving effects, reusable capacity,
  asset reference cleanup, and unaffected previously live instances.
- Run 1,000 create/use/extend/destroy cycles under ASan/UBSan with live-resource
  accounting. Exercise stale IDs, slot reuse, member expiry, and service shutdown.
- Generate a fresh example and mechanically activate each of its three sections
  in disposable copies. Compile and run each against installed SDKs; a commented
  alternative that is never compiled is not verified. Check deterministic
  generation, persisted camera assignments, and preservation of custom main/build
  files. Keep existing examples building through the shared creators.
- Verify Nix and generic Linux SDK consumers, launches outside source directories,
  Windows cross-builds, and a real Linux preview accepted by the user. Native
  Windows runtime execution remains outside the platform acceptance gate.
- Keep construction work out of per-frame paths; retain existing frame-pacing and
  physics defaults. Use focused behavior checks and existing workload smoke checks
  for regressions, rather than reopening the paused physics performance goals.

The ordered goals and status are authoritative in
[Milestone 16](../NEXT_STEPS.md#16-scoped-creation-and-a-teaching-example).


## Project instance implementation

The public types and direct APIs are in
[project_instance.h](../include/project_instance.h); `rohr.h` exposes equivalent
`rohr_project_*` names. `ProjectDescription` begins with an `ObjectDescription`
root, followed by pointer/count pairs for `ProjectViewportDescription`,
`ProjectControllerDescription`, `ProjectSoundDescription`, and
`ProjectMusicDescription`. Each input controller has a copied controller
configuration and an array of `ProjectActionDescription` values, which contain
no runtime controller handle. Viewport entries pair a project-local name with a
plain immutable viewport recipe; runtime bindings and template chains are not
accepted in project recipes. Audio entries pair a project-local name with the
existing sound/music descriptions. Names are bounded by the existing entity or
input name limits and unique within each resource list.

The root must be a parentless container and is the same initial member used by
object-only creation. Authored children refer to `&description.root` (or their
actual nested parent). The typed-view adapter requires a real initial
`ProjectDescription` member; it is not valid to cast an older generated object
catalog to a project description. Goal 4 generates this layout in the root view's initial union, with `_project`
as the active `ProjectDescription` member. The common initial `ObjectDescription`
is also available as `_description`. Child parent pointers refer to the actual
`_project.root`; child views and named viewport recipes are reused by its lists.
The existing `project.viewports` recipe iteration remains available for custom
object-only applications. Root metadata is read through `project._description`
or `project._project.root`; child shorthand fields are unchanged.

Creation allocates a bounded ownership record and invokes the existing creators.
It resolves camera-object paths only in the new root hierarchy and publishes the
project ID only after every resource succeeds. `ROHR_PROJECT_LIMIT` is 16; other
resource counts use their existing engine-wide limits. Project IDs are monotonic
64-bit values that are never reset or wrapped. Input runtime names use checked
internal names with bounded collision retries; authored names remain available
through project-local lookup. Input generations now survive engine restart, and
exhausted input slots retire rather than reusing old handles.

Creation and destruction run on the application thread between system updates;
there is no project update loop or per-frame allocation. The audio service keeps
its existing synchronization and deferred playback-control semantics. Creation
loads audio stopped without calling audio update or changing the active track.
Failure destroys acquired resources and removes newly introduced layer names;
successful project destruction preserves globally shared layer registrations.

Destroy projects before service teardown. Engine shutdown also destroys any
remaining project owners before stopping audio/input and clearing entity tables.
If an independently stopped service has already invalidated a borrowed resource,
lookup fails and project destruction skips that stale handle. Resources created
after a service restart or slot reuse are not adopted by an older project. Normal
callers must still leave project-owned root, viewport, input, and audio owners to
project destruction. Entity members and runtime viewport extensions retain their
existing mutation and ownership rules.

## Goal 2 verification

The implementation preserves required physics dependencies while allowing valid
optional references outside the selected creation scope to remain unbound.
Standalone viewport recipes retain authored camera metadata and create empty
screens; runtime camera binding and clearing invalidate cached screen contents.
Previously bound cameras that become stale still produce diagnostics. No physics
solver, timing setting, project instance API, or generated example path changes
in this goal.

- Linux Release: all 23 focused engine/editor tests, five selected installed-SDK
  consumers, and three generated-project integration checks pass.
- ASan/UBSan with leak detection: the same 23 focused tests and five installed-SDK
  checks pass, including 1,000-cycle creation/destruction and failure rollback.
  Tests cover independent poses/deadlines, immutable catalogs, explicit bindings,
  required dependencies, stale cameras, empty-screen composition, and paused
  camera rebinding.
- All 14 installed-SDK consumer targets and four independent examples
  (`center_of_mass`, `player_controller`, `soft-body`, and `view-port`) build on
  Linux Release, Linux sanitizer, and Windows cross configurations. Windows
  engine/editor and affected tests cross-build; native Windows execution is
  unverified. These are isolated Nix-built SDK installs; generic Linux archive
  packaging and the new whole-project workflow remain Goal 5 verification.
- All four independent examples launch from `/tmp` in Release and ASan/UBSan.
  The interactive examples pass startup/shutdown smoke checks. Both headless car
  runs complete 3,600 frames and their declared 60-second simulation endpoint,
  with matching 14,400 substeps, 7,222,164 candidates, and 1,195,541 contacts.
  These are completion/regression checks, not rendered performance acceptance.
- The user accepted the real installed-SDK viewport preview cycling between
  empty screens, both cameras bound, both cleared, and only the inset bound,
  with backgrounds and UI preserved and no trails or warning stripes.

Evidence and reproducible verification scripts are in
`sandbox/milestone_16/goal_2/`. No compiler warnings or sanitizer findings remain.
Existing HarfBuzz pkg-config messages about unavailable `glib-2.0.pc` reproduce
in the prior milestone's environment; CMake finds the installed HarfBuzz library
and all configurations/builds succeed. The initial sanitizer harness requested
a disabled CLI target; enabling that target resolves the setup failure. Generated
configuration coverage now expects successful unbound viewport rendering rather
than the superseded missing-binding error. The user's `dist/` installation is
unchanged. Existing physics failure inventories and rendered performance gates
are not reopened or claimed fixed.


## Goal 3 verification

- Linux Release and ASan/UBSan: all 19 focused engine tests and five selected
  installed-SDK consumers pass. The final project test also passes after adding
  root-expiry coverage and requiring nonzero camera/drawing work. Leak detection
  is enabled in sanitizer runs; no sanitizer findings remain.
- The new project test covers two instances with the same authored names,
  cross-object camera/transform wiring, copied names, independent input and visual
  state, shared immutable sound data, stopped audio with authored settings,
  unchanged global settings, missing files, malformed references, resource limits,
  and recovery after failure. All 24 injected allocation failures roll back
  without changing a survivor's entities, input, audio, or text/font ownership.
- Each project test completes 1,000 create/extend/destroy cycles beside a surviving
  project. Root expiry, member deletion, slot reuse, stale lookups, and engine
  restart are covered. Project IDs and borrowed input handles do not revive after
  restart. The normal `rohr_tests` build target includes the new test.
- The engine, editor/CLI, affected tests, five SDK consumers, and four independent
  examples cross-build for Windows. Native Windows execution remains unverified.
  SDK installs are isolated from the user's `dist/`; generic Linux packaging and
  the generated whole-project example remain later milestone work.
- All four independent examples launch from `/tmp` in Release and sanitizer
  builds. Both car runs reach their full 3,600-frame / 60-second simulation
  endpoint with 14,400 substeps, 7,222,164 candidates, and 1,195,541 contacts,
  matching Goal 2. These are correctness/completion checks; the user was running
  another example, and timings are not controlled performance evidence.
- The user accepts the corrected installed-SDK two-project preview: two rows of
  views with rotated insets, moving particles, and independent orange/blue body
  visibility. The initial black preview came from disabled viewport defaults in
  the test fixture, not a project-creation failure. The fixture now enables its
  viewports and checks actual drawing work as well as report success.

Evidence and reproducible scripts are under `sandbox/milestone_16/goal_3/`.
The project test uses unique audio fixture filenames so parallel suites cannot
interfere. A disk-full Nix invocation was retried after removing only Goal 2's
disposable sanitizer consumer/example build directory; its logs and SDK remain.
The initial test build's incorrect default-description function name is corrected;
final builds have no compiler warnings. Existing HarfBuzz pkg-config fallback
messages remain as documented for Goal 2. Physics settings, rendered performance
requirements, and known physics failures are unchanged. Goal 4 still owns the
new generated project layout and teaching example; those are not implemented here.


## Goal 4 verification

Goal 4 is complete for review, 2026-11-07. New projects expose `project.example`,
use `example_frame_*.png` from the SDK's `example_assets` directory, and generate
an example main with three complete creation alternatives. Whole-project creation
is active; object and entity alternatives are line-commented. Startup applies
authored settings separately, starts audio when the project needs it, and checks
system reports in the shared loop. Each path destroys its owned resources before
stopping shared services. No extra direct-API tutorial bodies are added to the
active authored scene.

The generated root view starts with a union containing the actual immutable
`ProjectDescription`. Its root, named children, and viewport entries reference the
same records. The `_description` common-initial-member alias preserves object-root
access, and the existing viewport recipe iteration remains available. Use
`project._description` or `project._project.root` for root metadata; shorthand
metadata on named children remains unchanged. Per-project input/action arrays and
sound/music descriptions contain no mutable runtime handles. Viewport lookup names
use the same canonical spelling as editor persistence, avoiding differences
between generation before and after reload.

Only byte-exact previous main templates migrate to the generic project path.
Edited mains/build files and existing authored names remain unchanged. The exact
old template text is retained solely for migration recognition. The center-of-mass,
player-controller, and soft-body examples were regenerated with their custom
entry points intact. Simulation still composes the selected hierarchy and
viewport through its existing host lifecycle.

Verification evidence is in `sandbox/milestone_16/goal_4/logs/`:

- Fourteen focused Release checks pass across `release_tests.log`,
  `release_tests_final.log`, and `fixture_tests.log`. Coverage includes all three
  mechanically selected example mains, deterministic generation across reload,
  strict migration and near-match preservation, scene selection, partial spawning,
  build relocation/scaffolding, custom paths, and default particle/soft-body/beam
  fixtures updated for the Example name. Initial setup/fixture failures are
  superseded by the final passing logs.
- Six focused ASan/UBSan checks pass with leak detection enabled in
  `sanitizer_tests_final.log`. These compile and run all three main alternatives
  and both generated parity consumers against the installed sanitizer SDK.
  The project consumer verifies two independent projects, camera assignments,
  UI/layers, input defaults and disabled controllers, audio loading without
  playback, mutation isolation, and destruction with a surviving instance.
  Existing object coverage retains 1,000 create/destroy cycles.
- Four independent SDK examples build and launch from `/tmp` in both Release and
  sanitizer configurations: center of mass, player controller, soft-body car,
  and viewports. Both car runs reach all 3,600 frames and 60 simulated seconds,
  with 14,400 substeps, 7,222,164 candidates, and 1,195,541 contacts. No physics
  settings or performance gates change; concurrent user activity and sanitizer
  overhead mean elapsed timings are not performance evidence.
- Engine/editor, affected tests, four independent examples, all three main paths,
  and both generated configuration/project consumers cross-build for Windows.
  See `windows_*.log` and `platform_sequence.log`. Native Windows execution is
  unverified; generic Linux archive and wider workflow acceptance remain Goal 5.
- The unmodified generated example main was compiled against the installed SDK
  and shown for 60 seconds. The user accepted the authored scene, animation,
  and viewport UI. See `preview_build.log` and `preview_run.log`.

No new compiler warnings or sanitizer findings remain. The sanitizer setup needed
its SDK integration tests enabled and the CLI mock helper built; LeakSanitizer
also required execution outside the tracing sandbox. These were verification
setup corrections, not runtime changes. To recover disk space, disposable Goal 2
Release/Windows builds and Goal 3 consumer builds were removed; their logs and
installed SDKs remain. SDK/example builds were reused in place for this goal,
with current verification logs kept separately. Stop here for review and commit;
Goal 5 requires a new user instruction.

## Goal 5 verification

Goal 5 and milestone 16 are complete for review, 2026-11-07.

The installed-SDK workflow now exercises 1,000 object and 1,000 whole-project
create/extend/use/destroy cycles, with standalone entities in every cycle. Each
path creates two instances, mutates one, adds runtime screens and UI, and checks
that the surviving viewport still renders after the first owner is destroyed.
Caller-owned screens and standalone entities survive project destruction; borrowed
mounts are removed before their owning screen is destroyed. Project handles are
destroyed through their project owner rather than separately deleting resources
borrowed from it.

Installed Linux project-creation tests now exercise allocation failure injection.
The engine's 1,000-cycle project test also checks entity, texture, text, and font
counts after every teardown, alongside existing audio counts and stale-handle
checks. Nix SDK packaging now executes its installed consumers instead of only
compiling them. The [SDK workflow](building.md#make-a-game-with-the-sdk) explains
the three complete example mains, explicit settings, borrowed ownership, and
shared-world limits. No engine runtime, physics, or frame-pacing code changes.

Evidence is retained in `sandbox/milestone_16/goal_5/logs/`:

- Five focused engine checks pass in Release and ASan/UBSan: input actions,
  viewport descriptions, automatic viewports, project creation, and descriptor
  creation. Installed consumers cover the audio/input ownership and prerequisite
  failures as well as scope, resources, rendering, and rollback.
- All 15 installed consumers also pass with both engine and consumer code
  instrumented by ASan/UBSan, leak detection enabled, and undefined behavior set
  to halt. The project test rejects 24 injected allocation failures without
  damaging the surviving instance. The final run supersedes the initial run
  that instrumented only the engine. No compiler warnings or sanitizer findings
  remain. LeakSanitizer checks run outside the tracing sandbox.
- Fresh Nix and generic Linux packages pass all 15 installed-SDK consumers. The
  Nix derivation runs these during `installCheckPhase`; the generic archive is
  built and tested in Rocky Linux 8. All 148 regular archive files match the
  verified installed prefix; its SHA-256 is in `archive_parity.json`.
- Both packaged CLIs generate a fresh example whose project, object, and entity
  alternatives compile and execute 120-frame functional runs outside the source
  directory. Regeneration preserves the main and custom CMake file. The generic
  packaged CLI runs in the Linux container because NixOS does not directly run
  its generic ELF interpreter; host-compiled generic SDK consumers also pass.
- Four independent examples compile against each fresh SDK and launch from
  `/tmp`: center of mass, player controller, soft-body car, and viewports. Both
  car runs reach 3,600 frames and 60 simulated seconds, retaining 14,400 substeps,
  7,222,164 candidates, and 1,195,541 contacts. These functional runs make no
  performance claims and do not replace existing physics gates or failure records.
- All 15 updated consumer targets and the internal project-creation test
  cross-build for Windows. Goal 4's engine/editor, generated-main, and independent
  example cross-build evidence remains applicable; native Windows execution is
  still unverified. ARM Linux is not exercised by this x86-64 verification.
- The user accepts the real Linux whole-project SDK preview: two views, rotated
  insets, runtime UI, and visibility mutation confined to the selected instance.
  The preview launches from `/tmp` and exits successfully after teardown.

Disposable older build directories were removed to recover disk space after
preserving their logs. Existing `dist/` outputs and normal example builds were
left intact. New SDK artifacts are verification outputs under this goal's sandbox;
use the documented SDK build commands to refresh the normal distribution.
Stop here for review and commit. Agree the C game concept and completion criteria
with the user next; game implementation and deferred physics work do not start
automatically.
