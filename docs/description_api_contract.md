# Description API Contract and Migration Inventory

Status: Goals 1–4 of **Consistent Description APIs** are implemented.
Entity metadata, lifetime, authored configuration, spawning, and physics
configuration and standalone-particle descriptions are shipped. Names and missing interfaces described
as planned below are not declarations of shipped functionality. The symbol
inventory records the headers as they exist at this goal boundary.

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

Before each remaining goal, ask the user for any material clarifications and
resolve them before implementation. Do not advance goals automatically.

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

These are future migration requirements, not claims that every current full
setter already meets them. For example, the current camera value setter clears
attachments and active motion; Goal 10 must remove unrelated resets. Existing
soft-body creation descriptions lack matching full configuration interfaces;
Goal 5 completes that family.

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

## Migration families and intended interfaces

The following names are planned except where explicitly marked shipped. For a
listed description, “full set/get” means the appropriate existing family prefix
plus `_description_set/get`, with matching typed results and default getter when
needed. Retain the existing family prefix in both direct and facade APIs; this
is not a wholesale namespace rename. Exact current symbols, including aliases,
appear in the header inventory below.

| Family / owning goal | Current configuration types | Planned interface and gaps | Preservation requirements |
| --- | --- | --- | --- |
| Entity metadata and lifetime / 2 — shipped | `EntityDescription`, `EntityLifetimeDescription`; `LifeTime` remains storage | `entity_description_add/set/get` and `entity_lifetime_description_set/get`; retain bare `entity_add()` and focused name/owner/removal APIs. | Copied bounded names; atomic ownership/name replacement; explicit deadline enable flags; no implicit ownership from connections. |
| Authored objects / 2 — shipped | `ObjectDescription`, `ObjectSpawnDescription`, `ObjectMaterialDescription`, `ObjectCollisionDescription`, `ObjectColorDescription` | `object_*_description_set/get`, `object_spawn_description_default_get`, and existing dot catalogs. Force/torque descriptions remain compliant. | Immutable getters; setters require editable copies and affect future spawns. Color includes explicit override state. No live hierarchy replacement setter. |
| Physics configuration / 3 — shipped | `TransformLockDescription`, `AxisLockDescription`, `CollisionFilterDescription`; solver lock storage remains separate | Full lock set/get, explicit removal, and collision-filter full/default accessors. Nested filter types use the common description. | Degrees only in descriptions; explicit conversions for radians. Lock edits preserve current motion, ownership, and parenting; capture-current-transform stays direct. Missing locks return component-missing. |
| Particles / 4 — shipped | `ParticleDescription` | Standalone creation and full `physics_particle_description_set/get`, with the existing default getter. | Current configurable state and explicit owner replacement; atomic circle/polygon synchronization; static configuration retained; hybrid bodies and soft nodes use their own APIs. |
| Soft bodies and anchors / 5 | `SoftBodyDescription`, `SoftBodyNodeDescription`, `SoftBodyBeamDescription`, `SoftBodyAreaDescription`, `JointAnchorDescription`, `SoftBodyBeamCollisionConfig` | Complete existing description families with full set/get. Extend beam configuration to its stored rest/material/collision properties; use `SoftBodyBeamCollisionDescription` for the full collision group. Reuse `CollisionFilterDescription` for node/beam full filters. | Keep world/local creation variants explicit. Container edits do not reconstruct children; owner changes preserve connections. Beam getters preserve resolved rest values; automatic measurement happens only when requested. Anchor replacement updates references transactionally without implicit reownership of joints. |
| Shared assets and text / 6 | `TextureDescriptor`, `FontDescriptor`, `AnimationDescriptor`; positional text creation | Rename loading types to `TextureDescription`, `FontDescription`, `AnimationDescription`; add missing description/metadata retrieval for immutable inputs. Add `TextDescription` create/full set/get for font/value/color/wrapping. | No shared-resource mutation. Retain unique text ownership, queued payload revisions, immutable animation IDs/timing, and explicit borrowed metadata lifetime. |
| Sprites, frames, players / 7 | `Sprite`, `AnimatedSprite`, `AnimationFrame`, `AnimationPlayer` mix configuration with runtime values | Add `SpriteDescription`, `AnimatedSpriteDescription`, `AnimationFrameDescription`, `AnimationPlayerDescription` creation/full set/get; retain runtime state/result types where needed. Catalog frame retrieval uses the common frame description without exposing a catalog setter. | Keep frame IDs, empty animations, instance transforms, texture/catalog reference ownership, and independent playback. Ordinary visual edits preserve progress. |
| Audio / 8 | `SoundConfig`, `MusicConfig` | Rename to `SoundDescription`, `MusicDescription`; standardize defaults and add `audio_sound_description_set/get`, `audio_music_description_set/get`. | Volume/pan/loop/rate edits preserve progress; failed source replacement preserves playback and references; successful source replacement is stopped. Callback/reporting architecture is unchanged. |
| Logical input / 9 | Positional controller/action creation; `InputBinding`, `InputBindingList` | Add `InputControllerDescription`, `InputActionDescription` creation/full set/get. Include authored action behavior/default bindings; retain focused override-list/state APIs. | Controller-local name uniqueness, ownership, defaults versus overrides, and type/mode transitions remain explicit. No controller child-action rebuild or synthetic presses from unrelated edits. |
| Cameras and presentation / 10 | `CameraConfig`, `Camera`, `CameraAttachment`, `GraphicsWindowPresentationConfig` | Use `CameraDescription`, `CameraAttachmentDescription`, `GraphicsWindowPresentationDescription` with complete/default accessors. Retain resolved camera queries if needed separately from configuration retrieval. Include callback/context in full configuration while retaining its focused setter. | Unrelated edits preserve attachments and motion. Changing a driven transform/zoom supersedes only the affected motion channel. Timed commands stay direct. Global presentation needs full set/get, not an artificial resource constructor. |
| Screens, viewports, layers, persistent UI / 11 | `ScreenConfig`, `ViewportConfig`, `ViewportItemConfig`, `ViewportUiShapeConfig`, `ViewportUiTextConfig`, `ViewportUiSliderConfig`; positional layer creation | Rename to corresponding `*Description` types; add missing screen/viewport/layer full accessors using `GraphicsLayerDescription`; standardize existing persistent UI accessors. Mount descriptions include content references alongside placement. | Retain handles, mounted children, global layer identity, clipping/composition, resource references, and unrelated interaction state. Creating a mount must still distinguish existing reusable content from convenience creation of new content. |
| Final verification / 12 | All migrated families | Reconcile inventory and examples; no independent new configuration family. | Verify the complete installed SDK and generated/example parity, including the external force-toggle example. |

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
field. Soft-node/beam specialized full accessors remain assigned to Goal 5.

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

