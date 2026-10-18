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
   definitions generate public `ParticleDescription` constants and instantiate
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

### Scene Simulation Tabs and Explicit Starter Code

Run an isolated scene executable inside an editor viewport tab, and make the
new-project application main demonstrate explicit dot-catalog creation. The
user selected this milestone before the remaining roadmap priorities. See
[project entry points and ownership](docs/scene_projects.md).

1. [x] **Project scaffolding and starter tutorial** — Added preserved `scene.c`
   and an independent scene executable, explicit starter dot-catalog creation
   with handle-based joint wiring, whole-object `tug` spawning, generated instance
   binding/settings helpers, and exact scaffold migration. Whole-scene creation
   uses the same catalog path; visuals draw once per instance. Customized entry
   points/build files and existing authored scenes remain intact. Verified all
   93 native tests, four focused ASan/UBSan/leak tests and the installed-SDK
   generated-runtime sanitizer probe, Linux/Windows engine and dual-target
   project builds, and independent center_of_mass/joints example builds on both
   platforms without compiler warnings. Linux targets/examples launch from
   outside their source directories; Windows runtime execution is unverified.
   Regression checks cover source restoration/preservation, bindings, independent
   instances, force-driven spring motion, viewport setup, and failure cleanup.
2. [ ] **Isolated simulation hosting** — Run the scene in a separate process;
   provide scene lifecycle callbacks, bounded frame/input transport, pause and
   resume, audio/timing handling, diagnostics, and process cleanup.
3. [ ] **Build and Run menus** — Build Application, Build Scene, Build Project,
   Generate App C, and Compile App; Run Application and Simulate Scene use the
   latest successful build without implicitly compiling. Scene/project builds
   stop simulation first.
4. [ ] **Viewport tabs and simulation controls** — Editor and Scene Simulation
   tabs; open simulation stopped; Run/Pause and Stop controls; Editor selection
   stops simulation; restart from the scene executable; verify focused input,
   composition, lifecycle, and platform behavior.

Complete and verify one goal at a time, then stop for review and a user-created
commit. Goal 1 does not implement simulation hosting or editor menus/tabs.

### Soft-Body Force Following and Generated-App Regression

Restore soft-body force references during simulation, then investigate the
reported animation creation/deletion and black-screen regression separately.

1. [x] **Implement and verify soft-body origin tracking** — The equal-weight
   average of directly owned nodes and incremental best-fit rotation now drive
   the usable origin while preserving its offsets. Explicit transforms,
   ownership changes, degenerate geometry, persistence, and force references
   retain their contracts. Verified 92 native tests, six ASan/UBSan/leak checks,
   Linux/Windows builds, and installed-SDK generated-app and soft-body example
   builds on both platforms. The supplied project's `force_2` follows translation
   and rotation, renders, and stays independent across two spawned instances
   through 120 simulation steps. Linux applications launch headlessly from
   outside their source directories; Windows runtime execution remains unverified.
2. [x] **Diagnose and separately plan the animation creation/deletion and
   black-screen correction** — Investigated the corrected project path,
   `/tmp/rohr-projects/test-project`, using isolated copies. An empty animation
   reproduces error 21 (`invalid entity`): ordinary generation skips its creation
   but still emits its viewport layer assignment. Catalog spawning separately
   rejects the same empty animation with error 43 (`invalid game state`). An
   animation containing the project's 11 imported image frames starts through
   both paths. The saved viewport has no camera screen items, so the scene
   callback never runs; adding a screen in a diagnostic control restores it.
   Regeneration retains the missing screen. Animation deletion preserves both
   empty and populated viewport layouts; no deletion-induced screen loss was
   reproduced. The original project was left unchanged. These are Linux
   installed-SDK diagnostic checks; no production C changes, sanitizer run, or
   Windows verification were needed for this investigation.

Both investigation milestone goals are complete. The user confirmed the
black-screen issue is resolved and excluded camera/viewport repair from further
work.

### Valid Empty Animations

Allow empty animations to remain valid authored and runtime objects, with their
skipped playback/drawing state captured in entity info and system reports.

1. [x] **Support empty animations across creation, generation, inspection,
   reporting, and persistence** — Empty animated-sprite components now retain
   their identity, settings, and ownership without loading frame assets.
   Playback/drawing are skipped; entity info exposes `ANIMATION_EMPTY` and
   animation reports record INFORMATION diagnostics with skipped counts while
   remaining COMPLETE. Ordinary generation, catalog spawning, attached/free
   animations, frame removal, save/load, independent instances, and populated
   siblings are covered. Verified 93 native tests, eight ASan/UBSan/leak checks,
   Linux/Windows builds without compiler warnings, and installed-SDK generated
   applications and the joints example on both platforms. Linux probes verify
   both constructor paths after CLI last-frame removal, viewport setup, entity
   info, reports, and independent instances; applications launch from `/tmp`.
   Refreshed and launched `/tmp/rohr-projects/test-project`, preserving authored
   state and `main.c`. Windows runtime execution remains unverified. No camera
   or viewport configuration repair was performed.

Stop after this goal for review and a user-created commit.

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

**System Update Reports** Goals 1–6 and 8–11 (through stored retrieval and
simulation-failure recovery) are complete.
Goal 7 (input) is deferred by user choice; retain its current API and direct
errors. Goal 11 implements operation-level rejection and retry. The single-goal
**Report Bitmask Selection** milestone is complete. The single-goal **Force/Torque
Contract and Roadmap Reconciliation** documentation milestone is also complete;
the approved design is recorded in [the contract](docs/force_torque_contract.md).
Entity Inspection and Unified Forces and Torques Goals 1–4 are now complete.
The documentation-only **Goal 5 Contract Clarification** and the single-goal
**Runtime Soft-Body Persistence** and **Object Descriptions and Independent
Spawning** single-goal milestones are complete and the object implementation
has been committed. The user selected the milestone below ahead of Unified
Forces and Torques Goal 5, which remains unstarted.

