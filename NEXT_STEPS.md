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
6. **Advanced soft-body surfaces (superseded in part)** — Beams own independently
   configurable thick-segment collision with per-target endpoint exclusion,
   independently of visual fills. The old area authoring and generation paths
   are being removed by Node-Loop Soft-Body Areas; node/beam collision remains
   supported.
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

The user-selected **Correct Soft-Body Angular Velocity** milestone is complete.

Make angular-velocity control update node motion around COM, preserving
translation and deformation, with matching editor-generated initial motion.

1. [x] **Implement node-based soft-body angular-velocity control and verify
   editor/runtime parity** — Derive aggregate angular velocity from node
   momentum and inertia; set it atomically through node velocity changes.
   Prevent independent container angle integration. Correct generated units,
   direction, and COM offsets while preserving explicit node overrides. Verify
   invalid-state rollback, substep motion and beam response, generated/runtime
   parity, sanitizers, and Linux/Windows builds. Verified 71 native tests,
   three ASan/UBSan/leak checks, and Linux/Windows engine/editor builds without
   compiler warnings. Installed-SDK generated applications compile on both
   platforms; Linux runtime checks confirm direct/generated parity, explicit
   node overrides, and independent instances. The generated application and
   standalone soft-body example pass headless startup checks outside their
   source directories. Windows execution remains unverified. Existing projects
   must regenerate C to receive the initial-motion correction.

The user-selected **Reporting Workflow Standards and Audio Contract** milestone
is complete. Reporting instructions and the agreed audio contract are preserved
in the single-goal documentation prerequisite below.

**System Update Reports** Goals 1–6, 8 (engine/lifecycle reports), and 9 (coverage
and documentation) are complete.
Goal 7 (input) is deferred by user choice; retain its current API and direct
errors. Goal 10 (latest-report retrieval and composition) awaits explicit selection,
before resuming Goal 1 of **Unified Forces and Torques**. Establish
per-system timings and diagnostics first; preserve the agreed force/torque API design.
The user revised the goal sequence: shared reporting infrastructure, physics,
console/JSON output, then remaining systems. Simulation-failure recovery comes
after all reporting integrations and their coverage audit. The contract work is
preserved; this reorganization does not start an implementation goal.

The user-selected **Project Editor Error Logging** milestone is complete.

1. [x] **Add project-level editor logs** — Record editor errors in
   `logs/editor.log`, rotate at 1 MiB with three backups, summarize consecutive
   repeats, and handle project lifecycle and logging failures without UI assets.
   Verified 70 native tests, logging/selection ASan/UBSan/leak checks, and
   Linux/Windows builds. A full-editor project-creation probe recorded error 35
   once with 99 repeats and verified the generated ignore rule. Windows
   execution remains unverified.

The user reports that the **Soft-Body Selection Follow-up** now passes manual
verification: node/beam editors open correctly and the capacity error no longer
appears. No additional capacity fix is claimed; investigate further if it recurs.

1. [x] **Fix node editor navigation and area selection** — Verify sibling node
   clicks open the matching editor, preserve soft-body context after COM selection,
   and require viewport double-clicks to select/open areas. Preserve modal node
   picking, layer order, COM priority, and existing hierarchy interactions.
   Fixed stale drag capture when releases are consumed by UI or occur outside
   the viewport. Verified sequential node selection/panel contents, consumed and
   outside releases, COM-to-node navigation, and area double-clicks, including
   the eight-node saved project in `/tmp/rohr-projects/project-dir`. All 69 native
   tests and four ASan/UBSan/leak checks pass; Linux and Windows builds pass.
   Windows execution remains unverified. User manual verification now passes;
   retain project logging for any recurrence.

The user-selected **Soft-Body Mass and COM Controls** milestone is complete,
preceding Editor Forces and Torques. Node masses remain authoritative; total mass
and COM are derived controls without an automatic/manual override.

1. [x] **Implement derived soft-body mass and COM APIs** — Add the mass getter,
   proportional total-mass edits, and transactional minimum-relative-entropy COM
   redistribution. Share allocation-free calculations with future editor
   authoring. Verify transforms, deformation, degenerate node layouts, invalid
   edits, sanitizers, Linux/Windows, and installed-SDK consumers. Verified with
   68 native tests, four ASan/UBSan/leak checks, Linux and Windows compilation,
   an installed-SDK C99 API consumer, and an independently built soft-body
   example launched outside its source directory. Windows execution remains
   unverified.
2. [x] **Integrate mass and COM controls into the editor** — Add derived fields
   and the established selectable/draggable COM marker and dedicated editor.
   Reuse the calculation routine through undoable commands and CLI properties;
   persist and generate only resulting node masses. Verify editor interactions,
   history, duplication, persistence, and generated runtime parity. Generated
   soft bodies now retain their authored origin and rotation before node creation.
   Verified with 69 native tests, five ASan/UBSan/leak checks, Linux and Windows
   engine/editor and generated-application builds, installed-SDK mass/COM and
   instance-isolation checks, and application/example startup outside source
   directories. Windows execution remains unverified; user visual review of the
   Physics fields and COM marker is requested.

The user-selected **Reliable Particle–Beam Contacts** milestone is complete.

Correct particle clipping and preserve restitution when beam contacts interact
with other collisions, using the existing shared physics pipeline.

1. [x] **Fix beam penetration correction and rebound preservation** — Correct
   separation depth for contained projections and retain the initial rebound
   target throughout the iterative contact solve. Preserve mass-dependent
   response, material mixing, collision filters, and endpoint handling. Verify
   isolated impacts, floor-supported beams, the reported generated scene,
   sanitizers, and Linux/Windows builds. No public API or project-format changes;
   continuous collision detection remains outside this goal. Verified with 67
   native tests, six ASan/UBSan/leak checks, Linux and Windows compilation, and
   installed-SDK application/example startup checks. The supplied scene's first
   impact produces relative rebound 54.9956 from approach 68.7445, matching its
   effective restitution of 0.8. Its normal application build was refreshed.

The user-selected **Independent Beam Materials** milestone is complete.

Give soft-body beams independent collision friction and restitution across the
engine, editor, CLI, saved projects, and generated applications.

1. [x] **Add beam friction and restitution across engine, editor, and generated
   projects** — Use beam materials for beam contact response, with friction 0
   and restitution 0.25 as independent defaults. Add a Material accordion and
   multi-select editing, history, persistence, and end-to-end verification.
   Existing projects missing the values use these defaults: custom endpoint
   materials no longer determine beam response. Node contacts remain unchanged.
   Verified with 66 native tests, four focused ASan/UBSan/leak tests, Linux and
   Windows builds, and installed-SDK application/example smoke checks. The
   generated particle-in-hole scene confirms repeated beam contacts and rebound.

