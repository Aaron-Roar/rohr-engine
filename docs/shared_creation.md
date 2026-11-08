# Shared entity and object creation

Milestone 15 Goal 2 implements the entity/object portion of the
[creation contract](creation_and_viewport_contract.md). Goal 3 adds
[automatic viewports](automatic_viewports.md), and Goal 4 adds the unified
generated `project.h`. Existing generated catalogs remain compatible.

Milestone 16 Goal 2 implements the [scoped creation rules](scoped_creation_contract.md)
for optional external links and unbound viewport screens. Goal 3 adds
`rohr_project_create()` and project ownership. Goal 4 adds generated project
recipes and the three-path example main.

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
ProjectIdResult created = rohr_project_create(&project);
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
`rohr_object_create(&project.example)` and individual `rohr_entity_create()` calls.
Comment out the active main and uncomment exactly one alternative; the shared
loop stays in place. The object path creates a blank viewport then binds its screen
camera. The entity path supplies a camera in a local viewport creation descriptor.
Both explicitly clean up their resources. The independent entity path intentionally
shows a subset of the scene; it does not spawn a joint without its topology.

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
EntityResult instance = rohr_object_create(&objects.mass_demo);
EntityResult body = rohr_entity_create(&objects.mass_demo.automatic_body);
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
    &objects.mass_demo.automatic_body.automatic_tip, options);
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