### Unified Entity Ownership and Physics Creation Descriptions

Make physics ownership explicit and creation APIs consistent while preserving
independent connections. Ownership transfers preserve all connections, and
owned beams/areas may reference nodes from other owners. Replace old APIs and
migrate callers without compatibility wrappers. Each goal requires explicit
instruction; stop for review and a user-created commit after each goal.

1. [x] **Unify ownership and preserve connected topology** — Added generic
   entity owner set/get and authoritative lifetime ownership with derived
   soft-body membership. Transfers preserve topology; beams/areas support
   cross-owner nodes. Spawning and version 8 persistence use the unified model.
   Verified rollback, deletion, capacity, scoped identities, and round trips:
   86 native tests, four ASan/UBSan checks, Linux/Windows engine/editor builds,
   installed-SDK consumers and standalone examples on both platforms, plus
   Linux runtime/startup checks outside source directories. Windows execution
   remains unverified. Committed as `ffbd4ff`.
2. [x] **Migrate soft-body creation to descriptions** — Replaced soft-body,
   node, local-node, beam, and area constructors with typed description-based
   add APIs, without compatibility wrappers. Migrated callers and C generation,
   regenerated bundled examples and API documentation, and preserved defaults,
   independent ownership, and world/local placement. Verified 86 native tests,
   four ASan/UBSan tests, Linux/Windows builds, installed-SDK consumers and
   standalone examples, plus a regenerated 90-degree/offset placement probe
   comparing direct, generated, and catalog-spawned nodes on Linux. Windows
   execution remains unverified. Committed as `a183c00`.
3. [x] **Complete physics creation consistency** — Replaced particle, joint,
   anchor, and attachment-helper constructors with value descriptions and add
   APIs. Added force/torque description ownership backed by generic entity
   ownership, with atomic replacement, current-owner getters and inspection,
   ownerless one-shot apply, and preserved version 8 persistence. Migrated
   callers, generated code, examples, and documentation without compatibility
   wrappers. Verified 86 native tests, eight ASan/UBSan tests, allocation and
   anchor-capacity rollback, Linux/Windows builds, installed-SDK consumers and
   standalone examples, plus direct/generated/catalog particle and soft-body
   parity on Linux. Windows execution remains unverified. The implementation
   milestone is complete; await user review and a user-created commit.

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
Node-Loop Soft-Body Areas are complete. System Update Reports, Report Bitmask
Selection, Force/Torque Contract and Roadmap Reconciliation, and Entity Inspection
precede Unified Forces and Torques implementation.

The user-selected **Stable Area Colors During Motion** milestone is complete.
It fixes runtime color flashing caused by numerical region detection failures.

The user-selected **Beam-Bounded Soft-Body Area Colors** milestone is complete.
It supersedes the material-position fill behavior of **Stable Soft-Body Areas**.