The user-selected **Particle Editor Parity** milestone is complete.

Expose applicable particle properties in the same measured accordion structure
as rigid bodies, preserving standalone particle restrictions and radius editing.

1. [x] **Add missing particle fields and matching accordions** — Add Mass,
   transform, initial motion, physics, material, collision, parenting, appearance,
   and geometry sections. Use commands/history for edits including Auto Fit;
   verify UI input, persistence, generated configuration, and platform builds.
   Native tests (64), focused ASan/UBSan/leak tests (4), Linux and Windows
   compilation, and installed-SDK generated application/example smoke checks
   pass. Standalone restrictions and the dedicated radius editor are preserved.

The user-selected **COM Selection Priority** goal is complete, including its
dedicated COM editor and persistent automatic selection.

The user-selected **Consistent Collision Controls** single-goal milestone and
Node-Loop Soft-Body Areas are complete. System Update Reports precedes Unified
Forces and Torques.

The user-selected **Stable Area Colors During Motion** milestone is complete.
It fixes runtime color flashing caused by numerical region detection failures.

The user-selected **Beam-Bounded Soft-Body Area Colors** milestone is complete.
It supersedes the material-position fill behavior of **Stable Soft-Body Areas**.

The user-selected **Bugs Identified by Rohr User** milestone is complete.
The **System Update Reports** prerequisite and then **Unified Forces and
Torques** precede numbered priority 4. Pursue only an explicitly selected goal.

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

## Single-Goal Milestone: COM Selection Priority

Make visible COM markers render and select above scene objects and other handles,
opening their own editor for both automatic and explicit COM.

1. [x] **Prioritize COM rendering and selection, with a dedicated COM editor** —
   Give automatic and explicit COM first priority for ordinary viewport clicks,
   including autoshape. Persistent COM selection opens COM Properties; automatic
   coordinates remain read-only and explicit COM preserves dragging and undo.
   Dedicated and inline controls share values and commands. Back returns to the
   body; missing owners exit safely. Preserve zoom sizing, visibility, selection
   modifiers, and menu capture. Verified with 63 native tests, four focused
   ASan/UBSan/LSan tests, Linux and Windows builds, and an installed-SDK example
   build/startup check. Tests cover the main selection completion path, panel
   input, navigation persistence, mode switching, and undo/redo. Interactive
   visual review remains a user follow-up.

## Single-Goal Milestone: Consistent Collision Controls

Make collision settings available for every compatible selection, with independent
filter sections directly beneath their respective buttons, editable even when
collision is disabled.

1. [x] **Unify collision controls across individual and multi-selection editors** —
   Share eligibility for rigid bodies, particles, soft nodes, and beams; expose
   mixed states and independent category/collides-with expansion. Apply bulk
   edits atomically through existing commands/history, preserving unrelated bits.
   Verify layout, scrolling, selection changes, rollback, sanitizers, and Linux/
   Windows builds. No public API, persistence format, or physics changes.
   Completed with 62 native tests passing, four focused ASan/UBSan/LSan tests
   passing, Linux and Windows compilation, and an installed-SDK soft-body
   example build and headless startup check. Visual editor review remains a
   user follow-up.

## Single-Goal Milestone: Restore Rigid-Contact Stability Coverage

Correct the stability fixture's gravity setup so it exercises resting contact
and friction under negative gravity. This user-selected correction precedes
the next Bugs Identified by Rohr User goal; engine behavior remains unchanged.

1. [x] **Correct the stability test's gravity setup** — The fixture explicitly
   configures `(0, -9.8)` gravity with error handling, places the floor at
   `y = -100` and the body at `y = -20`, and preserves all geometry, timing,
   contact, drift, velocity, and friction assertions. The full Linux suite now
   passes 57/57 tests. The stability test passes ASan/LSan/UBSan with its own
   test source instrumented. Linux and Windows builds pass without compiler
   warnings; Windows verification is cross-compilation only.

## Single-Goal Milestone: Reliable Text Input Across the Editor

Keep text entry usable through initial selection, repeated clicks, double-click
selection, and focus transfers between fields, dialogs, and the terminal.
This user-selected correction precedes the next Bugs Identified by Rohr User goal.

1. [x] **Fix shared text-input focus and verify all editor field paths** —
   The terminal only stops text input on its own focus loss; focused shared
   fields restore inactive SDL text input without restarting active sessions.
   Modal controls block terminal input, and terminal-consumed events no longer
   reach UI widgets. Regression coverage checks real SDL activation alongside
   editing, including first/repeated clicks, selection replacement, delayed
   window focus, keyboard/programmatic activation, field/terminal transfers,
   multiline/numeric fields, and the actual new-project name dialog. All eight
   focused tests and four ASan/LSan/UBSan checks pass. Linux and Windows builds
   pass without compiler warnings; Windows verification is cross-compilation.
   The installed-SDK UI example builds and launches outside its source directory.
   At this goal's completion the full Linux suite passed 56/57 tests, with a
   pre-existing `rigid_contact_stability` fixture failure under negative gravity.
   The Restore Rigid-Contact Stability Coverage milestone above resolves that
   separate failure and restores 57/57 passing tests. Desktop interaction review remains
   recommended; automated editor interaction checks use SDL's dummy backend.

## Single-Goal Milestone: Default Gravity

Use downward gravity of `(0, -9.8)` for engine initialization and new editor
project settings. Existing explicitly authored gravity values remain intact.
This user-selected correction precedes the next Bugs Identified by Rohr User goal.

1. [x] **Set engine and editor default gravity to -9.8** — The shared default
   and engine initial state now use `(0, -9.8)`, including new editor projects.
   Five existing physics/editor tests pass. A generated-project check verifies
   startup, restart, and generated settings application on Linux; engine/editor
   and generated-project builds pass on Linux and Windows without compiler
   warnings. The installed-SDK center-of-mass example builds and launches outside
   its source directory. Windows verification is cross-compilation only;
   sanitizers were not rerun for this default-value-only change.

## Completed Milestone: Boundary-Correct Soft-Body Areas and Holes

Fill current outer-loop enclosed regions once, subtracting only the owning area's
hole loops. Preserve area identity and styling through crossings and deformation.

