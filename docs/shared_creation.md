# Shared creation APIs

Milestone 15 Goal 2 implements the entity/object portion of the
[creation contract](creation_and_viewport_contract.md). Goal 3 adds
[automatic viewports](automatic_viewports.md), and Goal 4 adds the unified
generated `project.h`.

Milestone 16 Goal 2 implements the [scoped creation rules](scoped_creation_contract.md)
for optional external links and unbound viewport screens. Goal 3 adds
`rohr_project_create()` and project ownership. Goal 4 adds generated project
recipes and the three-path example main.

Milestone 17 Goal 2 changes entity, object, and project creation to typed value
syntax. Rebuild the SDK and regenerate catalogs together; older generated headers
do not contain the required adapters. Existing developer-edited mains are preserved
and need the source migration below. Goal 3 adds named resource recipes and the
shared controller creator. Goal 4 extends the normal graphics resource creators
to authored recipes and migrates viewport creation to value syntax.

## Creation values and source migration

Pass a generated field or an engine descriptor directly: `rohr_entity_create(body)`,
`rohr_object_create(project.example)`, or `rohr_project_create(project)`. The direct
engine names behave identically. Both `_with_options` creators also take a value
as their first argument. Replace old `create(&value)` calls with `create(value)`;
when holding a valid descriptor pointer, explicitly use `create(*pointer)`.
Check nullable pointers before dereferencing them.

C11 dispatch accepts only the appropriate descriptor or a registered generated
view. Pointer arguments, unrelated structs (even with an identical first member),
and non-project fields passed to project creation are compile errors. The old
pointer functions and creator function-pointer spellings are removed. Applications
needing a function pointer can define an explicitly typed wrapper around a value
call. Do not call the `rohr_internal_*` implementation helpers directly.

Arguments and creation options are evaluated once. Hierarchy recipes are borrowed
at their original addresses: keep generated catalogs as named immutable values,
and do not copy them to rebind a hierarchy. Their parent/child and external-binding
references must point to the same canonical records. A parentless plain descriptor
may be a C compound literal; double-parenthesize compound literals when passing
them alongside options to protect initializer commas from macro argument parsing.
Only the returned runtime resources are owned by the caller. Dispatch adds no
allocation, runtime registry, or hierarchy copy.

## Named resources and controller creation

Milestone 17 Goal 3 exposes authored controllers, sounds, music, and layers as
direct project fields. Actions belong under their controller, and screen/UI item
recipes belong under their viewport. For a project with these authored names:

```c
InputControllerIdResult player = rohr_controller_create(project.player);
/* After checking player, select an action on this runtime controller. */
InputActionIdResult jump = rohr_input_action_by_name_get(
    player.result.value, project.player.jump.name);

/* These named recipes already use the native resource descriptor types. */
SoundResult sound = rohr_audio_sound_create(project.jump_sound);
MusicResult music = rohr_audio_music_create(project.theme);
GraphicsLayerIdResult layer = rohr_graphics_layer_create(project.world_layer);
ViewportCreateItemDescription screen = project.initial_viewport.world;
```

Check every result before using its handle. The field names come from the
authored names; they are not fixed engine names. Audio services must be started
before loading sounds/music, and loading does not start playback. Layers retain
their existing globally shared named identity. Merely reading a recipe creates
no runtime state.

`rohr_controller_create()` and direct `controller_create()` accept a generated
controller view, a `ControllerDescription` containing a controller plus its action
list, or an `InputControllerDescription` for a controller with no actions. Use
`rohr_controller_description_default_get()` for an enabled, unnamed controller
with no actions, then fill its name and optional action list. A
`ControllerActionDescription` contains an action's configuration without a live
controller handle. `ProjectControllerDescription` and `ProjectActionDescription`
remain aliases of these input-owned recipe types.

