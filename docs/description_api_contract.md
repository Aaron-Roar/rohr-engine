# Description API Contract and Migration Inventory

Status: **Consistent Description APIs is complete (Goals 1–12).**
Entity metadata, lifetime, authored configuration, spawning, and physics
configuration, particle, soft-body, anchor, asset-loading, text, sprite,
animation-instance, audio, logical input, camera, attachment, window presentation,
screen, viewport, layer, and persistent UI descriptions are shipped. The symbol
inventory records the headers at the completed milestone boundary. The permanent
audit and installed-SDK checks keep future changes reviewable.

## Scope and naming

The user narrowed this milestone to **creation and full configuration set/get**.
Focused setters and temporary operations stay direct. This supersedes the earlier
14-goal proposal: the drawing and immediate-mode UI conversion goals are removed;
final verification is now Goal 12. The first eleven goal numbers retain their
order, with operation-only work removed from their scope.

Use `ThingDescription` for a complete configurable value and
`thing_description_set/get` for its replacement and retrieval. Keep the existing
`add`, `create`, or `load` verb appropriate to its lifecycle. Provide
`thing_description_default_get` when useful defaults differ from zero. Defaults
must preserve the existing subsystem's documented behavior; zero initialization
is not a universal request for defaults. Use typed result wrappers consistent
with the engine's error conventions.

The target entity/resource handle, frame/item index, or other selector stays
outside the description. Named fields replace independent configuration
arguments. Scalars, vectors, colors, rectangles, ranges, shapes, lists, and a
single referenced handle remain ordinary values. A pointer and its count form
one list, not two independent settings. A callback and its context likewise
remain a focused callback binding. Do not wrap a focused setter just because
its value has several C fields or the signature contains several arguments.

Full configuration APIs complement focused setters. A whole collision filter,
for example, is a configuration description; setting its category alone remains
a direct setter. Existing force location/orientation values and beam endpoint
setters remain focused interfaces. Full descriptions include these values without
requiring their focused interfaces to be renamed.

Direct module functions, `rohr_` wrappers, and facade macro aliases must agree.
Existing facade differences such as `rohr_camera_*` versus `graphics_camera_*`
must be accounted for explicitly. This milestone does not require inventing a
wrapper for every internal declaration exposed by an installed header.

## Configuration, state, and ownership

C designated initializers produce zero/false for omitted fields. Full setters
replace configuration; they are **not patches** and have no field-presence masks.
Use get-modify-set or focused setters to preserve fields. Apply documented defaults
only where explicitly specified; invalid replacements preserve the old state.

- Creation copies configuration and initializes new state. Complete replacement
  validates and prepares dependencies before publishing any mutation. Failure
  preserves configuration, lifetime ownership, connections, and live state.
- A getter returns the current configurable values, not stale creation inputs.
  Setting an unchanged description does not reset progress. A changed field has
  the same intended effect as changing that field through its focused interface;
  do not blindly replay unchanged setters with side effects.
- Descriptions exclude solver caches, playback clocks, input transition history,
  native backend pointers, and reference counts. Keep runtime-state queries
  separate. Position/velocity that are explicitly configurable remain writable;
  playback progress is not implicitly restored from a creation description.
- Preserve unrelated simulation, attachment, input, and playback state. A
  compatible volume, scale, material, or appearance edit does not restart its
  subsystem. A changed audio source is prepared first and then installed stopped.
  An incompatible animation replacement initializes its first frame. Changed
  input type/mode reinitializes only incompatible state without fabricating
  transitions. Explicit play, stop, seek, and reset remain separate operations.
- Shared textures, fonts, and animation definitions remain immutable. Loading
  another definition creates or shares another resource; no full setter mutates
  a catalog for all users. Their inspection APIs report immutable metadata.
- Replacement acquires new resource dependencies before releasing old ones and
  undoes acquisitions on failure. Plain C copies do not retain handles. Getter
  output never silently acquires owning references. Copy bounded values where
  supported; document any borrowed strings/arrays/handles and exactly which
  mutation, release, or destruction invalidates them.
- Ownership remains independent of connections. A full description replaces
  ownership only where that configuration explicitly includes an owner. Focused
  setters preserve ownership. Do not infer ownership from joint endpoints,
  source targets, anchor attachments, or camera references.
- Keep world/local spaces, clockwise degree conventions, explicit radians
  alternatives, and automatic-versus-explicit rest values unambiguous. Do not
  create a description whose angle fields silently change units between calls;
  radians conveniences convert at the boundary into canonical descriptions.

## Compatibility and verification

Each implementation goal migrates its direct API, wrappers/aliases, caller code,
generated C, editor/CLI paths, SDK consumers, documentation, and affected
examples together. Remove replaced names/signatures without compatibility
wrappers. Renaming a referenced type in a later goal also updates earlier
migrated users of that type; it does not move ownership of their API family.

Preserve existing saved formats and behavior unless a concrete incompatibility
requires an explicit migration decision. Runtime configuration replacement does
not imply automatic rebuilding of a whole object hierarchy. Keep dot-catalog
spawning and explicit child topology intact. Customized project entry points
remain developer-owned; automatic migration requires an exact recognized
starter template. The explicitly selected external example is migrated
intentionally, preserving its inline main and two-second force toggling.

For each code goal, verify creation/getter round trips, get-modify-set and focused
setter equivalence, unchanged-state preservation, stale handles, invalid input,
allocation/resource failure rollback, and relevant ownership or topology rules.
Run affected native and sanitizer tests, Linux builds, Windows cross-builds,
independent installed-SDK consumers, and affected generated examples launched
outside their source directories. Request desktop review for visual/interaction
changes; never describe cross-compilation as native Windows execution.

Goal 1 is documentation-only: header coverage, naming, links, goal assignments,
and diff checks suffice. No runtime build, sanitizer, or platform result is
claimed for this goal.

Existing requirements remain authoritative for their domains:
[resource ownership](agent_instructions/assets_and_fonts.md),
[generated projects](agent_instructions/generated_projects.md),
[standalone particles](agent_instructions/standalone_particles.md),
[viewport composition](agent_instructions/viewports.md),
[editor interactions](agent_instructions/editor_interactions.md),
[reporting](agent_instructions/system_reporting.md),
[testing](agent_instructions/testing.md), and
[angle conventions](angle_convention_contract.md).

## Implemented configuration families

The following families use the implemented description interfaces. For a
listed description, “full set/get” means the appropriate existing family prefix
plus `_description_set/get`, with matching typed results and default getter when
needed. Retain the existing family prefix in both direct and facade APIs; this
is not a wholesale namespace rename. Exact current symbols, including aliases,
appear in the header inventory below.

| Family / owning goal | Current configuration types | Implemented interface | Preservation requirements |
| --- | --- | --- | --- |
| Entity metadata and lifetime / 2 — shipped | `EntityDescription`, `EntityLifetimeDescription`; `LifeTime` remains storage | `entity_description_add/set/get` and `entity_lifetime_description_set/get`; retain bare `entity_add()` and focused name/owner/removal APIs. | Copied bounded names; atomic ownership/name replacement; explicit deadline enable flags; no implicit ownership from connections. |
| Authored objects / 2 — shipped | `ObjectDescription`, `ObjectSpawnDescription`, `ObjectMaterialDescription`, `ObjectCollisionDescription`, `ObjectColorDescription` | `object_*_description_set/get`, `object_spawn_description_default_get`, and existing dot catalogs. Force/torque descriptions remain compliant. | Immutable getters; setters require editable copies and affect future spawns. Color includes explicit override state. No live hierarchy replacement setter. |
| Physics configuration / 3 — shipped | `TransformLockDescription`, `AxisLockDescription`, `CollisionFilterDescription`; solver lock storage remains separate | Full lock set/get, explicit removal, and collision-filter full/default accessors. Nested filter types use the common description. | Degrees only in descriptions; explicit conversions for radians. Lock edits preserve current motion, ownership, and parenting; capture-current-transform stays direct. Missing locks return component-missing. |
| Particles / 4 — shipped | `ParticleDescription` | Standalone creation and full `physics_particle_description_set/get`, with the existing default getter. | Current configurable state and explicit owner replacement; atomic circle/polygon synchronization; static configuration retained; hybrid bodies and soft nodes use their own APIs. |
| Soft bodies and anchors / 5 — shipped | `SoftBodyDescription`, `SoftBodyNodeDescription`, `SoftBodyBeamDescription`, `SoftBodyAreaDescription`, `JointAnchorDescription`, `SoftBodyBeamCollisionDescription` | Full set/get for all five families, including current motion, materials, collision, and existing visuals; use `SoftBodyBeamCollisionDescription` for the full collision group. Reuse `CollisionFilterDescription` for node/beam full filters. | Keep world/local creation variants explicit. Container edits do not reconstruct children; owner changes preserve connections. Beam getters preserve resolved rest values; automatic measurement happens only when requested. Beam collision defaults off and never derives settings from endpoints. Anchor replacement updates references transactionally without implicit reownership of joints. |
| Shared assets and text / 6 — shipped | `TextureDescription`, `FontDescription`, `AnimationDescription`, `TextDescription` | Loading/default/inspection for immutable assets; full text create/set/get. Animation uses `frame_count`. | Sharing and borrowed metadata; unique text ownership; atomic replacement; queued payloads retain their exact font revision. |
| Sprites, frames, players / 7 — shipped | `SpriteDescription`, `AnimatedSpriteDescription`, `AnimationFrameDescription`, `AnimationPlayerDescription` | Description creation/default/full accessors, plus catalog-to-instance helpers. `Sprite` is the stored configuration alias; `AnimatedSprite` and `AnimationPlayer` retain separate runtime state. | Atomic image/catalog replacement; borrowed getters; stable frame IDs preserve compatible progress. Frame index and clocks remain outside descriptions. |
| Audio / 8 — shipped | `SoundDescription`, `MusicDescription` | Description creation, muted defaults, and `audio_sound_description_set/get`, `audio_music_description_set/get`. | Full replacement is one queued command; unchanged sources preserve progress. Prepared source changes keep handles and install stopped. Getters borrow applied paths; focused controls remain direct. |
| Logical input / 9 — shipped | `InputControllerDescription`, `InputActionDescription`; copied `InputBindingList` defaults | Creation and full `_description_set/get`, plus default getters. Focused binding/state and text-input operations stay direct. | Stable handles and validated controller moves; compatible overrides survive. Incompatible edits reject atomically. No-op and rename preserve state/events; rebinding preserves persistent state; type/mode/initial changes reinitialize only the edited action. |
| Cameras and presentation / 10 — shipped | `CameraDescription`, `CameraAttachmentDescription`, `WindowPresentationDescription`; resolved `Camera` query retained | Creation and full/default accessors, including callback/context and enable/pause flags. Attachments have separate full accessors. | World edits update offsets and preserve attachment; timed movement interpolates offsets while following the target. Only explicit detach breaks the reference. Unrelated motion survives. Presentation validates before applying and attempts backend rollback on failure. |
| Screens, viewports, layers, persistent UI / 11 — shipped | `ScreenDescription`, `ViewportDescription`, `ViewportItemDescription`, `ViewportUiShapeDescription`, `ViewportUiTextDescription`, `ViewportUiSliderDescription`, `LayerDescription` | Creation and default/full accessors; item descriptions include screen/UI references and named-layer bindings. | Retain handles, children, clipping/composition, resource dependencies, and compatible interaction state. Content replacement acquires before release. Automatically created UI survives until its last mount is removed. |
| Final verification / 12 — complete | All migrated families | Permanent header inventory audit and native installed-SDK execution; no new configuration family. | Verified generated/example parity and preserved the external explicit main and two-second force toggling. |

Bare entity/group allocation and simple key/scancode binding builders remain direct. Likewise, the soft-node pin
helper already takes `SoftBodyNodeAnchorPinDescription`; it creates a combination
of resources, not a new resource kind needing an artificial full setter.

Dependencies follow the goal order: Goal 3 migrated the shared filter type,
including Goal 2's authored collision description; Goals 4/5 use that contract.
Goal 7 consumes Goal 6's asset contracts; Goals 10/11 consume the resource/text
work and Goal 11 follows
camera configuration. Every implementation goal depends on Goal 1. Goal 12
requires all preceding goals and does not defer their own necessary verification.

## Shipped Goal 2 behavior

`EntityDescription` contains a copied `char name[ENTITY_NAME_MAX]` and `Entity owner`.
Creation generates a unique name only when `name` is empty; a supplied name must
be unique. Full replacement requires a nonempty terminated name and rejects
oversized/duplicate names, dead owners, ownership cycles, and incompatible
soft-body membership. Validation and ownership preparation precede publication;
allocation failure leaves the original name and ownership intact. Getters copy
names. Parenting, components, connections, and live physics state are excluded.
Existing scoped member identities survive renaming and ownership transfers.
On first adoption the final metadata name becomes the member identity.

`EntityLifetimeDescription` contains absolute `expiry_time`/`expiry_tick` and
`time_enabled`/`tick_enabled`. Time must be finite and nonnegative, even when
disabled. Disabled deadline values canonicalize to zero; both flags false removes
the configuration. A live entity without lifetime data returns disabled zeros.
Enabled zero or past deadlines expire on the next lifecycle update, never in the
setter. Either enabled deadline can trigger expiry. Saved formats still store
remaining durations and explicit flags, including deadlines due at clock zero;
legacy saves infer flags from nonzero deadlines. Internal `LifeTime` storage and
its historical spelling remain separate from the public description.

Material and collision descriptions support physics bodies, particles, nodes,
and beams. Getters work on generated catalogs, loaded immutable catalogs, and
editable copies; setters require registered editable copies. Friction must be
finite/nonnegative and restitution finite in [0, 1]. Collision contains `enabled`
and `CollisionFilterDescription filter` (renamed in Goal 3). Color contains
`color`, `border_color`, and `overridden`; setting `overridden = false` restores
inherited style without discarding the stored colors. These edits change future
spawns only. `ObjectSpawnDescription` contains inclusion `flags` and world `offset`;
its default getter returns ALL at zero offset. Zero/unknown flags are invalid.
Existing `_with_options` dot-catalog convenience macros retain their names.

Removed positional APIs: `entity_life_time_set`, `object_material_set`,
`object_collision_set`, `object_color_set`, and their facade spellings.
`ObjectSpawnOptions` is replaced by `ObjectSpawnDescription` without an alias.

## Shipped Goal 3 behavior

`AxisLockDescription` contains `axis` and `point_on_axis` in world coordinates.
`physics_axis_lock_description_set/get` replaces or copies that configuration.
The axis must be finite and nonzero; normalization uses double precision so tiny
and large finite float vectors remain valid directions. The world point must
satisfy the existing coordinate bounds. Getters return the stored normalized axis.
`physics_axis_lock_remove` clears both component and storage and is idempotent
for a live entity.