1. [x] **Replace fixed fills and add owned holes** — Added copied geometry with
   an outer loop and up to 16 owned holes, explicit maximum-hole errors, and
   transactional replacement. Current boundaries generate nonoverlapping fills
   with area-local subtraction and alpha blending; unchanged poses reuse cached
   geometry. Removed obsolete fixed triangles and authored reference positions.
   Tests cover initial crossings, hourglass spill, windows/notches, overlapping
   holes, independent areas, transparency, collapse, ordering and deletion.
   Added moving-window visual review alongside hourglass and layered scenes.
   All 59 native tests, three ASan/UBSan/leak checks, Linux/Windows builds, a
   sanitized visual cycle, and the installed-SDK vehicle build/headless launch
   pass. Windows is compile-only; desktop review remains requested. Optimized
   geometry benchmarks on this machine measured about 0.005 ms per ordinary
   window rebuild and 16 ms for 17 tangled loops of 64 shared nodes each.
   The latter is an expensive worst case; unchanged draws reuse the pose cache.
   Resume Node-Loop Soft-Body Areas Goal 4 only on explicit user instruction.

## Completed Milestone: Soft-Body Area Visual Test

Inspect area colors through a square-to-hourglass fold while keeping diagnostic
scenes in test executables and the standalone vehicle example focused.

1. [x] **Isolate area demonstrations and add hourglass motion** — Moved the
   layered demo into `soft_body_areas_test --visual` and added a square whose
   bottom vertices swap and return over eight seconds, with endpoint pauses,
   fixed triangulation, white boundaries, and node markers. Onscreen buttons
   select scenes; Space pauses and L changes the layered scene's overlap order.
   Removed all demo flags/code from the vehicle example. Three focused native
   tests and three ASan/UBSan/leak checks pass, including motion endpoint and
   unchanged-triangulation checks. Linux/Windows builds pass without compiler
   warnings (Windows compile-only). A complete visual cycle also runs cleanly
   under sanitizers; the installed-SDK vehicle launches and exits from outside
   the source directory. Desktop visual review remains requested. Resume the
   main milestone only when the user requests Goal 4.

## Milestone: Node-Loop Soft-Body Areas

Replace beam-derived coloring with explicit ordered loops of existing soft-body
nodes. The last node connects implicitly to the first. Areas need no physical
boundary beams; separate areas may share nodes. Higher layers and later entries
within a layer draw on top, limited to each area's own footprint.

1. [x] **Remove beam-derived coloring** — Removed obsolete geometry discovery,
   storage, public area APIs, editor panels/picking, CLI selectors, persistence,
   and generated area code. Node/beam editing and physics remain operational;
   the separate manual triangle primitives remain supported. Project schema 7
   rejects old files without migration. Area fills are temporarily unavailable;
   state this explicitly in the commit. Also fixed the user-approved Auto Shape
   out-of-bounds read for selected hitbox vertices and soft-body nodes by keeping
   temporary positions indexed consistently with their original arrays.
   Regressions cover sparse shuffled selections, unchanged unselected points,
   and transactional rejection of duplicate/missing IDs. All 57 native tests,
   five ASan/UBSan/leak checks, Linux/Windows builds, and installed-SDK example
   and generated-app headless launches pass. Windows is compile-only; desktop
   visual review is recommended before committing.
2. [x] **Build ordered-loop geometry** — Added internal, allocation-free loop
   validation and deterministic triangulation for convex/concave polygons in
   either winding, retaining collinear boundary nodes. Invalid initial loops
   leave previous output intact. Fixed indices address the original node order;
   resolving current triangle positions permits later folds, inversions, and
   collapse without changing connectivity. Tests cover boundaries, area coverage,
   all-node participation, scale/translation, invalid input, deformation, and
   200 seeded polygons through the 100-point geometry limit. All 58 native tests,
   the geometry ASan/UBSan/leak check, Linux/Windows builds, and an installed-SDK
   soft-body example build/headless launch pass. Windows is compile-only.
   Runtime areas, public APIs, and editor fills remain for subsequent goals.
3. [x] **Add runtime areas and API** — Added copied, value-type node loops,
   transactional creation/edits, authored local reference positions, and private
   fixed triangulation. Areas own visibility/color overrides, use entity layers,
   and retain explicit list order; deletion preserves surviving order and node
   or body deletion removes dependent areas. Removed manual triangle APIs,
   components, and storage without compatibility wrappers; migrated callers to
   three-node areas. Originally added a runtime example mode for concave,
   overlapping colored fills; these diagnostics now belong to the separate
   visual-test milestone above. All 59 native tests, three ASan/UBSan/leak checks, Linux/Windows
   builds, and both installed-SDK example modes pass. Windows is compile-only;
   desktop visual review remains requested. No holes or independent visual
   vertices at this historical boundary; runtime holes and dynamic fills are
   superseded by the Boundary-Correct Soft-Body Areas and Holes milestone above.
   Editor persistence, generation, and authoring remain below.
4. [x] **Connect editor data and app generation** — Added persistent ordered
   areas and owned holes with stable owner-local IDs, styles, layers, and saved
   drafts. Cached current-boundary geometry drives scene/camera fills and
   occlusion-aware picking of the owning body; incomplete loops show amber
   outlines/markers without filling their area. Generation validates every loop
   before writing, reports the affected object/body/area/hole, and emits copied
   runtime geometry, color, visibility, and layer bindings. No reference poses
   or authored triangles are stored. Default UI/starter soft bodies have four
   square nodes, six beams, and one square area. Existing schema-7 bodies without
   areas load unchanged. Copies/history own independent caches; node deletion
   removes dependent areas and undo restores them. All 60 native tests, four
   editor ASan/UBSan/leak checks, Linux/Windows builds, and the independent
   installed-SDK generated-area fixture and vehicle example pass. Both apps
   launch from /tmp; Windows is compile-only. Desktop visual review remains
   requested; authoring controls follow in Goal 5.
5. [x] **Add area authoring and verify parity** — Added the Areas accordion,
   Add Area, and area/hole editors. Rows select on one click and open on two;
   creation opens the new editor. The node-picking box records click order,
   toggles membership, and appends reselected nodes; numbered rows reorder or
   remove members. Ending picking restores normal node navigation without
   changing the definition. Areas expose name, visibility, color, layers, and
   shared hierarchy reordering/context actions. Owned holes use the same loop
   controls without independent appearance, capped at 16 including drafts.
   GUI and selector-first CLI authoring share commands, validation, and undo;
   drafts, saved navigation, node-deletion cascades, and generated parity remain
   covered. Updated the ordinary wheel example to one tire loop with a hub hole,
   with no example flags or physics changes. Fixed accordion layout accumulation
   caught by the Auto Shape regression and kept modal Escape handling ahead of
   picker cancellation. All 61 native tests, six ASan/UBSan/leak checks, and
   Linux/Windows builds pass without compiler warnings. The installed-SDK
   generated-area app and wheel example build and launch from /tmp. Windows is
   compile-only; desktop visual review of authoring and generated fills remains
   requested before committing.


