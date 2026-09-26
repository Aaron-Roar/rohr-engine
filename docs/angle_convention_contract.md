# Clockwise angles and degree-default API contract

This is the approved contract for the **clockwise angles with degrees by
default** milestone in [the roadmap](../NEXT_STEPS.md). Goal 1 records the
contract and inventory only. The runtime still uses its previous conventions
until the implementation goals are completed; declarations below describe
planned APIs, not functions already available.

## Coordinates, units, and zero

World coordinates remain +X right and +Y up. Window, screen, and viewport-local
coordinates retain their existing +Y-down convention. Positive angular values
mean clockwise rotation as viewed on an ordinary, unmirrored display.

| Heading in degrees | Direction | World unit vector |
| --- | --- | --- |
| 0 | Up | `(0, 1)` |
| 90 | Right | `(1, 0)` |
| 180 | Down | `(0, -1)` |
| 270 | Left | `(-1, 0)` |

Zero orientation preserves authored geometry, offsets, text, and upright
artwork. Local +Y is the body's up/forward reference, not an instruction to
rotate an arbitrary shape's vertices. A shape drawn pointing right still
points right at zero. A horizontal slider stays horizontal at zero. No
automatic quarter-turn, geometry rewrite, or image-content inference occurs.
Rotation handles point up at zero. Relative zero means no additional rotation;
`math_vector_rotate(v, 0)` is identity. Parent and relative orientations add.

Default angles are degrees, angular velocities are degrees/second, and angular
accelerations are degrees/second squared. This applies to both engine API
layers, exposed component pools, public structs/results, editor state and
history, CLI values, JSON, generated C, and examples. C scalar representations
remain unchanged. Negative values and multiple revolutions, such as -450 or
810 degrees, remain valid and must round-trip without wrapping. Angle limits
operate on the stored, unwrapped numeric interval; a min/max pair is not an
implicitly wrapped compass interval. Preserve existing constraint policies.

Torque, angular impulse, and inertia keep their physical units: respectively
mass * distance squared / second squared, mass * distance squared / second,
and mass * distance squared. Positive torque and angular impulse act clockwise.
Their APIs do not acquire degree/radian variants. `Torque` remains a physical
quantity even though its existing C typedef aliases `Orientation`. Joint
`angular_stiffness` and `angular_damping` are coefficients, not angles: do not
rescale stored coefficients or activate currently unused behavior in this work.

## Transform and physics rules

Let `a` be a clockwise degree angle and `r = a * pi / 180`. For world vectors:

```text
rotate_world((x, y), a) = (x*cos(r) + y*sin(r), -x*sin(r) + y*cos(r))
heading_world(a)       = (sin(r), cos(r))
```

The rotation API applies an amount to an existing vector; it does not turn
that vector into a heading. Keep its zero identity and inverse/composition
properties. Trigonometric functions use radians internally, with conversion
at explicit boundaries. Do not add pi/2 inside the vector-rotation helper.
Geometric cross products remain `a.x*b.y - a.y*b.x`; winding, polygon
triangulation, centroids, area, and inertia calculations retain that meaning.

Screen-coordinate rotation must account for +Y down. Its clockwise matrix is
`(x*cos(r) - y*sin(r), x*sin(r) + y*cos(r))`. Rendering adapters and inverse
picking transforms must use the appropriate coordinate space. SDL's degree
arguments must not receive a second degree conversion or an obsolete sign
flip. Camera orientation describes its world frame: turning the camera
clockwise makes a stationary world appear to turn counterclockwise. Screen
placement and content rotations remain independent transforms.

Use the existing [origin/COM contract](physics_origin_contract.md): direct
orientation edits keep origin fixed, while simulation and solver corrections
rotate around COM. Geometry and local attachment offsets retain their values.
For clockwise-positive angular quantities:

```text
omega_radians = omega_degrees * pi / 180
velocity_at_offset = velocity_com + (omega_radians * offset.y,
                                    -omega_radians * offset.x)
torque_from_force = -cross(offset_from_com, force)
angular_acceleration_degrees = (torque / inertia) * 180 / pi
angular_impulse_delta_degrees = (angular_impulse / inertia) * 180 / pi
```

Update contact and joint Jacobians, angular impulses, orientation corrections,
soft-body torque distribution, transform-lock velocity inheritance, and
origin axis-lock calculations consistently. Keep physical calculations in
explicitly identified radian temporaries where needed; write exposed angular
state in degrees. Do not mix a degree-valued Jacobian with radian-based inertia.
Preserve the magnitude and direction of physical response when old descriptions
are rewritten for the new convention. Existing massless, static, held,
kinematic, geometryless, locked, and particle response restrictions remain.
This does not introduce force-at-point APIs or force-emitter editor tools.

## Public API inventory

Each function below retains its existing signature and return/error style.
Its default angular scalars change to degrees or degree-based rates. Add the
listed radians sibling with the same signature: convert only the angular
scalar inputs or successful scalar results. Radians siblings are also
clockwise-positive; they are not old-convention compatibility aliases.
Both paths must share behavior and validation. Conversion must not bypass
existing rejection of nonfinite or unrepresentable results.

