# Physics axis setters

Change one coordinate without fetching a snapshot or reconstructing a vector:

```c
EngineResult result = rohr_physics_velocity_x_set(box_id, 100.0f);
if(rohr_error_check(result)) goto done;

result = rohr_physics_acceleration_y_set(box_id, -20.0f);
if(rohr_error_check(result)) goto done;
```

Each setter reads the current runtime value, replaces the requested coordinate,
and uses the existing full-value setter. The other coordinate is preserved in
that property's coordinate space. Calls use `float` values and return
`EngineResult`. Both `rohr_physics_*` and direct `physics_*` names are available.
Existing full-vector setters remain available.

| Function pair (`_x_set` / `_y_set`) | First argument | Coordinates |
|---|---|---|
| `rohr_physics_velocity` | `Entity` | World velocity, distance units per second |
| `rohr_physics_acceleration` | `Entity` | World acceleration, distance units per second squared |
| `rohr_physics_gravity` | None | Global world acceleration |
| `rohr_physics_position` | `Entity` | World position |
| `rohr_physics_center_of_mass_local_position` | `Entity` | Body-local center of mass |
| `rohr_physics_particle_origin` | `Entity` | Body-local particle origin |
| `rohr_physics_joint_anchor_local_position` | `JointAnchorId` | Attached entity's local anchor offset |
| `rohr_physics_soft_body_node_local_position` | `Entity` (node) | Owning soft body's local position |

For example, global gravity uses `rohr_physics_gravity_y_set(-9.8f)`; an anchor
uses `rohr_physics_joint_anchor_local_position_x_set(anchor_id, 2.0f)`.

## Preservation and errors

- Velocity, acceleration, and position use zero for the untouched coordinate
  when the live entity has no stored value. Particle origins and rigid-body
  centers of mass use their existing effective defaults, also zero when absent.
- An automatic rigid-body center of mass is read from its current geometry.
  Editing an axis makes the resulting center explicit. Standalone particles
  continue to reject independent center-of-mass edits; edit their origin instead.
- Soft-body center-of-mass edits use the existing mass-redistribution operation.
  They preserve total mass, positions, and motion; unreachable centers fail.
  A soft body without a defined center of mass cannot supply the other axis.
- Moving a soft-body origin moves its nodes. Moving one local node coordinate
  preserves the other local coordinate; with a rotated body, both world
  coordinates can change. Standalone particle origin edits update its circle,
  polygon, and center of mass together, preserving radius and vertex count.
- Invalid or stale IDs and missing required anchors/soft-body relationships
  return errors. Nonfinite values are rejected, and coordinate bounds and other
  validation match the full-value setters. Rejected edits preserve state.
- These calls acquire no `EntityInfo` snapshot and add no allocation of their
  own. An underlying full-value setter may still allocate to establish geometry
  or component storage according to its existing contract.
- Serialize mutations with engine updates, as with existing physics setters.
  Setting an axis does not lock it; subsequent simulation can change it.

Matching scalar/vector getters and force/torque field access are documented in
[physics field access](physics_field_access.md). Other partial descriptor setters
remain milestone 19 Goal 3.

## Goal 1 verification

- Release: eight tests pass (`physics_axis_setters`, `physics_recovery`,
  `entity_info`, `shared_sources`, `descriptor_creation`, `forces`,
  `center_of_mass`, and `origin_offsets`).
- The focused axis suite passes ASan/UBSan with leak detection. It covers all
  32 direct/facade symbols, invalid/stale IDs, preservation, geometric effects,
  unreachable soft-body centers, and allocation-free ordinary motion edits.
- The same public-only test passes against installed Nix Linux, sanitizer and
  Rocky Linux 8 SDKs. The Windows engine, test and installed consumer cross-build.
- Center-of-mass, player-controller and soft-body examples independently build
  against the installed Release SDK and run from `/tmp`. The first two use
  SDL's dummy backend and exit normally after a bounded smoke run. The car
  completes its full 3,600-frame / 60-second simulation endpoint: 14,400 substeps,
  7,222,164 candidates and 1,195,541 contacts match the existing baseline.
  Its `realtime_pass=0` is retained; this is not performance acceptance.
  All three examples also cross-build against the installed Windows SDK.
- Final builds have no compiler warnings; sanitizer runs report no findings.
  Native Windows execution is unverified. The API inventory includes all new
  symbols and retains its 77 pre-existing diagnostics, with none added.

Evidence and reproducible commands are under `sandbox/milestone_19/goal_1/`.
The initial short car run is retained separately from the complete run. Early
build attempts exhausted disk space; disposable verification binaries were
removed and affected builds were rerun successfully. The existing milestone-18
SDK build/install directories were reused to limit disk usage.