Each call evaluates its argument once and copies the configured input state into
fixed engine pools. No entity owner or heap allocation is introduced. Destroy a
standalone controller and all its actions with `rohr_input_controller_destroy()`.
Malformed actions, duplicate names, and exhausted controller/action capacity
fail without leaving this attempt's resources or changing existing controllers.
Names and bindings are copied; caller recipe storage need only survive creation.

Standalone controller names remain globally unique. To create another named
instance, copy `project.player._description` into a `ControllerDescription` and
change its controller name before creation. Whole-project creation uses this same
creator after supplying its existing private instance name; project-local lookup
still uses the authored name, so independent project instances remain isolated.
Deleting one project does not destroy independently created controllers.

Viewport fields include named screen/UI recipes while retaining ordered
`items` and `item_count` access. Milestone 17 Goal 4 makes viewport creation use
value syntax: `rohr_viewport_create(project.viewport)` or
`rohr_viewport_create(description)`. When setting a native creation descriptor's
`template` pointer, use `&project.viewport._description`. Rebuild the SDK and
regenerate together. Generated teaching mains use these calls directly.

Generation rejects conflicting C identifiers across objects, viewports,
controllers, audio, and layers, and within each controller/viewport. For example,
`jumpSound` and `jump_sound` cannot occupy the same generated scope. Reserved
descriptor fields such as controller `actions` and viewport `item_count` are
also rejected. Diagnostics identify the conflicting names and resulting field;
failed validation preserves existing generated output and developer-edited mains.

## Shared graphics resource creation

Milestone 17 Goal 4 adds typed authored inputs to the normal resource creators.
These examples use illustrative authored names; check each result before use:

```c
ViewportIdResult view = rohr_viewport_create(project.game_viewport);
ScreenIdResult screen = rohr_screen_create(project.game_viewport.world);
GraphicsUiIdResult button = rohr_graphics_ui_shape_create(project.game_viewport.play_button);
GraphicsUiIdResult label = rohr_graphics_ui_text_create(project.game_viewport.title);
GraphicsUiIdResult slider = rohr_graphics_ui_slider_create(project.game_viewport.volume);
CameraIdResult camera = rohr_camera_create(project.player.camera);
TextureAssetResult image = rohr_graphics_texture_load(project.player.sprite);
AnimationAssetResult animation = rohr_graphics_animation_load(project.player.animation);
FontAssetResult font = rohr_graphics_font_load(project.game_viewport.title);
TextAssetResult text = rohr_graphics_text_create(project.game_viewport.title);
```

Each also accepts its existing native descriptor, including const values,
compound literals, and descriptor-returning function calls. Direct engine APIs
behave identically. Arguments are evaluated once. Authored viewport and object
views remain named lvalues borrowed at their original addresses. Unsupported C
types, pointers, and unrelated generated views are compile errors; a recipe with
the wrong resource kind returns an error. Replace old viewport `create(&value)`
with `create(value)` and dereference checked descriptor pointers explicitly.
The old public `graphics_viewport_create_description()` entry point is removed.

Camera creation returns only a camera, initially at its authored world pose. It
does not create an entity or bind to an existing target. A standalone authored
screen is automatic and initially unbound; connect an explicit live camera with
`rohr_screen_camera_set()`, or use a native `ScreenDescription` with runtime
bindings. Viewport object/camera bindings retain the existing scope rules.
Screen/UI creation does not mount the returned resource or apply its viewport
placement and layer. Mount it explicitly using the normal viewport item APIs.

Destroy cameras, screens, and UI with their existing destroy APIs. UI loaded from
a recipe owns its generated text, and that text owns its font dependency. UI
getters return borrowed text handles: do not destroy those as independent owners.
Replacing UI content preserves the recipe-owned text until UI destruction, so
previously obtained borrowed text stays valid for that lifetime. Native UI
creation still borrows the caller's text and acquires its existing private
render dependency; it does not take over the caller's public owner. Unmount UI
before destroying it. Viewport-owned UI uses the same resource creator and
releases these dependencies on rollback, viewport destruction, or shutdown.

