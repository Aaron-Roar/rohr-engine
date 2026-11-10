# Automatic viewports

Milestone 16 Goal 2 implements background-only standalone screens without required
runtime camera bindings, as defined by the [scoped creation contract](scoped_creation_contract.md).
Milestone 16 Goal 3 adds whole-project creation and its automatic camera wiring.

Milestone 15 Goal 3 implements complete viewport creation and automatic scene
rendering. Unified `project.h` generation is described in
[shared creation](shared_creation.md#unified-generated-project). By user direction,
Goal 4 also removes generated legacy viewport support and migrates starters and
simulation scenes. Customized entry points need the documented manual migration;
Goal 5 migrates every graphical bundled example to automatic rendering.
See the [contract](creation_and_viewport_contract.md) for the full milestone and
[shared creation](shared_creation.md) for entity/object creation.

Milestone 17 Goal 4 uses value-style viewport creation for both native descriptors
and named generated views. Rebuild the SDK and regenerate catalogs together;
replace old `create(&description)` calls with `create(description)`. Named
screen/UI resources can also use the [shared resource creators](shared_creation.md#shared-graphics-resource-creation).

## Create and show

Pass one `ViewportCreateDescription` to `rohr_viewport_create`. It contains scalar
viewport configuration, named screen/UI items, named layer definitions and runtime
camera bindings. For an immutable authored recipe, use a local descriptor whose
`.template` points to that recipe and whose binding arrays choose live instances:

```c
/* authored_viewport and player_description are immutable description data;
 * player is an already-created instance of player_description. */
ViewportObjectBinding binding = { &player_description, player };
ViewportCreateDescription creation = {
    .template = &authored_viewport,
    .objects = &binding,
    .object_count = 1,
};
ViewportIdResult created = rohr_viewport_create(creation);
if (rohr_error_check(created)) {
    /* Handle the creation error before using the result. */
    return;
}
ViewportId viewport = created.result.value;
/* In the application's frame loop, after its explicit updates: */
Report frame = rohr_graphics_viewport_show(viewport);
/* Check frame.header.status and diagnostics. */
```

The template supplies all configuration; use only `.template` and binding arrays
in the local descriptor. Template chains and bindings inside referenced templates
are rejected. Without a template, supply configuration and bindings together.
`graphics_viewport_description_default_get()` (public alias
`rohr_viewport_description_default_get()`) supplies scalar defaults. There is no
separate viewport options structure or `_with_options` call.

A screen item names its authored camera object and member. An object binding
resolves that member inside the chosen instance during creation. Alternatively,
`ViewportCameraBinding` overrides a named screen with exactly one camera entity
or low-level camera handle. Overrides take precedence. Duplicate, unconsumed,
stale, or incompatible explicit bindings fail; no camera is implicitly spawned.
An item without a runtime binding remains unbound even if its recipe names an
authored camera. It displays only its background, without scene content, fallback
cameras, warning stripes, or missing-camera diagnostics. Authored camera references
are still validated; a malformed reference is not treated as an empty screen.

For example, `rohr_viewport_create(project.initial_viewport)` creates the authored
layout without scene bindings. Use item lookup to retrieve a screen, then call
`rohr_screen_camera_set(screen, camera)` to connect it later. Passing
`CAMERA_INVALID` clears the binding. Invalid nonzero camera IDs are rejected by
this setter without replacing its current binding. A new binding invalidates
old screen content, so a paused camera starts with its background rather than
reusing another camera's image. UI remains available while screens are empty.

A camera destroyed after binding is different from an intentional empty screen:
its stale handle still produces the existing warning/placeholder until explicitly
cleared or rebound. Slot reuse never silently selects a replacement camera.

A bound screen draws the editor's diagonal backdrop underneath its camera image.
The backdrop follows screen placement and clipping; the image keeps its own fit,
scale, offset, rotation, and configured background color. Uncovered screen areas
therefore show the pattern instead of unrelated content underneath. This does not
turn an intentionally unbound screen into a missing-camera warning: unbound
screens still display only their configured background.

Instance matching uses copied catalog-root ID/name, member ID and retained kind,
then walks direct authored member names. It does not retain catalog pointers or
search global runtime names. This provenance is optional runtime persistence
data; older snapshots still load, but need explicit camera bindings if they lack
provenance. Independently authored catalogs must keep their namespaces distinct.

Use `rohr_graphics_viewports_show(ids, count)` for multiple viewports in one frame.
It composes in array order and presents/captures once. Shared screens render once;
distinct screens using one camera render separately at their own dimensions.
Camera motion advances once. Two singular show calls are two frames. Physics,
gameplay, audio and sprite animation updates remain application-owned.

## Live scene and extension

Automatic screens discover the current dense live-entity inventory on every
redraw. Retained body/particle appearance, soft-body element settings, markers,
sprites and animated sprites supply their existing draw data. A plain physics
hitbox does not become visible without appearance or a visual component. Attached
visual aliases are visited once with their actual entity. Low-level soft-body
elements have explicit visual settings and are eligible without retained object
appearance; their fallback color is white.

`rohr_entity_appearance_get/set` exposes the existing retained appearance, and
`rohr_entity_visible_check` includes containing-owner visibility. Hiding an owner
does not rewrite child flags or disable physics. Component visibility and graphics
layers still apply. Lower layers draw first; equal layers use discovery order
without stability guarantees. Use distinct layers when overlap order matters.
Each screen clears to its configured background before drawing, preventing trails.
Camera entity visibility suppresses its content; an already-rendered paused camera
keeps its content, while a new paused screen initializes only its background.

Use `rohr_viewport_item_by_name_get` for an authored item, then the existing item
get/set/remove APIs. Existing camera/screen/UI add calls extend the live viewport.
Screens and UI retain viewport-item transforms, clipping and layer order; at equal
item layers screens precede UI. Later-created scene entities need no registration
with the viewport. The test's `--preview` mode demonstrates two screens and a
runtime-added Toggle button controlling entity visibility.
The `--preview-unbound` mode cycles between empty screens, both cameras bound,
cleared screens, and only the inset bound, while keeping UI available.

Automatic scenes also honor the global AABB/contact debug toggles. Optional
`rohr_graphics_hit_boxes_debug_set`, `rohr_graphics_particles_debug_set`, and
`rohr_graphics_local_origins_debug_set` enable the corresponding diagnostic
overlays without a callback; matching `_check` functions query them. They default
to false on graphics startup. These overlays draw above ordinary scene content
at layer 100 and retain their diagnostic eligibility rules. They do not change
physics or authored appearance. The fly examples use them for their timed
debug phases.

The [bundled game workflow](../examples/reusable_instances.md) shows shared
creation, explicit instance binding, a frame loop, runtime UI and SDK setup.

## Ownership and compatibility

- Creation borrows input arrays, strings and descriptions only for the call. It
  copies runtime values and names, acquires assets and rolls back failed creation.
- The returned viewport owns its recipe-created screens/UI/text/fonts and screens
  created by its camera-add convenience helper. Recipe-created UI owns its text,
  which holds its font dependency. Removing their items unmounts
  them; resource lifetime lasts until viewport destruction. Destruction unmounts
  borrowed uses of those owned resources elsewhere before releasing them.
- Cameras and object instances are borrowed. Deleting one leaves a safe missing
  reference; slot reuse cannot bind a replacement implicitly. Binding a low-level
  camera directly clears any prior camera-entity association.
- Separately created screens/UI mounted through low-level add calls are borrowed.
  Their callers retain ownership. Existing legacy scalar viewport camera-add
  behavior remains graphics-lifetime ownership.
- Named layers are reused only with matching values. Failed creation removes new
  registrations; successful shared registrations last until explicit destruction
  or graphics teardown. Repeated creation does not accumulate layers.
- Legacy by-value `rohr_viewport_create(ViewportDescription)` and its callable
  symbol remain available through a C11 dispatch adapter. Direct `graphics_`
  functions have equivalent behavior. Rebuild consumers for extended structures.

Low-level screens default to `SCREEN_CONTENT_CALLBACK`. Recipe-created screens
use `SCREEN_CONTENT_AUTOMATIC`, ignoring any callback on their camera. A camera
can serve both modes through separate screens. Callback screens may opt into a
configured background clear with `clear_background`; automatic screens always
clear on redraw. Project JSON, editor camera previews and current generated
callbacks preserve screen background colors. Older project files use `0x12161eff`,
the previous generated background. Editor camera previews retain their owning
object restriction, while runtime automatic screens discover the runtime scene.

Invalid selections fail without consuming queued work. Empty/all-disabled
selections clear the window and still present. Full-frame selection discards
commands outside its visible screens; manual command execution remains explicit.
See [graphics reports](reports.md#graphics-reports) for diagnostics and stage
boundaries. Frame limiting, VSync, recording and hosted capture use the existing
frame pipeline.

## Verification and remaining scope

The focused `automatic_viewports` fixture covers two instances, explicit camera
overrides, scene visibility/layers, late-created entities, alias drawing,
shared screens, selection failures, manual stages, legacy callback recursion,
runtime interaction/extension, removal, allocation rollback, retained text/font
counts, camera deletion/slot reuse, saved instance bindings, paused initialization,
engine/graphics restart and 1,000 create/extend/destroy cycles.

Goal 3 evidence is kept in ignored `sandbox/milestone_15/goal_3/`:

- Thirteen focused Linux Release tests and the same thirteen ASan/UBSan tests.
- Six selected installed Linux SDK consumers; all consumer targets compile on
  Linux and cross-compile for Windows, as do independent car, flies and viewport
  examples. The generated starter builds and launches outside the source tree;
  flies and viewport examples also launch there and stop cleanly on interrupt.
- Windows engine/editor/test cross-builds; native Windows execution is unverified.
- The user accepted the installed-SDK two-screen interactive preview, including
  moving content without trails and the runtime Toggle button.
- The 600-frame headless car diagnostic retains 2,400 substeps, 701,387 candidates
  and 108,585 contacts. This is neither a complete 60-second physics run nor a
  rendered performance acceptance result. Existing joints/forces failures remain
  in the Goal 2 inventory.

Goal 5 migrates all twelve graphical examples and leaves the serialization
example headless. Eleven focused checks pass in both Release and ASan/UBSan.
All thirteen examples build as independent installed-SDK consumers on Linux
and for Windows; Linux Release and sanitizer launches from outside the source
tree pass, and the user accepted the graphical previews. Regeneration reproduces
the three authored catalogs without changing customized main/CMake files. The
car's short diagnostic retains the counts above. Evidence and scripts are under
`sandbox/milestone_15/goal_5/`; disposable consumer builds can be recreated.
Disk-full build/install attempts were rerun after temporary build cleanup.
Pong's unchanged component generator emits one unused-helper warning under the
extra warning flags; no new warnings or sanitizer findings remain. Native
Windows execution is still unverified.

Discovery adds no allocation and scans live entities once per redrawn automatic
screen; drawing retains existing command/resource costs. Creation allocates render
targets and loads requested fonts/text. No rendering speedup is claimed from the
shorter application API. Goal 6 completes the controlled three-run lightweight/
car/pit and multi-viewport comparisons, generic Linux packaging and integrated
generated workflow review. It also removes invisible zero-alpha outline work.
See the [integrated verification report](creation_rendering_verification.md) for
measured overhead, platform results, preserved failures and user acceptance.
