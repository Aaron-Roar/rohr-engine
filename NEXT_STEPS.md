# Next Steps

## Priority 0: Editor Example Authoring Parity

Complete this phase before beginning the ten major priorities below. Every
example's static scene and configuration must be representable by an editor
project. Gameplay logic and changes made dynamically at runtime remain
developer-owned C and do not need to be authored in the editor.

1. **Initial motion properties (complete)** — Author initial linear velocity,
   acceleration, and angular velocity for movable entities, plus initial
   soft-node velocity where applicable. Generated projects must use the
   authored values instead of hard-coded zero values.
2. **Project physics settings (complete)** — Author project-wide timestep,
   substep count, gravity or world acceleration, and the solver settings
   required by the examples. Includes editor undo/redo, malformed-input
   coverage, generated runtime application, and Linux/Windows generated-project
   compilation.
3. **Viewport UI sliders (complete)** — Add slider items with value, range, step,
   orientation, styling, interaction, JSON persistence, generated references,
   and runtime creation. Includes editor selection and accordion authoring,
   runtime value/change APIs, round-trip coverage, generated-project checks,
   and Linux/Windows compilation.
4. **Reusable spawning (complete)** — Editor-authored objects generate reusable
   create, draw, and destroy functions. Game code can call a generated create
   function repeatedly or in a loop; each call rebuilds independent entities
   and internal references. Repeated UI mounts likewise receive independent
   mutable runtime resources and placement handles. Spawn timing and gameplay
   decisions remain in C.
5. **Standalone particles (complete)** — Reusable editor-authored particle
   definitions generate public `ParticleConfig` constants and instantiate
   through the particle API.
6. **Advanced soft-body surfaces (complete)** — Beams own independently
   configurable thick-segment collision with per-target endpoint exclusion,
   while generated triangles are visual-only. Areas can disable visual surface
   generation without changing the physical openings defined by their beams,
   with direct C, editor, JSON, generated-C, example, documentation, and
   Linux/Windows coverage.
7. **Input controllers and action declarations (complete)** — Engine-owned
   keyboard, pointer, and text/IME snapshots feed top-level named logical
   controllers with Button, Axis 1D, and Axis 2D actions. Momentary and
   persistent Buttons, editor-authored defaults, runtime overrides,
   CLI/JSON/generated C, converted examples, and Linux/Windows tests are
   supported while gameplay responses remain in C. SDL gamepad device
   management and bindings remain a later extension.

For each item, complete the direct C API where applicable, editor interaction,
JSON persistence, generated C, project loading, runtime application, and
round-trip tests. Convert and build each affected example from its editor
project before marking the item complete.

## Current Priority Order

After Priority 0 is complete, this list is the authoritative priority order.
The detailed sections below are supporting implementation notes and backlog
items; when they conflict with this order, follow this list.

The user-selected **Bugs Identified by Rohr User** milestone takes precedence
over all other unfinished work. Complete its goals in the order below, one
explicitly selected goal at a time. The proposed Editor Forces and Torques
milestone follows it, before numbered priority 4.

1. **Audio (complete)** — Sound effects and music use a small, explicit SDL
   backend with reliable loading, playback, looping, mixing, volume control,
   playback-rate control, and unloading. Sounds decode WAV assets into memory;
   one active music track streams Ogg Vorbis.
2. **Asset and resource ownership (complete)** — Define handles, sharing,
   caching, reference/lifetime rules, failure cleanup, and destruction for
   textures, sounds, animations, fonts, and other shared resources before
   expanding asset-heavy features. Texture, animation, and font resources have
   generation-checked handles, sharing, explicit references, failure and
   shutdown cleanup; animation also has separate mutable playback. The
   Rendered text now has unique generation-checked handles, transactional
   payload revisions, and safe queued-draw and persistent-UI dependencies. The
   Sound players now share cached decoded WAV resources, while music instances
   remain unique with hardened validity, destruction, rollback, and shutdown.
   Direct and wrapper contracts have focused logical coverage, and the complete
   resource set runs under the Linux ownership sanitizer workflow.
3. **COM, inertia, and origin semantics (complete)** — Origin-relative geometry
   and anchors, automatic/explicit COM, derived inertia, COM-based simulation,
   editor authoring, persistence, CLI, and generated C now share the contract.
   Editable examples and integrated native/sanitizer checks verify parity;
   Linux and Windows builds and installed-SDK examples pass.
4. **Cross-path feature parity** — Keep direct C APIs, JSON, generated C, CLI,
   editor tooling, project loading, and runtime behavior equivalent. Treat
   persistence, generation, loading, and round-trip tests as part of every new
   feature rather than follow-up work.
5. **Adversarial testing and hardening** — Add sanitizer configurations,
   regression tests, malformed-input fixtures, allocation-failure coverage,
   physics stress cases, and generated-project build/run checks for Linux and
   Windows.