Every entry applies to the direct declaration and its public wrapper. Except
for the camera-ID attachment API noted below, public names add `rohr_` to the
direct name. Insert `_radians` before the operation, treating `cross_vec` as
one operation:

| Default function | Radians sibling | Angular scalar |
| --- | --- | --- |
| `math_vector_rotate` | `math_vector_radians_rotate` | angle |
| `math_angular_velocity_cross_vec` | `math_angular_velocity_radians_cross_vec` | omega |
| `physics_shape_world_translate` | `physics_shape_world_radians_translate` | angle |
| `physics_orientation_set` | `physics_orientation_radians_set` | angle |
| `physics_angular_velocity_set` | `physics_angular_velocity_radians_set` | velocity |
| `physics_angular_velocity_get` | `physics_angular_velocity_radians_get` | successful result |
| `physics_angular_velocity_maximum_set` | `physics_angular_velocity_maximum_radians_set` | maximum rate |
| `physics_angular_velocity_maximum_get` | `physics_angular_velocity_maximum_radians_get` | successful result |
| `physics_angular_acceleration_set` | `physics_angular_acceleration_radians_set` | acceleration |
| `physics_angle_lock_set` | `physics_angle_lock_radians_set` | min and max |
| `physics_transform_lock_set` | `physics_transform_lock_radians_set` | local_angle |
| `graphics_screen_quad_draw` | `graphics_screen_quad_radians_draw` | angle |
| `graphics_texture_draw` | `graphics_texture_radians_draw` | orientation |
| `graphics_screen_texture_draw` | `graphics_screen_texture_radians_draw` | orientation |
| `graphics_screen_text_scaled_rotated_draw` | `graphics_screen_text_scaled_rotated_radians_draw` | orientation |
| `graphics_sprite_orientation_offset_set` | `graphics_sprite_orientation_offset_radians_set` | offset |
| `graphics_sprite_orientation_offset_get` | `graphics_sprite_orientation_offset_radians_get` | successful result |
| `graphics_animated_sprite_orientation_offset_set` | `graphics_animated_sprite_orientation_offset_radians_set` | offset |
| `graphics_animated_sprite_orientation_offset_get` | `graphics_animated_sprite_orientation_offset_radians_get` | successful result |
| `graphics_camera_rotate` | `graphics_camera_radians_rotate` | rotation amount |
| `graphics_camera_attach` | `graphics_camera_radians_attach` | orientation_offset |
| `graphics_camera_with_options_attach` | `graphics_camera_with_options_radians_attach` | orientation_offset |
| `graphics_camera_attachment_set` | `graphics_camera_attachment_radians_set` | orientation_offset |
| `ui_quad` | `ui_radians_quad` | angle |

The existing camera-ID attachment wrapper is named `rohr_camera_attach`,
which delegates to `graphics_camera_attachment_set`. Its radians sibling is
`rohr_camera_radians_attach`, delegating to
`graphics_camera_attachment_radians_set`. Preserve this established naming;
do not introduce a second `rohr_graphics_camera_attachment_set` wrapper.

Add pure conversion helpers and identical `rohr_` wrappers:

```c
float math_degrees_to_radians(float degrees);
float math_radians_to_degrees(float radians);
```

These helpers only change units: no sign change, quarter-turn, or wrapping.
Use them when filling/reading structs from radian-valued application data.
There are no parallel radians structs or radians variants for whole-struct
operations. `AngularVelocityResult` and `SpriteOrientationResult` wrap scalar
results, so their radians getters are included above. Whole descriptions such
as `CameraResult` stay degree-based. No new orientation getter or unrelated
API redesign is part of this milestone.

### Stored field and indirect API inventory

| Owner/type | Angular fields or exposed values |
| --- | --- |
| Physics component pools | `orientations`, `angular_velocities`, `angular_velocity_maximums`, `angular_accelerations`, `torque_angular_accelerations` and their pool objects |
| `AngleLock` | `min`, `max` |
| `TransformLock` | `local_angle` |
| `Joint` | `rest_angle` |
| `Camera`, `CameraConfig` | `orientation` |
| `CameraAttachment` | `orientation_offset` |
| `Sprite`, `AnimatedSprite` | `orientation_offset` |
| `ViewportItemConfig` | `orientation`, `content_orientation` |
| `ViewportUiTextConfig`, `ViewportUiShapeConfig` | `orientation` |
| `UISliderConfig` | `angle` |
| `EditorRigidBody`, `EditorSoftBody` | `rotation`, `initial_angular_velocity` |
| `EditorAnchor` | `rotation` |
| `EditorJoint` | `rest_angle` |
| `EditorSprite`, `EditorCamera` | `rotation` |
| `EditorAnimatedSprite` | `editor_rotation` |
| `EditorViewportCameraItem` | `placement.orientation`, `placement.content_orientation`, `content_rotation` |
| `EditorViewportUiShape`, `EditorViewportUiItem` | `rotation` |
| Editor commands/history/clipboard | Every transform or rotation payload and initial-angular-velocity property |
| Editor viewport/rotation controls | `group_pointer_angle`, `rotation_pointer_offset`, drag-start angles, accumulated drag rotation |