`TransformLockDescription` contains `driver`, driver-local `local_offset`, clockwise
**degree** `local_angle`, `lock_position`, `lock_orientation`, and `inherit_velocity`.
`physics_transform_lock_description_set/get` replaces or copies configuration.
Every replacement requires a live driver, finite offsets, and a finite angle.
Omitted booleans become false and omitted offsets/angle become zero; all flags
false is a stored inactive configuration, not a removal request. Negative angles
and multiple revolutions remain intact. Use `math_radians_to_degrees` for incoming
radians and `math_degrees_to_radians` when radians are needed by caller code.
The positional radians transform-lock setter was removed; other scalar radians
conveniences are outside this goal. `physics_transform_lock_current_transform_set`
remains a direct capture operation, and `physics_transform_lock_remove` remains
explicit. Reference checking and solver ordering otherwise retain their behavior.

Both lock getters return `ERROR_ENGINE_COMPONENT_MISSING` when absent and reject
dead target handles. A stored transform-lock driver may become stale after its
entity is deleted: inspection still returns that handle, while replacing that
description rejects it until repaired. Setters prepare validated values before
publishing storage/component changes, require no new allocation, and do not move
entities, reset motion, reparent, or change ownership. Constraints apply during
the existing physics update. Save fields are unchanged; loading uses validated
setters. `AxisLock` and `TransformLock` remain solver/snapshot storage types.

`CollisionFilterDescription` replaces `CollisionFilterConfig`, including typed
results, pools, particle configuration, authored objects, beams, and inspection.
`physics_collision_filter_description_set/get` replace and inspect configuration;
`physics_collision_filter_description_default_get` supplies defaults. A live entity with no filter returns DEFAULT/ALL without adding a
component. An explicitly zero filter remains valid and permits no collisions.
Focused category/allowed-mask setters retain their names and preserve the other
field. Soft-node/beam specialized filter descriptions are shipped in Goal 5.

All new APIs have equivalent `rohr_` wrappers. Removed positional lock/filter
names and old filter types have no compatibility aliases.

## Shipped Goal 4 behavior

`physics_particle_add` and `physics_particle_description_set/get` share
`ParticleDescription`. Full accessors accept standalone particles only; ordinary
entities, hybrid rigid bodies with particle properties, and soft-body nodes return
component-missing. A stale handle returns an invalid-entity error. The getter
copies current position, velocity, acceleration, owner, material, filter, flags,
and circle geometry; it does not replay initial creation values. Get-modify-set
preserves unrelated configuration:

```c
ParticleDescriptionResult current = rohr_physics_particle_description_get(particle);
if(!rohr_error_check(current)) {
    current.result.value.radius = 4;
    EngineResult changed = rohr_physics_particle_description_set(particle, current.result.value);
    /* Handle changed before continuing. */
}
```

Replacement validates all values and prepares geometry/allocation before an
ownership transfer. Any failure leaves configuration, ownership, geometry, and
variant IDs unchanged. It does not reset names, parents, children, connections,
lifetimes, hold state, explicit kinematic mode, or visual resources. Position,
velocity, and acceleration are current configurable values and are replaced when
supplied; omitted fields become zero/false. Radius must be positive. A zero
`rigid_vertices` selects the established engine default; getters return the active
polygon's vertex count (the default when no polygon exists).

Creation and replacement store mass, velocity, acceleration, and the gravity flag
for static particles too. Static bodies remain excluded from dynamic simulation;
changing back to dynamic can reuse these stored settings. Saved formats already
carry these fields and need no version change. Older static saves lacking these
values still load; missing stored mass/motion is reported as zero. Mass zero on a
dynamic particle retains the engine's existing kinematic behavior.

Changing radius, local origin, or polygon quality rebuilds the active circular
polygon and particle-circle metadata together. Focused radius/origin setters do
the same for standalone particles, while preserving their other configuration.
The active hitbox ID, inactive variants, and animation bindings are retained.
Unchanged geometry is not rebuilt; a removed active polygon is recreated on the
next geometry update. Generic hitbox editing/activation remains explicit and
outside these particle-specific operations. Hybrid-body and node focused setters
retain their existing behavior. Polygon validation and coordinate bounds apply
before commit, including for focused setters. Standalone particles retain their
existing translation-only solver behavior and automatic COM at the local origin.

All new APIs have matching `rohr_` wrappers. Creation/default/focused function
names and the existing `ParticleDescription` fields remain source compatible.

## Shipped Goal 5 behavior

Containers, nodes, beams, areas, and anchors now expose copied full descriptions
and atomic typed set/get, with matching direct and `rohr_` interfaces. Existing
runtime-state getters remain distinct. Default getters supply standard values for
containers, nodes, beams, beam collision, and areas. Omitted literal fields are
zero/false; zero node mass/radius is invalid at creation. Node full accessors use world positions;
local creation and focused local-position calls retain their explicit convention.
Container orientation is clockwise degrees. Transform edits move existing nodes
and rotate their motion, preserving child topology, rest values, and handles.
Unchanged descriptions do not rebase the observed soft-body frame.

Nodes include motion, material/filter, static/gravity/collision flags, and color
overrides. Static settings survive later dynamic transitions and saves. Creation requires
positive node mass; full replacement also accepts zero mass established through
focused setters, preserving the existing kinematic behavior. Their
circle radius remains independent of the editable polygon, matching focused node
radius behavior. Area descriptions copy geometry and include color overrides and
visibility. Unchanged geometry reuses its cache, including temporarily deformed
areas; sibling order remains a focused operation. Ownership transfers preserve
connections and update immediate membership lists transactionally.

**Beam collision defaults off**, per the clarified user requirement. There is no
`collision_explicit` flag and no endpoint-derived initialization. Width, filter,
and enabled state are supplied directly; endpoint and node edits leave them alone.
Enabled collision requires the fixed engine minimum width, with no endpoint-radius
cap. Disabled collision permits width zero. Default getters provide the fixed
minimum width and standard filter, still disabled. Existing authored settings and
collision-test scenes explicitly opt in. `SoftBodyBeamCollisionDescription` and
`*_collision_description_set/get/default_get` replace the former config interface;
node/beam full filters now use `CollisionFilterDescription` and matching description
set/get. Focused enable/disable/thickness operations remain direct. Beam rest length
uses the existing joint convention: measure once unless explicit; getters return
resolved values marked explicit, including zero for disconnected measurement.

Anchor replacement preserves the handle and updates every referencing joint to
the new live entity with a new local offset. It checks capacity and coordinates
before mutation; joint owners/rest values are preserved. Authored anchor metadata
and persistence resolve the current attachment instead of a stale initial target.
Full replacement failures preserve ownership, geometry, references, and live state.
Existing save fields cover these configurations; no file-version change is needed.

## Shipped Goal 6 behavior

Texture, font, and animation loaders use `TextureDescription`, `FontDescription`,
and `AnimationDescription`. Each family has `_description_default_get` and a typed
`_description_get`; shared resources have no mutation setter. The old Descriptor
names are removed. Animation's `amount_of_descriptors` is now `frame_count`.
Zero-frame shared catalogs are valid and contain no drawable frames, matching
empty authored/instance animations. Stable IDs normalize as before and timing
remains unchanged.

Texture inspection returns the normalized resolved path and the supplied asset's
logical size; GPU sharing still depends only on the path. Font inspection returns
the normalized path and point size. The built-in font returns `{0}`; loading that
description selects the pinned font without acquiring a custom-font reference.
Custom files still require a finite positive point size. Animation inspection
copies ordered frame arrays and normalized IDs/timing; file pointers borrow the
paths held by its frame textures. Asset inspection never retains or allocates.
Borrowed paths remain valid while the corresponding resource has an owning
reference, ending at final release or graphics shutdown. Getter results may be
used to reload/share the same resource while that lifetime holds.

`graphics_text_create` now takes `TextDescription` containing a font handle, UTF-8
value, color, and wrap width. A zero/omitted font selects the built-in engine font;
a nonzero stale font is rejected. The value must be non-null; `""` is valid empty
text with zero size and no payload. The default description provides an empty
string, white color, no wrapping, and the zero font selector. A literal's omitted
color remains transparent zero. Wrap width is a nonnegative logical pixel count;
zero disables automatic wrapping. Built-in text supports word wrapping, hard
breaks for long words, explicit newlines/CRLF, four-space tabs, and one fallback
glyph per unsupported codepoint. It still uses the engine's fixed 8-pixel ASCII
font. A positive width smaller than one glyph displays one glyph per line.

Full `_description_set(TextAsset *, TextDescription)` preserves the handle and
persistent-UI dependencies, updates dimensions, and prepares replacement storage
and rendering before publishing. Failure leaves the original configuration,
size, and dependencies intact. Unchanged get-modify-set does not create a new
payload. Focused value/wrap setters remain direct and use the same transaction.
The getter returns the resolved current font and a borrowed string, with no new
ownership. Treat both as borrowed until a successful changed configuration,
public text destruction, or graphics shutdown; explicitly retain a custom font
or copy a string to keep it independently.

Every rendered payload owns its font dependency until its final queued draw is
executed/discarded. Empty text owns the font reference directly. Changing fonts
therefore preserves all earlier queued revisions, even after their sole public
text owner has been destroyed. Teardown remains UI, then text, then fonts.
All new functions have matching `rohr_` wrappers. Callers and generated code use
the new types/signatures; custom application entry points remain user-owned.

## Shipped Goal 7 behavior

Static and animated component addition takes `SpriteDescription` and
`AnimatedSpriteDescription`. Their `_description_set` requires an existing
component; `_add` can create or replace one. Full replacements validate before
retaining dependencies and acquire every new reference before releasing old
ones. Failure preserves the component, playback, and ownership. Names, entity
owners, transforms, layer bindings, and other components are untouched.
`Sprite` remains an alias for stored static configuration, since it has no
separate playback state.

`graphics_animated_sprite_description_from_animation_get` copies catalog frames
and timing into a description with scale one, facing right, visibility on, and
rotation following on. Its frames and configuration are independent copies;
images and catalog handles are borrowed. Zero handles yield empty defaults and
stale nonzero handles fail. The full description's frame list is authoritative:
zero frames means empty even with a nonzero catalog. The catalog preserves
identity for bindings and inspection; it never implicitly restores frames.

```c
AnimatedSpriteDescriptionResult prepared =
    rohr_graphics_animated_sprite_description_from_animation_get(animation);
if(!rohr_error_check(prepared)) {
    prepared.result.value.scale = (Scale){2, 2};
    if(prepared.result.value.frame_count > 0)
        prepared.result.value.frames[0].rotation = 45;
    EngineResult added = rohr_graphics_animated_sprite_add(entity, prepared.result.value);
    /* Handle added before continuing. */
}
```

`AnimationFrameDescription` replaces `AnimationFrame`. Catalog and entity frame
inspection use `_frame_description_get`; full frame mutation uses
`_frame_description_set`. Focused scale, offset, rotation, and frame-index APIs
remain direct. Frames draw using native texture dimensions, signed scale, and
clockwise degrees; zero scale collapses content. Full list replacement normalizes
zero IDs to one-based indices and rejects duplicates. Focused frame addition
assigns an unused ID; focused frame replacement with ID zero preserves that
slot's current ID. Changing the current ID is equivalent to removing that frame.

Appearance edits, image replacement, and timing changes preserve the current
stable frame ID and elapsed tick/time clocks. Reordering moves the current index
to the matching ID. Removing that ID or changing catalog handles resets to frame
zero and fresh clocks. Entity resets use current engine clocks; standalone
players/values use their last observed update clocks. A duration that is already
exceeded advances on the next update. The existing either-tick-or-time threshold
behavior is unchanged. A zero timing value disables its clock; two zero values
stop automatic stepping without implicitly clearing progress.

`AnimationPlayerDescription` contains only catalog and tick/time durations.
Standalone players derive frame count from their immutable catalog. Instance
players derive it from their explicit frame list. Indices, last-advance clocks,
and observed update clocks remain in runtime state. `graphics_animation_player_create`
and `graphics_animated_sprite_create` return typed results and create validated,
non-owning values. Their owners must keep assets alive; adding an instance to an
entity acquires its own references. Value description setters are only for
standalone values, never installed components. Static sprite/frame builders
simply copy descriptions (frames resolve native image size); add/set performs
validation. Default getters supply useful scale/visibility/direction values;
omitted literal fields stay zero/false. Creating a value does not retain assets.

Description getters copy configuration but do not retain images or catalogs.
Keep the original owner alive and unchanged until finished using borrowed
handles, or explicitly retain independent references. Catalog inspection remains
immutable. Authored starting-frame selection happens after creation through the
focused index setter. Runtime loading restores saved indices/elapsed clocks
explicitly after applying configuration, preserving existing save formats.
All changed APIs have matching `rohr_` wrappers; old positional signatures and
old frame type/accessor names are removed.

## Shipped Goal 8 behavior

`SoundDescription` and `MusicDescription` replace the old configuration type
names. Creation accepts these descriptions. Defaults are **volume zero (muted)**,
playback rate one, looping off, and centered sound pan. Master mixer volume stays
one. Explicit volume enables sound without changing playback rate or play state.
Omitted literal values remain zero/false; zero rate freezes an armed player.
Full replacements use existing focused normalization for volume, pan, and rate.

Full `_description_set` prepares a source on the calling thread and enqueues one
atomic command. Call `audio_update()` to submit it. Getters return applied state;
request acceptance does not mean the callback has executed it. Ordinary edits
preserve cursor, playing state, and explicit music pause. A changed source keeps
the same handle but installs stopped at its beginning, with music pause cleared.
Replacing an inactive track does not stop another active track. A later queued
play starts the replacement normally; same-source edits queued after it preserve
that play. Prior queued descriptions determine source identity at FIFO execution.

Paths are required, resolved, and copied before return. Same resolved paths reuse
the current/queued source and do not reload changed file contents. Failed file
loading, allocation, or queue acceptance preserves the existing configuration and
playback, and leaves previously accepted requests intact. Playback may continue
naturally while preparation runs. A target destroyed before application produces
a report warning; stale commands cannot affect a reused slot.

Description getters copy numeric values and borrow the applied resolved path.
That path is usable until a subsequent update, creation, destruction, full setter,
or shutdown; coordinate these operations across threads. Get-modify-set directly
is supported: setters resolve/copy their input before reclaiming retired paths.
Copy paths into caller storage if retaining them across those operations.

The existing 256-request limit includes each full description as one request.
Prepared and retired owners use a bounded array of 256 slots. The sound cache
allows 128 live-player sources plus 256 prepared sources; replacement does not
need a spare public player or track handle. The callback swaps owners and marks
retired slots without loading, allocating, freeing, or closing sources. Updates
and subsequent preparation/creation reclaim completed owners on the caller;
shutdown also releases cancelled requests. Existing callback decoding and report
collection semantics remain unchanged. Matching `rohr_` wrappers, the audible
example, installed-SDK checks, and API references use the new names.

## Shipped Goal 9 behavior

Controller and action creation now take `InputControllerDescription` and
`InputActionDescription`. Matching direct and `rohr_` full setters/getters and
default getters are available. Names and default bindings are copied inline;
getters return independent values with no borrowed storage or hidden allocation.
Controller names are globally unique; action names are unique within their owner.
Default getters select an enabled controller and a momentary Button with false
initial state and no bindings. Fill the required name and action owner before
creation. Literal omitted fields are zero/false, including controller enabled.