6. **Physics robustness** — Improve high-speed motion, tunneling, tiny shapes,
   extreme mass ratios, unstable contacts, and scaling behavior. Profile and
   add complexity only for demonstrated failures or measurable bottlenecks.
7. **Editor implementation scalability** — Reduce the number of manual edit
   sites required for each component or property. Move incrementally toward
   shared metadata and generation while preserving the engine's explicit C
   architecture and existing interaction patterns.
8. **Global-state assumptions** — Gradually isolate worlds, simulations,
   graphics state, windows, input, tests, and tools so multiple independent
   instances can coexist. Avoid a disruptive all-at-once rewrite.
9. **Python bindings** — Add a thin, ownership-safe binding over the stable C
   API after lifetime rules and cross-path behavior are clear. Keep C as the
   canonical API and test creation, mutation, errors, and destruction from
   Python.
10. **End-to-end game** — Build and package a small complete game exercising
    physics, joints, rendering, audio, UI, input, assets, saving/loading, and
    distribution. Use concrete problems found by the game to prioritize the
    following engine work.

Rohr Engine is the runtime beneath an optional C authoring tool. Work is
ordered by dependency and engine workflow value: shared runtime models first,
physics semantics second, authoring coverage third, and platform expansion
last.

## Requirements for Every Step

- Keep direct C, CLI, JSON, generated C, project loading, and editor preview in
  behavioral parity wherever the feature applies.
- Add focused tests in the same commit as behavior.
- Keep generated output deterministic and compile it on Linux and Windows.
- Document physics behavior changes before implementing them.
- Preserve explicit ownership, lifetime, and allocation-failure handling.

## Active Milestone: Bugs Identified by Rohr User

Resolve the reported editor selection, zoom, layering, text, auto-shape, and
animation issues, and add animation origins with individual frame offsets.
This milestone takes priority over all other unfinished work, including
Editor Forces and Torques. Preserve the user's goal order; goals 1–7 are complete.

1. [x] **Scale rigid-body line selection thickness with zoom** — Individual
   edge picking now converts the shared six-pixel screen tolerance to world
   units, matching body picking at every zoom. A regression reproduced the
   old failure at 10% zoom and now verifies both sides of horizontal/rotated
   edges at five zoom levels, including clicks outside the tolerance. Linux
   editor and Windows cross-builds pass without compiler warnings; navigation
   and history tests pass natively and under ASan/UBSan. Windows verification
   is cross-compilation only.
2. [x] **Show layers in the editor** — Authored numeric and named layers now
   govern scene drawing, camera previews, and frontmost selection. Soft-body
   children respect inherited layers and individual overrides; screen/UI
   composition resolves named layers immediately. Equal-layer ordering stays
   stable, and authored layers cannot cover editor menus or overlays. Focused
   Linux tests, ASan/UBSan with leak detection, and Windows cross-compilation
   pass without compiler warnings. The installed-SDK center-of-mass example
   builds and runs from outside its source directory. Windows execution and
   interactive visual review remain manual verification.
3. [x] **Scale rotation handles, origins, and COM markers with zoom** — Body
   and group rotation arms, handle rings, origin axes/rings, and the complete
   COM marker retain constant screen dimensions. Origin, rotation, and COM
   picking use screen-space tolerances. Regression coverage checks inside/
   outside tolerance, dragging, release, cancellation, and group rotation at
   10%, 50%, 100%, 200%, and 1000% zoom. Linux navigation/history/startup tests,
   ASan/UBSan with leak detection, and warning-free Linux/Windows builds pass.
   The installed-SDK center-of-mass example builds and runs from outside its
   source directory. Windows execution and interactive visual review remain
   manual verification.
4. [x] **Fix text selection interactions** — Shared text and numeric fields
   place the caret on single-click, select from the press position while
   dragging, and select the whole field on double-click. Numeric mouse focus
   no longer selects all; explicit rename select-all remains available.
   Range replacement/deletion, Ctrl+A, UTF-8 boundaries, and arrow collapse
   are supported without Shift+arrow selection. Captured dragging scrolls
   overflowing fields, respects clipped/translated panels, and cancels on
   focus loss, disappearance, or modal takeover. Public signatures are
   unchanged. Nine focused Linux tests and five ASan/UBSan tests with leak
   detection pass; Linux and Windows builds have no compiler warnings. The
   standalone UI example builds against the updated installed SDK and runs
   outside its source directory. Windows execution and interactive visual
   review remain manual verification. Inline column rename now captures input
   modally so the underlying row cannot select, open, or drag while placing
   the caret or selecting text. A focused regression verifies click, drag,
   double-click, submission, and resumed row input; five Linux follow-up tests,
   ASan/UBSan with leak detection, and Linux/Windows builds pass. The standalone
   UI example also builds and launches outside its source directory.