The current inventory contains **1482** classified declaration/macro entries.

| Classification | Meaning | Owning goal | Entries |
| --- | --- | --- | --- |
| M5: Migration required | Soft-body and anchor configuration | 5 | 18 |
| M6: Migration required | Asset loading and text configuration | 6 | 8 |
| M7: Migration required | Sprite, frame, and player configuration | 7 | 24 |
| M8: Migration required | Audio configuration | 8 | 8 |
| M9: Migration required | Logical input configuration | 9 | 4 |
| M10: Migration required | Camera and presentation configuration | 10 | 28 |
| M11: Migration required | Screen, viewport, layer, and persistent UI configuration | 11 | 46 |
| C: Compliant | Existing description interface | — | 88 |
| EV: Intentionally exempt | Focused/basic-value mutation or lookup | — | 275 |
| EQ: Intentionally exempt | Query, validation, snapshot, or result access | — | 398 |
| EO: Intentionally exempt | Operation, lifecycle, execution, or drawing | — | 456 |
| EM: Intentionally exempt | Pure math or utility calculation | — | 77 |
| ES: Intentionally exempt | Internal storage, reporting, or macro machinery | — | 48 |
| ET: Intentionally exempt | Build-time tooling | — | 4 |

The per-header lists are exhaustive. Every declaration/alias is classified once.

