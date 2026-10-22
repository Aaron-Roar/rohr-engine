# Object descriptions and instances

Editor-generated catalogs and runtime-loaded editor documents share the public
interface in `object.h`, also included by `rohr.h`. Regenerate existing projects
to add their catalog; existing generated constructors remain available.

```c
#include "generated/project_objects.h"

EntityResult body = entity_object_add(objects.mass_demo.automatic_body);
EntityResult camera = entity_object_by_name_add(objects.mass_demo, "main_camera");
ObjectDescriptionResult copy = object_copy(objects.mass_demo);
if (!error_check(copy)) {
    ObjectDescriptionResult child = object_description_by_name_get(copy.result.value,
        "automatic_body");
    if (!error_check(child)) object_mass_set(child.result.value, 7);
    EntityResult instance = entity_object_add(copy.result.value);
    /* Check instance, then use its entity handle. */
    object_destroy(copy.result.value);
}
```

Check every result in application code. The example uses the bundled COM
project's names; a default project's catalog exposes its own object/child names.
Generated views are lvalues. Common pointer functions such as
`entity_object_description_add()` also accept pointer expressions directly.

## Descriptions

The generated immutable `objects` catalog exposes properties beside named
children. Generated types are an implementation detail; common entry points
accept generated descriptions and runtime description handles. Runtime file
loading uses the editor document format and never manufactures C fields.
Recursive scoped lookup matches description names exactly and rejects ambiguity.

Copies own independent mutable data and include descendants by default. Copy and
spawn filters are positive inclusion masks: `ROHR_ENTITY_OBJECT_PHYSICS`,
`ROHR_ENTITY_OBJECT_VISUALS`, and their union `ROHR_ENTITY_OBJECT_ALL`. Omitted
options mean ALL; zero and unknown bits are errors. Names, hierarchy, and placement
survive filtering. No API creates descriptions from scratch. Mutation uses checked
setters on editable handles; defaults remain immutable.

Description resources have explicit release. Shared immutable assets follow
existing retain/release rules; live instances do not borrow mutable description
storage. Loading descriptions does not spawn entities or change global physics.
`object_load("path/to/project/objects/project.rohr.json")` returns a cached,
immutable catalog. Release every successful load, including cache hits, with
`object_destroy()`. Release only copy roots, never borrowed descendants or
generated constants. Standard project assets resolve relative to the project
directory; other document paths resolve assets relative to their directory.

Editable copies use common-handle setters for placement, linear motion, body
angular velocity, mass, materials, collision settings, radius, visibility,
colors, and sprite/animation scale. Soft-body mass edits proportionally update
node masses. Copies have dynamic layouts and use scoped lookup for descendants;
their property values are readable through the common description pointer.
There is no mutable generated global and no raw-description creation API.

Full grouped configuration uses `ObjectMaterialDescription` (friction/restitution),
`ObjectCollisionDescription` (enabled/filter), and `ObjectColorDescription`
(color/border_color/overridden), with `object_*_description_set/get` accessors.
Getters accept immutable catalogs too; setters accept only editable copies.
Replacement is complete: omitted fields become zero/false, not preserved values.
Use get-modify-set for partial edits. Color's `overridden` flag explicitly restores
inherited style. Existing instances keep their original configuration.

## Instances

New projects demonstrate explicit catalog spawning in `main.c` and whole-scene
creation in a separate `scene.c`. See [project entry points](scene_projects.md)
for generated instance bindings, build targets, and source ownership.

Each spawn creates new entities. Connections resolve only within that spawn;
existing entities are never recruited by matching names. Names first try the
description name, then available `_1`, `_2`, etc. Scoped member lookup uses a
separate description identity and survives ECS renaming.

Top-level positions are world positions; descendant placement is parent-local.
Direct descendant spawning retains resolved placement. An additional spawn
offset applies once. Editor overview position never contributes to gameplay.

`entity_object_add_with_options(description, flags)` and
`entity_object_by_name_add_with_options(scope, name, flags)` select categories.
Use `entity_object_description_add(object_get(description),
(ObjectSpawnDescription){.flags = ROHR_ENTITY_OBJECT_ALL, .offset = {100, 0}})`
for additional placement. `object_spawn_description_default_get()` returns ALL
at zero offset; a zero-initialized flags mask is invalid. Visual-only physical shapes retain static drawing
geometry with collision response and gravity disabled. Physics-only instances
do not load image resources. Included visuals attached to a body use its sprite
components; scoped lookup of their names returns that body entity.
Additional attached visuals of the same kind receive their own owned entity and
transform lock, so they do not replace an earlier visual. Those names resolve to
their individual entities. Body hitbox bindings use its primary animation.

