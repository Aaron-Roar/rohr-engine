# JSON game state

Game state uses yyjson and schema version `8`. A state may be split across
multiple files. `game_state_load_files()` registers every entity name before
loading component values, so a relationship may refer to an entity in any file
in the same call.

Angles and offsets use clockwise degrees; angular rates use degrees/second
and degrees/second squared. Negative values and multiple turns are preserved.
Zero keeps authored geometry unchanged, with local +Y as the world heading.
Torque and inertia keep their physical units. Versions other than 8 are rejected
without changing the currently loaded state.

The [game-state example](../examples/game-state/src/main.c) loads an off-center
polygon with explicit COM, checks that an origin teleport preserves local mass
properties, and verifies them again after saving and reloading. For an editable
project with a visible rotation demonstration, see the
[COM example](../examples/center_of_mass/README.md).

```json
{
  "version": 8,
  "entities": [
    {
      "name": "player",
      "count": 1,
      "components": {
        "flags": ["dynamic", "collision"],
        "position": {"x": 40, "y": 60},
        "mass": 4,
        "target": "goal"
      }
    }
  ]
}
```

Names are unique among live entities and may contain at most 63 bytes. The
loader accepts JSON comments and trailing commas for handcrafted files.

## User interface definitions

Fonts, standalone labels, and immediate-mode buttons can be authored under
`ui`:

```json
"ui": {
  "fonts": [{
    "name": "menu_font",
    "file": "assets/fonts/menu.ttf",
    "point_size": 24
  }],
  "labels": [{
    "name": "menu_title",
    "text": "Main Menu",
    "font": "menu_font",
    "color": {"red": 255, "green": 255, "blue": 255, "alpha": 255},
    "bounds": {"x": 0, "y": 30, "width": 640, "height": 50}
  }],
  "buttons": [{
    "name": "settings_button",
    "label": "Settings",
    "font": "menu_font",
    "text_color": {"red": 255, "green": 255, "blue": 255, "alpha": 255},
    "bounds": {
      "x": 220,
      "y": 205,
      "width": 200,
      "height": 55
    },
    "style": {
      "hovered": {"red": 95, "green": 80, "blue": 145, "alpha": 255}
    }
  }]
}
```

Font definitions provide a unique name, font-file path, and positive point
size. They are retrieved with `ui_font_by_name_get()`; the game explicitly
loads, owns, and releases the resulting shared `FontAsset` reference.

Standalone labels have a unique name, text, named font, color, and bounds. They
are retrieved with `ui_label_by_name_get()`. This data is independent from
buttons and can be drawn with `ui_label()` after the game creates its text
asset.

Button `name` identifies the authored definition for
`ui_button_by_name_get()` and is its default stable runtime interaction ID.
Pass that name to `ui_button()` for ordinary use, or supply a different runtime
ID when instantiating one definition more than once. `label` is optional and defaults to an empty string. A
non-empty button label requires a valid named `font`; `text_color` is optional
and defaults to opaque white. Bounds use logical screen coordinates and require
positive width and height.

The `style` object and each of its `idle`, `hovered`, `pressed`, and `disabled`
colors are optional. Omitted colors use the default button style. Each supplied
color requires unsigned `red`, `green`, `blue`, and `alpha` values from 0 to
255.

Loading a definition does not draw it or handle its click. The game retrieves
the definition, creates any desired text asset, calls `ui_button()` while the
menu is active, and handles the returned interaction result. Runtime and
template saves preserve loaded font, label, and button definitions.

Sliders are authored independently from their runtime values:

```json
"sliders": [{
  "name": "volume_slider",
  "center": {"x": 320, "y": 280},
  "length": 220,
  "angle": 0.25,
  "range": {"min": -100, "max": 100},
  "step": 5,
  "initial_value": 0,
  "label": "Volume",
  "font": "menu_font",
  "value_format": "%.0f%%",
  "text_color": {"red": 240, "green": 244, "blue": 250, "alpha": 255},
  "style": {
    "track": {"red": 50, "green": 58, "blue": 72, "alpha": 255},
    "fill": {"red": 100, "green": 140, "blue": 230, "alpha": 255},
    "track_thickness": 10,
    "handle_width": 18,
    "handle_height": 32
  }
}]
```