Texture, animation, and font loads return one caller-owned reference; release
it with the matching release function. Text creation returns one unique owner;
destroy it explicitly. Resources survive independently of other object/project
instances. Sprite and animation-player value builders remain non-owning and
require loaded handles. Use `rohr_entity_create(project.player.sprite)` to create
the complete visible entity. No resource creator implicitly creates an entity.

Named layer, sound, and music recipes already use the native descriptor types
and need no extra dispatch. Their existing value creators and ownership remain
unchanged. Whole-project, object, and viewport creation reuse these shared
resource paths; global settings and simulation behavior are unchanged.

## Unified generated project

Regeneration now writes `src/generated/project.h` for applications and
`src/generated/scene/project.h` for scene builds. It includes `rohr.h` and exposes
one immutable `project` catalog containing named objects, nested members, and
viewport creation templates. New teaching content uses **Example**, exposed as
`project.example`; regeneration does not rename existing authored objects.

The initial main uses the whole-project path:

```c
#include "project.h"

/* After starting engine/graphics and applying project_settings_apply() once.
 * Start audio as well if the project contains sound/music recipes. */
ProjectIdResult created = rohr_project_create(project);
/* Check created, then use its project-local resources. */
ViewportListResult views = rohr_project_viewports_get(created.result.value);
/* Check views before presenting its values/count in the ordinary game loop. */
/* Destroy the project before stopping graphics/audio/engine. */
```

`project._project` is the immutable `ProjectDescription` passed through the typed
adapter. Its root hierarchy, named viewport recipes, controllers/actions, and all
audio recipes are created together. Audio loads stopped. Global settings remain
a separate startup operation, and each system update remains explicit in the loop.
`rohr_project_root_get()` and project-local resource lookups return borrowed handles.
`rohr_project_destroy()` releases the owned resources without changing other instances.

The generated main contains complete, line-commented alternatives for
`rohr_object_create(project.example)` and individual `rohr_entity_create()` calls.
Comment out the active main and uncomment exactly one alternative. Each main
contains its own startup, error checks, frame loop, report checks, and cleanup;
there are no local utility functions or wrapper macros. The object path creates
a blank viewport then binds its named screen
camera. The entity path supplies a camera in a local viewport creation descriptor.
Both explicitly clean up their resources. The independent entity path intentionally
shows a subset of the scene; it does not spawn a joint without its topology.

The object/entity alternatives create only their selected resources. Add a named
controller with `rohr_controller_create(project.player)`, then resolve its actions
with `rohr_input_action_by_name_get(controller, project.player.move.name)` after
checking each result. The player-controller example demonstrates this sequence.
Destroy separately created controllers explicitly; whole-project creation owns
its controllers. Existing generated `ProjectControllers` compatibility helpers
now delegate to the same creator rather than generating an action-construction
algorithm.

Generated loops accept physics NOT_RUN only when the report belongs to physics,
the tick delta is zero, and the report is valid. Other unsuccessful reports are
written to the console and lead to cleanup. Owned instances are destroyed before
their shared services stop. Authored global settings remain a separate
`project_settings_apply()` startup operation.

Regeneration upgrades only exact recognized old main templates. The previous
helper-based templates upgrade to a catalog-independent whole-project main, so
renaming authored example members does not break that migration. A developer
edit, including an added comment, preserves the entire main. To migrate an edited
main, use a fresh generated example as a reference and copy the selected pathway's
explicit calls, checks, loop, and teardown while retaining your gameplay. Archived
template strings exist only for strict migration recognition.

Authored screen camera choices remain in the immutable recipe. Whole-project
creation resolves them within the new instance. Standalone viewport creation
leaves screens unbound unless runtime cameras or object bindings are supplied.
Named members and hierarchy traversal reference the same description records.
Optional links outside the selected creation scope remain unbound unless supplied;
required physics dependencies still need the bindings described below.
Generated C field names are normalized identifiers; runtime member lookup uses
the authored name, available as `project.example.name`. Conflicting or reserved
generated names are rejected before writing output.