5. [x] **Restore auto-shape picker interaction and verify selection scope** —
   Hitbox, soft-body, and multiple-selection pickers now share dismissal using
   their current layout bounds, panel translation, scroll offset, and clip.
   Shape clicks survive until release; outside clicks and Escape dismiss.
   Soft-body picker placement is valid on its opening frame. Failed application
   preserves geometry and does not enter auto-shape editing. Existing minimum
   point counts remain; no point selection reshapes all points, while a valid
   subset reshapes only those points. Regression coverage verifies real panel
   clicks, translated/scrolled layouts, accordion changes, disabled shapes,
   failure preservation, and one-entry undo/redo. Six focused Linux tests and
   three ASan/UBSan tests with leak detection pass. Linux and Windows builds
   have no compiler warnings; the installed-SDK UI example builds and launches
   outside its source directory. Windows execution and interactive editor
   verification remain manual checks.
   **Size-preserving conversion follow-up (complete):** Choosing an auto-shape
   now derives its initial dimensions from the affected geometry. Whole hitboxes
   preserve boundary area; selected subsets and soft-body nodes use convex-hull
   area. Scaling uses the generated polygon's area, including low-vertex circles,
   and seeds rectangle/triangle proportions from local bounds. Nonzero collinear
   points use their largest bound as the starting span; coincident, nonfinite,
   or unrepresentable inputs fail without changing geometry or prior settings.
   Whole-hitbox conversion preserves boundary adjacency through concave notches.
   Origins, rotations, and explicit COM remain unchanged; later numeric edits
   resize normally. Repeated conversions, both windings, concavity, subsets,
   scale extremes, rejection, and undo/redo have regression coverage. Six Linux
   tests, four ASan/UBSan tests with leak detection, warning-free Linux/Windows
   builds, and the installed-SDK UI example build/headless launch pass.
6. [x] **Fix Left/Right animation options** — Editor animation rendering now
   honors the authored direction, mirroring Left horizontally before rotation
   just like the app. The same draw path covers playing animations, stopped
   static/frame-editor previews, and camera previews. An internal screen-texture
   draw helper preserves queued texture ownership and existing public APIs.
   Asymmetric two-frame rendering tests reproduce the old failure and verify
   app/editor parity, both directions, playback advancement, stopped frames,
   rotated sprites, and two camera screens with different sizes and rotations.
   Seven focused Linux tests and four ASan/UBSan tests with leak detection pass;
   Linux and Windows builds have no compiler warnings. Player-controller and
   viewport examples build against the updated installed SDK and launch outside
   their source directories. Windows execution and interactive visual review
   remain manual verification.
7. [x] **Add animation origins and individual frame alignment** — Animation
   positions now serve as shared origins; frames retain local center offsets
   and clockwise degree rotations around their own centers. Animation scale
   affects offsets and dimensions, and animation rotation transforms the
   arrangement. Left/Right mirrors only image content. Numeric controls, frame
   dragging, rotation handles, origin markers, picking, context selection, and
   one-entry undo/cancellation use the authored transforms. Frame editing holds
   the selected frame; stopped previews honor the starting frame.
   Public descriptors/frame metadata, immutable cache identity, CLI, project
   JSON, runtime saves/templates, duplication, and generated C preserve
   alignment. Omitted transforms default to zero; nonfinite edits are rejected.
   Six focused Linux tests and five ASan/UBSan tests with leak detection pass.
   Linux/Windows builds have no compiler warnings. A deterministic generated
   fixture builds against both installed SDKs and runs on Linux, checking
   metadata, independent playback, and shared asset lifetime. Flies-in-pit and
   viewport examples build against the Linux SDK and launch outside their source
   directories. Windows execution and interactive visual review remain manual
   verification. Goal 8's frame-scaling investigation remains separate.
8. [ ] **Fix unintended translation during frame scaling** — Investigate and
   correct frames that move when scaled, using the animation-origin and
   frame-offset model established in goal 7.
9. [ ] **Default to rectangles instead of triangles** — Make rectangle the
   default shape.

Clarify material ambiguities when preparing the relevant goal, including
handle/marker zoom behavior, text-selection context, auto-shape with
one selected vertex, animation-origin/offset semantics, and where the default
shape applies. Recording this milestone does not resolve those details or
authorize starting a goal. Complete and verify only the goal the user selects,
then stop for review and a user-created commit.

## Proposed Milestone: Editor Forces and Torques

Author initial one-tick and persistent forces and torques as top-level items
in the object editor, with dedicated property editors and the same interaction
rules as other items. This work follows **Bugs Identified by Rohr User** and
precedes priority 4, cross-path feature parity. This entry records planning; implementation goals
have not been started, and the decisions below still require clarification.