Slider `name` is both the key used by `ui_slider_by_name_get()` and the
ordinary runtime ID passed to `ui_slider()` or `ui_slider_with_text()`. A game
may still pass a different runtime ID when reusing one definition for multiple
live sliders.

`center` and positive `length` are required. `angle` is optional and defaults
to zero; it is measured clockwise in logical screen-space degrees. The
range defaults to `0..1` when omitted. Explicit endpoints may be positive,
negative, or descending, but cannot be equal. `initial_value` defaults to the
first range endpoint and must fall between the two endpoints.

`step` is optional and defaults to zero, which gives continuous movement. A
positive step snaps values relative to the first range endpoint. Step controls
are implied by `style.step_button_size`: omission or zero disables them, while
a positive size enables them when `step` is positive. The minus control is
placed at the numerically lower end and the plus control at the higher end.
Their screen sides therefore reverse automatically for a descending range.

`label`, `font`, `value_format`, and `text_color` describe optional slider
text. A non-empty label or value format requires a valid named font. Text
assets remain owned by the game: pass prepared label/value and optional `-`/`+`
assets through `ui_slider_with_text()`. Recreate the formatted value asset only
when `UISliderResult.changed` is true. `NULL` assets omit the corresponding
automatic text.

Slider colors and sizes are optional and inherit the engine defaults. Supported
colors are `track`, `fill`, `handle`, `handle_hovered`, and `handle_pressed`.
Supported positive sizes are `track_thickness`, `handle_width`,
`handle_height`, and `step_button_size`; `step_button_size` defaults to zero,
and `step_button_gap` may always be zero. The game
retrieves definitions with
`ui_slider_by_name_get()`, retains the current value, and passes that value
back to `ui_slider()` each frame.

An entity description may use `count` to create multiple copies from one
component prototype. The first copy keeps the base name and later copies use
`_1`, `_2`, and so on. Expanded names are deterministic. Any clash within the inputs or with a live
entity rejects the complete load without renaming incoming entities.

Counted prototypes overlap at their component `position` by default. An
optional `placement` object can generate initial positions from that position:

```json
"placement": {
  "type": "grid",
  "columns": 25,
  "spacing": {"x": 8, "y": 8},
  "centered": true
}
```

Supported placement types are:

- `point`: keep every copy at the prototype position; this is the default when
  `placement` is omitted
- `line`: requires a `{ "x", "y" }` `step`; `centered` is optional
- `grid`: requires positive `columns` and `{ "x", "y" }` `spacing`;
  `centered` is optional
- `circle`: requires a non-negative `radius`; optional `start_angle` is a
  clockwise degree offset from the original +X layout. Zero preserves the
  original layout and instance ordering.

Placement requires the prototype to contain a `position`, except for `point`.
It only determines initial transforms and does not change physics behavior.

Generic groups are declared once at the document root and referenced by name
from entity components:

```json
{
  "groups": [{"name": "small_flies"}],
  "entities": [{
    "name": "small_fly",
    "count": 499,
    "placement": {
      "type": "grid",
      "columns": 25,
      "spacing": {"x": 8, "y": 8},
      "centered": true
    },
    "components": {
      "groups": ["small_flies"]
    }
  }]
}
```

Group names are unique among live generic groups and may contain at most 63
bytes. Parent-owned child groups remain internal and cannot be named through
this schema.

`components.mask` is an optional numeric `RohrComponentMask`. `flags` is the more readable
form for `static`, `dynamic`, `collision`, `targetable`, `particle`, and `hold`.
When saving, both the numeric mask and readable flags are emitted.

Version 1 files are rejected; rewrite them for the current origin/COM contract.
COM is stored as `"center_of_mass": {"mode": "automatic"}` or
`"center_of_mass": {"mode": "explicit", "offset": {"x": 3, "y": -2}}`.
Omitting the field means automatic. Explicit mode requires two finite,
in-range local coordinates. Unknown modes, missing coordinates, or an offset
in automatic mode fail the load and preserve previously loaded state.
Saves retain the mode and authored offset, never a calculated centroid or inertia.
Runtime saves capture current COM settings; template saves preserve the original
authored settings.

Standalone particles have automatic COM at their circle centroid and reject
explicit overrides. Their owned `particle_geometry` stores `standalone`,
`origin` (local x/y), and `radius` so this restriction and circle geometry
survive saving. For ordinary particle-collision bodies, omitted radius keeps
radius derived from geometry and omitted origin keeps the default local zero;
standalone particles require both. Ordinary rigid bodies with particle collision may still use
explicit COM. Simulated particle rotation remains disabled.