The earlier Geometrically Bounded Area Overlap goal is superseded. Its unresolved
missing-crossing recovery rules are discarded with the beam-derived model.
Earlier completed area milestones below record history, not current support.

## Completed Milestone: Stable Area Colors During Motion

Keep assigned area colors and ownership stable during ordinary movement while
preserving genuine beam-driven splits, merges, and layer ordering.

1. [x] **Fix motion-induced area color flashing** — Corrected mixed-precision
   geometry predicates, bound unchanged beam boundaries to existing owners, and
   preserved boundary identity across remapped or recycled entity IDs. Verified
   7,000 geometry poses, 1,024 physics updates, and 1,200 generated-runtime motion
   frames with unchanged ownership and colors. All 58 native tests and four
   ASan/UBSan regressions pass; Linux and Windows builds pass without compiler
   warnings. Generated app and installed-SDK soft-body example launch headlessly;
   desktop visual verification remains requested.

## Completed Milestone: Beam-Bounded Soft-Body Area Colors

Make fills follow current beam geometry while preserving editable area identities
and colors through deformation, merging, and separation.

1. [x] **Correct area boundaries, color preservation, and overlap ordering** —
   Resolve live beam intersections and enclosed regions; preserve parent IDs and
   colors through splits; resolve merged coverage by layer and editor list order
   without deleting covered definitions. Keep drawing, picking, outlines,
   persistence, and generated runtime behavior consistent. Verify geometry,
   color overrides, reorder/history, native tests, sanitizers, generated apps,
   installed-SDK examples, and Windows compilation. No collision changes.
   Completed: live beam intersections, stable definitions across split/merge
   and separation, shared coverage for newly enclosed regions, preserved
   explicit holes, and matching fill/picking/outline geometry. List order now
   controls equal-layer drawing, picking, and generated area creation. Fixed
   numerical duplication of crossing vertices and resolve drawing meshes once
   per body. Existing schema and stored color overrides remain supported.
   All 58 native tests and four ASan/UBSan/LSan checks pass, including 50
   deterministic changing-crossing poses. Linux and Windows builds pass without
   compiler warnings. A regenerated project fixture verifies colors, layers,
   order, merge/separation, and drawing against the installed SDK; the generated
   app and standalone soft-body example build and launch headlessly from outside
   their source directories. Windows verification is compile-only. User desktop
   verification remains recommended after restarting the rebuilt editor and
   regenerating/rebuilding the project against this engine.

## Completed Milestone: Stable Soft-Body Areas

Establish areas from beam connectivity and geometry when connections change,
then preserve their identities and material boundaries through deformation,
saving, loading, editor selection, and generated runtime rendering.

1. [x] **Fix area creation, identity, deformation, and persistence** — Discover
   minimal enclosed regions including beam intersections; preserve matching
   boundaries and inherit new-region styling by greatest overlap. Keep node
   movement independent of area discovery, support folded surfaces, expose
   persistent runtime handles, migrate version-5 area loops without rediscovery,
   and recover the reported project with a backup. Verify history, picking,
   JSON, generated C, native/sanitizer checks, and platform builds.
   Completed: persistent node/intersection boundaries, even-odd rendering and
   selection (including folds and nested holes), ID and overlap-based property
   retention, explicit runtime rebuild and area queries, and version-5 migration
   into version 6. Recovered the reported project with a timestamped backup;
   every pre-existing field except the format version is preserved. All 58 native
   tests and four ASan/UBSan/LSan editor/area checks pass, with editor code now
   instrumented. Fixed an uninitialized particle-vertex lock flag, an empty-list
   clone copy, and a stale test pointer exposed by that verification. Linux and
   Windows cross-builds pass without compiler warnings. The recovered scene plus
   a crossing-area fixture and the soft-body example build against the installed
   SDK and launch headlessly outside their source directories. Windows checks
   are compilation only; desktop visual review remains recommended.

## Completed Milestone: Bugs Identified by Rohr User

Resolve the reported editor selection, zoom, layering, text, auto-shape, and
animation issues, add animation origins with individual frame offsets, and
create square shapes by default. All nine goals are complete. The proposed
Editor Forces and Torques milestone follows this work.

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
   affects dimensions only (clarified in goal 8), and animation rotation
   transforms the arrangement. Left/Right mirrors only image content. Numeric controls, frame
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
   verification. Goal 8 subsequently corrects animation-scale offset handling.
   **Mutable frame values follow-up (complete):** Reusable frame values now hold
   scale, offset, and rotation independently of shared image catalogs. Each sprite
   owns copied transforms and image references; mutation preserves playback and
   rolls back on invalid input. Scale 1 uses native pixels, signed axes flip image
   content before rotation, Left combines with the horizontal sign, and zero scale
   collapses drawing and image selection. Editor controls, multi-edit, CLI, undo,
   generated C, runtime saves/templates, documentation, and bundled examples use
   this model. Project format 5 and runtime state version 4 reject old definitions.
   Runtime saves preserve replaced/appended frames and keep local image paths
   relative. The 56-test Linux suite and seven ASan/UBSan tests with leak detection
   pass; Linux/Windows builds have no compiler warnings. Nine standalone examples
   build against the installed Linux SDK and launch outside their source folders.
   Generated code with signed/zero scale builds against both SDKs and runs on Linux,
   verifying independent transforms, playback, and shared image lifetime. Windows
   execution and interactive editor review remain manual checks.
8. [x] **Fix unintended translation during frame scaling** — Animation scale
   multiplies only child image dimensions, leaving offsets and centers unchanged.
   Animation rotation still rotates offsets around the origin. Runtime drawing
   and the shared editor center calculation use unscaled offsets; frame dragging
   removes parent rotation without dividing by animation scale. Regression checks
   cover shrinking/enlarging, nonuniform scales, rotated frames, Left/Right,
   frame previews, two camera screens, picking, dragging at three zoom levels,
   cancellation, and undo/redo. The regression fails with the unchanged runtime.
   All 57 Linux tests and four ASan/LSan/UBSan checks pass. Linux and Windows
   builds pass without compiler warnings; Windows verification is cross-compilation.
   Flies-in-pit and viewport examples build against the updated installed SDK
   and launch outside their source directories. Public signatures and saved
   values/formats are unchanged; offset interpretation follows the clarified
   contract. Desktop visual review remains recommended.
