# Descriptor creation and automatic viewport rendering

> Historical creation design: Milestone 18 replaces the runtime hierarchy lookup
> APIs described below. Current code uses typed IDs, runtime names and explicit
> entity lists; see [runtime access migration](runtime_access_migration.md).

Milestone 15 Goal 1 design contract, 2026-11-06. This document specifies the
target API and behavior. Goal 2 implements the entity/object creation portion;
see [shared creation](shared_creation.md) for the available calls. Goal 3 implements
[viewport creation and automatic rendering](automatic_viewports.md); Goal 4 adds
[unified project generation](shared_creation.md#unified-generated-project).
Goals 2–6 in [the roadmap](../NEXT_STEPS.md#15-descriptor-based-creation-and-automatic-viewport-rendering)
implement, migrate, and verify it. Source inspection used baseline `7bae1db6`.
No physics algorithm, timestep, or solver setting changes are part of this work.
Goal 6's [integrated verification](creation_rendering_verification.md) records
completion, measured rendering costs and final user acceptance.

Milestone 16's [scoped creation contract](scoped_creation_contract.md) specifies
the next changes: optional external links for standalone creation, background-only
unbound viewport screens, automatic whole-project wiring, and a teaching starter.
Milestone 16 Goal 2 implements optional external links and unbound screens,
superseding the corresponding strict binding requirements below. The project API
and teaching starter remain planned; this milestone 15 document retains the
historical contract for its original implementation and verification.

Milestone 17 Goal 4 supersedes the pointer-based viewport dispatch below with
[value-style shared resource creation](shared_creation.md#shared-graphics-resource-creation).
The call examples use the current value spelling. Native scalar viewport
functions remain callable; the old public recipe-pointer creator is removed.

## User decisions

- Objects are immutable descriptions in the application. Creation produces
  independent mutable instances, not edits to the descriptions.
- Shared engine functions replace generated object-specific creators.
  `rohr_object_create(project.water)` creates the described hierarchy;
  `rohr_entity_create(project.water.particle)` creates the selected member.
- A viewport gathers and draws scene entities using their runtime appearance,
  visibility, and layer. Application camera callbacks are unnecessary.
- Equal-layer scene entities use discovery order, with no stability guarantee
  and no creation-order bookkeeping. Applications use distinct layers when
  overlap order matters.
- Named layers retain distinct identities even when their numeric draw order
  matches. Future camera allowlists/blocklists select a named identity, never
  every item sharing that number. Camera filtering remains future work.
- Game code explicitly chooses the runtime camera instance. A description name
  must never silently select the first or most recently created matching entity.
- An authored screen's camera assignment is applied during viewport creation,
  using the supplied runtime bindings. It does not require a second binding call.
- A created viewport remains extensible: application code can add, change, and
  remove screens, buttons, text, sliders, and other supported UI after creation.

## Existing foundations and compatibility constraints

| Current source | Relevant behavior |
| --- | --- |
| [object.h](../include/object.h) | `ObjectDescription` already describes hierarchy, geometry, appearance, cameras, and references; object spawning returns an `EntityResult`. |
| [entity_components.h](../include/entity_components.h) | `EntityDescription` currently contains only name and lifetime owner. Entity ownership is independent of transform parenting. |
| [object_spawn.c](../src/entity/object_spawn.c) | Spawns in dependency passes, retains instance appearance, owns cameras/anchors, resolves authored references, and cleans up failed spawns. Some attached visual descriptions alias a body entity. |
| [graphics.h](../include/graphics.h) | Existing `ViewportDescription` is scalar configuration. Separate screen/UI APIs create and mount content and support later mutation. |
| [graphics.c](../src/graphics/graphics.c) | Camera callbacks currently supply screen content. Composition, command execution, reports, and presentation are already separate stages. |
| [editor_workspace.c](../tools/editor/src/editor_workspace.c) | Generated viewport code currently creates resources, assigns callbacks, draws project objects, and tears down its resource arrays. |

Reuse these systems. Do not build a parallel appearance database, asset registry,
scene framework, or project-specific rendering backend. In particular, ordinary
collision components do not imply visible geometry: the existing explicit
appearance or visual component determines eligibility.

## Description and instance model

Authored descriptions contain configuration and authored references, never mutable
instance handles, frame counters, playback progress, or callback context. A local
viewport creation descriptor may additionally supply borrowed runtime camera
bindings; it does not modify the authored template.
Generated `project.h` exposes a `const` catalog; generated source files hold its
definitions. Named children such as `project.water.particle` are views of the
same description data traversed by creation, not another copy of that data.

Use `ObjectDescription` for an object template and `EntityCreateDescription`
for a member creation template. Preserve the current metadata-only
`EntityDescription` and its get/set APIs. The creation types may share existing
descriptor storage internally; they must not duplicate mutable visual state or
reinterpret a metadata setter as respawning an entity. A member template carries
the supported component configuration for its kind, including its appearance.
This milestone covers the existing authored inventory, not a universal plugin
or arbitrary component reflection system.

An instance is represented by existing generation-checked entity handles. An
object instance has a root entity owning its created members; this is a runtime
implementation detail, not a mutable authored object. An individual entity can
exist independently even when its description is nested inside an object.

Creators borrow input descriptions, entity/object options, arrays, strings, and bindings for
the duration of the call. They copy required runtime values and acquire required
asset references. After a successful call, releasing a loaded catalog or an
editable description copy cannot invalidate the instance. Generated static
catalogs need no destruction. Creating twice produces independent mutable state;
immutable texture/font/animation resources may share existing registries.

Existing explicit editable-description copies remain a compatibility facility.
They change future creations only and never mutate generated `const` catalogs.
There is no new implicit deep-copy or per-frame template interpretation facility.

## Creation API

The entity/object creation and member-lookup calls below are implemented in
Goal 2. Goal 3 adds complete viewport creation, item lookup, and selected show.
The user simplified viewport creation to one descriptor, replacing the originally
proposed separate options argument. Result values must be checked before handles are used.

| Target call | Result and responsibility |
| --- | --- |
| `rohr_entity_create(project.water.particle)` | `EntityResult`; instantiate one member template, with default options. |
| `rohr_entity_create_with_options(description, options)` | `EntityResult`; explicit owner, placement offset, and external-reference bindings. |
| `rohr_object_create(project.water)` | `EntityResult`; instantiate the full hierarchy and return its owning root. |
| `rohr_object_create_with_options(description, options)` | `EntityResult`; explicit creation options for the hierarchy. |
| `rohr_object_member_get(instance, "particle")` | `EntityResult`; find a direct named member of this instance. |
| `rohr_viewport_create(project.main_viewport)` | `ViewportIdResult`; instantiate a self-contained viewport description without external bindings. |
| `rohr_viewport_create(creation)` | `ViewportIdResult`; a local creation descriptor references the immutable authored template and supplies explicit runtime camera bindings. |
| `rohr_viewport_item_by_name_get(viewport, "world")` | `ViewportItemIdResult`; find the named created item for later modification. |
| `rohr_entity_delete(entity)` | Existing operation; destroy an entity and its owned members/resources. |
| `rohr_viewport_destroy(viewport)` | Destroy the viewport and resources it created and owns. |

Direct engine APIs and `rohr_` entry points have equivalent behavior. Use small
engine-owned adapters where generated typed catalog views need conversion to a
common description pointer. Such adapters evaluate their argument once, respect
C layout/aliasing rules, and dispatch to shared runtime functions. They are not
generated per-object creator implementations. Keep compiled, handwritten, and
loaded descriptions on the same implementation path.

An object creator accepts object/container and aggregate roots and traverses
their members. An entity creator does not instantiate separately authored
children, siblings, or an entire object. Aggregate templates, such as a soft body
with topology, go through object creation; invalid use of the entity creator
reports an error. A member's
configured components are created together, but a separately authored sibling
sprite is not implicitly pulled in by creating only its particle.

`EntityCreateOptions` and `ObjectCreateOptions` expose the existing world-space
translation offset and optional lifetime owner. Object options also preserve
the current physics/visual category selection. Defaults select all categories,
zero offset, no external bindings, and an independent root. Required references
excluded by a category selection fail validation rather than silently rebinding.
Authored ancestor transforms are resolved as in current spawning; an explicitly
supplied owner establishes lifetime ownership and does not apply its transform
a second time. Individual creation uses authored effective visibility as its
initial visibility; whole-object creation retains separate parent/child flags.

### Member access and dependency resolution

`rohr_object_member_get` searches immediate members by the copied authored name,
not the globally suffixed runtime name. Nested access repeats the lookup on the
returned aggregate. Duplicate sibling names, missing members, or stale roots
produce errors. The existing recursive `rohr_entity_object_member_by_name_get`
retains its ambiguity-checking semantics for compatibility.

Member handles are borrowed aliases, not independently owned references. Existing
attached sprite/animation aliases may resolve to the same entity as their body;
creation must preserve that behavior where safe. Discover and draw that entity's
visual components once. Deleting a borrowed alias deletes the actual entity,
not just the named visual; component removal is the way to remove that visual.

Internal authored references resolve only within the current creation attempt.
External bindings explicitly map a referenced description identity in that
catalog to a live entity; anchor references resolve through that entity's anchor
resource. Validate types, topology ownership, and generation before use. There
is no fallback to a global name lookup. Zero references retain their existing
documented disconnected/world semantics; missing nonzero required references fail.

Individual creation of a joint, beam, attached visual, camera attachment, or force
requires the referenced entities and any required aggregate owner to be supplied.
It must not manufacture a replacement body, turn a node into a standalone
particle, or attach to a different instance. A binding to an existing body does
not authorize replacing its sprite or changing its physics state. Create a
separate attached visual entity when necessary. Stage initial force/torque effects
on external entities until all fallible creation work has succeeded. Validate
their targets before commit; no failed creation may leave an applied impulse
behind. This is transaction handling around existing operations, not a change
to their physical meaning.

### Creation and failure lifetime

Creation may allocate bounded runtime storage and load required assets. Document
those costs as creation-time work; no allocation is hidden in a description
getter. Validate structural input and bindings before publishing the root. On
failure, release all newly acquired resources and remove newly created entities,
aliases, ownership entries, and registrations. Previously live instances and
borrowed resources remain valid and unchanged. Return the actual failure using
existing typed result/error conventions; no partially usable root is returned.

Runtime callers own returned roots and viewports. Copying a handle does not add
ownership. Do not repurpose `rohr_object_destroy`: today it releases a loaded or
copied **description**. Runtime instance deletion remains `rohr_entity_delete`.
Existing invalid/stale-handle rejection semantics remain in force.

## Viewport content and camera bindings

Introduce `ViewportCreateDescription` for the complete immutable recipe: scalar
viewport configuration, named screen/UI items, layout and clipping, content
transforms, render sizes, layer definitions/references, background, text/font
definitions, and authored camera references. Preserve `ViewportDescription` as
the existing scalar runtime configuration used by its get/set APIs. A scalar
replacement must not rebuild content or reset live UI state.

`ViewportCreateDescription` supplies runtime camera bindings in the same input as
the authored recipe. An application can use a local descriptor with `.template`
pointing to the immutable authored recipe and `.objects`/`.cameras` arrays naming
chosen live instances. There is no viewport `_with_options` call or separate
options-default getter. `graphics_viewport_description_default_get()` remains the
scalar configuration default getter. Bindings can identify
the chosen object instance for an authored object reference, letting creation
resolve its authored camera member automatically, or override a particular named
screen item with an explicit camera entity. An explicit item override takes
precedence over an object-instance binding. Duplicate/conflicting entries and
unconsumed binding keys are errors, not order-dependent choices. Direct camera
handles are also supported for cameras created through the low-level API.

The binding arrays are explicit pointer/count pairs. Each object-instance binding
contains an authored object description reference and its runtime root entity.
Each item-camera binding contains the authored item name and exactly one live
camera entity or direct camera handle. Empty arrays mean no bindings. Defaults
do not consult the active camera or any global last-created instance. Bound
entities/cameras are borrowed, not transferred to the viewport.

For example, supplying the chosen player instance while creating an authored
viewport resolves that player's camera onto its authored screen immediately.
The caller does not repeat screen geometry, camera member configuration, or a
post-creation attachment step. A viewport never spawns a second player merely
because its screen references the player's camera description.

An explicit camera entity binding is checked for the owned camera resource;
an instance binding is checked against the referenced template and member.
Store the resolved generation-checked references, not borrowed description
pointers. A declared camera reference without the required runtime binding fails
creation and identifies the screen/reference. A deliberately camera-less screen
is allowed and retains the existing missing-camera placeholder behavior. UI-only
viewports need no bindings. Destruction of a previously bound camera leaves its
screen alive, with the existing placeholder and composition warning when visible;
slot reuse must not attach another camera silently.

### Runtime extension and ownership

The created viewport is mutable. Keep the existing screen/UI creation, item-add,
item-remove, description-get/set, camera-set, and interaction APIs. They apply
equally to generated and application-added items. An application's new screen
can mount a camera created later; a button added later participates in layout,
clipping, layering, interaction, reporting, and teardown like an authored button.
No regeneration is needed to extend a live viewport.

Named authored items are discoverable through `rohr_viewport_item_by_name_get`;
the existing item-description getter exposes their screen/UI handles. Runtime
add APIs already return item handles, which game code can retain. Changing a
screen's camera through `rohr_screen_camera_set` updates its binding; a direct
camera assignment clears an obsolete entity-binding association. Runtime setters
never write back into the immutable catalog.

| Resource | Lifetime rule |
| --- | --- |
| Object instance, camera entity, or externally supplied camera | Borrowed by the viewport; never deleted by viewport teardown. |
| Screens, UI definitions, text, and font references created from the viewport recipe | Owned by the created viewport; released on rollback or teardown. |
| Existing screen/UI mounted through `rohr_viewport_screen_add` or `rohr_viewport_ui_add` | Borrowed public ownership, with current internal dependency rules; removing the item does not destroy the caller's resource. |
| UI created by convenience `rohr_viewport_ui_*_add` | Existing automatic cleanup after its last mount is removed; text/font input ownership still follows the asset APIs. |
| Screens created by camera-add on a new recipe-created viewport | Owned by that viewport's resource inventory; reused only within that inventory when mode/dimensions match. Explicit screen mounting is the way to share across viewport owners. |
| Shared textures, animation data, and fonts | Existing explicit retain/release rules; each owner releases only its acquired reference. |
| Named graphics layers | Global graphics resources; viewport deletion must not invalidate another viewport or entity's layer binding. |

Recipe-created content is retained in a viewport-owned resource inventory until
viewport destruction, even if its original item is removed. Borrowed handles
retrieved from this inventory must not be independently destroyed by the caller.
Mounting such a resource elsewhere does not transfer its public ownership; keep
the owner alive or create independently owned content. This keeps ownership
explicit while allowing arbitrary runtime item replacement.

Release item dependencies before screens/UI, UI before text, and text before
custom font references. Preserve queued draw payload references through execution
or discard. Graphics shutdown invalidates renderer-owned resources. Cameras remain
engine-owned state; an engine/graphics restart invalidates their prior handles too.

Named layer definitions are registered/reused by name and value during creation.
A conflicting definition fails rather than changing other instances' ordering.
Successful shared registrations remain graphics-owned until explicit layer
management or graphics teardown; repeated identical creation must not grow the
registry. Failure rolls back newly introduced unreferenced registrations. This
does not change existing explicit layer mutation/destruction APIs.

Legacy `rohr_viewport_camera_add` currently finds or creates a graphics-lifetime
screen for a camera; removing its item does not destroy that screen. Preserve
that behavior for legacy scalar-created viewports. The new recipe-created path
uses the owned-screen rule above and must not adopt an unrelated legacy screen.
Test this distinction rather than claiming the old helper already owns screens.

## Automatic drawing and presentation

Target frame entry points return the existing owned `Report` type:

```c
Report rohr_graphics_viewport_show(ViewportId viewport);
Report rohr_graphics_viewports_show(const ViewportId *viewports, size_t count);
```

The singular call is the one-element batch. Each successful frame invocation
clears the window composition target, updates camera motion once, gathers current
visual state, renders selected screens, executes commands, composes selected
viewports, records/captures once, and presents once. Overlapping viewports compose
in caller-provided array order. Two singular calls are two frames; applications
use the batch for split-screen or multiple panels in one frame.

Validate the complete selection before consuming queued work. Null with nonzero
count, invalid handles, or duplicate viewport IDs fails without presenting or
consuming pending work. An empty selection, or all-disabled selection, produces
only the window clear/background and still permits one paced presentation; it
does not resurrect old application visuals. No enabled viewport means no
application visuals. Hidden items are intentional omissions, not warnings.

Render a shared screen once per invocation, even if mounted repeatedly. Different
screens using the same camera may have different dimensions and need distinct
renders. Camera advancement is once per frame, not per screen. Physics, gameplay,
audio, and animation updates remain explicit and are never advanced by show.
Preserve camera enable/pause behavior and the existing frame limiter, VSync,
late-frame recovery, recording, and hosted capture semantics.

### Rendering state and ordering

- Gather live entities with actual visual components or retained appearance.
  Do not draw every physics hitbox or require membership in generated project
  arrays. Later-created entities become visible through the same path.
- Reuse existing sprite, animation, particle, body, soft-body, and marker drawing
  implementations. Expose missing appearance mutation/query access through the
  existing state model rather than storing another generated draw description.
- Effective app visibility includes applicable containing-object visibility;
  hiding a parent does not overwrite each child's own flag. It does not disable
  physics. Editor-only eyes stay separate. Camera-entity visibility participates
  in whether that bound camera produces content.
- Resolve current graphics layer bindings, then draw lower values before higher
  values. Within the same layer, use discovery order without a stable tie key.
  Slot reuse and ownership changes may change this order. Maintain the necessary
  local primitive sequence inside one visual, such as fill before its outline.
- Do not promise the legacy soft-body fill/beam/node tie order in automatic mode.
  If an example requires it, give those visuals distinct authored layers and
  disclose the migration. Do not change physical topology or solver behavior.
- Preserve viewport-item layer ordering, with screens below UI at equal item
  layers. Items at other layer values can appear between screens as today.
  Screen content transforms remain separate from viewport-local placement.
- Runtime cameras see the runtime scene; editor camera previews retain their
  existing owning-object restriction and editor-only no-camera fallback.

Automatic screens clear to their configured screen background each redraw so
moving or hidden objects cannot leave trails. Keep this separate from the
viewport's composition background; migrate the current generated scene background
explicitly. New background properties require persistence and round-trip parity.

### Callback compatibility and manual pipelines

Use an explicit per-screen content mode: automatic scene or legacy callback.
New recipe-created screens default to automatic scene. Existing low-level screen
descriptions default to legacy callback for source compatibility. A runtime-added
screen can explicitly select automatic mode using its screen description;
viewport convenience camera-add should inherit the created viewport's automatic
default. The same camera may be used by screens in either mode. An automatic
screen never invokes its camera's legacy callback, avoiding double drawing.

Keep `rohr_graphics_show()` as the compatibility all-enabled-viewports frame
entry point, routed through the same shared implementation. It respects screen
mode rather than running both paths. Existing callbacks remain available to
legacy consumers during this milestone; new generated game code uses none.

Preserve manual pipeline execution and add explicit selection variants for the
camera and composition stages:

```c
Report rohr_graphics_pipeline_cameras_update_for_viewports(
    const ViewportId *viewports, size_t count);
Report rohr_graphics_pipeline_viewports_draw_selected(
    const ViewportId *viewports, size_t count);
```

The existing no-argument stages select all enabled viewports. Commands, recording,
and presentation remain separate existing stages. Selected stages use the same
validation, rendering, and reporting helpers as show; there is no sticky global
selection affecting the next call. For manual orchestration, use the same
selection for camera and composition stages and execute in the existing documented
order. Do not run the camera stage again after show for the same intended frame.

Full selected-show calls do not execute pending commands targeting unselected
screens or expose window-target application drawings outside viewports. Discard
such commands with diagnostics and release their references once. Invalid
selection validation happens before this consumption. The explicit manual command
stage retains its existing consume-all-queued-work semantics.

### Reports, failures, and cost

Use `ROHR_GRAPHICS_REPORT_ID` and the existing camera, commands, viewports,
recording, and presentation child IDs. Each parent merges each executed child
once. Measure parent wall time independently; presentation timing includes its
wait. Scene gathering/setup belongs to camera-stage work, draw submission and
execution evidence to commands, and item composition to viewports. Update stage
labels that currently imply every camera render is a callback.

Preserve owned diagnostic snapshots, retained first-fatal evidence, bounded
capacity/overflow rules, and current recoverable-versus-fatal handling from the
[report contract](reports.md#graphics-reports). Bad individual visuals warn and
allow independent work to continue; invalid required render state stops dependent
stages and releases queued resources safely. An invalid frame invocation returns
a failed report without disturbing a currently executing frame. Rendering remains
single-threaded and recursive show calls remain invalid.

Discovery should use the live entity inventory or an equivalently cheap existing
index, avoid duplicate traversal through aliases/owners, and avoid new per-frame
heap allocation solely for scene discovery. Do not introduce stable-sort metadata,
per-entity creation timestamps, or a second renderer. Distinct camera projections
still require distinct drawing; no unmeasured cross-camera cache is promised.

## Generation and migration

The application includes `rohr.h` and one generated `project.h`. The latter is
necessary to declare project-specific names; the SDK header cannot know them.
Generated definitions may remain split into internal source modules for build
size, but the public catalog does not require a separate viewport header or
object-specific creator/draw functions. Generated modules hold any generated
runtime support state; developer gameplay remains developer-owned.

Retain current metadata APIs and low-level component/resource APIs. During Goal 4
the user explicitly chose to remove generated legacy viewport files and helpers,
folding required starter and scene migration into that goal. Preserve customized
entry points and document their required manual migration. Existing object
helpers remain available. The original Milestone 15 design for the by-value
`rohr_viewport_create(ViewportDescription)` source form used a narrow C11 adapter
dispatching by value versus a complete-description pointer; it kept the original
by-value callable symbol for existing function-pointer users and a separately
named descriptor-taking implementation for the pointer branch. Milestone 17
Goal 4 replaces that pointer branch with exact typed value dispatch in both
public and direct APIs. Recompilation against the SDK is required; binary
layout compatibility is not claimed for extended descriptions.

Do not silently overwrite a developer-edited `main.c`. Migrate only a recognized
untouched starter signature; otherwise preserve the file and provide migration
instructions/compatibility support. Creation, generation, starter migration, and
SDK acceptance remain separate goals; documenting them here does not execute them.

## Verification and goal boundaries

Goal 1 requires source/contract consistency, resolved ownership and binding
decisions, valid local links, and a roadmap with the full ordered checklist.
It changes documentation only; builds, sanitizer runs, GUI previews, and platform
execution are not evidence for this goal and are not claimed here.

Implementation goals verify their own changes before stopping. Goal 6 integrates
the following acceptance cases; it does not postpone relevant earlier checks:

| Area | Required evidence |
| --- | --- |
| Creation | Whole hierarchy versus selected member; defaults and explicit placement/owner; dependencies and invalid inputs; independent repeated instances; loaded catalog release after spawn; no external mutation after failure. |
| Visibility and layers | Runtime appearance changes, parent/child flags, later creation/deletion, aliased visuals drawn once, explicit layer ordering, no assertion of equal-layer stability. |
| Camera selection | Two instances from one template, explicit instance/override bindings, assignment during creation, missing/conflicting bindings, disabled/paused cameras, camera deletion and slot reuse. |
| Viewport mutation | Add a screen and interactive button after creation, change camera/layout/visibility, remove/replace items, and verify borrowed versus owned resource lifetime. |
| Composition | At least two differently placed/rotated/sized/layered screens, partial clipping, UI between/above screens, independent visibility, overlapping viewports, shared screens rendered once, one present/capture per batch. |
| Reporting and pacing | Standalone/full-frame/direct/public parity, no duplicate child counts, bad selections/recursion/backend failures, empty selection, late frames, VSync and focus/window switching. |
| Generation | Immutable catalog, deterministic output and round-trip property/reference parity; preserved developer files; generated and handwritten creation produce equivalent runtime state. |
| Lifecycle | 1,000 create/use/extend/destroy cycles with no sanitizer findings, accumulating live resources, or cross-instance interference; distinguish bounded shared layer caches from leaked ownership. |
| SDK | Installed generic Linux SDK consumers and affected independent examples run outside the source tree; Windows cross-builds. Native Windows execution is recorded separately, not implied. |
| User review | Actual editor/generated application review of rendering, live additions, controls, and the resulting creation/game-loop workflow. |

For performance, freeze equivalent appearance, entity counts, render dimensions,
camera selections, and physics settings before comparisons. Use the existing
[pacing procedure](frame_pacing.md#reproduction-and-verification) on AC power:
lightweight 600 frames, car 600 frames, and the 800-particle pit for 720 frames,
three serial complete runs per path. Add a two-viewport fixture exercising shared
and distinct screens. Report rendering CPU/work average and p95, total frame
intervals, limiter/VSync waits separately, and startup/creation cost separately.
Keep simulation work counts and known physics failures visible. The ten-second
car is a rendering diagnostic, not renewed physics acceptance.

Preserve the accepted 60 Hz pacing behavior on equivalent within-budget workloads.
Report measured overhead and variability rather than inferring improvement from
fewer API calls. No new numeric rendering-overhead allowance has been approved;
a reproducible material loss of the existing physics/rendering budget requires
user review before performance acceptance, not silently relaxed fixtures.

Exact private struct packing, bounded scratch-storage strategy, and internal
module split are implementation choices in Goals 2–4. They may not change the
public ownership, visibility, dependency, selection, or presentation rules above
without an explicit contract update and user direction where behavior changes.