Supported value keys are:

- `position`, `velocity`, and `acceleration`: `{ "x", "y" }`
- `mass`, `orientation`, `angular_velocity`, `angular_acceleration`,
  `friction`, and `restitution`: numbers
- `hit_box`: an array of 3 to 50 `{ "x", "y" }` vertices
- `center_of_mass`: automatic or explicit configuration described above
- `particle_geometry`: `{ "standalone", "origin": {"x", "y"}, "radius" }`
- `collision_filter`: `{ "category", "collides_with" }` unsigned 64-bit masks
- `target` and `parent`: entity-name strings
- `lifetime`: `{ "time", "tick", "time_enabled", "tick_enabled" }` (remaining durations)
- `angle_lock`: `{ "min", "max" }`
- `axis_lock`: `{ "axis": {"x", "y"}, "point": {"x", "y"} }`
- `transform_lock`: `driver`, `local_offset`, `local_angle`, `lock_position`,
  `lock_orientation`, and `inherit_velocity`
- `joint`: named `a` and `b` entities, `type` (`spring`, `weld`, or `pin`),
  anchors, rest values, stiffness, damping, and angular settings
- `animated_sprite`: a named `animation`, per-entity `scale`, and
  `time_per_frame`, `ticks_per_frame`, and `start_frame`
- `groups`: an array of named generic groups

Animations are named once in the top-level asset catalog. State loading must
occur after `rohr_graphics_start()` when a file contains animation assets, because
the engine creates SDL textures while connecting the state. Animation and frame
`id` values are stable unsigned project identities used by cross-system bindings.
Catalog frames contain only image files and IDs. Instance `animated_sprite.frames`
records contain `scale`, `offset`, and `rotation`, with optional `file` and `id`
overrides. Omit the array to copy the catalog with scale 1 and zero offset/rotation.
Each instance owns its transforms; runtime saves preserve replaced/appended frames,
while template saves preserve the authored description. Scale multiplies native
image dimensions, signed axes flip image content, and zero collapses the image.
Offsets place frame centers relative to the animation origin; rotation uses
clockwise degrees about each center. Transform values must be finite.
Version 3 and old catalog `size`/transform fields are rejected. To rewrite old
dimensions, divide them by the image's native width/height and put the resulting
scale on each sprite instance.

Legacy definitions may omit IDs; animations then use their one-based catalog
position and frames use their one-based frame position.

```json
"assets": {
  "animations": [{
    "name": "elderfly_flying",
    "id": 1,
    "ticks_per_frame": 0,
    "time_per_frame": 0.05,
    "frames": [{
      "id": 1,
      "file": "assets/elderfly/flying/f1.png"
    }]
  }]
}
```

Counted prototypes can vary sprite values deterministically by instance:

```json
"animated_sprite": {
  "animation": "elderfly_flying",
  "frames": [{"scale": {"x": 0.125, "y": 0.125},
              "offset": {"x": 0, "y": 0}, "rotation": 0}],
  "scale": {"x": 0.4, "y": 0.4},
  "time_per_frame": 0,
  "ticks_per_frame": 3,
  "start_frame": 0,
  "variation": {
    "scale": {
      "random": {
        "min": {"x": 0.3, "y": 0.3},
        "max": {"x": 0.5, "y": 0.5},
        "seed": 100
      }
    },
    "ticks_per_frame": {
      "cycle": [2, 3, 4, 5]
    },
    "start_frame": {
      "sequence": {"start": 0, "step": 1, "wrap": 12}
    }
  }
}
```

Variation supports `scale`, `time_per_frame`, `ticks_per_frame`, and
`start_frame`. Scalar fields support:

- `cycle`: choose array entries by instance index
- `sequence`: `start + step * instance`, with optional positive `wrap`
- `linear`: interpolate from `from` to `to` across the collection
- `random`: choose between `min` and `max` using an explicit unsigned `seed`

Scale supports the same generators with vector values, except vector
`sequence` does not use `wrap`. Random generation is derived independently from
the seed, field, vector axis, and instance index, so another varied field does
not perturb existing results. Generated scale values must be positive. Tick
rates and starting frames must resolve to non-negative integers, and starting
frames must exist in the referenced animation.