Full setters validate names, owners, types, binding capacity, binding validity,
and binding-name uniqueness before publishing anything. Actions may move between
controllers without changing their handle or consuming a spare slot. Destroying
the former controller leaves a moved action alive; destroying its current owner
invalidates it. Destination name collisions reject the entire edit.

Descriptions contain authored defaults, never runtime overrides or current
button state. Compatible overrides survive replacement, including active empty
lists. A type change incompatible with an active override rejects the entire
edit. Clear or replace the override explicitly before retrying. Default edits
hidden by an active override do not change effective input or pending events.

Unchanged descriptions and renames preserve persistent state and pending
pressed/released events. Binding-name and unused axis-value changes also preserve
Button events. Changed effective digital bindings or owner resample the affected
action and clear its pending transitions without synthetic presses/releases;
persistent state survives, while momentary state follows the current snapshot.
Changed type, button mode, or initial state reinitializes only that action. A held
key seeds the new physical baseline so it cannot cause an unintended toggle on
an unrelated event. Axis actions normalize unused button settings to
momentary/false. Controller enable changes retain the established focused enable
behavior; controller renames and unchanged descriptions do not clear child events.

Generated controllers/actions now create their complete authored configuration
in one call. Examples and installed-SDK tests use the description signatures.
Focused overrides, binding setters, button-state operations, raw input queries,
and text-input operations remain direct, with their existing semantics.

## Shipped Goal 10 behavior

`CameraDescription` replaces creation-only `CameraConfig` and the old whole-camera
setter. It contains world position/orientation, dimensions, zoom, enabled,
`pause_with_engine`, `render_callback`, and `render_context`. The default getter
returns enabled, window-sized dimensions, unit zoom, and rendering allowed while
paused. Literal omitted flags remain false. Zero dimensions/zoom use valid
defaults; negative or nonfinite values and invalid world positions are rejected.
`Camera` and `camera_get` remain resolved-view queries. Full description getters
also return the current resolved world view, including for inactive cameras,
without changing the active camera or renderer selection.

Attachments have separate `CameraAttachmentDescription` full setters/getters and
a default getter that follows position and orientation. The entity is non-owning;
callers supply a valid entity and any required transform components. Offsets are
local when following both position and orientation; position offsets are world
coordinates when position following is disabled. All description angles use
clockwise degrees. Positional/options/radians attachment overloads are removed;
use `math_radians_to_degrees` explicitly in the canonical description. The active
camera convenience `graphics_camera_attach` also takes a description. Facade
handle APIs retain the `rohr_camera_` prefix.

World position/orientation edits convert back into attachment offsets. They do
not detach. A full unchanged description preserves motion; a position change
replaces only positional movement, a zoom change replaces only zoom movement,
and orientation/dimension/callback/enable/pause edits preserve unrelated motion.
Focused setters remain direct. Timed attached movement converts its requested
world destination into an offset at command acceptance, then interpolates that
offset while inheriting subsequent target motion/rotation. Invalid requests
preserve the attachment and both motion channels. Explicit detach freezes the
last resolved world view and cancels offset movement, preserving zoom movement.

If a target becomes unavailable, the camera retains the exact generational
reference and its last resolved view. The stored attachment getter still returns
that description; the existing active-camera live-attachment predicate returns
false. Reused entity slots cannot capture the camera. World transform edits fail
while the retained target is unavailable; unrelated configuration edits remain
valid. Explicit detach makes it independently movable. Callbacks and contexts are copied but
remain caller-owned; descriptions never allocate or acquire ownership.

`WindowPresentationDescription` replaces `GraphicsWindowPresentationConfig`;
default/full accessors use `_description_` naming. Defaults and focused controls
retain existing behavior. Invalid dimensions/modes fail before backend mutation;
unchanged descriptions avoid SDL calls. If backend application fails, the engine
attempts to restore the preceding window and logical presentation, publishing
configuration only on complete success. Native OS recovery can itself fail;
callers must check the returned error. Hosted scenes retain the requested mode
while applying windowed output, preserving the existing desktop isolation.

```c
CameraDescription description = rohr_camera_description_default_get();
description.render_callback = draw_scene;
description.render_context = &scene;
CameraIdResult created = rohr_camera_create(description);
/* Check created before using its handle. */
CameraAttachmentDescription follow = rohr_camera_attachment_description_default_get();
follow.entity = player;
follow.position_offset = (Vec2D){0, 100};
EngineResult attached = rohr_camera_attachment_description_set(created.result.value, follow);
/* Check attached. Movement now adjusts the offset instead of detaching. */
EngineResult moved = rohr_camera_position_move(created.result.value, (Vec2D){50, 0}, 0.5);
/* Check moved. Use rohr_camera_detach explicitly to stop following. */
```

## Shipped Goal 11 behavior

Screen, viewport, mounted-item, and persistent UI `Config` types now use the
corresponding `Description` names. Full accessors consistently use
`_description_set/get`; default getters make useful defaults explicit. Immediate
mode UI types remain unchanged. Full descriptions replace configuration, rather
than patching omitted fields. Focused setters remain direct.

`LayerDescription` contains a copied nonempty unique name and numeric order.
Creation and full replacement reject duplicate names; renaming retains the handle
and all entity/UI assignments. Getters borrow the stored name until that layer is
changed or destroyed. Existing keyed numeric setters remain focused operations.

`ScreenDescription` contains a camera and texture dimensions. Resize prepares the
replacement texture before publishing, retaining the screen handle and every
mount; backend failure preserves the previous resource. Zero dimensions use the
window defaults, while negative dimensions and invalid camera references fail.
An unchanged description avoids backend work. Mutation while actively drawing
that screen is rejected. `ViewportDescription` includes rectangle, fitting,
background, and `enabled`; its default remains disabled. Viewport edits preserve
all child handles. Zero viewport dimensions normalize to window dimensions.

`ViewportItemDescription` includes exactly one nonzero `screen` or `ui` handle,
placement/content transforms in degrees, clipping, visibility, drag mode, literal
`layer`, and optional `layer_id`. A nonzero layer handle selects the registered
layer; otherwise the numeric value applies. The getter preserves that binding,
including after focused layer edits, so get/edit/set never silently loses it.
Destroying a named layer retains its last numeric order on existing assignments.
A full item replacement can change resource or switch between screen and UI while
retaining the item handle and parent viewport. Both-zero, both-nonzero, stale,
nonfinite, or invalid-enum configurations fail before changing dependencies.
Screen items normalize zero placement dimensions to window dimensions; UI items
may use zero dimensions. Existing zero-scale rendering conventions are preserved.

`graphics_viewport_item_add(viewport, description)` mounts existing content
without acquiring public ownership. Screen/UI/camera convenience mounting remains
available; the explicit content argument supplies the reference. UI creation
helpers additionally create an automatically managed definition. Removing or
replacing its original mount does not destroy that definition while another mount
still uses it; final unmount cleans it up. Explicitly created reusable UI remains
caller-owned and cannot be destroyed while mounted. Text dependencies are acquired
before release. An unchanged text reference keeps its existing dependency even
when its public owner has been destroyed, without restoring public validity.

Persistent UI full setters preserve compatible interaction state. Slider getters
include the current value; get/edit/set appearance edits retain that value and
active dragging. Supplied values clamp to the range and snap by step. Equal or
reversed bounds fail; use the default getter for a valid 0–1 range. Zero slider
geometry, text scale, border thickness, and hash spacing retain their documented
fallback defaults; negative/nonfinite geometry is rejected. Full item content
replacement, hiding, viewport disabling, and disabling an active slider cancel the
relevant interaction immediately. Drag-mode changes cancel incompatible captures.
No-op/style edits preserve valid capture and the latest interaction flags.

```c
ViewportItemDescriptionResult current = rohr_viewport_item_description_get(item);
if(!rohr_error_check(current)) {
    current.result.value.screen = SCREEN_INVALID;
    current.result.value.ui = reusable_slider;
    current.result.value.rectangle.x = 200;
    EngineResult changed = rohr_viewport_item_description_set(item, current.result.value);
    /* Check changed; item and its parent viewport retain their identities. */
    (void)changed;
}
```

## Explicit exemptions

- **Focused values:** retain setters for magnitude, transform values, scalar
  material properties, force location/orientation, beam endpoints, frame index,
  list replacement, callback/context, and individual handles. Name-based layer
  assignment is one keyed value, unlike layer creation with independent name
  and initial value. Full nested configurations listed above still migrate.
- **Selectors:** a hitbox binding setter selects an entity/animation/frame and
  assigns one hitbox reference; its IDs do not require a description wrapper.
  Angle limits are one range; logical size/aspect and clip rectangles are basic
  geometry values. Keep their existing direct signatures.
- **Temporary operations:** drawing, recording start/stop, timed camera moves,
  directed velocity/acceleration, procedural shape generation, current-transform
  capture, input text-area placement, immediate-mode UI, and execution pipelines
  are not part of this migration. `UIComponentConfig`, `UIDropdownConfig`,
  `UISliderConfig`, `UIButtonStyle`, `UISliderStyle`, `UIFieldBinding`,
  `SceneCallbacks`, and event/report payloads retain their names and roles.
- **Queries and lifecycle:** lookup, inspection, overlap/contact queries,
  reporting, validation, play/reset, retain/release, destruction, component masks,
  group membership, and saved-file I/O do not become configuration descriptions.
- **Math, storage, and tooling:** pure calculations, buffer operations, pool and
  result macros, internal table/cache maintenance, and `rohr_tools.h` build-time
  generation remain outside this runtime API milestone. `RohrToolsComponentDefinition`
  is a code-generation input, not an engine resource configuration.
- **Existing object values:** `ObjectHitbox`, `ObjectFrame`, area geometry,
  source-reference records, and `ObjectVisualStyle`/`ObjectVisuals` remain nested
  authored data. No universal object-graph replacement API or blanket struct
  rename is introduced. Their referenced configuration types migrate with the
  owning family.

A classification describes the role of the current interface. “Migration
required” can mean a family's missing full accessors, a type/default-accessor
rename, or caller adaptation, not necessarily removal of every listed function.
“Compliant” means the existing description interface fits the convention; it does
not assert unrelated features are complete. Exempt queries using renamed types
receive necessary type-reference edits in the owning goal without changing role.

## Exact header inventory

Audit scope: the 20 first-party headers installed by `install(DIRECTORY include/)`.
This inventory includes declarations without Doxygen comments, facade aliases,
callable macros, and direct APIs, rather than relying on the generated facade
reference. Counts are declaration/macro occurrences by header, not a count of
unique runtime operations. Pool-generated interfaces are covered separately
below. Numeric constants, typedefs, struct fields, extern storage, and callback
type declarations are not callable API entries; configuration types are covered
in the family table and exemptions above.

The current inventory contains **1604** classified declaration/macro entries.
The two additions after the milestone-4 inventory freeze are the direct and
public-wrapper joint placement operations; the frozen baseline remains 1,602.

| Classification | Meaning | Owning goal | Entries |
| --- | --- | --- | --- |
| C: Compliant | Existing description interface | — | 342 |
| EV: Intentionally exempt | Focused/basic-value mutation or lookup | — | 275 |
| EQ: Intentionally exempt | Query, validation, snapshot, or result access | — | 400 |
| EO: Intentionally exempt | Operation, lifecycle, execution, or drawing | — | 458 |
| EM: Intentionally exempt | Pure math or utility calculation | — | 77 |
| ES: Intentionally exempt | Internal storage, reporting, or macro machinery | — | 48 |
| ET: Intentionally exempt | Build-time tooling | — | 4 |

The per-header lists are exhaustive. Every declaration/alias is classified once.

<details>
<summary>audio.h: 41 entries</summary>

Source: [audio.h](../include/audio.h).

**C — Existing description interface**

- `audio_music_create`, `audio_music_description_default_get`, `audio_music_description_get`.
- `audio_music_description_set`, `audio_sound_create`, `audio_sound_description_default_get`.
- `audio_sound_description_get`, `audio_sound_description_set`.

**EV — Focused/basic-value mutation or lookup**

- `audio_music_loop_set`, `audio_music_playback_rate_set`, `audio_music_volume_set`.
- `audio_sound_loop_set`, `audio_sound_pan_set`, `audio_sound_playback_rate_set`.
- `audio_sound_volume_set`, `audio_volume_set`.

**EQ — Query, validation, snapshot, or result access**

- `audio_music_loop_check`, `audio_music_paused_check`, `audio_music_playback_rate_get`.
- `audio_music_playing_check`, `audio_music_valid_check`, `audio_music_volume_get`.
- `audio_sound_loop_check`, `audio_sound_pan_get`, `audio_sound_playback_rate_get`.
- `audio_sound_playing_check`, `audio_sound_valid_check`, `audio_sound_volume_get`.
- `audio_started_check`, `audio_volume_get`.

**EO — Operation, lifecycle, execution, or drawing**

- `audio_music_destroy`, `audio_music_pause`, `audio_music_play`.
- `audio_music_resume`, `audio_music_stop`, `audio_sound_destroy`.
- `audio_sound_play`, `audio_sound_stop`, `audio_start`.
- `audio_stop`, `audio_update`.

</details>

<details>
<summary>console.h: 8 entries</summary>

Source: [console.h](../include/console.h).

**EV — Focused/basic-value mutation or lookup**

- `console_debug_set`.

**EQ — Query, validation, snapshot, or result access**

- `console_active_get`.

**EO — Operation, lifecycle, execution, or drawing**

- `console_debug_write`, `console_logs_print`, `console_read`.
- `console_start`, `console_stop`, `console_write`.

</details>

<details>
<summary>engine.h: 16 entries</summary>

Source: [engine.h](../include/engine.h).

**EV — Focused/basic-value mutation or lookup**

- `engine_time_per_tick_set`.

**EQ — Query, validation, snapshot, or result access**

- `engine_paused_get`, `engine_tick_delta_get`, `engine_tick_get`.
- `engine_time_get`, `engine_time_per_tick_get`.

**EO — Operation, lifecycle, execution, or drawing**

- `engine_clock_reset`, `engine_event_poll`, `engine_lifecycle_update`.
- `engine_pause`, `engine_resume`, `engine_start`.
- `engine_stop`, `engine_tick_update`, `engine_time_update`.
- `engine_update`.

</details>

<details>
<summary>entity_components.h: 42 entries</summary>

Source: [entity_components.h](../include/entity_components.h).

**C — Existing description interface**

- `entity_description_add`, `entity_description_get`, `entity_description_set`.
- `entity_lifetime_description_get`, `entity_lifetime_description_set`.

**EV — Focused/basic-value mutation or lookup**

- `entity_child_set`, `entity_group_name_set`, `entity_name_set`.
- `entity_owner_set`, `entity_parent_set`.

**EQ — Query, validation, snapshot, or result access**