1. [ ] **Finalize the authoring and runtime contract** — Resolve the open
   questions below, agree on public API/data additions and failure behavior,
   and define acceptance checks before changing physics behavior.
2. [ ] **Implement force-at-point and persistent source support** — Add the
   agreed direct/public APIs, application-point and direction-following data,
   COM-based torque response, lifecycle behavior, and runtime serialization.
   Verify one-tick and persistent behavior across physics substeps and the
   supported target types.
3. [ ] **Add authored definitions, commands, and persistence** — Add object-owned
   force/torque records, target references, CLI operations, project JSON,
   validation, duplication/deletion rules, and undoable commands with round-trip
   and invalid-edit coverage.
4. [ ] **Generate runtime sources and initial applications** — Generate instance
   creation/destruction and code-accessible source references. Apply initial
   actions at the agreed lifecycle point, preserve developer-owned entry
   points, and verify repeated instances and deterministic output.
5. [ ] **Implement force editor interactions** — Add top-level object rows,
   dedicated properties, contact-dot/arrow rendering, and the agreed position,
   direction, and magnitude controls. Integrate selection, render-order picking,
   visibility, context menus, multi-edit, clipboard, and transactional history.
6. [ ] **Implement torque editor interactions** — Add top-level object rows,
   dedicated properties, and a 270-degree curved arrow with a tangential
   arrowhead. Reuse shared force/item interaction machinery and verify signed
   direction, zero magnitude, marker placement, and history.
7. [ ] **Verify integrated parity and examples** — Exercise editor, CLI, JSON,
   generated C, direct APIs, and runtime changes together. Add an editable
   demonstration, update public documentation, run focused sanitizers, build
   Linux/Windows installed-SDK consumers, and request visual review.

### Agreed behavior

- Forces and torques are top-level object-editor items, each with its own
  editor and the established editor interaction rules.
- A force is drawn as an arrow pointing away from its contact/application dot.
  A torque is drawn as a three-quarter-circle arrow whose head points along
  the curve, continuing toward its tail if the circle were completed.
- Support persistent sources and initial one-tick forces/torques. Runtime
  changes remain developer-owned code; no delayed editor event system is added.
- A persistent source may begin at zero magnitude and be increased later in
  code. Existing `force_apply`/`torque_apply` operations last one engine physics
  tick; immediate linear/angular impulses are separate operations.
- Retain clockwise-positive degrees and physical force/torque units from the
  completed conventions milestones.

### Open decisions and recommendations

These recommendations are recorded for discussion, not approved requirements.

1. **Targets:** confirm rigid bodies, standalone particles, whole soft bodies,
   and/or individual soft nodes. Suggested initial scope: rigid bodies plus
   forces on standalone particles; particles retain their nonrotating behavior.
2. **Application point:** confirm an origin-relative offset preserved on origin
   mutation, and whether points may lie inside or outside geometry. Recommended:
   permit either; the dot need not coincide with a collision surface. Off-centre
   application derives torque about COM.
3. **Rotation following:** confirm independent settings for the application
   point and force direction, following existing attachment conventions.
4. **Values:** recommend nonnegative force magnitude plus a clockwise degree
   direction, retaining direction at zero magnitude. Torque uses signed
   magnitude, reversing its arrow for negative values; settle its zero display.
5. **Force manipulation:** decide arrow-length scaling and whether its tip edits
   magnitude. Recommended: dot drag edits application point, the shared rotation
   handle edits direction, and a tip handle edits magnitude. Zero sources must
   remain visible and selectable.
6. **Torque marker:** decide COM-centred placement versus a movable display-only
   offset. Recommended: COM by default with an optional display offset so
   multiple sources on one target remain accessible; this offset has no physics
   meaning.
7. **Initial timing:** confirm application during the first physics tick of
   each newly created object instance, including later runtime spawns.
8. **References and lifetime:** recommended target deletion removes its authored
   sources, undo restores both, whole-object duplication remaps targets, and
   duplicating a source alone preserves its target.

### Existing support and remaining gaps

The engine already supports persistent source creation, setters/getters,
`entity_delete` lifetime, one-tick application, separate impulse APIs, COM and
derived inertia, clockwise torque response, and basic runtime JSON storage for
force/torque/target components. Soft bodies have dedicated one-tick distribution
APIs, but their inclusion in this milestone is undecided.

Ordinary force sources currently contain a vector and target, without an
application point or automatic offset-induced torque. A private joint helper
performs force-at-point calculations, but no general public force-at-point API
or persistent attachment description exists. Editor definitions, panels,
glyphs/handles, commands, project persistence, generated source ownership, and
their parity tests remain to be implemented.

Complete and verify one approved goal at a time. Resolve the open decisions
before implementation, and wait for the user to select each goal.

## Completed Milestone: Clockwise Angles with Degrees by Default