Empty animations are valid objects. They retain an animated-sprite component,
placement, settings, ownership, and scoped identity, but load no frame assets.
Drawing and playback skip them. This also reserves the primary animation slot
for an attached empty animation, so a populated sibling keeps its own entity.
Entity info identifies the empty state, and the animation-update report records
an informational skip. Adding frames later resumes ordinary playback.

`entity_description_add((EntityDescription){.name = "car"})` creates named metadata;
empty creation names are generated uniquely. `entity_description_set/get` replaces
or copies name and owner together; full replacement rejects an empty name.
The name array is copied, and failed ownership preparation cannot publish a rename.
`entity_add()` and the focused name/owner setters remain available.

Every live entity can own members through `entity_owner_set(member, owner)`
(or `rohr_entity_owner_set`). Ownership storage is initialized internally and may
allocate; no group identifier is required. The owner has `ROHR_OBJECT` and a
protected owned-members group. Membership is independent of runtime parenting
and ordinary groups, and forms an acyclic forest with one immediate owner per
entity. Failed transfers leave the original ownership intact.

Deleting an owner deletes its members recursively. Removing `ROHR_OBJECT`
performs the same owned-content cleanup while retaining the owner entity.
`entity_owner_set(member, ENTITY_INVALID)` releases an entity without deleting
it. Transfers and releases preserve world state, parenting, and all connections.
Member description identities survive transfers; old ancestor aliases that no
longer refer to owned descendants are removed. For newly adopted runtime entities,
the initial member identity is their current ECS name.

Soft-body node, beam, and area lists index immediate ownership. Adopting an
existing node into a soft body updates its membership and lifetime together;
adopting an ordinary particle controls lifetime without converting it to a node.
Moving a node preserves beam endpoints, area geometry, and joint connections.
Beams and areas may reference nodes owned by other bodies. Aggregate mass and
body transforms use owned nodes only. A bridge beam owned by A can reference a
node in B: deleting A deletes the beam, while B survives. An independently owned
bridge instead survives and clears its deleted endpoint.

Use `entity_owner_get()`, `entity_object_member_by_name_get()`, and
`entity_object_members_get()` to inspect ownership. The former
`entity_object_owner_get()`, `entity_object_member_release()`, and
`physics_soft_body_node_owner_set()` APIs have been replaced by the generic
owner getter/setter (including public `rohr_` wrappers).
Names of nested spawned entities include their instance prefix; scoped lookup
takes the original description name, independently of that prefix. Repeated
names in a scope are ambiguous and return an error.

`entity_object_camera_get()` and `entity_object_anchor_get()` return borrowed
typed resource handles. Their proxy entities own destruction. Game code chooses
camera callbacks and viewport mounts as usual. `entity_object_draw(root)` draws
retained appearance and owned descendants in the active camera. Callback code
and viewport composition remain caller-owned.

Joints without anchors, nodes without owners, and beams/areas with missing topology
are supported incomplete states. Detached nodes simulate normally. Incomplete
topology skips only calculations that require its missing connections and is
reported through inspection/update reports. Stale nonzero references, invalid
numbers, and corrupt ownership remain errors. Deleting a target disconnects external
connections without deleting their entities.

Pin, weld, and spring joints may have neither endpoint, only A, or only B set.
The editor, project JSON, compiled catalogs, direct generated constructors, and
scene simulation preserve these definitions and their settings. An unset
endpoint is zero; a nonzero authored anchor ID must resolve to an existing
anchor. The physics report identifies each unavailable joint contribution as
skipped with a warning. Completing its `JointDescription` activates the same
entity; there is no need to spawn a replacement joint. Anchors attached to soft
nodes are created after those nodes and exposed by generated instance bindings.

## Restoration and failure

Runtime saves preserve ownership, member identities, incomplete topology, and
supported instance resources without requiring the original document. Save/load
format changes follow exact-version validation. Templates remain separate from
runtime snapshots. All creation and restoration operations roll back their own
entities and resources on failure without modifying pre-existing world state.