- `entity_alive_at_get`, `entity_alive_check`, `entity_alive_count_get`.
- `entity_by_name_get`, `entity_children_get`, `entity_components_check`.
- `entity_from_index_get`, `entity_group_by_name_get`, `entity_group_entity_check`.
- `entity_group_get`, `entity_group_name_get`, `entity_groups_get`.
- `entity_index_alive_check`, `entity_index_components_check`, `entity_index_get`.
- `entity_name_get`, `entity_object_member_by_name_get`, `entity_object_member_name_get`.
- `entity_object_members_get`, `entity_owner_get`, `entity_parent_get`.

**EO — Operation, lifecycle, execution, or drawing**

- `entity_add`, `entity_child_remove`, `entity_components_add`.
- `entity_components_delete`, `entity_delete`, `entity_group_add`.
- `entity_group_create`, `entity_group_destroy`, `entity_group_remove`.
- `entity_life_time_remove`, `entity_parent_remove`.

</details>

<details>
<summary>entity_info.h: 6 entries</summary>

Source: [entity_info.h](../include/entity_info.h).

**EQ — Query, validation, snapshot, or result access**

- `entity_info_check`, `entity_info_get`, `entity_info_issues_count_get`.
- `entity_info_issues_get`, `entity_info_release`, `entity_info_retain`.

</details>

<details>
<summary>error.h: 10 entries</summary>

Source: [error.h](../include/error.h).

**ES — Internal storage, reporting, or macro machinery**

- `ERROR_DECLARE_RESULT_TYPE`, `ERROR_RESULT_MAKE_ERROR`, `ERROR_RESULT_MAKE_VALUE`.
- `error_check`, `error_code_message_get`, `error_detail_set`.
- `error_result_error`, `error_result_error_detail`, `error_result_message_get`.
- `error_result_value`.

</details>

<details>
<summary>game_state.h: 8 entries</summary>

Source: [game_state.h](../include/game_state.h).

**EQ — Query, validation, snapshot, or result access**

- `ui_button_by_name_get`, `ui_font_by_name_get`, `ui_label_by_name_get`.
- `ui_slider_by_name_get`.

**EO — Operation, lifecycle, execution, or drawing**

- `game_state_file_load`, `game_state_file_save`, `game_state_files_load`.
- `game_state_template_file_save`.

</details>

<details>
<summary>graphics.h: 263 entries</summary>

Source: [graphics.h](../include/graphics.h).

**C — Existing description interface**

- `graphics_animated_sprite_add`, `graphics_animated_sprite_create`, `graphics_animated_sprite_description_default_get`.
- `graphics_animated_sprite_description_from_animation_get`, `graphics_animated_sprite_description_get`, `graphics_animated_sprite_description_set`.
- `graphics_animated_sprite_frame_add`, `graphics_animated_sprite_frame_description_get`, `graphics_animated_sprite_frame_description_set`.
- `graphics_animated_sprite_value_description_get`, `graphics_animated_sprite_value_description_set`, `graphics_animated_sprite_value_frame_add`.
- `graphics_animated_sprite_value_frame_description_set`, `graphics_animation_description_default_get`, `graphics_animation_description_get`.
- `graphics_animation_frame_create`, `graphics_animation_frame_description_default_get`, `graphics_animation_frame_description_get`.
- `graphics_animation_load`, `graphics_animation_player_create`, `graphics_animation_player_description_default_get`.
- `graphics_animation_player_description_from_animation_get`, `graphics_animation_player_description_get`, `graphics_animation_player_description_set`.
- `graphics_camera_attach`, `graphics_camera_attachment_description_default_get`, `graphics_camera_attachment_description_get`.
- `graphics_camera_attachment_description_set`, `graphics_camera_create`, `graphics_camera_description_default_get`.
- `graphics_camera_description_get`, `graphics_camera_description_set`, `graphics_font_description_default_get`.
- `graphics_font_description_get`, `graphics_font_load`, `graphics_layer_create`.
- `graphics_layer_description_default_get`, `graphics_layer_description_get`, `graphics_layer_description_set`.
- `graphics_screen_create`, `graphics_screen_description_default_get`, `graphics_screen_description_get`.
- `graphics_screen_description_set`, `graphics_sprite_add`, `graphics_sprite_create`.
- `graphics_sprite_description_default_get`, `graphics_sprite_description_get`, `graphics_sprite_description_set`.
- `graphics_text_create`, `graphics_text_description_default_get`, `graphics_text_description_get`.
- `graphics_text_description_set`, `graphics_texture_description_default_get`, `graphics_texture_description_get`.
- `graphics_texture_load`, `graphics_ui_shape_create`, `graphics_ui_shape_description_default_get`.
- `graphics_ui_shape_description_get`, `graphics_ui_shape_description_set`, `graphics_ui_slider_create`.
- `graphics_ui_slider_description_default_get`, `graphics_ui_slider_description_get`, `graphics_ui_slider_description_set`.
- `graphics_ui_text_create`, `graphics_ui_text_description_default_get`, `graphics_ui_text_description_get`.
- `graphics_ui_text_description_set`, `graphics_viewport_camera_add`, `graphics_viewport_create`.
- `graphics_viewport_description_default_get`, `graphics_viewport_description_get`, `graphics_viewport_description_set`.
- `graphics_viewport_item_add`, `graphics_viewport_item_description_default_get`, `graphics_viewport_item_description_get`.
- `graphics_viewport_item_description_set`, `graphics_viewport_screen_add`, `graphics_viewport_ui_add`.
- `graphics_viewport_ui_shape_add`, `graphics_viewport_ui_slider_add`, `graphics_viewport_ui_text_add`.
- `graphics_window_presentation_description_default_get`, `graphics_window_presentation_description_get`, `graphics_window_presentation_description_set`.

**EV — Focused/basic-value mutation or lookup**

- `graphics_aabb_tree_debug_set`, `graphics_animated_sprite_body_offset_set`, `graphics_animated_sprite_frame_index_set`.
- `graphics_animated_sprite_frame_offset_set`, `graphics_animated_sprite_frame_rotation_radians_set`, `graphics_animated_sprite_frame_rotation_set`.
- `graphics_animated_sprite_frame_scale_set`, `graphics_animated_sprite_orientation_offset_radians_set`, `graphics_animated_sprite_orientation_offset_set`.
- `graphics_aspect_ratio_auto_set`, `graphics_aspect_ratio_set`, `graphics_camera_active_set`.
- `graphics_camera_disable_set`, `graphics_camera_enable_set`, `graphics_camera_entity_attachment_set`.
- `graphics_camera_pause_with_engine_set`, `graphics_camera_render_callback_set`, `graphics_camera_render_when_paused_set`.
- `graphics_contacts_debug_set`, `graphics_frame_limit_set`, `graphics_layer_active_id_set`.
- `graphics_layer_active_name_set`, `graphics_layer_active_set`, `graphics_layer_animation_id_set`.
- `graphics_layer_animation_name_set`, `graphics_layer_animation_set`, `graphics_layer_entity_id_set`.
- `graphics_layer_entity_name_set`, `graphics_layer_entity_set`, `graphics_layer_name_set`.
- `graphics_layer_set`, `graphics_layer_sprite_id_set`, `graphics_layer_sprite_name_set`.
- `graphics_layer_sprite_set`, `graphics_layer_ui_id_set`, `graphics_layer_ui_name_set`.
- `graphics_layer_ui_set`, `graphics_logical_size_set`, `graphics_screen_camera_set`.
- `graphics_soft_body_area_color_set`, `graphics_soft_body_area_visibility_set`, `graphics_soft_body_beam_color_set`.
- `graphics_soft_body_node_color_set`, `graphics_sprite_body_offset_set`, `graphics_sprite_orientation_offset_radians_set`.
- `graphics_sprite_orientation_offset_set`, `graphics_text_value_set`, `graphics_text_wrap_width_set`.
- `graphics_ui_slider_value_set`, `graphics_viewport_camera_set`, `graphics_viewport_disable_set`.
- `graphics_viewport_enable_set`, `graphics_viewport_item_drag_mode_set`, `graphics_vsync_set`.

**EQ — Query, validation, snapshot, or result access**

- `graphics_aabb_tree_debug_check`, `graphics_animated_sprite_body_offset_get`, `graphics_animated_sprite_frame_index_get`.
- `graphics_animated_sprite_frame_offset_get`, `graphics_animated_sprite_frame_rotation_get`, `graphics_animated_sprite_frame_rotation_radians_get`.
- `graphics_animated_sprite_frame_scale_get`, `graphics_animated_sprite_orientation_offset_get`, `graphics_animated_sprite_orientation_offset_radians_get`.
- `graphics_animation_info_get`, `graphics_animation_valid_check`, `graphics_camera_active_get`.
- `graphics_camera_attached_get`, `graphics_camera_get`, `graphics_camera_moving_get`.
- `graphics_camera_zoom_get`, `graphics_contacts_debug_check`, `graphics_font_default_get`.
- `graphics_font_valid_check`, `graphics_layer_active_get`, `graphics_layer_animation_get`.
- `graphics_layer_animation_id_get`, `graphics_layer_entity_get`, `graphics_layer_entity_id_get`.
- `graphics_layer_get`, `graphics_layer_name_get`, `graphics_layer_name_id_get`.
- `graphics_layer_sprite_get`, `graphics_layer_sprite_id_get`, `graphics_layer_ui_get`.
- `graphics_layer_ui_id_get`, `graphics_mouse_screen_position_get`, `graphics_render_output_size_get`.
- `graphics_screen_to_world_get`, `graphics_sprite_body_offset_get`, `graphics_sprite_orientation_offset_get`.
- `graphics_sprite_orientation_offset_radians_get`, `graphics_text_valid_check`, `graphics_texture_size_get`.
- `graphics_texture_valid_check`, `graphics_ui_slider_value_get`, `graphics_viewport_item_drag_mode_get`.
- `graphics_viewport_item_dragging_check`, `graphics_viewport_item_viewport_get`, `graphics_viewport_ui_hovered_check`.
- `graphics_viewport_ui_pressed_check`, `graphics_viewport_ui_slider_changed_check`, `graphics_window_to_screen_get`.
- `graphics_world_to_screen_get`.

**EO — Operation, lifecycle, execution, or drawing**

- `graphics_aabb_tree_draw`, `graphics_animated_sprite_draw`, `graphics_animated_sprite_update`.
- `graphics_animated_sprites_draw`, `graphics_animation_player_update`, `graphics_animation_release`.
- `graphics_animation_retain`, `graphics_background_draw`, `graphics_camera_attachment_remove`.
- `graphics_camera_destroy`, `graphics_camera_detach`, `graphics_camera_move`.
- `graphics_camera_position_from_entity_set`, `graphics_camera_position_move`, `graphics_camera_position_set`.
- `graphics_camera_radians_rotate`, `graphics_camera_rotate`, `graphics_camera_zoom_set`.
- `graphics_contacts_draw`, `graphics_events_poll`, `graphics_font_release`.
- `graphics_font_retain`, `graphics_hit_box_colored_draw`, `graphics_hit_box_draw`.
- `graphics_hit_boxes_draw`, `graphics_joint_draw`, `graphics_joints_draw`.
- `graphics_layer_animation_clear`, `graphics_layer_destroy`, `graphics_layer_entity_clear`.
- `graphics_layer_sprite_clear`, `graphics_layer_ui_clear`, `graphics_local_origins_draw`.
- `graphics_particle_draw`, `graphics_particles_draw`, `graphics_pipeline_cameras_update`.
- `graphics_pipeline_commands_execute`, `graphics_pipeline_present`, `graphics_pipeline_recording_update`.
- `graphics_pipeline_viewports_draw`, `graphics_recording_start`, `graphics_recording_stop`.
- `graphics_screen_clip_clear`, `graphics_screen_clip_pop`, `graphics_screen_clip_push`.
- `graphics_screen_clip_set`, `graphics_screen_destroy`, `graphics_screen_quad_draw`.
- `graphics_screen_quad_radians_draw`, `graphics_screen_rect_draw`, `graphics_screen_shape_filled_draw`.
- `graphics_screen_text_scaled_rotated_draw`, `graphics_screen_text_scaled_rotated_radians_draw`, `graphics_screen_texture_draw`.
- `graphics_screen_texture_radians_draw`, `graphics_show`, `graphics_soft_body_area_color_clear`.
- `graphics_soft_body_draw`, `graphics_sprite_draw`, `graphics_sprite_frames_update`.
- `graphics_sprites_draw`, `graphics_start`, `graphics_stop`.
- `graphics_text_destroy`, `graphics_text_draw`, `graphics_text_scaled_draw`.
- `graphics_texture_draw`, `graphics_texture_radians_draw`, `graphics_texture_release`.
- `graphics_texture_retain`, `graphics_textures_scale`, `graphics_ui_destroy`.
- `graphics_viewport_camera_clear`, `graphics_viewport_destroy`, `graphics_viewport_item_remove`.

**EM — Pure math or utility calculation**

- `graphics_color_hex_create`.

</details>

<details>
<summary>input.h: 53 entries</summary>

Source: [input.h](../include/input.h).

**C — Existing description interface**

- `input_action_create`, `input_action_description_default_get`, `input_action_description_get`.
- `input_action_description_set`, `input_controller_create`, `input_controller_description_default_get`.
- `input_controller_description_get`, `input_controller_description_set`.

**EV — Focused/basic-value mutation or lookup**

- `input_action_bindings_default_set`, `input_action_bindings_override_set`, `input_action_button_initial_state_set`.
- `input_action_button_mode_set`, `input_action_button_state_set`, `input_controller_enabled_set`.
- `input_mouse_relative_mode_set`.

**EQ — Query, validation, snapshot, or result access**

- `input_action_axis_1d_get`, `input_action_axis_2d_get`, `input_action_bindings_default_get`.
- `input_action_bindings_effective_get`, `input_action_bindings_override_check`, `input_action_bindings_override_get`.
- `input_action_button_down_check`, `input_action_button_initial_state_check`, `input_action_button_mode_get`.
- `input_action_button_pressed_check`, `input_action_button_released_check`, `input_action_by_name_get`.
- `input_action_type_get`, `input_binding_valid_check`, `input_controller_by_name_get`.
- `input_controller_enabled_check`, `input_key_down_check`, `input_key_pressed_check`.
- `input_key_released_check`, `input_modifiers_get`, `input_mouse_button_down_check`.
- `input_mouse_button_pressed_check`, `input_mouse_button_released_check`, `input_mouse_delta_get`.
- `input_mouse_position_get`, `input_mouse_relative_mode_check`, `input_mouse_wheel_get`.
- `input_text_state_get`.

**EO — Operation, lifecycle, execution, or drawing**

- `input_action_bindings_override_clear`, `input_action_button_state_reset`, `input_action_destroy`.
- `input_binding_key_create`, `input_binding_scancode_create`, `input_controller_destroy`.
- `input_frame_begin`, `input_text_area_set`, `input_text_start`.
- `input_text_stop`.

</details>

<details>
<summary>math2d.h: 29 entries</summary>