Make angles clockwise-positive with zero heading up across engine APIs,
physics, rendering, editor controls, persistence, and generated projects.
Degrees and degree-based rates are the default; explicit radians scalar APIs
provide alternatives. Preserve authored zero geometry, relative-zero identity,
coordinate axes, physical torque/inertia units, and multiple revolutions.
This breaking change is explicitly selected before priority 4. The
[angle convention contract](docs/angle_convention_contract.md) defines the
API/field inventory, conversion rules, format changes, and acceptance checks.

1. [x] **Document the contract and API inventory** — Recorded units, zero and
   relative-angle semantics, exact radians sibling names, degree-based structs
   and component storage, solver/rendering conversion boundaries, project/state
   version changes, example rewrite rules, and verification expectations.
   Documentation and inventory checks pass; runtime behavior is unchanged.
2. [x] **Implement engine conventions and API alternatives** — Shared math,
   angular physics, contacts, joints, soft bodies, attachments, cameras,
   sprites, and viewport/UI rendering use clockwise degrees. Added all 24
   direct/public radians siblings and conversion helpers, preserving physical
   torque/inertia units and unwrapped angles. Linux and Windows engine/editor
   builds and all 12 installed-SDK example builds pass; all 12 Linux examples
   launch headlessly outside their source directories. Runtime tests pass
   32/34 with the previously recorded cameras and ui_field failures unchanged;
   all 10 focused ASan/UBSan tests pass. Windows verification is cross-compilation
   only. Example appearance/motion repair follows in goal 3.
3. [x] **Repair bundled example angles and controls** — Converted authored
   runtime angles/rates and restored fly controls, wheel drive direction and
   the 7 rad/s wheel limit. Repaired cameras, random headings, joint torques,
   state assets, and the COM project's data/generated modules. Four focused
   native tests pass; three example tests pass under ASan/UBSan. All 12
   installed-SDK examples build on Linux and Windows and launch headlessly on
   Linux; Windows verification is cross-compilation only. Bundled data uses
   the new angular units while format-version changes remain goal 4.
4. [x] **Update authoring and persistence** — Editor fields, transforms,
   continuous rotation controls, picking, undo/redo, CLI, and generated C use
   clockwise degrees without wrapping. Project format 4 and state format 3
   reject old versions while preserving previously loaded data; bundled files
   use the new versions. Linux and Windows builds pass without compiler
   warnings. Native tests pass 52/54 with only the previously recorded cameras
   and ui_field failures; all seven focused ASan/UBSan tests pass. An independent
   installed-SDK generated fixture matches direct runtime motion and builds on
   Windows. All 12 installed-SDK examples build on both platforms and launch
   headlessly on Linux. Windows verification is cross-compilation only.
5. [x] **Finish example migration and verify parity** — Audited all bundled
   formats and regenerated both editor projects with identical owned modules
   and unchanged developer entry points. Repaired the standalone generated/direct
   comparison and strengthened multi-turn field, state/template, and attachment
   coverage. Published angular units and reproducible installed-SDK parity checks.
   Linux and Windows builds pass without compiler warnings; native tests pass
   52/54 with only the previously recorded cameras and ui_field failures.
   All 17 focused ASan/UBSan tests pass. The installed-SDK parity fixture runs
   successfully on Linux and cross-builds on Windows; all 12 examples build on
   both platforms and launch headlessly outside their source directories on
   Linux. Windows verification remains cross-compilation only.

Complete and verify one goal at a time, then stop for review and a user-created
commit. The angular-convention milestone is complete. Priority 4, cross-path
feature parity, remains the next unfinished numbered priority. The user has
selected Bugs Identified by Rohr User first, followed by Editor Forces and
Torques, as recorded above.

## Completed Milestone: Consistent Force and Torque API

Deliver the agreed force, torque, and impulse API as a breaking change before
priority 4. Editor authoring and force-at-point remain separate future work.

1. [x] **Implement and verify the force, torque, and impulse API** — Renamed
   one-tick operations to `force_apply`/`torque_apply`, including soft-body
   operations, and component setters to `force_set`/`torque_set`, without old
   aliases. Added stored-value getters and immediate COM-based
   `angular_impulse_apply` through both API layers. Preserved engine-tick
   lifetimes and `entity_delete` ownership for persistent sources. Migrated
   callers, examples, state loading, and public documentation. Direct/wrapper
   tests verify accumulation, expiry across substeps, zero values, deletion,
   stale handles, state round trips, inertia, nonrotating bodies, and invalid
   angular impulses. Linux and Windows builds and all 12 installed-SDK example
   builds pass; all 12 Linux examples launch headlessly outside their source
   directories. Seven focused ASan/UBSan tests pass. Full Linux results are
   49/51 with only the previously recorded cameras and ui_field failures.
   Windows verification is cross-compilation, not runtime execution.

## Temporary Milestone: Render-Ordered Viewport Picking