Saving includes all live entities and named generic groups. Collision reports and
broad-phase grid state are runtime-derived and are not serialized. Parent/child
structure is serialized from the child-side `parent` reference, and the
parent's children group is rebuilt while loading.

## Runtime saves and authored templates

`game_state_save_file()` writes an exact runtime-oriented save. Counted
prototypes are expanded so independently changed positions, velocities,
animation values, and relationships are not lost.

`game_state_save_template_file()` writes the immutable authored definitions
retained from every successfully loaded state document. It merges named groups,
animation assets, and entity descriptions while preserving `count`,
`placement`, and `variation`. It deliberately does not capture runtime
mutations or entities created only through C APIs.

Retained template documents are owned by the engine state module and released
by `rohr_engine_stop()`. Template saving returns an error when no state document
has been loaded. Loading more than 64 retained documents in one engine session
returns `ERROR_ENGINE_STATE_TEMPLATE_DOCUMENT_LIMIT_EXCEEDED`. The public
`GAME_STATE_MAX_TEMPLATE_DOCUMENTS` constant exposes this capacity.

Duplicate names in the asset catalog return
`ERROR_ENGINE_STATE_DUPLICATE_ASSET_DEFINITION`. An entity component referring
to an animation or other named asset that was not defined returns
`ERROR_ENGINE_STATE_ASSET_REFERENCE_NOT_FOUND`.

## Cameras

A state may attach the active camera to one named entity:

```json
"camera": {
  "attachment": {
    "entity": "player",
    "position_offset": {"x": 0, "y": 20},
    "orientation_offset": 0,
    "follow_position": true,
    "follow_orientation": false
  }
}
```

The entity may be declared in any file passed to the same
`game_state_load_files()` call. It must have both a position and orientation.
`follow_position` and `follow_orientation` are optional and default to `true`.
They independently control which parts of the entity transform are inherited.
When orientation is followed, `position_offset` is in entity-local space and
rotates with the entity. Otherwise it is world-space. When position is not
followed, `position_offset` is the fixed camera world position. Likewise,
`orientation_offset` is added to the entity orientation when it is followed,
and becomes the fixed world orientation when it is not.

A standalone camera uses an explicit world-space transform:

```json
"camera": {
  "transform": {
    "position": {"x": 320, "y": 180},
    "orientation": 0.5
  }
}
```

`attachment` and `transform` are mutually exclusive. Loading a standalone
transform detaches any currently followed entity.

Only one retained input document per engine session may declare a camera;
additional declarations are rejected as ambiguous.

Runtime saves emit either the active named attachment or the current standalone
transform. Compact template saves preserve the authored camera declaration.

## Force and torque source components

Versions 5 and 6 store copied descriptions instead of vector/scalar source values.
`force` contains `target`, `magnitude`, `location`, and `orientation`; `torque`
contains `target` and `magnitude`. Targets are entity names or JSON null for no
target. Location objects contain `offset: {x, y}` and the numeric reference
`kind`; COM/origin references add `entity`, and beam references add `entity`,
`anchor`, and `fraction`. Orientation objects contain degree `offset` and `kind`;
body/beam references add `entity`, and between-location references add `from`
and `to` location objects. Enum values match the public contract.

All ordinary components load before source descriptions, so referenced transforms
can appear later in the file. Invalid descriptions roll back newly loaded entities.
Saving a named source requires its referenced entities to have names; unresolved
or unnamed references cause an error rather than silently becoming no-target.
Runtime topology is described below; authored source definitions remain a later
milestone goal.

## Version 8 runtime topology contract

Version 8 makes entity ownership authoritative for lifetime and soft-body
membership. The redundant `owner` fields in `soft_body_node`, `soft_body_beam`,
and `soft_body_area` are removed and rejected on load. Exact-version validation
remains in effect; earlier documents must be regenerated or migrated. Migration
must reconcile old physics and object ownership into a single ownership forest;
changing the version number alone is insufficient when those fields are present.

`object` is an ordered array of `{ "entity": "runtime name", "name":
"description member name" }` entries. Its presence identifies a `ROHR_OBJECT`
owner, including an empty owner. Members must belong to this load transaction,
have only one owner, and form an acyclic ownership forest. Description member
names may repeat; scoped lookup then reports ambiguity. Runtime ECS names remain
unique. Ownership is independent of ordinary groups and hierarchy parenting.
Every live entity can own members. Immediate ownership determines soft-body
membership, without converting entity types. The ordered `soft_body` lists must
exactly match that derived membership. Connections may reference nodes owned by
other bodies; they never adopt those nodes.