The user-selected **Bugs Identified by Rohr User** milestone is complete.
The **System Update Reports** prerequisite, **Report Bitmask Selection**,
**Force/Torque Contract and Roadmap Reconciliation**, **Entity Inspection**, and
then **Unified Forces and Torques** implementation precede numbered priority 4.
Pursue only an explicitly selected goal.

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
10. [x] **Add latest stored report retrieval and automatic composition** — Each
    runnable pipeline keeps its latest returned report and overwrites it on its
    next update; no history. `rohr_report_get(id)` returns an independent copy,
    including child pipeline reports. `rohr_report_get(ROHR_REPORT)` combines the
    latest available results without counting child work twice. Never-run
    pipelines return NOT_RUN. Updates continue returning reports directly.
    Completed: errorable public/direct getters return independent copies, reject
    unsupported/custom/input IDs, and preserve snapshots across shutdown/restart
    until replaced, including by NOT_RUN. Process-wide publication versions and
    bounded provenance omit covered child snapshots while preserving independent
    versions as separate groups. Children expose the latest invocation; audio
    children expose the last published callback interval, without callback-side
    cache access. Retained audio shutdown delivery preserves its version.
    Concurrent getters observe complete published values and one consistent
    combined registry view. Added version/group validation and text/JSON version 2
    output; rebuild consumers for the changed Report ABI. Fixed storage is about
    2.2 MB on the native ABI; 100 combined reads measured about 1.5 ms in the
    stress fixture and 54 ms with every supported slot full; 100 full-report
    publications took about 0.15 ms. Verified 80 native tests, ten focused ASan/UBSan/LeakSanitizer
    tests, allocation guards, concurrent readers/publication, and Linux/Windows
    engine/editor builds without compiler warnings. Both installed-SDK consumers,
    all 12 standalone examples, and a generated application build on both
    platforms; Linux execution/startup/shutdown checks pass outside source
    directories. Windows execution remains unverified. See
    [Stored reports](docs/reports.md#stored-reports).
11. [x] **Define and implement simulation-failure recovery** — Approved policy:
    validate candidate values and commit the smallest coupled operation. Reject
    invalid operations without HOLD, component removal, destructive state resets,
    or persistent quarantine. Retain successful earlier work and use retained
    valid state for later operations. Retry at the next scheduled execution,
    including solver iterations/substeps; aggregate warning evidence. Preserve
    unavailable joint/lock definitions for developer repair, while retaining
    explicit ownership cleanup. Keep failures local to affected soft-body parts.
    Add immediate numerical setter/creator validation. Shared resource/argument
    failures remain fatal. Verify native tests, sanitizers, Linux/Windows builds,
    installed SDKs and affected examples before completion. Input stays deferred;
    resume the force contract only on subsequent explicit user instruction.
    Completed: candidate validation and coupled commits cover integration, loads,
    contact responses, joint corrections, locks, and soft-body operations. Missing
    runtime references remain available for repair; invalid geometry is excluded
    without removing authored hitboxes. Numerical API validation preserves state.
    Operation-specific warning stages support existing text/JSON output and stored
    report retrieval. Added regression coverage for repeated attempts, paired
    rejection, local soft-body continuation, repair, preserved contact evidence,
    and immediate setter rejection; replaced old quarantine assertions.
    Verified 81 native tests and 18 focused ASan/UBSan/LeakSanitizer tests;
    Linux/Windows engine/editor/test builds have no compiler warnings. Both
    installed-SDK consumers, all 12 standalone examples, and a generated application
    build on both platforms; Linux consumers execute and application startup/shutdown
    checks pass outside source directories. Windows runtime remains unverified.
    See [Physics recovery](docs/reports.md#physics-operation-rejection-and-retry).

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
  previous state. Future zero-initialized no-target force/torque sources are
  valid and produce no warning solely for inactivity. Explicit references must
  resolve even for zero/no-target sources; failures are reported by the owning
  update when the force milestone implements the
  [force/torque contract](docs/force_torque_contract.md).
- Force/torque failures discovered after an apply call returns belong in the
  physics report, including source/target identifiers even if a one-tick source
  later expires. Do not add a separate force-error queue.

Each implementation goal includes focused failure/recovery tests and applicable
sanitizer, example, and Linux/Windows verification. Register each selected goal
as a Codex goal, complete only that goal, then stop for user review. Implementation
details affecting subsystem behavior require their routed instructions first.

## Milestone: Report Bitmask Selection

Support selecting and combining latest stored reports with bitwise `|`,
preserving existing report ownership and composition behavior. This single-goal
milestone is complete. The documentation reconciliation and Entity Inspection
milestones precede force/torque implementation, each on explicit user instruction.
System Update Reports goals remain complete except deferred input.

1. [x] **Implement and verify report bitmask selection** — Add combinable
   selectors and an all-reports selector, distinct from report identity IDs.
   Preserve independent copies, consistent snapshots, and version-aware
   parent/child deduplication. Unselected parents must not suppress selected
   children. Reject unsupported selection bits. Migrate public/direct APIs,
   callers, and documentation. Verify single, combined, all, and invalid
   selections, matching and differing parent/child versions, public/direct
   parity, sanitizers, Linux/Windows builds, affected examples, and
   installed-SDK consumers.
   Completed: public/direct getters take `ReportFlags`; existing selector names
   combine with `|`, while report identities use `_ID` names with unchanged
   numeric values. Single selections preserve snapshots; combinations deduplicate
   only versions covered by selected parents. Zero, unsupported bits, and input
   selection return errors. Migrated producers, formatters, callers, and docs;
   text/JSON identity labels use the new names without changing the schema.
   Verified 81 native tests, nine focused ASan/UBSan/LeakSanitizer tests, and
   Linux/Windows builds without compiler warnings. Both installed-SDK consumers,
   all 12 standalone examples, and a generated application build on both
   platforms. Linux consumers execute and application startup/shutdown checks
   pass outside source directories; Windows runtime remains unverified.
   Allocation guards and concurrent publication checks pass. The report/cache
   sizes are unchanged; 100 combined reads took about 1.48 ms in the test fixture.

## Milestone: Force/Torque Contract and Roadmap Reconciliation

Consolidate the agreed force/torque design and schedule entity inspection before
implementation. This single-goal documentation milestone is complete; no runtime
API, physics behavior, or build configuration changed.

1. [x] **Finalize the force/torque contract and update implementation dependencies**
   — Record approved types, by-value APIs, zero/no-target defaults, validation,
   lifetime, beam calculations, ownership, migration, and acceptance tests in
   [the authoritative contract](docs/force_torque_contract.md). Replace superseded
   roadmap statements and reconcile reporting guidance. Schedule entity-info
   definition, then its implementation, before force/torque implementation.
   Verify examples, formulas, links, consistency, and `git diff --check` without
   implementing C APIs or advancing into entity-info design.
   Completed: consolidated the contract and removed superseded roadmap/reporting
   assumptions. Verified relative links and anchors, C11 syntax of all examples
   with warnings as errors, 300 beam moment/torque cases and 1,200 finite-difference
   endpoint-work checks, and `git diff --check`. Runtime motion/energy tests,
   sanitizers, and platform builds remain implementation acceptance requirements.
   Entity Inspection follows as its own milestone.

## Milestone: Entity Inspection

Provide one errorable current-state entity-info API with combinable section
selectors and `ROHR_ENTITY_INFO` for all sections. Preserve separate component
information in the combined result. Inspection must not execute operations or
mutate reports; reports remain evidence of completed update execution. Complete
this milestone before Unified Forces and Torques implementation, which adds its
new source details to the shared inspection API. Select each goal explicitly.

The [entity inspection contract](docs/entity_inspection_contract.md) records the
implemented API and ownership rules, including flat arrays and separate counts.

1. [x] **Define the entity inspection contract** — Completed: public/direct getter
   signatures, result structure, section inventory, selector flags, per-component
   configuration/eligibility details, ownership, validation, missing-component
   behavior, and acceptance criteria. Preserve `|` composition and all-section
   retrieval. Define how later force/torque integration reports valid zero/no-target
   states and unresolvable references without dedicated force-status getters.
   Flat immutable array pointers use separate counts, with get/retain/release
   snapshot ownership and copied, filterable issues. No overall entity health
   value hides component differences. Verified type/example consistency and
   documentation links; runtime verification belongs to Goal 2.
2. [x] **Implement and verify entity inspection** — Implemented the approved contract
   for existing components with shared filtering, no operation execution or report
   mutation, public/direct parity, documentation, and relevant tests, sanitizers,
   Linux/Windows builds, and installed-SDK checks. Leave new force/torque component
   integration to its subsequent implementation goals.
   Completed: all 61 selectors, allocated immutable snapshots with explicit
   get/retain/release, copied and filterable issues, count-only reads, read-only
   graphics layer lookup, and removal of the colliding `children` pool macro.
   Verified 82 native tests; inspection/report/graphics/physics ASan, UBSan, and
   leak checks (4 tests); Linux and Windows engine/editor/example builds;
   3 installed-SDK consumers, all 12 standalone examples, and a generated project
   compiled on both platforms. Linux consumers ran, and all examples plus the
   generated project passed bounded headless launches outside source directories.
   Windows runtime execution remains unverified. Contract/type consistency,
   strict C99 warning checks, and `git diff --check` passed. Measured snapshots
   and dynamic collections through 1,024 hitboxes; see the contract for costs.
   Unified Forces and Torques Goal 2 remains pending explicit user instruction.

## Completed Milestone: Diagnostic Reliability

Find false success, missing evidence, incorrect warnings, and lifetime defects in
inspection and reporting before returning to force/torque implementation. Add
reproducible adversarial tests and fix confirmed defects within existing contracts.

1. [x] **Stress entity inspection** — Exercise missing dependencies, stale
   references, component removal, invalid numeric values, empty versus unavailable
   fields, retained snapshots across restart, seeded ownership sequences, and
   allocation failures. Verify diagnostics and read-only behavior under sanitizers
   and relevant platform/SDK builds. Added six adversarial case groups, including
   8,000 seeded ownership operations across 40 restarts and 128 selector mixes.
   Fixed suppressed scale/beam diagnostics, unsafe hitbox metadata reads, and
   removed hitboxes appearing available. Verified 83 native tests, both inspection
   tests under ASan/UBSan with leak detection, Linux/Windows builds and installed
   SDK consumers, standalone example builds, and bounded Linux example launches.
   Windows validation is compile-only.
2. [x] **Stress report aggregation and storage** — Exercise overlapping selectors
   and versions, full capacities, saturation, first-fatal preservation, reset
   boundaries, publication ordering, and output failures. Added independent
   expectations for 512 seeded full-buffer operations and 96 nested publications
   across all seven selector combinations, 20 malformed-report cases, correlated
   concurrent writers/readers, complete JSON evidence comparisons, and allocation,
   partial-write, flush, close, and retry coverage. No production defect was
   confirmed; existing report semantics and APIs are unchanged. Verified all 83
   native tests, nine report suites under ASan/UBSan with leak detection,
   Linux/Windows builds without compiler warnings, installed-SDK consumers, and
   independent audio/player-controller example builds. Linux consumers run and
   both examples pass bounded headless launches outside their source directories.
   Windows validation is compile-only; operation-level injection is covered by Goal 3.
3. [x] **Verify diagnostics against actual engine failures** — Inject operation
   failures and compare engine state, inspection, and reports; verify unaffected
   work, recovery evidence, and immutable older snapshots. Correlated physics
   rejection and fatal-abort state, lifecycle deletion failures, graphics draw
   and animation failures, deferred audio output, and UI input overflow with
   their reports and inspection snapshots where applicable. Fixed missing
   inspection warnings for mismatched sprite/player frame counts after a failing
   regression; captured configuration, runtime behavior, and recovery policies
   are preserved. Verified all 83 native tests, 12 reporting/recovery/inspection
   suites under ASan/UBSan with leak detection, and Linux/Windows builds without
   compiler warnings. Four installed-SDK consumers build on both platforms and
   run on Linux. Independent audio/player-controller examples build on both
   platforms and pass bounded Linux headless launches outside their source
   directories. Windows validation is compile-only.

All three goals are complete. Unified Forces and Torques progress is recorded
below.

## Single-Goal Milestone: Diagnose Soft-Body Collision Penetration

Identify why soft-body nodes and beams penetrate triangular `rigid_body_8` in
`/tmp/rohr-projects/test`, then produce an evidence-backed resolution plan.
This user-selected investigation precedes Unified Forces and Torques Goal 4;
the separately approved production correction is recorded immediately below.

1. [x] **Reproduce the penetration, identify its cause, and define the fix** —
   Confirmed redundant collinear vertices split the triangle's support faces:
   SAT detects overlap but clipping produces no manifold, skipping node response
   and beam contact gathering. Minimal node and beam fixtures fail against the
   five-vertex triangle and respond against the identical three-vertex outline.
   Ten controlled Linux scene runs isolate the failure from timestep/iteration
   settings; original/current engine-library traces and regenerated sources
   match. Recorded measurements, preserved-source fingerprints, and a focused
   contact-face correction/verification plan in the
   [collision diagnosis](docs/soft_body_collision_diagnosis.md). Documentation
   links and diff checks pass. Production physics and the supplied project
   remained unchanged during diagnosis; the correction is recorded below.

## Single-Goal Milestone: Restore Contacts Across Subdivided Straight Faces

Restore node and beam contacts across contiguous collinear polygon edges while
preserving authored geometry and existing response policy. This approved
collision correction precedes Unified Forces and Torques Goal 4.

1. [x] **Implement complete contact faces and verify the original reproduction** —
   Added failing geometric and runtime regressions, then expanded reference and
   incident faces through adjacent co-directed collinear edges with bounded,
   allocation-free traversal and double-precision tolerance checks. Verified
   ordering, winding, wraparound, rotation, translation, maximum vertex count,
   near-collinear limits, corners, filters, and isolated node/beam response.
   The unchanged generated scene passes 900 fixed ticks at one substep/eight
   iterations with no node-centre penetration, missing supported contacts,
   nonfinite motion, or diagnostics. Node-only and beam-only controls also pass.
   All 84 native tests and ten focused ASan/UBSan suites pass with leak detection.
   Linux/Windows builds, five installed-SDK consumers, all 12 standalone examples,
   a generated fixture, and an isolated copy of the supplied game build without
   compiler warnings. Linux runtime checks pass; Windows is compile-only.
   Recorded evidence and the visual-review executable in the
   [collision diagnosis](docs/soft_body_collision_diagnosis.md). Original project
   sources and binaries remain unchanged. Stop for user review; Goal 4 is pending.

## Completed Single-Goal Milestone: Goal 5 Contract Clarification

Record the settled authored-source decisions and the runtime persistence
prerequisite before implementation.

1. [x] **Update the Goal 5 contract and roadmap** — Document object-local authored
   references, no-target/world-default deletion fallbacks, and the planned
   `ENTITY_NONE` alias. Distinguish authored deletion from runtime stale-handle
   behavior. Define the single-goal runtime soft-body persistence prerequisite
   and retain the remaining force/torque goal order. Documentation consistency,
   local links, and diff checks pass; no code or runtime behavior changes.

## Single-Goal Milestone: Runtime Soft-Body Persistence

Preserve soft-body geometry, topology, physical state, and references across
runtime save/load. This user-selected prerequisite precedes Unified Forces and
Torques Goal 5; editor project topology persistence is already present.

1. [x] **Implement and verify complete runtime soft-body save/load** — Version 6
   preserves ordered ownership, deformed node state and constraints, independent
   beam rest lengths/materials/collision settings, area loops/holes/drawing
   overrides, shared and unused anchors, joints, and force/torque references.
   Creation assigns unique editable names; incoming names are preflighted across
   documents and the live world without renaming. Relative enabled lifetimes
   survive restart, including already-due expiry. Private phased restoration
   and allocation-failure rollback preserve existing entities; atomic saves
   preserve the destination on failure and retain authored templates separately.
   All 85 native tests and five focused ASan/UBSan suites pass with leak
   detection. Linux and Windows engine builds, installed-SDK consumers (including
   a public persistence consumer), all 12 independent standalone examples, and a
   generated application pass their applicable checks. Linux programs run
   outside their source directories; Windows checks are compile-only. No
   compiler warnings; the Nix dirty-tree notice is expected. See the
   [version 6 contract](docs/game_state.md#version-6-runtime-topology-contract).

Stop after this one goal for review and a user-created commit. Resume Unified
Forces and Torques Goal 5 only on explicit user instruction. The `ENTITY_NONE`
alias belongs to Goal 5; it is not implemented by this prerequisite.

## Single-Goal Milestone: Object Descriptions and Independent Spawning

Expose editor objects as reusable descriptions supporting direct access,
runtime loading, editable copies, independent spawning, and complete instance
restoration. This user-selected milestone precedes Unified Forces and Torques
Goal 5. Its single goal is complete; stop for review and a user-created commit.

1. [x] **Implement and verify object descriptions, selective spawning, and
   instance persistence.**

   Implemented immutable generated dot-access catalogs, cached runtime document
   loading, editable filtered copies, independent spawning, protected instance
   ownership, typed cameras/anchors, retained drawing, and detached topology.
   Runtime state version 7 preserves instance membership, aliases, resources,
   and playback without retaining snapshots as authored templates. Reused entity
   slots now release stale physical component data. See the
   [object interface](docs/object_descriptions.md) and
   [persistence contract](docs/game_state.md).

   Verified all 86 native tests and five focused ASan/UBSan tests with leak
   detection. Linux and Windows engine/editor builds, installed-SDK consumers,
   all 12 standalone examples, and a generated default project compile without
   compiler warnings. Linux consumers, examples, and the default application run
   outside source directories. The default-project probe verifies individual
   box/camera spawning, independent compiled/runtime instances, complete cleanup,
   snapshot restoration, and continued simulation. Generated identifier/property
   clashes, failed allocation rollback, malformed ownership, filtered copies,
   independent animations, anchor following, and repeated resource restoration
   have focused coverage. Windows checks are compile-only. Interactive visual
   review of instance drawing remains requested; no Goal 5 work was begun.

### Description interface

- Generate an immutable `objects` catalog with properties and children at the
  same level through dot access, without requiring developers to spell generated
  types. Diagnose generated identifier and reserved-property collisions.
- Support runtime loading of editor object files through a common description
  handle. Runtime-loaded descriptions support name lookup rather than generated
  dot fields. Recursive `object_description_by_name_get(scope, name)` rejects
  missing and ambiguous matches.
- Provide explicitly owned editable copies through `object_copy`, common-handle
  setters, and destruction. Copying a parent includes descendants. Do not add
  creation of descriptions from scratch. Mutable copies are independent;
  immutable assets may be shared with explicit lifetime management. Destroying
  a description must not invalidate spawned instances.
- Resolve descriptions internally when needed; explicit `object_get` is optional.
  Cache runtime-loaded descriptions with explicit lifetime management. Generated
  descriptions require no runtime parsing of editor files.

### Spawning and filtering

- Provide `entity_object_add` and `entity_object_by_name_add`, with options
  variants sharing the same creation logic. Each spawn creates independent
  mutable state and applies complete selected initial conditions, collision
  settings, materials, and drawing settings.
- Copying and spawning accept bitwise inclusion flags
  `ROHR_ENTITY_OBJECT_PHYSICS`, `ROHR_ENTITY_OBJECT_VISUALS`, and
  `ROHR_ENTITY_OBJECT_ALL`. Default calls equal `ALL`; reject zero and unknown
  flags. Physics includes geometry, physical settings, anchors, and joints;
  visuals includes sprites, animations, and cameras. Retain names, hierarchy,
  and placement metadata. `ALL` cannot recover content excluded from a copy.
- Resolve internal connections only among members created by that spawn; never
  adopt existing entities by name. Separately spawning nodes and subsequently
  spawning their original object creates additional independent members.
- Try description-derived ECS names first, then `_1`, `_2`, and subsequent
  available suffixes. Skip occupied names without renaming existing entities;
  truncate generated prefixes as needed while preserving suffixes and uniqueness.
  Keep description member identities separate for scoped instance lookup after
  ECS renaming. Preserve existing unrelated entity-creation naming conventions.
- Top-level object positions are world positions; descendant positions are local
  to their description parent. Direct child spawning preserves resolved placement.
  Optional additional spawn offsets apply once. Editor overview placement has no
  gameplay effect. Preserve existing project placement when migrating hierarchy
  semantics.

### Instance ownership and incomplete topology

- Whole-object spawning returns a root entity with a dedicated owned-members
  component and group, proposed as `ROHR_OBJECT`. Nested spawned containers own
  their corresponding descendants. Keep this separate from ordinary groups,
  hierarchy parenting, and physical connections; these do not grant ownership.
- Deleting an instance recursively deletes its owned members and releases owned
  resources. Individual member deletion updates ownership and lookup records.
  Provide explicit release from ownership without deletion. Preserve ordinary
  parent/child deletion behavior and ordinary group semantics.
- Restore described welds and other internal connections during whole-object
  spawning. External connected entities survive instance deletion; references
  to deleted members disconnect safely. Cameras remain typed resources owned by
  instances, with typed access and standalone creation support.
- Allow disconnected joints, ownerless soft-body nodes, and incomplete beams and
  areas. Detached nodes retain applicable particle-like simulation. Skip only
  operations requiring missing topology; retain incomplete entities and expose
  warnings through reporting rather than per-tick log spam. Distinguish incomplete
  topology from temporarily undrawable fills. Reject corrupt references, invalid
  numeric data, and invalid ownership.

### Persistence and acceptance

- Extend engine save/load to preserve owned membership, scoped lookup, detached
  topology, and visual/camera resources needed for complete supported instance
  restoration. Restored instances must not require the original description to
  remain loaded. Keep editor documents separate from runtime snapshots.
- Update persistence contracts, fixtures, and format versions where necessary,
  retaining exact-version validation. Loading, copying, and spawning must roll
  back introduced entities and resources on failure. Saving must not mutate the
  world and must preserve the destination on failure.
- Preserve public entry points and generated constructors through compatible
  integration. Add no dependencies. Do not begin force/torque authoring Goal 5.
- Test direct/name-based parity, filtered recursive copies, independent repeated
  spawns, naming collisions and limits, nested placement, detached topology,
  ownership deletion/release, external connections, allocation failures, rollback,
  and repeated save/load cleanup. Verify full restored instance behavior and
  independent animation playback under matching caller-owned physics settings.
- Run the full native suite, focused ASan/UBSan suites with leak detection,
  Linux/Windows builds, installed-SDK consumers, affected standalone examples,
  and generated-project checks. Run Linux consumers/examples outside source
  directories; label Windows checks compile-only. Investigate warnings and review
  the final diff.
- Update this roadmap after verification. Report the outcome, changes, results,
  recommended commit message, and full single-goal checklist; send the required
  outcome notification. Stop for review without staging, committing, or advancing
  to Unified Forces and Torques Goal 5.

## Milestone: Separate Visibility and Correct Force Editing

User-selected corrections take priority over Unified Forces and Torques Goal 8.
Separate editing convenience from app rendering, then improve force controls.

1. [x] **Separate editor and app visibility** — Completed the reopened goal with
   runtime force arrows, signed 270-degree torque markers, and visible joint
   symbols. Visual item panels have a shared editor-only eye at the top left;
   Visible in App remains the first property below the header/name. Format 9
   preserves independent editor/app state, defaults missing app flags to visible,
   and retains parent suppression without changing child settings or physics.
   Source visibility survives CLI edits, copying, history, generation, and runtime
   save/load. The torque editor itself remains a later goal. Verified 90 native
   tests, six ASan/UBSan/leak suites, GUI startup, and Linux/Windows builds with no
   new compiler diagnostics. Seven installed-SDK consumers, all 12 standalone
   examples, and a generated default project compile on both platforms; Linux
   runtime/probe or bounded startup checks pass outside source directories.
   Regenerated and rebuilt `/tmp/rohr-projects/project-dir`; its unchanged entry
   point renders the scene, force arrow, and all three joint types. Windows
   execution remains unverified. Manual application and panel review is requested.
2. [x] **Add shared visual styling and previews** — Completed the selection
   follow-up: viewport picking now follows annotation and app-preview geometry,
   preserves anchor priority over overlapping same-layer joints, and aligns force
   rendering with picking. Camera guides no longer claim their empty interiors.
   Verified 90 native tests, five focused ASan/UBSan/leak suites, GUI startup,
   Linux/Windows compilation, and selectable force/pin/weld/spring regions in
   `/tmp/rohr-projects/project-dir`. No compiler warnings were introduced.
   Added force/torque/joint fill,
   border, signed X/Y scale, local offsets and rotation. Spring endpoint markers
   have independent styles; the coil connects their visual centers without
   changing physics anchors. App output and editor previews share world-space
   geometry below editing controls. Visuals accordions, compatible multi-edit,
   color pickers, history, clipboard, CLI replay/help, JSON, generated catalogs,
   and runtime snapshots preserve styles. Missing styles use defaults. Increased
   text-asset capacity for the expanded panels. Verified 90 native tests, six
   ASan/UBSan/leak suites, GUI startup, and Linux/Windows builds with no new
   compiler diagnostics. Seven installed-SDK consumers, all 12 examples, default
   generation, and a separately styled generated project build on both platforms;
   Linux runtime/probe and bounded startup checks pass. Regenerated and rebuilt
   `/tmp/rohr-projects/project-dir` with its entry point unchanged; the scene
   callback and force/joint output pass. Windows execution remains unverified.
   Manual editor/application review is requested; the torque editor stays deferred.
3. [x] **Improve force editing affordances** — Corrected the color follow-up:
   editor annotations retain cyan (muted gray for zero forces); only default app
   visuals use red-orange (`#FF6633`), retaining yellow selection and custom
   authored colors. The annotation-only correction passes focused force and
   visibility tests, GUI startup, and Linux/Windows editor builds without compiler
   diagnostics. The supplied application was rebuilt and its runtime marker
   drawing, scene callback and bounded startup checks pass. Completed the size follow-up:
   editor force arrows are now 88 pixels long with 4-pixel strokes; rotation
   arcs use a 120-pixel radius. Arrowheads, origin markers and picking tolerance
   scale by the same factor of two, independently of zoom and app visuals.
   Replaced the rotation dot with
   two short opposing curved arrows using shared drawing/picking geometry.
   Target and reference choices preview bodies, particles, whole soft bodies,
   nodes and beams, including filtered/scrolled lists and both between-location
   references. Hidden references temporarily show an outline without changing
   visibility, selection or history; previews never enter app/camera output.
   Verified signed/zero and reference-relative rotation, zoom-independent
   controls, cancellation and undo/redo. All 90 native tests and five focused
   ASan/UBSan/leak suites pass. Linux/Windows builds have no compiler diagnostics;
   GUI startup and the supplied project's selection, hover, curved-handle and
   rendering probe pass. Windows execution remains unverified. Manual editor
   visual review is requested.

Milestone complete. Stop for review; resume the torque editor only after
explicit user instruction.

## Milestone: Unified Forces and Torques

Support one shared force API and one shared torque API across rigid bodies,
standalone particles, soft-body nodes, beams, and whole soft bodies. Finish
with editor authoring and manipulation. This replaces the proposed Editor
Forces and Torques milestone and precedes numbered priority 4.

The [force/torque contract](docs/force_torque_contract.md) is authoritative for
approved behavior. Goal 1 is fulfilled by the documentation reconciliation
milestone. Entity Inspection and Diagnostic Reliability are complete. Goal 2
implements shared sources and rigid-body/particle behavior; Goal 3 adds receiving
beam loads; Goal 4 adds whole-soft-body loads. These runtime goals are complete.
Runtime Soft-Body Persistence is the completed prerequisite for Goal 5.
The user-selected Object Descriptions and Independent Spawning milestone is also
a completed prerequisite. Goals 5 and 6 now provide authored definitions,
persistence, runtime source instantiation, and generated catalog parity.
Editor handle presentation is finalized in the relevant editor goals.

1. [x] **Finalize the shared API contract** — Completed through **Force/Torque
   Contract and Roadmap Reconciliation**, with types, signatures, beam behavior,
   validation, lifetime, migration, examples, and acceptance criteria recorded.
2. [x] **Implement shared sources and rigid-body/particle behavior** — Added
   copied description APIs, atomic mutation, engine-tick applications, all
   location/orientation references, and coupled force-at-point response.
   Standalone particles/nodes remain nonrotating; rigid bodies with particle
   properties retain angular response. Entity-info captures descriptions and
   eligibility; reports count contributing, inactive, and suspended evaluations.
   Migrated callers, source persistence (runtime state version 5), SDK consumers,
   examples, and generated C11 scaffolds. Allocation-failure regressions exposed
   and fixed incomplete table-growth retries; malformed beam references reject
   before source creation. All 84 native tests and nine focused ASan/UBSan tests
   pass with leak detection. Linux and Windows builds pass without compiler
   warnings. Five installed-SDK consumers, all 12 standalone examples, and a
   generated project build on both platforms and run on Linux outside their
   source directories. Windows verification is compilation only.
3. [x] **Implement local beam loads** — Revised the contract so coincident
   application points produce identical loads regardless of fractional,
   distance, or world references. Shared sources prepare and atomically commit
   endpoint loads without allocation or child entities; inspection uses the
   same evaluator and reports count each source once. Verified net force and
   torque, 120 fixed-seed equivalence/work cases, deformation and energy
   convergence, fixed endpoints, numeric rejection, collapse recovery,
   endpoint changes/deletion, source lifetime, and direct/public parity.
   All 84 native tests and nine focused ASan/UBSan suites pass with leak
   detection. Linux/Windows builds pass without compiler warnings. Five
   installed-SDK consumers, all 12 standalone examples, and a generated project
   build on both platforms and run on Linux outside their source directories.
   Windows verification is compilation only.
4. [x] **Implement whole-soft-body loads** — Shared sources distribute translation
   and clockwise turning using current node masses, COM, and point-mass inertia.
   Double intermediates and relative coordinates preserve small radii; any
   positive inertia is accepted when loads and accumulators remain representable.
   Every node share is validated before atomic application, with fixed/held shares
   absorbed without redistribution. Inspection uses the same allocation-free
   evaluator; no child sources are created. Removed obsolete direct/public
   whole-body helpers and migrated callers and documentation. Analytic and
   100 fixed-seed cases cover net loads, equivalent points, mutable topology,
   full capacity, degeneracy, overflow, recovery, lifetime, and source reporting.
   Free and constrained beam deformation and energy convergence pass, including
   damping. All 84 native tests and 11 focused ASan/UBSan suites pass with leak
   detection. Linux/Windows builds have no compiler warnings. Five installed-SDK
   consumers, all 12 standalone examples, and a generated project build on both
   platforms and run on Linux outside their source directories. Windows checks
   are compile-only. Stop for user review before Goal 5.
5. [x] **Add authored definitions and persistence** — Added object-owned force
   and torque definitions, object-local typed references, CLI operations,
   validation, history, independent copying/duplication, and editable object
   descriptions. Project format 8 preserves modes, signed/zero magnitudes, and
   nested references. Referenced-element deletion retains sources and resets
   affected targets to none, locations to world origin, and orientations to
   world zero in the same undo transaction. Malformed documents are rejected
   transactionally. Added `ENTITY_NONE` as an alias of `ENTITY_INVALID` without
   changing runtime validation or game-state format 8. Goal 5 introduced temporary
   source spawning/generation guards, subsequently replaced by Goal 6 support;
   visual-only spawning and source-free generation continue.
   All 87 native tests and five focused ASan/UBSan suites pass with leak detection.
   Linux/Windows builds have no compiler warnings. Seven installed-SDK consumers,
   all 12 standalone examples, and a generated project build on both platforms;
   consumers and the generated object probe run on Linux, and applications pass
   bounded startup checks outside their source directories. Windows verification
   is compilation only. CLI lifecycle checks confirm invalid-edit rollback and
   generated-file preservation. Stop for user review before Goal 6. See the
   [Goal 5 contract](docs/force_torque_contract.md#goal-5-authored-definitions-and-persistence).
6. [x] **Generate runtime sources and verify API parity** — Loaded descriptions,
   generated catalogs, and source-bearing generated constructors share the object
   spawner. Sources resolve after topology and joint bindings, belong to their
   containing object, and roll back with failed instances. Persistent sources
   expose named mutable handles; initial sources use existing one-tick expiry
   with cancellation on instance deletion and no permanent placeholder. Save/load
   preserves ownership and remaining expiry without replaying expired sources.
   World references remain absolute and visual-only spawns exclude sources.
   Converted the existing soft-body car to an editable authored document with
   rear-wheel torque driven by A/D. Space applies a one-tick chassis-COM jump
   only when both wheels contact external rigid bodies; particle and self-contacts
   do not qualify. Enabled collision response on the existing walls/ramp and
   verified the actual starting-platform jump. Default project behavior is
   unchanged; graphical source controls remain Goals 7–8.
   All 88 native tests and five focused ASan/UBSan suites pass with leak detection.
   Linux/Windows builds have no compiler warnings. Seven installed SDK consumers,
   all 12 standalone examples, and the generated default project build on both
   platforms and pass Linux consumer/probe or bounded startup checks outside
   their source directories. An 85-source generated reference matrix compiles on
   both platforms and matches loaded descriptions on Linux. Windows verification
   is compilation only. Manual car-control review is requested at this boundary;
   stop for user review before Goal 7.
7. [x] **Implement the force editor** — Added top-level force rows and dedicated
   source/application/direction properties, with object-local reference pickers.
   Fixed-screen-size signed arrows support reference-preserving translation and
   rotation; zero forces remain selectable and magnitude is numeric. Integrated
   editor-only visibility, ordering, clipboard, duplication, multi-edit, history,
   cancellation, navigation persistence, and replayable visibility commands.
   Cross-object paste resets unavailable references; grouped transforms preserve
   offsets independently of selection order. Verified saved-project loading and
   rendering, GUI startup, 89 native tests, four ASan/UBSan/leak checks, and full
   Linux/Windows builds including examples. Windows verification is compilation
   only. Manual arrow/control review is requested before Goal 8.
8. [x] **Implement the torque editor and complete visual verification** — Added
   top-level torque rows, dedicated Visuals/Source/Editor Marker properties,
   target-hover outlines, and fixed-screen-size signed 270-degree controls.
   Markers follow target COM or beam midpoint plus editor-only world offsets;
   missing targets fall back to object origin. App-preview clicks select without
   dragging. Integrated editor eyes, independent app visibility, ordering,
   clipboard, duplicate, multi-edit, grouped transforms, cancellation, and history.
   Particle/node targets show no angular effect; rigid bodies with particle
   properties retain angular response. App torque defaults are red-orange;
   explicit styles are preserved. Project format 10 stores editor metadata and
   navigation while reading formats 8/9 with visible/zero defaults. CLI replay
   preserves offsets; generated/runtime descriptions exclude editor metadata.
   Verified 91 native tests, six ASan/UBSan/leak checks, GUI startup, saved-project
   force/joint picking, and Linux/Windows builds including examples. Installed-SDK
   generated applications compile on both platforms; Linux runtime probes confirm
   independent torque instances and editor/app separation. The generated app and
   standalone joints example launch from outside their source directories.
   Windows verification is compilation only. Final manual editor/app review is
   requested; stop for user review before beginning another goal.

### Approved contract summary

- Persistent and one-tick sources share target behavior; `add(description)` copies
  values immediately. Zero initialization is valid, with no target, world origin,
  world orientation zero, and zero magnitude. Both magnitudes are signed.
- Location and orientation have independent offsets/references. COM is explicit;
  anonymous unions omit `.data`, and enum names end in POSITION or ORIENTATION.
- Invalid configuration/stale API handles fail immediately and preserve state.
  Runtime reference/geometry failures warn and retry per substep, including at
  zero magnitude. No-target sources are valid no-ops, not unconfigured sources.
- Beam endpoint loads preserve net force, torque, and equivalent-attachment work
  using the resolved application point. Coincident fractional/distance/world
  points produce identical loads; fixed nodes retain their shares. No bending
  model or automatic reference reaction is added. Minimum beam length is a named
  compile-time `0.0001f` constant; failed coupled contributions commit nothing.
- Preserve engine-tick expiry. Entity-info inspection describes current state;
  physics reports count source evaluations and record actual update failures.
- Replace redundant APIs immediately with their implemented equivalents, remake
  examples, and regenerate saved/generated formats without backward compatibility.
  All exact types, APIs, calculations, and acceptance cases live in the
  [contract](docs/force_torque_contract.md); do not revive superseded create/add
  workflows, implicit COM defaults, or force-specific status getters.

### Existing support and verification

The shared add/description/field/apply APIs replace the old vector/scalar source
signatures without aliases. Nodes and whole soft bodies use the shared API;
whole-body sources recompute and atomically distribute loads each substep.
Joint point loads use one force-at-point source per endpoint. Beam references
and beam-target loads use the resolved application point for equivalent endpoint
contributions. Runtime state version 8 serializes source descriptions and full
soft-body topology with named references, shared anchors, relative lifetimes,
and generic ownership. Authored source persistence is complete in project format 8.

Runtime Soft-Body Persistence and all eight Unified Forces and Torques goals
are complete. Review the force and torque controls in the editor and app before
choosing the next roadmap goal.

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
- Add bounded physics timestep handling when elapsed ticks accumulate during
  window focus loss or other stalls. Define catch-up and backlog behavior
  explicitly, including interactions with one-tick forces and lifecycle
  expiry; do not silently discard elapsed simulation time or introduce a
  focus-pause policy. Verify normal frames and long delays with a player-and-floor
  regression. Before marking this goal complete, ask the user to retry
  `examples/player_controller`, switch away from the window and return, and
  confirm the player stays on the floor.
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