<details>
<summary>audio.h: 37 entries</summary>

Source: [audio.h](../include/audio.h).

**M8 — Audio configuration (Goal 8)**

- `audio_music_config_default_get`, `audio_music_create`, `audio_sound_config_default_get`.
- `audio_sound_create`.

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
<summary>graphics.h: 231 entries</summary>

Source: [graphics.h](../include/graphics.h).

**M6 — Asset loading and text configuration (Goal 6)**

- `graphics_animation_load`, `graphics_font_load`, `graphics_text_create`.
- `graphics_texture_load`.

**M7 — Sprite, frame, and player configuration (Goal 7)**

- `graphics_animated_sprite_add`, `graphics_animated_sprite_create`, `graphics_animated_sprite_frame_add`.
- `graphics_animated_sprite_frame_get`, `graphics_animated_sprite_frame_set`, `graphics_animated_sprite_value_frame_add`.
- `graphics_animated_sprite_value_frame_set`, `graphics_animation_frame_create`, `graphics_animation_frame_get`.
- `graphics_animation_player_create`, `graphics_sprite_add`, `graphics_sprite_create`.

**M10 — Camera and presentation configuration (Goal 10)**

- `graphics_camera_attach`, `graphics_camera_attachment_get`, `graphics_camera_attachment_radians_set`.
- `graphics_camera_attachment_set`, `graphics_camera_config_default_get`, `graphics_camera_create`.
- `graphics_camera_get`, `graphics_camera_radians_attach`, `graphics_camera_set`.
- `graphics_camera_with_options_attach`, `graphics_camera_with_options_radians_attach`, `graphics_window_presentation_default_get`.
- `graphics_window_presentation_get`, `graphics_window_presentation_set`.

**M11 — Screen, viewport, layer, and persistent UI configuration (Goal 11)**

- `graphics_layer_create`, `graphics_screen_config_default_get`, `graphics_screen_create`.
- `graphics_ui_shape_create`, `graphics_ui_shape_get`, `graphics_ui_shape_set`.
- `graphics_ui_slider_create`, `graphics_ui_slider_get`, `graphics_ui_slider_set`.
- `graphics_ui_text_create`, `graphics_ui_text_get`, `graphics_ui_text_set`.
- `graphics_viewport_camera_add`, `graphics_viewport_config_default_get`, `graphics_viewport_create`.
- `graphics_viewport_item_config_default_get`, `graphics_viewport_item_get`, `graphics_viewport_item_set`.
- `graphics_viewport_screen_add`, `graphics_viewport_ui_add`, `graphics_viewport_ui_shape_add`.
- `graphics_viewport_ui_slider_add`, `graphics_viewport_ui_text_add`.

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
- `graphics_camera_attached_get`, `graphics_camera_moving_get`, `graphics_camera_zoom_get`.
- `graphics_contacts_debug_check`, `graphics_font_default_get`, `graphics_font_valid_check`.
- `graphics_layer_active_get`, `graphics_layer_animation_get`, `graphics_layer_animation_id_get`.
- `graphics_layer_entity_get`, `graphics_layer_entity_id_get`, `graphics_layer_get`.
- `graphics_layer_name_get`, `graphics_layer_name_id_get`, `graphics_layer_sprite_get`.
- `graphics_layer_sprite_id_get`, `graphics_layer_ui_get`, `graphics_layer_ui_id_get`.
- `graphics_mouse_screen_position_get`, `graphics_render_output_size_get`, `graphics_screen_to_world_get`.
- `graphics_sprite_body_offset_get`, `graphics_sprite_orientation_offset_get`, `graphics_sprite_orientation_offset_radians_get`.
- `graphics_text_valid_check`, `graphics_texture_size_get`, `graphics_texture_valid_check`.
- `graphics_ui_slider_value_get`, `graphics_viewport_item_drag_mode_get`, `graphics_viewport_item_dragging_check`.
- `graphics_viewport_item_viewport_get`, `graphics_viewport_ui_hovered_check`, `graphics_viewport_ui_pressed_check`.
- `graphics_viewport_ui_slider_changed_check`, `graphics_window_to_screen_get`, `graphics_world_to_screen_get`.

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
<summary>input.h: 47 entries</summary>