Source: [math2d.h](../include/math2d.h).

**EM — Pure math or utility calculation**

- `math_aabb_create`, `math_aabb_overlap`, `math_angular_velocity_cross_vec`.
- `math_angular_velocity_radians_cross_vec`, `math_axis_magnitude`, `math_circle_create`.
- `math_circle_overlap_depth`, `math_circle_radius`, `math_cross_2d`.
- `math_degrees_to_radians`, `math_dot_product`, `math_normals_create`.
- `math_polygon_centroid`, `math_project_onto_axis`, `math_project_shape_on_axis`.
- `math_projection_overlap`, `math_radians_to_degrees`, `math_shape_scale`.
- `math_shape_x_scale`, `math_shape_y_scale`, `math_square_create`.
- `math_vector_magnitude`, `math_vector_normalize`, `math_vector_radians_rotate`.
- `math_vector_rotate`, `math_vector_subtract`, `math_vectors_normalize`.
- `math_vertex_add`, `math_vertex_delete`.

</details>

<details>
<summary>memory.h: 4 entries</summary>

Source: [memory.h](../include/memory.h).

**ES — Internal storage, reporting, or macro machinery**

- `MEMORY_DECLARE_OBJECT_POOL`, `MEMORY_DEFINE_OBJECT_POOL`, `MEMORY_POOL_ERROR`.
- `MEMORY_POOL_VALUE`.

</details>

<details>
<summary>object.h: 37 entries</summary>

Source: [object.h](../include/object.h).

**C — Existing description interface**

- `entity_object_add`, `entity_object_add_with_options`, `entity_object_by_name_add`.
- `entity_object_by_name_add_with_options`, `entity_object_description_add`, `entity_object_description_by_name_add`.
- `object_collision_description_get`, `object_collision_description_set`, `object_color_description_get`.
- `object_color_description_set`, `object_description_by_name_get`, `object_description_get`.
- `object_description_name_get`, `object_force_description_set`, `object_get`.
- `object_material_description_get`, `object_material_description_set`, `object_spawn_description_default_get`.
- `object_torque_description_set`.

**EV — Focused/basic-value mutation or lookup**

- `object_acceleration_set`, `object_angular_velocity_set`, `object_mass_set`.
- `object_orientation_set`, `object_position_set`, `object_radius_set`.
- `object_scale_set`, `object_velocity_set`, `object_visibility_set`.

**EQ — Query, validation, snapshot, or result access**

- `entity_object_anchor_get`, `entity_object_camera_get`.

**EO — Operation, lifecycle, execution, or drawing**

- `entity_object_draw`, `object_copy`, `object_description_copy`.
- `object_destroy`, `object_load`.

**EM — Pure math or utility calculation**

- `object_copy_with_options`.

**ES — Internal storage, reporting, or macro machinery**

- `object_description_ref`.

</details>

<details>
<summary>physics.h: 232 entries</summary>

Source: [physics.h](../include/physics.h).

**C — Existing description interface**

- `physics_axis_lock_description_get`, `physics_axis_lock_description_set`, `physics_collision_filter_description_default_get`.
- `physics_collision_filter_description_get`, `physics_collision_filter_description_set`, `physics_force_add`.
- `physics_force_description_get`, `physics_force_description_set`, `physics_joint_add`.
- `physics_joint_anchor_add`, `physics_joint_anchor_description_get`, `physics_joint_anchor_description_set`.
- `physics_joint_description_get`, `physics_joint_description_set`, `physics_particle_add`.
- `physics_particle_description_default_get`, `physics_particle_description_get`, `physics_particle_description_set`.
- `physics_soft_body_add`, `physics_soft_body_area_add`, `physics_soft_body_area_description_default_get`.
- `physics_soft_body_area_description_get`, `physics_soft_body_area_description_set`, `physics_soft_body_beam_add`.
- `physics_soft_body_beam_collision_description_default_get`, `physics_soft_body_beam_collision_description_get`, `physics_soft_body_beam_collision_description_set`.
- `physics_soft_body_beam_collision_filter_description_get`, `physics_soft_body_beam_collision_filter_description_set`, `physics_soft_body_beam_description_default_get`.
- `physics_soft_body_beam_description_get`, `physics_soft_body_beam_description_set`, `physics_soft_body_description_default_get`.
- `physics_soft_body_description_get`, `physics_soft_body_description_set`, `physics_soft_body_node_add`.
- `physics_soft_body_node_collision_filter_description_get`, `physics_soft_body_node_collision_filter_description_set`, `physics_soft_body_node_description_default_get`.
- `physics_soft_body_node_description_get`, `physics_soft_body_node_description_set`, `physics_soft_body_node_local_add`.
- `physics_soft_body_node_to_anchor_pin_add`, `physics_torque_add`, `physics_torque_description_get`.
- `physics_torque_description_set`, `physics_transform_lock_description_get`, `physics_transform_lock_description_set`.

**EV — Focused/basic-value mutation or lookup**

- `physics_acceleration_set`, `physics_angle_lock_radians_set`, `physics_angle_lock_set`.
- `physics_angular_acceleration_radians_set`, `physics_angular_acceleration_set`, `physics_angular_velocity_maximum_radians_set`.
- `physics_angular_velocity_maximum_set`, `physics_angular_velocity_radians_set`, `physics_angular_velocity_set`.
- `physics_center_of_mass_automatic_set`, `physics_center_of_mass_local_position_set`, `physics_collision_category_set`.
- `physics_collision_with_all_set`, `physics_collision_with_none_set`, `physics_collision_with_set`.
- `physics_dt_per_tick_set`, `physics_dynamic_set`, `physics_force_location_set`.
- `physics_force_magnitude_set`, `physics_force_orientation_set`, `physics_force_target_set`.
- `physics_friction_set`, `physics_gravity_set`, `physics_hitbox_active_id_set`.
- `physics_hitbox_active_index_set`, `physics_hitbox_animation_binding_at_set`, `physics_hitbox_animation_binding_set`.
- `physics_hitbox_at_set`, `physics_hitbox_by_id_set`, `physics_hitbox_id_at_set`.
- `physics_hitbox_set`, `physics_joint_anchor_local_position_set`, `physics_kinematic_driven_set`.
- `physics_mass_set`, `physics_orientation_radians_set`, `physics_orientation_set`.
- `physics_particle_origin_set`, `physics_particle_radius_set`, `physics_position_set`.
- `physics_restitution_set`, `physics_soft_body_area_geometry_set`, `physics_soft_body_area_order_set`.
- `physics_soft_body_beam_collision_thickness_set`, `physics_soft_body_beam_endpoints_set`, `physics_soft_body_node_local_position_set`.
- `physics_solver_iterations_set`, `physics_static_set`, `physics_substeps_set`.
- `physics_target_set`, `physics_torque_magnitude_set`, `physics_torque_target_set`.
- `physics_velocity_set`.

**EQ — Query, validation, snapshot, or result access**

- `physics_angular_velocity_get`, `physics_angular_velocity_maximum_get`, `physics_angular_velocity_maximum_radians_get`.
- `physics_angular_velocity_radians_get`, `physics_center_of_mass_automatic_check`, `physics_center_of_mass_local_position_get`.
- `physics_center_of_mass_world_position_get`, `physics_collision_between_check`, `physics_contact_check`.
- `physics_contact_count_get`, `physics_contact_entered_check`, `physics_contact_exited_check`.
- `physics_contact_get`, `physics_contact_stayed_check`, `physics_contact_total_impulse_get`.
- `physics_contacts_get`, `physics_dt_per_tick_get`, `physics_entity_held_get`.
- `physics_entity_movable_get`, `physics_entity_simulated_get`, `physics_force_location_get`.
- `physics_force_magnitude_get`, `physics_force_orientation_get`, `physics_force_target_get`.
- `physics_global_hit_box_get`, `physics_gravity_check`, `physics_gravity_get`.
- `physics_hitbox_active_id_get`, `physics_hitbox_active_index_get`, `physics_hitbox_animation_binding_at_get`.
- `physics_hitbox_animation_binding_get`, `physics_hitbox_at_get`, `physics_hitbox_by_id_get`.
- `physics_hitbox_count_get`, `physics_hitbox_get`, `physics_hitbox_id_at_get`.
- `physics_joint_anchor_local_position_get`, `physics_joint_anchor_world_position_get`, `physics_joint_anchors_get`.
- `physics_kinematic_driven_check`, `physics_mass_check`, `physics_mass_get`.
- `physics_moment_of_inertia_get`, `physics_overlap_check`, `physics_overlap_count_get`.
- `physics_overlap_entered_check`, `physics_overlap_exited_check`, `physics_overlap_get`.
- `physics_overlap_stayed_check`, `physics_overlaps_get`, `physics_particle_origin_get`.
- `physics_particle_overlap_get`, `physics_particle_radius_get`, `physics_position_get`.
- `physics_sat_overlap_get`, `physics_soft_body_area_get`, `physics_soft_body_beam_get`.
- `physics_soft_body_get`, `physics_soft_body_node_get`, `physics_soft_body_node_local_position_get`.
- `physics_solver_iterations_get`, `physics_substeps_get`, `physics_torque_magnitude_get`.
- `physics_torque_target_get`, `physics_world_position_check`.

**EO — Operation, lifecycle, execution, or drawing**

- `physics_acceleration_away_from_entity_set`, `physics_acceleration_away_from_position_set`, `physics_acceleration_toward_entity_set`.
- `physics_acceleration_toward_position_set`, `physics_angular_impulse_apply`, `physics_axis_lock_remove`.
- `physics_dt_update`, `physics_engine_time_per_tick_use`, `physics_entity_hold`.
- `physics_entity_stop`, `physics_entity_unhold`, `physics_force_apply`.
- `physics_gravity_disable`, `physics_gravity_enable`, `physics_group_acceleration_away_from_entity_set`.
- `physics_group_acceleration_toward_entity_set`, `physics_group_entities_hold`, `physics_group_entities_stop`.
- `physics_group_entities_unhold`, `physics_group_velocity_away_from_entity_set`, `physics_group_velocity_toward_entity_set`.
- `physics_hitbox_add`, `physics_hitbox_animation_binding_at_remove`, `physics_hitbox_animation_binding_remove`.
- `physics_hitbox_at_remove`, `physics_hitbox_autoshape_circle_set`, `physics_hitbox_autoshape_rectangle_set`.
- `physics_hitbox_autoshape_triangle_set`, `physics_hitbox_by_id_remove`, `physics_hitbox_remove`.
- `physics_impulse_apply`, `physics_joint_anchor_remove`, `physics_kinematic_driven_remove`.
- `physics_joint_place`.
- `physics_mass_remove`, `physics_pipeline_accelerations_clear`, `physics_pipeline_constraints_solve`.
- `physics_pipeline_contacts_gather`, `physics_pipeline_forces_apply`, `physics_pipeline_gravity_apply`.
- `physics_pipeline_integrate`, `physics_pipeline_joints_gather`, `physics_pipeline_step_begin`.
- `physics_pipeline_substep`, `physics_pipeline_substep_begin`, `physics_pipeline_update`.
- `physics_shape_world_radians_translate`, `physics_shape_world_translate`, `physics_soft_body_autoshape_circle_set`.
- `physics_soft_body_autoshape_rectangle_set`, `physics_soft_body_autoshape_triangle_set`, `physics_soft_body_beam_collision_disable`.
- `physics_soft_body_beam_collision_enable`, `physics_soft_body_node_impulse_apply`, `physics_torque_apply`.
- `physics_transform_lock_current_transform_set`, `physics_transform_lock_remove`, `physics_update`.
- `physics_velocity_away_from_entity_set`, `physics_velocity_away_from_position_set`, `physics_velocity_toward_entity_set`.
- `physics_velocity_toward_position_set`.

**EM — Pure math or utility calculation**

- `physics_circle_moment_of_inertia`, `physics_polygon_moment_of_inertia`.

**ES — Internal storage, reporting, or macro machinery**

- `physics_hitbox_animation_binding_hitbox_remove`, `physics_hitbox_animation_bindings_entity_clear`, `physics_hitbox_animation_bindings_update`.

</details>

<details>
<summary>report.h: 12 entries</summary>

Source: [report.h](../include/report.h).

**ES — Internal storage, reporting, or macro machinery**

- `report_console_write`, `report_create`, `report_diagnostic_add`.
- `report_diagnostic_detail_set`, `report_entry_add`, `report_get`.
- `report_header_set`, `report_json_write`, `report_merge`.
- `report_statistic_add`, `report_text_write`, `report_valid_check`.

</details>

<details>
<summary>rohr.h: 790 entries</summary>

Source: [rohr.h](../include/rohr.h).

**C — Existing description interface**

