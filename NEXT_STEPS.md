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
| 12 | Real-time physics pipeline rework | Highest unfinished priority; goal 1 complete for review |
| 11 | Soft-body correctness and real-time performance | Superseded by milestone 12; not complete; evidence preserved |
| 6 | Physics robustness | Goals 1–4 accepted; goal 5 failed verification |
| — | [C game experiment](#c-game-experiment) | Pending; agree concept and completion criteria when starting |
| 8 | Runtime isolation (global-state assumptions) | Foundations implemented; two independent worlds pending |
| 5 | Adversarial testing and hardening | Substantial coverage implemented; follows runtime isolation |
| 9 | Python bindings | Pending; bind the stabilized runtime API after isolation and hardening |
| 7 | Editor implementation scalability | Deferred until after Python unless game development makes it necessary sooner |
| 10 | End-to-end game | Pending; separate larger game directed by the user |

**Completed goal:** milestone 6 goal 4, **Joint stability**, including the user's
visual/window-switch acceptance. Its frozen fixtures and verification are
recorded below. Goal 5, **Soft-body contact and stability**, was pursued and
**failed verification** at checkpoint `63fccb26` on extreme-load penetration and
CPU limits. The preserved milestone 11 attempt passes all 48 existing numerical checks,
but still misses delayed-update and car real-time budgets; neither goal is complete.
Goal 3's timing rerun on the unchanged build passes at 45.432 ms delayed and
0.771 ms worst 60 Hz p95; earlier 69.505/58.421 ms failures remain evidence of
measurement variability, not an established physics regression. Milestone 12
supersedes milestone 11 and is the highest unfinished priority: achieve real-time
car and pit performance, then compare accuracy and behavior with the frozen
`physics-pipelines-overoptimized` reference at `6f283268`. Exclude `main` from
comparisons. Milestone 12 goal 1 establishes the reference and measurement
contract, with the full car run explicitly stopped by the user and retained as
incomplete. Goal 2 awaits explicit instruction; the C game experiment has not
started.

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

### 12. Real-time physics pipeline rework

Rework physics on `physics-pipelines`, using **`physics-pipelines-overoptimized`
at `6f283268`** (full commit `6f283268aa4bf038c4ed78271baeb9d5986bd7d9`) as the
frozen reference. **Exclude `main` from comparisons.** First achieve real-time
performance in the unchanged soft-body car and 800-particle pit. Then assess
accuracy and behavior against the reference and choose an acceptable middle
ground. This is the highest unfinished priority and supersedes milestone 11,
whose failed attempts, experiments, executables, and evidence remain preserved.

**Status: goal 1 complete for review; goal 2 not started.** The
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

Each pipeline goal includes analysis, a measured rework where justified,
verification, and a reviewable commit. A pipeline may be retained when
measurements show rewriting it offers no worthwhile benefit; document that
finding.

1. [x] **Establish the reference and measurement contract.** Pin the reference
   commit, preserve its executables and evidence, and establish repeatable
   car/pit runs. Measure each stage's exclusive cost, call counts, subdivisions,
   iterations, candidates and contacts without double-counting nested work.
   Record baseline physics errors and define the comparison report used by
   subsequent goals.
2. [ ] **Analyze and rework force preparation.** Cover acceleration clearing,
   gravity, external loads, spring/beam forces and soft-body frame updates.
   Identify repeated calculations and unnecessary scans. Preserve force
   ownership, application timing and standalone pipeline behavior.
3. [ ] **Analyze and rework integration and subdivision.** Cover rigid, joint
   and soft-body integration, swept collision checks, and adaptive step selection.
   Investigate why individual interactions cause repeated world-wide work.
   Evaluate timestep and subdivision settings alongside algorithm changes;
   consume all elapsed simulation time.
4. [ ] **Analyze and rework contact gathering.** Cover broadphase construction
   and update, candidate filtering, narrowphase geometry, rigid contacts,
   particle contacts, soft boundaries and beam pairs. Reduce repeated discovery
   and geometry work while measuring missed contacts, penetration and tunneling.
5. [ ] **Analyze and rework joint gathering.** Cover anchor resolution, pin/weld
   constraint assembly, validation and reusable joint data. Separate data that
   changes with topology from data that changes with motion, preserving
   entity-lifetime safety and manual gathering semantics.
6. [ ] **Analyze and rework rigid-contact solving.** Cover particle and polygon
   response, block solving, position correction, friction, restitution and
   retained contact state. Evaluate simpler execution and data layouts,
   iteration requirements and convergence behavior. Measure effects on mass
   ratios, impacts, stacks and dense particle piles.
7. [ ] **Analyze and rework joint solving.** Cover pin/weld response, spring
   coupling and transform locks. Measure constraint error, energy behavior and
   cost under the existing high-speed, high-rotation fixtures. Preserve
   documented ownership and lock semantics.
8. [ ] **Analyze and rework soft-body constraint solving.** Cover node–rigid,
   beam–rigid and beam–beam response, self-collision and contact refresh.
   Evaluate how rigid corrections and soft contacts interact. Measure
   deformation, seam penetration, tunneling, energy and car behavior.
9. [ ] **Analyze and rework pipeline orchestration and finalization.** Integrate
   the revised stages. Review their execution order, shared caches, invalidation,
   repeated passes, interaction publication and reporting overhead. Verify
   standalone stages and complete updates remain correctly composed.
10. [ ] **Meet the combined real-time gates.** Resolve remaining integration
    bottlenecks and select measured runtime settings. Verify both the car and
    particle pit meet the performance contract below. Produce visual previews
    and obtain motion/window-switch review. Numerical deviations remain
    explicitly recorded.
11. [ ] **Compare with the reference and select the middle ground.** Compare
    the real-time implementation against the frozen reference across all physics
    scenarios. Present accuracy, behavior and performance tradeoffs for user
    selection. Implement the selected balance, retain real-time performance,
    and verify the agreed final tolerances before updating milestone 6's status.

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
- Existing numerical tolerances and extreme-fixture CPU budgets become
  **comparison criteria during rework**, not mandatory intermediate completion
  gates. Keep their failures visible; do not silently redefine a pass. This
  policy supersedes milestone 11's fixed-settings and intermediate acceptance
  requirements, while preserving their historical evidence and milestone 6's
  recorded status.
- Public APIs and ownership contracts remain unchanged. Any necessary API
  redesign requires a separate decision.
- Final tolerance changes require explicit user acceptance in goal 11. Prefer
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
in milestone 12 goal 11, followed by verification before this status is updated.

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

Build a small, playful game in C after physics robustness to discover practical
engine needs before runtime isolation. This intermediate checkpoint is separate
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