9. [x] **Create square shapes by default** — New UI shapes and rigid-body
   hitboxes start as axis-aligned squares. The editor/CLI Add Soft Body command
   creates four square corner nodes and six unique beams connecting every node
   pair (both diagonals, no center node). Preserve authored/saved geometry and
   explicit Auto Shape choices. UI squares are 180-by-180; body squares are
   140-by-140. Low-level soft-body construction stays empty for custom geometry
   builders. Creation, all-pairs connectivity, undo/redo, and persistence are
   covered by regression checks. All 57 native tests and four focused
   ASan/UBSan/LSan checks pass; Linux and Windows cross-builds pass without
   compiler warnings. A fresh CLI-created project generates the expected
   geometry, builds against the installed SDK, and launches headlessly from
   outside its source directory; the installed-SDK UI example also builds and
   launches. Windows verification is compilation only. Desktop visual review
   remains recommended.

Clarify material ambiguities when preparing the relevant goal, including
handle/marker zoom behavior, text-selection context, auto-shape with
one selected vertex, animation-origin/offset semantics, and where the default
shape applies. Recording this milestone does not resolve those details or
authorize starting a goal. Complete and verify only the goal the user selects,
then stop for review and a user-created commit.

## Prerequisite Milestone: Reporting Workflow Standards and Audio Contract

Preserve reusable reporting standards and the agreed audio design before
resuming System Update Reports. This is documentation-only work; audio
implementation remains Goal 5 of the existing milestone, with numbering intact.

1. [x] **Document reporting instructions and the audio Goal 5 contract** — Add
   mandatory [reporting instructions](docs/agent_instructions/system_reporting.md)
   routed from `AGENTS.md`; preserve the approved but unimplemented
   [audio contract](docs/audio_reporting_contract.md), explicitly separating open
   implementation details. Update roadmap priority and contract links, then
   verify documentation consistency without runtime changes. Stop before audio
   implementation and wait for explicit instruction to resume Goal 5.
   Completed: reporting instructions and root routing, approved audio behavior
   separated from open implementation details, and roadmap/contract links.
   Verified local links, contract consistency, milestone numbering, and a
   documentation-only diff. Runtime builds and sanitizers are not applicable.
   `AGENTS.md` and `docs/agent_instructions/` remain locally maintained under the
   existing Git ignore rules; the audio contract and roadmap are versioned.

## Prerequisite Milestone: System Update Reports