- `rohr_audio_music_create`, `rohr_audio_music_description_default_get`, `rohr_audio_music_description_get`.
- `rohr_audio_music_description_set`, `rohr_audio_sound_create`, `rohr_audio_sound_description_default_get`.
- `rohr_audio_sound_description_get`, `rohr_audio_sound_description_set`, `rohr_camera_attachment_description_default_get`.
- `rohr_camera_attachment_description_get`, `rohr_camera_attachment_description_set`, `rohr_camera_create`.
- `rohr_camera_description_default_get`, `rohr_camera_description_get`, `rohr_camera_description_set`.
- `rohr_entity_description_add`, `rohr_entity_description_get`, `rohr_entity_description_set`.
- `rohr_entity_lifetime_description_get`, `rohr_entity_lifetime_description_set`, `rohr_entity_object_add`.
- `rohr_entity_object_add_with_options`, `rohr_entity_object_by_name_add`, `rohr_entity_object_by_name_add_with_options`.
- `rohr_entity_object_description_add`, `rohr_entity_object_description_by_name_add`, `rohr_graphics_animated_sprite_add`.
- `rohr_graphics_animated_sprite_create`, `rohr_graphics_animated_sprite_description_default_get`, `rohr_graphics_animated_sprite_description_from_animation_get`.
- `rohr_graphics_animated_sprite_description_get`, `rohr_graphics_animated_sprite_description_set`, `rohr_graphics_animated_sprite_frame_add`.
- `rohr_graphics_animated_sprite_frame_description_get`, `rohr_graphics_animated_sprite_frame_description_set`, `rohr_graphics_animated_sprite_value_description_get`.
- `rohr_graphics_animated_sprite_value_description_set`, `rohr_graphics_animated_sprite_value_frame_add`, `rohr_graphics_animated_sprite_value_frame_description_set`.
- `rohr_graphics_animation_description_default_get`, `rohr_graphics_animation_description_get`, `rohr_graphics_animation_frame_create`.
- `rohr_graphics_animation_frame_description_default_get`, `rohr_graphics_animation_frame_description_get`, `rohr_graphics_animation_load`.
- `rohr_graphics_animation_player_create`, `rohr_graphics_animation_player_description_default_get`, `rohr_graphics_animation_player_description_from_animation_get`.
- `rohr_graphics_animation_player_description_get`, `rohr_graphics_animation_player_description_set`, `rohr_graphics_camera_attach`.
- `rohr_graphics_font_description_default_get`, `rohr_graphics_font_description_get`, `rohr_graphics_font_load`.
- `rohr_graphics_layer_create`, `rohr_graphics_layer_description_default_get`, `rohr_graphics_layer_description_get`.
- `rohr_graphics_layer_description_set`, `rohr_graphics_sprite_add`, `rohr_graphics_sprite_create`.
- `rohr_graphics_sprite_description_default_get`, `rohr_graphics_sprite_description_get`, `rohr_graphics_sprite_description_set`.
- `rohr_graphics_text_create`, `rohr_graphics_text_description_default_get`, `rohr_graphics_text_description_get`.
- `rohr_graphics_text_description_set`, `rohr_graphics_texture_description_default_get`, `rohr_graphics_texture_description_get`.
- `rohr_graphics_texture_load`, `rohr_graphics_ui_shape_create`, `rohr_graphics_ui_shape_description_default_get`.
- `rohr_graphics_ui_shape_description_get`, `rohr_graphics_ui_shape_description_set`, `rohr_graphics_ui_slider_create`.
- `rohr_graphics_ui_slider_description_default_get`, `rohr_graphics_ui_slider_description_get`, `rohr_graphics_ui_slider_description_set`.
- `rohr_graphics_ui_text_create`, `rohr_graphics_ui_text_description_default_get`, `rohr_graphics_ui_text_description_get`.
- `rohr_graphics_ui_text_description_set`, `rohr_graphics_window_presentation_description_default_get`, `rohr_graphics_window_presentation_description_get`.
- `rohr_graphics_window_presentation_description_set`, `rohr_input_action_create`, `rohr_input_action_description_default_get`.
- `rohr_input_action_description_get`, `rohr_input_action_description_set`, `rohr_input_controller_create`.
- `rohr_input_controller_description_default_get`, `rohr_input_controller_description_get`, `rohr_input_controller_description_set`.
- `rohr_object_collision_description_get`, `rohr_object_collision_description_set`, `rohr_object_color_description_get`.
- `rohr_object_color_description_set`, `rohr_object_description_by_name_get`, `rohr_object_force_description_set`.
- `rohr_object_get`, `rohr_object_material_description_get`, `rohr_object_material_description_set`.
- `rohr_object_spawn_description_default_get`, `rohr_object_torque_description_set`, `rohr_physics_axis_lock_description_get`.
- `rohr_physics_axis_lock_description_set`, `rohr_physics_collision_filter_description_default_get`, `rohr_physics_collision_filter_description_get`.
- `rohr_physics_collision_filter_description_set`, `rohr_physics_force_add`, `rohr_physics_force_description_get`.
- `rohr_physics_force_description_set`, `rohr_physics_joint_add`, `rohr_physics_joint_anchor_add`.
- `rohr_physics_joint_anchor_description_get`, `rohr_physics_joint_anchor_description_set`, `rohr_physics_joint_description_get`.
- `rohr_physics_joint_description_set`, `rohr_physics_particle_add`, `rohr_physics_particle_description_default_get`.
- `rohr_physics_particle_description_get`, `rohr_physics_particle_description_set`, `rohr_physics_soft_body_add`.
- `rohr_physics_soft_body_area_add`, `rohr_physics_soft_body_area_description_default_get`, `rohr_physics_soft_body_area_description_get`.
- `rohr_physics_soft_body_area_description_set`, `rohr_physics_soft_body_beam_add`, `rohr_physics_soft_body_beam_collision_description_default_get`.
- `rohr_physics_soft_body_beam_collision_description_get`, `rohr_physics_soft_body_beam_collision_description_set`, `rohr_physics_soft_body_beam_collision_filter_description_get`.
- `rohr_physics_soft_body_beam_collision_filter_description_set`, `rohr_physics_soft_body_beam_description_default_get`, `rohr_physics_soft_body_beam_description_get`.
- `rohr_physics_soft_body_beam_description_set`, `rohr_physics_soft_body_description_default_get`, `rohr_physics_soft_body_description_get`.
- `rohr_physics_soft_body_description_set`, `rohr_physics_soft_body_node_add`, `rohr_physics_soft_body_node_collision_filter_description_get`.
- `rohr_physics_soft_body_node_collision_filter_description_set`, `rohr_physics_soft_body_node_description_default_get`, `rohr_physics_soft_body_node_description_get`.
- `rohr_physics_soft_body_node_description_set`, `rohr_physics_soft_body_node_local_add`, `rohr_physics_soft_body_node_to_anchor_pin_add`.
- `rohr_physics_torque_add`, `rohr_physics_torque_description_get`, `rohr_physics_torque_description_set`.
- `rohr_physics_transform_lock_description_get`, `rohr_physics_transform_lock_description_set`, `rohr_screen_create`.
- `rohr_screen_description_default_get`, `rohr_screen_description_get`, `rohr_screen_description_set`.
- `rohr_viewport_camera_add`, `rohr_viewport_create`, `rohr_viewport_description_default_get`.
- `rohr_viewport_description_get`, `rohr_viewport_description_set`, `rohr_viewport_item_add`.
- `rohr_viewport_item_description_default_get`, `rohr_viewport_item_description_get`, `rohr_viewport_item_description_set`.
- `rohr_viewport_screen_add`, `rohr_viewport_ui_add`, `rohr_viewport_ui_shape_add`.
- `rohr_viewport_ui_slider_add`, `rohr_viewport_ui_text_add`.

**EV — Focused/basic-value mutation or lookup**

- `rohr_audio_music_loop_set`, `rohr_audio_music_playback_rate_set`, `rohr_audio_music_volume_set`.
- `rohr_audio_sound_loop_set`, `rohr_audio_sound_pan_set`, `rohr_audio_sound_playback_rate_set`.
- `rohr_audio_sound_volume_set`, `rohr_audio_volume_set`, `rohr_camera_active_set`.
- `rohr_camera_disable_set`, `rohr_camera_enable_set`, `rohr_camera_entity_attachment_set`.
- `rohr_camera_pause_with_engine_set`, `rohr_camera_render_callback_set`, `rohr_camera_render_when_paused_set`.
- `rohr_console_debug_set`, `rohr_directory_working_set`, `rohr_engine_time_per_tick_set`.
- `rohr_entity_child_set`, `rohr_entity_group_name_set`, `rohr_entity_name_set`.
- `rohr_entity_owner_set`, `rohr_entity_parent_set`, `rohr_graphics_aabb_tree_debug_set`.
- `rohr_graphics_animated_sprite_body_offset_set`, `rohr_graphics_animated_sprite_frame_index_set`, `rohr_graphics_animated_sprite_frame_offset_set`.
- `rohr_graphics_animated_sprite_frame_rotation_radians_set`, `rohr_graphics_animated_sprite_frame_rotation_set`, `rohr_graphics_animated_sprite_frame_scale_set`.
- `rohr_graphics_animated_sprite_orientation_offset_radians_set`, `rohr_graphics_animated_sprite_orientation_offset_set`, `rohr_graphics_aspect_ratio_auto_set`.
- `rohr_graphics_aspect_ratio_set`, `rohr_graphics_contacts_debug_set`, `rohr_graphics_frame_limit_set`.
- `rohr_graphics_layer_active_id_set`, `rohr_graphics_layer_active_name_set`, `rohr_graphics_layer_active_set`.
- `rohr_graphics_layer_animation_id_set`, `rohr_graphics_layer_animation_name_set`, `rohr_graphics_layer_animation_set`.
- `rohr_graphics_layer_entity_id_set`, `rohr_graphics_layer_entity_name_set`, `rohr_graphics_layer_entity_set`.
- `rohr_graphics_layer_name_set`, `rohr_graphics_layer_set`, `rohr_graphics_layer_sprite_id_set`.
- `rohr_graphics_layer_sprite_name_set`, `rohr_graphics_layer_sprite_set`, `rohr_graphics_layer_ui_id_set`.
- `rohr_graphics_layer_ui_name_set`, `rohr_graphics_layer_ui_set`, `rohr_graphics_logical_size_set`.
- `rohr_graphics_soft_body_area_color_set`, `rohr_graphics_soft_body_area_visibility_set`, `rohr_graphics_soft_body_beam_color_set`.
- `rohr_graphics_soft_body_node_color_set`, `rohr_graphics_sprite_body_offset_set`, `rohr_graphics_sprite_orientation_offset_radians_set`.
- `rohr_graphics_sprite_orientation_offset_set`, `rohr_graphics_text_value_set`, `rohr_graphics_text_wrap_width_set`.
- `rohr_graphics_ui_slider_value_set`, `rohr_graphics_vsync_set`, `rohr_input_action_bindings_default_set`.
- `rohr_input_action_bindings_override_set`, `rohr_input_action_button_initial_state_set`, `rohr_input_action_button_mode_set`.
- `rohr_input_action_button_state_set`, `rohr_input_controller_enabled_set`, `rohr_input_mouse_relative_mode_set`.
- `rohr_object_acceleration_set`, `rohr_object_angular_velocity_set`, `rohr_object_mass_set`.
- `rohr_object_orientation_set`, `rohr_object_position_set`, `rohr_object_radius_set`.
- `rohr_object_scale_set`, `rohr_object_velocity_set`, `rohr_object_visibility_set`.
- `rohr_physics_acceleration_set`, `rohr_physics_angle_lock_radians_set`, `rohr_physics_angle_lock_set`.
- `rohr_physics_angular_acceleration_radians_set`, `rohr_physics_angular_acceleration_set`, `rohr_physics_angular_velocity_maximum_radians_set`.
- `rohr_physics_angular_velocity_maximum_set`, `rohr_physics_angular_velocity_radians_set`, `rohr_physics_angular_velocity_set`.
- `rohr_physics_center_of_mass_automatic_set`, `rohr_physics_center_of_mass_local_position_set`, `rohr_physics_collision_category_set`.
- `rohr_physics_collision_with_all_set`, `rohr_physics_collision_with_none_set`, `rohr_physics_collision_with_set`.
- `rohr_physics_dt_per_tick_set`, `rohr_physics_dynamic_set`, `rohr_physics_force_location_set`.
- `rohr_physics_force_magnitude_set`, `rohr_physics_force_orientation_set`, `rohr_physics_force_target_set`.
- `rohr_physics_friction_set`, `rohr_physics_gravity_set`, `rohr_physics_hitbox_active_id_set`.
- `rohr_physics_hitbox_active_index_set`, `rohr_physics_hitbox_animation_binding_at_set`, `rohr_physics_hitbox_animation_binding_set`.
- `rohr_physics_hitbox_at_set`, `rohr_physics_hitbox_by_id_set`, `rohr_physics_hitbox_id_at_set`.
- `rohr_physics_hitbox_set`, `rohr_physics_joint_anchor_local_position_set`, `rohr_physics_kinematic_driven_set`.
- `rohr_physics_mass_set`, `rohr_physics_orientation_radians_set`, `rohr_physics_orientation_set`.
- `rohr_physics_particle_origin_set`, `rohr_physics_particle_radius_set`, `rohr_physics_position_set`.
- `rohr_physics_restitution_set`, `rohr_physics_soft_body_area_geometry_set`, `rohr_physics_soft_body_area_order_set`.
- `rohr_physics_soft_body_beam_collision_thickness_set`, `rohr_physics_soft_body_beam_endpoints_set`, `rohr_physics_soft_body_node_local_position_set`.
- `rohr_physics_solver_iterations_set`, `rohr_physics_static_set`, `rohr_physics_substeps_set`.
- `rohr_physics_target_set`, `rohr_physics_torque_magnitude_set`, `rohr_physics_torque_target_set`.
- `rohr_physics_velocity_set`, `rohr_screen_camera_set`, `rohr_viewport_camera_set`.
- `rohr_viewport_disable_set`, `rohr_viewport_enable_set`, `rohr_viewport_item_drag_mode_set`.

**EQ — Query, validation, snapshot, or result access**