Make visible draw order govern viewport selection so selected items and their
children cannot capture clicks through covering content. This single-goal fix
is explicitly selected before resuming the origin/COM milestone below.

1. [x] **Resolve viewport input through the frontmost visible target** — Selection,
   child editing, drag initiation, and context menus now share the frontmost
   geometry check, honoring scene order and screen/UI composition layers.
   Regression coverage verifies overlaps, hidden covers, sibling hitboxes and
   soft nodes, modifier/group selection, particle restrictions, captured drags,
   and undo/redo. Linux editor tests and Windows cross-compilation pass;
   installed-SDK examples build and launch headlessly.

## Temporary Milestone: Particle-Enabled Rigid-Body Rotation Controls

Restore editor rotation controls for ordinary rigid bodies with particle
properties while preserving standalone-particle restrictions. This single-goal
fix is explicitly selected before resuming the origin/COM milestone below;
its remaining goals retain their existing order.

1. [x] **Restore and verify rigid-body rotation controls** — Rotation handles
   now display and respond for ordinary rigid bodies with Particle enabled in
   both rigid-body and particle views. Regression coverage verifies
   press/drag/release, one undo entry, undo/redo, unchanged local vertices, and
   standalone-particle restrictions. Linux editor tests and Windows
   cross-compilation pass; runtime physics is unchanged.

## Completed Milestone: Consistent Origins, Center of Mass, and Inertia

Make all origin-relative properties retain their local offsets. Support
automatic or explicit COM, derive inertia, and keep runtime and authoring
behavior equivalent. This implements current priority 3. The
[physics origin contract](docs/physics_origin_contract.md) defines the agreed
behavior and implemented public API. This is a breaking change: rewrite affected
projects and examples without legacy migration or automatic origin recentering.

1. [x] **Document the physics and API contract** — Record origin, rotation,
   COM, inertia, teleport, edit, and error semantics; specify matching direct
   and public wrapper APIs; remove origin-to-centroid functionality from the
   roadmap. This goal changed documentation only.
2. [x] **Preserve origin-relative geometry and attachments** — Removed implicit
   centroid subtraction and editor origin compensation. Collision and rendering
   now use origin-relative vertices; world-hitbox queries reflect edits before
   the next tick. Local anchors, particle centers, attachments, and soft-body
   node offsets are preserved. Off-center/rotated geometry, teleports, variants,
   camera offsets, editor commands, and undo/redo have regression coverage.
3. [x] **Implement COM and inertia throughout runtime physics** — Added entity-owned
   automatic/explicit COM state and matching direct/public APIs, derived inertia,
   COM-based integration, and consistent contact, torque, joint, and soft-body
   response. Origin-relative locks and geometryless motion are preserved;
   invalid geometry edits preserve existing colliders and mass properties.
   Analytic, mutation, body-mode, lock, and solver regression checks pass,
   including focused ASan/UBSan tests, Linux/Windows compilation, and independent
   installed-SDK example builds and headless launches. The full Linux suite has
   only its pre-existing cameras and ui_field failures (45/47 passing).
4. [x] **Persist and generate COM configuration** — Added COM mode/local-offset
   persistence to project data, JSON, CLI, runtime saves/templates, loading, and
   generated C. Invalid input preserves the previous successful description;
   project format 3 and state schema 2 reject older versions. Standalone particles
   retain centroid-only COM and their identity when collisions are disabled.
   Geometryless bodies retain collision filters; particle saves preserve implicit
   radius behavior. Fixed document ownership on reload/destruction. Round trips,
   rollback, CLI edits, deterministic generation, and installed-SDK generated/direct
   runtime parity pass. Linux and Windows builds, five focused ASan/UBSan tests,
   and nine installed-SDK example builds/headless launches pass. The full Linux
   suite has only its pre-existing cameras and ui_field failures (46/48 passing).
5. [x] **Expose COM authoring and visualization** — Added automatic/explicit mode,
   local-offset controls, derived inertia and angular-response readouts, and
   Unavailable values. Selected rigid bodies (including particle-enabled bodies)
   show a blue/white anvil-circle COM handle after their origin, yellow when
   selected. Explicit dragging preserves geometry/origin, respects occlusion,
   and creates one undo entry. Pure particles retain read-only centroid COM.
   Mode seeding/reset, runtime numeric parity, active variants, independent
   duplication, persistence, rotated dragging, cancellation, hidden controls,
   coincident origin priority, and covering body/sprite picking pass.
   Linux/Windows compilation, five focused ASan/UBSan checks, installed-SDK
   joints build/headless launch, and generated/direct COM parity pass. Full
   Linux results are 47/49, with only the known cameras and ui_field failures.
   Visual verification of the marker and controls is requested at review.