Give each top-level engine system its own update report containing timings,
system-specific statistics, recoverable warnings, and fatal failure details.
Use one shared diagnostic format with system-owned storage, not an engine-wide
queue or an additional simulation pipeline. Complete this milestone before
resuming Unified Forces and Torques. Build the shared reporting system first,
integrate physics next, then add console/JSON output and the remaining system
reports. Address simulation-failure recovery only after reporting coverage is
complete. The [reporting contract](docs/physics_reporting_contract.md) preserves
the agreed design; finalize system-specific runnable boundaries in each
integration goal before modifying that system. The documentation prerequisite
above preserves [audio Goal 5's approved contract](docs/audio_reporting_contract.md).

1. [x] **Implement and verify the shared report system** — Finalize and add
   the common types, enum IDs, constructors, collection helpers, report merging,
   and public/direct APIs in one goal. Verify value ownership, default records,
   aggregation rules, copied details, overflow, saturation, and fatal preservation.
   One shared fixed-size `Report` uses enum IDs such as `ROHR_PHYSICS_REPORT`,
   with 32 entries, 64 statistics, and 256 diagnostics defined by named constants.
   Include NONE severity for default records, visible overflow, and reserved
   fatal details. Collection and merging must not allocate, print, or run any
   simulation. Public helpers support developer-built reports and reserved custom
   report/metric ID ranges; custom metrics state their aggregation rule. Copy
   diagnostic text into a named 256-byte detail buffer and flag truncation.
   Preserve the first critical message in dedicated fatal storage even when the
   ordinary diagnostic list is full; that retained fatal is not counted as
   dropped. Merge order determines first fatal and latest-value statistics;
   successful children cannot clear failure or overwrite parent elapsed time.
   Do not integrate systems or console/JSON output in this goal.
   Completed: shared/public helpers, custom IDs, owned diagnostic text, bounded
   aggregation/merging, saturation, and reserved first-fatal preservation.
   Verified 72 native tests, focused ASan/UBSan/LeakSanitizer, Linux and Windows
   builds, installed-SDK consumers (Linux executed; Windows compiled), and an
   independent installed-SDK joints example with headless startup/shutdown.
   Windows execution remains unverified. See [Shared reports](docs/reports.md).
2. [x] **Integrate and verify physics reports** — Finalize directly runnable
   physics entry points and their prerequisites, including the editor's
   joints-only use case. Return the shared `Report` by value and combine child
   reports in full updates; internal helpers contribute without generating
   reports themselves. Migrate return types and callers, remove obsolete report
   getters, and preserve metric meaning. Verify standalone/full-update parity,
   substeps, custom composition, warning/fatal propagation, and retained values.
   Preserve current quarantine/simulation-failure behavior until Goal 11. Prepare
   for future force geometry diagnostics without implementing the force API.
   Completed: runnable physics APIs return owned reports, aggregate child work,
   retain substep/entity diagnostics, and preserve existing quarantine behavior.
   Removed the old metrics-only report/getters; migrated examples, generated
   entry points, and test callers. Previously ignored allocation failures now
   stop dependent work and return fatal details. Verified 73 native tests,
   ASan/UBSan/LeakSanitizer on reporting/lifecycle/beam cases, Linux and Windows
   builds without compiler warnings, nine independent installed-SDK example
   startup/shutdown checks, and generated-project runtime parity and startup.
   Corrected the generated COM test fixture's stale triangle assumption.
   Windows execution remains unverified. See [Physics reports](docs/reports.md#physics-reports).
3. [x] **Add console, JSON, and text-file report output** — Implement
   `rohr_report_console_write`, `rohr_report_json_write`, and
   `rohr_report_text_write`, with matching direct APIs. Share a readable text
   formatter and use existing yyjson for versioned JSON. Defer the general file
   API; output functions own their file handles. Increase the named console
   buffer to 1,024 bytes,
   format reports line by line, and detect truncation. Keep its 100-entry history
   distinct from full terminal output. Verify readable names, complete report
   content, overflow/fatal details, truncation and I/O errors, and temporary
   allocation cleanup. Output errors do not mutate source reports; collection
   remains independent of printing and serialization.
   Completed: shared console/text formatting, version 1 JSON with exact integer
   strings and enum names, checked output errors, and visible console truncation.
   Verified 74 native tests, three focused ASan/UBSan/LeakSanitizer tests including
   allocation/I/O failure injection, Linux and Windows builds without compiler
   warnings, installed-SDK consumers (Linux executed; Windows compiled), and an
   independent installed-SDK joints example launched outside its source directory.
   Windows execution remains unverified. The general file API stays deferred.
4. [x] **Define and integrate graphics update reports** — Audit rendering entry
   points and define submission/frame/presentation boundaries and compatibility
   changes. Reuse the shared report format for rendering-stage
   timings, counters, skipped work, and fatal failures. Cover drawing, cameras,
   screens/viewports, recording, and presentation. Preserve rendering order and
   resource ownership; verify recovery and retained diagnostics after failure.
   Completed: independently runnable cameras, commands, viewports, recording,
   and presentation return shared reports composed by `graphics_show()`;
   animation updates return an independent report. Command submission evidence
   survives until execution; failures retain copied backend details and queued
   resources are released on execution/abort. Recording failure still permits
   presentation. Verified 75 native tests, six ASan/UBSan/LeakSanitizer tests,
   Linux/Windows builds without compiler warnings, installed-SDK consumers
   (Linux executed; Windows compiled), all 11 independently built example
   startup/shutdown checks, and generated-application build/startup/shutdown.
   Windows execution remains unverified. User visual review of editor/application
   rendering parity remains a follow-up. See [Graphics reports](docs/reports.md#graphics-reports).
5. [x] **Define and integrate audio update reports** — Implemented the
   [audio contract](docs/audio_reporting_contract.md): a 256-request FIFO supports
   concurrent callers, deferred controls/setters, synchronous resource/service
   lifecycle, and applied-state getters. Update submission collects completed
   callback evidence for commands, sounds, music, and output, including later
   failures from earlier requests. Shutdown returns a final report retained for
   the next update once. Manual mixing/output is explicitly deferred; callback
   report owners are not exposed for direct execution in this goal.
   Verified 76 native tests, five ASan/UBSan/LeakSanitizer tests, deterministic
   delayed/overflow/fatal tests and concurrent producer/destruction/shutdown
   accounting, Linux/Windows builds without compiler warnings, installed-SDK
   consumers (Linux executed; Windows compiled), and independent installed-SDK
   audio example builds on both platforms with Linux startup/shutdown outside
   its source directory. Measured callback/report collection cost with a test
   output sink, including full diagnostic storage. Windows execution and audible
   playback on real hardware remain unverified. See [Audio reports](docs/reports.md#audio-reports).

6. [x] **Define and integrate UI reports** — Completed ahead of input reporting.
   Replaced begin/end without aliases: `ui_input_set()` replaces the snapshot,
   immediate widgets retain their behavior, and `ui_update()` returns one owned
   `ROHR_UI_REPORT`. Evidence includes events received before input/widgets,
   interaction evaluation counts, named capacity peaks, and recoverable capacity
   and scope warnings. Update clears transient input and registration, preserves
   ongoing controls, and unwinds only UI-owned scopes; graphics failures remain
   in graphics reports. Timing covers only update execution. Migrated editor,
   examples, wrappers, tests and documentation; audited generated templates.
   Verified 77 native tests, ten focused ASan/UBSan/LeakSanitizer tests, and
   Linux/Windows builds without compiler warnings. Installed-SDK consumers build
   on both platforms (Linux executed); audio and user-interface examples build
   independently on both and pass Linux headless startup/shutdown outside their
   source directories. Windows execution remains unverified. Manual editor/UI
   rendering review remains a follow-up. See [UI reports](docs/reports.md#ui-reports).
7. [ ] **Define and integrate input update reports — deferred** — The user chose
   to leave input unchanged. Keep immediate operation results and text/IME
   truncation flags; do not add counters, pipelines, or a report merely for
   symmetry. Revisit only on explicit user instruction.
8. [x] **Integrate engine-tick and remaining system reports** — Cover clock/tick
   advancement and entity lifetime cleanup. Audit remaining top-level update
   entry points for missing report owners. Verify update boundaries, warning
   context, failure propagation, and independence from other system reports.
   Completed: `engine_update()` advances ticks and merges its sole lifecycle
   child report once; independent tick and report-returning lifecycle updates
   remain available. Added a non-consuming latest tick-delta getter and matching
   public wrappers. Preserved expiration ordering and recoverable individual
   cleanup failures, with owned diagnostics and named counters. Removed old
   system tick/cleanup APIs without aliases; migrated examples and strictly
   recognized generated entry points while preserving developer edits. Audited
   remaining entry-point ownership; input stays deferred. Verified 78 native
   tests, eight focused ASan/UBSan/LeakSanitizer tests, and Linux/Windows builds
   without compiler warnings. Installed-SDK consumers, nine affected examples,
   and a generated application build independently on both platforms; Linux
   consumers execute and applications pass headless startup/shutdown outside
   source directories. Windows execution remains unverified.
   See [Engine reports](docs/reports.md#engine-reports).
9. [x] **Verify system-wide coverage and document usage** — Audit every entry
   in the accumulated system inventory against the final implementation. Document checking
   update results and reading reports, with examples of successful updates with
   warnings and fatal updates with retained details. Verify report isolation,
   overflow, repeated warnings, public/direct API parity, relevant examples and
   generated consumers, sanitizers, and Linux/Windows builds. All applicable systems must
   have reporting coverage before selecting simulation-failure recovery; input
   reporting remains explicitly deferred.
   Completed: consolidated the public/direct producer and report-free helper
   inventory, prerequisites, ownership, timing/status boundaries, built-in metric
   rules, and test coverage. Documented checked warning/fatal consumption and
   explicit application composition; marked old inventories as historical.
   Fixed audio parent timing to include application-gate acquisition and failed
   lock calls while preserving retained shutdown timing. Added deterministic
   timing, complete output-vocabulary, cross-system pending-evidence/copy isolation,
   and installed-SDK usage coverage. Verified 79 native tests, nine focused
   ASan/UBSan/LeakSanitizer tests, documentation links/API inventory, and
   Linux/Windows engine/editor builds without compiler warnings. Both installed-SDK
   consumers, all 12 standalone examples, and a generated application compile
   on both platforms; Linux execution/startup/shutdown checks pass outside source
   directories. Windows execution remains unverified. No public API or recovery
   policy changes; input stays deferred. See the
   [coverage inventory](docs/reports.md#system-coverage-inventory).
10. [ ] **Add latest stored report retrieval and automatic composition** — Each
    runnable pipeline keeps its latest returned report and overwrites it on its
    next update; no history. `rohr_report_get(id)` returns an independent copy,
    including child pipeline reports. `rohr_report_get(ROHR_REPORT)` combines the
    latest available results without counting child work twice. Never-run
    pipelines return NOT_RUN. Updates continue returning reports directly.
    Define invocation provenance for children updated independently after their
    parent, cross-system freshness, reset/lifetime, and callback synchronization
    before implementation. Latest reports may originate at different times.
11. [ ] **Define and implement simulation-failure recovery** — After all applicable systems
    have reporting, decide how a failed entity is excluded without destructively
    rewriting its normal authored/physical state. Compare a reserved failure
    flag with per-update retries, define developer repair/reset and automatic
    revalidation, and specify repeated diagnostics. Clarify state preservation,
    partial integration, contacts, joints, and soft-body dependencies. Implement
    only the approved policy; verify unaffected entities continue safely, failed
    entities can recover, and failures remain observable. Setter/creator
    validation remains immediate. No flag, retry, or rollback policy is approved
    yet; settle those choices when this goal is selected. Resume the force
    contract only on subsequent explicit user instruction.

### Agreed reporting behavior

- A physics update returns a self-contained report directly, including whether
  it completed. The developer retains that returned value if needed; normal
  report access does not require a separate latest-report getter. Recoverable
  warnings do not turn a completed update into a fatal failure. Fatal failures
  stop processing where continuing is unsafe and retain the collected report.
- Each system owns its collection and update boundary. Updating a system does
  not mutate previously returned report values. Reports contain no pointers to
  mutable engine storage. Define other systems' specific return/callback
  boundaries in their respective goals.
- Directly runnable pipelines return independent reports. Internal stages and
  helpers contribute to their pipeline's collector without generating reports
  themselves. System updates combine the reports of pipelines they execute;
  custom callers can inspect or combine the same reports. Preserve prerequisites,
  stage/source context, repeat counts, overflow, and fatal details when merging;
  merging executes no simulation and must not double-count nested work.
- Aggregate equivalent repeated warnings within an update with an occurrence
  count. Physics retains up to 256 distinct diagnostics, counts dropped
  occurrences, and reserves fatal details even when full. Timings/statistics
  remain system-specific.
- Invalid setter arguments continue to return immediate errors and preserve
  previous state. Intentionally incomplete force sources are suspended, not
  automatically warning-generating failures. Unexpected runtime invalidity is
  reported by the owning update when the force milestone implements it.
- Force/torque failures discovered after an apply call returns belong in the
  physics report, including source/target identifiers even if a one-tick source
  later expires. Do not add a separate force-error queue.

Each implementation goal includes focused failure/recovery tests and applicable
sanitizer, example, and Linux/Windows verification. Register each selected goal
as a Codex goal, complete only that goal, then stop for user review. Implementation
details affecting subsystem behavior require their routed instructions first.

## Milestone: Unified Forces and Torques

Support one shared force API and one shared torque API across rigid bodies,
standalone particles, soft-body nodes, beams, and whole soft bodies. Finish
with editor authoring and manipulation. This replaces the proposed Editor
Forces and Torques milestone and precedes numbered priority 4.

Goal 1 resumes after the System Update Reports prerequisite. Its contract
incorporates the agreed API lifecycle below and remains open for the remaining
decisions;
no force/torque API or physics changes have been implemented in this milestone.

1. [ ] **Finalize the shared API contract (in discussion)** — Specify direct
   ECS creation/configuration and typed creation from standalone descriptions,
   with shared validation, entity setters, and whole-description get/set.
   Define signatures, source data, independent application-point/direction
   references, defaults, and suspension behavior. Finish beam distribution,
   tick timing, invalid-input rules, and migration of soft-body-specific APIs.
   Record usage examples and acceptance criteria before implementation.
2. [ ] **Implement shared sources and rigid-body/particle behavior** — Add
   force-at-point support, consistent one-tick and persistent source handling,
   direct and description-based creation, mutation, and deletion. Verify
   shared validation, source suspension, translation and rotation, particle position
   ignoring, particle torque no-ops, and rigid bodies containing particles.
3. [ ] **Implement local beam loads** — Distribute force and torque to endpoint
   nodes without child source entities. Verify net force and torque, local
   deformation, collapsed beams, endpoint changes, and deletion.
4. [ ] **Implement whole-soft-body loads** — Distribute translation and turning
   contributions across nodes. Verify COM acceleration, off-centre forces,
   pure torque, changing masses/geometry, and natural beam stretching. Complete
   migration to the shared API.
5. [ ] **Add authored definitions and persistence** — Add object-owned source
   descriptions, target references, CLI operations, project/runtime
   serialization, validation, history, copying, duplication, and deletion.
   Preserve initial versus persistent behavior and zero-magnitude sources.
6. [ ] **Generate runtime sources and verify API parity** — Generate initial
   applications, persistent source references, and instance cleanup. Verify
   independent instances, runtime mutation, and direct/generated parity. Add
   a focused demonstration and API documentation.
7. [ ] **Implement the force editor** — Add top-level object rows, dedicated
   properties, target selection, application-dot/arrow rendering, and
   position/direction/magnitude controls. Integrate selection, visibility,
   ordering, clipboard, multi-edit, and history. Verify saved projects in app.
8. [ ] **Implement the torque editor and complete visual verification** — Add
   dedicated properties and movable 270-degree arrow markers. Reuse source
   interactions; show particle torque as having no physical effect. Verify
   both editors across all targets and request final manual review.

### Agreed behavior

- The target entity determines how the common API applies or distributes a
  load. One-tick and persistent sources use the same target behavior.
- Each persistent source has one ID for updating and deletion. Distributed
  loads do not create child force/torque entities. Deleting a source stops
  future contributions without undoing motion already produced.
- Rigid-body forces act at an application point and produce the corresponding
  torque about COM. Rigid bodies containing particles retain angular response.
- Pure standalone particles and individual soft nodes ignore force position;
  their forces act through their centroid. Torque succeeds but has no effect,
  including persistent sources that can still be updated and deleted.
- Beam loads act locally through endpoint nodes and propagate through the
  existing structure. Whole-soft-body loads distribute across all nodes,
  preserving requested net force and torque while permitting deformation.
- A whole-soft-body force through COM adds uniform linear acceleration;
  off-centre force adds turning about COM. Pure torque adds no net linear force.
  These loads do not impose rigid transforms or overwrite node velocities.
- Recalculate distributed contributions each physics substep using current
  geometry and masses. Angular motion can naturally stretch beams.
- Torque marker placement is visual only; it does not establish a physical
  pivot. Physical torque remains clockwise-positive and uses physical units.
- Forces and torques are top-level object-editor items with dedicated editors
  and established interaction rules. Forces use an arrow pointing away from
  their application dot; torques use a three-quarter-circle tangential arrow.
- The editor authors persistent sources and initial one-tick applications.
  Sources can begin at zero magnitude and change later in developer-owned C.
  No delayed event system is introduced. Immediate impulses remain separate.
- Angles remain clockwise degrees by default, with zero heading up.

### Goal 1 agreed API lifecycle

These are design requirements, not currently implemented signatures.

- `rohr_physics_force_create()` and `rohr_physics_torque_create()` create ECS
  source entities immediately, returning `EntityResult`. They begin incomplete
  and suspended; entity setters configure them initially and modify them later.
  There is no separate add step for this direct workflow.
- Developers can instead prepare standalone `ForceDescription` or
  `TorqueDescription` values and call
  `rohr_physics_force_from_description_add(description)` or
  `rohr_physics_torque_from_description_add(description)`. These typed wrappers
  require a complete, valid configuration and return `EntityResult`. Failure
  leaves no source entity. A generic entity-description factory is out of scope.
- Description-based creation copies configuration into the source; later edits
  to the original value do not mutate it. Referenced entities remain references,
  not cloned targets. Descriptions can be reused for multiple sources.
- Both creation paths produce the same source representation and share
  configuration/validation logic. Either supports the same entity setters and
  whole-description get/set after creation. Do not duplicate every entity setter
  as a separate description-building API; callers may fill description values
  directly, as with existing configurations.
- Setters and whole-description replacement may change the target entity or
  leave an existing persistent source incomplete. Such sources suspend until
  valid again. Malformed values, including nonfinite numbers, return an error
  and preserve the previous state. Whole-description replacement is atomic.
- Zero magnitude is allowed and does not require a usable direction. This is
  an inactive configuration, not an error. Other required references still
  follow the creation and suspension rules.
- A deleted reference suspends an existing source until repaired; recycled
  entity slots must not silently retarget it. Source deletion uses the existing
  entity lifetime API and does not delete referenced targets.
- Incomplete one-tick requests fail immediately. A persistent source whose
  current geometry cannot represent its turning contribution suspends entirely,
  rather than applying only its linear contribution, until valid again.
  Pure-particle/node torque remains the successful no-op described above.
- Agreed creation defaults are no target, application at the target's COM,
  unset direction, and zero magnitude. A valid target makes the default source
  inactive; a nonzero force also requires a usable direction.
- One-tick attachments resolve each substep. If references or geometry become
  invalid, skip that substep without extending the source lifetime. Report the
  failure through the physics update report established by the prerequisite;
  validation failures during the apply call still return immediately.

### Goal 1 agreed location and direction model

- Force configuration separates target entity, target location, direction, and
  magnitude. Target physics type is inferred from the entity, not duplicated
  in the description. Location and direction references are independent of the
  receiving target and do not imply an equal-and-opposite reaction.
- Direction supports a fixed vector, a vector between two locations, a
  rigid-body orientation reference, or a beam direction. Resolve and normalize
  the direction before multiplying by magnitude. Use existing math APIs for
  ordinary vector construction rather than force-specific math helpers.
- Locations support world points, entity-origin offsets, COM-relative points
  with world-axis offsets, and explicit beam locations. Rigid-body local
  offsets follow origin and rotation. Soft-body stored origin/orientation are
  authoring transforms, not an estimated physical pose; do not infer physical
  soft-body orientation for direction following.
- Beam locations use start/center/end references, an along offset expressed
  as distance or fraction of current full beam length, and a perpendicular
  distance offset. Start/end follow stored node order, not screen left/right.
  Positive along offsets point start-to-end; positive perpendicular offsets
  point to its left. Center plus a fraction of 0.5 reaches the end.
- Application points beyond the beam segment are permitted. Endpoint weighting,
  off-center torque distribution, fixed/held endpoint treatment, and collapsed
  beam force-only behavior still require final contract agreement.

### Goal 1 remaining decisions

The following details still need final contract definitions; they are not
approval to implement a particular default or policy:

1. **Public signatures and defaults:** finalize the complete type/enum/helper
   inventory, initial field values, direction angle-offset representation, and
   detailed source-status queries, including the distinction between malformed
   descriptions and incomplete or temporarily unresolvable references.
2. **Beam load distribution:** specify endpoint weighting, off-center torque,
   fixed/held endpoint treatment, and force-only behavior at coincident endpoints.
   External application points are already permitted; do not clamp them silently.
3. **One-tick timing:** specify first/last participating substeps and map failure
   details to the prerequisite report API. Per-substep reference resolution,
   skipping invalid contributions, reporting failures, and unchanged expiry
   are already agreed.
4. **Migration and acceptance:** finalize replacement of the current vector/
   scalar APIs and soft-body-specific helpers, breaking-format requirements,
   usage examples, and test scenarios for both creation paths.

Editor handle presentation details can be finalized before their respective
editor goals. Goal 1 stays incomplete until the remaining contract is settled.

### Existing support and verification

Existing force/torque create/set/get/apply APIs store vector/scalar values and
an entity target. They have no general public force application point. Current
whole-soft-body helper APIs distribute one-tick loads via node sources; the
new contract replaces that implementation with one source and substep
accumulation. A private joint helper calculates force-at-point response, but
it creates separate force and torque source entities and is not the proposed
shared implementation.

Each implementation goal includes relevant regression tests, sanitizer checks,
and Linux/Windows builds. Generated-code changes require application checks;
editor goals require visual review. Register and pursue only one Codex goal at
a time; the user selects each subsequent goal explicitly.

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
**Unified Forces and Torques** milestone above is authoritative for the next
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