- `rohr_audio_music_loop_check`, `rohr_audio_music_paused_check`, `rohr_audio_music_playback_rate_get`.
- `rohr_audio_music_playing_check`, `rohr_audio_music_valid_check`, `rohr_audio_music_volume_get`.
- `rohr_audio_sound_loop_check`, `rohr_audio_sound_pan_get`, `rohr_audio_sound_playback_rate_get`.
- `rohr_audio_sound_playing_check`, `rohr_audio_sound_valid_check`, `rohr_audio_sound_volume_get`.
- `rohr_audio_started_check`, `rohr_audio_volume_get`, `rohr_camera_active_get`.
- `rohr_camera_get`, `rohr_camera_moving_get`, `rohr_camera_zoom_get`.
- `rohr_console_active_get`, `rohr_directory_base_get`, `rohr_engine_paused_get`.
- `rohr_engine_tick_delta_get`, `rohr_engine_tick_get`, `rohr_engine_time_get`.
- `rohr_engine_time_per_tick_get`, `rohr_entity_alive_at_get`, `rohr_entity_alive_check`.
- `rohr_entity_alive_count_get`, `rohr_entity_by_name_get`, `rohr_entity_children_get`.
- `rohr_entity_components_check`, `rohr_entity_from_index_get`, `rohr_entity_group_by_name_get`.
- `rohr_entity_group_entity_check`, `rohr_entity_group_get`, `rohr_entity_group_name_get`.
- `rohr_entity_groups_get`, `rohr_entity_index_alive_check`, `rohr_entity_index_components_check`.
- `rohr_entity_index_get`, `rohr_entity_info_check`, `rohr_entity_info_get`.
- `rohr_entity_info_issues_count_get`, `rohr_entity_info_issues_get`, `rohr_entity_info_release`.
- `rohr_entity_info_retain`, `rohr_entity_name_get`, `rohr_entity_object_anchor_get`.
- `rohr_entity_object_camera_get`, `rohr_entity_object_member_by_name_get`, `rohr_entity_object_member_name_get`.
- `rohr_entity_object_members_get`, `rohr_entity_owner_get`, `rohr_entity_parent_get`.
- `rohr_graphics_aabb_tree_debug_check`, `rohr_graphics_animated_sprite_body_offset_get`, `rohr_graphics_animated_sprite_frame_index_get`.
- `rohr_graphics_animated_sprite_frame_offset_get`, `rohr_graphics_animated_sprite_frame_rotation_get`, `rohr_graphics_animated_sprite_frame_rotation_radians_get`.
- `rohr_graphics_animated_sprite_frame_scale_get`, `rohr_graphics_animated_sprite_orientation_offset_get`, `rohr_graphics_animated_sprite_orientation_offset_radians_get`.
- `rohr_graphics_animation_info_get`, `rohr_graphics_animation_valid_check`, `rohr_graphics_camera_attached_get`.
- `rohr_graphics_contacts_debug_check`, `rohr_graphics_font_default_get`, `rohr_graphics_font_valid_check`.
- `rohr_graphics_layer_active_get`, `rohr_graphics_layer_animation_get`, `rohr_graphics_layer_animation_id_get`.
- `rohr_graphics_layer_entity_get`, `rohr_graphics_layer_entity_id_get`, `rohr_graphics_layer_get`.
- `rohr_graphics_layer_name_get`, `rohr_graphics_layer_name_id_get`, `rohr_graphics_layer_sprite_get`.
- `rohr_graphics_layer_sprite_id_get`, `rohr_graphics_layer_ui_get`, `rohr_graphics_layer_ui_id_get`.
- `rohr_graphics_mouse_screen_position_get`, `rohr_graphics_render_output_size_get`, `rohr_graphics_screen_to_world_get`.
- `rohr_graphics_sprite_body_offset_get`, `rohr_graphics_sprite_orientation_offset_get`, `rohr_graphics_sprite_orientation_offset_radians_get`.
- `rohr_graphics_text_valid_check`, `rohr_graphics_texture_size_get`, `rohr_graphics_texture_valid_check`.
- `rohr_graphics_ui_slider_value_get`, `rohr_graphics_world_to_screen_get`, `rohr_input_action_axis_1d_get`.
- `rohr_input_action_axis_2d_get`, `rohr_input_action_bindings_default_get`, `rohr_input_action_bindings_effective_get`.
- `rohr_input_action_bindings_override_check`, `rohr_input_action_bindings_override_get`, `rohr_input_action_button_down_check`.
- `rohr_input_action_button_initial_state_check`, `rohr_input_action_button_mode_get`, `rohr_input_action_button_pressed_check`.
- `rohr_input_action_button_released_check`, `rohr_input_action_by_name_get`, `rohr_input_action_type_get`.
- `rohr_input_binding_valid_check`, `rohr_input_controller_by_name_get`, `rohr_input_controller_enabled_check`.
- `rohr_input_key_down_check`, `rohr_input_key_pressed_check`, `rohr_input_key_released_check`.
- `rohr_input_modifiers_get`, `rohr_input_mouse_button_down_check`, `rohr_input_mouse_button_pressed_check`.
- `rohr_input_mouse_button_released_check`, `rohr_input_mouse_delta_get`, `rohr_input_mouse_position_get`.
- `rohr_input_mouse_relative_mode_check`, `rohr_input_mouse_wheel_get`, `rohr_input_text_state_get`.
- `rohr_physics_angular_velocity_get`, `rohr_physics_angular_velocity_maximum_get`, `rohr_physics_angular_velocity_maximum_radians_get`.
- `rohr_physics_angular_velocity_radians_get`, `rohr_physics_center_of_mass_automatic_check`, `rohr_physics_center_of_mass_local_position_get`.
- `rohr_physics_center_of_mass_world_position_get`, `rohr_physics_collision_between_check`, `rohr_physics_contact_check`.
- `rohr_physics_contact_count_get`, `rohr_physics_contact_entered_check`, `rohr_physics_contact_exited_check`.
- `rohr_physics_contact_get`, `rohr_physics_contact_stayed_check`, `rohr_physics_contact_total_impulse_get`.
- `rohr_physics_contacts_get`, `rohr_physics_dt_per_tick_get`, `rohr_physics_entity_held_get`.
- `rohr_physics_force_location_get`, `rohr_physics_force_magnitude_get`, `rohr_physics_force_orientation_get`.
- `rohr_physics_force_target_get`, `rohr_physics_global_hit_box_get`, `rohr_physics_gravity_check`.
- `rohr_physics_gravity_get`, `rohr_physics_hitbox_active_id_get`, `rohr_physics_hitbox_active_index_get`.
- `rohr_physics_hitbox_animation_binding_at_get`, `rohr_physics_hitbox_animation_binding_get`, `rohr_physics_hitbox_at_get`.
- `rohr_physics_hitbox_by_id_get`, `rohr_physics_hitbox_count_get`, `rohr_physics_hitbox_get`.
- `rohr_physics_hitbox_id_at_get`, `rohr_physics_joint_anchor_local_position_get`, `rohr_physics_joint_anchor_world_position_get`.
- `rohr_physics_joint_anchors_get`, `rohr_physics_kinematic_driven_check`, `rohr_physics_mass_check`.
- `rohr_physics_mass_get`, `rohr_physics_moment_of_inertia_get`, `rohr_physics_overlap_check`.
- `rohr_physics_overlap_count_get`, `rohr_physics_overlap_entered_check`, `rohr_physics_overlap_exited_check`.
- `rohr_physics_overlap_get`, `rohr_physics_overlap_stayed_check`, `rohr_physics_overlaps_get`.
- `rohr_physics_particle_origin_get`, `rohr_physics_particle_overlap_get`, `rohr_physics_particle_radius_get`.
- `rohr_physics_position_get`, `rohr_physics_sat_overlap_get`, `rohr_physics_soft_body_area_get`.
- `rohr_physics_soft_body_beam_get`, `rohr_physics_soft_body_get`, `rohr_physics_soft_body_node_get`.
- `rohr_physics_soft_body_node_local_position_get`, `rohr_physics_solver_iterations_get`, `rohr_physics_substeps_get`.
- `rohr_physics_torque_magnitude_get`, `rohr_physics_torque_target_get`, `rohr_physics_world_position_check`.
- `rohr_ui_button_by_name_get`, `rohr_ui_button_style_default_get`, `rohr_ui_font_by_name_get`.
- `rohr_ui_key_pressed_check`, `rohr_ui_label_by_name_get`, `rohr_ui_navigation_focus_bounds_get`.
- `rohr_ui_pointer_consumed_get`, `rohr_ui_primary_pressed_check`, `rohr_ui_slider_by_name_get`.
- `rohr_ui_slider_config_default_get`, `rohr_viewport_item_drag_mode_get`, `rohr_viewport_item_dragging_check`.
- `rohr_viewport_item_viewport_get`, `rohr_viewport_ui_hovered_check`, `rohr_viewport_ui_pressed_check`.
- `rohr_viewport_ui_slider_changed_check`.

**EO — Operation, lifecycle, execution, or drawing**

- `rohr_audio_music_destroy`, `rohr_audio_music_pause`, `rohr_audio_music_play`.
- `rohr_audio_music_resume`, `rohr_audio_music_stop`, `rohr_audio_sound_destroy`.
- `rohr_audio_sound_play`, `rohr_audio_sound_stop`, `rohr_audio_start`.
- `rohr_audio_stop`, `rohr_audio_update`, `rohr_camera_destroy`.
- `rohr_camera_detach`, `rohr_camera_position_from_entity_set`, `rohr_camera_position_move`.
- `rohr_camera_position_set`, `rohr_camera_zoom_set`, `rohr_console_debug_write`.
- `rohr_console_logs_print`, `rohr_console_read`, `rohr_console_start`.
- `rohr_console_stop`, `rohr_console_write`, `rohr_engine_clock_reset`.
- `rohr_engine_event_poll`, `rohr_engine_lifecycle_update`, `rohr_engine_pause`.
- `rohr_engine_resume`, `rohr_engine_start`, `rohr_engine_stop`.
- `rohr_engine_tick_update`, `rohr_engine_time_update`, `rohr_engine_update`.
- `rohr_entity_add`, `rohr_entity_child_remove`, `rohr_entity_components_add`.
- `rohr_entity_components_delete`, `rohr_entity_delete`, `rohr_entity_group_add`.
- `rohr_entity_group_create`, `rohr_entity_group_destroy`, `rohr_entity_group_remove`.
- `rohr_entity_life_time_remove`, `rohr_entity_object_draw`, `rohr_entity_parent_remove`.
- `rohr_game_state_file_load`, `rohr_game_state_file_save`, `rohr_game_state_files_load`.
- `rohr_game_state_template_file_save`, `rohr_graphics_aabb_tree_draw`, `rohr_graphics_animated_sprite_draw`.
- `rohr_graphics_animated_sprite_update`, `rohr_graphics_animated_sprites_draw`, `rohr_graphics_animation_player_update`.
- `rohr_graphics_animation_release`, `rohr_graphics_animation_retain`, `rohr_graphics_background_draw`.
- `rohr_graphics_camera_detach`, `rohr_graphics_camera_move`, `rohr_graphics_camera_radians_rotate`.
- `rohr_graphics_camera_rotate`, `rohr_graphics_contacts_draw`, `rohr_graphics_events_poll`.
- `rohr_graphics_font_release`, `rohr_graphics_font_retain`, `rohr_graphics_hit_box_colored_draw`.
- `rohr_graphics_hit_box_draw`, `rohr_graphics_hit_boxes_draw`, `rohr_graphics_joint_draw`.
- `rohr_graphics_joints_draw`, `rohr_graphics_layer_animation_clear`, `rohr_graphics_layer_destroy`.
- `rohr_graphics_layer_entity_clear`, `rohr_graphics_layer_sprite_clear`, `rohr_graphics_layer_ui_clear`.
- `rohr_graphics_local_origins_draw`, `rohr_graphics_particle_draw`, `rohr_graphics_particles_draw`.
- `rohr_graphics_pipeline_cameras_update`, `rohr_graphics_pipeline_commands_execute`, `rohr_graphics_pipeline_present`.
- `rohr_graphics_pipeline_recording_update`, `rohr_graphics_pipeline_viewports_draw`, `rohr_graphics_recording_start`.
- `rohr_graphics_recording_stop`, `rohr_graphics_screen_clip_clear`, `rohr_graphics_screen_clip_pop`.
- `rohr_graphics_screen_clip_push`, `rohr_graphics_screen_clip_set`, `rohr_graphics_screen_quad_draw`.
- `rohr_graphics_screen_quad_radians_draw`, `rohr_graphics_screen_rect_draw`, `rohr_graphics_screen_shape_filled_draw`.
- `rohr_graphics_screen_text_scaled_rotated_draw`, `rohr_graphics_screen_text_scaled_rotated_radians_draw`, `rohr_graphics_screen_texture_draw`.
- `rohr_graphics_screen_texture_radians_draw`, `rohr_graphics_show`, `rohr_graphics_soft_body_area_color_clear`.
- `rohr_graphics_soft_body_draw`, `rohr_graphics_sprite_draw`, `rohr_graphics_sprite_frames_update`.
- `rohr_graphics_sprites_draw`, `rohr_graphics_start`, `rohr_graphics_stop`.
- `rohr_graphics_text_destroy`, `rohr_graphics_text_draw`, `rohr_graphics_text_scaled_draw`.
- `rohr_graphics_texture_draw`, `rohr_graphics_texture_radians_draw`, `rohr_graphics_texture_release`.
- `rohr_graphics_texture_retain`, `rohr_graphics_textures_scale`, `rohr_graphics_ui_destroy`.
- `rohr_input_action_bindings_override_clear`, `rohr_input_action_button_state_reset`, `rohr_input_action_destroy`.
- `rohr_input_binding_key_create`, `rohr_input_binding_scancode_create`, `rohr_input_controller_destroy`.
- `rohr_input_frame_begin`, `rohr_input_text_area_set`, `rohr_input_text_start`.
- `rohr_input_text_stop`, `rohr_object_copy`, `rohr_object_destroy`.
- `rohr_object_load`, `rohr_physics_acceleration_away_from_entity_set`, `rohr_physics_acceleration_away_from_position_set`.
- `rohr_physics_acceleration_toward_entity_set`, `rohr_physics_acceleration_toward_position_set`, `rohr_physics_angular_impulse_apply`.
- `rohr_physics_axis_lock_remove`, `rohr_physics_dt_update`, `rohr_physics_engine_time_per_tick_use`.
- `rohr_physics_entity_hold`, `rohr_physics_entity_stop`, `rohr_physics_entity_unhold`.
- `rohr_physics_force_apply`, `rohr_physics_gravity_disable`, `rohr_physics_gravity_enable`.
- `rohr_physics_group_acceleration_away_from_entity_set`, `rohr_physics_group_acceleration_toward_entity_set`, `rohr_physics_group_entities_hold`.
- `rohr_physics_group_entities_stop`, `rohr_physics_group_entities_unhold`, `rohr_physics_group_velocity_away_from_entity_set`.
- `rohr_physics_group_velocity_toward_entity_set`, `rohr_physics_hitbox_add`, `rohr_physics_hitbox_animation_binding_at_remove`.
- `rohr_physics_hitbox_animation_binding_remove`, `rohr_physics_hitbox_at_remove`, `rohr_physics_hitbox_autoshape_circle_set`.
- `rohr_physics_hitbox_autoshape_rectangle_set`, `rohr_physics_hitbox_autoshape_triangle_set`, `rohr_physics_hitbox_by_id_remove`.
- `rohr_physics_hitbox_remove`, `rohr_physics_impulse_apply`, `rohr_physics_joint_anchor_remove`.
- `rohr_physics_joint_place`.
- `rohr_physics_kinematic_driven_remove`, `rohr_physics_mass_remove`, `rohr_physics_pipeline_accelerations_clear`.
- `rohr_physics_pipeline_constraints_solve`, `rohr_physics_pipeline_contacts_gather`, `rohr_physics_pipeline_forces_apply`.
- `rohr_physics_pipeline_gravity_apply`, `rohr_physics_pipeline_integrate`, `rohr_physics_pipeline_joints_gather`.
- `rohr_physics_pipeline_step_begin`, `rohr_physics_pipeline_substep`, `rohr_physics_pipeline_substep_begin`.
- `rohr_physics_pipeline_update`, `rohr_physics_shape_world_radians_translate`, `rohr_physics_shape_world_translate`.
- `rohr_physics_soft_body_autoshape_circle_set`, `rohr_physics_soft_body_autoshape_rectangle_set`, `rohr_physics_soft_body_autoshape_triangle_set`.
- `rohr_physics_soft_body_beam_collision_disable`, `rohr_physics_soft_body_beam_collision_enable`, `rohr_physics_soft_body_node_impulse_apply`.
- `rohr_physics_torque_apply`, `rohr_physics_transform_lock_current_transform_set`, `rohr_physics_transform_lock_remove`.
- `rohr_physics_update`, `rohr_physics_velocity_away_from_entity_set`, `rohr_physics_velocity_away_from_position_set`.
- `rohr_physics_velocity_toward_entity_set`, `rohr_physics_velocity_toward_position_set`, `rohr_screen_destroy`.
- `rohr_system_physics_update`, `rohr_ui_border`, `rohr_ui_button`.
- `rohr_ui_button_disabled`, `rohr_ui_clip_begin`, `rohr_ui_clip_end`.
- `rohr_ui_component_bounds_get`, `rohr_ui_content`, `rohr_ui_dropdown`.
- `rohr_ui_dropdown_actions`, `rohr_ui_event_add`, `rohr_ui_field`.
- `rohr_ui_field_event_add`, `rohr_ui_field_focus_clear`, `rohr_ui_input_set`.
- `rohr_ui_interaction`, `rohr_ui_label`, `rohr_ui_menu`.
- `rohr_ui_modal_controls_begin`, `rohr_ui_modal_controls_end`, `rohr_ui_modal_set`.
- `rohr_ui_multiline_field`, `rohr_ui_navigation_activate`, `rohr_ui_navigation_move`.
- `rohr_ui_quad`, `rohr_ui_radians_quad`, `rohr_ui_scroll_region_begin`.
- `rohr_ui_scroll_region_end`, `rohr_ui_slider`, `rohr_ui_slider_with_text`.
- `rohr_ui_surface`, `rohr_ui_translation_y_pop`, `rohr_ui_translation_y_push`.
- `rohr_ui_update`, `rohr_viewport_camera_clear`, `rohr_viewport_destroy`.
- `rohr_viewport_item_remove`.