Definitions live in the existing `project_objects.c` translation unit.
`project.viewports` and `project.viewport_count` also allow application code to
iterate every authored viewport recipe. `project.h` includes the declarations
for project settings, controllers, audio, and existing object helpers.
For object compatibility, generation still emits the legacy
`objects` catalog and per-object instance wrappers; those wrappers now delegate
to shared engine creation rather than generating a separate construction
algorithm. The two catalogs temporarily duplicate immutable data and should not
be mixed in one set of description bindings.

By user direction, generated viewport compatibility support is removed in Goal 4.
Regeneration removes editor-owned `project_viewports.h/.c` from both application
and scene output. New examples use whole-project creation and present its viewport
batch once per frame. Simulation retains its selected hierarchy, camera binding,
and existing frame/host pipeline. It never spawns objects excluded by selection.
The active example contains only the authored scene, with no additional direct
physics tutorial bodies.

Exact recognized old starter/main and scene-build templates migrate automatically.
Developer-edited `main.c` and CMake files remain untouched. Customized callers must
replace `ProjectViewports` and its generated helpers with `ViewportId` handles,
`rohr_viewport_create()` and `rohr_graphics_viewports_show()`, then destroy each
viewport before its camera owner. Remove `project_viewports.c` from any custom
explicit source list. Regeneration reports old viewport-helper use in preserved
entry points. Archived template strings are only migration signatures; they are
not generated compatibility code. The engine's low-level screen/UI/viewport and
camera callback APIs remain available.

Milestone 16 Goal 5 integrates the wider SDK and game workflow verification.
Regenerate and rebuild consumers against the matching SDK:
`ObjectDescription` now includes named-layer identity, changing its binary layout.

## Named layers

`LayerDescription` contains a name and a numeric draw order. Creating it with
`rohr_graphics_layer_create()` returns a `GraphicsLayerId`; the existing layer
description getters/setters and destroy function remain available.

Object/entity descriptions preserve the assigned name in `layer_name` and its
authored value in `layer`. An empty name selects the numeric assignment. Creation
registers missing named layers, reuses matching definitions, and rejects a name
already registered with a different value. Successful registrations belong to
graphics globally until explicit destruction or graphics shutdown. Failed
creation removes only registrations introduced by that attempt. Destroying an
object does not destroy a shared layer.

Different names may have the same draw order. Changing one named layer affects
its bound content, not other names or unnamed content with that number. Future
camera allowlists/blocklists must use this identity; they are not implemented in
this goal. Named layers remain rendering categories, separate from ECS groups
and physics collision masks.

## Using descriptions

Include `rohr.h` and your existing generated catalog header. For example, the
bundled center-of-mass catalog exposes these descriptions:

```c
#include "rohr.h"
#include "project_objects.h"

/* Alternative creation operations; check each result before using its value. */
EntityResult instance = rohr_object_create(objects.mass_demo);
EntityResult body = rohr_entity_create(objects.mass_demo.automatic_body);
```

The first operation creates the whole object, including its bodies, anchors,
and camera. The second creates only the selected body and its configured
components. It does not create the body's separately authored anchor children
or any siblings. Container and soft-body aggregates require object creation.
Both operations return `EntityResult`; `rohr_entity_delete` releases the created
root and its owned contents. `rohr_object_destroy` still releases a loaded or
copied **description**, not a runtime instance.

Descriptions are borrowed during creation and never mutated. Each call copies
runtime state and acquires its own asset references. Releasing a loaded/copied
description after creation is safe. A const compiled catalog needs no release.
`EntityCreateDescription` names the existing common catalog record for a member;
the metadata-only `EntityDescription` API remains unchanged.

## Finding created members

After checking `instance`, find its immediate members with:

```c
EntityResult body = rohr_object_member_get(instance.result.value, "automatic_body");
/* After checking body: */
EntityResult anchor = rohr_object_member_get(body.result.value, "automatic_tip");
```