`game_state_file_save(path)`, `entity_delete(root)`, then
`game_state_file_load(path)` restores the saved live instances directly. A
snapshot contains the whole live world, so delete all entities represented in
it before loading into the same world, or load into a fresh engine. Loading
rejects name clashes; it does not silently clone a snapshot. Use descriptions
for independent repeated spawning. Image files remain required assets, while
the original editor document and description handles are unnecessary.

Ordinary hierarchy parenting and generic group deletion retain their current
behavior. Existing generated constructors and public entry points remain supported.
Force/torque authoring and viewport redesign are outside this goal.

## Authored forces and torques

Editor eyes are excluded from runtime descriptions. Each visual item's
`visible_in_app` becomes its description visibility; hidden parents also
suppress child sprite and camera output. Visibility does not change physics.

Force, torque, and joint descriptions store app styling in `.visuals`, an
`ObjectVisuals` value with `main`, `endpoint_a`, and `endpoint_b` styles.
`ObjectVisualStyle` contains `specified`, RGBA `fill_color` and `border_color`,
`border_thickness`, `scale`, `offset`, and clockwise degree `rotation`.
An unspecified style uses the engine defaults: unit scale, zero offset/rotation,
one-unit dark border, and amber fill (gold for joints). Set `specified = true`
when providing a style; all its fields then apply, including transparent colors
and zero scales. Borders must be nonnegative; numeric transforms must be finite.
Endpoint styles affect spring markers only. The main transform affects the whole
spring around its midpoint; endpoint offsets move marker centers and coil ends,
without changing anchors. Generated and loaded descriptions share the same
geometry and styling, which also survives runtime snapshots.

Editor document version 12 includes top-level force and torque items in each
object. Loading a document exposes them as `OBJECT_FORCE` and `OBJECT_TORQUE`
descriptions with `.force` and `.torque` values. `ObjectForceDescription` and
`ObjectTorqueDescription` use `ObjectSourceMode`: persistent (zero/default) or
initial. Defaults have no target and zero magnitude; force location and
orientation default to world origin and world zero.

`ObjectSourceReference` identifies a body, standalone particle, soft body, node,
or beam using a stable description ID. Nodes and beams also identify their
containing soft body. These are description identities, **not runtime entity
handles**. References stay within the owning object; world references and an
explicit `OBJECT_SOURCE_NONE` target are supported. `ENTITY_NONE` is an alias
of `ENTITY_INVALID` for optional runtime entity arguments; neither sentinel can
replace a required reference.

Object copies own their source data independently. Physics inclusion flags
include source definitions; visual-only copies exclude their physical values.
Use `object_force_description_set` and `object_torque_description_set` (also
available with `rohr_` prefixes) to atomically replace definitions on editable
copies. Read their values through `object_get`; immutable documents reject edits.
Definitions do not imply ownership of their targets.

Loaded and generated source descriptions use the same object spawner. Bodies,
topology, and joint bindings are created before sources, and every active
reference resolves inside the new instance. Failed spawning removes the entire
partial instance. Visual-only spawning excludes sources.

Persistent sources are named entities owned by the containing object. Retrieve
them through `entity_object_member_by_name_get(instance, "rear_drive")` and use
the existing force/torque setters. Ownership remains independent of the receiving
target; explicit ownership transfers preserve connections. Deleting the object
deletes its sources, while transferred sources follow their new owner.

Initial sources are also instance-owned but expire at creation tick plus one,
using the existing engine lifecycle. They leave no placeholder and are cancelled
if their instance is deleted first. Physics updates alone do not advance expiry.
Runtime snapshots preserve the remaining lifetime and never replay expired
initial applications. The public one-shot `apply()` APIs still require no owner.

Generated `*_create()` wrappers for source-bearing objects delegate to this same
spawner. Their `source_instance` field owns the object; `force_<name>` and
`torque_<name>` fields expose persistent handles. Initial handles are deliberately
omitted from these structs. Generated destruction deletes the instance, and
generated drawing uses its retained appearance.

World application points and headings remain absolute, including when spawning
with an offset or rotating a copied object. Use origin references for a point
that follows an instance transform, or COM references for a world-axis offset
from an instance's current COM.

Only editor document version 12 is accepted. Bundled documents use version
12, with `forces`/`torques` arrays and `next_force_id`/`next_torque_id` counters.
Runtime game-state format uses version 10 and keeps its existing generic
ownership, target restoration, and temporary-source lifetime rules.
