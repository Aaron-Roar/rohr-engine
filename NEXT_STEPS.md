# Next Steps

## Roadmap milestones

The table and breakdowns follow the authoritative priority order. Milestone
identifiers remain stable; they no longer indicate execution order. The small
C game checkpoint is separate from milestone 10. The later-features list has no
priority order.

| ID | Milestone / checkpoint | Status |
|---|---|---|
| 1 | Audio | Complete |
| 2 | Asset and resource ownership | Complete |
| 3 | COM, inertia, and origin semantics | Complete |
| 4 | Cross-path feature parity | Complete by user acceptance; recorded verification exceptions remain |
| 14 | Frame pacing before game development | Complete; corrected pacing verified and initial previews accepted |
| 15 | [Descriptor-based creation and automatic viewport rendering](#15-descriptor-based-creation-and-automatic-viewport-rendering) | Complete; all six goals verified, final generated workflow accepted |
| 16 | [Scoped creation and a teaching example](#16-scoped-creation-and-a-teaching-example) | Complete; all three creation paths and SDK workflows verified, final preview accepted |
| 17 | [Consistent creation APIs and report checks](#17-consistent-creation-apis-and-report-checks) | Complete; all six goal results recorded below |
| 18 | [Explicit runtime lists and ID/name access](#18-explicit-runtime-lists-and-idname-access) | Complete; Goals 1–3 verified and Linux preview accepted |
| 19 | [Physics setters that preserve unrelated values](#19-physics-setters-that-preserve-unrelated-values) | Complete; all three API goals verified, baseline solver failures preserved |
| 20 | [Independent animation attachments](#20-independent-animation-attachments) | Complete; all three goals verified, Linux editor and application previews accepted |
| 21 | [Animation playback modes and completion](#21-animation-playback-modes-and-completion) | Goal 1 complete; stopped for review; editor integration follows |
| — | [C game experiment](#c-game-experiment) | Follows the current API work; agree concept and completion criteria with the user |
| 13 | Physics rebuild from the simpler baseline | Paused by user direction for frame pacing and game development; goals 1, 2a, 2b and 4a complete; goals 2 and 3 failed; goal 4 blocking remains incomplete; goals 5–8 not started |
| 12 | Real-time physics pipeline rework | Closed by user direction; superseded by milestone 13, not complete; goals 1–13 complete, goal 14 unfinished, goals 15–19 not started; evidence preserved |
| 11 | Soft-body correctness and real-time performance | Superseded by milestone 12; not complete; evidence preserved |
| 6 | Physics robustness | Goals 1–4 accepted; goal 5 failed verification |
| 8 | Runtime isolation (global-state assumptions) | Foundations implemented; two independent worlds pending |
| 5 | Adversarial testing and hardening | Substantial coverage implemented; follows runtime isolation |
| 9 | Python bindings | Pending; bind the stabilized runtime API after isolation and hardening |
| 7 | Editor implementation scalability | Deferred until after Python unless game development makes it necessary sooner |
| 10 | End-to-end game | Pending; separate larger game directed by the user |

**Current priority:** Milestone 21 Goal 1 is complete: runtime loop/play-once
modes, writable persistent completion, restart, descriptions and save/load are
verified. Stop for review and commit. Goal 2 adds editor and generated-project
support after separate user instruction. The C game experiment follows this API
work; physics milestones remain paused and simulation defaults are unchanged.

Milestone 15 is complete. Its Goal 1 defines the
[creation and viewport contract](docs/creation_and_viewport_contract.md).
Goal 2 implements [shared entity/object creation](docs/shared_creation.md).
Goal 3 implements [automatic viewport rendering](docs/automatic_viewports.md),
complete creation from one descriptor, explicit camera bindings and selected/batched
presentation. Goal 4 implements unified project generation and, by explicit user
direction, removes generated viewport compatibility helpers and migrates the
starter and simulation entry points. Goal 5 migrates every graphical example.
Goal 6 verifies generic Linux SDK packaging, lifecycle, Windows cross-builds and
controlled rendering performance; the user accepts the final extended-viewport
workflow. See the [integrated results](docs/creation_rendering_verification.md).
Descriptions remain immutable, created instances remain mutable, camera bindings
are explicit and applied during viewport creation, and live viewports remain
extensible from application code. Automatic rendering reuses runtime visual
state and uses discovery order within equal layers.

Milestone 14 remains complete: corrected car/pit runs achieve about
60.06/60.05 FPS without accumulating pacing lag; the recorded Linux/sanitizer,
Windows cross-build, VSync/window-switch checks and initial preview acceptance
remain in the [report](docs/frame_pacing.md).
The user explicitly paused further physics development
to learn from a playable game. Preserve the current physics defaults, unfinished
requirements and known failures; do not resume physics automatically.

**Latest physics result:** Milestone 13 Goal 4a's bounded collision comparison is
**complete; both prototypes are rejected for adoption**. Sweeps pass 14/18 focused
cases versus 4/18 for control and prediction, but correction/rotation failures,
worse car deformation and added pit/flies CPU prevent adoption. Current engine
defaults remain unchanged. Goal 4's reliable blocking is still incomplete; stop
for user review before further implementation. See the
[comparison report](docs/physics_collision_experiments.md).
Goal 2b is **complete**: all 499 small flies
are standalone particles, preserving the overlapping spawn and large fly controls.
Three serial comparisons measured 51.42% less median physics CPU; Linux,
sanitizer and Windows cross-build checks pass, and the user accepted the preview.
See the [results](docs/physics_compact_solver.md#flies-in-pit-standalone-particle-correction-goal-2b).
Goal 2a is **complete**: the user-selected
compact scalar version from the last Goal 2 attempt is now this branch's default
engine. It uses fixed-step/gather-once physics at 60 Hz, four substeps, two
particle-contact passes and four other solver passes. Normal car/pit entry
points reproduce the accepted 600/720-frame traces exactly. Linux adoption
checks, ASan/UBSan and Windows cross-builds are verified; the numerical failure
inventory remains explicit in the [report](docs/physics_compact_solver.md#promotion-verification).
Goal 2 remains **failed** against the 4 ms average / 5 ms p95 and strict rendered
gates. Goal 3's guard remains rejected. Further optimization or protection work
requires user instruction.

**Prior direction:** [Milestone 13](#13-physics-rebuild-from-the-simpler-baseline)
rebuilds physics outward from the simpler historical execution model inside the
current engine. Milestone 12 is closed by user direction and superseded, not
complete. Its completed goals retain credit; unfinished Goal 14 and unstarted
Goals 15–19 are historical scope, not active work. Preserve all code, prototypes,
executables and evidence. Milestone 13 Goal 1 established the simpler baseline.
Goal 2's first optimization attempt failed its performance gates; its retained
implementation and evidence remain the comparison baseline. By user direction,
Goal 2 is rebased for seven ordered investigations, starting with 30 Hz physics.
The seven investigations are measured; Goal 2 remains failed against its strict
performance gates, with evidence preserved. The user selected retaining the
trajectory-preserving changes, preserving sleeping for later, and accepted the
60 Hz/two-particle-pass car and pit previews. The next priority at that time was
Goal 3: [minimal swept static-wall protection](docs/physics_swept_wall_guard.md),
measured at those settings. Former Goals 3–7 move to 4–8 with their scope intact.
This roadmap update does not start implementation or select production defaults.

**Preserved physics history:** milestone 6 goal 4, **Joint stability**, including the user's
visual/window-switch acceptance. Its frozen fixtures and verification are
recorded below. Goal 5, **Soft-body contact and stability**, was pursued and
**failed verification** at checkpoint `63fccb26` on extreme-load penetration and
CPU limits. The preserved milestone 11 attempt passes all 48 existing numerical checks,
but still misses delayed-update and car real-time budgets; neither goal is complete.
Goal 3's timing rerun on the unchanged build passes at 45.432 ms delayed and
0.771 ms worst 60 Hz p95; earlier 69.505/58.421 ms failures remain evidence of
measurement variability, not an established physics regression. Milestone 12
superseded milestone 11 with the then-highest priority: achieve real-time
car and pit performance, then compare accuracy and behavior with the frozen
`physics-pipelines-overoptimized` reference at `6f283268`. Exclude `main` from
comparisons. Milestone 12 goal 1 establishes the reference and measurement
contract, with the full car run explicitly stopped by the user and retained as
incomplete. Goal 2 is complete with a limited spring-preparation cleanup and
verified unchanged trajectories. Goals 3 and 4 preserve integration settings and
verified trajectory parity. Goal 5's joint gathering is committed; Goal 6's
rigid-contact solving is committed at `479d50a2`, including the requested
tick-rate experiments. Goal 7's particle-response optimization is committed at `13ac660e`: unwrapped pit CPU falls 5.15% with unchanged tested trajectories and
settings. Goal 8, particle–rigid response, is committed at `f6146a0a`: unwrapped
pit CPU falls 4.24%, while the car-prefix CPU median is 1.81% higher.
Goal 9's [global algorithm audit](docs/physics_algorithm_audit.md) is complete
for review, following its [investigation plan](docs/physics_algorithm_audit_plan.md).
It ranks measured causes and alternatives for user selection, retaining sweep
protection and production behavior.
[Goal 10's focused investigation](docs/physics_bottleneck_investigation.md) is
committed at `43f49903`, with causal tests and ranked solution tradeoffs for all
five questions. No production optimization is adopted. The former Goals 10–16
became 11–17 at that insertion. Goal 11's [shared sweep rework](docs/physics_sweep_preparation.md)
is committed at `506b696a`: retain rigid-tree topology, prepare swept bounds once,
and reuse soft geometry within each selector call. Tested trajectories and
settings are unchanged. Streaming-active clean CPU medians improve 1.56% in
the pit and 6.97% in the car prefix, with mixed paired results; the larger early
pit saving is not established. Goal 11 is committed at `506b696a`.
[Goal 12's contact-discovery rework](docs/physics_contact_discovery.md) is committed
at `81e76ff5` against that parent: track committed pose changes, prune redundant
tree traversal and reuse unchanged beam query bounds, preserving tested state
and collision work. Three streaming-active timing pairs reduce pit CPU median
5.54%. The car median is 8.26% lower, but two of three pairs are slower, so a
consistent car gain is not established. Both worst-update timings increase;
these remain diagnostics, not rendered acceptance.
Goal 13's [quality/work investigation](docs/physics_quality_experiments.md) is
complete: eight isolated screens, three combined additions and two finalists.
At 60 Hz/four substeps/four passes, median physics CPU is 24.465 seconds per
12-second pit and 54.354 seconds per ten-second car prefix. At 30 Hz/four/four,
it is 18.160/53.256 seconds, with larger update spikes. Neither meets real time;
numerical differences and legacy CPU-comparison failures remain recorded.
Production defaults, APIs and review limits are unchanged. The user subsequently
pursued [Goal 14: cheaper particle and node collision work](docs/physics_particle_collision_plan.md),
then ended Milestone 12 in favor of Milestone 13. Goal 14 remains unfinished;
Goals 15–19 were never started. The historical car comparison found median
physics CPU of 7.178 seconds at `a8a7940a` versus 48.456 seconds at `0b254c04`
for ten simulated seconds at 60 Hz/four substeps/four solver passes. Executed
steps increased from 2,400 to 25,099 and rigid gathers from 2,400 to 225,878.
This compares each version's own example, not equivalent trajectories or
particle-only changes. The numerical failures, review limits and safety
requirements carry into Milestone 13. The C game experiment has not started.

Regression checks and relevant sanitizer checks remain mandatory throughout
physics, the C game experiment, and runtime isolation. Deferring the broader
hardening milestone does not defer these checks.

## Milestone breakdowns

Milestones 1–3 remain complete. Milestone 4 is complete **by user acceptance**,
with the exceptions recorded below; acceptance does not turn unverified checks
into passing results. Milestones 4–9 retain their five acceptance goals. For
future work, implemented foundations retain credit without claiming the new
acceptance criteria are verified. Mark a goal complete only with supporting
evidence, or an explicit user acceptance exception recorded as such.

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
5. [x] **Verify supported platforms and interaction.** Build affected generated
   applications and standalone examples against installed SDKs, execute on Linux,
   and cross-build for Windows. Launch examples outside their source directories.
   Complete interactive Linux checks for remaining composition and editing
   behavior; every applicable inventory entry must have passing evidence.
   The [guided review](docs/cross_path_feature_inventory.md#goal-5-platform-and-interactive-review)
   preserves a reproducible project and five acceptance cases.
   **Complete by user acceptance (2026-10-27), not a claim that all checks passed.**
   Goals 1–4 and corrective goals C1–C16 retain their recorded evidence. C7's
   five manual checks remain deferred and unverified; C9's additional panel
   confirmation remains skipped and unverified. Broader goal 5 cases/retests
   without recorded confirmation remain unverified exceptions to this closure.
   Preserve these checks for later review; native Windows execution was not
   performed and is not a completion gate.

#### Corrective milestone for goal 5

Resolve the issues found during interactive acceptance, make project builds
reliable, and complete the agreed directional collision-response design across
all applicable paths. These correction goals belong to milestone 4 goal 5;
they do not reopen verified goals 1–4. All C1–C16 corrections are complete.
The five interactive acceptance cases remain recorded review procedures;
milestone 4 and goal 5 are closed by user acceptance with the exceptions above.
The accepted camera-preview retest retains credit.

Any later correction requires explicit user instruction and resolution of its
remaining clarifications before implementation. The [issue mapping](docs/cross_path_feature_inventory.md#goal-5-corrective-issue-mapping)
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
10. [x] **C10 — Directional collision response.** Rename Collides With to
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
    relationship. Existing data must not be silently reinterpreted; C11 provides
    explicit migration for the immediately preceding formats.
    Agreed C10 boundary: project format 12 and runtime-state format 10 reject
    older files with a migration-required diagnostic, leaving files untouched.
    Use `.responds_to`, `collision_response_set/all_set/none_set`, directional
    `collision_response_to_check`, and either-direction
    `collision_response_between_check`; remove the old C names. Mask queries
    do not test actual overlap/contact or motion eligibility. Verify five cases:
    (1) all four mask combinations and reversed pair order across rigid bodies,
    particles and nodes; (2) beam response, endpoints and sensor overlap;
    (3) moving/rotating nonresponders, materials and Static/Kinematic restrictions;
    (4) an independent third-body relationship and contact/overlap transitions;
    (5) editor/CLI/JSON/generated/runtime parity and atomic legacy rejection.
    Complete: all five cases verified. All 121 Linux tests passed, followed by
    three focused tests after preserving the shipped car/Pong exclusions.
    Seven focused ASan/UBSan checks passed with leak detection. Nine affected
    examples independently built against the installed SDK and launched from
    outside their source directories. Windows engine/editor/CLI, response and
    persistence tests, car/Pong examples and the car regression cross-built
    without compiler warnings; native Windows execution was not performed.
    Old formats remain rejected until explicitly migrated through C11. See the
    corrective issue mapping for evidence and compatibility details.
11. [x] **C11 — Behavior-preserving mask migration.** Convert existing mutual
    filters into the new directional system by adding/splitting categories and
    rebuilding response masks as needed. Preserve every previously allowed or
    excluded pair, including collider profiles sharing a category but having
    different filters. Use deterministic remapping, retain usable named-mask
    references, and verify equivalence before publishing migrated data. Check
    available category-bit capacity; report an unrepresentable conversion
    without partially modifying the project. No permanent legacy mode. Cover
    save/reload, regeneration, supported stored-data paths, and explain effects
    on developer-authored C that uses category constants.
    Complete: explicit CLI preview/apply supports project 11 → 12 and runtime
    state/snapshot 9 → 10; other versions remain rejected. One deterministic
    mapping covers all supplied files, preserving old bits where possible and
    using profile splits or legacy bit-pair channels when needed. Every ordered
    profile pair is verified in both directions before publishing. Apply keeps
    exact originals and a named/profile mapping report, rejects stale inputs
    and occupied sidecars, and rolls back failed replacements. Developer-owned
    C remains unchanged. All 123 Linux tests, focused ASan/UBSan with leak
    detection, installed-SDK generated consumer and game-state example execution,
    and Windows CLI/test cross-builds passed. Native Windows execution was not
    performed. See [migration usage and limits](docs/collision_migration.md) and
    the corrective issue mapping for the five acceptance cases. C7/C9 review
    exceptions remain unchanged; C12 implementation and automated verification are
    recorded below.
12. [x] **C12 — Explicit kinematic authoring.** Expose Dynamic / Static /
    Kinematic for supported physical entities using the engine's existing motion
    semantics. Preserve explicit kinematic intent and mass through mode changes,
    undo/redo, project persistence, CLI, generation, and spawning. Motion remains
    independent of per-category response masks. Verify prescribed movement,
    immunity to contact impulses, and restored positive-mass dynamic response.
    Complete: rigid bodies, standalone particles, individual soft nodes, and
    applicable mixed bulk selections expose all three modes with history.
    At the user's final review request, whole-soft-body motion moved from C13
    into C12: a body edit applies to existing nodes and persists a default for
    new nodes; independent node edits show Mixed Motion. Unrelated node settings
    are preserved. Static keeps dormant explicit intent; Dynamic clears it.
    Project saves use format 13, accept 12, and retain C11's 11 → 12 migration.
    Runtime format remains 10. All 125 Linux tests passed, including pointer
    controls and the adjusted soft-mass field regression. Six ASan/UBSan suites
    passed with leak detection. Installed-SDK generated construction/spawning
    passed on Linux and cross-built for Windows, including body-level authored
    modes. COM, player-controller, and soft-body examples built independently on
    both platforms and passed Linux startup/interrupt checks. Native Windows
    execution was not performed. User review accepted C12 conditional on the
    whole-body motion addition, now implemented and verified without image
    analysis. See the corrective issue mapping. C13 and C14 completion follow.
13. [x] **C13 — Remaining soft-body Physics controls.** Expose Gravity,
    Collision, Category, Responds To, Friction, and Restitution at soft-body level.
    Motion was completed in C12. Each parent edit changes only that property on
    existing nodes and its default for new nodes; mask edits change only the
    selected bit. Independent node overrides display Mixed, empty bodies show
    defaults, and beams retain independent settings with opt-in collision.
    Complete: parent and compatible bulk controls, Enter-only material commits,
    history, CLI, persistence, generated constructors/catalogs, runtime snapshots,
    and spawning are verified. Project 13 accepts optional defaults in 12/13;
    runtime format remains 10 and C11 migration remains supported. Generated
    constructors preserve disabled collision and stored masks after material
    setters. All 127 Linux tests passed, including eight launch tests rerun
    outside the sandbox after sandbox-only failures. Seven ASan/UBSan suites
    passed with leak detection. Windows editor/CLI/test and installed-SDK fixture
    cross-builds passed without compiler warnings. Three generated examples built
    independently against both SDKs and passed Linux startup/interrupt checks;
    native Windows execution was not performed. User editor review: PASS, with
    duplication UI explicitly deferred to later work. Existing object-data
    duplication is tested. No image analysis or developer-owned main.c changes.
    See the corrective issue mapping for the five acceptance cases and logs.
14. [x] **C14 — Immediate joint clamping and spring editing aids.** Apply
    editor placement immediately after joint configuration changes and while
    dragging connected geometry. Preserve the active transform subject to axis
    projection; connected geometry follows atomically. Static affects simulation
    only and never prevents editor movement. Honor world anchors and existing
    transform constraints. Project diagonal pointer movement onto custom axis
    lines; silently refuse impossible drag steps. A blocked configuration edit
    retains its setting and reports why placement failed without moving geometry.
    Keep each gesture in one undo transaction. Springs stretch freely unless
    their saved, editor-only rest-length lock is enabled. Draw centered solid
    rest-length segments with dotted stretched extensions, or compressed springs
    with dotted full-rest-length ghosts. Runtime creation/setters remain
    non-moving; runtime spring physics and Static semantics remain unchanged.
    Complete: Angle clockwise from +Y and Shift X/Y replace direction-component
    fields; Enter/focus loss commits and immediately places geometry, while
    Escape cancels drafts. CLI, persistence, history, generated construction, and
    runtime separation pass. All 129 Linux tests and seven ASan/UBSan suites pass
    with leak detection. Windows editor/CLI/tests and installed-SDK generated
    application/scene/consumer cross-builds pass without compiler warnings. Four
    examples build independently on both platforms and pass Linux startup checks.
    Final user editor review: PASS. Native Windows execution was not performed.
    No image analysis or developer-owned main.c changes. See the corrective issue
    mapping for acceptance evidence and logs.
15. [x] **C15 — Shared editor error presentation.** Derive current errors from
    authored state and show red exterior outlines, affected/ancestor rows,
    field borders, and accordion headers, including collapsed accordions.
    Show inline reasons and clear styling when the stored error is resolved.
    Log existing errors when opening a project; unchanged errors do not repeat,
    while resolved errors that return through edits or undo are logged again.
    Retain past notifications with object/item context. Rejected input leaves
    valid stored data unmarked, and drag refusals remain silent.
    Complete: all 130 Linux tests and five focused ASan/UBSan suites pass with
    leak detection. Windows editor/CLI/tests cross-build without compiler
    warnings. The joints example builds independently against both installed
    SDKs and passes Linux startup/interrupt checks outside its source directory.
    Final user editor review: PASS, including accordion styling and startup/
    recurring error logging. No image analysis, developer-owned main.c changes,
    persistence format changes, or simulation changes. Native Windows execution
    was not performed. See the corrective issue mapping for evidence and logs.
16. [x] **C16 — Numeric property locks.** Give every editable numeric field a
    toggleable editor lock, including X, Y, and angle. Accordion-level locks
    toggle all numeric children, such as Position locking both X and Y. Locks
    constrain direct field edits and indirect changes from dragging, joint
    placement, and other editor transforms; they remain independent of Static
    simulation mode. Define persistence, mixed/bulk behavior, and history before
    implementation. Body-relative axis references (origin/COM and following
    orientation) are deferred; C14 retains the existing direction/point API.
    **Complete:** locks are project-owned metadata (format 14; reads 12/13/14),
    protect shown/local values, and participate in history and existing duplication.
    Mixed group toggles lock all first. GUI and CLI preserve locked values while
    applying independent changes; connected placement remains constrained, and
    drag restrictions are summarized once on release. Runtime APIs ignore these
    authoring locks. Temporary tool inputs and read-only displays are excluded.
    The user accepted the five manual review checks and the final symbol-size/
    bulk-opacity control review. Accordion locks cover their displayed numeric
    children, including camera dimensions, force settings, and shared font size.
    All 132 Linux tests and 114 ASan/UBSan tests pass with leak detection.
    Windows editor/CLI/tests and the installed-SDK generated application, scene,
    and probe cross-build without compiler warnings. The UI example builds
    independently against both SDKs and passes Linux startup/interrupt checks
    outside its source directory. Native Windows execution was not performed.
    Evidence and review projects are under ignored `sandbox/milestone_1/c16/`;
    see the corrective issue mapping. Goal 5 is now complete by user acceptance
    with the recorded verification exceptions above.

### 14. Frame pacing before game development

Resolve the presentation-pacing defect before starting the small C game. Include
presentation cost in the software frame budget, preserve VSync ownership, and
recover from missed frames without accumulated catch-up bursts. Keep physics
settings and simulation behavior unchanged; distinguish scheduler stalls and
over-budget work from unnecessary limiter waits.

1. [x] **Fix and verify frame pacing.** Reproduce the old timing defect, correct
   the interval boundary, and test slow presentation, missed frames, mode/limit
   changes, reporting and restart. Measure a lightweight moving scene and unchanged
   car/pit workloads with controlled before/after runs. Verify VSync and window
   switching, relevant Linux/sanitizer checks, Windows cross-builds and standalone
   SDK consumers. Obtain visual feedback. Stop for review and commit; the next
   checkpoint at completion was agreeing the C game's concept, not more physics
   optimization. The user subsequently inserted milestone 15 before that work.
   **Result: complete.** Software limiting now includes presentation in its frame
   budget; late frames start a fresh interval without catch-up bursts. Three
   complete runs per workload/version improve car from 59.029–59.124 to
   60.061–60.064 FPS and pit from 58.973–58.992 to 60.046–60.047 FPS, with unchanged
   physics work counts. The lightweight scene, VSync, deliberate stall, focus
   switching, and mode changes are verified; transient VSync-switch irregularity
   is documented. Four Linux and four sanitizer checks pass, plus the sanitized
   standalone stall workload. Windows targets and SDK consumers cross-build;
   native Windows timing is unverified. The user accepted the initial previews.
   The ten-second car remains a diagnostic, not full physics acceptance. See
   [the report](docs/frame_pacing.md) and `sandbox/frame_pacing/`.

### 15. Descriptor-based creation and automatic viewport rendering

Make editor-authored content usable through shared engine creation functions,
with viewports drawing from runtime entity state without application camera
callbacks or object-specific generated creators. Immutable descriptions produce
independent mutable instances. Viewports apply authored camera assignments using
explicit runtime bindings at creation and remain extensible from application
code afterward. Preserve current physics and frame-pacing behavior.

1. [x] **Define the API and ownership contract.** Specify entity, object, and
   viewport descriptions; creation results; access to created members; camera
   bindings; destruction; and multiple-viewport presentation. Define individual
   member dependencies and existing API migration. **Result: complete for
   review.** The [contract](docs/creation_and_viewport_contract.md) records the
   user's immutable-description, explicit-camera-instance, runtime-extension,
   creation-time camera assignment, and equal-layer discovery decisions. It
   distinguishes existing machinery from target APIs and defines ownership,
   rollback, reports, compatibility, and later verification. Documentation/source
   consistency, local links, and whitespace are checked; no runtime code changes,
   builds, sanitizer runs, or platform execution are claimed for this goal.
2. [x] **Implement shared entity and object creation.** Support whole-object
   creation and individual members such as
   `rohr_entity_create(&project.water.particle)`. Reuse existing spawning
   machinery, resolve internal and explicit external references, preserve
   independent instances, and clean up partial creation failures. **Result:
   complete for review.** Shared creators, options, and direct member lookup
   reuse the existing catalog/spawner. Strict dependency validation, explicit
   external bindings, dependency ordering, and final owner attachment preserve
   independent state and roll back failed attempts. Individual creation excludes
   separately authored children. Existing generated catalogs work now; unified
   `project.h` generation remains Goal 4. The [API guide](docs/shared_creation.md)
   records ownership and usage. Eight focused Release tests and the same eight
   ASan/UBSan tests pass, including allocation-failure rollback and 1,000 lifecycle
   cycles. Three selected installed SDK consumers pass; all consumer targets and
   independent car/flies examples build on Linux and cross-build for Windows.
   Linux consumers and examples also launch outside the source tree. The car's
   requested 600-frame diagnostic preserves 2,400 substeps, 701,387 candidates,
   and 108,585 contacts; it is not full 60-second or rendered acceptance.
   Existing joints and forces failures reproduce with both pre-change and new
   implementations and remain in the failure inventory. No compiler warnings
   or sanitizer findings were observed in the focused checks. Native Windows
   execution and generic Linux distribution packaging are not claimed here.
   Evidence and isolated installed SDKs: `sandbox/milestone_15/goal_2/`.
3. [x] **Implement automatic viewport rendering.** Reuse existing runtime
   appearance, visibility, and layer data, adding only confirmed missing
   information. Implement complete viewport creation and its runtime camera
   bindings, keep later screen/UI extension available, remove the need for
   application camera callbacks, use discovery order within equal layers, and
   present multiple viewports once per frame. **Result: complete for review.**
   One creation descriptor combines an immutable recipe with explicit runtime
   camera bindings; separate viewport creation options are removed by user
   direction. Automatic screens discover live visual state, honor visibility and
   layers, clear their backgrounds, and share one presentation per selected batch.
   Runtime extension, recipe-owned resource teardown, legacy callbacks and manual
   stages remain supported. Screen backgrounds persist through the editor, JSON
   and existing generation. Thirteen focused Release and thirteen ASan/UBSan
   checks pass, including 1,000 lifecycle cycles, allocation rollback, paused
   initialization and engine/graphics restart. Six installed Linux SDK consumers
   pass; all consumer targets and independent car/flies/viewport examples build
   on Linux and cross-build for Windows. The generated starter and examples launch
   outside the source tree and stop cleanly. The user accepted the two-screen
   preview and runtime Toggle button. The 600-frame headless car retains 2,400
   substeps, 701,387 candidates and 108,585 contacts; no full physics or rendered
   performance acceptance is claimed. No compiler warnings or sanitizer findings
   were observed in the focused checks. Native Windows execution is unverified;
   generic Linux distribution packaging and the controlled rendering comparison
   remain Goal 6. Physics settings and the known Goal 2 failure inventory remain
   unchanged. See the [guide and results](docs/automatic_viewports.md); local
   evidence is under `sandbox/milestone_15/goal_3/`.
4. [x] **Generate descriptions through a unified project interface.** Expose
   named object, entity, and viewport descriptions through a project header,
   with definitions in generated source files. Replace object-specific creation
   code with data consumed by shared engine APIs. Preserve editor properties,
   persistence, references, and deterministic generation. By explicit user
   direction, preserve named-layer identity separately from numeric draw order,
   remove generated `project_viewports.h/.c` and their helpers, and include the
   necessary starter and simulation migration here. Retain engine screen/UI/
   viewport APIs and preserve developer-edited entry points and build files.
   **Result: complete for review.** `project.h` exposes immutable named object,
   entity and viewport descriptions; definitions remain in `project_objects.c`.
   Existing object wrappers delegate to shared creation and retain their owning
   runtime root. Named layers preserve identity independently of draw order;
   camera filters remain future work. Screen clipping now survives JSON round
   trips. Generation validates names/references before writes and remains
   deterministic. Old viewport modules are removed; exact recognized starter
   and scene-build templates migrate, while custom main/build files survive and
   require the documented manual viewport migration. The starter's direct
   entities use runtime appearance and automatic rendering, and the user
   accepted the final preview. Rebuild against the matching SDK because
   `ObjectDescription` gained named-layer storage.

   Eighteen focused Release checks pass. All eighteen sanitizer checks pass
   across the initial run and an isolated scene-session rerun: the initial
   scene-session stress test overflowed its input queue, and the unchanged test
   passed after temporary-build cleanup freed disk space. This intermittent
   stress-test result remains recorded; no ASan/UBSan memory findings occurred.
   Six installed-SDK consumers and the migrated starter pass Linux execution
   and sanitizer checks and cross-build for Windows. Partial spawning, custom
   file preservation, scaffold recovery, configured paths and relocation pass.
   Release assertions were restored in three authoring tests; obsolete generated
   source expectations and a screen fixture initializer were corrected. Final
   compilation has no outstanding warnings. Native Windows execution, generic
   Linux distribution packaging and controlled rendering performance remain
   unverified here; the integrated checks remain Goal 6. Existing physics
   settings and failure inventory are unchanged. See
   [shared creation](docs/shared_creation.md); evidence is under
   `sandbox/milestone_15/goal_4/` (disposable consumer build directories may be
   regenerated by the retained verification scripts).
5. [x] **Migrate all graphical examples and review the game workflow.** By user
   direction, migrate every graphical bundled example to automatic rendering;
   keep the serialization-only example headless. Demonstrate
   shared creation and automatic viewports in the bundled examples, finish the
   workflow documentation, and review a straightforward game loop. Starter and
   simulation migration moved into Goal 4 by user direction; preserve
   developer-edited `main.c` files throughout remaining regeneration work.
   **Result: complete for review.** All twelve graphical examples use automatic
   viewports; the thirteenth, game-state serialization, remains headless. The
   authored car, player controller and center-of-mass examples use `project.h`
   and shared creation with explicit camera bindings. Their old generated
   viewport modules are removed. Persistent UI supplies buttons, labels, sliders
   and diagnostic markers; the extreme-mass diagnostic keeps finite snapshots
   without adding simulated entities. Debug overlays remain available without
   camera callbacks. The [game workflow](examples/reusable_instances.md) covers
   creation, updates, presentation, runtime extension, ownership and SDK setup.

   Eleven focused Release and eleven ASan/UBSan tests pass, including control
   interaction, automatic rendering, lifecycle and authored-scene checks. All
   thirteen examples build independently against installed Linux SDKs, launch
   outside their source directories and stop cleanly in Release and sanitizer
   checks. All thirteen cross-build against the installed Windows SDK; native
   Windows execution remains unverified. Regenerating each authored example twice
   preserves customized main/CMake files and reproduces its checked-in catalog.
   The user accepted the twelve graphical previews. The 600-frame headless car
   retains 2,400 substeps, 701,387 candidates and 108,585 contacts; this is diagnostic
   evidence, not full performance acceptance. Physics settings and existing
   physics failures remain unchanged. An existing unused helper in Pong's
   component generator warns under the extra verification warning flags; no new
   compiler warnings or sanitizer findings remain. A disk-full sanitizer build
   was rerun successfully after removing disposable prior verification builds.
   Evidence and reproducible scripts: `sandbox/milestone_15/goal_5/`; disposable
   sanitizer/Windows consumer directories and the sanitizer SDK are cleaned up
   after verification. Integrated distribution/performance checks remain Goal 6.
6. [x] **Verify SDK parity, lifecycle, performance, and game workflow.** Build
   and run generated and handwritten consumers against the installed generic
   Linux SDK, cross-build for Windows, verify independent instances and cleanup
   under sanitizers, measure rendering overhead without changing physics
   settings, and review the resulting workflow visually. Include adding screens
   and interactive UI to a created viewport from application code.
   **Result: complete for review.** Generic Linux SDK packaging and all fourteen
   consumers pass; all thirteen examples independently build and launch outside
   their sources. The relocated SDK CLI creates a fresh starter without engine
   source access, and the starter builds and runs. Portable archives now support
   PIE consumers. Twenty focused Release and twenty ASan/UBSan checks pass, plus
   fourteen sanitizer SDK consumers, including 1,000 complete generated-object/
   viewport lifecycle cycles. Updated consumers, affected tests and comparison
   tools cross-build for Windows; native Windows execution remains unverified.

   Three serial comparisons per rendering path expose and then resolve a pit
   regression from invisible outline submission. Skipping zero-alpha outlines
   reduces automatic pit drawing work from about 4.42 to 3.26 ms, versus 2.92 ms
   for callbacks. Corrected car runs remain near 60.06 FPS and all three automatic
   pit runs pass the existing real-time gate at 59.94–59.98 FPS. Lightweight and
   shared-screen multi-viewport runs, VSync, stall recovery and focus switching
   pass; mode-switch transients remain documented. Physics settings and exact
   car/pit work counts are unchanged. By explicit user approval, the obsolete
   adaptive-only forces-test assertion now allows fixed-step timing, with every
   force/torque numerical check unchanged; other known physics failures remain.
   The user accepts the generated independent scenes, rotated insets and runtime
   toggle. See the [verification report](docs/creation_rendering_verification.md)
   and `sandbox/milestone_15/goal_6/`. The SDK and evidence remain isolated from
   the user's `dist/` installation. Stop for review and commit.

Each implementation goal includes relevant focused verification before its
boundary; Goal 6 integrates those checks rather than postponing them. The
contract defines the acceptance inventory, including the existing 1,000-cycle
lifecycle criterion and controlled rendering comparisons. No implementation goal
starts automatically. The user subsequently selected milestone 16 before the C
game experiment; physics work remains paused with its failures preserved.

### 16. Scoped creation and a teaching example

Support individual entities, whole objects, standalone viewports, and complete
project instances through shared creators. Resolve authored references within the
current creation scope, leave optional external links unbound unless explicitly
supplied, and preserve required physics dependencies. Whole-project creation wires
its scene and viewports and owns its input and audio resources. Teach all three
scene-creation pathways in the generated main, with only project creation active
by default. Preserve existing physics, presentation, and developer-owned code.

1. [x] **Define creation scope, APIs, ownership, and the teaching example.**
   Specify optional versus required references, standalone viewport camera
   binding, project creation/access/destruction, failure rollback, shared-service
   boundaries, and three complete alternative example mains. **Result: complete
   for review.** The [contract](docs/scoped_creation_contract.md) records the
   user's decisions to apply global settings separately and load all authored
   sound/music resources without autoplay. It defines project-local input/audio
   lookup, generation-checked ownership, background-only unbound screens, and
   preservation of internal authored wiring. Source consistency, local links,
   and whitespace checks pass. Documentation only: runtime behavior is unchanged;
   no build, test executable, sanitizer, or platform execution is claimed.
2. [x] **Implement scope-based creation and optional viewport bindings.**
   Resolve internal entity/object references, omit optional external links when
   unbound, preserve independent settings and initial poses, and keep required
   physics-reference validation. Standalone viewports create their layout without
   requiring cameras; explicit creation-time and later runtime bindings remain
   supported. Verify omission, invalid bindings, full-object parity, independent
   instances, empty screens, and focused failure/lifecycle behavior. **Result:
   complete for review.** Optional external links remain unbound without changing
   immutable recipes, initial poses, deadlines, or required physics dependencies.
   Empty viewport screens retain backgrounds/UI; binding, clearing, and paused
   rebinding work without cached-content trails. The user accepts the preview.
   All 23 focused tests and five installed-SDK checks pass in Release and
   ASan/UBSan, including 1,000-cycle lifecycle and failure rollback coverage;
   three generated-project integration checks pass. All 14 SDK consumer targets
   and four independent examples build on Linux and cross-build for Windows.
   Linux examples launch outside the source tree in Release and sanitizer builds;
   both car runs reach the full 60-second simulation endpoint with matching work
   counts. No new compiler warnings or sanitizer findings remain. Native Windows
   execution is unverified; generic archive/workflow integration remains Goal 5.
   See the [verification record](docs/scoped_creation_contract.md#goal-2-verification)
   and `sandbox/milestone_16/goal_2/`. Physics settings and failure inventories
   remain unchanged. Stop for review and commit.
3. [x] **Implement whole-project creation and ownership.** Add
   `rohr_project_create()` and instance lookup/destruction through shared creators.
   Wire the full scene and viewports; create separate controllers/actions and load
   every authored sound/music resource without playback. Keep global settings
   explicit, validate prerequisites, handle duplicate authored names across
   instances, and verify rollback and destruction without affecting survivors.
   **Result: complete for review.** The public project description/instance API
   composes existing creators, binds cameras within its new hierarchy, loads all
   audio stopped, and copies names for project-local lookup. Generation-safe
   ownership, input restart handles, root expiry, and failed-attempt layer cleanup
   preserve survivors and global settings. All 19 focused tests and five selected
   installed-SDK consumers pass in Release and ASan/UBSan, including 24 injected
   allocation failures and 1,000 project lifecycles. Four independent examples
   build and launch outside their source trees in both Linux configurations;
   full car runs retain Goal 2's work counts. Engine/editor, affected tests, SDK
   consumers, and examples cross-build for Windows; native Windows execution
   remains unverified. The user accepts the corrected two-project preview.
   No compiler warnings or sanitizer findings remain. Concurrent user activity
   means elapsed timings are not performance evidence. See the
   [verification record](docs/scoped_creation_contract.md#goal-3-verification)
   and `sandbox/milestone_16/goal_3/`. Generated whole-project descriptions and
   teaching mains remain Goal 4. Stop for review and commit.
4. [x] **Generate the new project description and example main.** Expose the
   project recipe through `project.h`. Make whole-project creation the only active
   main; include complete commented object/entity alternatives with clear dividers,
   explicit camera binding, and cleanup. Preserve authored camera assignments,
   scene-selection behavior, deterministic generation, and developer-edited mains
   and build files. Compile and run every alternative in disposable generated
   projects and keep affected examples compatible. **Result: complete for review.**
   The immutable project recipe reuses named object/viewport descriptions and
   includes input/audio recipes. New projects use `Example` and `example_assets`;
   existing authored names and developer-owned main/build files remain intact.
   The active main uses whole-project creation, and both commented alternatives
   compile and run against the installed SDK. Canonical viewport lookup names
   remain stable across reload. Fourteen focused Release checks and six sanitizer
   checks pass, including generated two-instance resource isolation; four
   independent SDK examples build and launch in both Linux configurations.
   All three main paths, two generated consumers, engine/editor, affected tests,
   and four examples cross-build for Windows. Native Windows execution remains
   unverified. The user accepts the generated example preview. No new compiler
   warnings or sanitizer findings remain. See the
   [verification record](docs/scoped_creation_contract.md#goal-4-verification)
   and `sandbox/milestone_16/goal_4/`. Physics and frame pacing are unchanged;
   timings are not performance evidence. Stop for review and commit.
5. [x] **Verify all pathways, SDK parity, and the game workflow.** Integrate
   standalone/object/project coverage, two-instance mutation and teardown,
   1,000-cycle sanitizer checks, failure injection, and input/audio ownership.
   Verify installed Nix and generic Linux SDK consumers, external-directory
   launches, Windows cross-builds, and real Linux visual behavior with the user.
   Retain existing physics/frame-pacing settings and known failure inventories.
   **Result: complete for review.** Fresh Nix and generic Linux packages pass all
   15 installed consumers; Nix packaging now executes its tests. All 15 consumers
   also pass with ASan/UBSan and leak detection, including 1,000 cycles per object
   and project workflow and 24 injected project allocation failures. Five focused
   engine tests pass in Release and sanitizer builds, with per-cycle resource
   counts checked. Both packaged CLIs generate, compile, and run all three example
   mains outside their source directory. Four independent examples build and
   launch against both SDKs; full car runs retain the previous work counts.
   All 15 updated consumers and the internal project test cross-build for Windows;
   native Windows execution remains unverified. The user accepts the extended
   whole-project preview. No new compiler warnings or sanitizer findings remain.
   See the [verification record](docs/scoped_creation_contract.md#goal-5-verification)
   and `sandbox/milestone_16/goal_5/`. Physics and frame pacing are unchanged; no
   performance claim is made. Stop for review and commit.

Each implementation goal includes focused verification before its boundary.
Only the user may start the next goal. The user subsequently selected milestone
17 before the C game concept and completion criteria; neither game development
nor deferred physics work starts automatically.

### 17. Consistent creation APIs and report checks

Expose authored resources by name and accept authored recipes or runtime
descriptors through one creator per resource. Keep whole-project creation as a
composition of those shared creators. Add explicit ordinary and strict report
success predicates, and teach the consistent workflow in generated examples.

Approved decisions: both report predicates mean success, with top-level NOT_RUN
false; the strict predicate also rejects warnings and evidence loss. Creation
uses value syntax and C11 typed dispatch with single argument evaluation. Large
hierarchy recipes remain named values borrowed internally to preserve catalog
identity; plain runtime descriptors may use temporary values. Remove old public
pointer-style creation calls as their families migrate. Authored and runtime
inputs have the same return types and ownership; resource creators do not
implicitly create entity owners. Controller recipes include actions and roll
back together. Expose direct named fields such as project.controller_1, with
nested actions/items and explicit generation errors for identifier collisions.
Bindings stay explicit; required missing dependencies remain errors. Authored
sprites/animations use normal asset loaders or entity creation; non-owning sprite
and player value builders still require explicit loaded handles. Mathematical
factories and new physics, pacing, or world-isolation behavior are out of scope.

1. [x] **Add report success predicates.** Implement rohr_report_check and
   rohr_report_status_check with direct-engine equivalents. Validate successful
   completion, warnings, information, failures, skipped execution, malformed
   reports, truncation, overflow, and saturation. Update generated loops to
   explicitly allow valid zero-tick physics skips and retain failure diagnostics.
   **Result: complete for review.** Both predicates accept report values and
   validate successful completion; only the strict check rejects warnings and
   evidence loss. New generated mains explicitly permit valid zero-tick physics
   NOT_RUN reports. Twelve focused checks and two installed-SDK consumers pass
   in Release and ASan/UBSan with leak detection, including all three generated
   paths with pause/resume and malformed/failed report rejection. Engine/editor,
   affected tests, SDK consumers, and generated paths cross-build for Windows;
   native Windows execution remains unverified. Four private test helpers were
   renamed to avoid collisions with the new direct API, preserving their checks.
   No compiler warnings or sanitizer findings remain. See
   [verification](docs/reports.md#milestone-17-goal-1-verification) and
   `sandbox/milestone_17/goal_1/`. Physics and pacing are unchanged. Stop for review
   and commit; later creation changes remain unimplemented.
2. [x] **Establish unified creation dispatch.** Implement value-style
   entity/object/project creation and typed-adapter conventions. Preserve
   immutable recipe identity, scope resolution, rollback, and lifetime rules;
   evaluate each argument once and reject unsupported types at compile time.
   **Result: complete for review.** C11 dispatch accepts engine descriptor values
   and exact generated view types, borrowing canonical recipe addresses without
   copying hierarchies. Old public pointer spellings and function-pointer entry
   points are removed. Affected generated templates, bundled examples, callers,
   and usage guides are migrated; developer-edited mains remain preserved.
   Eleven focused tests and five installed-SDK consumers pass in Release and
   ASan/UBSan with leak detection, including 1,000-cycle ownership checks.
   Supported forms compile and all 20 invalid input cases are rejected on Linux
   and Windows. Engine/editor, consumers, generated paths, and three independent
   examples cross-build for Windows; native Windows execution is unverified.
   Linux examples run outside their source directories; the car completes its
   full simulation with unchanged work counts. No compiler warnings or sanitizer
   findings remain. See [verification](docs/shared_creation.md#milestone-17-goal-2-verification)
   and `sandbox/milestone_17/goal_2/`. Goal 2 migrated entity/object/project creation.
3. [x] **Expose named controllers and resources.** Generate named
   controller/action, audio, layer, and nested viewport-item recipes. Add
   rohr_controller_create for runtime descriptors and complete authored recipes,
   and make project creation reuse it. Reject conflicting generated names.
   **Result: complete for review.** Generated catalogs expose direct named
   resources, controller actions, and viewport screen/UI items. Controller
   creation accepts native descriptors and generated views, creates actions
   atomically, and is shared by whole-project creation. Existing instance naming
   and ownership remain intact. Normalized/reserved name conflicts fail before
   replacing generated output. Thirteen focused tests and seven installed-SDK
   consumers pass in Release and ASan/UBSan with leak detection, including
   1,000-cycle controller checks, capacity rollback, and standalone/project
   survivor checks. Supported inputs compile and 27 invalid inputs are rejected
   on Linux and Windows. Engine/editor, generated paths/resource fixtures,
   consumers, and independent examples cross-build for Windows; native Windows
   execution remains unverified. Linux examples complete functional runs with
   no physics or rendering changes. No compiler warnings or sanitizer findings
   remain. See [verification](docs/shared_creation.md#milestone-17-goal-3-verification)
   and `sandbox/milestone_17/goal_3/`. Stop for review and commit; Goals 4–6 remain
   pending, including the explicit helper-free teaching mains in Goal 5.
4. [x] **Unify remaining authored-resource creators.** Apply the shared pattern
   to viewports, screens, UI, cameras, graphics assets, layers, and audio. Keep
   resource ownership explicit and reuse the creators in larger compositions.
   **Result: complete for review.** Viewport, screen, shape/text/slider UI, camera,
   texture, animation, font, and text creation accept native values or typed
   authored recipes. Native descriptor-returning expressions remain supported;
   catalog adapters borrow canonical lvalues and evaluate inputs once. Named
   layers/audio already use native value descriptors. Resource creators introduce
   no entity owners or implicit live camera bindings. Recipe-created UI owns its
   text/font dependencies; viewport and object construction reuse the shared
   paths with rollback. Old viewport pointer calls/adapters are removed, affected
   callers and generated catalogs migrated, and manual migration documented.
   Eighteen focused tests and seven installed-SDK consumers pass in Release and
   ASan/UBSan with leak detection, including 1,000-cycle ownership and failed-UI
   rollback checks. Supported inputs compile and all 67 invalid inputs are
   rejected on Linux and Windows. Engine/editor, generated paths/resource
   fixtures, consumers, and all 13 independent examples cross-build for Windows;
   all 13 examples build and run against the installed Linux SDK. Native Windows
   execution and generic Linux packaging remain unverified. The user accepted
   the real SDK workflow preview. No compiler warnings or sanitizer findings
   remain. See [verification](docs/shared_creation.md#milestone-17-goal-4-verification)
   and `sandbox/milestone_17/goal_4/`. Stop for review and commit; Goal 5's
   helper-free teaching mains and Goal 6's integrated parity review remain next.
5. [x] **Migrate examples and generated teaching code.** Update all three
   creation pathways and repository callers. Remove obsolete creation adapters
   and pointer spellings. Generated mains must show engine calls, error/report
   checks, the frame loop, and cleanup directly inside each main, without local
   utility functions such as ok, report_ok, or game_loop or replacement wrapper
   macros. Keep all three pathways self-contained with clear comment dividers;
   only whole-project creation is active, with the other two alternatives
   commented out. Verify each pathway independently, retaining failure
   diagnostics and valid zero-tick physics handling. Preserve edited mains
   during regeneration and document required migration for external source.
   **Result: complete for review.** Each generated pathway directly contains its
   startup, checks, loop, and cleanup, with only whole-project creation active.
   The player-controller example uses named shared creation and action lookup;
   generated compatibility wrappers delegate to the same creator. Strict old
   template migration preserves developer edits. Ten focused tests and seven
   installed-SDK consumers pass in Release and ASan/UBSan with leak detection.
   Each main handles pause/resume and injected report failures with diagnostics
   and cleanup before service shutdown. Engine/editor, consumers, generated paths,
   resource fixtures, and three independent examples cross-build for Windows;
   those examples also run against the Linux SDK. All 67 unsupported-input compile
   checks remain rejected. The user accepted the generated example preview.
   No compiler warnings or sanitizer findings remain. Native Windows execution
   and generic Linux packaging remain unverified. Physics and pacing are unchanged;
   the car's existing performance gate is not reclassified. See
   [verification](docs/shared_creation.md#milestone-17-goal-5-verification) and
   `sandbox/milestone_17/goal_5/`. Stop for review and commit before Goal 6.
6. [x] **Verify SDK parity and the workflow.** Verify both input forms,
   independent instances, required bindings, rollback, survivor-safe destruction,
   deterministic generation, 1,000-cycle sanitizer checks, installed Nix/generic
   Linux SDK consumers, Windows cross-builds, and real Linux previews with the user.
   **Result: complete for review.** All 17 installed consumers pass in Release,
   ASan/UBSan with leak detection, and both freshly packaged Linux SDKs; eleven
   focused engine checks pass in Release and sanitizer builds. Supported creation
   forms compile and all 67 unsupported inputs remain rejected. Both packaged
   CLIs produce the three explicit example paths, with deterministic catalogs,
   preserved developer files, report-failure checks, and independent resources.
   The generic SDK is additionally exercised inside Rocky Linux 8; all 149 archive
   files match the tested installation. Engine/editor, consumers, generated paths,
   and affected examples cross-build for Windows; native Windows and ARM Linux
   execution remain unverified. Four independent examples build and run against
   both Linux packages. The car completes its full simulation with unchanged
   physics work counts; its existing performance gate remains unresolved.
   User review exposed missing diagonal backdrops beneath bound camera images.
   Runtime composition now matches the editor, with clipped, batched short-band
   geometry avoiding the software-renderer timeout of individual stripe lines.
   The user accepted the corrected backgrounds, rotated insets, and behavior.
   No compiler warnings or sanitizer findings remain. See
   [verification](docs/shared_creation.md#milestone-17-goal-6-verification) and
   `sandbox/milestone_17/goal_6/`. Verification packages are in that sandbox;
   normal `dist/` outputs require the usual SDK rebuild. Milestone complete;
   stop for review and commit before selecting the game concept.

Complete and review one goal at a time. After this milestone, agree the C game
concept and completion criteria with the user; no later goal starts automatically.

### 18. Explicit runtime lists and ID/name access

Authored project/object structures remain creation recipes. Game code accesses
created resources through typed IDs and runtime names. Internal ownership
remains for safe grouped cleanup. This change is separate from existing
unfinished milestones.

1. [x] **Add typed entity lists with caller-owned storage.** Add `EntityList`
   and `EntityNameList`, each with a typed data pointer, capacity, and count.
   Provide `rohr_entity_list_get(&list)` and `rohr_entity_name_list_get(&list)`
   plus direct-engine equivalents returning `EngineResult`. Use the existing
   live count to size caller storage. Getters allocate nothing; insufficient
   capacity writes no elements and reports the required count in `list.count`.
   Enumerate all live entities in discovery order without sorting. Copy names
   by value, using empty names for unnamed entities; ID/name entries correspond
   without intervening creation/deletion. Document non-ownership, stale deleted
   IDs, and name-copy independence. Verify empty/exact/insufficient capacity,
   NULL storage, unnamed entities, correspondence, rename independence, and
   deletion/slot reuse after capture.
   **Result: complete for review.** Public/direct getters copy discovery-order
   snapshots into caller-owned arrays with explicit validation/capacity errors
   and required counts, without allocation or partial writes. Copied names and
   stale-ID behavior are documented. Six focused tests and an installed-SDK
   consumer pass in Release and ASan/UBSan with leak detection; all six tests
   and the consumer cross-build for Windows. All 13 examples build independently
   against the installed Linux SDK; center-of-mass, player-controller and
   viewport examples pass headless startup/interrupt checks outside their source
   directories. No compiler warnings or sanitizer findings remain. Native
   Windows execution is unverified; Goal 3's generic SDK/generated-workflow and
   real preview acceptance gates remain pending. See
   [verification](docs/entity_ids.md#milestone-18-goal-1-verification) and
   `sandbox/milestone_18/goal_1/`. Stop for review and commit.
2. [x] **Establish runtime ID/name access and consistent naming.** Remove
   project/object prefixes from created names. Preserve the authored name when
   available, otherwise append the lowest available suffix (`box`, `box_1`,
   `box_2`); skip occupied names and permit reuse after deletion. Add typed
   global name lookup and runtime-name getters for entities, viewports,
   controllers, sound and music, with separate resource-family namespaces.
   Keep actions scoped to controller IDs and viewport items to viewport IDs;
   screens use placements, cameras use entities, assets retain typed handles,
   and named layers retain shared-name semantics. Apply this consistently to
   standalone/object/project creation; resolve authored connections with
   operation-local identity mappings, never global name guesses. Preserve
   ownership, rollback and grouped cleanup, validating IDs and skipping deleted
   resources so reused slots survive. Temporarily retain hierarchy access APIs.
   Verify duplicate/already-suffixed names, length limits, independent namespaces,
   repeated projects, instance wiring, rollback, and cleanup after slot reuse.
   **Result: complete for review.** All five resource families use copied runtime
   names, typed lookup/getters and lowest available suffixes without ownership
   prefixes. Authored connections use instance-local identity; grouped cleanup
   preserves replacement IDs. Exhausted generations retire, and hierarchy APIs
   remain temporarily available. Twelve focused tests pass in Release and
   ASan/UBSan with leak detection and cross-build for Windows. Installed-SDK
   naming/persistence and regenerated configuration/project consumers pass;
   all 13 examples compile, with three headless startup/interrupt checks.
   No compiler warnings or sanitizer findings remain. The API-inventory audit
   has exactly the committed baseline's pre-existing diagnostics; native Windows
   execution and Goal 3's preview acceptance remain unverified. See
   [verification](docs/entity_ids.md#milestone-18-goal-2-verification) and
   `sandbox/milestone_18/goal_2/`. Stop for review and commit.
3. [x] **Migrate examples and generated mains, then remove obsolete access
   APIs.** Migrate all examples and generated creation paths to typed IDs,
   runtime names and useful entity enumeration. Keep whole-project creation
   active and object/individual alternatives commented out with clear dividers.
   Keep initialization, checks, loops and cleanup directly in `main`, without
   local utility functions or replacement helper macros. Remove public
   project/member hierarchy lookup helpers after migrating callers; retain
   creation, grouped destruction and operational relationship APIs. Update
   documentation and manual migration guidance; preserve developer-edited mains.
   Compile/run all examples and all three generated paths; verify deterministic
   generation, edited-main preservation, Release, ASan/UBSan, Nix/generic Linux
   SDK consumers and Windows cross-builds. Obtain real Linux preview acceptance.
   **Result: complete for review.** Examples and generated mains retain typed
   runtime IDs and use global names/caller-owned snapshots; immutable generated
   recipes replace mutable access wrappers. Public hierarchy lookup helpers are
   removed while private ownership, wiring and grouped destruction remain.
   Starter and exact-legacy-upgrade mains retain all three explicit creation
   alternatives; customized mains are preserved with manual migration guidance.
   Release runtime/generator checks and 18 installed consumers pass on Nix and
   generic Linux. All 13 examples compile/run on Linux and cross-build for
   Windows. Focused tests, six SDK consumers, the three migrated examples and
   all generated main alternatives pass ASan/UBSan with leak detection.
   CLI/editor tools are refreshed in all verification SDKs. Deterministic
   generation, exact migration, edited-main preservation and all scene selections
   pass. The user accepted the native Linux preview. No final compiler warnings
   or sanitizer findings remain. Native Windows execution is unverified; the
   API-inventory audit retains 77 pre-existing diagnostics, with none added.
   See [migration and verification](docs/runtime_access_migration.md) and
   `sandbox/milestone_18/goal_3/`. Stop for review and commit.

Boundaries: no generic `RohrType`/`RohrList`, hidden list allocation, sorting
guarantee, or physics changes. IDs are the durable access mechanism; names may
be reused. Complete and verify only one goal, then stop for review and commit.

### 19. Physics setters that preserve unrelated values

Game code changes individual physics coordinates and fields without acquiring
snapshots or reconstructing complete configurations. Preserve existing validation,
ownership, coordinate spaces and the side effects of full-value setters. This
milestone is additive; it does not change solver behavior or simulation defaults.

1. [x] **Add axis setters for motion and positions.** Add X/Y setters for
   velocity, acceleration, gravity, world position, local center of mass,
   particle origin, joint-anchor offsets and soft-body node local positions.
   Expose both `rohr_physics_*` and direct-engine APIs. Verify preservation,
   effective defaults, invalid inputs and stale IDs, rotated local coordinates,
   particle geometry and soft-body transform/mass behavior. Document allocation
   and error semantics; verify relevant regressions, examples, sanitizers,
   installed SDK consumers and Windows cross-builds.
   **Complete:** all 16 axis setters and direct-engine equivalents are implemented.
   Eight Release regressions pass; focused ASan/UBSan checks pass with leak
   detection. Public-only consumers pass against installed Nix, sanitizer and
   generic Linux SDKs, and cross-build against Windows. Center-of-mass,
   player-controller and soft-body examples compile/run through the installed
   Release SDK and cross-build for Windows. The car completes 3,600 frames / 60
   simulated seconds with unchanged baseline substep/candidate/contact counts.
   This is functional verification, not performance acceptance. Native Windows
   execution remains unverified. No final compiler warnings or sanitizer findings;
   the API inventory retains 77 pre-existing diagnostics with none added.
   See [contracts and verification](docs/physics_axis_setters.md) and
   `sandbox/milestone_19/goal_1/`. Stop for review and commit.
2. [x] **Complete force/torque field access and physics getters.** Add force
   X/Y and complete-vector setters/getters plus individual location-offset and
   reference access. Components use the configured orientation reference;
   preserve unrelated source fields, signed magnitude semantics and lifetime.
   Torque remains scalar. By explicit user direction, include missing getters
   across physics now: full motion vectors, scalar/axis access, materials,
   collision configuration, locks, joints, particles and soft-body properties.
   New errorable scalar reads use shared `ScalarResult` with a `float` value.
   Getters copy values without allocation or mutation. Verify preservation,
   reference modes, validation/stale IDs, SDK consumers and platform parity.
   **Complete:** 31 setters and 109 getters/checks, each with a direct-engine
   and facade entry point. Eleven Release regressions pass; both new suites
   pass ASan/UBSan with leak detection. Installed Nix, sanitizer and generic
   Linux SDK consumers pass, and Windows SDK consumers cross-build.
   Player-controller and soft-body examples compile/run through the installed
   Release SDK and cross-build for Windows. The car completes 3,600 frames / 60
   simulated seconds with unchanged baseline substep/candidate/contact counts;
   its existing performance gate remains unmet and is outside this API goal.
   Native Windows execution remains unverified. Final builds have no compiler
   warnings or sanitizer findings. The API audit retains 77 pre-existing
   diagnostics with none added. See [verification](docs/physics_field_access.md#verification)
   and `sandbox/milestone_19/goal_2/`. Stop for review and commit.
3. [x] **Complete remaining partial physics setters.** Audit locks, joints,
   particles, soft bodies and collision configuration. Add missing setters for
   independently editable fields, preserving related state and automatically
   calculated rest values when another property changes. Document and verify
   the completed inventory, error behavior and SDK/platform parity.
   **Complete:** 35 setters and one anchor-entity getter, with direct-engine and
   facade equivalents. Preserve resolved joint/beam rest values, owner/lifetime,
   coordinate semantics and particle geometry relationships. Generic node filter
   edits now keep node snapshots consistent. Fourteen Release regressions pass;
   the focused suite and installed consumer pass ASan/UBSan with leak detection.
   Installed Nix and generic Linux consumers pass; Windows engine, suite,
   consumer and affected examples cross-build. Player-controller and soft-body
   examples run from outside their source directories; the car completes 3,600
   frames / 60 simulated seconds with unchanged baseline operation counts.
   Its existing performance gate remains unmet. Enabling assertions in older
   joint/frame/mass tests fixes skipped Release setup; frame/mass tests pass.
   The separate joint high-speed angular-velocity assertion still fails and
   reproduces against the unchanged Goal 2 SDK; both diagnostic scenarios abort
   early and are incomplete. This is a preserved solver failure, not a new API
   regression or solver acceptance. Native Windows execution remains unverified.
   No final compiler warnings or sanitizer findings; the API inventory retains
   77 pre-existing diagnostics with none added. See
   [verification](docs/physics_field_access.md#goal-3-verification) and
   `sandbox/milestone_19/goal_3/`. Stop for review and commit.

See [axis setter contracts](docs/physics_axis_setters.md) and
[physics field access](docs/physics_field_access.md). Complete and verify
only one goal, then stop for review and commit.

### 20. Independent animation attachments

A rigid body can have many independently playing and rendered animations,
each with its own identity, visibility, transform, layer and playback state.
Use dynamically growing storage with no separate per-body attachment cap;
entity-backed attachments share the engine's existing MAX_ENTITIES limit.
This is simultaneous rendering, not a one-active-clip state machine.

1. [x] **Runtime attachments and controls.** Add creation, individual access,
   caller-owned enumeration and safe grouped cleanup. Reuse existing Entity
   IDs and animation controls; retain the legacy single-component API. Resolve
   target poses at drawing time without changing physics. Preserve bindings in
   runtime saves. Verify independent playback/rendering, dynamic growth,
   allocation/retain rollback, stale IDs, individual deletion and slot reuse.
   **Complete:** independent entity-backed attachments reuse existing animation
   controls, preserve legacy component behavior, and resolve drawing transforms
   without physics updates. Runtime saves preserve drawing targets and lifetime
   owners separately. Ten Release regressions and the focused ASan/UBSan suite
   pass, including 256 simultaneous attachments and allocation-failure rollback.
   Installed Nix/Linux SDK consumer and game-state/view-port examples pass;
   engine, suite, consumer and examples cross-build for Windows. Native Windows
   execution and generic Linux SDK parity remain unverified in this goal.
   Final builds have no compiler warnings or sanitizer findings. The API audit
   retains 77 pre-existing diagnostics with none added. See the
   [verification record](docs/animation_attachments.md#persistence-and-verification)
   and `sandbox/milestone_20/goal_1/`. Stop for review and commit.
2. [x] **Editor and generation support.** Allow multiple attachments in editor
   controls, JSON save/load, undo/redo and generated projects. Give each authored
   animation its own runtime identity and preserve sibling controls and bindings.
   **Complete:** dynamic editor rows and independent generated Entity identities
   support many attachments. Authored member access, editor controls and runtime
   ID-based APIs select one hitbox animation driver without changing sibling
   playback or bindings. JSON migration, undo/redo, runtime persistence and
   parent visibility preserve independent settings. Sixteen Release regressions,
   three ASan/UBSan suites with leak detection, all three generated creation
   paths with 40 attachments, repeated instances, deterministic generation and
   edited-main preservation pass. Engine/editor and affected tests cross-build
   for Windows; installed Nix/Linux SDK consumers and affected examples pass.
   No final compiler warnings or sanitizer findings; the API inventory retains
   77 pre-existing diagnostics with none added. See `sandbox/milestone_20/goal_2/`
   and the [verification record](docs/animation_attachments.md#persistence-and-verification).
   Stop for review and commit.
3. [x] **Full workflow verification.** Verify examples, installed SDKs, Linux and
   Windows builds, and obtain real editor/generated-app preview acceptance.
   **Complete:** 73 selected Release regressions and all 23 installed consumers
   pass; all 13 standalone examples build/run against Nix-built and generic
   Linux SDKs. Both generated workflow suites verify all three creation paths,
   save/reload determinism and developer-file preservation, including sanitizer
   runs with leak detection. Engine/editor, consumers, examples and generated
   workflows cross-build for Windows; native Windows execution remains unverified.
   The archived generic SDK passes relocation checks. Real Linux editor controls,
   independent animation/closing and runtime hitbox switching were accepted.
   Authoring-canvas behavior is documented: it shows all visible hitbox variants.
   Release test assertions and stale generation/relocation fixtures are repaired;
   final builds have no compiler warnings or sanitizer findings. The API audit
   retains 77 existing diagnostics. See `sandbox/milestone_20/goal_3/` and the
   [verification record](docs/animation_attachments.md#persistence-and-verification).
   Stop for review and commit.

See [runtime contract and verification](docs/animation_attachments.md).
Complete and verify only one goal, then stop for review and commit.

### 21. Animation playback modes and completion

Animations can loop forever or play once and freeze after the last frame's full
interval. Mode is per-player description configuration; completion is persistent,
writable runtime state. Changing mode preserves progress and completion. Clearing
completion resumes from the current frame with a fresh interval; restart selects
frame zero. Existing zero-initialized descriptions continue looping.

1. [x] **Runtime playback, descriptions and persistence.** Add playback mode to
   player/sprite and authored object descriptions; provide public/direct entity
   and standalone-player loop, completion and restart controls. Preserve state
   through compatible description edits and runtime saves, migrate old saves to
   looping/not completed, and reject invalid values without partial mutation.
   Verify clocks, single/empty animations, manual completion, seeks, resets,
   independent attachments, stale IDs and final-frame hitbox drivers. Run focused
   Release and ASan/UBSan suites, installed Linux SDK consumers, affected standalone
   examples and Windows cross-builds. Stop for review and commit.
   Complete: twelve focused Release regressions and seven sanitizer regressions
   pass; installed Linux playback consumers, external example launches and Windows
   cross-builds pass. See the [verification record](docs/animation_playback.md#goal-1-verification).
2. [ ] **Editor and generated workflows.** Add Loop Forever/Play Once authoring,
   preview Restart, JSON migration, command/CLI support, undo/redo and copying.
   Preview uses the runtime playback rules; Play does not clear completion.
   Generate the mode through descriptions and all three creation paths, preserving
   edited mains. Verify deterministic generation, installed Nix-built/generic
   Linux SDKs, sanitizers and Windows cross-builds; obtain editor/application
   preview acceptance. Authoring continues showing all visible hitbox variants.
   Stop for review and commit.

No callbacks, event queues, cycle counters, reverse playback, state machines,
physics changes or new dependencies. See [playback contract](docs/animation_playback.md).
Complete and verify only one goal at a time.

### 13. Physics rebuild from the simpler baseline

Rebuild physics outward from the simpler `a8a7940a` execution model **inside the
current engine**, adding necessary protection and quality in measured stages.
Prioritize accurate rigid–rigid interactions and reliable particle/node blocking
against rigid bodies, especially preventing passage through thick walls. Cheaper
particle bounce, friction, settling and contact precision may be evaluated.
Preserve current public APIs, editor/project integration, ownership, entity
lifetimes, reporting and independently runnable pipeline contracts. This is a
physics rebuild, not a reset of the repository to the historical engine.

**Status: paused by user direction for frame pacing and game development;
Goal 1 complete; Goal 2's compact solver
attempt failed the tighter and rendered performance gates, with a verified
scalar improvement promoted by user direction in completed Goal 2a. Goal 2b's
standalone flies conversion is complete with user behavior acceptance. Goal 3 failed behavior review;
its tracked implementation was restored by the user. Goal 4a's bounded comparison
is complete with both prototypes rejected; Goal 4 blocking remains incomplete.
Goals 5–8 have not started.**
Goal 2 investigated baseline particle cost. By user direction, a minimal swept
static-wall guard was inserted as Goal 3 before the user reopened Goal 2.
Former Goals 3–7 moved to 4–8; their scope and evidence are preserved.
The [baseline report](docs/physics_rebuild_baseline.md) records implementation,
measurements, numerical failures and verification. Milestone 12 is closed and superseded, not completed. This
work originally isolated candidates; the user has now selected the last Goal 2
compact scalar version as this branch's default in Goal 2a. Its documented
reduced protection and quality limits remain explicit. Keep the current implementation
as a comparison control; isolate experiments and preserve the user's workspace.
Unlike the retired Goal 14 scope, this milestone explicitly permits investigating
and rebuilding integration, elastic scheduling, subdivision and contact-refresh
policy. Each goal still requires the user's instruction before it starts.

The direction is **2× physics throughput in both unchanged workloads**, followed
by rendered verification and user selection of the final quality balance.
The historical car's 7.178 CPU seconds per ten simulated seconds exceeds the
five-second headroom target; restoring its schedule alone is not a promise of
success. No comparable historical pit result at these matched settings has yet
been established. Measure every added system's cost and benefit rather than
assuming the old model is sufficiently accurate or that all old code is faster.

1. [x] **Establish the simpler baseline inside the current engine.** Pin the
   implementation parent and preserve the old/current/reference sources, SDKs,
   executables and evidence. Inventory the historical and current physics
   features and execution order; explicitly list supported, temporarily reduced
   and deferred behavior, including joint locks, springs, soft boundaries,
   beam/self contacts, force timing and interaction publication. Reproduce the
   historical bounded fixed-step/contact-gather-once schedule in an isolated
   current-engine candidate, adapting its integration as needed rather than
   carrying over modern world-wide adaptive repetition implicitly. Keep current
   API, ownership, lifetime, reporting and standalone-stage semantics. Begin at
   60 Hz, four configured substeps and four solver passes. Run the unchanged
   current authored car and 800-particle pit against the current control; retain
   the historical example as a separate behavioral reference and identify scene
   differences. Repair diagnostic clock/temporary-force handling explicitly and
   preserve prior traces. Record complete workload timings, exclusive stage and
   pair-type costs, step/gather/solve counts, numerical failures and visual
   behavior. Verify Linux, relevant ASan/UBSan, Windows cross-builds and independent
   SDK examples. Deliver a reproducible baseline, feature/error inventory and
   ranked additions for blocking restoration (now Goals 3–4), not a production rollback. Completion requires
   functioning composed/manual pipelines, mandatory safety checks and completed
   declared car/pit diagnostic endpoints; final numerical tolerances, real-time
   gates and 2× headroom are not Goal 1 completion requirements.
   **Result:** isolated `ROHR_PHYSICS_SIMPLE_BASELINE` build (OFF by default),
   with fixed configured steps and one contact discovery per step. Three-run
   median CPU time is 2.912 s for the ten-second car and 12.755 s for the
   twelve-second pit, versus 49.455 s and 25.030 s in the current control.
   All declared workload endpoints and 228 stress endpoints/calibrations
   complete; 176 stress numerical failures remain visible. Linux safety/manual
   checks, relevant ASan/UBSan checks, Windows cross-builds and independent SDK
   consumers are verified. Option-disabled trajectories match the control.
   Single-window previews complete, but rendered gates remain unmet; this is
   not production or final quality acceptance. Evidence is in
   `sandbox/milestone_13/goal_1/`. Stop for review and the user's commit before
   starting Goal 2.
2. [ ] **Investigate baseline particle cost and quality tradeoffs — failed.**
   The user approved the [compact scalar/SIMD comparison](docs/physics_compact_solver.md)
   on 2026-11-06, with a 4 ms average / 5 ms p95 target and the previously accepted
   60 Hz/four-substep/two-particle-pass comparison. Preserve the prior failed
   outcomes below. The reopened attempt is verified but fails its performance
   requirements; further work requires user direction.
   Pin
   Goal 1's candidate and the immediate parent as controls. Explain the measured
   8,033.5 ms and 14,577,780 particle-pair response invocations: separate unique
   pairs, retained constraints, repeated solver visits, effective responses and
   cost per visit, including geometry/dispatch overhead. Measure gathering,
   finalization and the rest of physics separately so savings are not merely
   shifted elsewhere. Preserve the first attempt's verified four-pass candidate
   at `bdb622c6` as the new 60 Hz/four-substep/four-pass control, recording its full
   hash and the new implementation parent when execution begins. Preserve Goal 1
   and all prior failed/rejected evidence as separate comparisons.
   Follow the detailed [seven-investigation plan](docs/physics_pit_optimization.md#rebased-goal-2-investigation-plan)
   in this order, screening every item before selecting combinations:

   1. [x] **30 Hz physics first.** Compare 30 Hz/four/four with retained
      60 Hz/four/four on both unchanged workloads; keep rendering at 60 FPS.
      Measure complete-run cost, individual tick spikes, input response and
      quality. Do not assume half the tick rate halves cost or preserves accuracy.
   2. [x] **Refresh particle polygon geometry only when needed.** Identify all
      geometry consumers and preserve current data at mixed-contact, manual-stage
      and publication boundaries; measure saved flushes and replacement work.
   3. [x] **Simplify particle solver storage and execution.** Investigate compact
      constraints, prepared invariant properties and reduced generic dispatch,
      preserving ownership, sequential response and configured effort.
   4. [x] **Reduce particle-pair solver passes.** Reverify two passes against four,
      retaining particle–rigid and rigid–rigid effort; include soft-body nodes
      with explicit car quality checks. Preserve known one-pass failures.
   5. [x] **Decouple particle collision cadence from elastic integration.** Keep
      elastic steps where needed while testing fewer particle-pair searches and
      responses; document coupling and motion coverage for rigid contacts.
   6. [x] **Reuse candidates and previous velocity impulses.** Test conservative
      neighbour/contact reuse and warm starting independently, including new
      contacts, changed normals, corrections and lifetime-safe invalidation.
   7. [x] **Sleep settled particle groups.** Measure work actually avoided,
      detection/maintenance cost and wake-up correctness; distinguish standalone
      piles from nodes coupled through beams or joints.

   Record every item's cost/quality verdict, including mechanisms rejected after
   measurement. Test isolated changes before combining measured winners. Preserve
   generation-safe
   invalidation, ownership, elapsed-time consumption, interaction publication
   and manual-stage semantics. Measure maintenance and wake-up costs; these are
   candidates to evaluate, not requirements to add every mechanism. Any reduced
   response effort or other accuracy tradeoff must be explicit and measured;
   do not reduce rigid–rigid quality to achieve the particle target.

   Retain the 0.5-unit pit particle-pair overlap review limit and report full-run
   worst values. It is a review threshold, not solver correction slack. Record
   wall penetration, escapes, missed collisions/tunneling, rebound and settling
   separately; small sampled overlap does not prove collision protection.
   Keep the existing head-on and extreme-fixture criteria visible. Compare the
   unchanged car as well, retaining its five-CPU-second ten-second diagnostic
   budget and explicitly reporting any behavioral or numerical regression.

   Target at most six physics CPU seconds for the full twelve-second pit,
   measured with three serial clean runs per version under the shared contract.
   During Goal 2, the user clarified that a near-target result is preferable to
   sacrificing substantial physics quality merely to save another second.
   Retain the six-second comparison target and report any miss explicitly;
   do not silently reduce response effort or reinterpret it as a performance pass.
   Verify normal 1280×720 rendered pit playback separately against the 60 FPS,
   p95 work, simulation-time and backlog gates below; CPU throughput alone is
   not rendered acceptance. Require relevant regressions, ASan/UBSan, Linux
   execution, Windows cross-builds and independent installed-SDK examples.
   Preserve evidence in `sandbox/milestone_13/goal_2/`, including unsuccessful
   candidates and a quality/cost table. An unmet performance target remains
   failed rather than a completed optimization; report the remaining cost and
   required tradeoffs for user direction. Stop for review and commit before
   restoring systems. Goal 8 now owns combined final quality acceptance.
   **Preserved first-attempt result — failed:** four-pass candidate reduces the
   matched three-run pit CPU median
   from 13.511 s to 7.664 s per twelve simulated seconds (43.3% less), but misses
   the six-second budget by 1.664 s. The unchanged car median is 3.105 s per ten
   simulated seconds, with sampled trajectory matching Goal 1. Pit worst pair
   overlap is 0.266482, below 0.5; the user found its preview acceptable. Rendered
   pit runs achieve 58.20–58.80 FPS, p95 work 17.10–17.98 ms and simulation/elapsed
   ratios 0.9701–0.9801, so the strict rendered gates also fail. Linux, relevant
   ASan/UBSan, Windows cross-builds and independent SDK consumers are verified;
   all 228 stress endpoints complete with the same 52 passes/176 failures.
   Production defaults remain unchanged. The [optimization report](docs/physics_pit_optimization.md)
   records retained/rejected changes, grid size/distance fallbacks and the remaining
   response/solver/geometry costs. Evidence is in `sandbox/milestone_13/goal_2/`.
   **Rebased result — failed:** all seven levers have measured verdicts. The
   proposed geometry/solver patch preserves every sampled car/pit state at
   30/60 Hz. Three-run medians at 60 Hz are 2.582 s car and 6.387 s pit, versus
   2.592/7.240 s control: 11.79% less pit CPU, still 0.387 s over budget.
   Experimental PP2 gives 2.609/5.533 s with pit overlap 0.331; whole-scene
   sleeping gives 2.756/3.970 s with overlap 0.266 but freezes slow settling.
   The user selected retaining the trajectory-preserving patch and preserving
   sleeping for later; neither experimental policy is promoted. Normal retained
   pit playback achieves 58.70–58.85 FPS,
   p95 work 14.54–14.66 ms and simulation/elapsed 0.97839–0.98077, failing the
   timing contract despite frame work fitting. Presentation pacing adds cost
   outside the nominal interval; graphics code remains unchanged. Thirty hertz
   exceeds the pit overlap limit and misses the 1,000-units/s head-on collision.
   Linux/default/sanitizer checks and Windows/SDK builds are recorded; all 228
   original stress endpoints complete with 52 passes/176 failures. Separate
   four/four stress batches hit the wall cap and remain explicitly incomplete.
   See [the complete results](docs/physics_pit_optimization.md#rebased-investigation-results)
   and `sandbox/milestone_13/goal_2/rebased/`. Initial previews were insufficient
   to judge; the later 60 Hz/two-pass car and pit previews were both accepted.
   Their single rendered runs measured 59.11/58.60 FPS and 6.83/12.61 ms frame-work
   p95. This is behavior acceptance of the next comparison configuration, not
   final quality or performance acceptance. The failed Goal 2 outcome is due to
   measured performance gates, not missing user feedback.
   Stop for review and commit; do not advance automatically to restoration.
   Report the now-approved 4 ms average / 5 ms p95 full-game target separately;
   it supplements the existing gates. Added blocking/structural work must be
   costed with protection enabled, not assumed to restore accuracy for free.

   **Compact-solver result — failed:** opt-in scalar preparation retains contact
   order, checked state changes, manual-stage semantics and production defaults.
   Three clean pairs against a native two-pass control reduce pit CPU from
   5.953452 to 4.393571 s (26.20%); car CPU is 2.676039 versus 2.684137 s, with
   no consistent saving. Candidate average/p95 is 6.102/8.225 ms pit and
   4.474/5.751 ms car, failing 4/5 ms while meeting the old 6/5-second CPU gates.
   The earlier retained-control comparison gives 15.62% less pit CPU and a small,
   inconsistent 3.69% car median gain; keep the two measurement blocks separate.
   All 720 pit and 600 car sampled states match the control. Pit worst pair
   overlap remains 0.330803; existing car deformation and blocking errors remain.
   Ordered SIMD has no measured gain; colored SIMD reaches 0.794605 pit overlap
   and is rejected. Its scalar arithmetic control reproduces that error.
   Production-default regressions pass 25/25; experimental Linux/ASan suites
   retain the same five baseline failures. All 228 original stress endpoints
   complete with unchanged 52 passes/176 failures and non-timing results.
   Sanitized car/pit endpoints, Windows cross-builds and independent SDK example
   builds/launch smoke are verified. Three complete rendered runs per workload/
   version remain below strict 60 FPS/time gates; one interrupted car run is
   preserved separately. The user accepted behavior. See the
   [complete compact-solver report](docs/physics_compact_solver.md) and
   `sandbox/milestone_13/goal_2/compact/`. No positive 4/5 ms protection budget is
   established; mixed wall response and gathering are the next measured cost
   candidates. Stop for review; do not advance automatically.

   1. [x] **Goal 2a: Promote the accepted compact scalar version to the branch default.**
      User requested the exact version from the last failed Goal 2 attempt, including
      the compact scalar solver. Enable fixed-step/gather-once physics and compact
      scalar response by default, with two particle passes and 60 Hz/four-substep/
      four-other-pass defaults. Align normal car/pit entry points and scripted force
      lifetime with the reviewed configuration. Keep explicit runtime overrides,
      manual-stage semantics, legacy build comparisons and historical evidence.
      Completion means verified adoption and behavior parity, not passing Goal 2's
      unmet numerical or performance targets. Verify relevant Linux/ASan/UBSan,
      Windows cross-builds and independent SDK examples; stop for review and commit.
      **Result: complete.** CMake defaults/presets and the current Linux build use
      the selected version. Normal and diagnostic car/pit traces match all prior
      states and work counts. Seven Linux adoption checks and three sanitized
      solver/default/force-lifetime checks pass. Linux/sanitizer broader suites
      pass 21/26; existing accuracy limits remain visible, including two beam
      rebound errors reproduced in the preserved Goal 2 engine at four substeps.
      Linux editor/examples and Windows engine/SDK examples build without warnings.
      See [promotion verification](docs/physics_compact_solver.md#promotion-verification).

   2. [x] **Goal 2b: Convert the small flies to standalone particles.**
      Approved by the user after diagnosing the flies-in-pit startup slowdown.
      Convert all 499 small flies, retaining their radius, material, visuals,
      intentionally overlapping circle placement and 120 Hz example schedule.
      Keep the large fly as a rotating rigid body with its existing controls.
      Verify loaded particle behavior, the complete 12-second physics comparison,
      a rendered preview, relevant regression/sanitizer checks, and independent
      SDK example builds. This scene correction does not complete Goal 2's car/pit
      performance gates or restore Goal 3's rejected guard. Stop for review.
      **Result: complete.** All 499 flies load as nonrotating standalone particles;
      the initial 4,491 overlapping pairs and all other authored data are preserved.
      Three serial twelve-second comparisons reduce median physics CPU from
      12.925861 to 6.279297 seconds (51.42%). All six endpoints complete without
      physics diagnostics. The normal recording/debug preview completes at
      43.47 FPS and is accepted by the user; sustained 60 FPS is not claimed.
      Three focused Linux checks and two ASan/UBSan checks pass; Windows regression
      and independent SDK example cross-builds pass. The normal Linux executable's
      staged assets are updated. Evidence is in `sandbox/flies_in_pit_diagnostic/`;
      see the [report](docs/physics_compact_solver.md#flies-in-pit-standalone-particle-correction-goal-2b).

3. [ ] **Add and measure a minimal swept static-wall guard — failed.** The user
   rejected its behavior and restored its tracked changes. Preserve its evidence
   and three untracked implementation/test files; further implementation needs
   a new user decision. The original scope follows the
   [implementation and measurement plan](docs/physics_swept_wall_guard.md).
   Compare guard-disabled/enabled candidates at 60 Hz, four configured substeps,
   two particle–particle passes and four rigid-contact passes, without sleeping.
   Guard circular particles and soft-body nodes against static solid rigid walls
   along integration and solver-correction movement. Query conservative swept
   bounds, stop at the earliest impact and remove inward velocity. Advance all
   simulation time while accepting bounded lost travel/sticking rather than
   tracing the remaining bounce/slide path. Do not restore world-wide adaptive
   steps or repeated global gathering. Moving/rotating obstacles and rigid–rigid
   sweeps stay in Goal 4. Verify thin/thick and angled walls, high speeds, delayed
   updates, corners, embedding, filters, lifetime safety and manual composition.
   Measure exclusive added work and complete unchanged car/pit endpoints, with
   three serial clean and rendered runs per workload/version under the shared
   contract. Preserve penetration/tunneling and performance failures explicitly;
   require verified in-scope blocking and relevant safety/platform checks.
   Record previews and the cost/quality decision. Keep production defaults and
   public APIs unchanged; store evidence in `sandbox/milestone_13/goal_3/`.
   Stop for review and commit before broader restoration.
4. [ ] **Restore reliable rigid and particle/node blocking.** Starting from the
   baseline's observed failures, add only the protection needed for rigid–rigid
   impacts, stacks and mass ratios, plus particle/node contacts with static,
   moving and rotating rigid bodies. Cover high-speed, oblique, thick/thin-wall,
   initially embedded and multiple-impact cases. Response must consume remaining
   time without carrying bodies through a detected obstacle. Keep rigid quality
   separate from cheaper particle response. Evaluate local handling and
   conservative risk tests before making every interaction repeat the world.
   Measure each addition independently; retain unmet numerical criteria visibly.
   1. [x] **Goal 4a: Compare predictive contacts and selective sweeps.**
      Freeze the current branch default as the control, classify integration
      versus correction-created crossings, and implement two isolated prototypes
      using unchanged collision geometry. Prioritize particle/node versus rigid
      and rigid versus rigid interactions, including moving and rotating obstacles.
      Measure particle pairs separately. Compare bounce, sliding, deformation,
      crossings and average/p95 CPU on focused cases and unchanged car/pit scenes;
      retain initial-overlap flies as an additional diagnostic. Complete bounded
      endpoints, relevant safety/platform checks and a reproducible recommendation.
      Existing numerical and 4/5 ms performance gates remain visible. Prototype
      rejection is a valid comparison result, not successful protection. Only
      promising candidates proceed to rendered review; no default promotion is
      included. Stop for user review before any follow-up implementation.
      **Result: comparison complete; neither prototype adopted.** All 18 focused
      endpoints complete in each mode on Linux and under ASan/UBSan. Sweeps pass
      14/18 versus 4/18 for both control and prediction, preserving translating
      impact bounce/sliding but failing correction-created and rotating-obstacle
      crossings. Both candidates worsen car beam error. Three serial runs per
      workload/mode complete; sweeps add 16.22% pit and 51.20% flies CPU with
      essentially unchanged total car CPU. Neither satisfies quality and 4/5 ms
      budgets together. Six focused checks pass in all three modes on Linux and
      under sanitizers; sanitized candidate workloads complete without memory
      findings. Windows tools/SDK consumers cross-build; native Windows execution
      is unverified. No candidate warranted rendered review. Production defaults
      remain unchanged. See the [report](docs/physics_collision_experiments.md)
      and `sandbox/milestone_13/goal_4a/`. Follow-up needs user direction.
5. [ ] **Restore soft-body and structural quality in measured stages.** Cover
   spring/beam integration and damping, pin/weld joints, transform locks, node
   support, beam–rigid and beam–beam contacts and self-collision. Use the baseline
   inventory to retain working systems and add missing protections separately.
   Measure strain, seam penetration, energy, constraint error, tunneling and car
   behavior. Separate elastic stability needs from collision discovery cadence;
   do not let a local stability restriction silently imply a full-world collision
   pass. Keep explicit dependencies and fallbacks where bodies are coupled.
6. [ ] **Make subdivision and contact refresh selective.** Use the preceding
   failure and cost evidence to define conservative triggers and the entities or
   interactions affected. Evaluate collision-risk subdivision, bounded candidate
   reuse, correction-created contacts, affected-beam refresh and solve cadence.
   Compare full-world fallback with local work, including dependency propagation,
   filter/shape changes and generation-safe deletion/reuse. Preserve elapsed time,
   publication and standalone-stage behavior. Record false negatives, avoided
   work and maintenance overhead; retain simpler execution when it is better.
7. [ ] **Optimize retained systems and select candidate settings.** Rank actual
   bottlenecks in the composed result, reduce unnecessary operations first, then
   evaluate data layout, batching/SIMD or caches where measurements justify them.
   Tune tick rate (at most 60 Hz), configured substeps and independent response
   effort without changing scenes or materials. Record existing API overrides
   separately from internal experiments; any public API redesign requires its
   own decision. Test isolated changes before combinations. Produce repeatable
   savings in both workloads toward 2× headroom, with an explicit quality/cost
   table and full safety/platform verification. Do not hide a failed target by
   changing the benchmark or selecting a shorter endpoint.
8. [ ] **Verify the combined result and obtain final quality acceptance.** Run
   controlled car/pit comparisons, the relevant Milestone 6 scenarios and
   regressions, rendered previews and motion/window-switch review. Require
   normal 1280×720 playback at sustained 60 FPS, p95 frame work at most 16.67 ms
   excluding pacing, simulation within 1% of elapsed time and no accumulating
   backlog. Verify 2× physics headroom separately from rendering. Present
   performance, protection and accuracy tradeoffs against the historical model,
   immediate parent and frozen reference for explicit user selection. Implement
   and verify the final selected balance before changing Milestone 6's status.
   Goal 2a's earlier branch-default adoption does not complete this final gate. Any unmet gate remains failed/incomplete; numerical
   tolerance changes require explicit user acceptance.

#### Shared measurement and quality contract

- Preserve three distinct pins: historical execution-model reference
  `a8a7940a6e6d039c80aed20c6f6b2624169e1ee3`, current engine comparison checkpoint
  `0b254c045b806fb13c022cd7470f063a03a130db` (plus the exact immediate parent for
  each goal), and frozen accuracy reference
  `6f283268aa4bf038c4ed78271baeb9d5986bd7d9`. Exclude `main`. The new goal's parent
  may include this documentation commit; record it rather than assuming its hash.
- Keep evidence under `sandbox/milestone_13/goal_<n>/`. Preserve all Milestone
  11/12 evidence in place, including rejected and incomplete trials and the
  [historical comparison](docs/physics_particle_collision_plan.md#historical-car-diagnostic-checkpoint).
  Record source/build/SDK/executable hashes, settings and commands. Do not reuse
  interrupted or faulty-harness timings as accepted results.
- Use the unchanged current car (200 particles) and full 12-second pit (800
  particles, diameter 5, restitution 0.5). Ten-second car prefixes are declared
  diagnostics, not the inherited full 60-second car acceptance sequence. Keep
  no-jump historical comparisons separate from jump-enabled workload runs;
  correctly advance lifetime-owned inputs and record diagnostic corrections.
  Do not change body counts, geometry, masses, materials or input sequences to
  manufacture performance. The 2× diagnostic budgets are five physics CPU
  seconds per ten-second car prefix and six per full twelve-second pit; full
  workload acceptance must also be measured, not inferred from those prefixes.
- Each test execution remains capped at one minute of wall time including
  shutdown. Preserve incomplete endpoints, do not rerun the same known-slow
  candidate, and never silently shorten a required scenario. The inherited
  full 60-second rendered car requirement is not waived: if it cannot complete
  within the cap, obtain an explicit endpoint/cap decision before claiming
  final acceptance. Shorter staged diagnostics can still proceed.
- Final comparisons use three serial runs per workload/version on AC power,
  streaming closed and no concurrent builds. Record changed conditions as
  exploratory evidence. Separate clean CPU, exclusive profiled wall time,
  rendered work, pacing, intentional slow motion and debug drawing. Measure
  calls and successful work, candidate/contact counts and actual subdivisions;
  nested time must not be counted twice. Open visual trials one at a time, with
  no automatic sequence of replacement windows after user interruption.
- For every addition, record milliseconds/operations added, failures fixed,
  remaining failures and supported causes. Keep numerical pass/fail separate
  from endpoint completion, and accumulate full-run worst errors to the endpoint.
  Invalid/nonfinite state, fatal errors, inability to advance and interruption
  make a run incomplete. Ownership, memory safety, lifecycle correctness and
  truthful reporting are mandatory even in deliberately rough candidates.
- Retain 0.5-unit particle-pair review for the pit and 1,000-units/s head-on case,
  and 0.1-unit penetration review in the Milestone 6 extreme fixtures. These are
  review thresholds, not solver correction slack; no generic particle–wall pit
  threshold is invented. Preserve separate tunneling, support, energy, rebound
  and joint-error checks. Numerical failures may remain in intermediate candidates
  with values, thresholds, excess and explanation; they do not become passes.
- Continue relevant Milestone 6 Goals 1–5 and focused regressions. Preserve
  fixed high-rate stress schedules as diagnostics even though new runtime
  candidates remain at 60 Hz or below. Require relevant sanitizers, Linux
  execution, Windows cross-builds and independent installed-SDK examples;
  distinguish cross-build success from native Windows execution. Stop after
  each goal for user review and commit; do not advance automatically. Refine
  later goals from measured findings without silently expanding their scope.

### 12. Real-time physics pipeline rework

**Closed by user direction; superseded by Milestone 13, not complete.** Goals
1–13 remain complete. Goal 14 ended unfinished, with its prototypes, measurements
and historical-car comparison preserved; Goals 15–19 were not started. The
following plan and status details are historical evidence, not authorization to
resume or advance this milestone. No real-time or final quality acceptance was
achieved, and no experimental candidate is adopted by closing the milestone.

Rework physics on `physics-pipelines`, using **`physics-pipelines-overoptimized`
at `6f283268`** (full commit `6f283268aa4bf038c4ed78271baeb9d5986bd7d9`) as the
frozen reference. **Exclude `main` from comparisons.** First achieve real-time
performance in the unchanged soft-body car and 800-particle pit. Then assess
accuracy and behavior against the reference and choose an acceptable middle
ground. This was the highest unfinished priority and superseded milestone 11,
whose failed attempts, experiments, executables, and evidence remain preserved.

**Status: goals 1–13 complete; Goal 8 is committed at `f6146a0a`. Goal 9's
investigation uses parent `ab3c634b`; Goal 10 against parent `f9abe365` is
committed at `43f49903`. Goal 11 is committed at `506b696a`; Goal 12 is committed
at `81e76ff5` against that parent. Goal 13's quality/work investigation is complete
against parent `db1d52cb`, committed at `ca874c2e`; it adopted no production
change. Goal 14 is superseded unfinished; Goals 15–19 were not started and are
retired scope under this milestone.**
The user now limits runtime candidates and new workload experiments to **60 Hz
physics and below**. Higher-rate evidence remains historical; production defaults
are unchanged during the audit. The requested isolated comparison uses 60 Hz,
four configured substeps and eight solver passes, with particle–particle sweep
restrictions disabled versus retained. Other adaptive limits and the agreed
penetration-review limits remain in force.
After the first comparison, the user explicitly selected **keeping existing
particle–particle sweep protection** and continuing the five-question audit.
The no-sweep version remains rejected diagnostic evidence.
Goal 7 is committed at `13ac660e`.
Goal 6 is committed at `479d50a2`. The original goals 7–11 are now goals 15–19;
completed goal numbers and evidence directories remain unchanged. Goal 2's
[force-preparation report](docs/physics_force_preparation.md) records the limited
spring scan/COM-offset cleanup and the decision to retain the other force passes.
Fourteen Linux and fourteen ASan/UBSan regressions pass, Windows cross-builds
pass, and all 219 timed scenarios plus four calibrations finish with numerical
and state/work trace parity against the frozen reference. The car's ten-second
segment and full headless pit also retain exact traces. The final soft-body
CPU comparison still fails at 397.578 ms versus 50 ms. No tolerance, API,
ownership, tick rate, scene or material changes. Explicit SIMD was deferred in Goal 2;
compiler-vectorization findings and collision-batching candidates are recorded.
The user requested no broader force optimization.

Goal 3's [integration/subdivision report](docs/physics_integration_subdivision.md)
records compact temporary sweep geometry and reused axis projections, retaining
all integration passes, duration rules and accuracy safeguards. Fourteen Linux
and fourteen sanitizer regressions pass, Windows cross-builds pass, and all 219
timed scenarios plus four calibrations, the ten-second car segment and full pit
retain exact reference traces. Three paired sweep microbenchmarks show 21–37%
less kernel cost; the car's sweep stage is 19.6% lower in one diagnostic pair,
with only 1.6% lower total time. These battery/headless timings do not establish
rendered acceptance. Lower-substep alternatives increase penetration and are
rejected in accordance with the user's preference; 240 Hz, four substeps and
eight iterations remain unchanged. The abandoned 60 Hz car probe is explicitly
incomplete, with its trace preserved. The soft-body delayed CPU comparison still
fails at 372.958 ms versus 50 ms. Goal 3 evidence remains under
`sandbox/milestone_12/goal_3/`.

Goal 4's [contact-gathering report](docs/physics_contact_gathering.md) records
duplicate refit-candidate suppression, direct scalar SAT projection, reused
circle geometry and ordered four-pair particle batches. Particle SIMD is retained
with a scalar fallback: its isolated kernel is 2.5–3 times faster, but matched
pit updates show only 0.10% less rigid-gather time and 0.03% less physics time,
within measurement noise. Three whole-pit trials per mode also do not establish
an overall SIMD speedup. Polygon SIMD packing loses on common small shapes and
is not selected. Paired kernels show 8–45% less polygon-overlap cost and 14.6%
less duplicate-heavy gathering cost. These battery/headless diagnostics do not
establish rendered acceptance. Fifteen Linux SIMD, fifteen scalar-fallback and
fifteen sanitizer regressions pass; Windows cross-builds pass. All 219 timed
scenarios plus four calibrations, the ten-second car prefix and full pit preserve
exact state/contact/subdivision traces and numerical records against the frozen
reference. Candidate counts intentionally decrease. The soft-body delayed CPU
comparison still fails at 368.877 ms versus 50 ms. Settings, APIs and tolerances
remain unchanged. Evidence is under
`sandbox/milestone_12/goal_4/`.

Goal 5's [joint-gathering report](docs/physics_joint_gathering.md) retains the
identity-only scan: collection costs only 0.038% of car physics time and 0.0069%
of pit time in the diagnostic runs. Gathered entries now retain generation-checked
handles, preventing a deleted joint's slot replacement from being solved before
it is gathered. Current anchor/type/motion edits remain visible to surviving
joints; no topology or geometry cache is added. Sixteen Linux and sixteen
ASan/UBSan regressions and Windows cross-builds pass. All 219 timed scenarios
plus four calibrations finish. Numerical records and state/work traces for the
ten-second car prefix and full pit match the parent and frozen reference under the existing
comparison rules. The soft-body delayed CPU comparison still fails at 382.453 ms
versus 50 ms. No speedup or rendered acceptance is claimed; settings, ownership,
public APIs and tolerances remain unchanged. Evidence is under
`sandbox/milestone_12/goal_5/`.

Goal 6's [rigid-contact solving report](docs/physics_rigid_contact_solving.md)
records pose-checked circle/COM reuse and generation checks before retained
contact response/publication. Three serial pit trials per version show 14.6%
less physics CPU time at unchanged settings. Seventeen Linux and seventeen
ASan/UBSan regressions, Windows cross-builds and independent installed-SDK
examples pass. All 219 timed scenarios plus four calibrations, the ten-second
car prefix and full pit preserve the existing numerical/state/work comparisons.
The soft-body delayed CPU comparison still fails at 364.860 ms versus 50 ms.
The user considered the two-particle-pass candidate, then clarified that
"0.5 slack" means a penetration-review threshold, not increased solver
correction slack. The 0.5/1.0 solver-slack variants remain historical experiments
based on the agent's misinterpretation, not approved runtime configurations.
Corrected full-pit trials show 44.8% less CPU time versus the instrumented
eight-pass control. Reducing configured subdivisions from four to one gives
roughly 50 updates/s headlessly, but p95 physics work is 28.382 ms, particle
penetration reaches 0.580482 and floor penetration 0.086014. No rendered gate
or global subdivision change is accepted. Further requested 120/60 Hz trials
(two/one ticks per nominal frame, one subdivision) complete twelve seconds:
p95 physics CPU is 16.546/14.406 ms, worst particle overlap 1.041389/1.617722,
and floor penetration 0.350550/0.982507. Both exceed the proposed particle
overlap limits; neither establishes rendered acceptance or selects a global
tick rate. Exclusive profiles identify gathering,
sweeps and response as the remaining dominant costs. A further owned-field
copying experiment offers no established benefit and is not selected. Production
settings, public APIs and ownership remain unchanged; evidence is under
`sandbox/milestone_12/goal_6/`.

**User-directed priority insertion:** optimize the four expensive collision
categories before joint solving and the remaining pipeline work. The detailed
[collision breakdown](docs/physics_rigid_contact_solving.md#collision-breakdown-and-follow-up-priorities)
records exclusive milliseconds over the same twelve simulated seconds, not
per-frame milliseconds. At 240/120/60 Hz, particle–particle response costs
3,379/2,271/1,409 ms, particle–rigid response 1,903/1,155/699 ms, shared sweep
preparation/traversal 2,664/1,405/830 ms, and shared gathering broadphase/filtering
3,849/2,185/1,336 ms. These single instrumented trials identify priorities;
1,000 ms over the run is not a new per-frame budget or acceptance threshold.
Review particle–particle pit penetration against 0.5 units. The user separately
sets all Milestone 6 extreme-fixture penetration checks to 0.1 units for now.
These are review thresholds only; solver correction settings remain unchanged.
The timings above used experimental solver slack and must be remeasured with
the original correction settings before selecting subsequent optimizations.

Goal 1's
[measurement contract](docs/physics_measurement_contract.md) records reference
provenance, measured exclusive stage costs, baseline errors and the comparison
format. All 219 timed fixture scenarios plus four calibrations finish, with
matching profiled/unprofiled state and work traces. Ten Linux and ten ASan/UBSan
tests pass; installed-SDK consumers build on Linux and cross-build for Windows.
The user explicitly stopped the full car baseline at 15.417 of 60 simulated
seconds; it remains incomplete, not a full-run bound. The full headless pit
finishes; six rendered pit windows closed before their endpoints and remain
incomplete, with observed frame-work p95 29.505–209.506 ms, above 16.67 ms.
The user directed stopping further trials and retaining this evidence. Complete
rendered coverage and uncapped headroom remain unverified. The
soft-body delayed-update CPU comparison still fails at 376.743 ms versus 50 ms.
Pursue one goal at a time only on explicit user instruction, stopping after verification
for review and the user's commit before advancing. Keep executables and evidence
under `sandbox/milestone_12/goal_<n>/` without replacing earlier evidence.

**User-directed algorithm audit insertion:** Goal 9 follows completed Goal 8
before further subsystem optimization. That insertion moved the then-existing
Goals 9–15 to 10–16;
the subsequent Goal 10 insertion moved those remaining goals to 11–17.
The later quality/work experiment insertion moved the then-unstarted Goals 13–17
to 14–18 and made the new Goal 13 next; that investigation is now complete.
Completed goals and their evidence paths retain their numbers. The
initial experiment excluded particle–particle speed from adaptive timestep
restrictions while retaining particle–rigid and rigid–rigid protection, subject
to the existing penetration and no-tunneling checks. During the audit, the user
clarified that high-speed protection must remain, requested the isolated 60 Hz
comparison, and then selected retaining existing sweep protection. The
investigation does not change production stepping behavior. The user specified
an investigation-only goal: explain each problem's current benefits and costs,
compare alternatives and tradeoffs, and propose future goals for later selection.

Each pipeline goal includes analysis, a measured rework where justified,
verification, and a reviewable commit. A pipeline may be retained when
measurements show rewriting it offers no worthwhile benefit; document that
finding.
Goals 9 and 10 are explicit exceptions to adopting a measured rework: their
deliverables are investigation reports and evaluated solutions, with prototypes
isolated from production and implementation decisions reserved for the user.

Goal 7's [particle-response report](docs/physics_particle_response.md) rebaselines
with original solver correction settings and selects compact, atomic velocity
commits with diagnostic handles resolved only on rejection. Retained one-point
rebound targets avoid redundant calculation. Three serial unwrapped pit trials
per version reduce median physics CPU from 62097.934 to 58902.988 ms (5.15%).
Exclusive particle response falls 7.45% in the repeated default-setting profile.
All 219 stress scenarios plus four calibrations and five head-on schedules pass
numerically on Linux and ASan/UBSan; eighteen regressions per platform, Windows
cross-builds and installed-SDK examples pass. Default pit, car-prefix and fixture
traces preserve parent/reference parity under the existing comparison rules.
Head-on overlap remains 0.000500202179. Original-correction two-pass pit probes
reach 0.189830 overlap at 240 Hz/one subdivision, but 120/60 Hz reach
0.556209/0.803069 and fail the retained 0.5 review limit. No tuning is adopted.
The ten-second car prefix remains incomplete relative to sixty seconds, no car
speedup is claimed, and the soft-body delayed CPU comparison still fails at
373.020 ms versus 50 ms. Real-time rendered acceptance remains open. Evidence is
under `sandbox/milestone_12/goal_7/`.

Goal 8's [particle–rigid response report](docs/physics_particle_rigid_response.md)
selects exact supporting-face reuse, compact mixed-contact velocity commits and
retained rebound targets. Solver settings and measured trajectories remain
unchanged. Three paired unwrapped pit trials reduce median total physics CPU
61514.739 → 58904.753 ms (4.24%); the instrumented car-prefix median increases
105561.547 → 107467.918 ms (1.81%), so no car speedup is established. The
one-subdivision/two-particle-pass pit profile reduces mixed response 9.42%.
Twenty-two Linux and twenty-two sanitizer regressions, all 219 timed scenarios
plus four calibrations, five head-on schedules, Windows cross-builds and
independent installed-SDK examples pass their relevant checks. The historical
soft-body delayed CPU comparison still fails at 367.545 ms versus 50 ms.
The extra car classification finds 6,866,172 moving-polygon overlap-only visits
(3569.629 ms), versus 3,629 responding visits (8.202 ms): the earlier mixed timer
included overlap refresh. Goal 9 distinguishes these paths.
The 120/60 Hz two-pass pit diagnostics still exceed 0.5 particle overlap; no
runtime tuning is adopted. Full car and rendered real-time acceptance remain
open. Interrupted/battery trials are preserved and excluded from controlled
comparisons. Evidence is under `sandbox/milestone_12/goal_8/`.

Goal 9's [decision report](docs/physics_algorithm_audit.md) ranks the questions
by likely runtime contribution: repeated discovery, adaptive restrictions,
unnecessary response/overlap work, unrelated world work, then candidate/data
costs. It adds explicit hypotheses and disproof tests. Three clean serial
60/4/8 trials give median physics CPU 34465.826 ms over twelve pit seconds and
68498.759 ms over the ten-second car prefix; p95 is 93.306 / 200.970 ms.
Both remain far from real time. The audit measures eight iterations on every
step, a speed-independent car elastic cap of 0.804 ms, and 1,438 restrictions
from one soft pair in the worst car update. Repeated rigid/soft discovery and
particle response are major downstream costs. Existing sweep protection is
retained; blanket exclusion and the narrower existing-contact-rule removal are
rejected by high-speed crossing evidence. Twenty-two new Linux and twenty-two
sanitizer observer regressions pass; exact-source Goal 8 stress/platform evidence
is reused and 222,314 historical reference updates are rechecked. All selected
runs, diagnostic limitations and nonadditive work estimates are preserved under
`sandbox/milestone_12/goal_9/`. No production optimization, tolerance change or
new implementation ordering was adopted by the audit itself. The user has
since selected investigation Goal 10 before the remaining implementation goals.
Its [completed report](docs/physics_bottleneck_investigation.md) follows the
[ordered plan](docs/physics_bottleneck_investigation_plan.md). It identifies
repeated unchanged-pose/beam discovery, a near-collapsed tire beam's conservative
sweep/response mismatch, useful later response passes, limited pit isolation,
and traversal/layout opportunities. Eight observer workloads preserve exact
state/work traces; all four observer SDKs pass 22 sanitizer regressions each.
Snapshot comparisons favor testing retained topology and compact particle data,
without establishing a whole-update speedup. Streaming-active diagnostics from
Q2 onward are separated from the controlled baseline. Evidence is preserved
under `sandbox/milestone_12/goal_10/`; no solution is adopted. Former Goals 10–16
became 11–17 at that insertion. Goal 11's [completed sweep report](docs/physics_sweep_preparation.md)
records five measured alternatives. The selected implementation retains only
rigid-tree topology across calls, refits current swept bounds, and reuses soft
geometry within each call. Bounds calculations halve, with unchanged tested
state/work traces and subdivision counts. Final detailed rigid-sweep probes
improve 10.22% in the pit and 18.18% in the car; clean total CPU medians improve
1.56% / 6.97%, with substantial streaming-active variation and mixed paired
results. No consistent large whole-workload gain or rendered acceptance is
claimed. All 23 Linux and 23 sanitizer regressions, 219 timed scenarios plus
four calibrations per platform, Windows cross-builds and installed-SDK example
checks pass; existing soft-body CPU-budget failures remain visible. Evidence
is preserved under `sandbox/milestone_12/goal_11/`, including the approved
roadmap additions that were uncommitted at its parent `43f49903`.
Goal 12's [contact-discovery report](docs/physics_contact_discovery.md) records
seven candidates, including the selected combination. Exact pose worklists,
duplicate/unchanged subtree pruning and reusable beam query bounds preserve
all tested trajectories and collision work. Pit CPU improves in all three
streaming-active pairs (median reduction 5.54%); car timing is inconclusive
despite an 8.26% lower median, with two pairs slower. Worst update times increase
in both workloads. All 23 Linux and 23 sanitizer regressions, 219 stress endpoints
plus four calibrations per platform, 1,000 cache-lifetime cycles per platform,
Windows cross-builds and independent SDK checks pass. Penetration limits and
existing CPU-comparison failures remain visible. Full particle/polygon geometry
separation was investigated and remains cross-consumer work under Goal 17;
no benefit for that unimplemented design is claimed. Evidence is preserved
under `sandbox/milestone_12/goal_12/`. Goal 12 is committed at `81e76ff5`; Goal 13
is complete against parent `db1d52cb`, with its separate experiment record preserved.

**User-directed quality/work experiment insertion:** Goal 13 follows
the [eight-lever experiment plan](docs/physics_quality_experiments_plan.md).
All eight experiments and a combined comparison belong to one goal with one
review/commit boundary. Numerical prototypes are permitted within this planned
scope; production adoption and tolerance changes are separate decisions. Former
Goals 13–17 move to 14–18; completed goals, archived reports and evidence paths
retain their historical identities. The user subsequently authorized execution
of the detailed plan. The existing 0.5/0.1 review limits remain unchanged.

**User-directed particle/node collision insertion:** the new
[Goal 14 plan](docs/physics_particle_collision_plan.md) applies cheaper collision
processing to both standalone particles and soft-body nodes. Reliable
particle–rigid blocking and rigid–rigid accuracy take priority over particle
response precision. Former Goals 14–18 move to 15–19. This insertion does not
start implementation, accept new tolerances or change the force/structural model.
Elastic stepping that repeats world-wide work is reserved for a separate
discussion, without authorizing an elastic scheduling or integrator rewrite.
The user's 2× physics-throughput target is tracked separately from the unchanged
rendered acceptance gates; no unmeasured speedup is promised.

1. [x] **Establish the reference and measurement contract.** Pin the reference
   commit, preserve its executables and evidence, and establish repeatable
   car/pit runs. Measure each stage's exclusive cost, call counts, subdivisions,
   iterations, candidates and contacts without double-counting nested work.
   Record baseline physics errors and define the comparison report used by
   subsequent goals.
2. [x] **Analyze and rework force preparation.** Cover acceleration clearing,
   gravity, external loads, spring/beam forces and soft-body frame updates.
   Identify repeated calculations and unnecessary scans. Preserve force
   ownership, application timing and standalone pipeline behavior.
3. [x] **Analyze and rework integration and subdivision.** Cover rigid, joint
   and soft-body integration, swept collision checks, and adaptive step selection.
   Investigate why individual interactions cause repeated world-wide work.
   Evaluate timestep and subdivision settings alongside algorithm changes;
   consume all elapsed simulation time.
4. [x] **Analyze and rework contact gathering.** Cover broadphase construction
   and update, candidate filtering, narrowphase geometry, rigid contacts,
   particle contacts, soft boundaries and beam pairs. Reduce repeated discovery
   and geometry work while measuring missed contacts, penetration and tunneling.
5. [x] **Analyze and rework joint gathering.** Cover anchor resolution, pin/weld
   constraint assembly, validation and reusable joint data. Separate data that
   changes with topology from data that changes with motion, preserving
   entity-lifetime safety and manual gathering semantics.
6. [x] **Analyze and rework rigid-contact solving.** Cover particle and polygon
   response, block solving, position correction, friction, restitution and
   retained contact state. Evaluate simpler execution and data layouts,
   iteration requirements and convergence behavior. Measure effects on mass
   ratios, impacts, stacks and dense particle piles.
7. [x] **Optimize particle–particle response.** Profile executed response and
   skipped-pass overhead separately. Evaluate repeated calculations, data access,
   contact-state reuse and safe batching while preserving sequential effects on
   shared entities, friction, restitution and lifetime safety. Use the selected
   two-particle-pass idea for evaluation while retaining the original solver
   correction settings. Re-establish the baseline after the clarified review
   policy; the 0.5-unit review threshold is not a solver input. Measure full-pit behavior and relevant
   impact, mass-ratio and pile regressions, plus car effects. Select a change
   only with repeatable exclusive and total physics savings under the shared
   verification contract.
   Include the dedicated head-on regression: two diameter-5 particles moving
   at 1,000 units/s each, restitution 0.5, no gravity, and measured overlap
   at most 0.5 units at every completed substep. Cover normal and delayed
   updates across impact; reject pass-through and missed collision response.
8. [x] **Optimize particle–rigid response.** Separately measure particle–wall
   and particle–moving-body response, including geometry/manifold work, offsets,
   impulses and position correction. Reduce repeated work without applying the
   particle-only pass reduction to mixed contacts or increase solver correction
   slack. Apply the provisional 0.1-unit extreme-fixture penetration review
   threshold, preserving friction/restitution and rigid
   rotation. Verify impacts, floors, thin barriers, mass ratios and car contacts;
   record exclusive and end-to-end savings with all elapsed time consumed.
9. [x] **Audit global physics algorithms and adaptive stepping.** Follow the
   [ordered investigation plan](docs/physics_algorithm_audit_plan.md): freeze
   the baseline, instrument causes, measure workload/scaling costs, explain
   each problem and alternatives, run isolated experiments, then deliver a
   ranked decision report. Cover repeated contact discovery, adaptive limits,
   unrelated bodies repeating work, unnecessary contact/overlap work, and
   candidate growth/data access. For each, document evidence, current benefits
   and drawbacks, alternatives including retaining the design, tradeoffs,
   experimental results, and a recommendation with proposed future acceptance
   criteria. Distinguish shared root causes from symptoms; do not double-count
   potential savings. Test more selective particle–particle adaptive triggers
   while retaining high-speed crossing and particle–rigid/rigid–rigid protection, without
   implicitly changing ticks, configured subdivisions, passes or correction.
   Preserve the 0.5 particle and 0.1 extreme-fixture review limits, high-speed
   head-on protection, elapsed time, ownership and APIs. Keep failed experiments
   visible. Compare with the pinned parent and frozen reference under the
   measurement contract. **Investigation only:** do not adopt prototypes,
   create/reorder implementation goals or start them. Present candidate goals
   and their relationship to the then-planned Goals 11–17 for the user's later selection.
10. [x] **Investigate the ranked bottleneck questions and potential solutions.**
    Follow the [Goal 10 plan](docs/physics_bottleneck_investigation_plan.md), in
    order: repeated contact discovery; adaptive restrictions; unnecessary
    response/overlap work; unrelated bodies repeating work; candidate growth
    and data access. Extend Goal 9 with a focused causal test for each question,
    including the worst car pair's repeated restrictions, per-pass convergence,
    dynamic dependency groups and data-access evidence. Compare retaining the
    current design with concrete solutions, using isolated prototypes where
    needed. Record benefits, drawbacks, whole-update cost, full-run errors,
    confidence, dependencies and acceptance checks; distinguish measured gains
    from hypotheses and avoid adding overlapping savings. Retain sweep
    protection, candidates at 60 Hz or below, the 0.5/0.1 review limits and
    existing APIs/ownership. Deliver an evidence-backed solution recommendation
    mapped to the then-planned Goals 11–17 for user selection. **Investigation only:** preserve
    evidence under `sandbox/milestone_12/goal_10/`, keep production unchanged,
    and stop for review before adopting solutions or starting another goal.
11. [x] **Optimize shared sweep preparation and traversal.** Cover repeated
   entity scans, bounds preparation, swept-tree construction/update, tree queries
   and eligibility filtering. Evaluate conservative reuse and invalidation
   across adaptive subdivisions. Keep pair geometry costs separately measured;
   preserve moving-body coverage, earliest-contact protection and complete
   elapsed-time consumption. Measure missed contacts, tunneling and full-run
   penetration against the retained limits, alongside exclusive and total cost.
   Investigate separating reusable shape information from motion-dependent
   bounds, sharing per-body motion preparation across candidate checks, and
   retaining tree topology across subdivisions. Define validity against pose,
   velocity, acceleration and proposed-duration changes; current-pose bounds
   and swept bounds must retain their distinct meanings. Apply the shared
   recomputation investigation requirements below.
12. [x] **Optimize shared contact-gathering broadphase and filtering.** Cover
    tree build/refit, query traversal, duplicate checks, pair filtering and
    particle queueing. Evaluate reuse and compact data without stale entities,
    missed pairs or changed standalone-stage semantics. Keep narrowphase and
    publication costs distinguishable from shared discovery work. Verify
    unchanged collision coverage, retained penetration limits and contact
    lifetime safety; measure candidate work, exclusive savings and total physics
    cost in the unchanged pit and car.
    Investigate updating particle centers/radii independently of transformed
    polygon vertices, producing detailed geometry only for consumers that need
    it. Evaluate conservative bounds reused until motion or correction leaves
    their envelope, precise pose versus velocity-only invalidation, and affected
    collider/beam worklists. Reusing bounds must still test current collision
    positions and discover correction-created pairs; do not freeze hitboxes
    through substeps. Apply the shared recomputation requirements below.
13. [x] **Experiment with physics work and quality.** Follow the
    [experiment plan](docs/physics_quality_experiments_plan.md). Complete all eight
    ordered sub-goals within this single goal:

    1. [x] Configured substeps: 4 / 2 / 1 at 60 Hz, retaining adaptive protection.
    2. [x] Particle–particle passes: 8 / 4 / 2, other constraints at eight.
    3. [x] Global solver passes: 8 / 4 / 2, independently of particle-only changes.
    4. [x] Physics tick rate: 60 / 30 / 20 Hz, comparing equal simulated durations.
    5. [x] Contact rediscovery: every pass, every second pass, initial/final boundaries.
    6. [x] Adaptive collision restrictions: selective closing-motion/feature-size
       triggers by pair type, including beam restrictions and high-speed protection.
       Include the user's stricter-trigger experiment: require conservative
       collision-risk evidence before subdivision, retaining uncertain-case
       fallback and existing penetration review limits.
       Allowance, beam/floor and stricter-trigger screening is complete; no
       useful both-workload saving is established. Retain the original policy.
    7. [x] Soft-body integration: retain small elastic steps while testing collision
       and constraint refresh every one / two / four steps with explicit fallbacks.
       Both reduced schedules add stress penetration failures without an
       established car speedup. Retain original cadence for combinations.
    8. [x] Convergence and sleeping: separately test residual-based early exit and
       sleeping/waking thresholds, including maintenance cost and disturbed piles.
       No pit rounds or sleeping work are removed; retain original policies.
       All eight isolated screens, bounded combinations and final
       sanitizer/platform verification are complete.

    Screen every remaining lever, then shortlist at most the top three for
    combination testing, prioritizing savings in both workloads and labeling
    workload-specific or incomplete evidence. Other combinations wait for the
    user's later decision. Combine shortlisted candidates and report measured cost,
    actual work removed, full-run errors and remaining real-time gaps. Inventory
    existing public controls and propose missing game-specific overrides, including
    safe change boundaries and cache invalidation; defaults must remain overridable.
    Distinguish benchmark settings from shipped defaults. New public APIs and
    production defaults require separate selection. Keep scenes,
    inputs, ownership, public APIs and 0.5/0.1 review limits unchanged; consume all
    elapsed time. Test all levers, preserving rejected and incomplete results;
    numerical failures do not terminate an otherwise valid experiment early.
    Use the full pit and authorized 10-second car prefix, with relevant stress,
    sanitizer, Linux, Windows cross-build and installed-SDK checks. Repeat clean
    baseline/finalist timings three times per workload/version on AC with streaming
    closed and no concurrent builds. Headless/prefix results are diagnostic.
    Preserve evidence under `sandbox/milestone_12/goal_13/`. One final report and
    review/commit boundary; no automatic production adoption, tolerance change,
    rendered-acceptance claim or advancement to Goal 14.
    Maintain a ranked register of additional experimental factors from these
    results and source inspection, with one proposed future experiment goal.
    Do not add dedicated experiments for those factors in Goal 13. Use the
    approved staged search, capped at sixteen combinations and three finalists.
    The user now caps individual test runs at one minute of wall time. Preserve
    capped runs as incomplete, skip repeats of the same slow candidate, and keep
    the requested simulated endpoints unchanged. The one-substep car was stopped
    by the user at 7.45/10 seconds; its clean repeat was skipped.
    **Completed:** the [report](docs/physics_quality_experiments.md) records three
    combined additions, three clean repetitions per workload for both finalists,
    exclusive attribution, API transitions and a ranked future-factor proposal.
    Recommend 60 Hz/four substeps/four passes as the balanced candidate; retain
    30 Hz/four/four as a pit-focused option for later user selection. Median
    physics costs are 24.465/54.354 seconds and 18.160/53.256 seconds respectively
    for the 12-second pit/ten-second car prefix. Numerical finalist checks pass;
    two delayed soft-body CPU comparisons still fail. All 24 sanitizer regression
    batches pass (391 test invocations); Windows cross-builds and independent SDK
    checks pass, with capped supplementary prefixes explicitly incomplete.
    No rendered acceptance, Milestone 6 completion, default/API change, tolerance
    change, future-goal insertion or advancement to Goal 14 occurs here.
14. [ ] **Reduce particle and node collision work while preserving rigid protection — superseded, unfinished.**
    Follow the [ordered plan](docs/physics_particle_collision_plan.md). Apply the
    cheaper collision policy to standalone particles and soft-body nodes, retaining
    spring/beam forces and structural constraints. Reuse conservative movement
    candidates with explicit invalidation and current contact geometry; preserve
    reliable particle–rigid crossing detection and response for static and moving
    bodies. Evaluate local particle impact handling where feasible without
    changing elastic integration or scheduling. Separate particle response budgets
    from rigid–rigid quality, and reduce particle–particle discovery/response cadence
    across standalone/node combinations while respecting filters. Treat reduced
    particle–particle continuous protection as an explicit experiment, retaining
    the head-on test and recording any failure; do not silently adopt it.
    Measure rougher bounce, friction, settling, overlap and node deformation against
    unchanged review criteria. Keep rigid–rigid protection and quality, public APIs,
    ownership, elapsed time and standalone-stage semantics intact. Preserve beam
    collision and self-collision coverage; no rigid proxy or elastic-model redesign.
    Compare isolated changes and the measured combination in the unchanged pit/car,
    with exclusive work counts, clean timings, crossing checks, lifecycle/sanitizer,
    Linux, Windows cross-build and independent SDK verification. Record evidence
    under `sandbox/milestone_12/goal_14/`; obey the one-minute per-run wall cap and
    preserve incomplete endpoints. Report progress toward 2× physics throughput
    separately from rendered acceptance, without assuming that this goal alone
    closes the car's global-stepping gap. Stop for review and the user's commit.
15. [ ] **Analyze and rework joint solving.** Cover pin/weld response, spring
   coupling and transform locks. Measure constraint error, energy behavior and
   cost under the existing high-speed, high-rotation fixtures. Preserve
   documented ownership and lock semantics.
   Investigate separating connected-entity/local-anchor/parameter data from
   current world anchors and solver state. Measure repeated anchor transforms
   and mass calculations before adding caches; retain motion-dependent terms
   and refresh on parameter, mass/inertia, origin, topology and lifetime changes.
   Reuse existing Goal 5 data where valid. Apply the shared recomputation
   requirements below; retain the design if savings do not justify maintenance.
16. [ ] **Analyze and rework soft-body constraint solving.** Cover node–rigid,
   beam–rigid and beam–beam response, self-collision and contact refresh.
   Evaluate how rigid corrections and soft contacts interact. Measure
   deformation, seam penetration, tunneling, energy and car behavior.
   Investigate node-to-beam dependencies, cheap conservative beam bounds before
   detailed geometry, and geometry reuse across sweep, gathering and response
   only where their pose/shape assumptions agree. Refresh affected beams and
   contacts, including unchanged beams approached by moved rigid bodies.
   Investigate Goal 10's collapsed-beam sweep/response mismatch with conservative
   fallbacks and retained node/beam collision coverage. Apply the shared
   recomputation requirements below.
17. [ ] **Analyze and rework pipeline orchestration and finalization.** Integrate
   the revised stages. Review their execution order, shared caches, invalidation,
   repeated passes, interaction publication and reporting overhead. Verify
   standalone stages and complete updates remain correctly composed.
   Investigate shared-cache ownership and per-consumer validity, deduplicating
   refresh requests, deferring unused derived geometry until needed, and avoiding
   redundant intermediate interaction work. One consumer must not clear another
   consumer's pending invalidation. Preserve callback-visible state, reporting,
   overlap/contact publication and manual-stage semantics. Apply the shared
   recomputation requirements below.
18. [ ] **Meet the combined real-time gates.** Resolve remaining integration
    bottlenecks and select measured runtime settings. Verify both the car and
    particle pit meet the performance contract below. Produce visual previews
    and obtain motion/window-switch review. Numerical deviations remain
    explicitly recorded.
    Reconcile the existing car benchmark's jump-force lifetime handling before
    final motion/playback acceptance; preserve the historical fixture traces.
    See the [Goal 3 clock-ownership finding](docs/physics_integration_subdivision.md#clock-ownership-follow-up).
19. [ ] **Compare with the reference and select the middle ground.** Compare
    the real-time implementation against the frozen reference across all physics
    scenarios. Present accuracy, behavior and performance tradeoffs for user
    selection. Implement the selected balance, retain real-time performance,
    and verify the agreed final tolerances before updating milestone 6's status.

#### Recompute only when needed: investigation requirements for Goals 11–12 and 14–17

These user-approved additions define investigations within Goals 11–12 and 14–17;
they do not start a goal, reorder implementation, adopt a cache or authorize a
numerical approximation. Check existing reuse before proposing another layer.
Goal 13 separately permits isolated numerical experiments under its linked plan;
it does not automatically adopt changes in these implementation goals. Goal 14
permits cheaper particle/node collision experiments under its own linked plan;
the preservation requirements below apply to reuse, not a demand for identical
particle trajectories in explicitly identified numerical experiments.
For each opportunity:

- Identify which data depends on topology, shape/parameters, pose or motion,
  which consumers need it, and the exact invalidation and consumption boundaries.
  Preserve current geometry/response where required; investigate cheaper
  boolean overlap checks or conservative bounds when a caller does not need
  detailed geometry. This does not authorize a public API redesign.
- Count refresh requests, actual recomputations, successful reuse and reasons
  for invalidation. Measure exclusive cost and complete-update time, including
  lookup, deduplication, storage, validation, conversion and fallback costs.
  Distinguish avoided scans from avoided geometry work; do not add overlapping
  savings or assume fewer calculations guarantee a useful speedup.
- Verify corrections and high-speed motion, bound expansion/fallback, shape,
  radius, origin, mass and filter edits as applicable, entity deletion/reuse,
  and independently invoked stages. Conservative envelopes must trigger refresh
  before reuse becomes invalid. Keep error detection and lifetime safety intact.
- Compare current and proposed paths under the existing measurement contract,
  with full-run behavior checks and relevant platform/sanitizer verification.
  Document a measured no-change decision when maintenance outweighs savings.

Particle geometry separation, precise invalidation and affected-beam refresh
are priority hypotheses within their assigned goals, not established speedups.
Preserve the 0.5 particle-pair and 0.1 extreme-fixture review limits, existing
sweep protection and current settings unless a separate measured settings
experiment is explicitly identified. Goal 14 defines such particle/node
experiments, preserving rigid–rigid quality and particle–rigid crossing protection.
No new numerical tolerance or failed configuration is accepted by this insertion.

#### Performance and verification contract

- **Car:** unchanged authored scene, terrain, materials and 200 particles;
  existing 60-second input sequence: idle until 2 s, drive right until 30 s,
  then left until 60 s, requesting grounded jumps at 5 and 35 s.
- **Pit:** unchanged 800-particle fixture, diameter 5 and restitution 0.5;
  existing 12-second drop-and-pile sequence. Preserve the remaining fixture
  geometry, masses, materials and initial layout recorded under milestone 11.
- **Both:** normal rendered playback at 1280×720, sustained 60 FPS, p95 frame
  work ≤16.67 ms excluding pacing, simulated time within 1% of elapsed time,
  and no accumulating backlog. Report uncapped throughput/headroom separately.
- Final measurements use three serial runs per workload and version on AC
  power, with streaming closed and no concurrent builds. Headless results are
  diagnostics, not rendered-FPS acceptance.
- Continue running milestone 6 goals 1–5 and the relevant regressions.
  Scenarios finish at their declared time or state endpoint and report full-run
  worst measurements despite numerical failures. Report numerical pass/fail
  separately from endpoint completion.
- Invalid/nonfinite state, fatal errors or inability to advance time produce
  incomplete results; interruption also remains incomplete. Memory safety,
  ownership, lifecycle correctness and truthful reporting remain mandatory.
- Require relevant sanitizer checks, Linux execution, Windows cross-builds and
  independent installed-SDK examples. Measure debug drawing and intentional
  slow motion separately.

#### Tradeoff policy

- Physics tick rate, configured substeps and solver iterations may be tuned.
  Record every configuration; do not alter scene geometry, body counts or
  materials to manufacture performance.
- **Clarified penetration review policy:** flag particle–particle penetration
  in the diameter-5 pit only when the measured full-run maximum exceeds
  **0.5 units**. Set the Milestone 6 extreme-drop, thin-barrier, stack, joint
  and soft-body fixture penetration checks to **0.1 units for now**, including
  their particle–rigid contacts. A value equal to the threshold passes the
  penetration check; unrelated failures still fail. These thresholds do not
  change solver correction slack, contact detection, geometry or dynamics.
  The new two-particle head-on regression uses the explicitly requested
  0.5-unit particle-pair threshold, rather than the 0.1 extreme-fixture override.
  No general particle–wall limit for the pit is invented by this change.
  Keep tunneling, nonfinite-state, support, energy, rebound and joint-error
  requirements separate and unchanged. Report numerical maxima even below
  the thresholds, and preserve original criteria/results as historical evidence.
  The 0.5/1.0 solver-slack experiments arose from a misinterpretation and are
  not approved configurations. Rebaseline new goals with original correction
  settings. Do not round overshoots into a pass.
  The earlier 0.501142-unit result and the larger reduced-subdivision/tick-rate
  errors remain recorded failures of a strict 0.5 bound. A selected result for
  these goals must respect the review thresholds; faster failing
  experiments remain diagnostics, not accepted configurations.
- Compare each new optimization with its immediate parent at matched settings
  and retain the frozen-reference comparison. Record exclusive response,
  sweep and gathering costs by pair type plus shared overhead, full-run error
  maxima, work counts, total physics time and p95 update work. Use new workload
  candidates at 60 Hz or below with explicit configurations; retain earlier
  240/120 Hz diagnostics and fixed stress schedules as historical evidence. Retain existing failures
  and require repeatable savings before adopting an optimization.
- Existing numerical tolerances and extreme-fixture CPU budgets become
  **comparison criteria during rework**, not mandatory intermediate completion
  gates. Keep their failures visible; do not silently redefine a pass. This
  policy supersedes milestone 11's fixed-settings and intermediate acceptance
  requirements, while preserving their historical evidence and milestone 6's
  recorded status.
- Public APIs and ownership contracts remain unchanged. Any necessary API
  redesign requires a separate decision.
- Final tolerance changes require explicit user acceptance in goal 19. Prefer
  the most accurate accepted configuration that still meets both real-time
  gates. Update milestone 6's status only after the selected balance and agreed
  final tolerances are verified.

### 11. Soft-body correctness and real-time performance

**Superseded by milestone 12; not complete. Evidence preserved.** The following
scope, settings, acceptance requirements, branch instructions and dated results
are historical records, not active directions. Milestone 12 replaces this work
with pipeline-by-pipeline rework and its performance/tradeoff contract. Preserve
all existing executables, experiment archives and measurements under
`sandbox/milestone_11/goal_1/`; failed or incomplete runs remain labelled as such.

Historical scope:

Identify and fix excessive soft-body simulation cost and contact penetration,
using milestone 6 goal 5 and the authored soft-body car as acceptance workloads.
Complete this milestone before resuming milestone 6 completion.

1. [ ] **Diagnose and resolve soft-body contact and performance failures — superseded, not complete.**
   Compare pre-goal-5 commit `b00baf6e`, checkpoint `63fccb26`, and the corrected
   implementation with identical Release settings and installed SDKs. Keep the
   car, terrain, 200 particles, 240 Hz ticks, and four configured substeps.
   The repeatable 60-second input sequence idles until 2 s, drives right until
   30 s, then left until 60 s, requesting grounded jumps at 5 and 35 s.
   Measure physics CPU, frame/render work, simulated time, completed substeps,
   collision candidates and solver work. Separate debug drawing and intentional
   collision slow motion from normal-speed acceptance. At 1280×720 require
   sustained 60 FPS, p95 frame work ≤16.67 ms excluding pacing, simulated time
   within 1% of elapsed wall time, and no accumulated catch-up backlog.
   Retain all three serial AC-powered repetitions per version with streaming
   applications closed and no concurrent builds. All 48 milestone 6 goal 5
   cases must meet their unchanged correctness and timing limits; preserve
   goals 1–4 and strengthen swept-contact coverage where necessary. Profile
   before choosing fixes; do not reduce geometry, collision coverage, or elapsed
   time to improve timings. Public APIs and ownership remain unchanged.
   Require focused regressions, ASan/UBSan, Linux execution, Windows cross-builds,
   independently built installed-SDK examples, and visual/window-switch review
   of the car and goal 5 preview. Executables and evidence belong under
   `sandbox/milestone_11/goal_1/`. Milestone 6 goal 5 remains incomplete until
   its remaining gates pass. Do not advance to the C game automatically.

   **Verification attempt (2026-10-30): failed performance requirements.** Rigid
   correction could introduce a beam overlap after contact gathering. Refreshing
   contacts during the solve fixes the observed penetration failures while
   preserving accumulated impulses and endpoint filtering. Solver-local tree
   refits, exact-pose geometry reuse, conservative swept bounds, compact beam
   candidates and allocation-free SAT projection loops reduce repeated work.
   The tight contact-travel bound remains: larger-step experiments violated
   penetration and COM/origin invariance and were discarded.

   All 48 existing goal 5 numerical checks finish successfully; peak penetration
   is 0.002126 units (limit 0.02). Exploratory normal 60 Hz p95 is at most
   1.392 ms, but the worst delayed update is 2878.504 ms (limit 50). A short
   rendered car probe ended early at 112 frames: physics CPU p95 517.376 ms,
   render work p95 2.163 ms and frame work p95 520.130 ms. It advanced 1.867
   simulated seconds in 24.589 elapsed seconds and is explicitly incomplete.
   These are exploratory observations with streaming not confirmed closed,
   not the required controlled three-version, three-repetition acceptance.

   All 24 focused regressions, 13 ASan/UBSan checks and goals 1–4 numerical
   regressions pass. Linux execution, Windows cross-builds, five independently
   built examples against Linux/sanitizer/Windows SDKs, and normal/delayed
   Linux/sanitizer player-floor probes pass. Native Windows execution was not
   performed. No final compiler warnings or sanitizer findings. The scripted
   car benchmark and usage are in `examples/soft-body/`; local evidence is in
   `sandbox/milestone_11/goal_1/verification.txt`. Full 60-second comparisons,
   debug/slow-motion measurements, strengthened swept-surface verification and
   the dedicated goal 5 preview/visual acceptance remained unfinished at that
   attempt. Review
   the evidence from this failed attempt. The user subsequently directed continued
   work on this same goal; its acceptance criteria remain unchanged.

   **Resumed verification (2026-10-30): incomplete at supersession.** Acceleration-aware
   collision bounds and conservative moving-beam side bounds reduce excessive
   substeps without selecting particular examples or fixtures. Exact-pose
   geometry reuse now spans a complete synchronous substep. All 48 existing
   goal 5 numerical checks pass, with peak penetration 0.003006 units; the
   worst exploratory delayed update remains 422.616 ms (limit 50).
   Twenty-six focused regressions, fifteen ASan/UBSan tests, goals 1–4 numerical
   fixtures, and Linux/Windows installed-SDK consumer verification pass.
   The dedicated goal 5 recorded preview passes Linux and sanitizer smoke
   checks; the user accepted its motion and window switching. Replay acceptance
   does not establish real-time performance. The car's real-time requirements,
   controlled comparisons, car visual review and strengthened swept-surface
   verification remain open. Current evidence is under the milestone 11 sandbox.

   **User-directed diagnostic within this goal:** isolate rigid collision work
   with 800 standalone particles, each diameter 5, mass 1, 16 rigid vertices,
   friction 0.5 and restitution 0.5. Use gravity 9.8, 240 Hz ticks, four configured
   substeps and eight solver iterations. No car, soft-body nodes, beams, areas,
   or joints. Drop a staggered 40-column, 20-row grid with 5.5-unit spacing and
   30-unit initial bottom clearance into a 230-unit-wide rigid pit. Record
   per-frame CPU, integration/gather/solve wall time, completed substeps and
   candidate/contact counts over 12 simulated seconds; provide a live view.
   Smaller particle counts are diagnostic scaling comparisons, not substitutes
   for the requested 800-particle run. This does not create another milestone
   or relax the existing acceptance requirements.
   The user accepted the particle-only preview as the requested fixture. Its
   first full 12-second Release run completed in 178.130 seconds, with physics
   CPU p95 356.767 ms. Constraint solving accounts for about 68% of measured
   stage wall time. No invalid state or escaped particle was detected; this
   diagnostic does not establish independent swept-contact acceptance. Linux,
   sanitizer and Windows SDK builds pass, as do 800-particle graphics smoke
   checks and a smaller sanitizer impact run. Timings remain exploratory.
   See `sandbox/milestone_11/goal_1/particle_pit_controls.txt` and its CSV/logs.
   At the user's request, the same pit was also built against `a8a7940a`, the
   parent of the commit introducing the ten-milestone roadmap (`27e49645`).
   A fresh unmodified historical Release SDK and a shared API-adapted harness
   completed all 12 seconds. Matching four individual 240 Hz ticks per frame,
   historical/current total physics CPU was 130.191/174.507 seconds and p95 was
   283.560/408.848 ms. Historical batching combined four ticks into one update
   and ran only four configured subdivisions per frame instead of sixteen:
   42.506 seconds total CPU, p95 84.698 ms, with larger sampled overlaps.
   These serial exploratory comparisons do not replace controlled acceptance
   measurements or prove the cause of the regression. Source provenance,
   adapters and results are retained in the milestone 11 sandbox.

   **User-directed sensitivity investigation (2026-10-31):** prioritize the
   authored car's frame rate; 60 FPS is a minimum, with substantial headroom
   desired for this basic example. Sandbox-only consumers vary solver settings
   without changing engine defaults or acceptance limits. In identical 10-second
   headless car screens, four configured substeps with 8/4/2 iterations achieved
   4.44/7.04/9.06 simulation frames per second; one configured substep with two
   iterations achieved 10.77. These are short physics-throughput diagnostics,
   not rendered FPS or full 60-second acceptance. Lower iterations therefore
   do not resolve the performance failure. All 48 existing goal 5 numerical
   checks pass at eight iterations; four iterations fail two cases and two
   iterations fail thirteen, exceeding the unchanged 0.02 penetration limit.
   Those earlier failed runs stopped early, so their observed depths do not
   establish replacement tolerances. Delayed-update timing still fails. Isolate contact response,
   joint response, collision gathering/refresh and swept-collision checks using
   separate diagnostic bypasses from identical copied car states. Bypassed
   stages alter physics and cannot qualify as correctness or FPS acceptance.
   Retain executable consumers and evidence under the milestone 11 sandbox.

   **Experiment sequence clarified (2026-11-01):** continue experiments until
   the car and soft-body workloads run well and the other physics cases pass
   or have measured, reviewable deviations. Then propose the minimal solution
   and its effects on every acceptance case. Preserve the experimental evidence
   before restoring experimental changes and applying only the selected fixes.
   No restore or acceptance-limit change is authorized by a timing result alone.
   Each scenario must declare a time or state/event endpoint, continue after
   numerical tolerance failures, and report pass/fail and full-run worst cases
   after reaching that endpoint. Fatal errors, invalid state, lack of progress
   and interruption produce explicitly incomplete results. Existing milestone
   fixtures retain their frozen durations; no settling threshold becomes an
   early termination condition. Repeat the lower-iteration tests with this
   reporting before assessing possible tolerance tradeoffs.

   All 48 cases now reach their frozen endpoints at each of 8/4/2 iterations.
   Their pass/fail counts remain 48/0, 46/2 and 35/13, but the two-iteration
   worst penetration increases from the early-stopped 0.095019 to 0.128841
   units. Four/eight iterations reach 0.029730/0.007038 units. Exploratory
   delayed-update maxima remain 345.903–373.631 ms, above 50 ms. Full-run
   results are in `sandbox/milestone_11/goal_1/logs/solver_full_run_goal5_summary.json`;
   none establishes acceptable replacement tolerances or real-time car performance.

   **Contact-work experiments (2026-11-01):** avoid polygon scratch storage and
   shape copies on the particle-pair response path; index retained rigid pairs
   without reordering constraints or changing accumulated impulses. Separate
   serial 10-second car screens measured 135.292 → 129.834 seconds of physics
   CPU for the scratch change and 128.057 → 126.048 seconds for the pair lookup.
   The lookup's p95 worsened slightly, so that single pair does not establish
   a useful performance gain. Both comparisons match all 600 frame-state
   checksums and all reported work counts. These are experimental candidates,
   not a selected final solution or rendered-FPS acceptance.
   All 219 timed fixtures retain identical numerical records, including all
   48 goal 5 cases at the original 16 iterations. Goal 5's peak penetration
   remains 0.003006; exploratory worst delayed CPU is 398.387 ms, still above
   50 ms. Twenty-six regressions, sixteen ASan/UBSan checks, Windows cross-builds,
   and independent Linux/sanitizer/Windows SDK delivery pass. Evidence and
   pre-experiment archives are under the milestone 11 sandbox, including
   `rigid_lookup_evidence.json`. Continue diagnosis before selecting the minimal
   changes; the car and delayed-update performance gates remain unresolved.
   A physics-only function profile completes the same 600 frames with identical
   state checksums. It records 7.34 billion entity-liveness checks, 3.49 billion
   component checks and 2.58 billion live-entity iteration calls. Investigate
   repeated membership validation and whole-world scans using the existing
   synchronous-substep lifetime boundary. Instrumented timings are not FPS
   evidence; public/manual-stage validation and ownership must remain intact.

   **Membership reuse experiment (2026-11-01):** reuse the synchronous substep's
   live-entity snapshot for internal liveness checks and collider-only scans.
   Component masks remain live, recovery deletion invalidates the snapshot, and
   manual stages retain current-table validation. One serial 10-second car
   comparison measured physics CPU 125.938 → 117.599 seconds (6.6% lower),
   with p95 445.279 → 412.707 ms. All 600 frame-state checksums and work counts
   match. Throughput remains about 5 simulation frames/s; these exploratory
   headless measurements do not establish rendered FPS or controlled acceptance.
   All 219 timed fixtures finish with identical numerical records. Goal 5's
   maximum penetration remains 0.003006; worst exploratory normal 60 Hz p95 is
   0.523 ms and delayed CPU is 383.701 ms, still failing the 50 ms limit.
   Twenty-six regressions, sixteen ASan/UBSan checks, Windows cross-builds and
   independent Linux/sanitizer/Windows SDK examples pass. Retained evidence and
   before/after archives are under the milestone 11 sandbox; see
   `membership_snapshot_evidence.json`. No final solution or tolerance changes
   have been selected. Continue measuring repeated contact/filter validation
   and collision-refresh work; bookkeeping savings alone remain insufficient.
   The subsequent pit-runner correction separates numerical escape failures
   from invalid-state/fatal stops, retaining worst depths through the declared
   endpoint. Injected finite escapes complete and fail numerically; missing or
   nonfinite state produces incomplete results even on the last scheduled frame.

   **Internal filter experiment (2026-11-01):** read collision filters by entity
   index in validated internal paths, preserving public validation, beam overrides,
   directional masks and stale-target rejection. A serial 10-second car comparison
   measured physics CPU 117.560 → 111.493 seconds (5.2% lower) and p95
   410.875 → 379.943 ms. All 600 frame-state checksums and work counts match.
   Headless throughput remains only 5.35 simulation frames/s; this exploratory
   pair does not establish rendered FPS or controlled acceptance. All 219 timed
   fixtures reach their endpoints with unchanged numerical records. Goal 5's
   peak penetration remains 0.003006; worst exploratory normal 60 Hz p95 is
   0.492 ms and delayed CPU is 374.695 ms, still above 50 ms. Twenty-six
   regressions, sixteen ASan/UBSan checks, Windows cross-builds and independent
   Linux/sanitizer/Windows SDK examples pass. Linux and sanitizer pit probes
   verify full-run numerical failure accumulation and incomplete invalid-state
   reporting. The 800-particle pit finishes all 12 seconds without escapes or
   invalid state: exploratory CPU is 79.157 seconds, p95 160.112 ms and maximum
   floor depth 0.002340. Evidence is retained under the milestone 11 sandbox in
   `index_filter_evidence.json` and its before/after archives. The final
   solution and tolerance decisions remain open; these bookkeeping improvements
   have not resolved the performance gap.

   **Refresh-cadence diagnostic (2026-11-01):** sandbox SDK wrappers defer
   contact discovery to every fourth solver iteration and the last scheduled
   iteration. Retained pairs still evaluate current geometry; convergence is
   deferred until discovery runs. Initial gathering, sweeps, geometry, materials
   and elapsed-time consumption remain unchanged. Three serial 600-frame car
   screens measure 5.38 simulation frames/s for the reference, 6.71 with deferred
   refresh at eight iterations, and 11.05 with deferred refresh at two iterations.
   These exploratory headless results remain far below 60 FPS. The reference
   wrapper preserves all 600 prior frame-state checksums.
   All 315 timed variant cases reach their endpoints, plus four state-based
   particle calibrations. At original fixture iteration settings, deferred
   refresh fails three stack settling schedules: worst post-toppling linear
   speed is 1.319054 units/s and angular speed 15.336437 degrees/s, against
   0.05 and 0.5. All 48 soft-body cases pass at both sixteen and eight iterations,
   with worst depths 0.003011 and 0.007395. Two iterations fail 13 of 48 cases,
   reaching depth 0.142258 against 0.02. Delayed soft-body CPU remains
   303.630–334.866 ms against 50. Seven focused regressions pass in both
   reference and deferred-refresh modes on Linux and ASan/UBSan; Windows
   cross-builds pass. The current engine implementation is unchanged by these
   wrappers. Reject uniform deferred refresh as a standalone solution; preserve
   the errors and timings for subsequent pipeline rework. See
   `sandbox/milestone_11/goal_1/contact_cadence_evidence.json` and its archive.

   **Historical branch sequence (2026-11-01; superseded by milestone 12):** conclude the current
   optimization comparisons and record the optimized implementation as a
   physics reference. The user will create the reference branch, then direct
   the rebase and a separate branch for reworking the affected pipelines. Use
   both main and the optimized reference when evaluating the rework. That
   comparison direction is superseded: milestone 12 excludes `main` and uses
   only the frozen reference at `6f283268`. This was
   a checkpoint within goal 1; its correctness and real-time acceptance gates
   remain unchanged. Preserve experimental evidence and distinguish sandbox
   behavior-changing variants from the implementation selected for reference.

### 6. Physics robustness

Pass five finite scenarios covering extreme scales, fast motion, contact
stability, constraints, and deformable bodies. Existing contact-stability,
concave-collision, beam-contact, and recovery fixes/tests remain the baseline.
The AABB-tree broad phase and live-entity iteration are already implemented.

**During milestone 12:** retain the status and historical criteria below and
continue running all five fixtures. Numerical tolerances and extreme-fixture CPU
budgets are comparison criteria, not intermediate rework completion gates;
failures remain visible. Final tolerance changes require explicit user acceptance
in milestone 12 goal 19, followed by verification before this status is updated.
**Current user-approved override:** every extreme-fixture penetration check now
uses 0.1 units provisionally, replacing the original drop 0.0001, thin-barrier
0.001 and soft-body 0.02 limits; stack and joint collision checks already use
0.1. This changes review/verdict thresholds only. Original criteria and dated
results below remain historical evidence. Positive support separation, centered
support retention, tunneling, energy, settling and joint-error requirements are
unchanged. The separate particle–particle pit review threshold is 0.5 units;
neither threshold changes solver correction slack.

**Review-policy update verified:** the 219 existing timed scenarios and four
rebound calibrations pass with unchanged state/work traces against the Goal 6
build. The new two-particle head-on regression passes all five schedules,
with worst overlap 0.000500202179 units against its 0.5 limit. The three changed
fixture suites and the new regression also pass ASan/UBSan: 193 timed scenarios
plus four calibrations, with Linux/sanitizer trace parity. Windows cross-builds
and independent installed-SDK drop-demo observation/control checks pass;
native Windows execution was not performed. No engine source or solver setting
changes. Evidence is under `sandbox/milestone_12/penetration_review/`. These
checks do not rerun rendered acceptance or clear the existing CPU-budget failure.

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

**Settling criteria:** Goal 1's centered stack has user-approved limits: settle
within 2 simulated seconds after impact, then stay supported for 10 seconds with
each body's linear speed at most 0.0001 units/s and angular speed at most 0.0001
degrees/s. Offset cases may move naturally. Goal 3's user-approved limits are
recorded with its fixture below. Goal 5's zero-damping, observational settling
policy is approved, and its recorded preview's motion and window switching were
accepted during milestone 11 goal 1. No automatic freezing is agreed;
settling criteria do not relax collision requirements during motion or require
undamped pendulums to stop.

1. [x] **Extreme mass/size drop.** Place a 0.01 × 0.01, mass 0.0001 rigid body on
   a 0.01 × 0.1 static floor whose stored mass is 0.0001. Drop a 100 × 100,
   mass 10,000,000 rigid body from rest with a 1,000-unit gap between its bottom
   and the tiny body's top. Use gravity -9.8, restitution 0, and friction 0.8.
   Test centered and slightly offset impacts. Penetration must stay at or below
   0.0001 world units at completed substeps; the static floor must remain fixed
   regardless of stored mass. The centered stack must remain supported without
   crushing geometry, sinking, or explosive numerical behavior. Offset impacts
   may tip or slide naturally without artificial locking. Resolve settling via
   the visual review above.
   The [dedicated C demo](examples/extreme_mass_drop/README.md)
   provides centered and ±0.0025-unit offsets, plus user-requested ±0.01-unit
   tipping comparisons beyond the tiny body's initial support footprint.
   The user revised the floor width from 1,000 to 0.01 in all five cases,
   matching the tiny body's width so the overhang has room to tip. Its center
   remains (0, -0.05); stored mass and the other fixture values are unchanged. These
   comparisons run for 30 seconds and must rotate at least 1° toward the
   overhang after impact, with the same penetration and finite-state limits;
   falling off the narrow floor is allowed for offset cases. The centered stack
   must remain supported. The demo includes 30/60/120 Hz controls,
   overview and magnified contact views, and angle readouts.
   Executables and local review evidence belong in
   ignored `sandbox/milestone_6/goal_1/`. After observing the tiny body sink in the
   baseline, the user explicitly chose to fix support and collision first, then
   review settling from the corrected motion. The implementation adds scaled
   penetration slop, collision-aware subdivisions, and simultaneous support
   constraints, with per-substep regression measurements. The tipping review
   also covers geometric correction and transitions from sticking to sliding.
   The user accepted the narrow-floor motion and window-switch review and the
   centered settling limits above. Complete: all twenty 30-second runs pass,
   including every completed substep during the centered 10-second hold.
   Maximum penetration is 0.000005958 units; Release normal 60 Hz p95 is at most
   0.490 ms and delayed updates at most 9.001 ms on the reference laptop on AC.
   Verification: 21 focused Linux engine regressions, nine ASan/UBSan tests,
   independent installed-SDK demo builds and two demo checks each on Linux and
   ASan/UBSan, normal/delayed player-floor probes on both, and Windows engine/test
   and demo cross-builds. No compiler warnings or sanitizer findings; native
   Windows execution was not performed and is not a completion gate.
   Local evidence is recorded in `sandbox/milestone_6/goal_1/collision_verification.txt`.
2. [x] **Fast objects against thin barriers.** Fire a 1 × 1 rigid body and a
   radius-0.5 particle at a 0.1-thick static wall at 100, 1,000, and 10,000
   units/second. Cover straight and oblique impacts with restitution 0 and 0.8.
   Require no tunneling and compliance with penetration and rebound criteria.
   Frozen fixture: a mass-1, initially unrotated square or radius-0.5 standalone
   particle (32 collision vertices), moving toward a static 0.1 × 40,000 wall
   centered at the origin. Gravity and friction are zero; both materials use
   the selected restitution. Initial velocity is `(speed, 0)` or
   `(speed / sqrt(2), speed / sqrt(2))`; initial center is
   `(-0.55 - velocity.x * 0.25, -velocity.y * 0.25)`, targeting the wall's
   middle at 0.25 seconds. Rotation stays unrestricted for the square.
   Run for 2.5 seconds at each rate. Two additional 60 Hz schedules start a
   100 ms or 1 s update at 0.2 seconds, followed by the other delay at 1.3
   seconds, so each delay crosses impact in a separate run. Observe every
   completed substep: penetration ≤0.001, fixed wall, finite state, no crossing,
   no energy growth above 0.1%, and full elapsed time within 1e-9 seconds.
   After impact, require normal velocity `-restitution * initial_velocity.x`
   within 1% (absolute 0.0001 units/s for zero restitution), tangential velocity
   unchanged within 0.01%, and no spurious spin above 0.0001 degrees/s.
   Keep executables and evidence under `sandbox/milestone_6/goal_2/`.
   Complete: all 120 runs pass using one configured substep and eight solver
   iterations. Contact subdivision now bounds closing normal motion, avoiding
   excessive work during sliding. Iterative contacts retain their restitution
   targets for the substep, preserving particle rebound through repeated solves.
   Peak penetration is 0.000010003 units; Release normal 60 Hz p95 is at most
   0.564 ms and delayed updates at most 27.040 ms on the reference laptop on AC.
   Verification: 21 focused Linux regressions, ten ASan/UBSan tests, all twenty
   Goal 1 runs including its performance limits, and circle/circle rebound at
   1/2/4/8 solver iterations. The extreme-drop, player-controller, and particle
   pit examples build independently against Linux, sanitizer, and Windows SDKs;
   Linux and sanitizer demo/startup/interrupt and delayed player-floor checks pass.
   No compiler warnings or sanitizer findings. Native Windows execution was not
   performed. The accepted Goal 1 window-switch review remains the interactive
   baseline; no rendering or event-loop behavior changed. Local evidence:
   `sandbox/milestone_6/goal_2/verification.txt`.
3. [x] **Persistent stack stability.** Stack twenty 10 × 10, mass-1 boxes on a
   static floor, simulate for 60 seconds, then strike the stack sideways with
   another box. Require no progressive sinking, growing jitter, or explosive
   numerical collapse; natural toppling is valid. Freeze the strike and
   post-strike observation interval, and agree settling limits after visual
   review before completing the goal.
   Frozen fixture: floor 1,000 × 10 centered at (0, -5), stored mass 1;
   twenty initially unrotated boxes centered at (0, 5 + 10i), i=0..19.
   Gravity is (0, -9.8), friction 0.8, restitution 0, with unrestricted dynamic
   motion. At exactly 60 seconds create a 10 × 10, mass-1 striker at (-30, 105)
   with velocity (50, 0) and the same gravity/materials. Observe until 90 seconds.
   Use 30/60/120 Hz and a 60 Hz schedule with a 100 ms update at 59.8 seconds
   and a 1 s update at 60.2 seconds; split at the exact striker creation time.
   Measure every completed substep, including independent rotated-rectangle
   penetration (limit 0.1 units), fixed floor, finite state, consumed time,
   energy, pre-strike drift, and residual linear/angular speeds. The user
   accepted the motion and window-switch preview and these settling limits:
   from 2 seconds until the strike, each body's linear speed must stay at most
   0.0001 units/s and angular speed at most 0.0001 degrees/s, with at most
   0.0001 units of further displacement from its settled position. After natural
   toppling, every body's linear speed must stay at most 0.05 units/s and angular
   speed at most 0.5 degrees/s throughout seconds 85–90. Small residual motion
   is accepted; the 0.1-unit penetration limit applies throughout. Executables
   and evidence belong under `sandbox/milestone_6/goal_3/`.
   Complete: all four 90-second schedules pass collision, finite-state, energy,
   time-consumption, drift, and accepted settling assertions. Maximum measured
   penetration is 0.001425 units; worst 60 Hz Release p95 is 0.652 ms and the
   longest delayed update is 40.631 ms on the AC-powered reference laptop.
   The bounded contact solver now supports larger coupled sets, independent
   sticking/sliding friction, and pose-checked geometry reuse within a solve.
   Twenty-one focused regressions and eleven sanitizer tests pass, with the
   final settling assertions also rerun under sanitizers. Goals 1–2 performance
   regressions, independent installed-SDK examples, normal/delayed player-floor
   probes, and Windows cross-builds pass. Native Windows execution was not run.
4. [x] **Joint stability.** Three independent executables exercise pin, weld,
   and spring joints, each in isolation and with floor impacts. The user revised
   the spring-supported-weight case to two rapidly orbiting, freely moving
   blocks. Use zero damping, friction 1, restitution 1, and 3,600 degrees/s
   clockwise rotation. After each dynamic body's first floor-contact episode
   ends in separation, change only that body's restitution to 0.1. Remove the
   arbitrary low-speed restitution cutoff; retain numerical tolerances only.
   Frozen fixtures (all dynamic masses 1, gravity (0, -9.8), configured substeps
   1, solver iterations 8; no sleeping, velocity clamps, or forced settling):
   - Pin: horizontal 100 × 10 arm centered at (50, 60), left endpoint pinned to
     (0, 60). Initialize COM velocity consistently with rotation about the pin.
     At exactly 5 seconds apply a magnitude-1,000 tangential COM impulse in the
     current clockwise direction. Reference length 100; anchor error ≤0.1 units.
   - Weld: 10 × 10 boxes centered at (-5, 200) and (5, 200), joined at their
     touching edges with relative angle zero. Initialize both spins and orbital
     COM velocities for rigid assembly rotation, plus common velocity (600, -800).
     At 5 seconds apply impulse (1,000, 0) only to the left box. Reference length
     10; anchor error ≤0.01 units and relative angular error ≤0.1 degree.
   - Spring: 10 × 10 boxes centered at (-50, 200) and (50, 200), center anchors,
     rest length 100, stiffness 20, damping 0. Initialize clockwise orbital motion
     and individual spins at 3,600 degrees/s, plus common velocity (600, -800).
     At 5 seconds apply impulse (1,000, 0) only to the right box. Compare isolated
     relative motion with an independent double-precision reference: position
     error ≤1% of max(100, reference peak separation), velocity error ≤1% of
     max(1,000, reference peak relative speed). Halve the reference integration
     step until changes are below 1% of those error limits. Also calibrate with
     opposite axial impulses of magnitude 1 against the analytic undamped
     oscillator, with position error ≤0.01 units and velocity error ≤0.05 units/s.
   Run each main case for 30 seconds at 30/60/120 Hz and at 60 Hz with a 100 ms
   update at 4.8 seconds and a 1 s update at 5.2 seconds; split exactly at the
   impulse. Isolated cases disable collision response. Impact cases use a static
   1,000,000 × 10 floor centered at (0, -5), stored mass 1, friction/restitution 1;
   welded bodies exclude mutual collision. Measure every completed substep:
   joint errors, finite state, fixed supports, energy, consumed time, independent
   penetration ≤0.1 units, and rotational sweeps against the floor. Isolated
   energy error must stay within 1%, accounting for gravity, elastic energy and
   constraint-consistent impulse work; verify applicable momentum conservation.
   Retain the shared rebound and CPU limits, including the observer in timings.
   Report settling after the final impulse as speeds ≤0.0001 units/s and
   ≤0.0001 degrees/s for 2 continuous seconds, or "not settled within 30 seconds";
   it is an observation, not a pass deadline. Obtain visual/window-switch review.
   Executables, previews and evidence belong under `sandbox/milestone_6/goal_4/`.
   Complete: automated verification passes, and the user accepted motion and
   window switching across the three joints in the review preview.
   All 27 joint cases pass, including three analytic spring calibrations.
   Release 60 Hz p95 is at most 0.630 ms; delayed updates at most 30.198 ms on
   the AC-powered reference laptop, including measurements. Peak penetration
   is 0.003139 units, pin error 0.000020 units, weld error 0.004407 units, and
   weld angular error zero. Pin impact cases settle at 5.3 seconds; weld and
   spring impact cases retain motion at 30 seconds, as permitted. Twenty-one
   focused regressions, twelve ASan/UBSan checks, and Goals 1–3 performance and
   settling regressions pass. Affected examples build independently against
   Linux, sanitizer, and Windows SDKs; Linux/sanitizer startup, interrupt,
   extreme-drop demo checks, and normal/delayed player-floor probes pass.
   Windows cross-builds pass; native Windows execution was not performed.
   No final compiler warnings or sanitizer findings. The shared solver uses
   coupled pin/weld rows, constrained integration for independent supported
   pairs, precise reduction of accumulated turns, and consistent low-speed
   restitution targets. Public APIs and persistence are unchanged. See
   `sandbox/milestone_6/goal_4/verification.txt` and `preview_controls.txt` there.
5. [ ] **Soft-body contact and stability — failed.** Drop a beam-braced soft body onto a
   floor and ramp, then apply rigid-body and particle loads. Deformation is valid;
   enabled collider contacts must meet the shared penetration/no-tunneling
   criteria. Agree damping and settling limits through visual review before
   completing the goal.

   **Goal 5 fixture frozen for implementation:** eight mass-1, radius-5 nodes:
   square corners `(±50, ±50)` and outward tips `(0, ±75)`, `(±75, 0)`.
   Four square edges, two diagonals, eight tip edges; stiffness 100, damping 0,
   geometric rest lengths. Only tip edges collide (thickness 2); nodes collide.
   Full nonincident self-contact, including beam pairs, is required; own endpoints
   and shared-endpoint beam pairs are excluded. Friction 0.5, restitution 0.
   Gravity `(0,-9.8)`; 4,000 × 20 floor or 15-degree ramp; initial lowest
   collider clearance 100. Each terrain runs 30 s with drop only, mass-80
   120 × 20 rigid load released 20 above the current node bounds at 8 s,
   radius-5 mass-1 particle at 10,000 units/s at 8 s (separate node and beam
   midpoint targets), and combined rigid load at 8 s / beam impact at 12 s.
   Targets are rightmost at launch, fixture-index tie break; particle starts
   200 units right and moves left. User-approved delays start at 8.01 s
   (100 ms) and 12.01 s (1 s), after exact load creation boundaries.
   The 10-second folding run uses 4.01 s and 8.01 s for those two delays.
   Gravity-free folding: opposite horizontal tips move inward
   at 200 units/s, 10 s, actual nonadjacent self-contact required. Isolated
   contact-free control: one tip displaced outward 5, 30 s, energy within 1%.
   Penetration limit 0.02; loaded energy may exceed its accounted budget by
   at most 1%. Zero-restitution separation tolerance 0.0001 units/s.
   **User-approved damping/settling policy:** zero beam damping; bounded
   oscillation accepted and settling observational. The user accepted the
   recorded preview's motion and window switching during milestone 11 goal 1;
   real-time performance remains unresolved. Original executables and evidence
   belong in `sandbox/milestone_6/goal_5/`; the corrective preview and verification
   belong in `sandbox/milestone_11/goal_1/`.

   **Verification outcome (2026-10-29): failed.** The 48-case AC-powered Release
   run has 25 passes and 23 failures. Peak observed penetration is 0.138491031
   units (limit 0.02), maximum 60 Hz p95 is 16.830 ms (limit 2), and the longest
   delayed update is 5829.767 ms (limit 50). Failed cases may stop before their
   full duration; these are not completed scenario passes. The isolated
   undamped control passes its 1% energy criterion. Full surface beam-pair
   response, spring integration, conservative motion bounds, and shared-node
   correction improvements are implemented, but do not satisfy the goal.
   All 22 affected regressions, 11 focused sanitizer tests, and sanitized
   folding/control probes pass. Linux and sanitizer installed-SDK examples,
   player-floor probes, Windows cross-builds, and Windows SDK consumers pass;
   native Windows execution was not performed. No final compiler warnings.
   Goals 1, 2, and 4 performance regressions pass. Goal 3 retains its numerical
   results but its delayed update measures 69.505 ms, then 58.421 ms on repeat;
   those measurements remain failure evidence, while the subsequent unchanged-build
   rerun recorded above passes. A dedicated Goal 5 visual preview
   and visual acceptance remain unfinished. See local `verification.txt` and
   `logs/performance_soft_body_stability_test.log` under the Goal 5 sandbox.

Choose algorithms to satisfy measured failures in these scenarios. Adaptive
substeps with reported counts, swept collisions through the shared pipeline,
concave-piece caching, stable boundary contact identifiers, sleeping with correct
wake propagation, compact active constraints, and removal of remaining capacity
scans are implementation options, not additional completion gates. Profile before
changing data structures. If compound contours or holes are needed by a fixture,
define their ownership and authoring before implementation; do not silently expand
collision semantics.

### C game experiment

Build a small, playful game in C after the frame-pacing fix, milestone 15's
creation/rendering workflow, milestone 16's scoped creation and teaching example,
and milestone 17's consistent creation APIs/report checks. Discover practical
engine needs before runtime isolation. By user direction, further physics work
is paused with its known failures preserved; robust general physics is no longer
a prerequisite for this checkpoint. This intermediate checkpoint is separate
from the larger end-to-end game in milestone 10. Agree its concept, scope, and
completion criteria with the user when starting; do not choose them implicitly
in this planning update.

1. [ ] Agree the game concept and completion criteria with the user, including
   the engine features it will exercise and how completion will be verified.
2. [ ] Implement the agreed C game, recording practical engine needs as they
   emerge and obtaining direction for changes that affect engine scope.
3. [ ] Verify the agreed completion criteria, run regression and relevant
   sanitizer checks, and record findings for runtime isolation and later work.

### 8. Global-state assumptions

Support two independent runtime worlds in one process, including presentation,
explicit ownership, input routing, and teardown. Complete this milestone after
the C game experiment and before hardening and Python bindings.
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

### 5. Adversarial testing and hardening

Expose malformed input, resource failures, lifecycle defects, and integration
regressions within the frozen inventory of all first-party structured-data entry
points and affected subsystems. Begin after runtime isolation so hardening covers
the resulting ownership and lifecycle boundaries.

Implemented foundations: Linux ASan/UBSan support; failure-injection and lifecycle
suites for allocation, capacity, stale handles, ownership, initialization,
rollback, and recreation; malformed project/configuration/runtime-state fixtures
and diagnostic recovery coverage; installed-SDK automation, distribution
workflows, generated scaffolding, and developer-file preservation tests.

1. [ ] **Fuzz malformed input.** Inventory all first-party structured-data entry
   points and run a sanitizer-enabled one-hour campaign per entry point, including
   project, runtime-state, object description, and configuration loaders/parsers
   where applicable. Use a fixed seed corpus and record reproduction details.
   Fix discovered crashes and sanitizer findings, retain
   regression inputs, and finish with a clean campaign for each entry point.
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

### 9. Python bindings

Expose the public runtime C API stabilized by runtime isolation and hardening,
and authored-object spawning, through thin bindings with explicit ownership.
Freeze applicable API coverage at milestone start. Editor project automation is
outside scope; document exclusions for C-only mechanisms such as code-generation
macros.

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

### 7. Editor implementation scalability

Reduce manual property edit sites and keep large scenes usable while preserving
explicit C ownership and existing interaction/serialization behavior. Defer this
milestone until after Python bindings unless game development makes it necessary
sooner and the user explicitly reprioritizes it. Retain the shared-property and
10,000-item goals below.

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

### 10. End-to-end game

Develop a large game last, separately from the earlier C game experiment,
iterating engine improvements during its development.
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

- Camera named-layer allowlists and blocklists in descriptions and runtime APIs.
  Match layer identity, not numeric draw order: different names sharing a number
  must remain independently selectable. Goal 4 of milestone 15 preserves the
  identities; filtering itself is not implemented there.
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
- Manual inertia authoring, additional force-emitter tooling, and broader
  attachment API redesign beyond existing support; requirements remain undefined.