Force/torque description owners use the same version 8 ownership records; no
second owner is serialized in source components. Loading source descriptions
preserves the ownership restored earlier in the load transaction. A source with
both components retains one shared lifetime owner, independently of its targets.

`object_aliases` records attached visual description names using the same entry
shape. Alias targets must be the owner or its owned descendants. `object_anchor`
stores an anchor proxy's `owner` name and per-owner anchor `index`.
`object_appearance` retains kind, visibility, drawing enablement, and colors.
`sprite_state` and `animation_state` store image paths, sizes, transforms,
visibility, and independent playback state; entity, sprite, and animation layers
are saved separately. `object_camera` stores dimensions, zoom, position,
orientation, enable/pause flags, and an optional named attachment. Camera
callbacks and viewport mounts remain application-owned.

Engine snapshots carry `"runtime_snapshot": true`. Loading them restores live
state without adding to the retained authored-template documents. This permits
repeated save/delete/load cycles without accumulating template definitions.
Authored documents omit this marker (or set it false) and remain available to
`game_state_template_file_save()` without capturing later runtime edits. Runtime
animation frames are self-contained records; template animation definitions
remain in their original authored format. Image paths beneath the working
directory are saved relatively, as with previous runtime saves.

Beam endpoints, area node slots, and joint endpoints may be explicit JSON null
for disconnected state. An entity absent from all owned-member arrays is
independent. Non-null references must resolve to the appropriate live type. Area loops may contain fewer than three nodes and
null slots; such areas retain their definition without a fill. Duplicate non-null
nodes remain invalid. Joint anchor indices must match their corresponding owner;
either endpoint can independently use an anchor or an inline local offset.

Ordinary entity creation APIs assign unique editable names: `entity_n`, `force_n`,
`torque_n`, `particle_n`, `joint_n`, or `soft_body_n`. Soft-body children use
`<owner>_node_n`, `<owner>_beam_n`, and `<owner>_area_n`. Generated suffixes skip
occupied names. Only the generated owner prefix may be truncated to fit 63
bytes. Component changes and owner renames preserve existing child names.
Object-description spawning instead starts with description-derived names and
uses `_1`, `_2`, and subsequent suffixes for collisions, as described in
[the object interface](object_descriptions.md).

The loader preflights all expanded names across every document and the existing
world. It then creates exact names, loads ordinary state, restores ownership,
rebuilds soft-body membership while resolving topology, restores owned anchors,
and finally connects joints and force/torque descriptions. Failed loads release
their entities and resources and preserve the existing world.
References are names, never runtime handles. Counts, ownership, unique ordered
membership, finite numeric fields, and references must be valid.

`soft_body` stores ordered `nodes`, `beams`, and `areas` name arrays. Its origin
is the owning entity; the ordinary transform records its saved origin pose.
`soft_body_node` stores positive `radius`, `draw_color`, and
`draw_color_overridden`; ordinary components preserve world position, mass,
motion, constraints and collision settings. `soft_body_beam` stores `node_a`, `node_b`, `rest_length`, `stiffness`, `damping`, `collision_thickness`,
`collision_enabled`, `category`, `collides_with`, `friction`, `restitution`, and
drawing overrides. `soft_body_area` stores an ordered `outer` node-name
array, ordered `holes` arrays, `visible`, and drawing overrides. Derived fill
caches are rebuilt; temporarily undrawable deformations do not invalidate
otherwise valid topology. Owner transforms are never applied to saved node poses.

`joint_anchors` is an array of local positions on each owner, including unused
and shared anchors. Joint `anchor_a` and `anchor_b` are zero-based indices into
the respective owners' arrays, or null for ordinary inline local anchors.

Runtime `lifetime.time` and `lifetime.tick` are remaining durations, with
`time_enabled` and `tick_enabled` explicitly distinguishing disabled expiry
from zero remaining. Restoring re-bases deadlines on the current engine clock;
already-due entities expire at the next normal lifecycle pass. Persistent and
one-tick sources retain their lifetime behavior. Global physics configuration
remains caller-owned. Saves do not mutate simulation and replace the destination
only after successful serialization to a temporary sibling file. Authored
template saving continues to retain the original loaded document.