**EM — Pure math or utility calculation**

- `rohr_graphics_color_hex_create`, `rohr_math_aabb_create`, `rohr_math_angular_velocity_cross_vec`.
- `rohr_math_angular_velocity_radians_cross_vec`, `rohr_math_axis_magnitude`, `rohr_math_circle_create`.
- `rohr_math_circle_overlap_depth`, `rohr_math_circle_radius`, `rohr_math_cross_2d`.
- `rohr_math_degrees_to_radians`, `rohr_math_dot_product`, `rohr_math_normals_create`.
- `rohr_math_polygon_centroid`, `rohr_math_project_onto_axis`, `rohr_math_project_shape_on_axis`.
- `rohr_math_projection_overlap`, `rohr_math_radians_to_degrees`, `rohr_math_shape_scale`.
- `rohr_math_shape_x_scale`, `rohr_math_shape_y_scale`, `rohr_math_square_create`.
- `rohr_math_vector_magnitude`, `rohr_math_vector_normalize`, `rohr_math_vector_radians_rotate`.
- `rohr_math_vector_rotate`, `rohr_math_vector_subtract`, `rohr_math_vectors_normalize`.
- `rohr_math_vertex_add`, `rohr_math_vertex_delete`, `rohr_object_copy_with_options`.
- `rohr_physics_circle_moment_of_inertia`, `rohr_physics_polygon_moment_of_inertia`, `rohr_tools_append_string`.
- `rohr_tools_binary_to_string`, `rohr_tools_delay`, `rohr_tools_random_range`.
- `rohr_tools_random_range_float`, `rohr_tools_sizeof_string`.

**ES — Internal storage, reporting, or macro machinery**

- `rohr_error_check`, `rohr_error_code_message_get`, `rohr_error_message_get`.
- `rohr_error_result_error`, `rohr_error_result_value`, `rohr_physics_hitbox_animation_bindings_update`.
- `rohr_report_console_write`, `rohr_report_create`, `rohr_report_diagnostic_add`.
- `rohr_report_diagnostic_detail_set`, `rohr_report_entry_add`, `rohr_report_get`.
- `rohr_report_header_set`, `rohr_report_json_write`, `rohr_report_merge`.
- `rohr_report_statistic_add`, `rohr_report_text_write`, `rohr_report_valid_check`.

</details>

<details>
<summary>rohr_tools.h: 4 entries</summary>

Source: [rohr_tools.h](../include/rohr_tools.h).

**ET — Build-time tooling**

- `rohr_tools_component_registry_component_add`, `rohr_tools_component_registry_generate`, `rohr_tools_component_registry_init`.
- `rohr_tools_component_registry_tag_add`.

</details>

<details>
<summary>scene.h: 2 entries</summary>

Source: [scene.h](../include/scene.h).

**EO — Operation, lifecycle, execution, or drawing**

- `rohr_scene_run`, `scene_run`.

</details>

<details>
<summary>systems.h: 1 entries</summary>

Source: [systems.h](../include/systems.h).

**EO — Operation, lifecycle, execution, or drawing**

- `system_physics_update`.

</details>

<details>
<summary>tools.h: 6 entries</summary>

Source: [tools.h](../include/tools.h).

**EM — Pure math or utility calculation**

- `binary_to_string`, `delay`, `tool_sizeof_string`.
- `tools_append_string`, `tools_random_range`, `tools_random_range_float`.

</details>

<details>
<summary>ui.h: 40 entries</summary>

Source: [ui.h](../include/ui.h).

**EQ — Query, validation, snapshot, or result access**

- `ui_button_style_default_get`, `ui_key_pressed_check`, `ui_navigation_focus_bounds_get`.
- `ui_pointer_consumed_get`, `ui_primary_pressed_check`, `ui_slider_config_default_get`.

**EO — Operation, lifecycle, execution, or drawing**

- `ui_border`, `ui_button`, `ui_button_disabled`.
- `ui_clip_begin`, `ui_clip_end`, `ui_component_bounds_get`.
- `ui_content`, `ui_dropdown`, `ui_dropdown_actions`.
- `ui_event_add`, `ui_field`, `ui_field_event_add`.
- `ui_field_focus_clear`, `ui_field_focus_set`, `ui_input_set`.
- `ui_interaction`, `ui_label`, `ui_menu`.
- `ui_modal_controls_begin`, `ui_modal_controls_end`, `ui_modal_set`.
- `ui_multiline_field`, `ui_navigation_activate`, `ui_navigation_move`.
- `ui_quad`, `ui_radians_quad`, `ui_scroll_region_begin`.
- `ui_scroll_region_end`, `ui_slider`, `ui_slider_with_text`.
- `ui_surface`, `ui_translation_y_pop`, `ui_translation_y_push`.
- `ui_update`.

</details>

### Macro-generated storage interfaces

`MEMORY_DECLARE_OBJECT_POOL` and `MEMORY_DEFINE_OBJECT_POOL` generate the
following methods for each pool prefix below. Every method is **ES: intentionally
exempt**, because it manages typed storage, not resource configuration:
`_init`, `_expand`, `_defrag`, `_shrink`, `_clear`, `_destroy`, `_acquire`, `_store`,
`_store_at`, `_release_at`, `_release`. The two declaration macros themselves
are listed in the header inventory. Result constructors and token-pasting
internals are support machinery, not additional configuration APIs.

- `entity_components.h`: `EntityMaskPool`, `GroupEntityPool`, `GroupIdPool`, `ParentPool`, `ChildrenPool`, `EntityGroupMembershipPool`, `EntityGroupPool`, `LifeTimePool`, `TargetPool`, `EntityNamePool`.
- `graphics.h`: `SpritePool`, `AnimatedSpritePool`.
- `physics.h`: `PositionPool`, `ParticleGeometryPool`, `VelocityPool`, `AccelerationPool`, `MassPool`, `ForcePool`, `ShapePool`, `HitboxVariantListPool`, `HitboxAnimationBindingListPool`, `CollisionFilterDescriptionPool`, `OrientationPool`, `AngularVelocityPool`, `AngularAccelerationPool`, `TorquePool`, `FrictionPool`, `RestitutionPool`, `AngleLockPool`, `AxisLockPool`, `TransformLockPool`, `JointPool`, `SoftBodyPool`, `SoftBodyNodePool`, `SoftBodyBeamPool`.

## Final milestone verification

The integrated verification covers the following:

| Check | Result |
| --- | --- |
| Public inventory | 20 installed headers, 1,602 classified entries, three exempt Config names; no remaining migration classifications |
| Permanent audit | CTest integration and thirteen validation cases; independent prototype extraction and exact symbol/category counts |
| Native suite | All 105 tests pass, including the installed-SDK driver executing twelve consumers |
| Sanitizers | Nineteen migration regressions plus two refreshed soft-body example regressions pass with ASan, UBSan, and leak detection |
| Platforms | Linux and Windows cross-builds and installed-SDK consumer builds pass; native consumers execute on Linux |
| Standalone examples | Twelve independent SDK examples build on both platforms and launch on Linux |
| Generated projects | Center-of-mass, player-controller, and soft-body outputs match regeneration; their custom mains are preserved; both-platform builds and Linux launches pass |
| Fresh starter and parity fixtures | A newly created starter and generated COM/area fixtures build on both platforms and execute on Linux |
| External project | Custom main unchanged; hosted scene and application launch pass; both forces toggle at 2, 4, 6, and 8 seconds |

The soft-body example had stale generated instance-binding/settings helpers and
marker metadata. Its generated modules now match the authored project, including
app-visible joint markers and current marker colors/categories. Editor-hidden
flags remain separate. Physics values, authored JSON, and the developer-owned
main are unchanged. The refreshed output passes authored/runtime parity and
control regressions.

Standard builds report no new warnings. Strict example builds retain Pong's
existing unused generated helper warning. Native Windows execution and real
desktop visual/interaction review remain unverified; cross-compilation and dummy
SDL execution do not claim those results. The inventory checker validates names
and classifications, not ABI compatibility or semantic test coverage of every
public function.

## Keeping this inventory current

When an API changes, update its symbols/types, classification, intentional
exemptions, and header/category counts. Check declarations, facade macros, and
direct aliases against the same boundary; argument count alone is not a
classification rule. Run `perl scripts/verify_api_inventory.pl` or the
`api_inventory` CTest check. The separate `api_inventory_validation` test proves
that representative inventory regressions fail with useful diagnostics.

The audit recursively covers the installed `include/` headers, counts callable
macros and facade aliases, cross-checks prototype extraction, and verifies exact
per-header/classification/global totals and documented Config/Descriptor types.
It checks local file links but does not validate anchor fragments or C signatures.
Compilation and native execution of the installed-SDK consumers provide the
complementary interface and runtime checks. See the [workflow](README.md#api-inventory-check).

## Authored configuration and explicit placement

Milestone 4 goal 2 adds initial configuration to `ObjectDescription` without
changing full-replacement runtime setters: omitted description members still
receive their documented zero/default values; focused setters change only their
property. `ObjectEntitySettings` is copied independently for each spawn.

- Lifetimes have independent seconds/tick enable flags. Authored values default
  to durations after spawn; each can instead use the absolute engine clock.
  A spawn captures its starting time/tick once for all members. Enabled zero or
  past deadlines expire on the next lifecycle update. Tick addition overflow
  rejects and rolls back the spawn. Initial forces/torques still expire no later
  than the next engine tick.
- Gameplay targets and transform drivers are non-owning references within one
  top-level authored object. Nested node/beam/area references also identify their
  soft body. Catalog compilation remaps these identities to description IDs;
  spawning resolves them only after all members exist. Removing a referenced
  record clears references; undo restores them. Duplicating an object remaps its
  self-reference. Cross-instance connections and ownership reassignment remain
  explicit gameplay C operations, independent of joint/beam endpoints.
- Axis locks default to top-level object coordinates and may instead be world
  fixed. Transform offsets are driver-local. Angles, angular acceleration, and
  angular-velocity limits use degrees. A transform driver does not become owner.
  The editor exposes initial angular configuration on rotational rigid bodies.
- Static nodes are explicit. Beams default to automatic measured rest length;
  enabling Explicit freezes the current endpoint distance, and editing the
  length enables Explicit. Switching back to Auto measures at construction.
- Cameras independently follow position and orientation and optionally pause
  with the engine. When position following is disabled, the authored position
  remains in object coordinates, independent of target motion. World setters
  continue to edit offsets while preserving attachments.
- Camera mounts optionally specify a render resolution separate from their
  viewport placement size. The default remains the camera dimensions. Custom
  fonts default to 12 points; text defaults to wrap width 0 and rotation 0.
  Text rotation composes with the existing mount/content transforms.
- Negative static-sprite width/height flip the corresponding image axis. Width
  and height must be finite and nonzero. A negative width with the generated
  `DIRECTION_RIGHT` is visually equivalent to positive width with
  `DIRECTION_LEFT`; both flips compose with rotation and attachment offsets.
  Editor picking and bounds use absolute dimensions.

The shared entity accordions follow each item's existing controls. Settings →
Application configures the generated application's window and named audio
recipes separately from editor-window preferences. Applying window settings is
opt-in for legacy projects. Recipes expose `project_audio.sounds.<name>` as
`SoundDescription` and `project_audio.music.<name>` as `MusicDescription`;
volume defaults to 0 and playback rate to 1. They load nothing, own no player,
and never autoplay. Gameplay passes a recipe to the existing audio APIs.
Names must be valid non-keyword generated identifiers, unique within sound or
music definitions. Generation preserves developer-owned `main.c`.

Project format 11 stores these settings. Only format 11 is accepted; older and
future versions fail without replacing the current project. Optional fields keep
their documented defaults. Saved joint rest values remain explicit. Configured objects use the canonical catalog spawning path even from
legacy generated `<object>_create` convenience functions; the latter's position
argument remains the requested absolute object origin. Existing catalog spawn
options retain their relative-offset semantics.

### Joint placement

Creating/reconfiguring a joint, changing its type, or selecting endpoints does
not place bodies. Use the joint editor's **Place A Relative to B**, the CLI
`joint place <project-file> <object-id> <joint-id>`, or
`rohr_physics_joint_place(joint)` explicitly.

Placement translates the first anchor's pin/weld-connected group toward the
second. Welds also satisfy the stored rest angle; springs use their rest distance
along B-to-A (world +Y when coincident). A soft body moves as a whole. The selected
joint is excluded from graph expansion; a second path back to B, a fixed world
anchor, a static/held member, or an incompatible lock rejects the entire change.
An already satisfied placement succeeds without mutation. Owned attachment
helpers follow their driver; unrelated locked entities cannot be displaced by
this action. B stays fixed. Geometry, rest values, motion, ownership, and
connections are preserved. Scratch allocation and all validation precede commit.
Editor placement is one undoable object edit. World-anchor proxies are explicitly
static in both catalog and legacy generated construction.

A node-attached anchor with zero local offset plus a pin joint expresses
`SoftBodyNodeAnchorPinDescription`: the node owns its anchor, and the selected
owner owns the joint. A separate editor item is unnecessary.

### CLI configuration examples

The existing selector form accepts shared entity properties, for example:

```sh
rohr-cli --project objects/project.rohr.json --object-id 1 --body-id 1 --property lifetime-time-enabled true
rohr-cli --project objects/project.rohr.json --object-id 1 --body-id 1 --property lifetime-time 2.5
```

The command log can also emit the positional form. References use `none`,
`object:1`, `body:2`, or `node:3:4` (node ID 3 in soft body ID 4), with analogous
kind names for other records. Tick values use exact unsigned 64-bit parsing.
New project-level commands use these positional forms:

```sh
rohr-cli audio add objects/project.rohr.json false impact assets/impact.wav
rohr-cli application-window set objects/project.rohr.json true 0 1280 720 640 360 true
rohr-cli ui-font set objects/project.rohr.json 0 1 point-size 24
rohr-cli ui-definition set objects/project.rohr.json 0 1 wrap-width 160
rohr-cli viewport-camera set objects/project.rohr.json 1 1 render-width 320
rohr-cli viewport-camera set objects/project.rohr.json 1 1 render-height 200
rohr-cli viewport-camera set objects/project.rohr.json 1 1 render-size-explicit true
```

For UI definitions/fonts the object slot is unused (0); camera mounts use viewport
ID then mount ID. Audio `set` replaces the complete recipe in the order
`id name path music volume pan loop playback-rate`; removal takes its ID.
Use the shared property definitions in `editor_entity_settings.h` for the full
entity-property spelling list. Audio triggers, runtime snapshot extensions, and
named-layer registry restoration remain separate roadmap work.
