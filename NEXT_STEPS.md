# Next Steps

## Roadmap milestones

These ten milestones are the authoritative priority order. The breakdowns below
retain their scope; the later-features list has no priority order.

| # | Milestone | Status |
|---|---|---|
| 1 | Audio | Complete |
| 2 | Asset and resource ownership | Complete |
| 3 | COM, inertia, and origin semantics | Complete |
| 4 | Cross-path feature parity | Goals 1–4 verified; platform and interactive acceptance in progress |
| 5 | Adversarial testing and hardening | Substantial coverage implemented; incomplete |
| 6 | Physics robustness | Partially implemented |
| 7 | Editor implementation scalability | Partially implemented |
| 8 | Global-state assumptions | Foundations implemented; runtime isolation pending |
| 9 | Python bindings | Pending |
| 10 | End-to-end game | Pending; demonstrations exist |

## Milestone breakdowns

Milestones 1–3 remain complete. Milestones 4–9 each have five acceptance goals;
the implemented foundations below retain credit without claiming that the new
acceptance criteria have already been verified. Mark a goal complete only when
its full criteria have supporting evidence.

Freeze each milestone's current feature inventory, applicable paths, fixtures,
and measurement procedure when it begins. Resolve material clarifications before
pursuing each goal. Future features and optional improvements do not expand the
acceptance scope unless they invalidate an agreed criterion. Record explicit
not-applicable paths rather than inventing authoring support for runtime-only
features.

Platform acceptance is Linux execution and Windows cross-builds. Native Windows
runtime verification is not a completion gate. Use interactive Linux checks for
rendering, audio, and input behavior that headless tests cannot establish.
Performance measurements use release builds on the Intel Core Ultra 5 125H
reference laptop on AC power; record fixture, build, sampling, and timing details
so results can be repeated. Lifecycle acceptance means 1,000 create/use/destroy
cycles without sanitizer findings, leaked owned resources, accumulating live
resource counts, or cross-instance interference; cached resident memory alone is
not a leak criterion.

### 1. Audio

Provide sound effects and music through the SDL audio backend.

1. [x] Load and share decoded WAV sound assets, with independent players.
2. [x] Stream one active Ogg Vorbis music track.
3. [x] Support playback, looping, mixing, volume, playback rate, pause/resume,
   and explicit unloading.
4. [x] Define resource lifetime, control submission, applied-state getters,
   update reports, and shutdown behavior.

### 2. Asset and resource ownership

Make ownership, sharing, failure cleanup, and destruction explicit before
expanding asset-heavy features.

1. [x] Use generation-checked texture, animation, and font handles with explicit
   references and safe destruction.
2. [x] Share immutable animation data while retaining independent playback state.
3. [x] Give rendered text safe payload updates and queued-draw/persistent-UI
   dependencies.
4. [x] Share decoded sound resources and keep streamed-music instances independent.
5. [x] Cover creation failure, rollback, release, and shutdown across the resource
   APIs and their public wrappers.

### 3. COM, inertia, and origin semantics

Keep origins, geometry, attachments, and physical response consistent across
runtime physics and authoring.

1. [x] Preserve origin-relative geometry and attachment offsets during transforms.
2. [x] Support automatic and explicit rigid-body COM, with derived inertia.
3. [x] Apply COM-based integration, forces, torques, joints, and collision response.
4. [x] Preserve standalone-particle restrictions and derive soft-body aggregate
   properties from nodes.
5. [x] Support matching editor controls, CLI, JSON, generated C, and runtime
   persistence.
6. [x] Provide examples and regression coverage for off-center geometry, rotation,
   attachments, and generated/direct parity.

### 4. Cross-path feature parity

Make all current supported features equivalent across direct C APIs, public
wrappers, editor, CLI, JSON, generated C, loading, and runtime behavior wherever
those paths apply. Gameplay logic remains developer-owned C.

Implemented foundations: description API migration and public/direct consistency;
explicit ownership, offsets, independent spawning, and internal connections;
camera/screen/viewport/layer/visibility/attachment authoring and generation;
persistence, round-trip, deterministic-generation, and independent-instance tests,
including incomplete topology and references; authored-object and direct-source
examples; installed-SDK Linux execution and Windows cross-build coverage.