Source: [input.h](../include/input.h).

**M9 — Logical input configuration (Goal 9)**

- `input_action_create`, `input_controller_create`.

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
<summary>physics.h: 213 entries</summary>

Source: [physics.h](../include/physics.h).

**M5 — Soft-body and anchor configuration (Goal 5)**

- `physics_joint_anchor_add`, `physics_soft_body_add`, `physics_soft_body_area_add`.
- `physics_soft_body_beam_add`, `physics_soft_body_beam_collision_config_set`, `physics_soft_body_beam_collision_filter_set`.
- `physics_soft_body_node_add`, `physics_soft_body_node_collision_filter_set`, `physics_soft_body_node_local_add`.

**C — Existing description interface**

- `physics_axis_lock_description_get`, `physics_axis_lock_description_set`, `physics_collision_filter_description_default_get`.
- `physics_collision_filter_description_get`, `physics_collision_filter_description_set`, `physics_force_add`.
- `physics_force_description_get`, `physics_force_description_set`, `physics_joint_add`.
- `physics_joint_description_get`, `physics_joint_description_set`, `physics_particle_add`.
- `physics_particle_description_default_get`, `physics_particle_description_get`, `physics_particle_description_set`.
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
<summary>rohr.h: 729 entries</summary>

Source: [rohr.h](../include/rohr.h).

**M5 — Soft-body and anchor configuration (Goal 5)**

- `rohr_physics_joint_anchor_add`, `rohr_physics_soft_body_add`, `rohr_physics_soft_body_area_add`.
- `rohr_physics_soft_body_beam_add`, `rohr_physics_soft_body_beam_collision_config_set`, `rohr_physics_soft_body_beam_collision_filter_set`.
- `rohr_physics_soft_body_node_add`, `rohr_physics_soft_body_node_collision_filter_set`, `rohr_physics_soft_body_node_local_add`.

**M6 — Asset loading and text configuration (Goal 6)**

- `rohr_graphics_animation_load`, `rohr_graphics_font_load`, `rohr_graphics_text_create`.
- `rohr_graphics_texture_load`.

**M7 — Sprite, frame, and player configuration (Goal 7)**

- `rohr_graphics_animated_sprite_add`, `rohr_graphics_animated_sprite_create`, `rohr_graphics_animated_sprite_frame_add`.
- `rohr_graphics_animated_sprite_frame_get`, `rohr_graphics_animated_sprite_frame_set`, `rohr_graphics_animated_sprite_value_frame_add`.
- `rohr_graphics_animated_sprite_value_frame_set`, `rohr_graphics_animation_frame_create`, `rohr_graphics_animation_frame_get`.
- `rohr_graphics_animation_player_create`, `rohr_graphics_sprite_add`, `rohr_graphics_sprite_create`.

**M8 — Audio configuration (Goal 8)**

- `rohr_audio_music_config_default_get`, `rohr_audio_music_create`, `rohr_audio_sound_config_default_get`.
- `rohr_audio_sound_create`.

**M9 — Logical input configuration (Goal 9)**

- `rohr_input_action_create`, `rohr_input_controller_create`.

**M10 — Camera and presentation configuration (Goal 10)**

- `rohr_camera_attach`, `rohr_camera_config_default_get`, `rohr_camera_create`.
- `rohr_camera_get`, `rohr_camera_radians_attach`, `rohr_camera_set`.
- `rohr_graphics_camera_attach`, `rohr_graphics_camera_attachment_get`, `rohr_graphics_camera_radians_attach`.
- `rohr_graphics_camera_with_options_attach`, `rohr_graphics_camera_with_options_radians_attach`, `rohr_graphics_window_presentation_default_get`.
- `rohr_graphics_window_presentation_get`, `rohr_graphics_window_presentation_set`.