Lookup uses authored member names, independent of runtime name suffixes or later
runtime renaming. It does not recursively select a member from another nested
object. Missing or ambiguous names return an error. The older recursive
`rohr_entity_object_member_by_name_get` retains its original semantics.

Returned member handles are borrowed aliases. Some attached sprite/animation
descriptions resolve to their body's entity; deleting that handle deletes the
body. Remove the visual component when only that visual should be removed.

## Placement, ownership, and dependencies

Use `rohr_entity_create_options_default_get()` or
`rohr_object_create_options_default_get()` and change the required fields.
Both options support `owner`, a world-space translation `offset`, and a
`bindings` pointer with `binding_count`. Object options also expose the existing
physics/visual `flags`; zero flags are invalid, and the default getter selects
both categories. Supplying a lifetime owner does not apply its transform again.

Internal references resolve within the new object instance. Standalone cameras,
sprites, animations, gameplay targets, and transform drivers do not automatically
connect to anything outside that creation scope. Their authored targets must still
identify valid catalog members, but need not have live instances. Creating the
whole containing object restores those internal links automatically. Descriptions
are never rewritten by partial creation, and no global instance lookup occurs.

Independent settings, lifetime deadlines, and authored initial poses remain in
effect; omitted links do not create ancestors or reset position to zero. To connect
an external dependency, bind its description to the intended live entity:

```c
/* live_body is a previously created compatible entity. */
ObjectCreateBinding binding = {
    .description = rohr_object_get(objects.mass_demo.automatic_body),
    .entity = live_body
};
EntityCreateOptions options = rohr_entity_create_options_default_get();
options.bindings = &binding;
options.binding_count = 1;
EntityResult anchor = rohr_entity_create_with_options(
    objects.mass_demo.automatic_body.automatic_tip, options);
```

Binding descriptions must belong to the same catalog as the selected template.
Dangling authored identities, missing required physics references,
duplicate/conflicting identities, wrong kinds, stale explicit handles, cycles,
and invalid topology ownership fail creation. A joint binding
names an anchor entity; its owned anchor resource is resolved automatically.
Individual creation of a node, beam, or area requires the intended soft-body
owner. Attached anchors require their referenced bodies. Force/torque members
require their runtime owner and referenced targets.
Zero references retain the existing disconnected/world semantics.

Binding an attached visual to an existing body creates a separate attached
entity; it does not overwrite that body's existing components. Newly created
members within one object can still use the established visual alias behavior.

Creation allocates temporary bounded traversal storage and may allocate runtime
components/load assets. Dependencies are created before their users even when
listed later in the catalog. Existing owners receive the completed root only
after all other fallible work succeeds. Failure deletes the attempt's entities,
anchors, cameras, aliases, and acquired asset references. Initial force/torque
sources retain the existing next-update lifetime; creation does not apply a
force or advance simulation.

## Verification