1. [x] **Freeze the coverage inventory.** The
   [property and behavior inventory](docs/cross_path_feature_inventory.md)
   reconciles 1,602 API entries, 456 public configuration/nested fields, and 507
   editor data fields. It records applicable paths, source and existing-test
   evidence, explicit exclusions, comparison fixtures, and concrete gaps assigned
   to goals 2–5. Inventory checks pass; behavioral and platform acceptance remain
   with those goals.
2. [x] **Verify configuration and references.** Close confirmed configuration,
   ownership, attachment, and reference gaps in the inventory. Full descriptions
   and focused setters must retain their documented semantics. Resolve explicit
   pin/weld/spring placement: when requested, move the first anchor's object
   toward the second; low-level connection creation must not move objects
   implicitly. Cover reversed arguments, world/shared anchors, locks, and
   connected groups, plus remaining camera/attachment/viewport editing gaps.
   Completed configuration gaps and evidence are recorded in the
   [goal 2 closure](docs/cross_path_feature_inventory.md#goal-2-configuration-closure):
   109 Linux tests, eight focused sanitizer tests, Linux/Windows installed SDK
   consumers, and seven independently built examples on both platforms.
   Interactive visual/audio acceptance remains goal 5.
3. [x] **Verify persistence and generation.** For every applicable inventory
   entry, project/runtime round trips, loading, and generated C preserve supported
   values, references, and behavior. Generation is deterministic; independent
   instances do not share mutable state. Add regressions for confirmed omissions.
   Completed [goal 3 evidence](docs/cross_path_feature_inventory.md#goal-3-persistence-and-generation-closure)
   covers named-layer identity/transactional merging, optional angular limits,
   explicit kinematic intent, exact project timesteps, initialized camera
   defaults, deterministic generation and independent restored instances.
   Only current project format 11/runtime format 9 are accepted; bundled files
   and generated modules are refreshed. Verification: 109 Linux tests, eight
   sanitizer suites plus the generation fixture, Linux/Windows SDK consumers,
   and eight independently built examples with Linux startup checks.
4. [x] **Verify authored and direct construction.** Equivalent authored-object
   and direct-C fixtures produce matching runtime configuration and behavior,
   including soft-body topology, UI layouts, and repeated spawning. Keep the
   starter's high-level object spawn plus additional direct-API construction.
   Completed [goal 4 evidence](docs/cross_path_feature_inventory.md#goal-4-authored-and-direct-construction-closure)
   compares independent individual-API construction with project catalogs,
   file loading, generated catalogs and convenience constructors. Physics,
   topology, camera/UI, animation and ownership checks pass without engine/API
   changes. Verification: 111 Linux tests, ten focused sanitizer suites plus
   both generators and SDK consumers, Windows cross-builds, and independent
   Linux/Windows builds of the starter and eight examples. Interactive review
   remains goal 5.
5. [ ] **Verify supported platforms and interaction.** Build affected generated
   applications and standalone examples against installed SDKs, execute on Linux,
   and cross-build for Windows. Launch examples outside their source directories.
   Complete interactive Linux checks for remaining composition and editing
   behavior; every applicable inventory entry must have passing evidence.
   The [guided review](docs/cross_path_feature_inventory.md#goal-5-platform-and-interactive-review)
   provides a reproducible project and five acceptance cases. Automated passes
   do not close this goal until the Linux visual, input and audio results are
   confirmed.

#### Corrective milestone for goal 5

Resolve the issues found during interactive acceptance, make project builds
reliable, and complete the agreed directional collision-response design across
all applicable paths. These correction goals belong to milestone 4 goal 5;
they do not reorder the ten roadmap milestones or reopen verified goals 1–4.
The five interactive acceptance cases remain the completion gate. The accepted
camera-preview retest retains credit; rerun affected checks after corrections.

Unchecked entries are planned work. Pursue one correction goal
at a time only when requested, and settle its remaining clarifications before
implementation. The [issue mapping](docs/cross_path_feature_inventory.md#goal-5-corrective-issue-mapping)
accounts for every item in the user's review. Update the applicable inventory,
contracts, and generated examples when a goal changes supported behavior.

1. [x] **C1 — Generation and build diagnostics.** Route Generate C,
   configuration, compilation, and launch-preparation diagnostics to the editor
   terminal, including failures before a subprocess starts. Retain notifications
   and identify the failing stage and affected object/path. A deliberate
   generation failure and a compiler failure must both expose their actual
   reasons; disabling optional tree output must not suppress diagnostics.
   Complete: output is always captured in terminal scrollback; visibility and
   focus remain manual-only. Removed the build-output toggle while retaining
   optional tree output and notifications. Verified actual GUI controller
   generation/configuration/compiler/startup failures, cancellation/recovery,
   and successful builds with the terminal hidden. Linux regression checks,
   focused ASan/UBSan, and Windows editor/test cross-compilation passed; see the
   corrective issue mapping for verification details.
2. [x] **C2 — Relocation and repeat builds.** Resolve project-owned paths from
   the project's current root. Reuse valid CMake caches on repeated builds and
   recover stale project/build-root metadata after relocation. Preserve source,
   assets, and developer configuration. External engine/SDK paths remain explicit
   external dependencies. Verify initial, unchanged, edited, and relocated
   application/scene builds without a manual clean-build requirement.
   Complete: editor/CLI share root resolution and CMake relocation recovery.
   Valid caches are retained; relocated metadata is archived and regenerated
   from saved settings without migrating cache-only options. Stale launches
   are blocked, including copies whose original still exists. Installed-SDK
   fixtures cover unchanged/edited/moved/copied app and scene builds and launches,
   preserved developer files/assets, and cache reuse. Linux regressions, focused
   ASan/UBSan, and Windows editor/CLI/test cross-builds passed; verification is
   recorded in the corrective issue mapping.
3. [x] **C3 — Missing CMake regeneration.** Restore missing engine-generated
   project CMake scaffolding during build preparation and let CMake regenerate
   disposable build metadata. Preserve existing customized files and
   developer-owned main.c. Verify rebuilding after removal of the root template,
   generated scene CMake file, and CMake cache/build metadata separately.
   Complete: shared editor/CLI build preparation restores missing root and scene
   CMake files, including custom-command and compile-only builds. Existing CMake
   customizations are preserved through generation/build; source, assets, and
   settings remain untouched by recovery. Configuration stays explicitly
   controllable, and restoration/errors reach the terminal without changing
   visibility. Installed-SDK checks cover separate scaffolding/cache/build-file
   deletions, unchanged rebuilds, and application/scene launches. All 114 Linux
   tests, focused ASan/UBSan, and Windows editor/CLI/test cross-builds passed;
   see the corrective issue mapping for details.
4. [x] **C4 — Runtime setup with partially spawned projects.** Reproduce and
   resolve the invalid-entity startup in /tmp/rohr-projects/rohr-project-1.
   Generated presentation setup must handle objects that main.c has not spawned
   without blindly using zero handles. Missing authored object/camera screen
   references fail generation before file writes. Unavailable runtime cameras
   retain their screens and show the editor-style construction-stripe pattern,
   with a viewport-composition report warning instead of a startup failure.
   Camera destruction preserves screens; replacement requires explicit binding.
   Preserve developer-owned entry points. Verify partial/full spawning, invalid
   authored references, missing-camera composition, and camera replacement.
   Complete: the copied affected project starts without rewriting main.c;
   generated setup/drawing skip unspawned entities. Runtime missing-camera
   screens retain placement and draw construction stripes with report warnings.
   Authored reference validation runs before generation writes. Linux regression
   checks pass after refreshing the committed generated example; installed-SDK
   partial/full/missing-camera applications and scene startup, focused ASan/UBSan,
   and Windows cross-builds passed. User visual verification: PASS. See the
   corrective issue mapping for evidence and platform limitations.
5. [x] **C5 — Incomplete joint parity.** Preserve otherwise valid joints with
   unset endpoints in the editor, saved project, generated descriptions, and
   runtime instances. Match the engine: retain the definition, skip only the
   unavailable physics contribution, and expose diagnostics. Do not drop the
   joint during generation or reject an entire scene merely for an unset
   endpoint. Invalid numerical data and nonzero stale handles remain errors.
   Verify zero/one/two endpoints, save/load, generation, execution, and later
   completion of the same joint.
   Complete: object conversion and both generated constructor paths retain
   unset endpoints. Soft-node anchor declarations/bindings and creation order
   are corrected. Existing physics skip/report behavior is unchanged; completing
   endpoints activates the same entity. Invalid authored references/numbers
   still fail. All 117 Linux tests, including installed-SDK application and scene
   startup, three focused ASan/UBSan checks, and Windows cross-builds passed.
   Native Windows execution was not performed; see the corrective issue mapping.
6. [x] **C6 — Shared panel lifecycle and scrolling.** Remove inactive child
   controls when returning to object/top-level editors. Fix overlapping panels,
   first-element activation glitches, stale input capture, and blocked scrolling
   in object/joint editors. Cover all editor kinds through shared navigation,
   including opening the first row, returning to a parent, switching siblings,
   and scrolling without an off-column or viewport click to recover.
   Keep Back/Close, the editor eye, and name outside the property scroll region.
   New child and sibling editors start at the top; only Back restores a parent's
   previous scroll position, clamped to its current content. History is local to
   the open project and drops deleted items. Verify five cases: first-row open;
   nested Back; sibling/type switching; wheel/scrollbar and fixed-header access;
   and navigation with active controls, deletion, and project close/reopen.
   Complete: shared editor/item identity clears inactive interaction capture;
   fixed headers, measured content, and Back-only scroll history cover parent
   and sibling navigation. Shared settings target the open item and object
   settings now undo independently. All 118 Linux tests, five focused ASan/UBSan
   checks, standalone installed-SDK UI example startup, and Windows cross-builds
   passed. User verification of all five cases: PASS. An initial scrolling
   failure could not be reproduced after switching windows; the exact
   hitbox/body/object return path has regression coverage. See the issue mapping.
7. [x] **C7 — Build path settings.** Expose editable project, engine/SDK,
   source, assets, generated source, objects, editor-state, build, application,
   and scene paths with defaults, Browse, Reset, and resolved values. Share path
   configuration between editor and CLI. Changing Project moves the project;
   changing content paths moves contents and managed references. Internal paths
   follow the project; external locations never move merely because Project
   moves. Executable overrides control build output and launch. Preserve custom
   main.c, valid caches, and compatible custom scaffolding; reject conflicting
   destinations before mutation and retain recoverable originals through commit.
   Verify five cases: (1) defaults/persistence/reset, (2) project relocation with
   external paths and across filesystems, (3) nested/spaced content relocation
   with previews, persistence, generation, application and scene, (4) custom
   SDK/build/output paths and editor/CLI repeat-build parity, and (5) conflicts,
   rollback, cancellation, custom scaffolding, and modal/scroll interactions.
   Complete: implementation and automated verification passed. By explicit user
   decision, C7's five manual review checks are **deferred and unverified**, to
   revisit after milestone implementation and during incremental full-game
   testing. This exception applies only to C7; all other manual checks remain
   required unless the user explicitly defers them when presented. Preserve the
   review cases in the corrective issue mapping; do not record them as passed.
8. [x] **C8 — Force and torque selection geometry.** Require direct hits on
   visible arrow geometry, including app visuals when shown. Use matching force
   and torque shaft widths (8.32 / 1.4 screen pixels), with shorter shared
   chevron heads at 70% shaft thickness and one joined tip. The force origin
   circle is decorative; empty torque centers/gaps and surrounding space do
   not select sources. Retain the original force rotation-handle size and
   forgiving 14-pixel buffer. Selecting a body must clear a previously selected
   force/torque from both the active selection and shared selection list.
   Verify five cases: nearby body selection and clean handoff; visibility;
   zoom/transforms; overlapping visuals and context-menu targets; and
   dragging/rotation with undo/redo. Complete: user review PASS after the final
   size, arrowhead and selection-handoff refinements. All 120 Linux tests, Linux
   examples, five focused ASan/UBSan tests with leak detection, and Windows
   editor/force/torque test cross-builds passed without compiler warnings. Native
   Windows execution was not performed; see the issue mapping for evidence.
9. [x] **C9 — Appearance and visual layers.** Move render-layer controls from
   Transform to Appearance and provide missing layer controls for editor
   visuals, including anchors. Distinguish editor-only annotations from
   application visuals; do not make anchors render in applications implicitly.
   Use one shared numeric/named layer per item. Editor annotations always draw
   above application visuals; layers sort within each category. Rename source
   and joint Visuals accordions to Appearance. Anchors and camera guides remain
   editor-only. Preserve soft-child inheritance, existing values, and zero
   defaults for old documents. Verify five cases: panel placement/scrolling;
   overlap drawing/picking/context targets; editor/app separation; persistence,
   copy and history; and CLI/loading/generated application/scene parity.
   Complete: verified by 120 Linux tests, five focused ASan/UBSan tests,
   Linux examples, and the Windows editor cross-build. User review PASS for
   the five cases; a final wiring audit connected the layer control to the
   force, torque, joint, anchor, and camera panels. Eight affected tests and
   both builds pass after that correction. The user explicitly skipped the
   additional panel confirmation; it remains unverified as a recorded exception.
10. [ ] **C10 — Directional collision response.** Rename Collides With to
    Responds To across applicable API, editor, CLI, JSON, generation, and docs.
    For each overlapping pair, independently decide which body responds: both
    gives normal response; one gives one-sided response; neither gives no
    physical response. Treat a nonresponding side as zero inverse mass/inertia
    for that contact only, retaining its motion and both bodies' material
    combination rules. Apply the same decision to penetration correction,
    impulses, friction, and restitution across supported collider pairs.
    Preserve overlap detection independently of these masks and respect
    Static/Kinematic restrictions. Test all four mask combinations, reversed
    arguments, moving nonresponders, and a third body with a different response
    relationship. Existing data must not be silently reinterpreted while C11's
    migration is unavailable.
11. [ ] **C11 — Behavior-preserving mask migration.** Convert existing mutual
    filters into the new directional system by adding/splitting categories and
    rebuilding response masks as needed. Preserve every previously allowed or
    excluded pair, including collider profiles sharing a category but having
    different filters. Use deterministic remapping, retain usable named-mask
    references, and verify equivalence before publishing migrated data. Check
    available category-bit capacity; report an unrepresentable conversion
    without partially modifying the project. No permanent legacy mode. Cover
    save/reload, regeneration, supported stored-data paths, and explain effects
    on developer-authored C that uses category constants.
12. [ ] **C12 — Explicit kinematic authoring.** Expose Dynamic / Static /
    Kinematic for supported physical entities using the engine's existing motion
    semantics. Preserve explicit kinematic intent and mass through mode changes,
    undo/redo, project persistence, CLI, generation, and spawning. Body-wide
    Kinematic remains independent of per-category response masks. Verify driven
    movement, immunity to contact impulses, and normal response after returning
    to Dynamic with valid mass.
13. [ ] **C13 — Soft-body Physics controls.** Expose collision, gravity, and
    supported motion modes at soft-body level. Changing a body-level property
    bulk-applies that property to all existing nodes and sets its default for
    new nodes. Individual nodes can then be edited independently; a subsequent
    body-level edit overwrites that property's node values again without
    resetting unrelated properties. Apply this policy consistently to collision
    settings and applicable flags. Keep beam collision explicitly opt-in.
    Include mixed-state display, persistence, generation, and undo/redo.
14. [ ] **C14 — Immediate joint clamping.** Apply editor joint placement without
    requiring an unrelated subsequent movement. Respect world/fixed anchors,
    locks, connected groups, incomplete joints, and a single undo transaction
    per gesture. Update placement both when joint properties change and
    continuously while dragging connected geometry. Keep runtime low-level
    joint creation non-moving unless placement is explicitly requested.

### 5. Adversarial testing and hardening

Expose malformed input, resource failures, lifecycle defects, and integration
regressions within the frozen inventory of first-party loaders and subsystems.

Implemented foundations: Linux ASan/UBSan support; failure-injection and lifecycle
suites for allocation, capacity, stale handles, ownership, initialization,
rollback, and recreation; malformed project/configuration/runtime-state fixtures
and diagnostic recovery coverage; installed-SDK automation, distribution
workflows, generated scaffolding, and developer-file preservation tests.

1. [ ] **Fuzz malformed input.** Run a sanitizer-enabled one-hour campaign per
   in-scope first-party loader, including project, runtime-state, and object
   description loaders where applicable. Use a fixed seed corpus and record
   reproduction details. Fix discovered crashes and sanitizer findings, retain
   regression inputs, and finish with a clean campaign for each loader.
2. [ ] **Verify failure rollback.** Audit existing coverage and exercise remaining
   allocation/capacity failures, partial initialization, invalid handles and
   references, degenerate geometry, and extreme values. Each injected failure
   must preserve documented state, report the failure, and release resources
   acquired by the failed operation without corrupting surviving owners.
3. [ ] **Verify lifecycle stress.** Apply the shared 1,000-cycle criterion to
   in-scope subsystem creation, ownership changes, use, shutdown, and recreation.
   Cover shared resources, partial initialization, and repeated shutdown with
   Linux ASan/UBSan and live-resource accounting.
4. [ ] **Verify diagnostics and recovery.** Invalid input, report overflow, and
   recoverable failures must produce the documented errors/reports and permit
   subsequent valid operations. Verify real desktop/audio/input behavior on Linux
   where headless tests are insufficient; retain recovery regressions.
5. [ ] **Automate consumer verification.** Cover installed-SDK consumers,
   generated applications, and examples with Linux execution and Windows
   cross-builds, including launches outside source directories and preservation
   of developer-owned files. Enforce first-party/generated snake_case filenames
   in CI while preserving required ecosystem names.

### 6. Physics robustness

Pass five finite scenarios covering extreme scales, fast motion, contact
stability, constraints, and deformable bodies. Existing contact-stability,
concave-collision, beam-contact, and recovery fixes/tests remain the baseline.
The AABB-tree broad phase and live-entity iteration are already implemented.

Run all five fixtures at 30, 60, and 120 updates per second, plus a 60 Hz run with
one 100 ms stall and one 1 s stall. Process all elapsed time in the immediately
following update, taking at most 50 ms of physics CPU time for that update;
do not discard time. Preserve one-tick force and lifetime semantics and verify
normal/delayed player-floor behavior, including switching away and returning.
Normal 60 Hz physics CPU time must be at most 2 ms at the 95th percentile on the
reference laptop. Record completed-substep measurements, not just rendered frames.

No scenario may tunnel through enabled colliders or produce invalid numbers or
uncontrolled numerical energy. For scenarios 2–5, penetration must be at most 1%
of the smallest colliding feature. Pin/weld positional error must be at most
0.1% of the fixture reference length; weld angular error at most 0.1 degree.
Isolated rebound speed must be within 1% of the expected value, with zero
restitution checked as no rebound within the fixture's numerical tolerance.
Freeze exact geometry, offsets, impulses, reference lengths, run lengths, and
measurement tolerances before implementing the relevant goal.

**Unresolved settling criterion:** review a visual example with the user before
choosing settling time and residual-motion limits for supported stacks and damped
soft bodies. No 30-second settling limit or automatic freezing is agreed. This
review is a prerequisite to completing affected goals; it does not relax
collision requirements during motion or require undamped pendulums to stop.

1. [ ] **Extreme mass/size drop.** Place a 0.01 × 0.01, mass 0.0001 rigid body on
   a 1,000 × 0.1 static floor whose stored mass is 0.0001. Drop a 100 × 100,
   mass 10,000,000 rigid body from rest with a 1,000-unit gap between its bottom
   and the tiny body's top. Use gravity -9.8, restitution 0, and friction 0.8.
   Test centered and slightly offset impacts. Penetration must stay at or below
   0.0001 world units at completed substeps; the static floor must remain fixed
   regardless of stored mass. The centered stack must remain supported without
   crushing geometry, sinking, or explosive numerical behavior. Offset impacts
   may tip or slide naturally without artificial locking. Resolve settling via
   the visual review above.
2. [ ] **Fast objects against thin barriers.** Fire a 1 × 1 rigid body and a
   radius-0.5 particle at a 0.1-thick static wall at 100, 1,000, and 10,000
   units/second. Cover straight and oblique impacts with restitution 0 and 0.8.
   Require no tunneling and compliance with penetration and rebound criteria.
3. [ ] **Persistent stack stability.** Stack twenty 10 × 10, mass-1 boxes on a
   static floor, simulate for 60 seconds, then strike the stack sideways with
   another box. Require no progressive sinking, growing jitter, or explosive
   numerical collapse; natural toppling is valid. Freeze the strike and
   post-strike observation interval, and agree settling limits after visual
   review before completing the goal.
4. [ ] **Joint stability.** Exercise a pinned pendulum, welded pair, and
   spring-supported weight under gravity and a defined impulse. Pin/weld errors
   must meet the shared tolerances; spring motion must match its configured
   stiffness and damping against the frozen fixture's reference behavior.
5. [ ] **Soft-body contact and stability.** Drop a beam-braced soft body onto a
   floor and ramp, then apply rigid-body and particle loads. Deformation is valid;
   enabled collider contacts must meet the shared penetration/no-tunneling
   criteria. Agree damping and settling limits through visual review before
   completing the goal.

Choose algorithms to satisfy measured failures in these scenarios. Adaptive
substeps with reported counts, swept collisions through the shared pipeline,
concave-piece caching, stable boundary contact identifiers, sleeping with correct
wake propagation, compact active constraints, and removal of remaining capacity
scans are implementation options, not additional completion gates. Profile before
changing data structures. If compound contours or holes are needed by a fixture,
define their ownership and authoring before implementation; do not silently expand
collision semantics.

### 7. Editor implementation scalability

Reduce manual property edit sites and keep large scenes usable while preserving
explicit C ownership and existing interaction/serialization behavior.

Implemented foundations: editor-core extraction; command, document, build, and
scene-session modules; shared controls and extracted item editors/panels/viewport
controls; dynamic storage for major authored collections, including objects,
bodies, hitboxes, soft-body elements, animations, cameras, and viewport items.

Freeze a mixed scene of 10,000 individually counted items: bodies, nodes, beams,
visuals, and UI count separately. All items must be loaded; include a view showing
all items. Use the reference laptop and cached assets for interaction measurements.
A 100,000-item scene is not required.

1. [ ] **Use one shared property definition.** Audit duplicated definitions and
   demonstrate that, once an ordinary scalar runtime property exists on an
   existing component, one authoring definition supplies editor controls, CLI,
   validation, persistence, and generation. Tests must prove coverage of each
   path. Introduce metadata/generation incrementally without hiding ownership.
2. [ ] **Render the large scene.** Meet a frame time of at most 16.7 ms at the
   95th percentile with the full fixture loaded, including the all-items view.
   Preserve rendering/composition behavior and record the measurement procedure.
3. [ ] **Keep editing responsive.** Common selection, property edits, and undo
   actions must complete within 100 ms at the 95th percentile on the fixture,
   with correct selection/state restoration and no interaction regressions.
4. [ ] **Bound load, save, and generation.** Each operation must finish within
   5 seconds for the fixture, preserving data and deterministic generation.
   Record asset/cache conditions for each measurement.
5. [ ] **Verify growth and cleanup.** Grow, remove, and recreate fixture items
   across collection growth boundaries without lost references, invalid state,
   or leaked owned resources. Replace remaining fixed capacities where needed
   with explicit allocation/failure handling. Split oversized modules where it
   supports these goals, retaining interaction and serialization tests.

### 8. Global-state assumptions

Support two independent runtime worlds in one process, including presentation.
Multiple OS windows and parallel simulation threads are not required.

Implemented foundations: explicit editor document/session ownership and teardown;
subprocess-hosted simulation with session-owned controls, frames, and logs;
independent spawned-object/UI instances, resource lifetime tests, and sequential
engine recreation. These do not yet establish independent in-process worlds.

1. [ ] **Isolate simulation and clocks.** Inventory globals and define world and
   subsystem lifetimes before moving state. Run two worlds with independent
   entities, physics, clocks, pause, and step controls. Mutating, advancing, or
   pausing one must not alter the other.
2. [ ] **Isolate presentation.** Render both worlds through their own camera and
   viewport output. Presentation changes and teardown in one must not change or
   invalidate the other's output; one OS window may contain both.
3. [ ] **Route input explicitly.** Route input to the intended world and verify
   focus/selection changes do not leak events or mutable input state into the
   other world. Keep tool and simulation dependencies explicit.
4. [ ] **Share resources with explicit ownership.** Verify independent mutable
   state and safe sharing of immutable assets across worlds. Releasing one
   world's references must not invalidate the survivor; release final resources
   when their final owner is destroyed.
5. [ ] **Verify teardown and recreation.** Apply the shared 1,000-cycle criterion
   while repeatedly destroying/recreating one world with the other live. Verify
   simulation, presentation, input routing, and shared resources remain valid,
   with no cross-world mutation or accumulating live resources.

### 9. Python bindings

Expose the stable public runtime C API and authored-object spawning through thin
bindings with explicit ownership. Freeze applicable API coverage at milestone
start. Editor project automation is outside scope; document exclusions for C-only
mechanisms such as code-generation macros.

1. [ ] **Cover the runtime API.** Define handle, value, error, and lifetime
   mappings, then bind every applicable inventory entry with C as the canonical
   API. Include loading/spawning authored objects and direct construction.
2. [ ] **Verify C/Python equivalence.** Equivalent representative workloads must
   produce matching configuration and behavior for creation, updates, focused
   mutation, authored-object spawning, and shared resources.
3. [ ] **Verify ownership and errors.** Preserve explicit destruction, stale
   handle behavior, and error reporting. Apply the shared 1,000-cycle criterion
   to Python creation/use/destruction, including shared resources and failures.
4. [ ] **Meet the overhead budget.** Each agreed equivalent representative
   C/Python workload must have at most 20% additional CPU time in Python. Freeze
   workloads and measurement procedures before implementation. Exclude imports,
   startup, render waits, and frame caps; measure whole workloads, not individual
   call overhead.
5. [ ] **Package and demonstrate usage.** Provide installation instructions and
   runnable authored-object and direct-runtime examples. Verify package/consumer
   use by Linux execution and Windows cross-builds within the agreed platform
   scope.

### 10. End-to-end game

Develop a large game last, iterating engine improvements during its development.
Existing Pong and vehicle demonstrations are groundwork. The user determines
completion; no automatic release, feature-complete, or numeric acceptance gate
is imposed on this milestone.

1. [ ] Agree the game's scope and development direction with the user, including
   how it exercises physics, rendering, audio, UI, input, and assets.
2. [ ] Build gameplay using authored content and direct C where appropriate.
3. [ ] Integrate the game's saving/loading and start/play/exit lifecycle as its
   requirements develop.
4. [ ] Build and check distribution within the supported platform scope, with
   additional release requirements decided by the user.
5. [ ] Iterate engine improvements exposed by game development and obtain the
   user's verification that this milestone is complete.

## Unordered later features

- Tile maps.
- Prefabs.
- Navigation and pathfinding.
- Scripting.
- Networking.
- Asset hot reload.
- Gamepad discovery, device lifetime, buttons, triggers, and sticks.
- Object Add palette: a grid of item types, repeated insertion, keyboard
  navigation, disabled-state explanations, and predictable dismissal.
- Multiple windows: owned handles, independent presentation, focus and event
  routing, and per-window camera/screen/viewport composition. Build on the
  instance-isolation work; preserve existing single-window viewport support.
- Game-state serialization refactor: clearer JSON-construction helper names and
  focused asset, UI, entity, and component modules, preserving the file format.
- General file API, deferred during report-output work.
- Manually driven audio sample generation and output, deferred during audio
  reporting work.
- Input update reports, explicitly deferred; retain current immediate errors and
  text/IME truncation flags unless this feature is selected.
- Soft-body car example cleanup: spawn the authored car as one object and build
  additional scenery through the direct API. This remains a separate proposal
  from the generated starter tutorial.
- Manual inertia authoring, additional force-emitter tooling, and broader
  attachment API redesign beyond existing support; requirements remain undefined.