**M11 — Screen, viewport, layer, and persistent UI configuration (Goal 11)**

- `rohr_graphics_layer_create`, `rohr_graphics_ui_shape_create`, `rohr_graphics_ui_shape_get`.
- `rohr_graphics_ui_shape_set`, `rohr_graphics_ui_slider_create`, `rohr_graphics_ui_slider_get`.
- `rohr_graphics_ui_slider_set`, `rohr_graphics_ui_text_create`, `rohr_graphics_ui_text_get`.
- `rohr_graphics_ui_text_set`, `rohr_screen_config_default_get`, `rohr_screen_create`.
- `rohr_viewport_camera_add`, `rohr_viewport_config_default_get`, `rohr_viewport_create`.
- `rohr_viewport_item_config_default_get`, `rohr_viewport_item_get`, `rohr_viewport_item_set`.
- `rohr_viewport_screen_add`, `rohr_viewport_ui_add`, `rohr_viewport_ui_shape_add`.
- `rohr_viewport_ui_slider_add`, `rohr_viewport_ui_text_add`.

**C — Existing description interface**

- `rohr_entity_description_add`, `rohr_entity_description_get`, `rohr_entity_description_set`.
- `rohr_entity_lifetime_description_get`, `rohr_entity_lifetime_description_set`, `rohr_entity_object_add`.
- `rohr_entity_object_add_with_options`, `rohr_entity_object_by_name_add`, `rohr_entity_object_by_name_add_with_options`.
- `rohr_entity_object_description_add`, `rohr_entity_object_description_by_name_add`, `rohr_object_collision_description_get`.
- `rohr_object_collision_description_set`, `rohr_object_color_description_get`, `rohr_object_color_description_set`.
- `rohr_object_description_by_name_get`, `rohr_object_force_description_set`, `rohr_object_get`.
- `rohr_object_material_description_get`, `rohr_object_material_description_set`, `rohr_object_spawn_description_default_get`.
- `rohr_object_torque_description_set`, `rohr_physics_axis_lock_description_get`, `rohr_physics_axis_lock_description_set`.
- `rohr_physics_collision_filter_description_default_get`, `rohr_physics_collision_filter_description_get`, `rohr_physics_collision_filter_description_set`.
- `rohr_physics_force_add`, `rohr_physics_force_description_get`, `rohr_physics_force_description_set`.
- `rohr_physics_joint_add`, `rohr_physics_joint_description_get`, `rohr_physics_joint_description_set`.
- `rohr_physics_particle_add`, `rohr_physics_particle_description_default_get`, `rohr_physics_particle_description_get`.
- `rohr_physics_particle_description_set`, `rohr_physics_soft_body_node_to_anchor_pin_add`, `rohr_physics_torque_add`.
- `rohr_physics_torque_description_get`, `rohr_physics_torque_description_set`, `rohr_physics_transform_lock_description_get`.
- `rohr_physics_transform_lock_description_set`.

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
- `rohr_camera_moving_get`, `rohr_camera_zoom_get`, `rohr_console_active_get`.
- `rohr_directory_base_get`, `rohr_engine_paused_get`, `rohr_engine_tick_delta_get`.
- `rohr_engine_tick_get`, `rohr_engine_time_get`, `rohr_engine_time_per_tick_get`.
- `rohr_entity_alive_at_get`, `rohr_entity_alive_check`, `rohr_entity_alive_count_get`.
- `rohr_entity_by_name_get`, `rohr_entity_children_get`, `rohr_entity_components_check`.
- `rohr_entity_from_index_get`, `rohr_entity_group_by_name_get`, `rohr_entity_group_entity_check`.
- `rohr_entity_group_get`, `rohr_entity_group_name_get`, `rohr_entity_groups_get`.
- `rohr_entity_index_alive_check`, `rohr_entity_index_components_check`, `rohr_entity_index_get`.
- `rohr_entity_info_check`, `rohr_entity_info_get`, `rohr_entity_info_issues_count_get`.
- `rohr_entity_info_issues_get`, `rohr_entity_info_release`, `rohr_entity_info_retain`.
- `rohr_entity_name_get`, `rohr_entity_object_anchor_get`, `rohr_entity_object_camera_get`.
- `rohr_entity_object_member_by_name_get`, `rohr_entity_object_member_name_get`, `rohr_entity_object_members_get`.
- `rohr_entity_owner_get`, `rohr_entity_parent_get`, `rohr_graphics_aabb_tree_debug_check`.
- `rohr_graphics_animated_sprite_body_offset_get`, `rohr_graphics_animated_sprite_frame_index_get`, `rohr_graphics_animated_sprite_frame_offset_get`.
- `rohr_graphics_animated_sprite_frame_rotation_get`, `rohr_graphics_animated_sprite_frame_rotation_radians_get`, `rohr_graphics_animated_sprite_frame_scale_get`.
- `rohr_graphics_animated_sprite_orientation_offset_get`, `rohr_graphics_animated_sprite_orientation_offset_radians_get`, `rohr_graphics_animation_info_get`.
- `rohr_graphics_animation_valid_check`, `rohr_graphics_camera_attached_get`, `rohr_graphics_contacts_debug_check`.
- `rohr_graphics_font_default_get`, `rohr_graphics_font_valid_check`, `rohr_graphics_layer_active_get`.
- `rohr_graphics_layer_animation_get`, `rohr_graphics_layer_animation_id_get`, `rohr_graphics_layer_entity_get`.
- `rohr_graphics_layer_entity_id_get`, `rohr_graphics_layer_get`, `rohr_graphics_layer_name_get`.
- `rohr_graphics_layer_name_id_get`, `rohr_graphics_layer_sprite_get`, `rohr_graphics_layer_sprite_id_get`.
- `rohr_graphics_layer_ui_get`, `rohr_graphics_layer_ui_id_get`, `rohr_graphics_mouse_screen_position_get`.
- `rohr_graphics_render_output_size_get`, `rohr_graphics_screen_to_world_get`, `rohr_graphics_sprite_body_offset_get`.
- `rohr_graphics_sprite_orientation_offset_get`, `rohr_graphics_sprite_orientation_offset_radians_get`, `rohr_graphics_text_valid_check`.
- `rohr_graphics_texture_size_get`, `rohr_graphics_texture_valid_check`, `rohr_graphics_ui_slider_value_get`.
- `rohr_graphics_world_to_screen_get`, `rohr_input_action_axis_1d_get`, `rohr_input_action_axis_2d_get`.
- `rohr_input_action_bindings_default_get`, `rohr_input_action_bindings_effective_get`, `rohr_input_action_bindings_override_check`.
- `rohr_input_action_bindings_override_get`, `rohr_input_action_button_down_check`, `rohr_input_action_button_initial_state_check`.
- `rohr_input_action_button_mode_get`, `rohr_input_action_button_pressed_check`, `rohr_input_action_button_released_check`.
- `rohr_input_action_by_name_get`, `rohr_input_action_type_get`, `rohr_input_binding_valid_check`.
- `rohr_input_controller_by_name_get`, `rohr_input_controller_enabled_check`, `rohr_input_key_down_check`.
- `rohr_input_key_pressed_check`, `rohr_input_key_released_check`, `rohr_input_modifiers_get`.
- `rohr_input_mouse_button_down_check`, `rohr_input_mouse_button_pressed_check`, `rohr_input_mouse_button_released_check`.
- `rohr_input_mouse_delta_get`, `rohr_input_mouse_position_get`, `rohr_input_mouse_relative_mode_check`.
- `rohr_input_mouse_wheel_get`, `rohr_input_text_state_get`, `rohr_physics_angular_velocity_get`.
- `rohr_physics_angular_velocity_maximum_get`, `rohr_physics_angular_velocity_maximum_radians_get`, `rohr_physics_angular_velocity_radians_get`.
- `rohr_physics_center_of_mass_automatic_check`, `rohr_physics_center_of_mass_local_position_get`, `rohr_physics_center_of_mass_world_position_get`.
- `rohr_physics_collision_between_check`, `rohr_physics_contact_check`, `rohr_physics_contact_count_get`.
- `rohr_physics_contact_entered_check`, `rohr_physics_contact_exited_check`, `rohr_physics_contact_get`.
- `rohr_physics_contact_stayed_check`, `rohr_physics_contact_total_impulse_get`, `rohr_physics_contacts_get`.
- `rohr_physics_dt_per_tick_get`, `rohr_physics_entity_held_get`, `rohr_physics_force_location_get`.
- `rohr_physics_force_magnitude_get`, `rohr_physics_force_orientation_get`, `rohr_physics_force_target_get`.
- `rohr_physics_global_hit_box_get`, `rohr_physics_gravity_check`, `rohr_physics_gravity_get`.
- `rohr_physics_hitbox_active_id_get`, `rohr_physics_hitbox_active_index_get`, `rohr_physics_hitbox_animation_binding_at_get`.
- `rohr_physics_hitbox_animation_binding_get`, `rohr_physics_hitbox_at_get`, `rohr_physics_hitbox_by_id_get`.
- `rohr_physics_hitbox_count_get`, `rohr_physics_hitbox_get`, `rohr_physics_hitbox_id_at_get`.
- `rohr_physics_joint_anchor_local_position_get`, `rohr_physics_joint_anchor_world_position_get`, `rohr_physics_joint_anchors_get`.
- `rohr_physics_kinematic_driven_check`, `rohr_physics_mass_check`, `rohr_physics_mass_get`.
- `rohr_physics_moment_of_inertia_get`, `rohr_physics_overlap_check`, `rohr_physics_overlap_count_get`.
- `rohr_physics_overlap_entered_check`, `rohr_physics_overlap_exited_check`, `rohr_physics_overlap_get`.
- `rohr_physics_overlap_stayed_check`, `rohr_physics_overlaps_get`, `rohr_physics_particle_origin_get`.
- `rohr_physics_particle_overlap_get`, `rohr_physics_particle_radius_get`, `rohr_physics_position_get`.
- `rohr_physics_sat_overlap_get`, `rohr_physics_soft_body_area_get`, `rohr_physics_soft_body_beam_get`.
- `rohr_physics_soft_body_get`, `rohr_physics_soft_body_node_get`, `rohr_physics_soft_body_node_local_position_get`.
- `rohr_physics_solver_iterations_get`, `rohr_physics_substeps_get`, `rohr_physics_torque_magnitude_get`.
- `rohr_physics_torque_target_get`, `rohr_physics_world_position_check`, `rohr_ui_button_by_name_get`.
- `rohr_ui_button_style_default_get`, `rohr_ui_font_by_name_get`, `rohr_ui_key_pressed_check`.
- `rohr_ui_label_by_name_get`, `rohr_ui_navigation_focus_bounds_get`, `rohr_ui_pointer_consumed_get`.
- `rohr_ui_primary_pressed_check`, `rohr_ui_slider_by_name_get`, `rohr_ui_slider_config_default_get`.
- `rohr_viewport_item_drag_mode_get`, `rohr_viewport_item_dragging_check`, `rohr_viewport_item_viewport_get`.
- `rohr_viewport_ui_hovered_check`, `rohr_viewport_ui_pressed_check`, `rohr_viewport_ui_slider_changed_check`.

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

## Keeping this inventory current

When a migration goal completes, update its current symbols/types, mark its
family compliant, record intentional exemptions, and update header counts.
Preserve the planned-versus-shipped distinction for unfinished goals. Check
new declarations, facade macros, and direct aliases against the same boundary;
argument count alone is not a classification rule. Goal 12 repeats the complete
header comparison and removes obsolete migration references.