6. [x] **Rewrite examples and verify milestone parity** — Added the editable
   center_of_mass example with off-center automatic/explicit bodies, free rotation,
   and local anchors; regenerated the player project with automatic COM.
   Rewrote the game-state example to verify explicit COM, derived inertia,
   teleport preservation, and runtime save/reload. Integrated tests verify analytic
   editor values, project round trips, all four deterministic generated modules,
   generated/direct rotation and anchor parity, and mutation behavior. Updated
   the public API reference and removed obsolete staged-implementation claims.
   Linux/Windows engine/editor builds and all 12 independent installed-SDK example
   builds pass; all 12 Linux examples launch from outside their source directories.
   Eleven focused native tests and eleven ASan/UBSan checks pass. Full Linux
   results are 48/50; only the previously recorded cameras and ui_field failures
   remain. Windows results are cross-compilation, not Windows runtime execution.
   Visual review of the new demo is requested. Priority 3 is complete.

Complete and verify one goal at a time, then stop for review and a user-created
commit. Each goal includes focused tests and relevant example builds; update
affected fixtures when behavior changes, without deferring known failures to
goal 6. All six goals are complete, including bundled example rewrites and
integrated milestone parity. The next priority requires a new user instruction.
Manual inertia, force-emitter tooling, and the broader attachment API redesign
are outside this milestone.

## 1. Shared Render Attachment Model

This is first because sprites already expose body-specific APIs, and the same
model is required by animated sprites and cameras.

- Rename sprite APIs that use `sprite_body_*`; body attachment is optional and
  should not define the sprite API's identity.
- Introduce one explicit attachment value containing the target rigid body,
  position offset, orientation offset, and position/orientation lock settings.
- Share the model between sprites, animated sprites, and later cameras.
- Keep free-placed transforms valid when no attachment is present.
- Migrate engine API, CLI, JSON, generated C, editor GUI, loading, and undo/redo
  in separate reviewable commits.
- Test attachment, detachment, offsets, rotation locks, target destruction, and
  save/load/codegen parity.

## 2. Center of Mass

Center of mass must exist before force-at-point, torque authoring, and accurate
camera or debug visualization of physical bodies.

- Add an explicit rigid-body center of mass used by integration, torque,
  contacts, joints, and debug rendering.
- Author it as a local offset from the origin. When unspecified, calculate the
  active shape centroid as the automatic default.
- Keep local vertices, COM, and attached-point offsets unchanged when the
  origin moves. Do not provide automatic origin recentering or an
  origin-to-centroid action.
- Recalculate automatic COM after active geometry edits; retain explicit COM
  offsets. Derive inertia from supported geometry, mass, and effective COM,
  without manual inertia authoring.
- Follow the [physics origin contract](docs/physics_origin_contract.md) and the
  active milestone above for public APIs, implementation order, and acceptance.
- Test centroid defaults, explicit offsets, transformed bodies, and shape edits.

## 3. Force-at-Point and Torque Response

- Add a force-at-point operation that derives torque from the force vector and
  its offset from the center of mass.
- A force through the center of mass must produce no torque.
- Accumulate derived torque for the current physics tick; do not create a
  persistent hidden torque component.
- Test aligned, perpendicular, rotated, zero-mass, static, and
  kinematic-driven bodies.

## 4. Authored Motion, Forces, and Torques

Initial motion authoring and COM semantics are implemented. The proposed
**Editor Forces and Torques** milestone above is authoritative for the next
work, including initial one-tick applications, persistent sources, top-level
editors, visual representations, and unresolved target/attachment decisions.
Do not treat earlier force-emitter sketches as approval of those open choices.

## 5. Camera Authoring

Camera authoring follows the attachment model but does not depend on force
authoring, so it can begin as soon as section 1 is stable.

- Make cameras object-owned items with position, orientation, zoom, viewport
  dimensions, attachment, visibility, and all remaining camera properties.
- Draw a camera origin, rotation handle, and dotted view boundary. Allow
  resizing through boundary handles and numeric fields.
- Use the shared attachment model so dragging an attached camera or body keeps
  their relationship consistent with sprites and animations.
- Add hierarchy, selection, multi-edit, delete, undo/redo, CLI, JSON, generated
  C, project-load, editor-load, and runtime-load coverage.

## 6. Deterministic Joint Anchor Placement

- Audit pin, weld, and spring authoring so argument order has one meaning:
  `joint_create(anchor_1, anchor_2)` moves the object associated with
  `anchor_1` toward `anchor_2` when placement is requested.
- Preserve the rule for future bounded or maximum-length joints.
- Keep low-level runtime constraint construction free of implicit body movement;
  expose placement as an explicit authoring/helper operation.
- Test reversed arguments, world anchors, shared anchors, locked anchors, and
  connected groups.

## 7. Physics Robustness and Scaling

Complete these as separate physics commits after the immediate COM and force
semantics are stable.