Whole-struct camera create/set/get and attachment queries, sprite addition,
viewport item and UI configuration APIs, `ui_slider`/`ui_slider_with_text`,
and `physics_joint_component_set` consume/return these degree-based fields
without new radians variants. Joint constructors and
`physics_transform_lock_current_transform_set` derive degree-based relative
angles from stored state. Shape generation APIs with no angle parameter keep
their authored vertex layout; sorting geometry with internal `atan2` does not
make its winding a public clockwise heading. Animation playback `Direction`,
boolean rotation-following flags, vectors, and slider values are not angles.

Audit direct `sin`/`cos`/`atan2`, angle constants, cross products used in physics,
and native draw calls in math, body/COM motion, rigid/soft solvers, constraints,
graphics/UI, editor project transforms and auto-shapes, viewport drawing and
picking, and rotation controls. Update consumers of the helpers as well as
the helpers themselves; do not blindly negate geometric cross products.

## Authoring, compatibility, and goal boundaries

The editor shows degrees by default, and angular-rate fields show degrees/s
or degrees/s squared. Numeric edits, attachment offsets, previews, outlines,
rotation controls, inverse picking, group rotation, and generated runtime
must agree. Continuous dragging accumulates signed pointer deltas across
`atan2` branch boundaries so repeated revolutions are not lost. Preserve the
initial grab offset, one undo transaction, cancellation, and existing particle
editing restrictions. Do not clamp rotation fields to one revolution.

Goal 3 increments `EDITOR_PROJECT_FORMAT_VERSION` from 3 to 4 and
`GAME_STATE_VERSION` from 2 to 3. Runtime saves and templates share the new
state version. Reject previous versions clearly through the existing loader
error paths without replacing the previous successful project description.
Do not infer units from numeric magnitude, offer a compatibility switch, or
silently convert old files. The workspace manifest version stays unchanged:
it has no angular payload affected by this change. Remove the stale comment
claiming that project schemas remain version 1 when updating that header.

Keep existing field names; their units are established by the format version.
Project rotations, initial angular velocities, joint rest angles, camera and
sprite offsets, viewport/UI placement and content angles, and runtime state
`orientation`, `angular_velocity`, `angular_acceleration`, `angle_lock`,
`transform_lock.local_angle`, `joint.rest_angle`, and camera/sprite fields use
the contract above. Persisted physical torque and coefficients keep physical
units. Serialization remains faithful to the properties each format supports;
this work does not add unrelated missing persistence features.

Bundled examples are rewritten to preserve intended appearance and motion.
For an old counterclockwise-radian transform/rate, the equivalent new value is
`-old * 180 / pi`. For old signed physical torque/angular impulse, negate the
value without degree scaling. Negating an interval also swaps its endpoints;
nonnegative angular-speed limits receive unit scaling without sign reversal.
Audit UI/screen values separately against their actual old implementation,
since their coordinate-space adapters may already reverse the sign. Do not
apply a global text substitution to all numbers or all trigonometric calls.

Goal 2 verifies the engine behavior and radians siblings with focused runtime
fixtures. Goal 3 verifies authoring, formats, and generated-code fixtures.
Goal 4 completes bundled example rewrites and integrated parity. Migrate
tests needed to verify each current goal rather than deferring its failures.
Until goals 2 and 3 are both complete, engine/editor unit parity is not ready
for general project use; report that boundary explicitly. Regenerate owned
modules without overwriting developer-owned entry points.

## Acceptance and verification

- Verify 0/90/180/270-degree headings; clockwise vector rotation; inverse,
  composition, and zero identity; radians equivalence and conversion helpers.
- Verify -450 and 810 degree round trips and multi-revolution dragging across
  branch boundaries, including relative offsets, cancellation, and undo/redo.
- Verify `torque / I` and `impulse / I` produce the expected degree-based
  response, offset-point velocity has the correct clockwise sign, and COM stays
  fixed during free rotation. Check direct setters keep origin fixed.
- Verify contacts, pin/weld/spring joints, soft-body torque, inherited motion,
  angle/axis locks, and restricted bodies against analytic or equivalent
  rewritten physical scenarios. Keep area, winding, and inertia unchanged.
- Verify both API layers and radians siblings share handle/error behavior;
  cover conversion overflow where results must remain finite.
- Verify camera forward/inverse transforms, world and screen textures/text,
  static/animated sprite offsets, sliders, viewport content/placement, and
  editor picking/handles. Use two differently rotated/layered screens, clipping,
  and UI above/between them, with independent visibility.
- Verify CLI values, old-format rejection, project/state/template round trips,
  deterministic generated C, and direct/generated/editor runtime equivalence.
- Build affected examples independently against the installed SDK and launch
  Linux binaries from outside their source directories. Verify Windows
  cross-builds and relevant ASan/UBSan tests, distinguishing cross-compilation
  from Windows runtime execution. Request visual review of editor rotations.

Goal 1 verification is documentation-only: compare the inventory to current
headers and serializers, check links and diff whitespace, and confirm no
runtime declarations or behavior changed. Builds and sanitizer runs belong to
the implementation goals.