Verification results and inherited failures are recorded in the milestone 15
entry in [NEXT_STEPS.md](../NEXT_STEPS.md#15-descriptor-based-creation-and-automatic-viewport-rendering).
Goal 4 evidence and staged SDKs are under `sandbox/milestone_15/goal_4/`.
Eighteen focused Release checks pass. The sanitizer checks pass across the
initial run and an isolated scene-session rerun; the initial session stress test
overflowed its input queue, with no memory finding. The unchanged test passed
after temporary-build cleanup freed disk space. Six installed-SDK consumers and
the migrated starter execute on Linux and under ASan/UBSan, and cross-build for
Windows. Generated consumers cover descriptor identity, named-layer separation,
camera bindings, clipping, fonts, UI, independent instances and 1,000 lifecycle
cycles. Preservation, partial-spawn, scaffold recovery, configured-path and
relocation checks pass. The user accepted the final automatic-rendering starter
preview. Native Windows execution and generic Linux packaging remain unverified.

The following Goal 2 baseline evidence remains under `sandbox/milestone_15/goal_2/`.
The focused creation suite covers public/direct calls, generated-layout views,
independent instances, released description copies, nested member access,
explicit bindings, forward references, ownership, malformed catalogs, allocation
failure rollback, and 1,000 creation/destruction cycles with camera invalidation
and texture ownership accounting.

The eight focused suites (`descriptor_creation`, `object_instances`,
`runtime_persistence`, `shared_sources`, `soft_body`, `texture_assets`,
`animation_assets`, and `authored_car`) pass in Release and under ASan/UBSan with
leak detection. Allocation injection covers entity/component allocation,
external anchors, owner attachment, and texture-reference rollback. The latest
creation tests also pass after adding explicit external torque and anchor cases.

All installed consumer targets and independent car/flies examples build against
isolated Linux and Windows SDK installations. Three Linux consumers execute
successfully, including the generated-catalog creation case. Consumers and
examples launch from a temporary directory outside the source tree with dummy
SDL drivers. The car completes the requested 600-frame diagnostic prefix with
2,400 substeps, 701,387 candidates, and 108,585 contacts. Its own report correctly
marks the full 60-second benchmark incomplete; this is neither full benchmark
acceptance nor a rendered timing claim. Flies starts and exits cleanly on SIGINT.
No compiler warnings were observed in these builds. Native Windows execution
was not performed. These are Nix-built installed SDK checks; generic Linux
distribution packaging and the integrated game workflow remain Goal 6.

Two broader physics checks remain failing: `joints` exits with SIGSEGV in Release
and fails the angular-velocity assertion at `tests/joints.c:340` in the sanitizer
configuration; `forces` fails `completed > substeps` at `tests/forces.c:95`.
Both reproduce with matched builds of the pre-change implementation from
`409041e2` and with the new implementation. The comparison substitutes the two
modified entity implementation translation units in an otherwise identical
engine archive, then relinks the unchanged tests. Neither sanitizer run reports
a memory or undefined-behavior finding for those assertion failures. The focused
eight-suite result does not imply the full suite is clean; physics defaults and
these inherited failures remain unchanged.

This is the historical Goal 2 result. In Goal 6, the user approved updating only
the obsolete forces-test timing assertion to allow the adopted fixed-step mode.
All force/torque numerical checks remain intact and pass; the joint and other
physics failures are not claimed resolved. See the
[integrated verification report](creation_rendering_verification.md).

## Milestone 17 Goal 2 verification

Value-style entity/object/project creation passes eleven focused checks in both
Release and ASan/UBSan with leak detection: `automatic_viewports`,
`object_instances`, `project_creation`, `descriptor_creation`,
`editor_example_pathways`, `editor_partial_spawning`,
`editor_construction_generated`, `editor_configuration_generated`,
`editor_scene_generation`, `editor_cli_scene_selection`, and `editor_project`.
These include immutable catalog identity, optional/required scope bindings,
independent runtime instances, released source descriptions, allocation-failure
rollback, survivor-safe destruction, and 1,000-cycle resource-lifetime checks.

Five consumers pass against separately installed Release and sanitizer SDKs:
`rohr_creation_dispatch`, `rohr_object_descriptions`, `rohr_automatic_viewports`,
`rohr_generated_workflow`, and `rohr_project_creation`. The workflow consumer
performs 1,000 object and 1,000 project create/extend/use/destroy cycles.
New dispatch checks exercise side-effecting arguments and options exactly once,
both generated catalogs, plain descriptors, const/mutable inputs, compound
literals, root-only creation, and public/direct equivalents. Compile checks
first accept supported forms, then reject all twenty unsupported-type cases.
The expected negative compiler diagnostics are successful contract checks,
not unresolved build failures.

The Linux engine/editor build succeeds. Engine/editor, affected tests, all five
consumers, and the three generated example pathways cross-build for Windows.
The same compile contract passes against the installed Windows SDK. Native
Windows execution remains unverified. No compiler warnings or sanitizer findings
remain in the verified builds/runs.

Center of mass, player controller, and soft-body car build independently against
the installed Linux and Windows SDKs. Linux runs use dummy SDL drivers from
`/tmp`; center of mass and player controller exit cleanly after SIGINT. The car
reaches all 3,600 frames / 60 simulated seconds with 14,400 substeps, 7,222,164
candidates, and 1,195,541 contacts, matching the earlier functional reference.
This is not a performance acceptance run or a new visual review; its
`realtime_pass=0` is not claimed resolved. Physics and presentation behavior were
not changed. Generic Linux distribution packaging remains a later parity goal.

Regenerating copies of all three bundled projects produces byte-identical
catalogs and preserves their custom main/build files. The generated pathway
test also preserves edited entry points and build files. Historical exact-match
main snapshots remain unchanged; external mains using pointer-style calls need
the explicit migration described above. Build/test logs, SDK installs, and
disposable verification projects are under `sandbox/milestone_17/goal_2/`.

## Milestone 17 Goal 3 verification

Thirteen focused checks pass in Release and ASan/UBSan with leak detection:
the eleven Goal 2 checks above plus `input_actions` and `controller_creation`.
The new controller test exercises native/simple and complete recipe forms,
single evaluation, copied configuration, independent state, stale handles,
malformed actions, duplicate names, controller capacity, partial action-capacity
rollback, survivor preservation, and 1,000 create/destroy cycles.

Seven consumers pass against independently installed Release and sanitizer SDKs:
the five Goal 2 consumers plus `rohr_controller_creation` and
`rohr_input_descriptions`. Compile-contract checks accept native and generated
inputs and reject all 27 invalid cases, including passing an action, object, or
pointer to controller creation and passing a controller to entity creation.

The generated-resource fixture exercises named controller/action, sound/music,
layer, and screen/UI recipes. It runs 1,000 generated-controller cycles, compares
named data with ordered recipes, and verifies independently created controllers
survive destruction of two project instances using the same authored recipes.
Disabled controllers and initial action state remain correct. Ten name-conflict
cases cover cross-category collisions, normalized action/screen/UI collisions,
and reserved fields; each checks diagnostics and unchanged generated files.
Save/reload/regeneration produces identical artifacts and preserves an edited
main. Named recipe fields duplicate immutable values for direct access; the
runtime lists still instantiate each configured resource exactly once.

The engine/editor, affected tests, seven SDK consumers, all three generated
creation paths, and the generated named-resource fixture cross-build for Windows.
Center of mass, player controller, and soft-body car build independently against
the installed Linux and Windows SDKs. Native Windows execution and generic Linux
distribution packaging were not performed in this goal. No compiler warnings or
sanitizer findings remain in the verified builds/runs.

Linux examples run with dummy SDL drivers from `/tmp`. Center of mass and player
controller exit cleanly after SIGINT; the car reaches all 3,600 frames / 60
simulated seconds with the unchanged 14,400 substeps, 7,222,164 candidates, and
1,195,541 contacts. These are functional checks, not performance acceptance;
the benchmark's `realtime_pass=0` is not claimed resolved. Physics and rendering
behavior are unchanged. Evidence and disposable SDK/consumer builds are under
`sandbox/milestone_17/goal_3/`.


## Milestone 17 Goal 4 verification

Evidence and isolated SDK installations are in `sandbox/milestone_17/goal_4/`.
The user accepted the real installed-SDK workflow preview: independent views,
rotated insets, text/buttons, and the first instance's visibility toggle.

- Eighteen focused Release and ASan/UBSan checks pass with leak detection:
  controllers/input, viewport descriptions, cameras, automatic viewports,
  texture/animation/text assets, object/project/descriptor creation, generated
  example pathways, partial spawning, construction/configuration parity,
  scene generation/selection, and editor-project generation/preservation.
- Seven installed-SDK consumers pass in both configurations: controller creation,
  input descriptions, creation dispatch, object descriptions, automatic viewports,
  generated workflow, and project creation. Generated resource fixtures exercise
  direct named resource creation, no hidden entities, unbound cameras/screens,
  explicit binding, wrong-kind rejection, and independent assets/UI over 1,000
  cycles. UI-capacity failures release newly loaded text and preserve survivors;
  shutdown and ordinary destruction invalidate the correct owned text handles.
- Supported native and generated forms compile, including native function-return
  and compound-literal expressions. All 67 negative compile cases reject pointer,
  unrelated, or wrong-family inputs on Linux, sanitizer builds, and Windows.
- Engine/editor, affected tests, consumers, all three generated example pathways,
  and the full named-resource fixture cross-build for Windows. All 13 bundled
  examples independently build against both installed SDKs; all 13 run on Linux
  outside their source directories. Native Windows execution remains unverified.
- The car completes 3,600 frames / 60 simulated seconds with the same 14,400
  substeps, 7,222,164 candidates, and 1,195,541 contacts. This is functional
  verification during other work, not a controlled performance measurement;
  its existing `realtime_pass=0` gate is not reclassified or resolved here.
- No compiler warnings, ASan/UBSan findings, or leak findings remain. No new
  dependencies, physics settings, or presentation algorithms are introduced.
  Generic Linux packaging and the wider final SDK review remain Goal 6.

Rebuild the SDK and regenerate catalogs together before migrating external
callers. Developer-edited mains remain preserved. The following Goal 5 updates
the teaching mains without changing the resource-creation contracts above.

## Milestone 17 Goal 5 verification

Evidence and isolated SDK installations are in `sandbox/milestone_17/goal_5/`.
The user accepted the generated whole-project example's scene, animation, and
viewport UI. Its unchanged production main ran outside the source directory and
exited cleanly.

- Ten focused tests pass in Release and ASan/UBSan with leak detection: all three
  example pathways, editor project generation/preservation, build scaffolding
  recovery, partial spawning, construction/configuration parity, scene generation,
  scene session lifecycle, CLI scene selection, and center-of-mass integration.
- Each teaching main compiles and runs independently. Test-only copies exercise
  pause/resume and valid zero-tick skips, plus failed physics, wrong-system and
  malformed NOT_RUN reports, and engine, animation-update, and presentation
  failures. Failure paths attempt diagnostics, return failure, and destroy all
  entities before service shutdown. Test wrappers are never generated into mains.
- Exact old templates migrate; near matches and developer comments remain
  untouched. Regeneration preserves edited mains and custom CMake files. Generated
  controller compatibility wrappers preserve action settings while using the
  shared creator. The scene runner retains its required lifecycle callbacks;
  its redundant viewport-construction helper is removed.
- Seven installed-SDK consumers pass in Release and ASan/UBSan: controller
  creation, input descriptions, creation dispatch, object descriptions, automatic
  viewports, generated workflow, and project creation. The 67 negative compile
  cases continue to reject unsupported inputs on Linux and Windows.
- Engine/editor, affected tests, SDK consumers, all three generated pathways and
  their failure harnesses, and the named-resource fixture cross-build for Windows.
  Center of mass, player controller, and the soft-body car independently build
  against both installed SDKs and run on Linux outside their source directories.
  Native Windows execution and generic Linux packaging remain Goal 6 work.
- The car completes 3,600 frames / 60 simulated seconds with 14,400 substeps,
  7,222,164 candidates, and 1,195,541 contacts. These are functional checks;
  the existing `realtime_pass=0` gate remains unresolved. No physics, presentation,
  or performance behavior is changed by this goal.

The expanded checks caught and corrected a controller-value dereference in the
generated scene. The sanitizer scene-session runner also required building its
existing child fixture. A test-only macro redefinition warning was fixed and the
three pathway checks rerun. No compiler warnings or sanitizer findings remain in
the final runs. Goal 6's integrated SDK parity review remains pending.