- Add bounded adaptive substeps based on predicted movement and collision
  feature size, exposing the selected count through debug statistics.
- Add swept collision detection for fast rigid bodies, particles, and soft-body
  boundaries through the existing contact pipeline.
- Profile concave contacts, then cache the selected convex-piece pair between
  overlap detection and manifold generation if it materially reduces work.
- Profile persistent concave stacks and replace piece/point feature identifiers
  with original boundary-edge identifiers if topology changes still disturb
  warm-start matching.
- Add rigid-body sleeping only after wake propagation and world-scale-aware
  thresholds are validated without changing ordinary falling or friction.
- Add compound contours and holes only after their ownership and authoring
  representation are explicit.
- Profile before adding a sparse broad-phase interaction candidate index.
- Keep active constraints compact and hot iteration free of full-capacity
  scans.

## 8. Asset Ownership, Audio, and Input Foundations

These runtime foundations should precede broad asset-heavy example authoring.

- Engine-owned texture and animation handles now provide explicit sharing,
  unloading, failure cleanup, and shutdown behavior. Animation data is immutable
  and shared while playback state remains per consumer.
- Cached decoded WAV data now backs independent sound players, and unique
  streamed-music instances have hardened validity, destruction, rollback, and
  shutdown behavior. Focused logical and Linux sanitizer verification covers
  the complete texture, animation, font, text, sound, and music ownership set.
  Shared custom-font handles and unique handle-backed rendered text with safe
  UI and queued-draw dependencies are complete. The binding contracts and audit
  scope are recorded in the
  [architecture guide](docs/architecture.md#remaining-resource-ownership).
- The SDL-backed audio API is complete for in-memory WAV sounds and one
  streamed Ogg Vorbis music track, including looping, mixing, volume, playback
  rate, pause/freeze behavior, and explicit destruction.
- Extend the completed keyboard, pointer, text/IME, and action snapshot with
  SDL gamepad discovery, device lifetime, buttons, triggers, and sticks.
- Keep gamepad changes as separate reviewable commits.

## 9. Remaining Example Authoring Coverage

- Filled soft-body surfaces and topology tooling beyond simple triangles.
- UI layout authoring using primitive UI components.
- Spawn templates and repeated entity arrays.
- Runtime behavior hooks for controls, timed spawning, scoring, and recording;
  behavior remains developer-owned C rather than serialized arbitrary code.

## 10. Object Add Palette

This improves editor usability but does not unblock runtime or generated-C
features, so it follows the missing authoring foundations.

- Replace the object editor's long add list with one Add button that opens a
  grid of named item tiles, following the auto-shape palette style.
- Keep the palette open after adding an item.
- Close it on Escape, an empty click, or opening another menu.
- Add keyboard navigation, disabled-state explanations, and pointer-event
  tests.

## 11. Game-State Serialization Refactor

This is maintenance rather than a feature dependency, so it should happen
after higher-value runtime gaps unless the serializer blocks new work.

- Rename JSON construction helpers such as `state_vec2_write()` and
  `state_color_write()` to names such as `state_vec2_json_create()`.
- Reserve `_write` for functions that actually write into an output
  destination.
- Add save/load round-trip tests before moving code.
- Split the large serializer into focused asset, UI, entity, and component
  modules without changing the format.

## 12. Multiple Windows

This is a large public-API change and should wait until the single-window
authoring/runtime path is stable.

- Add engine-owned window handles and APIs for creating, configuring,
  presenting, and destroying multiple windows.
- Remove assumptions that graphics, UI, input, cameras, or presentation state
  belong to one global window.
- Define event routing, focus, ownership, and shutdown behavior first.
- Validate native Linux and Windows behavior.

## 13. Multiple Viewports per Window

- Allow each window to own multiple viewport handles with independent
  rectangles, cameras, logical sizing, clipping, and draw queues.
- Define how input coordinates and UI focus resolve through overlapping
  viewports.
- Test resize, clipping, camera assignment, destruction, and event routing.

## 14. Viewport Layout Editor

This depends on stable window and viewport handles.

- Add an object/project-level viewport layout model.
- Support adding, selecting, resizing, splitting, reordering, and assigning a
  camera to viewports.
- Generate the same layout through direct C, CLI, JSON, and generated C.
- Test overlapping layouts, invalid camera references, resize behavior, and
  save/load/codegen parity.

## Ongoing Maintenance

- Add CI enforcement for first-party and generated snake_case filenames while
  allowing required ecosystem names such as `CMakeLists.txt` and `README.md`.
- Add sanitizer builds and malformed-project fuzz/fixture coverage.
- Continue converting fixed-capacity editor collections to dynamic arrays.
- Split oversized modules when tests make the move mechanical and safe.

## Later Features

- Tile maps
- Prefabs
- Navigation and pathfinding
- Scripting
- Networking
- Asset hot reload
