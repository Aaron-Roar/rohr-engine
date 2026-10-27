# Cross-path feature inventory

This is the frozen acceptance inventory for [roadmap milestone 4](../NEXT_STEPS.md#4-cross-path-feature-parity).

**Milestone 4 is complete by user acceptance (2026-10-27).** Goals 1–4 and
corrective goals C1–C16 retain their recorded evidence. C7's five deferred manual
checks and C9's skipped additional panel confirmation remain **unverified**.
Broader goal 5 cases/retests without recorded confirmation also remain unverified
exceptions; acceptance does not mean they passed. The procedures and historical
results below are retained for later review.

The [roadmap priority order](../NEXT_STEPS.md#roadmap-milestones) now puts physics,
a small C game, and runtime isolation before hardening and Python bindings;
editor scalability follows Python unless explicitly reprioritized for the game.
This planning update changes no runtime API or persistence compatibility.
Goal 1 records the original coverage baseline; goals 2–5 own the closures below.

## Baseline and interpretation

Audit begun 2026-10-19, completed 2026-10-20 at commit `e078695808112b41af7384bb902b7d619ffac759`.
The starting worktree also contained documentation edits in `NEXT_STEPS.md`,
`docs/audio_reporting_contract.md`, `docs/entity_inspection_contract.md`, and
`docs/force_torque_contract.md`. Those edits are part of the reviewed baseline;
there were no starting source changes. This goal adds this inventory and updates
only the roadmap's goal 1 entry. Later features do not automatically expand it.

The [existing exact API inventory](description_api_contract.md#exact-header-inventory)
is the declaration baseline, not semantic evidence. The appendices reconcile
all 1,602 declaration/macro occurrences in 20 installed headers and list every
field of the public description/configuration/definition types, their nested
configuration payloads, and editor project/workspace/configuration records.
Array members represent all elements; nested payloads link through their named
types. Stable property identities are `Type.member`; F/X identifiers group only
the applicable path assessment. Polymorphic object fields use the kind-specific
rules below. Constants/enums supply domains for these fields, not extra features.

Columns: **D** direct API, **W** public wrapper, **E** editor interaction,
**C** CLI authoring, **J** project JSON save/load, **G** generated C/object catalog
conversion and loading, **S** runtime snapshot/template persistence, **R** runtime
behavior. Tool-only rows use their direct editor-core interfaces for D; no public
runtime API is implied. Legacy state JSON is S, not J.

- **I**: a source implementation route was located. This is not a passing
  behavioral result or proof that every value/combination survives every path.
- **V**: a concrete verification task remains; source evidence does not establish
  the whole property's path. Its gap IDs describe what to resolve.
- **G**: a specific missing representation or authoring route was found.
- **N**: intentionally not applicable for the stated reason; never inferred merely
  because an implementation is absent. Dynamic gameplay remains developer-owned C.

Test links identify existing fixtures to inspect/extend. Only the checks recorded
in the verification section were executed for this goal. Every I/V cell remains
subject to its row's V2–V5 obligations; no cell claims full behavioral acceptance.
Defaults, zero values, signed degrees, reference ownership, atomic replacement,
focused mutation, deletion, and errors follow the existing contracts.

## Property and behavior matrix

| ID | Property/behavior group | D | W | E | C | J | G | S | R | Evidence | Open work / N reason |
|---|---|---|---|---|---|---|---|---|---|---|---|
| <a id="f01"></a>F01 | Entity names, ownership, child membership, deletion and independent connections | I | I | V | V | I | I | I | I | [entity](#evidence-entity) | V2,V3,V4,V5; G01 |
| <a id="f02"></a>F02 | Generic groups and hierarchy parenting, independent of ownership | I | I | N | N | N | N | I | I | [entity](#evidence-entity) | V2,V3; N1 |
| <a id="f03"></a>F03 | Lifetime deadlines and explicit enable flags | I | I | G | G | G | G | I | I | [entity](#evidence-entity) | G02,V2,V3 |
| <a id="f04"></a>F04 | Engine/physics tick duration, override, substeps, gravity and solver iterations | I | I | I | I | I | I | N | I | [settings](#evidence-settings) | V2,V3,V4,V5; N2 |
| <a id="f05"></a>F05 | Rigid hitbox vertices, variants, stable IDs, active hitbox and auto-shapes | I | I | I | I | I | I | I | I | [geometry](#evidence-geometry) | V2,V3,V4,V5 |
| <a id="f06"></a>F06 | Initial position, degree orientation, velocity, acceleration and angular velocity | I | I | I | I | I | I | I | I | [motion](#evidence-motion) | V2,V3,V4,V5 |
| <a id="f07"></a>F07 | Mass, automatic/explicit COM and derived inertia | I | I | I | I | I | I | I | I | [mass](#evidence-mass) | V2,V3,V4,V5 |
| <a id="f08"></a>F08 | Friction and restitution on bodies, particles, nodes and beams | I | I | I | I | I | I | I | I | [material](#evidence-material) | V2,V3,V4,V5 |
| <a id="f09"></a>F09 | Body/particle static, gravity and collision flags; category/with masks | I | I | I | I | I | I | I | I | [collision](#evidence-collision) | V2,V3,V4,V5 |
| <a id="f10"></a>F10 | Body rotation lock at authored orientation | I | I | I | I | I | I | I | I | [motion](#evidence-motion) | V2,V3,V4,V5 |
| <a id="f11"></a>F11 | Axis and full transform locks, driver/offsets/flags | I | I | G | G | G | G | I | I | [motion](#evidence-motion) | G03,V2,V3 |
| <a id="f12"></a>F12 | Standalone particle radius, origin, rigid vertex count and nonrotation | I | I | I | I | I | I | I | I | [particle](#evidence-particle) | V2,V3,V4,V5 |
| <a id="f13"></a>F13 | Soft-body usable origin, node-average frame, angular motion and membership | I | I | I | I | I | I | I | I | [soft](#evidence-soft) | V2,V3,V4,V5 |
| <a id="f14"></a>F14 | Node position, mass, radius, initial motion, gravity, collision and inherited color | I | I | I | I | I | I | I | I | [soft](#evidence-soft) | V2,V3,V4,V5 |
| <a id="f15"></a>F15 | Static soft-body node configuration | I | I | G | G | G | G | I | I | [soft](#evidence-soft) | G04,V2,V3 |
| <a id="f16"></a>F16 | Beam endpoints, stiffness, damping, inherited color and connection independence | I | I | I | I | I | I | I | I | [beam](#evidence-beam) | V2,V3,V4,V5 |
| <a id="f17"></a>F17 | Explicit beam rest length versus measured creation length | I | I | G | G | G | G | I | I | [beam](#evidence-beam) | G05,V2,V3 |
| <a id="f18"></a>F18 | Beam collision opt-in, thickness and independent category/with masks | I | I | I | I | I | I | I | I | [beam](#evidence-beam) | V2,V3,V4,V5 |
| <a id="f19"></a>F19 | Node-loop area outer boundary, holes, ordering, fill/color and incomplete topology | I | I | I | I | I | I | I | I | [area](#evidence-area) | V2,V3,V4,V5 |
| <a id="f20"></a>F20 | World/body/node anchors and local offsets | I | I | I | I | I | I | I | I | [joint](#evidence-joint) | V2,V3,V4,V5 |
| <a id="f21"></a>F21 | Pin/weld/spring endpoints, stiffness, damping, rest length/angle and explicit flags | I | I | I | I | I | I | I | I | [joint](#evidence-joint) | V2,V3,V4,V5 |
| <a id="f22"></a>F22 | Explicit joint placement versus connection-only creation | V | V | V | V | N | N | N | V | [joint](#evidence-joint) | G06; N3 |
| <a id="f23"></a>F23 | Node-to-anchor pin helper and ownership | I | I | V | V | V | V | V | I | [joint](#evidence-joint) | G07,V2,V3,V4 |
| <a id="f24"></a>F24 | Force owner/target/magnitude; persistent versus initial application | I | I | I | I | I | I | I | I | [source](#evidence-source) | V2,V3,V4,V5 |
| <a id="f25"></a>F25 | Force location/orientation references, offsets, beam anchor/fraction and between-locations direction | I | I | I | I | I | I | I | I | [source](#evidence-source) | V2,V3,V4,V5 |
| <a id="f26"></a>F26 | Torque owner/target/magnitude; persistent versus initial application | I | I | I | I | I | I | I | I | [source](#evidence-source) | V2,V3,V4,V5 |
| <a id="f27"></a>F27 | Force/torque/joint marker fill, border, thickness, scale, offset, rotation; separate spring endpoints | I | I | I | I | I | I | I | I | [visual](#evidence-visual) | V2,V3,V4,V5 |
| <a id="f28"></a>F28 | App visibility, body colors/borders, hierarchy suppression and stored child visibility | I | I | I | I | I | I | I | I | [visual](#evidence-visual) | V2,V3,V4,V5 |
| <a id="f29"></a>F29 | Editor-only eyes and persisted navigation | N | N | I | I | I | N | N | N | [editing](#evidence-editing) | V2,V3,V5; N4 |
| <a id="f30"></a>F30 | Texture path/logical size and explicit retain/release lifetime | I | I | I | I | I | I | I | I | [texture](#evidence-texture) | V2,V3,V4,V5 |
| <a id="f31"></a>F31 | Sprite image, direction, scale, offsets, rotation inheritance and visibility | I | I | V | V | I | I | I | I | [sprite](#evidence-sprite) | G08,V2,V3,V4,V5 |
| <a id="f32"></a>F32 | Animation/frame IDs, paths, transforms, count and empty animations | I | I | I | I | I | I | I | I | [animation](#evidence-animation) | V2,V3,V4,V5 |
| <a id="f33"></a>F33 | Animation timing, starting frame, playback and hitbox-frame bindings | I | I | I | I | I | I | I | I | [animation](#evidence-animation) | V2,V3,V4,V5 |
| <a id="f34"></a>F34 | Camera position, orientation, dimensions, zoom and enablement | I | I | I | I | I | I | I | I | [camera](#evidence-camera) | V2,V3,V4,V5 |
| <a id="f35"></a>F35 | Camera target and position/orientation offsets and follow flags | I | I | V | V | I | I | I | I | [camera](#evidence-camera) | G09,V2,V3,V4,V5 |
| <a id="f36"></a>F36 | Camera pause-with-engine configuration | I | I | G | G | G | G | I | I | [camera](#evidence-camera) | G10,V2,V3 |
| <a id="f37"></a>F37 | Camera callback/context and application-owned render scheduling | I | I | N | N | N | N | N | I | [camera](#evidence-camera) | V2,V5; N5 |
| <a id="f38"></a>F38 | Named/numeric render layers, item assignments and explicit draw overrides | I | I | I | I | I | I | V | I | [layers](#evidence-layers) | V2,V3,V4,V5; G11 |
| <a id="f39"></a>F39 | Screen source camera and render dimensions | I | I | V | V | V | V | N | I | [viewport](#evidence-viewport) | G12,V2,V3,V4,V5; N5 |
| <a id="f40"></a>F40 | Viewport rectangle/fit/background/enabled; item placement, content transforms, clip, drag and layering | I | I | I | I | I | I | N | I | [viewport](#evidence-viewport) | V2,V3,V4,V5; N5 |
| <a id="f41"></a>F41 | Persistent UI shape geometry, border styles, interaction colors and button enablement | I | I | I | I | I | I | N | I | [ui](#evidence-ui) | V2,V3,V4,V5; N5 |
| <a id="f42"></a>F42 | Persistent UI text/font/value/color/wrap and text transforms | I | I | V | V | V | V | N | I | [ui](#evidence-ui) | G13,V2,V3,V4,V5; N5 |
| <a id="f43"></a>F43 | Persistent sliders: range, value, step, geometry, thumb shape/offset, colors and enabled | I | I | I | I | I | I | N | I | [ui](#evidence-ui) | V2,V3,V4,V5; N5 |
| <a id="f44"></a>F44 | Legacy JSON button/font/label/slider definitions | I | I | N | N | N | N | I | I | [immediate](#evidence-immediate) | V2,V3,V5; N6 |
| <a id="f45"></a>F45 | Font path/point size, built-in default; text replacement and ownership | I | I | V | V | V | V | V | I | [font](#evidence-font) | G13,V2,V3,V4,V5 |
| <a id="f46"></a>F46 | Sound/music paths, muted default volume, pan, loop and playback rate | I | I | G | G | G | G | N | I | [audio](#evidence-audio) | G14,V2,V5; N2 |
| <a id="f47"></a>F47 | Audio lifecycle, playback requests, callback-applied state and reports | I | I | N | N | N | N | N | I | [audio](#evidence-audio) | V2,V5; N1 |
| <a id="f48"></a>F48 | Controller/action names, enabled/type/mode/initial state and default bindings | I | I | I | I | I | I | N | I | [input](#evidence-input) | V2,V3,V4,V5; N2 |
| <a id="f49"></a>F49 | Input override bindings, live button/axis state, keyboard/mouse/text snapshots | I | I | N | N | N | N | N | I | [input](#evidence-input) | V2,V5; N1 |
| <a id="f50"></a>F50 | Application window mode, window/logical size and automatic aspect ratio | I | I | G | G | G | G | N | I | [window](#evidence-window) | G15,V2,V5; N2 |
| <a id="f51"></a>F51 | Object catalogs: stable references, editable copies, category flags, spawn offset and member access | I | I | I | I | I | I | I | I | [object](#evidence-object) | V2,V3,V4,V5 |
| <a id="f52"></a>F52 | Runtime save/load, multi-file references, templates, rollback and topology restoration | I | I | N | N | N | N | I | I | [state](#evidence-state) | V2,V3,V4; N7 |
| <a id="f53"></a>F53 | Physics queries, overlap/contact transitions, impulses and transient steering | I | I | N | N | N | N | N | I | [operations](#evidence-operations) | V2; N1 |
| <a id="f54"></a>F54 | Initial angular acceleration and generic target configuration | I | I | G | G | G | G | I | I | [motion](#evidence-motion) | G17,V2,V3 |
| <a id="f55"></a>F55 | Scene simulation object inclusion, viewport choice, regenerated scene source and session lifecycle | I | N | I | I | I | I | N | I | [scene](#evidence-scene) | V2,V3,V4,V5; N9 |
| <a id="f56"></a>F56 | Workspace creation/loading, generated file ownership, SDK builds and external launches | N | N | I | I | I | I | N | I | [build](#evidence-build) | V3,V4,V5; N9 |
| <a id="f57"></a>F57 | Editor selection/picking, drag transactions, undo/redo, context menus and tabs | N | N | I | V | N | N | N | N | [editing](#evidence-editing) | V2,V5; N4 |
| <a id="f58"></a>F58 | Kinematic flags, holds and simulation participation state | I | I | N | N | N | N | V | I | [operations](#evidence-operations) | G16,V2,V3; N1 |
| <a id="f59"></a>F59 | Immediate-mode UI interaction, styles, fields, menus and callbacks | I | I | N | N | N | N | N | I | [immediate](#evidence-immediate) | V2,V5; N6 |
| <a id="f60"></a>F60 | Angular-velocity maximum configuration and snapshot preservation | I | I | G | G | G | G | G | I | [motion](#evidence-motion) | G17,V2,V3 |
| <a id="f61"></a>F61 | Custom font point-size selection for generated viewport UI | I | I | G | G | G | G | N | I | [font](#evidence-font) | G13,V2,V3,V4,V5; N5 |
| <a id="x01"></a>X01 | Pure math/value construction and temporary geometry helpers | I | I | N | N | N | N | N | I | [math](#evidence-math) | V2; N8 |
| <a id="x02"></a>X02 | Result/report machinery, entity inspection, console and diagnostic output | I | I | N | N | N | N | N | I | [report](#evidence-report) | V2,V5; N8 |
| <a id="x03"></a>X03 | Pool storage macros and generated component registration/code generation | I | V | N | N | N | N | N | I | [storage](#evidence-storage) | V2; N8 |
| <a id="x04"></a>X04 | Engine/system/scene lifecycle, clocks, callbacks and standalone pipeline execution | I | I | N | N | N | N | N | I | [lifecycle](#evidence-lifecycle) | V2,V5; N1 |
| <a id="x05"></a>X05 | Immediate draw commands, renderer controls and transient graphics state | I | I | N | N | N | N | N | I | [draw](#evidence-draw) | V2,V5; N1 |
| <a id="x06"></a>X06 | Platform/directory/random/string/size utilities | I | I | N | N | N | N | N | I | [tools](#evidence-tools) | V2,V5; N8 |
| <a id="x07"></a>X07 | Editor-only hierarchy IDs, names, overview/navigation state and tool preferences | N | N | I | V | I | N | N | N | [editing](#evidence-editing) | V2,V3,V5; N4 |

## Applicability boundaries

- **N1 — Runtime operations:** groups/parenting used by gameplay, live input,
  transient loads, steering, queries, callbacks, and subsystem update calls do
  not become editor controls or serialized execution. Stored configurations
  reachable through those operations are assessed separately. Generic runtime
  groups are not graphics layers or soft-body owners.
- **N2 — Application services:** the current entity snapshot contract does not
  promise restoration of engine clocks/solver settings, audio devices/streams,
  logical input controllers, or window presentation. Project authoring of their
  static settings remains applicable, including the gaps below. Saving runtime
  input overrides or playback position is not added by this audit.
- **N3 — Placement operation:** explicit joint placement changes a pose; save the
  resulting pose, not a historical placement action. Low-level connection creation
  must stay free of implicit movement.
- **N4 — Editor state:** eyes, names of editor sub-items, overview placement,
  selection, undo, tabs and navigation are editor-only. They must not affect app
  visibility or physics. Storage capacity, caches and next-ID bookkeeping are
  implementation data, not authorable runtime properties.
- **N5 — Presentation ownership:** camera callbacks/context and viewport mounts
  remain application-owned under the [runtime topology contract](game_state.md#runtime-topology-contract).
  Persistent viewport UI is reconstructed from authored/generated definitions;
  it is distinct from legacy state-file UI definitions. Object-owned cameras and
  their attachments are persisted and remain in scope.
- **N6 — Immediate UI:** immediate-mode controls are C operations. Legacy JSON
  definitions load through the state API; the editor authors persistent viewport
  UI instead. Do not require identical structs or editor controls for every
  immediate-mode interaction parameter. Equivalent supported visible properties
  still require V4 checks between authored UI and direct persistent-UI APIs.
- **N7 — Runtime state files:** the editor loads project documents, not arbitrary
  live-world snapshots into its authoring model. Snapshot file operations remain
  directly callable from C, with multi-file merge/rollback verification.
- **N8 — Support APIs:** math, value constructors, storage pools, report/result
  helpers, code generation, file-directory and string utilities have no scene
  authoring equivalent. Public/direct behavior still matters where both exist.
- **N9 — Tool workflows:** generation/build selection is a tool operation, not an
  ECS component. Generated source is refreshed, developer-owned `main.c` is
  preserved, and selecting a viewport does not author a new camera.

The `ObjectDescription` union-like record must be interpreted by `kind`:
`endpoint_a/b`, `stiffness`, `damping`, and `rest_length` route to F16/F17 for beams
and F21 for joints; `target` routes to F20/F31/F35 for anchors/visuals/cameras;
`size`, `scale`, `asset_path`, and `direction` route to F30–F35 as appropriate.
`follow_position`, `follow_orientation`, and `zoom` use F34/F35 for cameras;
`position`/`orientation` may be attachment offsets, not world poses. The ledger's
primary assignment does not make every field applicable to every object kind.
`Shape` concave caches/preparation flags and editor capacities/caches are X03
implementation storage; only vertices/count are authored geometry.

## Concrete gaps and owning goals

Each entry below is an observed omission or a bounded verification question.
A verification gap is not a confirmed bug. Goal 2 closes configuration/reference
issues; goal 3 carries corresponding JSON, conversion, generation and load work.
If applicability requires a product decision, resolve it before that goal rather
than silently treating the gap as N or adding an unrelated feature.

| ID | Finding and required closure | Owner |
|---|---|---|
| G01 | Runtime `entity_owner_set` permits ownership independent of connections. The authoring model nests children under objects/soft bodies. Verify expressible cross-owner connections, deletion and reassignment separately; record exactly which relationships require direct C. Inspect `EditorObject`, `EditorSoftBody`, source references and `object_spawn.c`. | 2; round trips 3 |
| G02 | `EntityLifetimeDescription` and state loading/saving support deadlines and enable flags. `EditorProject`/`ObjectDescription` have no lifetime fields. Resolve authored initial lifetime configuration and generated application of its semantics; absolute runtime deadlines must not be confused with authored durations. | 2, 3 |
| G03 | General axis/transform lock descriptions have no matching editor/catalog fields. Generated anchor attachment uses transform locks internally, which does not expose arbitrary driver/axis/flags. Verify and close the general configuration gap without changing attachment ownership. | 2, 3 |
| G04 | `SoftBodyNodeDescription.static_body` exists; `EditorSoftNode` has no static flag and `object_document.c` does not transfer one. Authoring cannot currently express that runtime setting. | 2, 3 |
| G05 | `SoftBodyBeamDescription` supports explicit rest length; `EditorSoftBeam` stores no rest length. `object_document.c` computes it from endpoint distance, although the catalog/runtime can store an explicit value. Close editor/CLI/project-to-catalog conversion without losing measured-default creation. | 2, 3 |
| G06 | `editor_project.c` contains placement/connected-body movement while `joint.c` provides connection APIs. Freeze cases for first-to-second placement, reversed endpoints, world/shared anchors, locks and connected groups; distinguish intentional placement from implicit movement. Source presence alone does not establish the requested semantics. | 2 |
| G07 | The public node-to-anchor pin helper has no obviously corresponding dedicated editor item. Determine and verify whether existing node-attached anchors plus pin joints express it, including ownership and save/load; add a gap only for behavior that cannot be represented. | 2, 3, 4 |
| G08 | Runtime sprites have a `direction`; `EditorSprite` has size and follow-rotation but no direction field. Verify signed size/scale equivalence for flips and transforms before declaring a missing feature. Animation direction is represented separately. | 2, 3, 4 |
| G09 | Runtime camera attachments have independent follow-position/follow-orientation flags. Editor attachments expose `inherit_orientation`; generated attachment code sets follow-position true. Verify/close the position-follow choice and preserve world setters' offset semantics. | 2, 3 |
| G10 | `CameraDescription.pause_with_engine` and runtime camera snapshots support pause flags. `EditorCamera` has no pause field and generated/catalog creation omits that choice. | 2, 3 |
| G11 | Numeric assignments for entity/sprite/animation layers are captured by runtime visuals. Verify whether named layer identity and assignment survive restore; do not confuse saved numeric output with an independently restored named registry. | 3 |
| G12 | Direct `ScreenDescription` supplies camera and width/height. Editor camera mounts generate screens through viewport helpers. Verify which dimensions are derived and whether direct equivalent composition can be reproduced; independent application-owned screen instances need not become authored objects. | 2, 3, 4 |
| G13 | Generated custom fonts are explicitly loaded at `point_size=12.0f` in `editor_workspace.c`; `EditorUiFont` has no point-size field (F61). Text wrap width and nested text orientation also do not map one-to-one to editor text box/scales. Trace generation and compare layout for built-in/custom fonts, wrapping, independent text rotation and repeated mounts. Record semantic omissions rather than demanding identical field names. | 2, 3, 4 |
| G14 | Sound/music configuration exists in `audio.h`; there are no sound/music collections in `EditorProject`, object kinds, or workspace generation. Static audio authoring is missing; playback triggers remain developer-owned. Existing audio tests do not prove authoring parity. | 2, 3; audible check 5 |
| G15 | Window presentation descriptions configure the application. The editor Visual settings panel saves GUI preferences through `EditorGuiState`; `EditorProject`/workspace config do not carry application window settings. Do not count changing the editor's own window as application authoring support. | 2, 3; desktop check 5 |
| G16 | Interaction/broad-phase state is deliberately derived. Check stored kinematic/hold-related flags against snapshot semantics individually; do not require serializing transient contacts or impulses. | 3 |
| G17 | Angular acceleration, angular-velocity maximum and target setters are public. The rigid-body authoring record exposes initial angular velocity but not these independent fields. Angular acceleration and target have explicit state load/save routes; `angular_velocity_maximums` has no value load/save route in the state modules (F60). Close that persistence omission and distinguish it from ordinary angular velocity. | 2, 3 |

Common verification gaps apply to the finite field ledger, not future APIs:

| ID | Required evidence before closure | Owner |
|---|---|---|
| V2 | For each applicable property/behavior, compare direct and wrapper behavior, defaults/full replacement/focused mutation, invalid references and state preservation. Exercise editor controls and CLI with non-default values, undo/redo and unsupported-value diagnostics. Existing fixture names are starting evidence, not per-field assertions. | 2 |
| V3 | Trace each applicable field through JSON write/read, catalog conversion, generated C and runtime restoration; compare values, stable references and observable behavior. Cover incomplete topology, empty animations, rejected input rollback and repeated save/load without template accumulation. Explicitly retain N boundaries. | 3 |
| V4 | Execute equivalent direct construction and authored/generated instances; compare supported configuration, topology, local/world transforms, visual properties and observable behavior. Spawn twice, mutate/delete one, and verify the survivor. Cover the starter's whole-object spawn plus extra direct API content. | 4 |
| V5 | Execute affected installed-SDK consumers/examples on Linux from outside source directories, cross-build for Windows, and review graphics/audio/input on Linux where headless assertions cannot establish behavior. No native Windows execution gate. | 5 |

## Goal 2 configuration closure

The matrix and field ledgers above/below remain the frozen goal-1 baseline.
This section records the goal-2 delta; it does not mark V3–V5 complete. The
[current authored configuration contract](description_api_contract.md#authored-configuration-and-explicit-placement)
defines defaults, ownership boundaries, placement, migration, and CLI syntax.

| Gap | Configuration outcome and evidence |
|---|---|
| G01 | References remain within one authored object; independent spawned instances resolve their own members. Generic targets/drivers never transfer ownership. Removal/undo and duplicate self-references are covered by `editor_configuration_parity`; existing `object_instances`, `soft_body`, `joints`, `forces`, and `torques` retain direct ownership/connection coverage. Cross-instance gameplay wiring remains direct C. |
| G02 | Shared lifetime settings reach editor/CLI/JSON/catalog/generated spawning. Relative/absolute modes, exact 64-bit ticks, expiry and overflow rollback are covered by `editor_configuration_parity`. Initial source expiry retains its one-tick bound. |
| G03, G17 (configuration) | Axis/transform locks, gameplay target, initial angular acceleration and enabled angular maximum are authored with shared properties. Every shared field is exercised through CLI mutation and exact JSON round trip. Runtime reference resolution, world/object axis space, limits and independent spawning are asserted. Runtime angular-maximum snapshot restoration remains G17/V3. |
| G04–G05 | Static nodes and Auto/Explicit beam lengths reach all construction paths. Tests freeze measured length on opt-in, preserve explicit lengths during endpoint edits, and verify generated runtime values. |
| G06 | Connection configuration no longer places bodies. Explicit editor/CLI/direct/wrapper placement validates before mutation. `joints` and `editor_configuration_parity` cover all joint kinds, reversed arguments, world/static anchors, shared paths, connected groups, locks, owned helpers, soft bodies, particle nonrotation, idempotence and undo. World anchors are static in catalog and generated paths. |
| G07 | A zero-offset node anchor plus a pin is equivalent to the direct node-to-anchor helper. The fixture compares endpoint entity/offset, joint type/owner and whole-soft-body placement; no extra authored item is needed. |
| G08 | Signed sprite dimensions were supported by rendering but rejected by authoring. CLI/JSON now accept finite nonzero signed dimensions; bounds/picking use absolute sizes. The generated SDK fixture retains both flips, rotation and position. Direction/size rendering equivalence follows the same flip-bit calculation in `graphics_sprite_draw`/`graphics_texture_draw_flipped`; visual review remains V5. |
| G09–G10 | Independent camera following and pause settings reach preview, commands, JSON, catalog and generated runtime. Non-following positions retain object coordinates; the generated consumer checks attachment flags and initial pose. |
| G12–G13 | Mount render resolution, font point size, text wrap and text rotation are exposed and preserved. The installed generated fixture checks two render resolutions, rotated/clipped placements, custom font size, wrapping and repeated UI mounts. |
| G14–G15 | Settings → Application authors independent application-window configuration and named sound/music recipes. No playback is created. Generated getters/recipes verify non-default values, muted defaults and window configuration; command/UI tests cover validation and undo/redo. Audible/desktop acceptance remains V5. |
| G11, G16 | Unchanged: runtime named-layer registry and stored kinematic/hold snapshot semantics belong to goal 3. |

New public declarations are `physics_joint_place` and its `rohr_` wrapper:
1,604 current API entries versus the frozen 1,602. The field delta consists of
`ObjectEntityReference`, `ObjectLifetimeDescription`, `ObjectEntitySettings`,
shared entity settings on authored records/catalog descriptions, joint rest-mode
flags, node static state, beam rest-mode/value, camera follow/pause choices,
mount resolution, font point size, text wrap/rotation, and project window/audio
records. The original 456/507 field counts remain historical, not current counts.

Configuration verification uses existing description suites for replacement,
focused setters and invalid-input preservation (physics/body, particles,
soft-body descriptions, joints, force/torque, camera/viewport, assets, audio,
input, lifetime and object descriptions). New `editor_configuration_parity`,
`editor_configuration_controls` and `editor_configuration_generated` cover the
added paths. UI input tests exercise lifetime and application controls and
undo/redo; the 27-case placement matrix compares editor and runtime results and
checks unchanged poses on rejection. Runtime persistence and exhaustive
construction equivalence continue in goals 3 and 4; this does not replace those
goals with a configuration smoke test.

Verification on Linux, 2026-10-20:

- Full native build and CTest: 109/109 pass, including both API inventory checks,
  the installed SDK consumer, and generated configuration consumer.
- ASan/UBSan with leak checking: eight focused suites pass (`joints`,
  `editor_configuration_parity`, `editor_configuration_controls`, `editor_sources`,
  `editor_history`, `editor_project`, `editor_navigation`, `editor_beam_materials`).
  LeakSanitizer requires execution outside the process-inspection sandbox.
- Windows cross-build: engine/editor/tests and both standard/generated installed
  SDK consumers build. Native Windows execution was not performed.
- Independently installed-SDK examples: `center_of_mass`, `joints`, `soft-body`,
  `view-port`, `user-interface`, `audio`, and `player_controller` build for Linux
  and Windows. Each Linux example starts from `/tmp` with staged assets using
  dummy SDL video/audio and a bounded two-second startup run. These are startup
  checks, not interactive visual/audio acceptance.
- No compiler warnings in the native, cross, or sanitizer builds; Nix's dirty
  worktree notice is expected. `git diff --check` passes.

Reproduction: build with `nix develop -c cmake --build build --parallel 4`;
install into a fresh prefix and set `ROHR_SDK_INSTALL_PREFIX` when configuring
the consumer tests. Run `nix develop -c ctest --test-dir build --output-on-failure`.
`tools/editor_core/tests/configuration_generation.cmake` accepts the test
generator, font, consumer source, fixture directory and SDK prefix; adding
`TOOLCHAIN` performs the Windows build without execution. The verification used
fresh `/tmp/parity_goal2_sdk` and `/tmp/parity_goal2_sdk_windows` installations.

## Goal 3 persistence and generation closure

This closes G11, G16, G17's snapshot omission and V3 within the frozen
applicability boundaries. The earlier matrix and ledgers remain historical;
construction-equivalence acceptance (V4) and interactive acceptance (V5) remained
open at that boundary. V4 is closed in the goal 4 section below. Readers at that
boundary accepted only project format 11 and runtime-state format 9;
older and future versions fail transactionally. The frozen P4 migration case is
replaced by current-version round trips and unsupported-version rejection.
Public APIs and application-owned state boundaries are unchanged.
The later C10 corrective goal advances these to project 12/runtime 10 and
introduces directional response; its compatibility boundary is recorded below.

| Inventory scope | Persistence/generation evidence and outcome |
|---|---|
| P1: F01–F18, F53–F54, F58/F60 | `runtime_persistence` compares repeated snapshots and continued simulation with the same settings; `object_instances` covers particle/soft-body descriptions and ownership; `center_of_mass_persistence` covers COM, origin, material/filter and angular values. `editor_configuration_parity` now carries loaded project/catalog instances through runtime save/delete/load and checks limits, acceleration, targets, lifetime, locks, static nodes, beam length and cameras again. |
| G17: optional angular limit | Snapshot pool presence distinguishes absent, explicit zero and nonzero maximum. Negative/overflow input rejects the entire load. Public installed-SDK restoration is exercised too. |
| G16: stored versus derived body state | The explicit kinematic choice survives static/inactive states and subsequent dynamic/mass transitions. The effective kinematic flag is recomputed after loading. Dedicated assertions distinguish explicit from zero-mass inferred kinematics and verify held/static/inactive bodies remain stationary. Existing hold, locks, velocity and lifetime round trips remain covered. No contacts, broad-phase caches or solver impulses are serialized. |
| P2: F19–F28 | `runtime_persistence`, `object_instances`, `shared_sources`, `editor_sources`, `editor_core`, `editor_visibility` and `editor_torque` exercise references, independent ownership, anchors, joints, areas/holes, incomplete topology, source modes and marker styles through their applicable project/catalog/runtime routes. Multi-file rollback and continued simulation remain covered. |
| P3: F30–F33, F45/F61 | `animation_assets`, `empty_animations`, `object_instances`, `hitbox_animation_binding`, asset-description consumers and generated configuration checks cover asset identity/lifetime, transforms, frames/timing, empty players, bindings, signed sprite dimensions, font point size and text configuration. Shared immutable assets retain independent mutable instance state. |
| P4: F27–F44, F57 | Project/CLI round trips and generated consumers retain visibility, camera attachment/pause, screen resolution, text wrap/rotation, layer order and repeated mounts. `object_instances`, `camera_descriptions` and configuration runtime restoration cover object cameras. `editor_project`, `editor_core`, viewport, UI and layer suites retain existing configuration coverage. Persistent viewport mounts/UI remain reconstructed from generated definitions (N5); legacy state UI definitions keep their separate persistence path (N6). |
| G11: layer identity | Snapshots retain all named definitions in sorted order, including unused entries. Entity/sprite/animation bindings distinguish named, numeric and unset. Matching existing definitions are reused; conflicting definitions, invalid references and full registries roll back the load. Templates deduplicate authored records without capturing runtime edits. Allocation-failure tests verify newly registered layers are removed. |
| P5: F04, F46–F50, X04 | Project time values now read into `Time` without narrowing to float. Non-default engine and overridden physics timesteps remain exactly equal after reload and regenerate identical C. Configuration fixtures retain audio recipes, muted defaults and application presentation; existing input-generation and project tests retain controller/action configuration. Live audio/input/clocks remain outside snapshots (N2). |
| P6: F51–F56 | Two regenerations from a reloaded project compare all ten generated application/scene files byte for byte and preserve edited `main.c`. Installed-SDK generated instances retain separate targets, poses, beam lengths and angular limits after mutation/deletion. Repeated runtime snapshots do not accumulate authored templates. Invalid project/runtime versions preserve existing state. |
| P7: F02/F44/F59, X01–X03/X05–X07 | Existing group/template, legacy UI-definition and editor-state round trips remain applicable. Pure utilities, reports, storage helpers and live operations keep their N boundaries; this goal adds no serialized execution or new authoring API. |

See [the runtime contract](game_state.md#snapshot-settings-and-layer-identity)
for the version-9 layer/kinematic/angular-limit representation. Bundled runtime
files use version 9; bundled editor projects use format 11, including the
soft-body car's explicit torque hierarchy entry. Rejection tests replace old
version-migration assertions; optional fields still retain documented defaults.
Sanitizer verification also caught uninitialized optional camera follow/pause
flags in the project reader. Camera records now initialize before parsing, so
omitted fields retain false defaults. The bundled generated object modules were
refreshed with the current generator; developer entry points were preserved.

Verification completed 2026-10-21:

- Full Linux build and CTest: 109/109 pass, including the API/inventory checks,
  installed SDK consumers, generated configuration consumer and bundled COM
  generation comparison.
- ASan/UBSan with leak detection: 8/8 pass (`runtime_persistence`,
  `object_instances`, `empty_animations`, `animation_assets`,
  `center_of_mass_persistence`, `editor_configuration_parity`, `editor_torque`,
  `editor_visibility`). The configuration generator also passes with assets,
  reload/regeneration comparisons and edited-main preservation under sanitizers.
- Windows engine/editor/tests and both standard/generated installed-SDK
  consumers cross-build successfully. Native Windows execution was not performed.
- Eight independently installed-SDK examples build for Linux and Windows:
  `center_of_mass`, `soft-body`, `player_controller`, `game-state`, `pong`,
  `user-interface`, `fly-to-finish`, `flies-in-pit`. Linux launches from `/tmp`
  pass with staged assets; graphical examples use bounded two-second dummy-SDL
  startup checks, and `game-state` exits successfully after its assertions.
  These checks do not replace goal 5's interactive review.
- No compiler warnings or sanitizer findings remain. Nix emitted expected
  dirty-tree notices and occasional ignored concurrent-cache busy notices.
  Whitespace checks pass for goal changes; unrelated user edits in `notes.txt`
  were preserved.

Reproduction follows goal 2's commands with fresh `/tmp/parity_goal3_sdk` and
`/tmp/parity_goal3_sdk_windows` prefixes. The expanded
`configuration_generation.cmake` fixture uses the same Linux/Windows invocation
contract. Sanitizer tests use `ASAN_OPTIONS=detect_leaks=1` and
`UBSAN_OPTIONS=halt_on_error=1`, outside the process-inspection sandbox.

## Goal 4 authored and direct construction closure

V4 is complete within the frozen applicability boundaries. The independent
baseline uses individual entity, particle, soft-body, anchor, joint, force,
torque, camera, screen, viewport and asset APIs. It neither copies spawned
configuration nor builds a second `ObjectDescription` tree. The existing
per-feature suites remain part of the evidence; the new fixtures supplement
them with explicit cross-construction comparisons. No engine, public API,
project format or simulation behavior changes were needed.

The five acceptance cases are:

| Case and inventory scope | Passing evidence |
|---|---|
| 1. Physics configuration and behavior: P1, F01–F18, F53–F54, F58/F60 | [`editor_construction_parity.c`](../tools/editor_core/tests/editor_construction_parity.c) independently authors a rigid body, standalone particle and rotated soft body. [`construction_checks.h`](../tools/editor_core/tests/construction_checks.h) constructs their counterparts through individual APIs and compares motion, geometry, COM/inertia, materials, filters, node radii/colors and angular limits. Fresh engine runs compare 120 updates at 1/120 second, two substeps and eight solver iterations. Existing COM, description, lock, hold, contact/filter and runtime-persistence suites retain the remaining feature cases. |
| 2. Topology, sources and placement: P2, F19–F28 | The fixture compares ordered area/hole loops, measured/explicit beam lengths, world/node anchors, all three joint types, all 16 location/orientation combinations, persistent torque and initial-source expiry. References are normalized to logical roles instead of entity/anchor numbers. `editor_configuration_parity`, `editor_sources`, `shared_sources`, `editor_core`, `editor_visibility` and `editor_torque` retain body/shared anchors, source target modes, incomplete topology, placement/locks and marker geometry/style checks. |
| 3. Assets and composition: P3–P4, F27–F45/F61 | [`configuration_direct_checks.h`](../tools/editor_core/tests/configuration_direct_checks.h) compares an independently created attached camera, two screen sizes, viewport settings, clipped/rotated text, custom font metrics, slider fields/placement, signed sprite dimensions and animation frame transforms/timing/advancement against generated instances. Empty animations remain valid. Repeated generated mounts have independent slider state. Existing viewport, UI, layer, visibility, asset, hitbox-binding and editor suites cover screen placement/clipping, interaction, style geometry and visibility variants. |
| 4. Instance and resource independence: P1–P4/P6, F01/F16/F19/F30–F45/F51–F54 | [`construction_generated_consumer.c`](../tools/editor_core/tests/construction_generated_consumer.c) compares generated catalogs, convenience constructors and file-loaded instances against the same independent baseline. A translated convenience instance matches independently translated direct construction. Disconnecting endpoints, transferring a node, connecting different owners and deleting one owner preserve borrowed surviving nodes/beams/joints as specified. Mutation/deletion leaves a second instance unchanged. Asset/UI checks release original texture/font/animation owners, retain consumer dependencies, reject deletion of mounted UI, and preserve unrelated generated resources after teardown. |
| 5. Starter and application settings: P5–P7, F04/F46–F56 and runtime-only boundaries | The generated starter still spawns complete authored objects at the high level, then adds a rigid platform/box, standalone particle and soft body through individual APIs in `main.c`. Existing `editor_project` assertions guard that structure and developer-file ownership. The unchanged starter builds against both installed SDKs and runs from `/tmp`. Configuration consumers and existing input/audio/settings suites retain timestep/window recipes, controller/action configuration, muted defaults and explicit sound settings. Gameplay triggers and live service state remain C-owned under N1/N2/N5. |

Marker styling is retained `ObjectVisuals` metadata for object rendering, not a
new low-level setter on arbitrary joint/force entities. Its existing literal
geometry, authoring, loading and retained-appearance tests remain applicable;
this goal does not manufacture an individual styling API to replace that
contract. Interactive appearance, audio and input review remains V5.

Discrete values (roles, enums, flags, counts and colors) compare exactly.
Configuration floats use `1e-4 + 1e-5 * max(abs(a), abs(b))`; simulated values
use absolute tolerance `0.002`, with unwrapped degree orientations. Direct
composition fields compare exactly where no numerical transformation occurs.
No tolerances were loosened to make a discrepancy pass.

Verification completed 2026-10-21:

- Linux full build and CTest: **111/111 pass**, including the native comparison,
  both generated installed-SDK comparisons and inventory checks.
- ASan/UBSan with leak detection: **10/10 focused suites pass**:
  `editor_construction_parity`, `editor_configuration_parity`, `object_instances`,
  `empty_animations`, `viewport_descriptions`, `shared_sources`, `animation_assets`,
  `font_assets`, `text_assets` and `editor_sources`. Both fixture generators and
  both generated consumers also pass with the sanitizer-installed SDK.
- Windows engine/editor/tests, general installed-SDK consumer and both generated
  consumers cross-build. Native Windows execution was not performed.
- The starter and eight standalone examples independently build against fresh
  Linux and Windows SDKs: `center_of_mass`, `soft-body`, `player_controller`,
  `joints`, `user-interface`, `view-port`, `audio`, `game-state`. Linux launches
  from `/tmp` pass with staged assets. Graphical examples use bounded dummy-SDL
  startup checks; the starter also shuts down cleanly after an interrupt.
- No compiler warnings or sanitizer findings remain. Nix dirty-tree notices
  are expected. Unrelated user edits in `notes.txt` remain untouched.

Reproduce with goal 2's installed-SDK setup using fresh
`/tmp/parity_goal4_sdk` and `/tmp/parity_goal4_sdk_windows` prefixes. Enable
`ROHR_BUILD_SDK_CONSUMER_TESTS` and set `ROHR_SDK_INSTALL_PREFIX` before running:

```sh
nix develop -c ctest --test-dir build --output-on-failure
```

The new [`construction_generation.cmake`](../tools/editor_core/tests/construction_generation.cmake)
accepts `TEST_EXECUTABLE`, `FIXTURE`, `SDK_PREFIX` and optional `TOOLCHAIN`.
[`configuration_generation.cmake`](../tools/editor_core/tests/configuration_generation.cmake)
also requires `FONT` and `CONSUMER_SOURCE`. Both compile standalone generated
consumers and run native executables outside the source directory. For sanitizer
consumers, use an SDK installed from `ROHR_ENABLE_SANITIZERS=ON` and matching
consumer compile/link sanitizer flags; run with `ASAN_OPTIONS=detect_leaks=1`
and `UBSAN_OPTIONS=halt_on_error=1` outside the process-inspection sandbox.

## Goal 5 platform and interactive review

**Complete by user acceptance; unverified exceptions are retained above.**
Case 1 composition and presentation passed its corrective retests.
The user confirmed the two rotated screens and stable resizing, then reported
PASS after retesting the corrected layering and window/workspace switching.
The lower-left text's narrow wrapping, clockwise rotation and intentional
clipping were clarified before acceptance. Subsequent viewport-editor review
passed camera zoom, but found an unrotated background leaking beyond rotated
screens, misplaced simulation content and mismatched content-offset movement.
The next review also failed camera-content clipping: objects were cropped against
unrotated screen bounds. The user subsequently confirmed PASS after the camera
texture and rotated-clipping corrections. Cases 2–5 have only the evidence
recorded below; their outstanding checks are unverified exceptions to the
user-accepted closure of goal 5.

Further review reported build/lifecycle, authoring-panel, selection, and physics
control issues. The [corrective milestone](../NEXT_STEPS.md#corrective-milestone-for-goal-5)
tracks their ordered implementation goals, including the explicitly agreed
directional-response expansion. These findings do not revoke the accepted camera
preview correction or imply that the remaining acceptance cases passed.

The compositor now sorts numeric/named layers before content kinds, keeping UI
above screens only at equal layers. Tick-based physics advances each elapsed
tick separately with the configured substeps, rather than using one enlarged
duration after delayed rendering. It retains elapsed time, aggregates reports,
and stops on fatal errors; interaction queries describe the most recent tick,
possibly partial after a fatal error. Explicit-delta updates retain their existing
semantics. The review text
has moved below the button, with intentional clipping in its own rectangle.

Build `editor_interaction_fixture` and run it with a new/empty destination:

```sh
build/tests/editor_interaction_fixture /tmp/rohr_parity_review \
  examples/user-interface/assets/jetbrains_mono_bold_italic.ttf \
  third_party_licenses/jetbrains_mono_ofl.txt
```

The generator refuses an occupied destination through the workspace creation
API. It preserves the normal starter entry point, adds two overlapping rotated
screens and distinguishable UI layers, a slider, clipped custom-font text,
force/torque markers, independent spring endpoint styles, a mirrored sprite and
an empty animation. It stages the font license/attribution, saves and reloads
the project, and regenerates application and scene modules. The original
starter and existing user projects are unchanged.

Configure the review project's CMake build against an installed SDK, then launch
`/tmp/rohr_parity_review/build/RohrParityReview` from `/tmp`. For editor checks,
start `rohr-gui`, choose File → Open Project and select
`/tmp/rohr_parity_review`. Use real Linux graphics/audio backends for this review.
Reset between cases by reopening without saving edits, or generate a new
destination; never overwrite an existing user project to reset a test.

For the corrective retest, use the rebuilt project at
`/tmp/rohr_parity_review_fixed`, executable
`/tmp/rohr_parity_review_fixed/build/RohrParityReviewFixed`. Check red/green
layering, the separate lower-left clipped text, and several switches to a
browser and another Sway workspace, waiting at least two seconds before
returning. The original failed fixture remains available for comparison.

| Case | Actions and required observations | User-confirmed result |
|---|---|---|
| 1. Composition and presentation | The left/right screens have different rotation, placement and resolution. The left screen extends past the viewport edge and clips. The red bar draws over the left screen but under the right; the green panel stays above both. Text wraps, rotates and clips. Resize wider/taller without losing content or distorting aspect. In the viewport editor, move/rotate a screen and change camera zoom/content transforms; placement and content remain independent after rebuilding. | PASS after corrective retests: runtime layering, text wrapping/rotation/clipping, resizing, Sway stability and camera zoom were accepted earlier. The user then confirmed PASS for the corrected camera preview and rotated clipping. Earlier screen-editor failures are resolved. |
| 2. Visuals and visibility | Inspect `push_marker`, `spin_marker`, pin/weld/spring, the mirrored sprite, playing animations and `empty_animation`. Spring endpoints have different colors/scales/offsets. Toggle editor eyes separately from Visible in App, including the parent; rebuild and verify only app visibility affects runtime drawing and hidden bodies still simulate. Restore parent visibility without changing child settings. Empty animation must not prevent rendering. | Unverified; retained as an exception to user-accepted closure |
| 3. Editor interaction | Select geometry, drag from the selecting press, use child-selection and rotation handles, and undo/redo each gesture. Pick force targets/references from menus and the viewport, checking hover highlights. Scroll a long selector; check modal menus do not move underlying items. Drag the slider thumb without moving its parent. Rename via Enter/Escape and confirm subsequent keyboard navigation works. | User-reported failures: oversized force/torque picking, persistent/overlapping child panels, first-element activation, object/joint scrolling, and delayed joint clamping. C6 panel lifecycle/scrolling and C8 source picking complete with user acceptance; C14 immediate joint/axis placement and spring controls complete with user acceptance; broader interaction retests remain unverified exceptions to user-accepted closure. |
| 4. Input and audio | Exercise the starter's configured logical input actions and text entry without unintended gameplay input while editing. Run the installed-SDK `audio` example: Play Water Splash produces sound; music play/pause/resume and track switching work; Quit/escape shuts down. Review window size/aspect settings. Muted description defaults and queued/applied audio state retain their automated fixture evidence; silent startup alone is not proof of muted playback. | Unverified; retained as an exception to user-accepted closure |
| 5. Project lifecycle and delivery | Build/run the application and Simulate Scene. The application includes direct-API extras; scene simulation contains authored objects. Verify run/pause/stop, return to Editor, close/reopen/reorder tabs through View, rebuild a visible edit and replay. Close/switch a project during an active build and verify cancellation; terminal interrupt reaches the subprocess. Launch installed-SDK artifacts outside their source directories. | User-reported failures: relocation, repeat builds, missing CMake recovery, generation diagnostics, and incomplete-joint generation. Startup error 21 reproduced in rohr-project-1. C1 diagnostics, C2 relocation/repeat builds, C3 missing CMake recovery, C4 partial-spawn startup, and C5 incomplete joints complete with verification below; C6 panel lifecycle/scrolling and C8 source picking also complete; C7 complete with its manual checks explicitly deferred; C9 complete with the additional panel check explicitly skipped; C10 directional response and C11 explicit migration complete; C12 explicit motion authoring complete (including whole-body motion added at final review); C13 whole-body node physics complete; C14, C15, and C16 complete; broader interactive retests remain unverified exceptions to user-accepted closure. |

Preparation evidence (2026-10-21): full Linux build and **111/111 CTest tests
pass**, including the installed-SDK comparisons and scene/build lifecycle
regressions. The review application and scene compile against both the Linux
and Windows installed SDKs; both Linux executables pass external-directory
dummy-SDL startup and clean interrupt shutdown. Goal 4's independently built
eight examples and starter remain valid unchanged. Windows runtime execution is
not required. The new generator also cross-builds for Windows and passes
ASan/UBSan with leak detection while creating, saving, reloading and generating
the complete review project. These automated passes do not establish manual
results. Milestone 4 and goal 5 are now closed by user acceptance with the
unverified exceptions recorded above; retain the cases for later review and
record any future observations separately.

Corrective verification (2026-10-21): all 111 Linux tests pass, with three
scene-session/lifecycle tests rerun outside the sandbox to allow their local
sockets. New regressions check actual compositor submission order, named-layer
changes, equal-layer screen/UI ordering, independent visibility, delayed-frame
soft-body equivalence, aggregated tick reports and fatal catch-up termination.
Both focused suites and the corrected fixture generator pass ASan/UBSan with
leak detection. Full Windows cross-build and fresh Linux/Windows installed-SDK
consumer checks pass. The corrected application/scene and all nine examples
using tick-based physics build against both updated SDKs; Linux application,
scene and example startup/shutdown checks use dummy SDL outside source folders.
The user subsequently confirmed PASS for the runtime visual and Sway
workspace-switch retest. The later viewport-editor corrective retest also passed;
cases 2–5 remain required.

The subsequent screen-editor review reproduced an automatic-aspect bug using
the saved review project: clearing a 480×320 camera target changed its logical
presentation to 1080×720, shrinking and shifting its contents. Automatic window
aspect sizing now runs on the window target before camera rendering; background
clears only queue a clear. The editor draws the empty-camera background as a
rotated quad and clips stripe endpoints locally before rotation. Camera zoom
has user-confirmed acceptance; the subsequent corrective retest also passed.

The angle-dependent clipping correction renders each editor camera's object into
an explicitly owned, cached texture before fitting, scaling, offsetting and
rotating the result. Source geometry clips at the camera texture edge; the final
image clips only to its containing viewport, matching runtime composition.
Body and particle scanline fills use source-target dimensions during capture.
Queued source and composition commands retain their images, isolate clip/layer
state, preserve translucent colors, and release references on execution or
discard. The editor releases cached images on teardown or when no longer used;
previews do not consume application camera/screen handles.

Verification: 111/111 Linux tests, graphics/editor ASan/UBSan tests with leak
detection, the full Windows cross-build, and fresh Linux/Windows SDK consumers
pass. Regressions cover repeated camera clears at different target aspect ratios,
center projection, content-offset displacement and rotated background geometry.
Additional geometry/state checks cover camera-corner objects at five screen
angles, all four fit modes, two render sizes, viewport clipping, content
offset/scale/rotation, image reuse/resizing and queued resource lifetime.
The user's saved review application and scene were rebuilt without altering
authored screen settings. Both also build against the updated installed SDKs,
as does the viewport example; Linux artifacts pass startup/shutdown checks from
outside their source folders. The user confirmed PASS on the rebuilt editor's
camera-preview corrective retest. Continue the remaining review cases with
`build/tools/rohr-gui/rohr-gui` and `/tmp/rohr_parity_review_fixed`.

### Goal 5 corrective issue mapping

The review's full issue list maps to the ordered correction goals in
NEXT_STEPS.md. Entries remain pending unless explicitly marked complete below.
Existing automated evidence above predates these corrections;
each pursued goal needs its own relevant checks and interactive retest.

| Reported issue or agreed clarification | Correction goal |
|---|---|
| Moving the project leaves project-owned paths stale | C2 complete; shared root resolution and stale cache/output recovery |
| Missing project CMake files should regenerate during build | C3 complete; missing root/scene scaffolding restored and disposable build metadata regenerated |
| Build Settings should expose executable, engine, and relevant paths | C7; every listed path is editable, with defaults and resettable overrides |
| Building again fails instead of using the valid cache | C2 complete; unchanged/edited builds retain valid caches |
| Force picking includes excessive space around/behind the arrow; rotation controls need padding | C8 |
| Torque picking is slightly too large | C8 |
| Soft-body controls remain after returning to the object editor | C6 |
| Soft-body Physics lacks collision, gravity, and motion controls | C12 motion and C13 gravity/collision/materials complete; body edits change only the selected property or mask bit on nodes and new-node defaults; later per-node edits remain available |
| Other child editors also remain after returning to top level | C6 |
| Object joint editor does not scroll | C6 |
| Joint clamping waits for a later movement | C14 complete; immediate property/axis commits and connected dragging, atomic constraints, one undo step per gesture, and user acceptance |
| Render Layer belongs in Appearance, not Transform | C9 |
| Anchors and other editor visuals lack a layer option | C9 |
| Opening an element blocks object scrolling and leaves overlaying controls | C6 |
| Opening the first object element glitches its editor over the object editor | C6 |
| Generate C failures and all build-stage diagnostics must reach the terminal | C1 complete; output retained with manual-only terminal visibility |
| An incompletely defined joint breaks the build/run path | C5 |
| Invalid entity at startup in /tmp/rohr-projects/rohr-project-1 | C4 complete; generated presentation tolerates unspawned objects and runtime missing cameras |
| Asymmetric masks should permit one-sided physical response | C10 |
| Kinematic should be exposed as an explicit editor motion mode | C12 |
| Preserve old pair behavior by remapping categories/masks into the new system | C11 |
| Neither side responding must not suppress geometric overlap queries | C10 |
| One-sided contacts use both bodies' materials | C10 |
| Current errors need red outlines, rows, fields, and collapsed accordion headers | C15 complete; shared derived diagnostics clear styling after correction |
| Existing errors must reach the notification log when opening a project | C15 complete; unchanged snapshots are deduplicated, returning errors log again, and history remains visible |

Read-only diagnosis reproduced `error 21: invalid entity` when launching the
saved rohr-project-1 application with dummy SDL drivers. Its developer-owned
main.c creates starter and tug, while generated viewport setup also assigns
layers to unspawned pendulum entities. C4 addresses this path without rewriting
developer gameplay code; verification is recorded below.

The engine already accepts joints with unset endpoints and retains their
definitions when the physics pipeline skips unavailable contributions with a
warning. C5 extends this behavior through editor object-document conversion and
generated constructors while preserving validation of malformed data.

**C1 verification:** generation/build diagnostics always reach the embedded
terminal, regardless of visibility or optional generated-tree output. The build
output toggle has been removed. Stage messages and original errors cover
generation, configuration, compilation, process startup, and launch preparation;
notifications are retained and direct users to terminal details. No operation
automatically opens or focuses the terminal. Scrollback remains bounded by the
existing terminal capacity.

The `editor_build_diagnostics` regression invokes the actual GUI controller with
temporary projects and real CMake/compiler subprocesses. It verifies generation
failure before subprocess creation, successful generation with the tree disabled,
configure/compiler failures, invalid Lua, missing commands/executables, and
cancellation followed by a successful build. Output remains readable after the
user opens the previously hidden terminal. All 111 existing Linux tests passed;
the new regression passed twice after correcting its temporary-directory reuse.
The full Linux build includes examples. Focused `editor_build` and
`editor_build_diagnostics` ASan/UBSan tests passed, and the Windows editor and both
test executables cross-compiled without compiler warnings. Windows execution was
not performed. Broader interactive acceptance remains open.

**C2 verification:** an isolated copy of rohr-project-1 reproduced CMake's old
source/build-directory errors. Shared editor/CLI recovery now archives stale
cache/output metadata, reconfigures using saved settings, and prevents resolving
executables in the original project. The affected copy compiled successfully
after recovery and on repeated builds; a final unchanged build compiled no C
objects. No separate same-location build failure was reproduced.

The installed-SDK `editor_build_relocation` fixture verifies initial, unchanged,
source-edited, copied, and moved builds through editor-core and CLI paths. It
launches application and scene targets outside their source directories, preserves
developer source/CMake/configuration/assets and unrelated build files, retains
cache-only options on normal builds, and discards them only on relocation while
reapplying saved options. The original copy's cache remains unchanged. Unit tests
cover disabled configuration, interrupted recovery, and linked-build protection.
All 113 Linux tests passed, including this SDK fixture; focused ASan/UBSan checks
and Windows editor/CLI/test cross-compilation passed. Windows execution was not
performed.

**C3 verification:** shared editor/CLI build preparation restores missing root
and scene CMake files from the same templates used for new projects. It preserves
existing files, including customized scene CMake integration during generation.
Compile-only recovery does not regenerate C sources or modify developer inputs.
Custom commands receive recovery; disabled configure commands stay disabled.
Default-directory CMake builds report missing metadata with instructions to
re-enable configuration, while custom builders validate their own metadata.
Standalone Generate C still leaves a missing root CMake file absent.

The installed-SDK `editor_build_scaffolding` fixture removes root CMake, scene
CMake, cache, CMakeFiles, native build files, and the full build directory in
separate cases. Editor-core and CLI builds recover successfully, retain valid
cache-only settings, preserve file hashes, and avoid recompiling unchanged
applications. Both application and scene targets launch outside their source
directories. Unit tests cover unreadable files, failed writes, invalid path
types, dangling links, partial restoration diagnostics, and custom/disabled
commands. The actual GUI controller verifies recovery and pre-command failures
in a hidden terminal without changing visibility or focus.

All 114 Linux tests passed. The full Linux build includes examples; the SDK
fixture independently compiles and runs generated application/scene examples.
Focused ASan/UBSan build/diagnostics tests passed. Windows editor, CLI, and both
build test executables cross-compiled without compiler warnings; Windows
execution was not performed.

**C4 verification:** an isolated copy of rohr-project-1 reproduced startup error
21 before regeneration. After regeneration/build against the updated SDK it
entered and sustained its application loop without the startup error. Its
main.c hash remained identical to the original; the original project was not
modified. Generated layer setup and drawing skip zero entity handles, while
nonzero stale entities still fail. Screen references missing from authored data
fail generation with viewport/screen/object/camera context before file writes.

`editor_partial_spawning` builds the default generated project against the
installed SDK and runs partial spawning, complete spawning, and an unspawned
camera owner. It verifies warning-only presentation for the missing camera,
recovery after creating its owner, invalid nonzero entity rejection, preserved
main.c, and complete scene startup outside the source directory. Generation
tests verify missing-object/missing-camera diagnostics and unchanged output on
failure. Graphics tests cover zero/stale camera references, camera deletion,
screen lifetime, explicit replacement, hidden mounts, shared-screen warning
deduplication, direct/public pipeline parity, and retained report evidence.

Geometry interception verifies rotated stripe endpoints stay inside screen
bounds, viewport clipping is active, and missing screens do not sample old
camera textures. The user passed the real-window visual check for rotated and
partially clipped construction placeholders, a live-camera screen, and UI
between and above the screen layers. The repeatable visual fixture is
`build/tests/graphics_reports_test --missing-camera-demo`.

The full Linux suite passed 114/115 checks; its only failure identified the
committed center-of-mass example's outdated generated files. After refreshing
those files, the example comparison, generation, and SDK partial-spawn checks
all passed. The full Linux build includes examples; the center-of-mass example
also built independently against the SDK and ran from outside its source tree
until the intentional smoke-test timeout. Focused ASan/UBSan checks passed.
Windows editor/CLI, graphics/reporting, viewport, and generation tests
cross-compiled without compiler warnings; native Windows execution was not
performed.

**C5 verification:** editor conversion retains unset joint endpoints instead of
rejecting the object, and direct generated constructors emit these joints rather
than omitting them. Soft-node anchors have generated fields and bindings, and
are created once after their nodes exist. Invalid nonzero authored anchor IDs,
negative spring parameters, and nonfinite joint values still fail conversion.
The generation-diagnostics fixture now uses an invalid nonzero anchor reference.
The physics pipeline and public API semantics are unchanged.

`editor_incomplete_joints` checks pin, weld, and spring joints with zero endpoints,
only A, only B, and both, using rigid-body, world, and soft-node anchors. It covers
editor and runtime persistence, retained identity/settings/visuals, relationship
commands, report warnings, and completion of the same runtime entity. `joints`
also verifies free motion while incomplete and physical response after completion.
`editor_incomplete_joints_generated` compiles and runs direct constructors,
compiled catalogs, JSON loading, and physics-only spawning against the installed
SDK outside the source directory; it also builds and starts Simulate Scene.
All 117 Linux tests and three focused ASan/UBSan tests passed. Linux examples
built, and Windows editor/CLI and joint tests cross-compiled without compiler
warnings. Native Windows execution was not performed.

**C6 verification:** panel identity follows the open editor and item, including
same-type siblings, rather than the selected child row. Navigation cancels old
dropdown, field, pointer, and scroll capture. Back/Close, editor visibility, and
the name stay fixed above measured scrolling properties. New editors start at
the top; only Back restores parent scroll, clamped to current content. History
is bounded and project-local, with deleted entries pruned. Shared settings use
the open item; object property commands now have their own undo snapshot.

`editor_panel_lifecycle` covers fresh child/sibling entry, Back-only restoration,
deletion/project reset, stale dropdown and field cancellation, fixed-header
clicks, clipped rows, measured content, clamped offsets, nested wheel routing,
and keyboard scrolling. It also draws the real object, rigid-body, and hitbox
panels through the return sequence and verifies wheel input without an off-panel
click. `editor_configuration_controls` verifies that selected child rows do not
redirect object edits and that undo preserves the body's previous setting.

All 118 Linux tests passed, including integration retries outside the sandbox
for local scene sockets. Five focused ASan/UBSan checks passed with leak
detection outside the sandbox: panel lifecycle, navigation, UI reports, history,
and configuration controls. The added test's panel-cache cleanup order was fixed
after sanitizer detection. Linux examples built; the UI example also built
independently against a fresh installed SDK and ran from `/tmp` with dummy
drivers until its three-second smoke-test timeout, without runtime errors.
Windows editor and panel/configuration tests cross-compiled without compiler
warnings; native Windows execution was not performed.

The user confirmed all five C6 interactive checks PASS in the identified current
editor window. An earlier reported scroll failure cleared after switching
windows and could not be reproduced. Two editor builds were open; the cause of
that transient observation is unconfirmed. C7–C14 and the remaining broader
interactive acceptance checks remain open.

**C7 complete; manual verification explicitly deferred:** Build
Settings now exposes ten shared path locations, with scrolling, resolved paths,
Browse, Reset, and fixed Apply/Cancel controls. Workspace path changes retain
original content until copies are verified and metadata commits; conflicts,
unsupported copy entries, and metadata failures preserve the old workspace.
Nested generated/state paths follow their parent; external locations stay put
when the project moves. Explicit output paths control compilation and launch.
Managed CMake templates consume shared paths and record their configured values,
so unchanged builds reuse caches while changed settings block stale launches.
Custom main.c is preserved; custom scaffolding requires explicit compatibility
before relocation. Applying settings cancels project children, releases handles,
refreshes previews, and clears authored undo history.

`editor_workspace_paths` covers defaults, persisted overrides, nested locations,
external assets, project moves, resets, conflicts, symbolic-link ancestors,
copy-failure rollback, metadata-failure rollback, custom-scaffold rejection,
asset resolution, and command substitutions. `editor_workspace_paths_build`
builds and launches installed-SDK applications/scenes from external paths with
spaces through CLI and the GUI's controller; checks main.c preservation, asset
staging, cache retention, output changes, and stale-launch rejection. A separate
run moved a project from `/tmp` to `/dev/shm` and back successfully. Existing
scaffold tests now assert the configurable CMake interface.

The 120-test Linux suite passed; four focused ASan/UBSan checks passed with leak
detection outside the sandbox (workspace paths, build processes, editor project,
reference picking). Linux examples built, and installed-SDK application/scene
startup checks passed. Windows GUI, CLI, path tests, and build tests
cross-compiled; native Windows execution has not been performed. Final focused
regressions, sanitizer checks, and Windows cross-builds also passed after the
nested-path, preview-font, and durable metadata-recovery follow-up fixes. The
120-test suite passed before the final metadata-backup-only change.

The user explicitly deferred C7's five manual checks; they remain **unverified**,
not passed. Revisit them after milestone implementation and during incremental
full-game testing. This is a C7-only exception: other manual checks remain
required unless the user explicitly chooses to defer them when presented.
Use the C7 review editor with
`/tmp/c7-path-review`: verify all rows and fixed actions; Apply custom assets,
generated, build, application, and scene paths; build and simulate; reopen to
check persistence; relocate the project while preserving `/tmp/c7-sdk`; reject
an occupied generated-source destination without losing content. C7 is closed
with this recorded verification exception. C8 continues below; C9–C14 remain
untouched.

**C8 complete; manual review passed:** Force and torque
annotation picking follows the drawn shaft rectangles and shared arrowhead
polygons. Both shafts use 8.32 / 1.4 screen pixels; shorter chevron heads use 70%
of that thickness and a joined tip. Drawing and picking consume the same head
polygons. The force origin circle is decorative; empty torque centers and gaps
allow underlying-body selection. Force rotation handles retain their original
size and 14-pixel click buffer. App visual polygon picking remains unchanged.
Returning to a previously edited body now replaces the shared selection list,
preventing stale force/torque highlights.

Force/torque regressions cover exact edges and joined tips, empty regions,
underlying-body selection, zoom, signed/zero magnitude, visibility, rotation
controls, dragging and history. Both pass at the current geometry, including
regressions that reproduced the stale-selection failure before its fix.

Final verification after all refinements: all 120 Linux tests pass, including
installed-SDK application and scene integration; Linux editor and examples
build without compiler warnings. Five focused ASan/UBSan tests (force, torque,
navigation, visibility and history) pass with leak detection outside the
sandbox. Windows editor and force/torque tests cross-compile without compiler
warnings; native Windows execution was not performed. `git diff --check HEAD`
passes. Verification logs: `/tmp/c8_review_build.log`,
`/tmp/c8_review_tests.log`, `/tmp/c8_review_sanitizer_build.log`,
`/tmp/c8_sanitizer_tests.log` and `/tmp/c8_review_windows_build.log`.

User verification: PASS in the updated editor after the final geometry and
selection-handoff refinements. Review fixture: `/tmp/c8-picking-review`. The five
checks cover nearby-body selection and clearing the old highlight; hidden/app
visuals; zoom and transformed arrows; overlap and context-menu targeting; and
dragging/rotation with one-step undo/redo. Earlier review requests are resolved
by this accepted refinement; C9–C14 remain unstarted. C7's deferred manual checks
remain unverified.

**C9 complete; additional panel confirmation explicitly skipped:** Render Layer controls
live under Appearance, including the source/joint panels previously labeled
Visuals. Anchors, camera guides, forces, and torques now have numeric/named
bindings. Each item shares one binding across its visuals; editor annotations
draw above application visuals, with deterministic layer ordering and matching
selection/context targeting within each category. Anchors and camera guides
remain editor-only, and soft-child inheritance is preserved.

Bindings survive JSON, old-document defaults, duplicate/copy, undo/redo, named
layer deletion, CLI commands, runtime import, and generated object catalogs.
The layer command and compact history snapshots cover direct Appearance edits
and registry changes without adding public engine APIs.

Verification: 120/120 Linux tests pass after refreshing the installed SDK used
by generated-consumer tests; the earlier stale-SDK failure is resolved. Linux
editor/examples and Windows editor/tests cross-build without compiler warnings.
Five focused ASan/UBSan tests pass with leak detection. Native Windows execution
was not performed. Logs: `/tmp/c9_final_tests.log`, `/tmp/c9_sanitizer_tests.log`,
`/tmp/c9_build.log`, and `/tmp/c9_windows_build.log`.

The user confirmed PASS for the five review cases in
`/tmp/c9-appearance-review`. A final audit then found missing GUI context wiring
for the force, torque, joint, anchor, and camera layer controls. That omission
is corrected; both platform builds and eight affected tests pass
(`/tmp/c9_wiring_build.log`, `/tmp/c9_wiring_windows_build.log`,
`/tmp/c9_wiring_tests.log`). The user explicitly skipped the additional focused
review of those five panels in “C9 review - final appearance controls”. That
check remains unverified; the earlier review PASS is retained. C9 closes with
this recorded verification exception.
C10–C14 remain untouched; C7's deferred manual checks remain unverified.

**C10 complete — directional collision response:** Masks now decide each side's
contact response independently. Only responding, simulated bodies contribute
inverse mass/inertia to correction and normal/friction impulses. Nonresponders'
motion remains part of relative velocity, and both materials retain the existing
combination rules. This applies to rigid bodies, particles, soft nodes, and
enabled beam boundaries. Beam endpoint cutouts only suppress equivalent initial
node contacts; full geometric overlap is retained independently of masks.

The C API uses `responds_to`, `collision_response_set/all_set/none_set`,
`collision_response_to_check`, and `collision_response_between_check` (with
equivalent `rohr_` wrappers). Old names are removed. Mask queries do not report
actual contact. Editor controls, command/history paths, CLI `responds-to`, JSON,
runtime snapshots, and generated code use the new names. Project format 12 and
runtime format 10 reject older files before replacing state, with a
migration-required diagnostic retained through runtime object loading. C11 now
provides the explicit migration described below. Shipped car hubs have an explicit category
and room exclusion; Pong walls exclude paddles in both directions. These targeted
fixture updates preserve their prior allowed/excluded relationships.

Five acceptance cases:

1. `collision_response` checks all four mask combinations across rigid, particle,
   and node pairs with both allocation orders, inverse query normals, and
   unchanged nonresponders. Particle/node pairs also assert expected velocities.
2. Beam cases check both sides independently, weighted endpoint response,
   friction, and overlap without contact. `soft_body` and `beam_contacts` retain
   seam/cutout, sensor, material, and endpoint-order regressions.
3. Moving/rotating nonresponders, off-center angular response, Static/Kinematic
   restrictions, restitution and zero combined friction are verified against
   explicit expected values and the corresponding kinematic reference case.
4. A third body's independent response leaves the original pair unchanged;
   response-mask changes end contact while geometric overlap stays active.
5. Editor controls/CLI/history, project import and generated/installed-SDK
   catalogs, runtime persistence, and rejection of prior project/snapshot
   versions pass. The authored-car regression compares all 36 directed profile
   pairs against the previous mutual-filter behavior.

Verification: 121/121 Linux tests pass (`/tmp/c10_final_tests.log`), including
scene subprocess integration outside the sandbox; the initial sandbox run
could not create loopback listeners. After the final example refinements,
`collision_response`, `example_soft_body_angles`, and `authored_car` pass.
Seven focused ASan/UBSan checks pass with leak detection
(`/tmp/c10_sanitizer_tests.log`, `/tmp/c10_car_sanitizer_tests.log`). Nine affected
examples independently build against the installed SDK and launch from `/tmp`;
interactive examples exit cleanly after interrupt, and game-state exits normally.
Evidence: `/tmp/c10_examples.log`, `/tmp/c10_examples_remaining.log`,
`/tmp/c10_examples_final.log`, and `/tmp/c10_car_sdk_final.log` (the first harness
used the target name instead of PlayerController's output name; the continuation
corrects it). Windows engine/editor/CLI, response/persistence tests, car/Pong
examples, and the car regression cross-build without compiler warnings.
Final platform logs: `/tmp/c10_windows_final_build.log`,
`/tmp/c10_windows_examples_build.log`, `/tmp/c10_car_windows_build.log`.
Native Windows execution was not performed. C10 did not implement C11–C14;
subsequent C11 evidence follows. C7/C9 manual-review exceptions remain unchanged.

**C11 complete — explicit behavior-preserving collision migration:**
`rohr-cli migrate-collisions preview|apply FILE.json [FILE.json ...]` supports
editor project 11 → 12 and runtime state/snapshot 9 → 10. Preview is read-only;
apply preserves exact originals and a JSON mapping report beside the inputs.
Loaders remain strict. There is no automatic conversion or legacy simulation
mode. Developer-owned C, assets and manifests are not rewritten.

Migration retains category bits when they can express the old mutual relation.
Otherwise it groups equal response neighborhoods or uses exact reversed legacy
bit-pair channels, with deterministic names and a per-profile mapping. It verifies
both new response directions against every old ordered pair, including repeated
instances of one profile. All interacting files must be supplied together;
unseen files, future edits and entities created in developer-owned C require
manual mask review. [Usage, recovery and bounded capacity](collision_migration.md)
document the supported encodings rather than claiming a minimum Boolean encoding.

Five acceptance cases:

1. Read-only deterministic preview, explicit application, exact original bytes,
   strict supported versions, duplicate-key rejection, and malformed numeric
   filters are covered by `editor_collision_migration`.
2. Shared categories with different filters retain every acceptance/exclusion
   in both directions. Coverage includes zero/all filters, all 64 original bits,
   a remapped 64-neighborhood boundary, and 130 profiles encoded via bit pairs.
3. Project rigid bodies, standalone particles, soft nodes and disabled beams
   survive migration and save/reload. Runtime templates, implicit default
   filters, node/beam topology snapshots and subsequent snapshots load correctly.
4. A provably over-capacity 66-target case, stale inputs and occupied sidecars
   refuse without modifying inputs. Injected failure after a prior file has
   been replaced restores the entire batch. Originals remain available for
   manual recovery after process interruption; multi-file crash atomicity is
   not claimed.
5. `editor_collision_migration_generated` runs CLI preview/apply, validates the
   project, regenerates C, verifies the developer-owned main's checksum, builds
   against the installed SDK, and checks the old relationships through both
   generated catalogs and runtime object loading from outside the source tree.

Verification: all **123/123 Linux tests** passed (`/tmp/c11_final_tests.log`).
The focused migration suite passed ASan/UBSan with leak detection
(`/tmp/c11_sanitizer_tests.log`), including the injected replacement failure.
Linux engine/editor/CLI/tests built; the new migration module also passed
`-Wall -Wextra -Wpedantic -Werror`. The standalone game-state example independently
built against the installed SDK and exited successfully from `/tmp`
(`/tmp/c11_example_build.log`, `/tmp/c11_example_run.log`). Windows CLI and
migration tests cross-built (`/tmp/c11_windows_build.log`); native Windows
execution was not performed. Final audit tightened rejection of explicitly
empty/incomplete legacy filters and bounded generated category labels without
compiler warnings. Both migration tests then passed again
(`/tmp/c11_audit_tests.log`), as did ASan/UBSan with leak detection
(`/tmp/c11_audit_sanitizer_tests.log`) and the Windows cross-build
(`/tmp/c11_audit_windows_build.log`). No rendering behavior changed or new manual visual
gate was introduced. C7/C9's explicit review exceptions remain unchanged.
C12 implementation and verification follow; remaining C13–C14 work is deferred.

## Frozen fixture plan

These are comparison cases for this inventory, not tests implemented in goal 1.
Existing fixture links are listed in the evidence index. Goals 2–5 may extend
those fixtures or add focused fixtures to exercise the listed properties; they
must not substitute one broad smoke test for the field comparisons.

| Case | Properties/behaviors and expected comparison |
|---|---|
| P1 Entity/physics configuration | F01–F18, F53–F54, F58/F60: non-default motion, mass/material/filter, COM/origin, locks, standalone particle restrictions, node motion and beam rest/collision settings. Compare description/getter values and applicable post-update behavior. Include ownership separate from connections and deletion. |
| P2 Topology and sources | F19–F28: area outer/holes and disconnected slots; world/body/node anchors; pin/weld/spring; force location/orientation modes and initial/persistent force/torque. Compare references, offsets, order, application and independent instances. Use G06's placement cases. |
| P3 Assets and animation | F30–F33, F45/F61: shared textures/fonts, per-instance transforms, signed scale, frame IDs/timing/hitbox bindings, empty animation and replacement/destruction. Compare owned lifetimes and restored configuration. |
| P4 Composition and UI | F27–F29, F34–F44, F57: at least two screens with different placement/rotation/size/layer, partial clipping, UI between/above screens, independent screen/UI/app/editor visibility, camera attachment, text and sliders, repeated mounts and legacy migration. Check state assertions plus Linux visual review. |
| P5 Services and input | F04, F46–F50, X04: default/non-default settings, muted versus explicit audible volume, queued versus applied audio state, controller/action defaults and overrides, keyboard/mouse/text routing, window presentation. Runtime-only triggers remain C. |
| P6 Loading and tools | F51–F56: direct/catalog/file-loaded objects, repeated spawns, runtime snapshots versus retained templates, invalid-reference rollback, generation determinism, simulation object/viewport selection, preserved developer entry point, external-directory launch and SDK packaging. |
| P7 Support APIs | X01–X03, X05–X07, F02/F44/F59 runtime-only portions: reconcile public/direct entry points and their existing fixtures. Do not manufacture editor equivalents for pure utilities, reports, storage or immediate drawing. |

Use exact equality for IDs/enums/flags/strings and retained discrete configuration.
For floating-point conversions and simulated results, use the existing feature
contract and fixture tolerances, recording any missing tolerance as a pre-goal
clarification. Match elapsed simulation time and configuration between paths;
physics robustness/performance targets belong to milestone 6. Compare degree
angles with the contract's unwrapped semantics, not arbitrary modulo equivalence.
Fixture payloads and expected assertions are frozen against this field inventory;
any additional numeric choices are settled before their owning implementation goal.

## Source and existing-test evidence

Shared authoring routes are [project types](../tools/editor/src/editor_project.h),
[project mutations](../tools/editor/src/editor_project.c),
[command definitions](../tools/editor_core/include/editor_command.h),
[CLI parsing/writing](../tools/editor_core/src/editor_cli_standard.c),
[project JSON](../tools/editor/src/editor_project_json.c),
[project-to-catalog conversion](../src/entity/object_document.c),
[catalog writing](../tools/editor_core/src/editor_object_catalog.c),
[workspace generation](../tools/editor/src/editor_workspace.c),
[object spawning](../src/entity/object_spawn.c), and
[public wrappers](../src/core/rohr.c).
For each matrix row, combine these applicable routes with its evidence bundle.
A test file's presence is not a claim that it asserts every property in the row.
Missing assertions are V2/V3/V4 work, with exact property IDs from the ledgers.

| Bundle | Source routes | Existing fixtures; not executed in this goal |
|---|---|---|
| <a id="evidence-entity"></a>entity | [src/entity/entity_components.c](../src/entity/entity_components.c); [src/entity/entity_info.c](../src/entity/entity_info.c); [src/state/game_state.c](../src/state/game_state.c) | [tests/object_instances.c](../tests/object_instances.c); [tests/entity_info_adversarial.c](../tests/entity_info_adversarial.c); [tests/runtime_persistence.c](../tests/runtime_persistence.c) |
| <a id="evidence-settings"></a>settings | [src/physics/pipeline/physics_config.c](../src/physics/pipeline/physics_config.c); [src/core/engine.c](../src/core/engine.c); [tools/editor/src/panels/editor_physics_settings_panel.c](../tools/editor/src/panels/editor_physics_settings_panel.c); [tools/editor/src/editor_workspace.c](../tools/editor/src/editor_workspace.c) | [tests/timing.c](../tests/timing.c); [tools/editor/tests/editor_project.c](../tools/editor/tests/editor_project.c) |
| <a id="evidence-geometry"></a>geometry | [src/physics/collision/hitbox.c](../src/physics/collision/hitbox.c); [src/physics/auto_shape.c](../src/physics/auto_shape.c); [tools/editor/src/editors/geometry/editor_hitbox.c](../tools/editor/src/editors/geometry/editor_hitbox.c) | [tests/hitbox_animation_binding.c](../tests/hitbox_animation_binding.c); [tests/auto_shape.c](../tests/auto_shape.c); [tools/editor_core/tests/editor_core.c](../tools/editor_core/tests/editor_core.c) |
| <a id="evidence-motion"></a>motion | [src/physics/body/motion_state.c](../src/physics/body/motion_state.c); [src/physics/body/body_state.c](../src/physics/body/body_state.c); [tools/editor/src/editors/physics/editor_rigid_body.c](../tools/editor/src/editors/physics/editor_rigid_body.c); [src/state/game_state.c](../src/state/game_state.c) | [tests/angles.c](../tests/angles.c); [tests/constraint_solver.c](../tests/constraint_solver.c); [tools/editor_core/tests/editor_core.c](../tools/editor_core/tests/editor_core.c) |
| <a id="evidence-mass"></a>mass | [src/physics/body/mass_properties.c](../src/physics/body/mass_properties.c); [tools/editor/src/editors/physics/editor_center_of_mass.c](../tools/editor/src/editors/physics/editor_center_of_mass.c) | [tests/center_of_mass.c](../tests/center_of_mass.c); [tests/origin_offsets.c](../tests/origin_offsets.c); [tools/editor_core/tests/center_of_mass_persistence.c](../tools/editor_core/tests/center_of_mass_persistence.c) |
| <a id="evidence-material"></a>material | [src/physics/collision/collision_material.c](../src/physics/collision/collision_material.c); [tools/editor_core/src/editor_object_commands.c](../tools/editor_core/src/editor_object_commands.c) | [tests/beam_materials.c](../tests/beam_materials.c); [tools/editor/tests/editor_beam_materials.c](../tools/editor/tests/editor_beam_materials.c) |
| <a id="evidence-collision"></a>collision | [src/physics/collision/collision_filter.c](../src/physics/collision/collision_filter.c); [tools/editor/src/editors/editor_collision_controls.c](../tools/editor/src/editors/editor_collision_controls.c) | [tests/collision_sensor.c](../tests/collision_sensor.c); [tools/editor/tests/editor_collision_controls.c](../tools/editor/tests/editor_collision_controls.c); [tools/editor_core/tests/editor_core.c](../tools/editor_core/tests/editor_core.c) |
| <a id="evidence-particle"></a>particle | [src/physics/particles/particle.c](../src/physics/particles/particle.c); [tools/editor/src/editors/physics/editor_particle.c](../tools/editor/src/editors/physics/editor_particle.c) | [tests/particle_collision_mode.c](../tests/particle_collision_mode.c); [tools/editor/tests/editor_particle_authoring.c](../tools/editor/tests/editor_particle_authoring.c) |
| <a id="evidence-soft"></a>soft | [src/physics/soft_body/soft_body_state.c](../src/physics/soft_body/soft_body_state.c); [src/physics/soft_body/soft_body_frame.c](../src/physics/soft_body/soft_body_frame.c); [src/entity/object_document.c](../src/entity/object_document.c); [tools/editor/src/editors/soft_body/editor_soft_node.c](../tools/editor/src/editors/soft_body/editor_soft_node.c) | [tests/soft_body_frame.c](../tests/soft_body_frame.c); [tests/soft_body_mass.c](../tests/soft_body_mass.c); [tests/soft_body_angular_velocity.c](../tests/soft_body_angular_velocity.c); [tools/editor/tests/editor_soft_mass_authoring.c](../tools/editor/tests/editor_soft_mass_authoring.c) |
| <a id="evidence-beam"></a>beam | [src/physics/soft_body/soft_body_state.c](../src/physics/soft_body/soft_body_state.c); [src/entity/object_document.c](../src/entity/object_document.c); [tools/editor/src/editors/soft_body/editor_soft_beam.c](../tools/editor/src/editors/soft_body/editor_soft_beam.c) | [tests/beam_contacts.c](../tests/beam_contacts.c); [tests/beam_materials.c](../tests/beam_materials.c); [tests/soft_body.c](../tests/soft_body.c) |
| <a id="evidence-area"></a>area | [src/physics/soft_body/soft_body_area.c](../src/physics/soft_body/soft_body_area.c); [tools/editor_core/src/editor_soft_area.c](../tools/editor_core/src/editor_soft_area.c); [tools/editor/src/editors/soft_body/editor_soft_area.c](../tools/editor/src/editors/soft_body/editor_soft_area.c) | [tests/node_loop_geometry.c](../tests/node_loop_geometry.c); [tests/soft_body_areas.c](../tests/soft_body_areas.c); [tools/editor/tests/editor_area_authoring.c](../tools/editor/tests/editor_area_authoring.c); [tools/editor_core/tests/editor_soft_areas.c](../tools/editor_core/tests/editor_soft_areas.c) |
| <a id="evidence-joint"></a>joint | [src/physics/joints/joint.c](../src/physics/joints/joint.c); [tools/editor/src/editor_project.c](../tools/editor/src/editor_project.c); [tools/editor/src/editors/physics/editor_joint.c](../tools/editor/src/editors/physics/editor_joint.c); [src/entity/object_spawn.c](../src/entity/object_spawn.c) | [tests/joints.c](../tests/joints.c); [tests/constraint_solver.c](../tests/constraint_solver.c); [tools/editor_core/tests/editor_core.c](../tools/editor_core/tests/editor_core.c) |
| <a id="evidence-source"></a>source | [src/physics/body/force_state.c](../src/physics/body/force_state.c); [src/entity/object_source.c](../src/entity/object_source.c); [tools/editor_core/src/editor_source_commands.c](../tools/editor_core/src/editor_source_commands.c); [tools/editor/src/editor_sources_json.c](../tools/editor/src/editor_sources_json.c); [tools/editor/src/editors/physics/editor_force.c](../tools/editor/src/editors/physics/editor_force.c) | [tests/forces.c](../tests/forces.c); [tools/editor_core/tests/editor_sources.c](../tools/editor_core/tests/editor_sources.c); [tools/editor/tests/editor_force.c](../tools/editor/tests/editor_force.c); [tools/editor/tests/editor_torque.c](../tools/editor/tests/editor_torque.c) |
| <a id="evidence-visual"></a>visual | [src/entity/object_visuals.c](../src/entity/object_visuals.c); [src/state/runtime_visuals.c](../src/state/runtime_visuals.c); [tools/editor/src/editor_sources_json.c](../tools/editor/src/editor_sources_json.c) | [tools/editor/tests/editor_visibility.c](../tools/editor/tests/editor_visibility.c); [tools/editor/tests/editor_force.c](../tools/editor/tests/editor_force.c); [tools/editor/tests/editor_torque.c](../tools/editor/tests/editor_torque.c); [tests/runtime_persistence.c](../tests/runtime_persistence.c) |
| <a id="evidence-editing"></a>editing | [tools/editor/src/editor_history.c](../tools/editor/src/editor_history.c); [tools/editor/src/editors/editor_reference_picker.c](../tools/editor/src/editors/editor_reference_picker.c); [tools/editor/src/editor_navigation.c](../tools/editor/src/editor_navigation.c) | [tools/editor/tests/editor_history.c](../tools/editor/tests/editor_history.c); [tools/editor/tests/editor_reference_picker.c](../tools/editor/tests/editor_reference_picker.c); [tools/editor/tests/editor_navigation.c](../tools/editor/tests/editor_navigation.c) |
| <a id="evidence-texture"></a>texture | [src/graphics/texture_assets.c](../src/graphics/texture_assets.c); [src/entity/object_spawn.c](../src/entity/object_spawn.c) | [tests/texture_assets.c](../tests/texture_assets.c); [tests/installed_sdk_consumer/asset_descriptions.c](../tests/installed_sdk_consumer/asset_descriptions.c) |
| <a id="evidence-sprite"></a>sprite | [src/graphics/graphics.c](../src/graphics/graphics.c); [src/entity/object_document.c](../src/entity/object_document.c); [tools/editor/src/editors/render/editor_sprite.c](../tools/editor/src/editors/render/editor_sprite.c); [src/state/runtime_visuals.c](../src/state/runtime_visuals.c) | [tests/texture_assets.c](../tests/texture_assets.c); [tests/runtime_persistence.c](../tests/runtime_persistence.c); [tools/editor_core/tests/editor_core.c](../tools/editor_core/tests/editor_core.c) |
| <a id="evidence-animation"></a>animation | [src/graphics/animation_assets.c](../src/graphics/animation_assets.c); [src/graphics/graphics.c](../src/graphics/graphics.c); [tools/editor/src/editors/render/editor_animated_sprite.c](../tools/editor/src/editors/render/editor_animated_sprite.c); [tools/editor/src/editor_workspace.c](../tools/editor/src/editor_workspace.c) | [tests/animation_assets.c](../tests/animation_assets.c); [tests/empty_animations.c](../tests/empty_animations.c); [tests/hitbox_animation_binding.c](../tests/hitbox_animation_binding.c); [tools/editor_core/tests/editor_core.c](../tools/editor_core/tests/editor_core.c) |
| <a id="evidence-camera"></a>camera | [src/graphics/graphics.c](../src/graphics/graphics.c); [src/graphics/camera_state.h](../src/graphics/camera_state.h); [tools/editor/src/editors/render/editor_camera.c](../tools/editor/src/editors/render/editor_camera.c); [src/entity/object_spawn.c](../src/entity/object_spawn.c) | [tests/cameras.c](../tests/cameras.c); [tests/camera_descriptions.c](../tests/camera_descriptions.c); [tools/editor_core/tests/editor_core.c](../tools/editor_core/tests/editor_core.c); [tests/runtime_persistence.c](../tests/runtime_persistence.c) |
| <a id="evidence-layers"></a>layers | [src/graphics/graphics_layer_order.c](../src/graphics/graphics_layer_order.c); [src/graphics/graphics.c](../src/graphics/graphics.c); [tools/editor/src/editor_project_json.c](../tools/editor/src/editor_project_json.c) | [tests/graphics_layer_order.c](../tests/graphics_layer_order.c); [tools/editor/tests/editor_layers.c](../tools/editor/tests/editor_layers.c) |
| <a id="evidence-viewport"></a>viewport | [src/graphics/graphics.c](../src/graphics/graphics.c); [tools/editor/src/editors/layout/editor_layout_viewport.c](../tools/editor/src/editors/layout/editor_layout_viewport.c); [tools/editor/src/editor_workspace.c](../tools/editor/src/editor_workspace.c) | [tests/viewport_descriptions.c](../tests/viewport_descriptions.c); [tests/graphics_layer_order.c](../tests/graphics_layer_order.c); [tools/editor/tests/editor_project.c](../tools/editor/tests/editor_project.c) |
| <a id="evidence-ui"></a>ui | [src/graphics/graphics.c](../src/graphics/graphics.c); [tools/editor/src/editors/layout/editor_layout_viewport.c](../tools/editor/src/editors/layout/editor_layout_viewport.c); [tools/editor/src/editor_workspace.c](../tools/editor/src/editor_workspace.c) | [tests/viewport_descriptions.c](../tests/viewport_descriptions.c); [tests/ui_instance_isolation.c](../tests/ui_instance_isolation.c); [tools/editor/tests/editor_project.c](../tools/editor/tests/editor_project.c) |
| <a id="evidence-immediate"></a>immediate | [src/graphics/ui.c](../src/graphics/ui.c); [src/state/game_state.c](../src/state/game_state.c) | [tests/ui_dropdown.c](../tests/ui_dropdown.c); [tests/ui_field.c](../tests/ui_field.c); [tests/ui_field_selection.c](../tests/ui_field_selection.c); [tests/runtime_persistence.c](../tests/runtime_persistence.c) |
| <a id="evidence-font"></a>font | [src/graphics/font_assets.c](../src/graphics/font_assets.c); [src/graphics/text_assets.c](../src/graphics/text_assets.c); [tools/editor/src/editor_workspace.c](../tools/editor/src/editor_workspace.c) | [tests/font_assets.c](../tests/font_assets.c); [tests/text_assets.c](../tests/text_assets.c); [tests/installed_sdk_consumer/asset_descriptions.c](../tests/installed_sdk_consumer/asset_descriptions.c) |
| <a id="evidence-audio"></a>audio | [src/audio/audio.c](../src/audio/audio.c); [include/audio.h](../include/audio.h) | [tests/audio.c](../tests/audio.c); [tests/audio_reports.c](../tests/audio_reports.c); [tests/installed_sdk_consumer/audio_descriptions.c](../tests/installed_sdk_consumer/audio_descriptions.c) |
| <a id="evidence-input"></a>input | [src/input/input.c](../src/input/input.c); [include/input.h](../include/input.h); [src/scene/scene_input.c](../src/scene/scene_input.c); [tools/editor/src/panels/editor_input_settings_panel.c](../tools/editor/src/panels/editor_input_settings_panel.c); [tools/editor/src/editor_workspace.c](../tools/editor/src/editor_workspace.c) | [tests/input_actions.c](../tests/input_actions.c); [tools/editor_core/tests/editor_core.c](../tools/editor_core/tests/editor_core.c); [tools/editor/tests/editor_project.c](../tools/editor/tests/editor_project.c) |
| <a id="evidence-window"></a>window | [src/graphics/window_presentation.c](../src/graphics/window_presentation.c); [tools/editor/src/panels/editor_visual_settings_panel.c](../tools/editor/src/panels/editor_visual_settings_panel.c); [tools/editor_core/src/editor_config.c](../tools/editor_core/src/editor_config.c) | [tests/window_presentation.c](../tests/window_presentation.c); [tools/editor_core/tests/editor_config.c](../tools/editor_core/tests/editor_config.c) |
| <a id="evidence-object"></a>object | [src/entity/object_description.c](../src/entity/object_description.c); [src/entity/object_document.c](../src/entity/object_document.c); [src/entity/object_spawn.c](../src/entity/object_spawn.c); [tools/editor_core/src/editor_object_catalog.c](../tools/editor_core/src/editor_object_catalog.c) | [tests/object_instances.c](../tests/object_instances.c); [tests/installed_sdk_consumer/object_descriptions.c](../tests/installed_sdk_consumer/object_descriptions.c); [tools/editor/tests/editor_project.c](../tools/editor/tests/editor_project.c) |
| <a id="evidence-state"></a>state | [src/state/game_state.c](../src/state/game_state.c); [src/state/runtime_topology.c](../src/state/runtime_topology.c); [src/state/runtime_visuals.c](../src/state/runtime_visuals.c) | [tests/runtime_persistence.c](../tests/runtime_persistence.c); [tests/installed_sdk_consumer/runtime_persistence.c](../tests/installed_sdk_consumer/runtime_persistence.c) |
| <a id="evidence-operations"></a>operations | [src/physics/physics_operation.c](../src/physics/physics_operation.c); [src/physics/collision/interaction_query.c](../src/physics/collision/interaction_query.c); [src/physics/collision/contact_query.c](../src/physics/collision/contact_query.c); [src/physics/collision/overlap_query.c](../src/physics/collision/overlap_query.c) | [tests/physics_interaction_set.c](../tests/physics_interaction_set.c); [tests/contact_manifold.c](../tests/contact_manifold.c); [tests/collision_sensor.c](../tests/collision_sensor.c) |
| <a id="evidence-math"></a>math | [src/math/math2d.c](../src/math/math2d.c) | [tests/angles.c](../tests/angles.c); [tests/auto_shape.c](../tests/auto_shape.c) |
| <a id="evidence-report"></a>report | [src/core/report.c](../src/core/report.c); [src/core/report_output.c](../src/core/report_output.c); [src/entity/entity_info.c](../src/entity/entity_info.c); [src/core/error.c](../src/core/error.c) | [tests/system_reports.c](../tests/system_reports.c); [tests/report.c](../tests/report.c); [tests/report_output.c](../tests/report_output.c); [tests/entity_info.c](../tests/entity_info.c) |
| <a id="evidence-storage"></a>storage | [include/memory.h](../include/memory.h); [src/tools/component_codegen.c](../src/tools/component_codegen.c) | [tests/component_registry.c](../tests/component_registry.c); [tests/generated_components_runtime.c](../tests/generated_components_runtime.c); [tests/memory_pool_example.c](../tests/memory_pool_example.c) |
| <a id="evidence-lifecycle"></a>lifecycle | [src/core/engine.c](../src/core/engine.c); [src/scene/scene.c](../src/scene/scene.c); [src/physics/pipeline/physics_pipeline.c](../src/physics/pipeline/physics_pipeline.c) | [tests/engine_lifecycle.c](../tests/engine_lifecycle.c); [tests/scene_runner.c](../tests/scene_runner.c); [tests/system_reports.c](../tests/system_reports.c) |
| <a id="evidence-draw"></a>draw | [src/graphics/graphics.c](../src/graphics/graphics.c) | [tests/graphics_reports.c](../tests/graphics_reports.c); [tests/graphics_layer_order.c](../tests/graphics_layer_order.c) |
| <a id="evidence-tools"></a>tools | [src/tools/tools.c](../src/tools/tools.c); [src/core/rohr.c](../src/core/rohr.c) | [tests/directory.c](../tests/directory.c); [tests/installed_sdk_consumer/main.c](../tests/installed_sdk_consumer/main.c) |
| <a id="evidence-scene"></a>scene | [tools/editor/src/editor_workspace.c](../tools/editor/src/editor_workspace.c); [tools/editor_core/src/editor_scene_session.c](../tools/editor_core/src/editor_scene_session.c); [tools/editor/src/panels/editor_scene_settings_panel.c](../tools/editor/src/panels/editor_scene_settings_panel.c) | [tools/editor_core/tests/editor_scene_session.c](../tools/editor_core/tests/editor_scene_session.c); [tools/editor_core/tests/editor_scene_generation.c](../tools/editor_core/tests/editor_scene_generation.c); [tools/cli/tests/scene_selection.cmake](../tools/cli/tests/scene_selection.cmake) |
| <a id="evidence-build"></a>build | [tools/editor/src/editor_workspace.c](../tools/editor/src/editor_workspace.c); [tools/editor_core/src/editor_build.c](../tools/editor_core/src/editor_build.c); [tools/editor_core/src/editor_build_process.c](../tools/editor_core/src/editor_build_process.c) | [tools/editor/tests/editor_project.c](../tools/editor/tests/editor_project.c); [tools/editor_core/tests/editor_build.c](../tools/editor_core/tests/editor_build.c); [tests/run_installed_sdk_consumer.cmake](../tests/run_installed_sdk_consumer.cmake) |

## Exact configuration-field ledger

The public ledger covers **80 types and 456 field paths**; the editor ledger
covers **44 types and 507 field paths**. Counts include nested union alternatives
but not array expansion. Each field occurs once per type; multiple fields share a
row only when they inherit the same path assessment. A referenced nested type
has its own ledger entry. Explicit exceptions in G01–G17 override broad groups.

Value types (`Color`, `Scale`, `UIRect`, `ViewportRectangle`) describe numbers used
by their containing property; they do not independently require authored objects.
Legacy and persistent UI structs remain distinct. `UIButtonStyle` and
`UISliderStyle`/`UISliderConfig` serve both legacy definitions (F44/S) and live
immediate calls (F59/N for persistence of interaction state). Editor item caches
mirror reusable definitions; those caches are not separate stored settings.

<details>
<summary>Public configuration and nested values</summary>

| Type / source | Exact fields | Primary feature group |
|---|---|---|
| [AnimatedSpriteDescription](../include/graphics.h) | `AnimatedSpriteDescription.frames`, `AnimatedSpriteDescription.frame_count`, `AnimatedSpriteDescription.direction`, `AnimatedSpriteDescription.scale`, `AnimatedSpriteDescription.body_offset`, `AnimatedSpriteDescription.orientation_offset`, `AnimatedSpriteDescription.follow_entity_rotation`, `AnimatedSpriteDescription.visible` | [F32](#f32) |
| [AnimatedSpriteDescription](../include/graphics.h) | `AnimatedSpriteDescription.player` | [F33](#f33) |
| [AnimationDescription](../include/graphics.h) | `AnimationDescription.id`, `AnimationDescription.frame_files`, `AnimationDescription.frame_ids`, `AnimationDescription.frame_count`, `AnimationDescription.ticks_per_frame`, `AnimationDescription.time_per_frame` | [F32](#f32) |
| [AnimationFrameDescription](../include/graphics.h) | `AnimationFrameDescription.id`, `AnimationFrameDescription.texture`, `AnimationFrameDescription.scale`, `AnimationFrameDescription.offset`, `AnimationFrameDescription.rotation` | [F32](#f32) |
| [AnimationPlayerDescription](../include/graphics.h) | `AnimationPlayerDescription.animation`, `AnimationPlayerDescription.ticks_per_frame`, `AnimationPlayerDescription.time_per_frame` | [F33](#f33) |
| [AxisLockDescription](../include/physics.h) | `AxisLockDescription.axis`, `AxisLockDescription.point_on_axis` | [F11](#f11) |
| [CameraAttachmentDescription](../include/graphics.h) | `CameraAttachmentDescription.entity`, `CameraAttachmentDescription.position_offset`, `CameraAttachmentDescription.orientation_offset`, `CameraAttachmentDescription.follow_position`, `CameraAttachmentDescription.follow_orientation` | [F35](#f35) |
| [CameraDescription](../include/graphics.h) | `CameraDescription.position`, `CameraDescription.orientation`, `CameraDescription.dimensions`, `CameraDescription.zoom`, `CameraDescription.enabled` | [F34](#f34) |
| [CameraDescription](../include/graphics.h) | `CameraDescription.pause_with_engine` | [F36](#f36) |
| [CameraDescription](../include/graphics.h) | `CameraDescription.render_callback`, `CameraDescription.render_context` | [F37](#f37) |
| [CollisionFilterDescription](../include/physics.h) | `CollisionFilterDescription.category`, `CollisionFilterDescription.responds_to` | [F09](#f09) |
| [Color](../include/math2d.h) | `Color.red`, `Color.green`, `Color.blue`, `Color.alpha` | [X01](#x01) |
| [EntityDescription](../include/entity_components.h) | `EntityDescription.name`, `EntityDescription.owner` | [F01](#f01) |
| [EntityLifetimeDescription](../include/entity_components.h) | `EntityLifetimeDescription.expiry_time`, `EntityLifetimeDescription.expiry_tick`, `EntityLifetimeDescription.time_enabled`, `EntityLifetimeDescription.tick_enabled` | [F03](#f03) |
| [FontDescription](../include/graphics.h) | `FontDescription.file` | [F45](#f45) |
| [FontDescription](../include/graphics.h) | `FontDescription.point_size` | [F61](#f61) |
| [ForceDescription](../include/physics.h) | `ForceDescription.owner` | [F01](#f01) |
| [ForceDescription](../include/physics.h) | `ForceDescription.target`, `ForceDescription.magnitude` | [F24](#f24) |
| [ForceDescription](../include/physics.h) | `ForceDescription.location`, `ForceDescription.orientation` | [F25](#f25) |
| [ForceLocation](../include/physics.h) | `ForceLocation.offset`, `ForceLocation.reference` | [F25](#f25) |
| [ForceOrientation](../include/physics.h) | `ForceOrientation.offset`, `ForceOrientation.reference` | [F25](#f25) |
| [ForceOrientationReference](../include/physics.h) | `ForceOrientationReference.kind`, `ForceOrientationReference.entity`, `ForceOrientationReference.between.from`, `ForceOrientationReference.between.to` | [F25](#f25) |
| [ForcePositionReference](../include/physics.h) | `ForcePositionReference.kind`, `ForcePositionReference.entity`, `ForcePositionReference.beam.entity`, `ForcePositionReference.beam.anchor`, `ForcePositionReference.beam.fraction` | [F25](#f25) |
| [HitboxAnimationBinding](../include/physics.h) | `HitboxAnimationBinding.animation_id`, `HitboxAnimationBinding.frame_id`, `HitboxAnimationBinding.hitbox_id` | [F33](#f33) |
| [InputActionDescription](../include/input.h) | `InputActionDescription.controller`, `InputActionDescription.name`, `InputActionDescription.type`, `InputActionDescription.button_mode`, `InputActionDescription.button_initial_state`, `InputActionDescription.bindings_default` | [F48](#f48) |
| [InputBinding](../include/input.h) | `InputBinding.name`, `InputBinding.source`, `InputBinding.input.key`, `InputBinding.input.mouse_button`, `InputBinding.input.axis_component`, `InputBinding.modifiers`, `InputBinding.affects_x`, `InputBinding.affects_y`, `InputBinding.scale`, `InputBinding.inverted_x`, `InputBinding.inverted_y`, `InputBinding.direction` | [F48](#f48) |
| [InputBindingList](../include/input.h) | `InputBindingList.values`, `InputBindingList.count` | [F48](#f48) |
| [InputControllerDescription](../include/input.h) | `InputControllerDescription.name`, `InputControllerDescription.enabled` | [F48](#f48) |
| [JointAnchorDescription](../include/physics.h) | `JointAnchorDescription.entity`, `JointAnchorDescription.local_offset` | [F20](#f20) |
| [JointDescription](../include/physics.h) | `JointDescription.owner` | [F01](#f01) |
| [JointDescription](../include/physics.h) | `JointDescription.type`, `JointDescription.anchor_a`, `JointDescription.anchor_b`, `JointDescription.rest_length`, `JointDescription.stiffness`, `JointDescription.damping`, `JointDescription.rest_angle`, `JointDescription.rest_length_explicit`, `JointDescription.rest_angle_explicit` | [F21](#f21) |
| [LayerDescription](../include/graphics.h) | `LayerDescription.name`, `LayerDescription.value` | [F38](#f38) |
| [MusicDescription](../include/audio.h) | `MusicDescription.path`, `MusicDescription.volume`, `MusicDescription.loop`, `MusicDescription.playback_rate` | [F46](#f46) |
| [ObjectAreaGeometry](../include/object.h) | `ObjectAreaGeometry.outer`, `ObjectAreaGeometry.holes`, `ObjectAreaGeometry.hole_count` | [F19](#f19) |
| [ObjectAreaLoop](../include/object.h) | `ObjectAreaLoop.nodes`, `ObjectAreaLoop.node_count` | [F19](#f19) |
| [ObjectCollisionDescription](../include/object.h) | `ObjectCollisionDescription.enabled`, `ObjectCollisionDescription.filter` | [F09](#f09) |
| [ObjectColorDescription](../include/object.h) | `ObjectColorDescription.color`, `ObjectColorDescription.border_color`, `ObjectColorDescription.overridden` | [F28](#f28) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.geometry`, `ObjectDescription.geometry_count`, `ObjectDescription.active_geometry` | [F05](#f05) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.position`, `ObjectDescription.orientation`, `ObjectDescription.velocity`, `ObjectDescription.acceleration`, `ObjectDescription.angular_velocity` | [F06](#f06) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.mass`, `ObjectDescription.center_of_mass_explicit`, `ObjectDescription.center_of_mass` | [F07](#f07) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.friction`, `ObjectDescription.restitution` | [F08](#f08) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.static_body`, `ObjectDescription.gravity_enabled`, `ObjectDescription.collision_enabled`, `ObjectDescription.collision_filter` | [F09](#f09) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.rotation_locked` | [F10](#f10) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.radius`, `ObjectDescription.particle_origin`, `ObjectDescription.particle_vertices` | [F12](#f12) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.thickness` | [F18](#f18) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.area` | [F19](#f19) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.endpoint_a`, `ObjectDescription.endpoint_b`, `ObjectDescription.joint_type`, `ObjectDescription.rest_length`, `ObjectDescription.stiffness`, `ObjectDescription.damping`, `ObjectDescription.rest_angle` | [F21](#f21) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.force` | [F24](#f24) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.torque` | [F26](#f26) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.visuals` | [F27](#f27) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.visible`, `ObjectDescription.color`, `ObjectDescription.border_color`, `ObjectDescription.color_overridden` | [F28](#f28) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.asset_path`, `ObjectDescription.size`, `ObjectDescription.scale` | [F31](#f31) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.frames`, `ObjectDescription.frame_count` | [F32](#f32) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.hitbox_bindings`, `ObjectDescription.hitbox_binding_count`, `ObjectDescription.ticks_per_frame`, `ObjectDescription.time_per_frame`, `ObjectDescription.starting_frame`, `ObjectDescription.direction`, `ObjectDescription.playing` | [F33](#f33) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.follow_position`, `ObjectDescription.follow_orientation`, `ObjectDescription.zoom` | [F35](#f35) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.layer` | [F38](#f38) |
| [ObjectDescription](../include/object.h) | `ObjectDescription.id`, `ObjectDescription.name`, `ObjectDescription.kind`, `ObjectDescription.categories`, `ObjectDescription.parent`, `ObjectDescription.children`, `ObjectDescription.child_count`, `ObjectDescription.target` | [F51](#f51) |
| [ObjectForceDescription](../include/object.h) | `ObjectForceDescription.mode`, `ObjectForceDescription.target`, `ObjectForceDescription.magnitude` | [F24](#f24) |
| [ObjectForceDescription](../include/object.h) | `ObjectForceDescription.location`, `ObjectForceDescription.orientation` | [F25](#f25) |
| [ObjectForceLocation](../include/object.h) | `ObjectForceLocation.kind`, `ObjectForceLocation.offset`, `ObjectForceLocation.reference`, `ObjectForceLocation.anchor`, `ObjectForceLocation.fraction` | [F25](#f25) |
| [ObjectForceOrientation](../include/object.h) | `ObjectForceOrientation.kind`, `ObjectForceOrientation.offset`, `ObjectForceOrientation.reference`, `ObjectForceOrientation.from`, `ObjectForceOrientation.to` | [F25](#f25) |
| [ObjectFrame](../include/object.h) | `ObjectFrame.id`, `ObjectFrame.path`, `ObjectFrame.scale`, `ObjectFrame.offset`, `ObjectFrame.rotation` | [F32](#f32) |
| [ObjectHitbox](../include/object.h) | `ObjectHitbox.id`, `ObjectHitbox.shape` | [F05](#f05) |
| [ObjectMaterialDescription](../include/object.h) | `ObjectMaterialDescription.friction`, `ObjectMaterialDescription.restitution` | [F08](#f08) |
| [ObjectSourceReference](../include/object.h) | `ObjectSourceReference.kind`, `ObjectSourceReference.id`, `ObjectSourceReference.soft_body` | [F25](#f25) |
| [ObjectSpawnDescription](../include/object.h) | `ObjectSpawnDescription.flags`, `ObjectSpawnDescription.offset` | [F51](#f51) |
| [ObjectTorqueDescription](../include/object.h) | `ObjectTorqueDescription.mode`, `ObjectTorqueDescription.target`, `ObjectTorqueDescription.magnitude` | [F26](#f26) |
| [ObjectVisualStyle](../include/object.h) | `ObjectVisualStyle.specified`, `ObjectVisualStyle.fill_color`, `ObjectVisualStyle.border_color`, `ObjectVisualStyle.border_thickness`, `ObjectVisualStyle.scale`, `ObjectVisualStyle.offset`, `ObjectVisualStyle.rotation` | [F27](#f27) |
| [ObjectVisuals](../include/object.h) | `ObjectVisuals.main`, `ObjectVisuals.endpoint_a`, `ObjectVisuals.endpoint_b` | [F27](#f27) |
| [ParticleDescription](../include/physics.h) | `ParticleDescription.owner` | [F01](#f01) |
| [ParticleDescription](../include/physics.h) | `ParticleDescription.position`, `ParticleDescription.velocity`, `ParticleDescription.acceleration` | [F06](#f06) |
| [ParticleDescription](../include/physics.h) | `ParticleDescription.mass_value` | [F07](#f07) |
| [ParticleDescription](../include/physics.h) | `ParticleDescription.friction`, `ParticleDescription.restitution` | [F08](#f08) |
| [ParticleDescription](../include/physics.h) | `ParticleDescription.collision_filter`, `ParticleDescription.static_body`, `ParticleDescription.gravity_enabled`, `ParticleDescription.collision_enabled` | [F09](#f09) |
| [ParticleDescription](../include/physics.h) | `ParticleDescription.local_origin`, `ParticleDescription.radius`, `ParticleDescription.rigid_vertices` | [F12](#f12) |
| [RohrToolsComponentDefinition](../include/rohr_tools.h) | `RohrToolsComponentDefinition.name`, `RohrToolsComponentDefinition.type_name`, `RohrToolsComponentDefinition.type_declaration`, `RohrToolsComponentDefinition.default_value` | [X03](#x03) |
| [Scale](../include/graphics.h) | `Scale.x`, `Scale.y` | [X01](#x01) |
| [SceneCallbacks](../include/scene.h) | `SceneCallbacks.context`, `SceneCallbacks.initialize`, `SceneCallbacks.update`, `SceneCallbacks.shutdown` | [X04](#x04) |
| [ScreenDescription](../include/graphics.h) | `ScreenDescription.camera`, `ScreenDescription.width`, `ScreenDescription.height` | [F39](#f39) |
| [Shape](../include/math2d.h) | `Shape.amount_of_vertices`, `Shape.vertices` | [F05](#f05) |
| [Shape](../include/math2d.h) | `Shape.concave_pieces`, `Shape.concave_piece_count`, `Shape.collision_geometry_prepared` | [X03](#x03) |
| [SoftBodyAreaDescription](../include/physics.h) | `SoftBodyAreaDescription.owner` | [F01](#f01) |
| [SoftBodyAreaDescription](../include/physics.h) | `SoftBodyAreaDescription.geometry`, `SoftBodyAreaDescription.draw_color`, `SoftBodyAreaDescription.draw_color_overridden`, `SoftBodyAreaDescription.visible` | [F19](#f19) |
| [SoftBodyAreaGeometry](../include/physics.h) | `SoftBodyAreaGeometry.outer`, `SoftBodyAreaGeometry.holes`, `SoftBodyAreaGeometry.hole_count` | [F19](#f19) |
| [SoftBodyAreaLoop](../include/physics.h) | `SoftBodyAreaLoop.nodes`, `SoftBodyAreaLoop.node_count` | [F19](#f19) |
| [SoftBodyBeamCollisionDescription](../include/physics.h) | `SoftBodyBeamCollisionDescription.enabled`, `SoftBodyBeamCollisionDescription.thickness`, `SoftBodyBeamCollisionDescription.filter` | [F18](#f18) |
| [SoftBodyBeamDescription](../include/physics.h) | `SoftBodyBeamDescription.owner` | [F01](#f01) |
| [SoftBodyBeamDescription](../include/physics.h) | `SoftBodyBeamDescription.friction`, `SoftBodyBeamDescription.restitution` | [F08](#f08) |
| [SoftBodyBeamDescription](../include/physics.h) | `SoftBodyBeamDescription.node_a`, `SoftBodyBeamDescription.node_b`, `SoftBodyBeamDescription.stiffness`, `SoftBodyBeamDescription.damping`, `SoftBodyBeamDescription.draw_color`, `SoftBodyBeamDescription.draw_color_overridden` | [F16](#f16) |
| [SoftBodyBeamDescription](../include/physics.h) | `SoftBodyBeamDescription.rest_length`, `SoftBodyBeamDescription.rest_length_explicit` | [F17](#f17) |
| [SoftBodyBeamDescription](../include/physics.h) | `SoftBodyBeamDescription.collision` | [F18](#f18) |
| [SoftBodyDescription](../include/physics.h) | `SoftBodyDescription.owner` | [F01](#f01) |
| [SoftBodyDescription](../include/physics.h) | `SoftBodyDescription.position`, `SoftBodyDescription.orientation` | [F13](#f13) |
| [SoftBodyNodeAnchorPinDescription](../include/physics.h) | `SoftBodyNodeAnchorPinDescription.owner` | [F01](#f01) |
| [SoftBodyNodeAnchorPinDescription](../include/physics.h) | `SoftBodyNodeAnchorPinDescription.node`, `SoftBodyNodeAnchorPinDescription.anchor` | [F23](#f23) |
| [SoftBodyNodeDescription](../include/physics.h) | `SoftBodyNodeDescription.owner` | [F01](#f01) |
| [SoftBodyNodeDescription](../include/physics.h) | `SoftBodyNodeDescription.position`, `SoftBodyNodeDescription.velocity`, `SoftBodyNodeDescription.acceleration` | [F06](#f06) |
| [SoftBodyNodeDescription](../include/physics.h) | `SoftBodyNodeDescription.mass` | [F07](#f07) |
| [SoftBodyNodeDescription](../include/physics.h) | `SoftBodyNodeDescription.friction`, `SoftBodyNodeDescription.restitution` | [F08](#f08) |
| [SoftBodyNodeDescription](../include/physics.h) | `SoftBodyNodeDescription.collision_filter`, `SoftBodyNodeDescription.gravity_enabled`, `SoftBodyNodeDescription.collision_enabled` | [F09](#f09) |
| [SoftBodyNodeDescription](../include/physics.h) | `SoftBodyNodeDescription.radius`, `SoftBodyNodeDescription.draw_color`, `SoftBodyNodeDescription.draw_color_overridden` | [F14](#f14) |
| [SoftBodyNodeDescription](../include/physics.h) | `SoftBodyNodeDescription.static_body` | [F15](#f15) |
| [SoundDescription](../include/audio.h) | `SoundDescription.path`, `SoundDescription.volume`, `SoundDescription.pan`, `SoundDescription.loop`, `SoundDescription.playback_rate` | [F46](#f46) |
| [SpriteDescription](../include/graphics.h) | `SpriteDescription.texture`, `SpriteDescription.direction`, `SpriteDescription.scale`, `SpriteDescription.body_offset`, `SpriteDescription.orientation_offset`, `SpriteDescription.follow_entity_rotation`, `SpriteDescription.visible` | [F31](#f31) |
| [TextDescription](../include/graphics.h) | `TextDescription.font`, `TextDescription.value`, `TextDescription.color`, `TextDescription.wrap_width` | [F45](#f45) |
| [TextureDescription](../include/graphics.h) | `TextureDescription.file`, `TextureDescription.size` | [F30](#f30) |
| [TorqueDescription](../include/physics.h) | `TorqueDescription.owner` | [F01](#f01) |
| [TorqueDescription](../include/physics.h) | `TorqueDescription.target`, `TorqueDescription.magnitude` | [F26](#f26) |
| [TransformLockDescription](../include/physics.h) | `TransformLockDescription.driver`, `TransformLockDescription.local_offset`, `TransformLockDescription.local_angle`, `TransformLockDescription.lock_position`, `TransformLockDescription.lock_orientation`, `TransformLockDescription.inherit_velocity` | [F11](#f11) |
| [UIButtonDefinition](../include/ui.h) | `UIButtonDefinition.name`, `UIButtonDefinition.label`, `UIButtonDefinition.font`, `UIButtonDefinition.text_color`, `UIButtonDefinition.bounds`, `UIButtonDefinition.style` | [F44](#f44) |
| [UIButtonStyle](../include/ui.h) | `UIButtonStyle.idle`, `UIButtonStyle.hovered`, `UIButtonStyle.pressed`, `UIButtonStyle.disabled` | [F44](#f44) |
| [UIComponentConfig](../include/ui.h) | `UIComponentConfig.components`, `UIComponentConfig.text_padding_x`, `UIComponentConfig.text_padding_y` | [F59](#f59) |
| [UIDropdownConfig](../include/ui.h) | `UIDropdownConfig.visible_row_limit`, `UIDropdownConfig.fit_window`, `UIDropdownConfig.hint`, `UIDropdownConfig.pick_bounds`, `UIDropdownConfig.pick_exclusion`, `UIDropdownConfig.pick`, `UIDropdownConfig.pick_context` | [F59](#f59) |
| [UIFieldBinding](../include/ui.h) | `UIFieldBinding.kind`, `UIFieldBinding.string`, `UIFieldBinding.string_capacity`, `UIFieldBinding.number` | [F59](#f59) |
| [UIFontDefinition](../include/ui.h) | `UIFontDefinition.name`, `UIFontDefinition.file`, `UIFontDefinition.point_size` | [F44](#f44) |
| [UILabelDefinition](../include/ui.h) | `UILabelDefinition.name`, `UILabelDefinition.text`, `UILabelDefinition.font`, `UILabelDefinition.color`, `UILabelDefinition.bounds` | [F44](#f44) |
| [UIRect](../include/ui.h) | `UIRect.x`, `UIRect.y`, `UIRect.width`, `UIRect.height` | [X01](#x01) |
| [UISliderConfig](../include/ui.h) | `UISliderConfig.center`, `UISliderConfig.length`, `UISliderConfig.angle`, `UISliderConfig.min_value`, `UISliderConfig.max_value`, `UISliderConfig.step`, `UISliderConfig.style` | [F44](#f44) |
| [UISliderDefinition](../include/ui.h) | `UISliderDefinition.name`, `UISliderDefinition.label`, `UISliderDefinition.font`, `UISliderDefinition.value_format`, `UISliderDefinition.text_color`, `UISliderDefinition.config`, `UISliderDefinition.initial_value` | [F44](#f44) |
| [UISliderStyle](../include/ui.h) | `UISliderStyle.track`, `UISliderStyle.fill`, `UISliderStyle.handle`, `UISliderStyle.handle_hovered`, `UISliderStyle.handle_pressed`, `UISliderStyle.track_thickness`, `UISliderStyle.handle_width`, `UISliderStyle.handle_height`, `UISliderStyle.step_button_size`, `UISliderStyle.step_button_gap` | [F44](#f44) |
| [ViewportDescription](../include/graphics.h) | `ViewportDescription.rectangle`, `ViewportDescription.fit`, `ViewportDescription.background_color`, `ViewportDescription.enabled` | [F40](#f40) |
| [ViewportItemDescription](../include/graphics.h) | `ViewportItemDescription.screen`, `ViewportItemDescription.ui`, `ViewportItemDescription.rectangle`, `ViewportItemDescription.fit`, `ViewportItemDescription.orientation`, `ViewportItemDescription.content_offset`, `ViewportItemDescription.content_scale`, `ViewportItemDescription.content_orientation`, `ViewportItemDescription.layer`, `ViewportItemDescription.layer_id`, `ViewportItemDescription.visible`, `ViewportItemDescription.drag_mode`, `ViewportItemDescription.clip_enabled`, `ViewportItemDescription.clip_rectangle` | [F40](#f40) |
| [ViewportRectangle](../include/graphics.h) | `ViewportRectangle.x`, `ViewportRectangle.y`, `ViewportRectangle.width`, `ViewportRectangle.height` | [F40](#f40) |
| [ViewportUiShapeDescription](../include/graphics.h) | `ViewportUiShapeDescription.shape`, `ViewportUiShapeDescription.position`, `ViewportUiShapeDescription.orientation`, `ViewportUiShapeDescription.border_enabled`, `ViewportUiShapeDescription.border_type`, `ViewportUiShapeDescription.border_thickness`, `ViewportUiShapeDescription.border_hash_spacing`, `ViewportUiShapeDescription.border_corner_radius`, `ViewportUiShapeDescription.border_color`, `ViewportUiShapeDescription.fill_color`, `ViewportUiShapeDescription.button_enabled`, `ViewportUiShapeDescription.hover_border_color`, `ViewportUiShapeDescription.hover_fill_color`, `ViewportUiShapeDescription.click_border_color`, `ViewportUiShapeDescription.click_fill_color` | [F41](#f41) |
| [ViewportUiShapeDescription](../include/graphics.h) | `ViewportUiShapeDescription.text` | [F42](#f42) |
| [ViewportUiSliderDescription](../include/graphics.h) | `ViewportUiSliderDescription.minimum`, `ViewportUiSliderDescription.maximum`, `ViewportUiSliderDescription.value`, `ViewportUiSliderDescription.step`, `ViewportUiSliderDescription.length`, `ViewportUiSliderDescription.track_thickness`, `ViewportUiSliderDescription.thumb_width`, `ViewportUiSliderDescription.thumb_height`, `ViewportUiSliderDescription.thumb_radius`, `ViewportUiSliderDescription.thumb_offset`, `ViewportUiSliderDescription.thumb_shape`, `ViewportUiSliderDescription.track_color`, `ViewportUiSliderDescription.filled_track_color`, `ViewportUiSliderDescription.thumb_color`, `ViewportUiSliderDescription.hover_thumb_color`, `ViewportUiSliderDescription.pressed_thumb_color`, `ViewportUiSliderDescription.enabled` | [F43](#f43) |
| [ViewportUiTextDescription](../include/graphics.h) | `ViewportUiTextDescription.text`, `ViewportUiTextDescription.position`, `ViewportUiTextDescription.offset`, `ViewportUiTextDescription.scale`, `ViewportUiTextDescription.orientation` | [F42](#f42) |
| [WindowPresentationDescription](../include/graphics.h) | `WindowPresentationDescription.mode`, `WindowPresentationDescription.window_width`, `WindowPresentationDescription.window_height`, `WindowPresentationDescription.logical_width`, `WindowPresentationDescription.logical_height`, `WindowPresentationDescription.aspect_ratio_auto` | [F50](#f50) |

</details>

<details>
<summary>Editor project, workspace and tool configuration</summary>

| Type / source | Exact fields | Primary feature group |
|---|---|---|
| [EditorAnchor](../tools/editor/src/editor_project.h) | `EditorAnchor.position`, `EditorAnchor.rotation`, `EditorAnchor.rigid_body`, `EditorAnchor.attachment_kind`, `EditorAnchor.attachment_soft_body`, `EditorAnchor.attachment_soft_node`, `EditorAnchor.position_follows_body`, `EditorAnchor.rotation_follows_body` | [F20](#f20) |
| [EditorAnchor](../tools/editor/src/editor_project.h) | `EditorAnchor.visible` | [F29](#f29) |
| [EditorAnchor](../tools/editor/src/editor_project.h) | `EditorAnchor.id`, `EditorAnchor.name` | [X07](#x07) |
| [EditorAnimatedSprite](../tools/editor/src/editor_project.h) | `EditorAnimatedSprite.app_hidden` | [F28](#f28) |
| [EditorAnimatedSprite](../tools/editor/src/editor_project.h) | `EditorAnimatedSprite.visible` | [F29](#f29) |
| [EditorAnimatedSprite](../tools/editor/src/editor_project.h) | `EditorAnimatedSprite.editor_position`, `EditorAnimatedSprite.editor_rotation`, `EditorAnimatedSprite.rigid_body`, `EditorAnimatedSprite.frames`, `EditorAnimatedSprite.frame_count`, `EditorAnimatedSprite.ticks_per_frame`, `EditorAnimatedSprite.time_per_frame`, `EditorAnimatedSprite.starting_frame`, `EditorAnimatedSprite.scale`, `EditorAnimatedSprite.direction`, `EditorAnimatedSprite.follow_body_rotation`, `EditorAnimatedSprite.playing` | [F33](#f33) |
| [EditorAnimatedSprite](../tools/editor/src/editor_project.h) | `EditorAnimatedSprite.graphics_layer` | [F38](#f38) |
| [EditorAnimatedSprite](../tools/editor/src/editor_project.h) | `EditorAnimatedSprite.frame_capacity` | [X03](#x03) |
| [EditorAnimatedSprite](../tools/editor/src/editor_project.h) | `EditorAnimatedSprite.id`, `EditorAnimatedSprite.name` | [X07](#x07) |
| [EditorAnimationFrame](../tools/editor/src/editor_project.h) | `EditorAnimationFrame.path`, `EditorAnimationFrame.scale`, `EditorAnimationFrame.offset`, `EditorAnimationFrame.rotation` | [F32](#f32) |
| [EditorAnimationFrame](../tools/editor/src/editor_project.h) | `EditorAnimationFrame.id`, `EditorAnimationFrame.name` | [X07](#x07) |
| [EditorCamera](../tools/editor/src/editor_project.h) | `EditorCamera.app_hidden` | [F28](#f28) |
| [EditorCamera](../tools/editor/src/editor_project.h) | `EditorCamera.visible` | [F29](#f29) |
| [EditorCamera](../tools/editor/src/editor_project.h) | `EditorCamera.position`, `EditorCamera.rotation`, `EditorCamera.dimensions`, `EditorCamera.zoom`, `EditorCamera.attachment_kind`, `EditorCamera.attachment`, `EditorCamera.attachment_soft_body`, `EditorCamera.inherit_orientation` | [F35](#f35) |
| [EditorCamera](../tools/editor/src/editor_project.h) | `EditorCamera.id`, `EditorCamera.name` | [X07](#x07) |
| [EditorCollisionMask](../tools/editor/src/editor_project.h) | `EditorCollisionMask.name` | [X07](#x07) |
| [EditorConfig](../tools/editor_core/include/editor_config.h) | `EditorConfig.font`, `EditorConfig.font_set`, `EditorConfig.config_path_override`, `EditorConfig.config_path_override_set`, `EditorConfig.gui_state_path`, `EditorConfig.gui_state_path_set`, `EditorConfig.gui_state_path_override`, `EditorConfig.gui_state_path_override_set`, `EditorConfig.project_configure`, `EditorConfig.project_compile`, `EditorConfig.cli_configure`, `EditorConfig.cli_compile`, `EditorConfig.gui_configure`, `EditorConfig.gui_compile` | [F56](#f56) |
| [EditorConfigCommand](../tools/editor_core/include/editor_config.h) | `EditorConfigCommand.arguments`, `EditorConfigCommand.count`, `EditorConfigCommand.set` | [F56](#f56) |
| [EditorForce](../tools/editor/src/editor_project.h) | `EditorForce.description` | [F24](#f24) |
| [EditorForce](../tools/editor/src/editor_project.h) | `EditorForce.visuals` | [F27](#f27) |
| [EditorForce](../tools/editor/src/editor_project.h) | `EditorForce.app_hidden` | [F28](#f28) |
| [EditorForce](../tools/editor/src/editor_project.h) | `EditorForce.visible` | [F29](#f29) |
| [EditorForce](../tools/editor/src/editor_project.h) | `EditorForce.id`, `EditorForce.name` | [X07](#x07) |
| [EditorGraphicsLayer](../tools/editor/src/editor_project.h) | `EditorGraphicsLayer.value` | [F38](#f38) |
| [EditorGraphicsLayer](../tools/editor/src/editor_project.h) | `EditorGraphicsLayer.id`, `EditorGraphicsLayer.name` | [X07](#x07) |
| [EditorGraphicsLayerBinding](../tools/editor/src/editor_project.h) | `EditorGraphicsLayerBinding.value`, `EditorGraphicsLayerBinding.layer` | [F38](#f38) |
| [EditorGuiState](../tools/editor_core/include/editor_config.h) | `EditorGuiState.logical_width`, `EditorGuiState.logical_height`, `EditorGuiState.aspect_ratio`, `EditorGuiState.window_mode`, `EditorGuiState.grid_visible` | [X07](#x07) |
| [EditorHierarchyItem](../tools/editor/src/editor_project.h) | `EditorHierarchyItem.kind`, `EditorHierarchyItem.id` | [X07](#x07) |
| [EditorHitbox](../tools/editor/src/editor_project.h) | `EditorHitbox.vertices`, `EditorHitbox.line_names`, `EditorHitbox.vertex_count` | [F05](#f05) |
| [EditorHitbox](../tools/editor/src/editor_project.h) | `EditorHitbox.visible` | [F29](#f29) |
| [EditorHitbox](../tools/editor/src/editor_project.h) | `EditorHitbox.vertex_capacity` | [X03](#x03) |
| [EditorHitbox](../tools/editor/src/editor_project.h) | `EditorHitbox.id`, `EditorHitbox.name` | [X07](#x07) |
| [EditorHitboxAnimationBinding](../tools/editor/src/editor_project.h) | `EditorHitboxAnimationBinding.animation`, `EditorHitboxAnimationBinding.frame`, `EditorHitboxAnimationBinding.hitbox` | [F33](#f33) |
| [EditorInputAction](../tools/editor/src/editor_project.h) | `EditorInputAction.type`, `EditorInputAction.button_mode`, `EditorInputAction.button_initial_state`, `EditorInputAction.binding_ids`, `EditorInputAction.bindings`, `EditorInputAction.binding_count` | [F48](#f48) |
| [EditorInputAction](../tools/editor/src/editor_project.h) | `EditorInputAction.id`, `EditorInputAction.name` | [X07](#x07) |
| [EditorInputController](../tools/editor/src/editor_project.h) | `EditorInputController.enabled`, `EditorInputController.actions`, `EditorInputController.action_count` | [F48](#f48) |
| [EditorInputController](../tools/editor/src/editor_project.h) | `EditorInputController.action_capacity` | [X03](#x03) |
| [EditorInputController](../tools/editor/src/editor_project.h) | `EditorInputController.id`, `EditorInputController.name` | [X07](#x07) |
| [EditorJoint](../tools/editor/src/editor_project.h) | `EditorJoint.kind`, `EditorJoint.anchor_a`, `EditorJoint.anchor_b`, `EditorJoint.rest_length`, `EditorJoint.stiffness`, `EditorJoint.damping`, `EditorJoint.rest_angle` | [F21](#f21) |
| [EditorJoint](../tools/editor/src/editor_project.h) | `EditorJoint.visual_size`, `EditorJoint.visuals` | [F27](#f27) |
| [EditorJoint](../tools/editor/src/editor_project.h) | `EditorJoint.app_hidden` | [F28](#f28) |
| [EditorJoint](../tools/editor/src/editor_project.h) | `EditorJoint.visible` | [F29](#f29) |
| [EditorJoint](../tools/editor/src/editor_project.h) | `EditorJoint.graphics_layer` | [F38](#f38) |
| [EditorJoint](../tools/editor/src/editor_project.h) | `EditorJoint.id`, `EditorJoint.name` | [X07](#x07) |
| [EditorLayoutViewport](../tools/editor/src/editor_project.h) | `EditorLayoutViewport.app_hidden` | [F28](#f28) |
| [EditorLayoutViewport](../tools/editor/src/editor_project.h) | `EditorLayoutViewport.config`, `EditorLayoutViewport.background_color`, `EditorLayoutViewport.enabled`, `EditorLayoutViewport.camera_items`, `EditorLayoutViewport.camera_item_count`, `EditorLayoutViewport.ui_items`, `EditorLayoutViewport.ui_item_count` | [F40](#f40) |
| [EditorLayoutViewport](../tools/editor/src/editor_project.h) | `EditorLayoutViewport.camera_item_capacity`, `EditorLayoutViewport.ui_item_capacity` | [X03](#x03) |
| [EditorLayoutViewport](../tools/editor/src/editor_project.h) | `EditorLayoutViewport.id`, `EditorLayoutViewport.name`, `EditorLayoutViewport.overview_position` | [X07](#x07) |
| [EditorNavigationState](../tools/editor/src/editor_project.h) | `EditorNavigationState.mode`, `EditorNavigationState.selection`, `EditorNavigationState.object`, `EditorNavigationState.selected_line`, `EditorNavigationState.selected_vertex`, `EditorNavigationState.rigid_body`, `EditorNavigationState.hitbox`, `EditorNavigationState.joint`, `EditorNavigationState.force`, `EditorNavigationState.torque`, `EditorNavigationState.anchor`, `EditorNavigationState.soft_body`, `EditorNavigationState.soft_node`, `EditorNavigationState.soft_beam`, `EditorNavigationState.soft_area`, `EditorNavigationState.soft_hole`, `EditorNavigationState.sprite`, `EditorNavigationState.animated_sprite`, `EditorNavigationState.camera`, `EditorNavigationState.animation_frame`, `EditorNavigationState.input_controller`, `EditorNavigationState.input_action`, `EditorNavigationState.input_binding`, `EditorNavigationState.origin_kind` | [X07](#x07) |
| [EditorObject](../tools/editor/src/editor_project.h) | `EditorObject.app_hidden` | [F28](#f28) |
| [EditorObject](../tools/editor/src/editor_project.h) | `EditorObject.visible` | [F29](#f29) |
| [EditorObject](../tools/editor/src/editor_project.h) | `EditorObject.position`, `EditorObject.rigid_bodies`, `EditorObject.rigid_body_count`, `EditorObject.force_items`, `EditorObject.force_count`, `EditorObject.torque_items`, `EditorObject.torque_count`, `EditorObject.joint_items`, `EditorObject.joint_count`, `EditorObject.anchors`, `EditorObject.anchor_count`, `EditorObject.soft_body_items`, `EditorObject.soft_body_count`, `EditorObject.sprites`, `EditorObject.sprite_count`, `EditorObject.animated_sprite_items`, `EditorObject.animated_sprite_count`, `EditorObject.cameras`, `EditorObject.camera_count`, `EditorObject.hierarchy`, `EditorObject.hierarchy_count` | [F51](#f51) |
| [EditorObject](../tools/editor/src/editor_project.h) | `EditorObject.simulation_excluded` | [F55](#f55) |
| [EditorObject](../tools/editor/src/editor_project.h) | `EditorObject.rigid_body_capacity`, `EditorObject.force_capacity`, `EditorObject.torque_capacity`, `EditorObject.joint_capacity`, `EditorObject.anchor_capacity`, `EditorObject.soft_body_capacity`, `EditorObject.sprite_capacity`, `EditorObject.animated_sprite_capacity`, `EditorObject.camera_capacity`, `EditorObject.hierarchy_capacity` | [X03](#x03) |
| [EditorObject](../tools/editor/src/editor_project.h) | `EditorObject.id`, `EditorObject.name`, `EditorObject.overview_position` | [X07](#x07) |
| [EditorProject](../tools/editor/src/editor_project.h) | `EditorProject.engine_time_per_tick`, `EditorProject.physics_timestep_override`, `EditorProject.physics_dt_per_tick`, `EditorProject.physics_substeps`, `EditorProject.physics_gravity`, `EditorProject.physics_solver_iterations` | [F04](#f04) |
| [EditorProject](../tools/editor/src/editor_project.h) | `EditorProject.viewport_camera_offset`, `EditorProject.viewport_camera_zoom`, `EditorProject.viewport_local_view`, `EditorProject.navigation`, `EditorProject.selected` | [F29](#f29) |
| [EditorProject](../tools/editor/src/editor_project.h) | `EditorProject.graphics_layers`, `EditorProject.graphics_layer_count` | [F38](#f38) |
| [EditorProject](../tools/editor/src/editor_project.h) | `EditorProject.layout_viewports`, `EditorProject.layout_viewport_count` | [F40](#f40) |
| [EditorProject](../tools/editor/src/editor_project.h) | `EditorProject.ui_definitions`, `EditorProject.ui_definition_count` | [F41](#f41) |
| [EditorProject](../tools/editor/src/editor_project.h) | `EditorProject.ui_fonts`, `EditorProject.ui_font_count` | [F45](#f45) |
| [EditorProject](../tools/editor/src/editor_project.h) | `EditorProject.input_controllers`, `EditorProject.input_controller_count` | [F48](#f48) |
| [EditorProject](../tools/editor/src/editor_project.h) | `EditorProject.collision_masks`, `EditorProject.collision_mask_count`, `EditorProject.objects`, `EditorProject.object_count`, `EditorProject.hierarchy`, `EditorProject.hierarchy_count` | [F51](#f51) |
| [EditorProject](../tools/editor/src/editor_project.h) | `EditorProject.simulation_viewport` | [F55](#f55) |
| [EditorProject](../tools/editor/src/editor_project.h) | `EditorProject.collision_mask_capacity`, `EditorProject.object_capacity`, `EditorProject.layout_viewport_capacity`, `EditorProject.hierarchy_capacity`, `EditorProject.ui_font_capacity`, `EditorProject.graphics_layer_capacity`, `EditorProject.ui_definition_capacity`, `EditorProject.input_controller_capacity` | [X03](#x03) |
| [EditorProject](../tools/editor/src/editor_project.h) | `EditorProject.next_id`, `EditorProject.next_vertex_id`, `EditorProject.next_rigid_body_id`, `EditorProject.next_hitbox_id`, `EditorProject.next_force_id`, `EditorProject.next_torque_id`, `EditorProject.next_joint_id`, `EditorProject.next_anchor_id`, `EditorProject.next_soft_body_id`, `EditorProject.next_soft_node_id`, `EditorProject.next_soft_beam_id`, `EditorProject.next_sprite_id`, `EditorProject.next_animated_sprite_id`, `EditorProject.next_camera_id`, `EditorProject.next_layout_viewport_id`, `EditorProject.next_viewport_camera_item_id`, `EditorProject.next_viewport_ui_item_id`, `EditorProject.next_ui_font_id`, `EditorProject.next_graphics_layer_id`, `EditorProject.next_ui_definition_id`, `EditorProject.next_input_controller_id`, `EditorProject.next_input_action_id`, `EditorProject.next_input_binding_id` | [X07](#x07) |
| [EditorProjectHierarchyItem](../tools/editor/src/editor_project.h) | `EditorProjectHierarchyItem.kind`, `EditorProjectHierarchyItem.id` | [X07](#x07) |
| [EditorRigidBody](../tools/editor/src/editor_project.h) | `EditorRigidBody.hitboxes`, `EditorRigidBody.hitbox_count`, `EditorRigidBody.active_hitbox_index` | [F05](#f05) |
| [EditorRigidBody](../tools/editor/src/editor_project.h) | `EditorRigidBody.parent`, `EditorRigidBody.position`, `EditorRigidBody.rotation`, `EditorRigidBody.initial_velocity`, `EditorRigidBody.initial_acceleration`, `EditorRigidBody.initial_angular_velocity` | [F06](#f06) |
| [EditorRigidBody](../tools/editor/src/editor_project.h) | `EditorRigidBody.mass_value`, `EditorRigidBody.center_of_mass_explicit`, `EditorRigidBody.center_of_mass_offset` | [F07](#f07) |
| [EditorRigidBody](../tools/editor/src/editor_project.h) | `EditorRigidBody.friction`, `EditorRigidBody.restitution` | [F08](#f08) |
| [EditorRigidBody](../tools/editor/src/editor_project.h) | `EditorRigidBody.static_body`, `EditorRigidBody.gravity_enabled`, `EditorRigidBody.collision_enabled`, `EditorRigidBody.collision_category`, `EditorRigidBody.collision_response` | [F09](#f09) |
| [EditorRigidBody](../tools/editor/src/editor_project.h) | `EditorRigidBody.rotation_locked` | [F10](#f10) |
| [EditorRigidBody](../tools/editor/src/editor_project.h) | `EditorRigidBody.particle`, `EditorRigidBody.standalone_particle`, `EditorRigidBody.particle_auto_fit`, `EditorRigidBody.particle_radius`, `EditorRigidBody.particle_rigid_vertices`, `EditorRigidBody.particle_origin`, `EditorRigidBody.particle_ring_color`, `EditorRigidBody.particle_fill_color` | [F12](#f12) |
| [EditorRigidBody](../tools/editor/src/editor_project.h) | `EditorRigidBody.border_color`, `EditorRigidBody.surface_color`, `EditorRigidBody.app_hidden` | [F28](#f28) |
| [EditorRigidBody](../tools/editor/src/editor_project.h) | `EditorRigidBody.visible` | [F29](#f29) |
| [EditorRigidBody](../tools/editor/src/editor_project.h) | `EditorRigidBody.hitbox_animation_bindings`, `EditorRigidBody.hitbox_animation_binding_count` | [F33](#f33) |
| [EditorRigidBody](../tools/editor/src/editor_project.h) | `EditorRigidBody.graphics_layer` | [F38](#f38) |
| [EditorRigidBody](../tools/editor/src/editor_project.h) | `EditorRigidBody.hitbox_capacity`, `EditorRigidBody.hitbox_animation_binding_capacity` | [X03](#x03) |
| [EditorRigidBody](../tools/editor/src/editor_project.h) | `EditorRigidBody.id`, `EditorRigidBody.name` | [X07](#x07) |
| [EditorSceneSelection](../tools/editor/src/editor_workspace.h) | `EditorSceneSelection.objects`, `EditorSceneSelection.object_count`, `EditorSceneSelection.objects_selected`, `EditorSceneSelection.viewport`, `EditorSceneSelection.viewport_selected` | [F55](#f55) |
| [EditorSoftArea](../tools/editor/src/editor_project.h) | `EditorSoftArea.color`, `EditorSoftArea.outer`, `EditorSoftArea.holes`, `EditorSoftArea.hole_count` | [F19](#f19) |
| [EditorSoftArea](../tools/editor/src/editor_project.h) | `EditorSoftArea.app_hidden` | [F28](#f28) |
| [EditorSoftArea](../tools/editor/src/editor_project.h) | `EditorSoftArea.visible` | [F29](#f29) |
| [EditorSoftArea](../tools/editor/src/editor_project.h) | `EditorSoftArea.graphics_layer`, `EditorSoftArea.graphics_layer_inherited` | [F38](#f38) |
| [EditorSoftArea](../tools/editor/src/editor_project.h) | `EditorSoftArea.cache` | [X03](#x03) |
| [EditorSoftArea](../tools/editor/src/editor_project.h) | `EditorSoftArea.id`, `EditorSoftArea.name`, `EditorSoftArea.next_hole_id` | [X07](#x07) |
| [EditorSoftAreaLoop](../tools/editor/src/editor_project.h) | `EditorSoftAreaLoop.nodes`, `EditorSoftAreaLoop.node_count` | [F19](#f19) |
| [EditorSoftBeam](../tools/editor/src/editor_project.h) | `EditorSoftBeam.node_a`, `EditorSoftBeam.node_b`, `EditorSoftBeam.friction`, `EditorSoftBeam.restitution`, `EditorSoftBeam.stiffness`, `EditorSoftBeam.damping`, `EditorSoftBeam.color`, `EditorSoftBeam.color_overridden` | [F16](#f16) |
| [EditorSoftBeam](../tools/editor/src/editor_project.h) | `EditorSoftBeam.collision_enabled`, `EditorSoftBeam.collision_thickness`, `EditorSoftBeam.collision_category`, `EditorSoftBeam.collision_response` | [F18](#f18) |
| [EditorSoftBeam](../tools/editor/src/editor_project.h) | `EditorSoftBeam.app_hidden` | [F28](#f28) |
| [EditorSoftBeam](../tools/editor/src/editor_project.h) | `EditorSoftBeam.visible` | [F29](#f29) |
| [EditorSoftBeam](../tools/editor/src/editor_project.h) | `EditorSoftBeam.graphics_layer`, `EditorSoftBeam.graphics_layer_inherited` | [F38](#f38) |
| [EditorSoftBeam](../tools/editor/src/editor_project.h) | `EditorSoftBeam.id`, `EditorSoftBeam.name` | [X07](#x07) |
| [EditorSoftBody](../tools/editor/src/editor_project.h) | `EditorSoftBody.position`, `EditorSoftBody.rotation`, `EditorSoftBody.initial_velocity`, `EditorSoftBody.initial_acceleration`, `EditorSoftBody.initial_angular_velocity`, `EditorSoftBody.node_color`, `EditorSoftBody.beam_color`, `EditorSoftBody.nodes`, `EditorSoftBody.node_count`, `EditorSoftBody.beams`, `EditorSoftBody.beam_count`, `EditorSoftBody.areas`, `EditorSoftBody.area_count`, `EditorSoftBody.hierarchy`, `EditorSoftBody.hierarchy_count` | [F13](#f13) |
| [EditorSoftBody](../tools/editor/src/editor_project.h) | `EditorSoftBody.app_hidden` | [F28](#f28) |
| [EditorSoftBody](../tools/editor/src/editor_project.h) | `EditorSoftBody.visible` | [F29](#f29) |
| [EditorSoftBody](../tools/editor/src/editor_project.h) | `EditorSoftBody.graphics_layer` | [F38](#f38) |
| [EditorSoftBody](../tools/editor/src/editor_project.h) | `EditorSoftBody.node_capacity`, `EditorSoftBody.beam_capacity`, `EditorSoftBody.area_capacity`, `EditorSoftBody.hierarchy_capacity` | [X03](#x03) |
| [EditorSoftBody](../tools/editor/src/editor_project.h) | `EditorSoftBody.id`, `EditorSoftBody.name`, `EditorSoftBody.next_area_id` | [X07](#x07) |
| [EditorSoftHierarchyItem](../tools/editor/src/editor_project.h) | `EditorSoftHierarchyItem.kind`, `EditorSoftHierarchyItem.id` | [X07](#x07) |
| [EditorSoftHole](../tools/editor/src/editor_project.h) | `EditorSoftHole.loop` | [F19](#f19) |
| [EditorSoftHole](../tools/editor/src/editor_project.h) | `EditorSoftHole.id`, `EditorSoftHole.name` | [X07](#x07) |
| [EditorSoftNode](../tools/editor/src/editor_project.h) | `EditorSoftNode.position`, `EditorSoftNode.initial_velocity`, `EditorSoftNode.initial_acceleration`, `EditorSoftNode.initial_motion_inherited`, `EditorSoftNode.node_mass`, `EditorSoftNode.radius`, `EditorSoftNode.friction`, `EditorSoftNode.restitution`, `EditorSoftNode.gravity_enabled`, `EditorSoftNode.collision_enabled`, `EditorSoftNode.collision_category`, `EditorSoftNode.collision_response`, `EditorSoftNode.color`, `EditorSoftNode.color_overridden` | [F14](#f14) |
| [EditorSoftNode](../tools/editor/src/editor_project.h) | `EditorSoftNode.app_hidden` | [F28](#f28) |
| [EditorSoftNode](../tools/editor/src/editor_project.h) | `EditorSoftNode.visible` | [F29](#f29) |
| [EditorSoftNode](../tools/editor/src/editor_project.h) | `EditorSoftNode.graphics_layer`, `EditorSoftNode.graphics_layer_inherited` | [F38](#f38) |
| [EditorSoftNode](../tools/editor/src/editor_project.h) | `EditorSoftNode.id`, `EditorSoftNode.name` | [X07](#x07) |
| [EditorSprite](../tools/editor/src/editor_project.h) | `EditorSprite.app_hidden` | [F28](#f28) |
| [EditorSprite](../tools/editor/src/editor_project.h) | `EditorSprite.visible` | [F29](#f29) |
| [EditorSprite](../tools/editor/src/editor_project.h) | `EditorSprite.path`, `EditorSprite.position`, `EditorSprite.rotation`, `EditorSprite.rigid_body`, `EditorSprite.size`, `EditorSprite.follow_body_rotation` | [F31](#f31) |
| [EditorSprite](../tools/editor/src/editor_project.h) | `EditorSprite.graphics_layer` | [F38](#f38) |
| [EditorSprite](../tools/editor/src/editor_project.h) | `EditorSprite.id`, `EditorSprite.name` | [X07](#x07) |
| [EditorTorque](../tools/editor/src/editor_project.h) | `EditorTorque.description` | [F26](#f26) |
| [EditorTorque](../tools/editor/src/editor_project.h) | `EditorTorque.marker_offset`, `EditorTorque.visuals` | [F27](#f27) |
| [EditorTorque](../tools/editor/src/editor_project.h) | `EditorTorque.app_hidden` | [F28](#f28) |
| [EditorTorque](../tools/editor/src/editor_project.h) | `EditorTorque.visible` | [F29](#f29) |
| [EditorTorque](../tools/editor/src/editor_project.h) | `EditorTorque.id`, `EditorTorque.name` | [X07](#x07) |
| [EditorUiFont](../tools/editor/src/editor_project.h) | `EditorUiFont.path` | [F45](#f45) |
| [EditorUiFont](../tools/editor/src/editor_project.h) | `EditorUiFont.id`, `EditorUiFont.name` | [X07](#x07) |
| [EditorVertex](../tools/editor/src/editor_project.h) | `EditorVertex.position` | [F05](#f05) |
| [EditorVertex](../tools/editor/src/editor_project.h) | `EditorVertex.position_locked` | [F29](#f29) |
| [EditorVertex](../tools/editor/src/editor_project.h) | `EditorVertex.id`, `EditorVertex.name` | [X07](#x07) |
| [EditorViewportCameraItem](../tools/editor/src/editor_project.h) | `EditorViewportCameraItem.app_hidden` | [F28](#f28) |
| [EditorViewportCameraItem](../tools/editor/src/editor_project.h) | `EditorViewportCameraItem.graphics_layer` | [F38](#f38) |
| [EditorViewportCameraItem](../tools/editor/src/editor_project.h) | `EditorViewportCameraItem.object`, `EditorViewportCameraItem.camera`, `EditorViewportCameraItem.placement`, `EditorViewportCameraItem.content_offset`, `EditorViewportCameraItem.content_scale`, `EditorViewportCameraItem.content_rotation` | [F40](#f40) |
| [EditorViewportCameraItem](../tools/editor/src/editor_project.h) | `EditorViewportCameraItem.id`, `EditorViewportCameraItem.name` | [X07](#x07) |
| [EditorViewportUiDefinition](../tools/editor/src/editor_project.h) | `EditorViewportUiDefinition.kind`, `EditorViewportUiDefinition.border_enabled`, `EditorViewportUiDefinition.border_type`, `EditorViewportUiDefinition.border_thickness`, `EditorViewportUiDefinition.border_hash_spacing`, `EditorViewportUiDefinition.border_corner_radius`, `EditorViewportUiDefinition.border_color`, `EditorViewportUiDefinition.fill_color`, `EditorViewportUiDefinition.hover_border_color`, `EditorViewportUiDefinition.hover_fill_color`, `EditorViewportUiDefinition.click_border_color`, `EditorViewportUiDefinition.click_fill_color`, `EditorViewportUiDefinition.value.shape`, `EditorViewportUiDefinition.value.text`, `EditorViewportUiDefinition.value.slider` | [F41](#f41) |
| [EditorViewportUiDefinition](../tools/editor/src/editor_project.h) | `EditorViewportUiDefinition.id`, `EditorViewportUiDefinition.name` | [X07](#x07) |
| [EditorViewportUiItem](../tools/editor/src/editor_project.h) | `EditorViewportUiItem.app_hidden` | [F28](#f28) |
| [EditorViewportUiItem](../tools/editor/src/editor_project.h) | `EditorViewportUiItem.visible` | [F29](#f29) |
| [EditorViewportUiItem](../tools/editor/src/editor_project.h) | `EditorViewportUiItem.graphics_layer` | [F38](#f38) |
| [EditorViewportUiItem](../tools/editor/src/editor_project.h) | `EditorViewportUiItem.definition`, `EditorViewportUiItem.position`, `EditorViewportUiItem.scale`, `EditorViewportUiItem.rotation`, `EditorViewportUiItem.clip_rectangle`, `EditorViewportUiItem.layer`, `EditorViewportUiItem.drag_mode`, `EditorViewportUiItem.clip_enabled`, `EditorViewportUiItem.kind`, `EditorViewportUiItem.border_enabled`, `EditorViewportUiItem.border_type`, `EditorViewportUiItem.border_thickness`, `EditorViewportUiItem.border_hash_spacing`, `EditorViewportUiItem.border_corner_radius`, `EditorViewportUiItem.border_color`, `EditorViewportUiItem.fill_color`, `EditorViewportUiItem.hover_border_color`, `EditorViewportUiItem.hover_fill_color`, `EditorViewportUiItem.click_border_color`, `EditorViewportUiItem.click_fill_color` | [F40](#f40) |
| [EditorViewportUiItem](../tools/editor/src/editor_project.h) | `EditorViewportUiItem.value.shape`, `EditorViewportUiItem.value.text`, `EditorViewportUiItem.value.slider` | [X03](#x03) |
| [EditorViewportUiItem](../tools/editor/src/editor_project.h) | `EditorViewportUiItem.id`, `EditorViewportUiItem.name` | [X07](#x07) |
| [EditorViewportUiShape](../tools/editor/src/editor_project.h) | `EditorViewportUiShape.vertices`, `EditorViewportUiShape.vertex_count`, `EditorViewportUiShape.rotation`, `EditorViewportUiShape.outline_color`, `EditorViewportUiShape.fill_color`, `EditorViewportUiShape.button_enabled`, `EditorViewportUiShape.text` | [F41](#f41) |
| [EditorViewportUiSlider](../tools/editor/src/editor_project.h) | `EditorViewportUiSlider.minimum`, `EditorViewportUiSlider.maximum`, `EditorViewportUiSlider.value`, `EditorViewportUiSlider.step`, `EditorViewportUiSlider.length`, `EditorViewportUiSlider.track_thickness`, `EditorViewportUiSlider.thumb_width`, `EditorViewportUiSlider.thumb_height`, `EditorViewportUiSlider.thumb_radius`, `EditorViewportUiSlider.thumb_offset`, `EditorViewportUiSlider.legacy_vertical`, `EditorViewportUiSlider.thumb_shape`, `EditorViewportUiSlider.track_color`, `EditorViewportUiSlider.filled_track_color`, `EditorViewportUiSlider.thumb_color`, `EditorViewportUiSlider.hover_thumb_color`, `EditorViewportUiSlider.pressed_thumb_color`, `EditorViewportUiSlider.enabled` | [F43](#f43) |
| [EditorViewportUiText](../tools/editor/src/editor_project.h) | `EditorViewportUiText.text`, `EditorViewportUiText.offset`, `EditorViewportUiText.font`, `EditorViewportUiText.color`, `EditorViewportUiText.box_width`, `EditorViewportUiText.box_height`, `EditorViewportUiText.width_scale`, `EditorViewportUiText.height_scale` | [F42](#f42) |
| [EditorWorkspace](../tools/editor/src/editor_workspace.h) | `EditorWorkspace.config`, `EditorWorkspace.directory`, `EditorWorkspace.open` | [F56](#f56) |
| [EditorWorkspaceCommand](../tools/editor/src/editor_workspace.h) | `EditorWorkspaceCommand.type`, `EditorWorkspaceCommand.directory` | [F56](#f56) |
| [EditorWorkspaceConfig](../tools/editor/src/editor_workspace.h) | `EditorWorkspaceConfig.format_version`, `EditorWorkspaceConfig.source_directory`, `EditorWorkspaceConfig.generated_directory`, `EditorWorkspaceConfig.asset_directory`, `EditorWorkspaceConfig.object_directory`, `EditorWorkspaceConfig.editor_state_file` | [F56](#f56) |
| [EditorWorkspaceConfig](../tools/editor/src/editor_workspace.h) | `EditorWorkspaceConfig.name` | [X07](#x07) |

</details>

## Exact callable reconciliation

All **1,602** `(header, symbol)` occurrences below map to one primary feature
group or an explicit X exclusion. Shared operations (for example full object
configuration) also use the field ledger above; a primary group does not erase
their property-specific obligations. The existing API checker independently
reconciles these declarations with installed headers. The partition below was
checked against that inventory, including duplicate/missing entries and totals.

| Header | Mapped occurrences |
|---|---|
| [audio.h](../include/audio.h) | 41 |
| [console.h](../include/console.h) | 8 |
| [engine.h](../include/engine.h) | 16 |
| [entity_components.h](../include/entity_components.h) | 42 |
| [entity_info.h](../include/entity_info.h) | 6 |
| [error.h](../include/error.h) | 10 |
| [game_state.h](../include/game_state.h) | 8 |
| [graphics.h](../include/graphics.h) | 263 |
| [input.h](../include/input.h) | 53 |
| [math2d.h](../include/math2d.h) | 29 |
| [memory.h](../include/memory.h) | 4 |
| [object.h](../include/object.h) | 37 |
| [physics.h](../include/physics.h) | 231 |
| [report.h](../include/report.h) | 12 |
| [rohr.h](../include/rohr.h) | 789 |
| [rohr_tools.h](../include/rohr_tools.h) | 4 |
| [scene.h](../include/scene.h) | 2 |
| [systems.h](../include/systems.h) | 1 |
| [tools.h](../include/tools.h) | 6 |
| [ui.h](../include/ui.h) | 40 |

<details>
<summary>audio.h: 41 mapped entries</summary>

[F46](#f46) (24):

- `audio_music_create`, `audio_music_description_default_get`, `audio_music_description_get`, `audio_music_description_set`.
- `audio_music_loop_check`, `audio_music_loop_set`, `audio_music_playback_rate_get`, `audio_music_playback_rate_set`.
- `audio_music_volume_get`, `audio_music_volume_set`, `audio_sound_create`, `audio_sound_description_default_get`.
- `audio_sound_description_get`, `audio_sound_description_set`, `audio_sound_loop_check`, `audio_sound_loop_set`.
- `audio_sound_pan_get`, `audio_sound_pan_set`, `audio_sound_playback_rate_get`, `audio_sound_playback_rate_set`.
- `audio_sound_volume_get`, `audio_sound_volume_set`, `audio_volume_get`, `audio_volume_set`.

[F47](#f47) (17):

- `audio_music_destroy`, `audio_music_pause`, `audio_music_paused_check`, `audio_music_play`.
- `audio_music_playing_check`, `audio_music_resume`, `audio_music_stop`, `audio_music_valid_check`.
- `audio_sound_destroy`, `audio_sound_play`, `audio_sound_playing_check`, `audio_sound_stop`.
- `audio_sound_valid_check`, `audio_start`, `audio_started_check`, `audio_stop`.
- `audio_update`.

</details>

<details>
<summary>console.h: 8 mapped entries</summary>

[X02](#x02) (8):

- `console_active_get`, `console_debug_set`, `console_debug_write`, `console_logs_print`.
- `console_read`, `console_start`, `console_stop`, `console_write`.

</details>

<details>
<summary>engine.h: 16 mapped entries</summary>

[F04](#f04) (2):

- `engine_time_per_tick_get`, `engine_time_per_tick_set`.

[X04](#x04) (14):

- `engine_clock_reset`, `engine_event_poll`, `engine_lifecycle_update`, `engine_pause`.
- `engine_paused_get`, `engine_resume`, `engine_start`, `engine_stop`.
- `engine_tick_delta_get`, `engine_tick_get`, `engine_tick_update`, `engine_time_get`.
- `engine_time_update`, `engine_update`.

</details>

<details>
<summary>entity_components.h: 42 mapped entries</summary>

[F01](#f01) (21):

- `entity_add`, `entity_alive_at_get`, `entity_alive_check`, `entity_alive_count_get`.
- `entity_by_name_get`, `entity_children_get`, `entity_components_add`, `entity_components_check`.
- `entity_components_delete`, `entity_delete`, `entity_description_add`, `entity_description_get`.
- `entity_description_set`, `entity_from_index_get`, `entity_index_alive_check`, `entity_index_components_check`.
- `entity_index_get`, `entity_name_get`, `entity_name_set`, `entity_owner_get`.
- `entity_owner_set`.

[F02](#f02) (15):

- `entity_child_remove`, `entity_child_set`, `entity_group_add`, `entity_group_by_name_get`.
- `entity_group_create`, `entity_group_destroy`, `entity_group_entity_check`, `entity_group_get`.
- `entity_group_name_get`, `entity_group_name_set`, `entity_group_remove`, `entity_groups_get`.
- `entity_parent_get`, `entity_parent_remove`, `entity_parent_set`.

[F03](#f03) (3):

- `entity_life_time_remove`, `entity_lifetime_description_get`, `entity_lifetime_description_set`.

[F51](#f51) (3):

- `entity_object_member_by_name_get`, `entity_object_member_name_get`, `entity_object_members_get`.

</details>

<details>
<summary>entity_info.h: 6 mapped entries</summary>

[X02](#x02) (6):

- `entity_info_check`, `entity_info_get`, `entity_info_issues_count_get`, `entity_info_issues_get`.
- `entity_info_release`, `entity_info_retain`.

</details>

<details>
<summary>error.h: 10 mapped entries</summary>

[X02](#x02) (10):

- `ERROR_DECLARE_RESULT_TYPE`, `ERROR_RESULT_MAKE_ERROR`, `ERROR_RESULT_MAKE_VALUE`, `error_check`.
- `error_code_message_get`, `error_detail_set`, `error_result_error`, `error_result_error_detail`.
- `error_result_message_get`, `error_result_value`.

</details>

<details>
<summary>game_state.h: 8 mapped entries</summary>

[F44](#f44) (4):

- `ui_button_by_name_get`, `ui_font_by_name_get`, `ui_label_by_name_get`, `ui_slider_by_name_get`.

[F52](#f52) (4):

- `game_state_file_load`, `game_state_file_save`, `game_state_files_load`, `game_state_template_file_save`.

</details>

<details>
<summary>graphics.h: 263 mapped entries</summary>

[F14](#f14) (1):

- `graphics_soft_body_node_color_set`.

[F16](#f16) (1):

- `graphics_soft_body_beam_color_set`.

[F19](#f19) (3):

- `graphics_soft_body_area_color_clear`, `graphics_soft_body_area_color_set`, `graphics_soft_body_area_visibility_set`.

[F30](#f30) (7):

- `graphics_texture_description_default_get`, `graphics_texture_description_get`, `graphics_texture_load`, `graphics_texture_release`.
- `graphics_texture_retain`, `graphics_texture_size_get`, `graphics_texture_valid_check`.

[F31](#f31) (13):

- `graphics_sprite_add`, `graphics_sprite_body_offset_get`, `graphics_sprite_body_offset_set`, `graphics_sprite_create`.
- `graphics_sprite_description_default_get`, `graphics_sprite_description_get`, `graphics_sprite_description_set`, `graphics_sprite_draw`.
- `graphics_sprite_frames_update`, `graphics_sprite_orientation_offset_get`, `graphics_sprite_orientation_offset_radians_get`, `graphics_sprite_orientation_offset_radians_set`.
- `graphics_sprite_orientation_offset_set`.

[F32](#f32) (39):

- `graphics_animated_sprite_add`, `graphics_animated_sprite_body_offset_get`, `graphics_animated_sprite_body_offset_set`, `graphics_animated_sprite_create`.
- `graphics_animated_sprite_description_default_get`, `graphics_animated_sprite_description_from_animation_get`, `graphics_animated_sprite_description_get`, `graphics_animated_sprite_description_set`.
- `graphics_animated_sprite_draw`, `graphics_animated_sprite_frame_add`, `graphics_animated_sprite_frame_description_get`, `graphics_animated_sprite_frame_description_set`.
- `graphics_animated_sprite_frame_offset_get`, `graphics_animated_sprite_frame_offset_set`, `graphics_animated_sprite_frame_rotation_get`, `graphics_animated_sprite_frame_rotation_radians_get`.
- `graphics_animated_sprite_frame_rotation_radians_set`, `graphics_animated_sprite_frame_rotation_set`, `graphics_animated_sprite_frame_scale_get`, `graphics_animated_sprite_frame_scale_set`.
- `graphics_animated_sprite_orientation_offset_get`, `graphics_animated_sprite_orientation_offset_radians_get`, `graphics_animated_sprite_orientation_offset_radians_set`, `graphics_animated_sprite_orientation_offset_set`.
- `graphics_animated_sprite_value_description_get`, `graphics_animated_sprite_value_description_set`, `graphics_animated_sprite_value_frame_add`, `graphics_animated_sprite_value_frame_description_set`.
- `graphics_animated_sprites_draw`, `graphics_animation_description_default_get`, `graphics_animation_description_get`, `graphics_animation_frame_create`.
- `graphics_animation_frame_description_default_get`, `graphics_animation_frame_description_get`, `graphics_animation_info_get`, `graphics_animation_load`.
- `graphics_animation_release`, `graphics_animation_retain`, `graphics_animation_valid_check`.

[F33](#f33) (9):

- `graphics_animated_sprite_frame_index_get`, `graphics_animated_sprite_frame_index_set`, `graphics_animated_sprite_update`, `graphics_animation_player_create`.
- `graphics_animation_player_description_default_get`, `graphics_animation_player_description_from_animation_get`, `graphics_animation_player_description_get`, `graphics_animation_player_description_set`.
- `graphics_animation_player_update`.

[F34](#f34) (16):

- `graphics_camera_create`, `graphics_camera_description_default_get`, `graphics_camera_description_get`, `graphics_camera_description_set`.
- `graphics_camera_destroy`, `graphics_camera_disable_set`, `graphics_camera_enable_set`, `graphics_camera_get`.
- `graphics_camera_move`, `graphics_camera_moving_get`, `graphics_camera_position_move`, `graphics_camera_position_set`.
- `graphics_camera_radians_rotate`, `graphics_camera_rotate`, `graphics_camera_zoom_get`, `graphics_camera_zoom_set`.

[F35](#f35) (9):

- `graphics_camera_attach`, `graphics_camera_attached_get`, `graphics_camera_attachment_description_default_get`, `graphics_camera_attachment_description_get`.
- `graphics_camera_attachment_description_set`, `graphics_camera_attachment_remove`, `graphics_camera_detach`, `graphics_camera_entity_attachment_set`.
- `graphics_camera_position_from_entity_set`.

[F36](#f36) (2):

- `graphics_camera_pause_with_engine_set`, `graphics_camera_render_when_paused_set`.

[F37](#f37) (3):

- `graphics_camera_active_get`, `graphics_camera_active_set`, `graphics_camera_render_callback_set`.

[F38](#f38) (38):

- `graphics_layer_active_get`, `graphics_layer_active_id_set`, `graphics_layer_active_name_set`, `graphics_layer_active_set`.
- `graphics_layer_animation_clear`, `graphics_layer_animation_get`, `graphics_layer_animation_id_get`, `graphics_layer_animation_id_set`.
- `graphics_layer_animation_name_set`, `graphics_layer_animation_set`, `graphics_layer_create`, `graphics_layer_description_default_get`.
- `graphics_layer_description_get`, `graphics_layer_description_set`, `graphics_layer_destroy`, `graphics_layer_entity_clear`.
- `graphics_layer_entity_get`, `graphics_layer_entity_id_get`, `graphics_layer_entity_id_set`, `graphics_layer_entity_name_set`.
- `graphics_layer_entity_set`, `graphics_layer_get`, `graphics_layer_name_get`, `graphics_layer_name_id_get`.
- `graphics_layer_name_set`, `graphics_layer_set`, `graphics_layer_sprite_clear`, `graphics_layer_sprite_get`.
- `graphics_layer_sprite_id_get`, `graphics_layer_sprite_id_set`, `graphics_layer_sprite_name_set`, `graphics_layer_sprite_set`.
- `graphics_layer_ui_clear`, `graphics_layer_ui_get`, `graphics_layer_ui_id_get`, `graphics_layer_ui_id_set`.
- `graphics_layer_ui_name_set`, `graphics_layer_ui_set`.

[F39](#f39) (6):

- `graphics_screen_camera_set`, `graphics_screen_create`, `graphics_screen_description_default_get`, `graphics_screen_description_get`.
- `graphics_screen_description_set`, `graphics_screen_destroy`.

[F40](#f40) (27):

- `graphics_viewport_camera_add`, `graphics_viewport_camera_clear`, `graphics_viewport_camera_set`, `graphics_viewport_create`.
- `graphics_viewport_description_default_get`, `graphics_viewport_description_get`, `graphics_viewport_description_set`, `graphics_viewport_destroy`.
- `graphics_viewport_disable_set`, `graphics_viewport_enable_set`, `graphics_viewport_item_add`, `graphics_viewport_item_description_default_get`.
- `graphics_viewport_item_description_get`, `graphics_viewport_item_description_set`, `graphics_viewport_item_drag_mode_get`, `graphics_viewport_item_drag_mode_set`.
- `graphics_viewport_item_dragging_check`, `graphics_viewport_item_remove`, `graphics_viewport_item_viewport_get`, `graphics_viewport_screen_add`.
- `graphics_viewport_ui_add`, `graphics_viewport_ui_hovered_check`, `graphics_viewport_ui_pressed_check`, `graphics_viewport_ui_shape_add`.
- `graphics_viewport_ui_slider_add`, `graphics_viewport_ui_slider_changed_check`, `graphics_viewport_ui_text_add`.

[F41](#f41) (5):

- `graphics_ui_destroy`, `graphics_ui_shape_create`, `graphics_ui_shape_description_default_get`, `graphics_ui_shape_description_get`.
- `graphics_ui_shape_description_set`.

[F42](#f42) (4):

- `graphics_ui_text_create`, `graphics_ui_text_description_default_get`, `graphics_ui_text_description_get`, `graphics_ui_text_description_set`.

[F43](#f43) (6):

- `graphics_ui_slider_create`, `graphics_ui_slider_description_default_get`, `graphics_ui_slider_description_get`, `graphics_ui_slider_description_set`.
- `graphics_ui_slider_value_get`, `graphics_ui_slider_value_set`.

[F45](#f45) (15):

- `graphics_font_default_get`, `graphics_font_description_default_get`, `graphics_font_description_get`, `graphics_font_load`.
- `graphics_font_release`, `graphics_font_retain`, `graphics_font_valid_check`, `graphics_text_create`.
- `graphics_text_description_default_get`, `graphics_text_description_get`, `graphics_text_description_set`, `graphics_text_destroy`.
- `graphics_text_valid_check`, `graphics_text_value_set`, `graphics_text_wrap_width_set`.

[F50](#f50) (6):

- `graphics_aspect_ratio_auto_set`, `graphics_aspect_ratio_set`, `graphics_logical_size_set`, `graphics_window_presentation_description_default_get`.
- `graphics_window_presentation_description_get`, `graphics_window_presentation_description_set`.

[X05](#x05) (53):

- `graphics_aabb_tree_debug_check`, `graphics_aabb_tree_debug_set`, `graphics_aabb_tree_draw`, `graphics_background_draw`.
- `graphics_color_hex_create`, `graphics_contacts_debug_check`, `graphics_contacts_debug_set`, `graphics_contacts_draw`.
- `graphics_events_poll`, `graphics_frame_limit_set`, `graphics_hit_box_colored_draw`, `graphics_hit_box_draw`.
- `graphics_hit_boxes_draw`, `graphics_joint_draw`, `graphics_joints_draw`, `graphics_local_origins_draw`.
- `graphics_mouse_screen_position_get`, `graphics_particle_draw`, `graphics_particles_draw`, `graphics_pipeline_cameras_update`.
- `graphics_pipeline_commands_execute`, `graphics_pipeline_present`, `graphics_pipeline_recording_update`, `graphics_pipeline_viewports_draw`.
- `graphics_recording_start`, `graphics_recording_stop`, `graphics_render_output_size_get`, `graphics_screen_clip_clear`.
- `graphics_screen_clip_pop`, `graphics_screen_clip_push`, `graphics_screen_clip_set`, `graphics_screen_quad_draw`.
- `graphics_screen_quad_radians_draw`, `graphics_screen_rect_draw`, `graphics_screen_shape_filled_draw`, `graphics_screen_text_scaled_rotated_draw`.
- `graphics_screen_text_scaled_rotated_radians_draw`, `graphics_screen_texture_draw`, `graphics_screen_texture_radians_draw`, `graphics_screen_to_world_get`.
- `graphics_show`, `graphics_soft_body_draw`, `graphics_sprites_draw`, `graphics_start`.
- `graphics_stop`, `graphics_text_draw`, `graphics_text_scaled_draw`, `graphics_texture_draw`.
- `graphics_texture_radians_draw`, `graphics_textures_scale`, `graphics_vsync_set`, `graphics_window_to_screen_get`.
- `graphics_world_to_screen_get`.

</details>

<details>
<summary>input.h: 53 mapped entries</summary>

[F48](#f48) (24):

- `input_action_bindings_default_get`, `input_action_bindings_default_set`, `input_action_button_initial_state_check`, `input_action_button_initial_state_set`.
- `input_action_button_mode_get`, `input_action_button_mode_set`, `input_action_by_name_get`, `input_action_create`.
- `input_action_description_default_get`, `input_action_description_get`, `input_action_description_set`, `input_action_destroy`.
- `input_action_type_get`, `input_binding_key_create`, `input_binding_scancode_create`, `input_binding_valid_check`.
- `input_controller_by_name_get`, `input_controller_create`, `input_controller_description_default_get`, `input_controller_description_get`.
- `input_controller_description_set`, `input_controller_destroy`, `input_controller_enabled_check`, `input_controller_enabled_set`.

[F49](#f49) (29):

- `input_action_axis_1d_get`, `input_action_axis_2d_get`, `input_action_bindings_effective_get`, `input_action_bindings_override_check`.
- `input_action_bindings_override_clear`, `input_action_bindings_override_get`, `input_action_bindings_override_set`, `input_action_button_down_check`.
- `input_action_button_pressed_check`, `input_action_button_released_check`, `input_action_button_state_reset`, `input_action_button_state_set`.
- `input_frame_begin`, `input_key_down_check`, `input_key_pressed_check`, `input_key_released_check`.
- `input_modifiers_get`, `input_mouse_button_down_check`, `input_mouse_button_pressed_check`, `input_mouse_button_released_check`.
- `input_mouse_delta_get`, `input_mouse_position_get`, `input_mouse_relative_mode_check`, `input_mouse_relative_mode_set`.
- `input_mouse_wheel_get`, `input_text_area_set`, `input_text_start`, `input_text_state_get`.
- `input_text_stop`.

</details>

<details>
<summary>math2d.h: 29 mapped entries</summary>

[X01](#x01) (29):

- `math_aabb_create`, `math_aabb_overlap`, `math_angular_velocity_cross_vec`, `math_angular_velocity_radians_cross_vec`.
- `math_axis_magnitude`, `math_circle_create`, `math_circle_overlap_depth`, `math_circle_radius`.
- `math_cross_2d`, `math_degrees_to_radians`, `math_dot_product`, `math_normals_create`.
- `math_polygon_centroid`, `math_project_onto_axis`, `math_project_shape_on_axis`, `math_projection_overlap`.
- `math_radians_to_degrees`, `math_shape_scale`, `math_shape_x_scale`, `math_shape_y_scale`.
- `math_square_create`, `math_vector_magnitude`, `math_vector_normalize`, `math_vector_radians_rotate`.
- `math_vector_rotate`, `math_vector_subtract`, `math_vectors_normalize`, `math_vertex_add`.
- `math_vertex_delete`.

</details>

<details>
<summary>memory.h: 4 mapped entries</summary>

[X03](#x03) (4):

- `MEMORY_DECLARE_OBJECT_POOL`, `MEMORY_DEFINE_OBJECT_POOL`, `MEMORY_POOL_ERROR`, `MEMORY_POOL_VALUE`.

</details>

<details>
<summary>object.h: 37 mapped entries</summary>

[F51](#f51) (37):

- `entity_object_add`, `entity_object_add_with_options`, `entity_object_anchor_get`, `entity_object_by_name_add`.
- `entity_object_by_name_add_with_options`, `entity_object_camera_get`, `entity_object_description_add`, `entity_object_description_by_name_add`.
- `entity_object_draw`, `object_acceleration_set`, `object_angular_velocity_set`, `object_collision_description_get`.
- `object_collision_description_set`, `object_color_description_get`, `object_color_description_set`, `object_copy`.
- `object_copy_with_options`, `object_description_by_name_get`, `object_description_copy`, `object_description_get`.
- `object_description_name_get`, `object_description_ref`, `object_destroy`, `object_force_description_set`.
- `object_get`, `object_load`, `object_mass_set`, `object_material_description_get`.
- `object_material_description_set`, `object_orientation_set`, `object_position_set`, `object_radius_set`.
- `object_scale_set`, `object_spawn_description_default_get`, `object_torque_description_set`, `object_velocity_set`.
- `object_visibility_set`.

</details>

<details>
<summary>physics.h: 231 mapped entries</summary>

[F04](#f04) (10):

- `physics_dt_per_tick_get`, `physics_dt_per_tick_set`, `physics_dt_update`, `physics_engine_time_per_tick_use`.
- `physics_gravity_get`, `physics_gravity_set`, `physics_solver_iterations_get`, `physics_solver_iterations_set`.
- `physics_substeps_get`, `physics_substeps_set`.

[F05](#f05) (21):

- `physics_global_hit_box_get`, `physics_hitbox_active_id_get`, `physics_hitbox_active_id_set`, `physics_hitbox_active_index_get`.
- `physics_hitbox_active_index_set`, `physics_hitbox_add`, `physics_hitbox_at_get`, `physics_hitbox_at_remove`.
- `physics_hitbox_at_set`, `physics_hitbox_autoshape_circle_set`, `physics_hitbox_autoshape_rectangle_set`, `physics_hitbox_autoshape_triangle_set`.
- `physics_hitbox_by_id_get`, `physics_hitbox_by_id_remove`, `physics_hitbox_by_id_set`, `physics_hitbox_count_get`.
- `physics_hitbox_get`, `physics_hitbox_id_at_get`, `physics_hitbox_id_at_set`, `physics_hitbox_remove`.
- `physics_hitbox_set`.

[F06](#f06) (10):

- `physics_acceleration_set`, `physics_angular_velocity_get`, `physics_angular_velocity_radians_get`, `physics_angular_velocity_radians_set`.
- `physics_angular_velocity_set`, `physics_orientation_radians_set`, `physics_orientation_set`, `physics_position_get`.
- `physics_position_set`, `physics_velocity_set`.

[F07](#f07) (12):

- `physics_center_of_mass_automatic_check`, `physics_center_of_mass_automatic_set`, `physics_center_of_mass_local_position_get`, `physics_center_of_mass_local_position_set`.
- `physics_center_of_mass_world_position_get`, `physics_circle_moment_of_inertia`, `physics_mass_check`, `physics_mass_get`.
- `physics_mass_remove`, `physics_mass_set`, `physics_moment_of_inertia_get`, `physics_polygon_moment_of_inertia`.

[F08](#f08) (2):

- `physics_friction_set`, `physics_restitution_set`.

[F09](#f09) (13):

- `physics_collision_response_between_check`, `physics_collision_response_to_check`, `physics_collision_category_set`, `physics_collision_filter_description_default_get`, `physics_collision_filter_description_get`.
- `physics_collision_filter_description_set`, `physics_collision_response_all_set`, `physics_collision_response_none_set`, `physics_collision_response_set`.
- `physics_dynamic_set`, `physics_gravity_check`, `physics_gravity_disable`, `physics_gravity_enable`.
- `physics_static_set`.

[F10](#f10) (2):

- `physics_angle_lock_radians_set`, `physics_angle_lock_set`.

[F11](#f11) (7):

- `physics_axis_lock_description_get`, `physics_axis_lock_description_set`, `physics_axis_lock_remove`, `physics_transform_lock_current_transform_set`.
- `physics_transform_lock_description_get`, `physics_transform_lock_description_set`, `physics_transform_lock_remove`.

[F12](#f12) (8):

- `physics_particle_add`, `physics_particle_description_default_get`, `physics_particle_description_get`, `physics_particle_description_set`.
- `physics_particle_origin_get`, `physics_particle_origin_set`, `physics_particle_radius_get`, `physics_particle_radius_set`.

[F13](#f13) (8):

- `physics_soft_body_add`, `physics_soft_body_autoshape_circle_set`, `physics_soft_body_autoshape_rectangle_set`, `physics_soft_body_autoshape_triangle_set`.
- `physics_soft_body_description_default_get`, `physics_soft_body_description_get`, `physics_soft_body_description_set`, `physics_soft_body_get`.

[F14](#f14) (11):

- `physics_soft_body_node_add`, `physics_soft_body_node_collision_filter_description_get`, `physics_soft_body_node_collision_filter_description_set`, `physics_soft_body_node_description_default_get`.
- `physics_soft_body_node_description_get`, `physics_soft_body_node_description_set`, `physics_soft_body_node_get`, `physics_soft_body_node_impulse_apply`.
- `physics_soft_body_node_local_add`, `physics_soft_body_node_local_position_get`, `physics_soft_body_node_local_position_set`.

[F16](#f16) (6):

- `physics_soft_body_beam_add`, `physics_soft_body_beam_description_default_get`, `physics_soft_body_beam_description_get`, `physics_soft_body_beam_description_set`.
- `physics_soft_body_beam_endpoints_set`, `physics_soft_body_beam_get`.

[F18](#f18) (8):

- `physics_soft_body_beam_collision_description_default_get`, `physics_soft_body_beam_collision_description_get`, `physics_soft_body_beam_collision_description_set`, `physics_soft_body_beam_collision_disable`.
- `physics_soft_body_beam_collision_enable`, `physics_soft_body_beam_collision_filter_description_get`, `physics_soft_body_beam_collision_filter_description_set`, `physics_soft_body_beam_collision_thickness_set`.

[F19](#f19) (7):

- `physics_soft_body_area_add`, `physics_soft_body_area_description_default_get`, `physics_soft_body_area_description_get`, `physics_soft_body_area_description_set`.
- `physics_soft_body_area_geometry_set`, `physics_soft_body_area_get`, `physics_soft_body_area_order_set`.

[F20](#f20) (8):

- `physics_joint_anchor_add`, `physics_joint_anchor_description_get`, `physics_joint_anchor_description_set`, `physics_joint_anchor_local_position_get`.
- `physics_joint_anchor_local_position_set`, `physics_joint_anchor_remove`, `physics_joint_anchor_world_position_get`, `physics_joint_anchors_get`.

[F21](#f21) (3):

- `physics_joint_add`, `physics_joint_description_get`, `physics_joint_description_set`.

[F23](#f23) (1):

- `physics_soft_body_node_to_anchor_pin_add`.

[F24](#f24) (8):

- `physics_force_add`, `physics_force_apply`, `physics_force_description_get`, `physics_force_description_set`.
- `physics_force_magnitude_get`, `physics_force_magnitude_set`, `physics_force_target_get`, `physics_force_target_set`.

[F25](#f25) (4):

- `physics_force_location_get`, `physics_force_location_set`, `physics_force_orientation_get`, `physics_force_orientation_set`.

[F26](#f26) (8):

- `physics_torque_add`, `physics_torque_apply`, `physics_torque_description_get`, `physics_torque_description_set`.
- `physics_torque_magnitude_get`, `physics_torque_magnitude_set`, `physics_torque_target_get`, `physics_torque_target_set`.

[F33](#f33) (9):

- `physics_hitbox_animation_binding_at_get`, `physics_hitbox_animation_binding_at_remove`, `physics_hitbox_animation_binding_at_set`, `physics_hitbox_animation_binding_get`.
- `physics_hitbox_animation_binding_hitbox_remove`, `physics_hitbox_animation_binding_remove`, `physics_hitbox_animation_binding_set`, `physics_hitbox_animation_bindings_entity_clear`.
- `physics_hitbox_animation_bindings_update`.

[F53](#f53) (34):

- `physics_acceleration_away_from_entity_set`, `physics_acceleration_away_from_position_set`, `physics_acceleration_toward_entity_set`, `physics_acceleration_toward_position_set`.
- `physics_angular_impulse_apply`, `physics_contact_check`, `physics_contact_count_get`, `physics_contact_entered_check`.
- `physics_contact_exited_check`, `physics_contact_get`, `physics_contact_stayed_check`, `physics_contact_total_impulse_get`.
- `physics_contacts_get`, `physics_entity_stop`, `physics_group_acceleration_away_from_entity_set`, `physics_group_acceleration_toward_entity_set`.
- `physics_group_entities_stop`, `physics_group_velocity_away_from_entity_set`, `physics_group_velocity_toward_entity_set`, `physics_impulse_apply`.
- `physics_overlap_check`, `physics_overlap_count_get`, `physics_overlap_entered_check`, `physics_overlap_exited_check`.
- `physics_overlap_get`, `physics_overlap_stayed_check`, `physics_overlaps_get`, `physics_particle_overlap_get`.
- `physics_sat_overlap_get`, `physics_update`, `physics_velocity_away_from_entity_set`, `physics_velocity_away_from_position_set`.
- `physics_velocity_toward_entity_set`, `physics_velocity_toward_position_set`.

[F54](#f54) (3):

- `physics_angular_acceleration_radians_set`, `physics_angular_acceleration_set`, `physics_target_set`.

[F58](#f58) (10):

- `physics_entity_held_get`, `physics_entity_hold`, `physics_entity_movable_get`, `physics_entity_simulated_get`.
- `physics_entity_unhold`, `physics_group_entities_hold`, `physics_group_entities_unhold`, `physics_kinematic_driven_check`.
- `physics_kinematic_driven_remove`, `physics_kinematic_driven_set`.

[F60](#f60) (4):

- `physics_angular_velocity_maximum_get`, `physics_angular_velocity_maximum_radians_get`, `physics_angular_velocity_maximum_radians_set`, `physics_angular_velocity_maximum_set`.

[X01](#x01) (3):

- `physics_shape_world_radians_translate`, `physics_shape_world_translate`, `physics_world_position_check`.

[X04](#x04) (11):

- `physics_pipeline_accelerations_clear`, `physics_pipeline_constraints_solve`, `physics_pipeline_contacts_gather`, `physics_pipeline_forces_apply`.
- `physics_pipeline_gravity_apply`, `physics_pipeline_integrate`, `physics_pipeline_joints_gather`, `physics_pipeline_step_begin`.
- `physics_pipeline_substep`, `physics_pipeline_substep_begin`, `physics_pipeline_update`.

</details>

<details>
<summary>report.h: 12 mapped entries</summary>

[X02](#x02) (12):

- `report_console_write`, `report_create`, `report_diagnostic_add`, `report_diagnostic_detail_set`.
- `report_entry_add`, `report_get`, `report_header_set`, `report_json_write`.
- `report_merge`, `report_statistic_add`, `report_text_write`, `report_valid_check`.

</details>

<details>
<summary>rohr.h: 789 mapped entries</summary>

[F01](#f01) (21):

- `rohr_entity_add`, `rohr_entity_alive_at_get`, `rohr_entity_alive_check`, `rohr_entity_alive_count_get`.
- `rohr_entity_by_name_get`, `rohr_entity_children_get`, `rohr_entity_components_add`, `rohr_entity_components_check`.
- `rohr_entity_components_delete`, `rohr_entity_delete`, `rohr_entity_description_add`, `rohr_entity_description_get`.
- `rohr_entity_description_set`, `rohr_entity_from_index_get`, `rohr_entity_index_alive_check`, `rohr_entity_index_components_check`.
- `rohr_entity_index_get`, `rohr_entity_name_get`, `rohr_entity_name_set`, `rohr_entity_owner_get`.
- `rohr_entity_owner_set`.

[F02](#f02) (15):

- `rohr_entity_child_remove`, `rohr_entity_child_set`, `rohr_entity_group_add`, `rohr_entity_group_by_name_get`.
- `rohr_entity_group_create`, `rohr_entity_group_destroy`, `rohr_entity_group_entity_check`, `rohr_entity_group_get`.
- `rohr_entity_group_name_get`, `rohr_entity_group_name_set`, `rohr_entity_group_remove`, `rohr_entity_groups_get`.
- `rohr_entity_parent_get`, `rohr_entity_parent_remove`, `rohr_entity_parent_set`.

[F03](#f03) (3):

- `rohr_entity_life_time_remove`, `rohr_entity_lifetime_description_get`, `rohr_entity_lifetime_description_set`.

[F04](#f04) (12):

- `rohr_engine_time_per_tick_get`, `rohr_engine_time_per_tick_set`, `rohr_physics_dt_per_tick_get`, `rohr_physics_dt_per_tick_set`.
- `rohr_physics_dt_update`, `rohr_physics_engine_time_per_tick_use`, `rohr_physics_gravity_get`, `rohr_physics_gravity_set`.
- `rohr_physics_solver_iterations_get`, `rohr_physics_solver_iterations_set`, `rohr_physics_substeps_get`, `rohr_physics_substeps_set`.

[F05](#f05) (21):

- `rohr_physics_global_hit_box_get`, `rohr_physics_hitbox_active_id_get`, `rohr_physics_hitbox_active_id_set`, `rohr_physics_hitbox_active_index_get`.
- `rohr_physics_hitbox_active_index_set`, `rohr_physics_hitbox_add`, `rohr_physics_hitbox_at_get`, `rohr_physics_hitbox_at_remove`.
- `rohr_physics_hitbox_at_set`, `rohr_physics_hitbox_autoshape_circle_set`, `rohr_physics_hitbox_autoshape_rectangle_set`, `rohr_physics_hitbox_autoshape_triangle_set`.
- `rohr_physics_hitbox_by_id_get`, `rohr_physics_hitbox_by_id_remove`, `rohr_physics_hitbox_by_id_set`, `rohr_physics_hitbox_count_get`.
- `rohr_physics_hitbox_get`, `rohr_physics_hitbox_id_at_get`, `rohr_physics_hitbox_id_at_set`, `rohr_physics_hitbox_remove`.
- `rohr_physics_hitbox_set`.

[F06](#f06) (10):

- `rohr_physics_acceleration_set`, `rohr_physics_angular_velocity_get`, `rohr_physics_angular_velocity_radians_get`, `rohr_physics_angular_velocity_radians_set`.
- `rohr_physics_angular_velocity_set`, `rohr_physics_orientation_radians_set`, `rohr_physics_orientation_set`, `rohr_physics_position_get`.
- `rohr_physics_position_set`, `rohr_physics_velocity_set`.

[F07](#f07) (12):

- `rohr_physics_center_of_mass_automatic_check`, `rohr_physics_center_of_mass_automatic_set`, `rohr_physics_center_of_mass_local_position_get`, `rohr_physics_center_of_mass_local_position_set`.
- `rohr_physics_center_of_mass_world_position_get`, `rohr_physics_circle_moment_of_inertia`, `rohr_physics_mass_check`, `rohr_physics_mass_get`.
- `rohr_physics_mass_remove`, `rohr_physics_mass_set`, `rohr_physics_moment_of_inertia_get`, `rohr_physics_polygon_moment_of_inertia`.

[F08](#f08) (2):

- `rohr_physics_friction_set`, `rohr_physics_restitution_set`.

[F09](#f09) (13):

- `rohr_physics_collision_response_between_check`, `rohr_physics_collision_response_to_check`, `rohr_physics_collision_category_set`, `rohr_physics_collision_filter_description_default_get`, `rohr_physics_collision_filter_description_get`.
- `rohr_physics_collision_filter_description_set`, `rohr_physics_collision_response_all_set`, `rohr_physics_collision_response_none_set`, `rohr_physics_collision_response_set`.
- `rohr_physics_dynamic_set`, `rohr_physics_gravity_check`, `rohr_physics_gravity_disable`, `rohr_physics_gravity_enable`.
- `rohr_physics_static_set`.

[F10](#f10) (2):

- `rohr_physics_angle_lock_radians_set`, `rohr_physics_angle_lock_set`.

[F11](#f11) (7):

- `rohr_physics_axis_lock_description_get`, `rohr_physics_axis_lock_description_set`, `rohr_physics_axis_lock_remove`, `rohr_physics_transform_lock_current_transform_set`.
- `rohr_physics_transform_lock_description_get`, `rohr_physics_transform_lock_description_set`, `rohr_physics_transform_lock_remove`.

[F12](#f12) (8):

- `rohr_physics_particle_add`, `rohr_physics_particle_description_default_get`, `rohr_physics_particle_description_get`, `rohr_physics_particle_description_set`.
- `rohr_physics_particle_origin_get`, `rohr_physics_particle_origin_set`, `rohr_physics_particle_radius_get`, `rohr_physics_particle_radius_set`.

[F13](#f13) (8):

- `rohr_physics_soft_body_add`, `rohr_physics_soft_body_autoshape_circle_set`, `rohr_physics_soft_body_autoshape_rectangle_set`, `rohr_physics_soft_body_autoshape_triangle_set`.
- `rohr_physics_soft_body_description_default_get`, `rohr_physics_soft_body_description_get`, `rohr_physics_soft_body_description_set`, `rohr_physics_soft_body_get`.

[F14](#f14) (12):

- `rohr_graphics_soft_body_node_color_set`, `rohr_physics_soft_body_node_add`, `rohr_physics_soft_body_node_collision_filter_description_get`, `rohr_physics_soft_body_node_collision_filter_description_set`.
- `rohr_physics_soft_body_node_description_default_get`, `rohr_physics_soft_body_node_description_get`, `rohr_physics_soft_body_node_description_set`, `rohr_physics_soft_body_node_get`.
- `rohr_physics_soft_body_node_impulse_apply`, `rohr_physics_soft_body_node_local_add`, `rohr_physics_soft_body_node_local_position_get`, `rohr_physics_soft_body_node_local_position_set`.

[F16](#f16) (7):

- `rohr_graphics_soft_body_beam_color_set`, `rohr_physics_soft_body_beam_add`, `rohr_physics_soft_body_beam_description_default_get`, `rohr_physics_soft_body_beam_description_get`.
- `rohr_physics_soft_body_beam_description_set`, `rohr_physics_soft_body_beam_endpoints_set`, `rohr_physics_soft_body_beam_get`.

[F18](#f18) (8):

- `rohr_physics_soft_body_beam_collision_description_default_get`, `rohr_physics_soft_body_beam_collision_description_get`, `rohr_physics_soft_body_beam_collision_description_set`, `rohr_physics_soft_body_beam_collision_disable`.
- `rohr_physics_soft_body_beam_collision_enable`, `rohr_physics_soft_body_beam_collision_filter_description_get`, `rohr_physics_soft_body_beam_collision_filter_description_set`, `rohr_physics_soft_body_beam_collision_thickness_set`.

[F19](#f19) (10):

- `rohr_graphics_soft_body_area_color_clear`, `rohr_graphics_soft_body_area_color_set`, `rohr_graphics_soft_body_area_visibility_set`, `rohr_physics_soft_body_area_add`.
- `rohr_physics_soft_body_area_description_default_get`, `rohr_physics_soft_body_area_description_get`, `rohr_physics_soft_body_area_description_set`, `rohr_physics_soft_body_area_geometry_set`.
- `rohr_physics_soft_body_area_get`, `rohr_physics_soft_body_area_order_set`.

[F20](#f20) (8):

- `rohr_physics_joint_anchor_add`, `rohr_physics_joint_anchor_description_get`, `rohr_physics_joint_anchor_description_set`, `rohr_physics_joint_anchor_local_position_get`.
- `rohr_physics_joint_anchor_local_position_set`, `rohr_physics_joint_anchor_remove`, `rohr_physics_joint_anchor_world_position_get`, `rohr_physics_joint_anchors_get`.

[F21](#f21) (3):

- `rohr_physics_joint_add`, `rohr_physics_joint_description_get`, `rohr_physics_joint_description_set`.

[F23](#f23) (1):

- `rohr_physics_soft_body_node_to_anchor_pin_add`.

[F24](#f24) (8):

- `rohr_physics_force_add`, `rohr_physics_force_apply`, `rohr_physics_force_description_get`, `rohr_physics_force_description_set`.
- `rohr_physics_force_magnitude_get`, `rohr_physics_force_magnitude_set`, `rohr_physics_force_target_get`, `rohr_physics_force_target_set`.

[F25](#f25) (4):

- `rohr_physics_force_location_get`, `rohr_physics_force_location_set`, `rohr_physics_force_orientation_get`, `rohr_physics_force_orientation_set`.

[F26](#f26) (8):

- `rohr_physics_torque_add`, `rohr_physics_torque_apply`, `rohr_physics_torque_description_get`, `rohr_physics_torque_description_set`.
- `rohr_physics_torque_magnitude_get`, `rohr_physics_torque_magnitude_set`, `rohr_physics_torque_target_get`, `rohr_physics_torque_target_set`.

[F30](#f30) (7):

- `rohr_graphics_texture_description_default_get`, `rohr_graphics_texture_description_get`, `rohr_graphics_texture_load`, `rohr_graphics_texture_release`.
- `rohr_graphics_texture_retain`, `rohr_graphics_texture_size_get`, `rohr_graphics_texture_valid_check`.

[F31](#f31) (13):

- `rohr_graphics_sprite_add`, `rohr_graphics_sprite_body_offset_get`, `rohr_graphics_sprite_body_offset_set`, `rohr_graphics_sprite_create`.
- `rohr_graphics_sprite_description_default_get`, `rohr_graphics_sprite_description_get`, `rohr_graphics_sprite_description_set`, `rohr_graphics_sprite_draw`.
- `rohr_graphics_sprite_frames_update`, `rohr_graphics_sprite_orientation_offset_get`, `rohr_graphics_sprite_orientation_offset_radians_get`, `rohr_graphics_sprite_orientation_offset_radians_set`.
- `rohr_graphics_sprite_orientation_offset_set`.

[F32](#f32) (39):

- `rohr_graphics_animated_sprite_add`, `rohr_graphics_animated_sprite_body_offset_get`, `rohr_graphics_animated_sprite_body_offset_set`, `rohr_graphics_animated_sprite_create`.
- `rohr_graphics_animated_sprite_description_default_get`, `rohr_graphics_animated_sprite_description_from_animation_get`, `rohr_graphics_animated_sprite_description_get`, `rohr_graphics_animated_sprite_description_set`.
- `rohr_graphics_animated_sprite_draw`, `rohr_graphics_animated_sprite_frame_add`, `rohr_graphics_animated_sprite_frame_description_get`, `rohr_graphics_animated_sprite_frame_description_set`.
- `rohr_graphics_animated_sprite_frame_offset_get`, `rohr_graphics_animated_sprite_frame_offset_set`, `rohr_graphics_animated_sprite_frame_rotation_get`, `rohr_graphics_animated_sprite_frame_rotation_radians_get`.
- `rohr_graphics_animated_sprite_frame_rotation_radians_set`, `rohr_graphics_animated_sprite_frame_rotation_set`, `rohr_graphics_animated_sprite_frame_scale_get`, `rohr_graphics_animated_sprite_frame_scale_set`.
- `rohr_graphics_animated_sprite_orientation_offset_get`, `rohr_graphics_animated_sprite_orientation_offset_radians_get`, `rohr_graphics_animated_sprite_orientation_offset_radians_set`, `rohr_graphics_animated_sprite_orientation_offset_set`.
- `rohr_graphics_animated_sprite_value_description_get`, `rohr_graphics_animated_sprite_value_description_set`, `rohr_graphics_animated_sprite_value_frame_add`, `rohr_graphics_animated_sprite_value_frame_description_set`.
- `rohr_graphics_animated_sprites_draw`, `rohr_graphics_animation_description_default_get`, `rohr_graphics_animation_description_get`, `rohr_graphics_animation_frame_create`.
- `rohr_graphics_animation_frame_description_default_get`, `rohr_graphics_animation_frame_description_get`, `rohr_graphics_animation_info_get`, `rohr_graphics_animation_load`.
- `rohr_graphics_animation_release`, `rohr_graphics_animation_retain`, `rohr_graphics_animation_valid_check`.

[F33](#f33) (16):

- `rohr_graphics_animated_sprite_frame_index_get`, `rohr_graphics_animated_sprite_frame_index_set`, `rohr_graphics_animated_sprite_update`, `rohr_graphics_animation_player_create`.
- `rohr_graphics_animation_player_description_default_get`, `rohr_graphics_animation_player_description_from_animation_get`, `rohr_graphics_animation_player_description_get`, `rohr_graphics_animation_player_description_set`.
- `rohr_graphics_animation_player_update`, `rohr_physics_hitbox_animation_binding_at_get`, `rohr_physics_hitbox_animation_binding_at_remove`, `rohr_physics_hitbox_animation_binding_at_set`.
- `rohr_physics_hitbox_animation_binding_get`, `rohr_physics_hitbox_animation_binding_remove`, `rohr_physics_hitbox_animation_binding_set`, `rohr_physics_hitbox_animation_bindings_update`.

[F34](#f34) (16):

- `rohr_camera_create`, `rohr_camera_description_default_get`, `rohr_camera_description_get`, `rohr_camera_description_set`.
- `rohr_camera_destroy`, `rohr_camera_disable_set`, `rohr_camera_enable_set`, `rohr_camera_get`.
- `rohr_camera_moving_get`, `rohr_camera_position_move`, `rohr_camera_position_set`, `rohr_camera_zoom_get`.
- `rohr_camera_zoom_set`, `rohr_graphics_camera_move`, `rohr_graphics_camera_radians_rotate`, `rohr_graphics_camera_rotate`.

[F35](#f35) (9):

- `rohr_camera_attachment_description_default_get`, `rohr_camera_attachment_description_get`, `rohr_camera_attachment_description_set`, `rohr_camera_detach`.
- `rohr_camera_entity_attachment_set`, `rohr_camera_position_from_entity_set`, `rohr_graphics_camera_attach`, `rohr_graphics_camera_attached_get`.
- `rohr_graphics_camera_detach`.

[F36](#f36) (2):

- `rohr_camera_pause_with_engine_set`, `rohr_camera_render_when_paused_set`.

[F37](#f37) (3):

- `rohr_camera_active_get`, `rohr_camera_active_set`, `rohr_camera_render_callback_set`.

[F38](#f38) (38):

- `rohr_graphics_layer_active_get`, `rohr_graphics_layer_active_id_set`, `rohr_graphics_layer_active_name_set`, `rohr_graphics_layer_active_set`.
- `rohr_graphics_layer_animation_clear`, `rohr_graphics_layer_animation_get`, `rohr_graphics_layer_animation_id_get`, `rohr_graphics_layer_animation_id_set`.
- `rohr_graphics_layer_animation_name_set`, `rohr_graphics_layer_animation_set`, `rohr_graphics_layer_create`, `rohr_graphics_layer_description_default_get`.
- `rohr_graphics_layer_description_get`, `rohr_graphics_layer_description_set`, `rohr_graphics_layer_destroy`, `rohr_graphics_layer_entity_clear`.
- `rohr_graphics_layer_entity_get`, `rohr_graphics_layer_entity_id_get`, `rohr_graphics_layer_entity_id_set`, `rohr_graphics_layer_entity_name_set`.
- `rohr_graphics_layer_entity_set`, `rohr_graphics_layer_get`, `rohr_graphics_layer_name_get`, `rohr_graphics_layer_name_id_get`.
- `rohr_graphics_layer_name_set`, `rohr_graphics_layer_set`, `rohr_graphics_layer_sprite_clear`, `rohr_graphics_layer_sprite_get`.
- `rohr_graphics_layer_sprite_id_get`, `rohr_graphics_layer_sprite_id_set`, `rohr_graphics_layer_sprite_name_set`, `rohr_graphics_layer_sprite_set`.
- `rohr_graphics_layer_ui_clear`, `rohr_graphics_layer_ui_get`, `rohr_graphics_layer_ui_id_get`, `rohr_graphics_layer_ui_id_set`.
- `rohr_graphics_layer_ui_name_set`, `rohr_graphics_layer_ui_set`.

[F39](#f39) (6):

- `rohr_screen_camera_set`, `rohr_screen_create`, `rohr_screen_description_default_get`, `rohr_screen_description_get`.
- `rohr_screen_description_set`, `rohr_screen_destroy`.

[F40](#f40) (27):

- `rohr_viewport_camera_add`, `rohr_viewport_camera_clear`, `rohr_viewport_camera_set`, `rohr_viewport_create`.
- `rohr_viewport_description_default_get`, `rohr_viewport_description_get`, `rohr_viewport_description_set`, `rohr_viewport_destroy`.
- `rohr_viewport_disable_set`, `rohr_viewport_enable_set`, `rohr_viewport_item_add`, `rohr_viewport_item_description_default_get`.
- `rohr_viewport_item_description_get`, `rohr_viewport_item_description_set`, `rohr_viewport_item_drag_mode_get`, `rohr_viewport_item_drag_mode_set`.
- `rohr_viewport_item_dragging_check`, `rohr_viewport_item_remove`, `rohr_viewport_item_viewport_get`, `rohr_viewport_screen_add`.
- `rohr_viewport_ui_add`, `rohr_viewport_ui_hovered_check`, `rohr_viewport_ui_pressed_check`, `rohr_viewport_ui_shape_add`.
- `rohr_viewport_ui_slider_add`, `rohr_viewport_ui_slider_changed_check`, `rohr_viewport_ui_text_add`.

[F41](#f41) (5):

- `rohr_graphics_ui_destroy`, `rohr_graphics_ui_shape_create`, `rohr_graphics_ui_shape_description_default_get`, `rohr_graphics_ui_shape_description_get`.
- `rohr_graphics_ui_shape_description_set`.

[F42](#f42) (4):

- `rohr_graphics_ui_text_create`, `rohr_graphics_ui_text_description_default_get`, `rohr_graphics_ui_text_description_get`, `rohr_graphics_ui_text_description_set`.

[F43](#f43) (6):

- `rohr_graphics_ui_slider_create`, `rohr_graphics_ui_slider_description_default_get`, `rohr_graphics_ui_slider_description_get`, `rohr_graphics_ui_slider_description_set`.
- `rohr_graphics_ui_slider_value_get`, `rohr_graphics_ui_slider_value_set`.

[F44](#f44) (4):

- `rohr_ui_button_by_name_get`, `rohr_ui_font_by_name_get`, `rohr_ui_label_by_name_get`, `rohr_ui_slider_by_name_get`.

[F45](#f45) (15):

- `rohr_graphics_font_default_get`, `rohr_graphics_font_description_default_get`, `rohr_graphics_font_description_get`, `rohr_graphics_font_load`.
- `rohr_graphics_font_release`, `rohr_graphics_font_retain`, `rohr_graphics_font_valid_check`, `rohr_graphics_text_create`.
- `rohr_graphics_text_description_default_get`, `rohr_graphics_text_description_get`, `rohr_graphics_text_description_set`, `rohr_graphics_text_destroy`.
- `rohr_graphics_text_valid_check`, `rohr_graphics_text_value_set`, `rohr_graphics_text_wrap_width_set`.

[F46](#f46) (24):

- `rohr_audio_music_create`, `rohr_audio_music_description_default_get`, `rohr_audio_music_description_get`, `rohr_audio_music_description_set`.
- `rohr_audio_music_loop_check`, `rohr_audio_music_loop_set`, `rohr_audio_music_playback_rate_get`, `rohr_audio_music_playback_rate_set`.
- `rohr_audio_music_volume_get`, `rohr_audio_music_volume_set`, `rohr_audio_sound_create`, `rohr_audio_sound_description_default_get`.
- `rohr_audio_sound_description_get`, `rohr_audio_sound_description_set`, `rohr_audio_sound_loop_check`, `rohr_audio_sound_loop_set`.
- `rohr_audio_sound_pan_get`, `rohr_audio_sound_pan_set`, `rohr_audio_sound_playback_rate_get`, `rohr_audio_sound_playback_rate_set`.
- `rohr_audio_sound_volume_get`, `rohr_audio_sound_volume_set`, `rohr_audio_volume_get`, `rohr_audio_volume_set`.

[F47](#f47) (17):

- `rohr_audio_music_destroy`, `rohr_audio_music_pause`, `rohr_audio_music_paused_check`, `rohr_audio_music_play`.
- `rohr_audio_music_playing_check`, `rohr_audio_music_resume`, `rohr_audio_music_stop`, `rohr_audio_music_valid_check`.
- `rohr_audio_sound_destroy`, `rohr_audio_sound_play`, `rohr_audio_sound_playing_check`, `rohr_audio_sound_stop`.
- `rohr_audio_sound_valid_check`, `rohr_audio_start`, `rohr_audio_started_check`, `rohr_audio_stop`.
- `rohr_audio_update`.

[F48](#f48) (24):

- `rohr_input_action_bindings_default_get`, `rohr_input_action_bindings_default_set`, `rohr_input_action_button_initial_state_check`, `rohr_input_action_button_initial_state_set`.
- `rohr_input_action_button_mode_get`, `rohr_input_action_button_mode_set`, `rohr_input_action_by_name_get`, `rohr_input_action_create`.
- `rohr_input_action_description_default_get`, `rohr_input_action_description_get`, `rohr_input_action_description_set`, `rohr_input_action_destroy`.
- `rohr_input_action_type_get`, `rohr_input_binding_key_create`, `rohr_input_binding_scancode_create`, `rohr_input_binding_valid_check`.
- `rohr_input_controller_by_name_get`, `rohr_input_controller_create`, `rohr_input_controller_description_default_get`, `rohr_input_controller_description_get`.
- `rohr_input_controller_description_set`, `rohr_input_controller_destroy`, `rohr_input_controller_enabled_check`, `rohr_input_controller_enabled_set`.

[F49](#f49) (29):

- `rohr_input_action_axis_1d_get`, `rohr_input_action_axis_2d_get`, `rohr_input_action_bindings_effective_get`, `rohr_input_action_bindings_override_check`.
- `rohr_input_action_bindings_override_clear`, `rohr_input_action_bindings_override_get`, `rohr_input_action_bindings_override_set`, `rohr_input_action_button_down_check`.
- `rohr_input_action_button_pressed_check`, `rohr_input_action_button_released_check`, `rohr_input_action_button_state_reset`, `rohr_input_action_button_state_set`.
- `rohr_input_frame_begin`, `rohr_input_key_down_check`, `rohr_input_key_pressed_check`, `rohr_input_key_released_check`.
- `rohr_input_modifiers_get`, `rohr_input_mouse_button_down_check`, `rohr_input_mouse_button_pressed_check`, `rohr_input_mouse_button_released_check`.
- `rohr_input_mouse_delta_get`, `rohr_input_mouse_position_get`, `rohr_input_mouse_relative_mode_check`, `rohr_input_mouse_relative_mode_set`.
- `rohr_input_mouse_wheel_get`, `rohr_input_text_area_set`, `rohr_input_text_start`, `rohr_input_text_state_get`.
- `rohr_input_text_stop`.

[F50](#f50) (6):

- `rohr_graphics_aspect_ratio_auto_set`, `rohr_graphics_aspect_ratio_set`, `rohr_graphics_logical_size_set`, `rohr_graphics_window_presentation_description_default_get`.
- `rohr_graphics_window_presentation_description_get`, `rohr_graphics_window_presentation_description_set`.

[F51](#f51) (36):

- `rohr_entity_object_add`, `rohr_entity_object_add_with_options`, `rohr_entity_object_anchor_get`, `rohr_entity_object_by_name_add`.
- `rohr_entity_object_by_name_add_with_options`, `rohr_entity_object_camera_get`, `rohr_entity_object_description_add`, `rohr_entity_object_description_by_name_add`.
- `rohr_entity_object_draw`, `rohr_entity_object_member_by_name_get`, `rohr_entity_object_member_name_get`, `rohr_entity_object_members_get`.
- `rohr_object_acceleration_set`, `rohr_object_angular_velocity_set`, `rohr_object_collision_description_get`, `rohr_object_collision_description_set`.
- `rohr_object_color_description_get`, `rohr_object_color_description_set`, `rohr_object_copy`, `rohr_object_copy_with_options`.
- `rohr_object_description_by_name_get`, `rohr_object_destroy`, `rohr_object_force_description_set`, `rohr_object_get`.
- `rohr_object_load`, `rohr_object_mass_set`, `rohr_object_material_description_get`, `rohr_object_material_description_set`.
- `rohr_object_orientation_set`, `rohr_object_position_set`, `rohr_object_radius_set`, `rohr_object_scale_set`.
- `rohr_object_spawn_description_default_get`, `rohr_object_torque_description_set`, `rohr_object_velocity_set`, `rohr_object_visibility_set`.

[F52](#f52) (4):

- `rohr_game_state_file_load`, `rohr_game_state_file_save`, `rohr_game_state_files_load`, `rohr_game_state_template_file_save`.

[F53](#f53) (34):

- `rohr_physics_acceleration_away_from_entity_set`, `rohr_physics_acceleration_away_from_position_set`, `rohr_physics_acceleration_toward_entity_set`, `rohr_physics_acceleration_toward_position_set`.
- `rohr_physics_angular_impulse_apply`, `rohr_physics_contact_check`, `rohr_physics_contact_count_get`, `rohr_physics_contact_entered_check`.
- `rohr_physics_contact_exited_check`, `rohr_physics_contact_get`, `rohr_physics_contact_stayed_check`, `rohr_physics_contact_total_impulse_get`.
- `rohr_physics_contacts_get`, `rohr_physics_entity_stop`, `rohr_physics_group_acceleration_away_from_entity_set`, `rohr_physics_group_acceleration_toward_entity_set`.
- `rohr_physics_group_entities_stop`, `rohr_physics_group_velocity_away_from_entity_set`, `rohr_physics_group_velocity_toward_entity_set`, `rohr_physics_impulse_apply`.
- `rohr_physics_overlap_check`, `rohr_physics_overlap_count_get`, `rohr_physics_overlap_entered_check`, `rohr_physics_overlap_exited_check`.
- `rohr_physics_overlap_get`, `rohr_physics_overlap_stayed_check`, `rohr_physics_overlaps_get`, `rohr_physics_particle_overlap_get`.
- `rohr_physics_sat_overlap_get`, `rohr_physics_update`, `rohr_physics_velocity_away_from_entity_set`, `rohr_physics_velocity_away_from_position_set`.
- `rohr_physics_velocity_toward_entity_set`, `rohr_physics_velocity_toward_position_set`.

[F54](#f54) (3):

- `rohr_physics_angular_acceleration_radians_set`, `rohr_physics_angular_acceleration_set`, `rohr_physics_target_set`.

[F58](#f58) (8):

- `rohr_physics_entity_held_get`, `rohr_physics_entity_hold`, `rohr_physics_entity_unhold`, `rohr_physics_group_entities_hold`.
- `rohr_physics_group_entities_unhold`, `rohr_physics_kinematic_driven_check`, `rohr_physics_kinematic_driven_remove`, `rohr_physics_kinematic_driven_set`.

[F59](#f59) (39):

- `rohr_ui_border`, `rohr_ui_button`, `rohr_ui_button_disabled`, `rohr_ui_button_style_default_get`.
- `rohr_ui_clip_begin`, `rohr_ui_clip_end`, `rohr_ui_component_bounds_get`, `rohr_ui_content`.
- `rohr_ui_dropdown`, `rohr_ui_dropdown_actions`, `rohr_ui_event_add`, `rohr_ui_field`.
- `rohr_ui_field_event_add`, `rohr_ui_field_focus_clear`, `rohr_ui_input_set`, `rohr_ui_interaction`.
- `rohr_ui_key_pressed_check`, `rohr_ui_label`, `rohr_ui_menu`, `rohr_ui_modal_controls_begin`.
- `rohr_ui_modal_controls_end`, `rohr_ui_modal_set`, `rohr_ui_multiline_field`, `rohr_ui_navigation_activate`.
- `rohr_ui_navigation_focus_bounds_get`, `rohr_ui_navigation_move`, `rohr_ui_pointer_consumed_get`, `rohr_ui_primary_pressed_check`.
- `rohr_ui_quad`, `rohr_ui_radians_quad`, `rohr_ui_scroll_region_begin`, `rohr_ui_scroll_region_end`.
- `rohr_ui_slider`, `rohr_ui_slider_config_default_get`, `rohr_ui_slider_with_text`, `rohr_ui_surface`.
- `rohr_ui_translation_y_pop`, `rohr_ui_translation_y_push`, `rohr_ui_update`.

[F60](#f60) (4):

- `rohr_physics_angular_velocity_maximum_get`, `rohr_physics_angular_velocity_maximum_radians_get`, `rohr_physics_angular_velocity_maximum_radians_set`, `rohr_physics_angular_velocity_maximum_set`.

[X01](#x01) (31):

- `rohr_math_aabb_create`, `rohr_math_angular_velocity_cross_vec`, `rohr_math_angular_velocity_radians_cross_vec`, `rohr_math_axis_magnitude`.
- `rohr_math_circle_create`, `rohr_math_circle_overlap_depth`, `rohr_math_circle_radius`, `rohr_math_cross_2d`.
- `rohr_math_degrees_to_radians`, `rohr_math_dot_product`, `rohr_math_normals_create`, `rohr_math_polygon_centroid`.
- `rohr_math_project_onto_axis`, `rohr_math_project_shape_on_axis`, `rohr_math_projection_overlap`, `rohr_math_radians_to_degrees`.
- `rohr_math_shape_scale`, `rohr_math_shape_x_scale`, `rohr_math_shape_y_scale`, `rohr_math_square_create`.
- `rohr_math_vector_magnitude`, `rohr_math_vector_normalize`, `rohr_math_vector_radians_rotate`, `rohr_math_vector_rotate`.
- `rohr_math_vector_subtract`, `rohr_math_vectors_normalize`, `rohr_math_vertex_add`, `rohr_math_vertex_delete`.
- `rohr_physics_shape_world_radians_translate`, `rohr_physics_shape_world_translate`, `rohr_physics_world_position_check`.

[X02](#x02) (31):

- `rohr_console_active_get`, `rohr_console_debug_set`, `rohr_console_debug_write`, `rohr_console_logs_print`.
- `rohr_console_read`, `rohr_console_start`, `rohr_console_stop`, `rohr_console_write`.
- `rohr_entity_info_check`, `rohr_entity_info_get`, `rohr_entity_info_issues_count_get`, `rohr_entity_info_issues_get`.
- `rohr_entity_info_release`, `rohr_entity_info_retain`, `rohr_error_check`, `rohr_error_code_message_get`.
- `rohr_error_message_get`, `rohr_error_result_error`, `rohr_error_result_value`, `rohr_report_console_write`.
- `rohr_report_create`, `rohr_report_diagnostic_add`, `rohr_report_diagnostic_detail_set`, `rohr_report_entry_add`.
- `rohr_report_get`, `rohr_report_header_set`, `rohr_report_json_write`, `rohr_report_merge`.
- `rohr_report_statistic_add`, `rohr_report_text_write`, `rohr_report_valid_check`.

[X04](#x04) (26):

- `rohr_engine_clock_reset`, `rohr_engine_event_poll`, `rohr_engine_lifecycle_update`, `rohr_engine_pause`.
- `rohr_engine_paused_get`, `rohr_engine_resume`, `rohr_engine_start`, `rohr_engine_stop`.
- `rohr_engine_tick_delta_get`, `rohr_engine_tick_get`, `rohr_engine_tick_update`, `rohr_engine_time_get`.
- `rohr_engine_time_update`, `rohr_engine_update`, `rohr_physics_pipeline_accelerations_clear`, `rohr_physics_pipeline_constraints_solve`.
- `rohr_physics_pipeline_contacts_gather`, `rohr_physics_pipeline_forces_apply`, `rohr_physics_pipeline_gravity_apply`, `rohr_physics_pipeline_integrate`.
- `rohr_physics_pipeline_joints_gather`, `rohr_physics_pipeline_step_begin`, `rohr_physics_pipeline_substep`, `rohr_physics_pipeline_substep_begin`.
- `rohr_physics_pipeline_update`, `rohr_system_physics_update`.

[X05](#x05) (52):

- `rohr_graphics_aabb_tree_debug_check`, `rohr_graphics_aabb_tree_debug_set`, `rohr_graphics_aabb_tree_draw`, `rohr_graphics_background_draw`.
- `rohr_graphics_color_hex_create`, `rohr_graphics_contacts_debug_check`, `rohr_graphics_contacts_debug_set`, `rohr_graphics_contacts_draw`.
- `rohr_graphics_events_poll`, `rohr_graphics_frame_limit_set`, `rohr_graphics_hit_box_colored_draw`, `rohr_graphics_hit_box_draw`.
- `rohr_graphics_hit_boxes_draw`, `rohr_graphics_joint_draw`, `rohr_graphics_joints_draw`, `rohr_graphics_local_origins_draw`.
- `rohr_graphics_mouse_screen_position_get`, `rohr_graphics_particle_draw`, `rohr_graphics_particles_draw`, `rohr_graphics_pipeline_cameras_update`.
- `rohr_graphics_pipeline_commands_execute`, `rohr_graphics_pipeline_present`, `rohr_graphics_pipeline_recording_update`, `rohr_graphics_pipeline_viewports_draw`.
- `rohr_graphics_recording_start`, `rohr_graphics_recording_stop`, `rohr_graphics_render_output_size_get`, `rohr_graphics_screen_clip_clear`.
- `rohr_graphics_screen_clip_pop`, `rohr_graphics_screen_clip_push`, `rohr_graphics_screen_clip_set`, `rohr_graphics_screen_quad_draw`.
- `rohr_graphics_screen_quad_radians_draw`, `rohr_graphics_screen_rect_draw`, `rohr_graphics_screen_shape_filled_draw`, `rohr_graphics_screen_text_scaled_rotated_draw`.
- `rohr_graphics_screen_text_scaled_rotated_radians_draw`, `rohr_graphics_screen_texture_draw`, `rohr_graphics_screen_texture_radians_draw`, `rohr_graphics_screen_to_world_get`.
- `rohr_graphics_show`, `rohr_graphics_soft_body_draw`, `rohr_graphics_sprites_draw`, `rohr_graphics_start`.
- `rohr_graphics_stop`, `rohr_graphics_text_draw`, `rohr_graphics_text_scaled_draw`, `rohr_graphics_texture_draw`.
- `rohr_graphics_texture_radians_draw`, `rohr_graphics_textures_scale`, `rohr_graphics_vsync_set`, `rohr_graphics_world_to_screen_get`.

[X06](#x06) (8):

- `rohr_directory_base_get`, `rohr_directory_working_set`, `rohr_tools_append_string`, `rohr_tools_binary_to_string`.
- `rohr_tools_delay`, `rohr_tools_random_range`, `rohr_tools_random_range_float`, `rohr_tools_sizeof_string`.

</details>

<details>
<summary>rohr_tools.h: 4 mapped entries</summary>

[X03](#x03) (4):

- `rohr_tools_component_registry_component_add`, `rohr_tools_component_registry_generate`, `rohr_tools_component_registry_init`, `rohr_tools_component_registry_tag_add`.

</details>

<details>
<summary>scene.h: 2 mapped entries</summary>

[X04](#x04) (2):

- `rohr_scene_run`, `scene_run`.

</details>

<details>
<summary>systems.h: 1 mapped entries</summary>

[X04](#x04) (1):

- `system_physics_update`.

</details>

<details>
<summary>tools.h: 6 mapped entries</summary>

[X06](#x06) (6):

- `binary_to_string`, `delay`, `tool_sizeof_string`, `tools_append_string`.
- `tools_random_range`, `tools_random_range_float`.

</details>

<details>
<summary>ui.h: 40 mapped entries</summary>

[F59](#f59) (40):

- `ui_border`, `ui_button`, `ui_button_disabled`, `ui_button_style_default_get`.
- `ui_clip_begin`, `ui_clip_end`, `ui_component_bounds_get`, `ui_content`.
- `ui_dropdown`, `ui_dropdown_actions`, `ui_event_add`, `ui_field`.
- `ui_field_event_add`, `ui_field_focus_clear`, `ui_field_focus_set`, `ui_input_set`.
- `ui_interaction`, `ui_key_pressed_check`, `ui_label`, `ui_menu`.
- `ui_modal_controls_begin`, `ui_modal_controls_end`, `ui_modal_set`, `ui_multiline_field`.
- `ui_navigation_activate`, `ui_navigation_focus_bounds_get`, `ui_navigation_move`, `ui_pointer_consumed_get`.
- `ui_primary_pressed_check`, `ui_quad`, `ui_radians_quad`, `ui_scroll_region_begin`.
- `ui_scroll_region_end`, `ui_slider`, `ui_slider_config_default_get`, `ui_slider_with_text`.
- `ui_surface`, `ui_translation_y_pop`, `ui_translation_y_push`, `ui_update`.

</details>

### Macro-generated and non-configuration types

The [storage interface inventory](description_api_contract.md#macro-generated-storage-interfaces)
is included by reference: all eleven generated pool methods for each of its
35 listed pool prefixes map to X03. Result-construction/token-pasting macros
remain X02/X03 support machinery, not additional authorable fields. Public
non-configuration records such as interaction/contact results, entity inspection
snapshots, reports, asset handles, animation progress and pool backing storage
are outputs or live/derived state. Their operations map through the callable
ledger; they are not additional description schemas to serialize automatically.
Enums/flags, scalar focused setters and non-description creation arguments are
covered by the matrix/callable ledger even when no configuration struct exists
(for example rigid hitbox geometry and F54 angular configuration).

## Verification and completion boundary

Goal 1 checks performed:

- `nix develop -c sh -c 'perl scripts/verify_api_inventory.pl && perl tests/api_inventory.pl'`
  passed: 20 installed headers, 1,602 entries, independent prototype comparison,
  three legacy configuration names, totals/links, and 13 inventory-checker
  validation cases.
- Exact API partition, public/editor field extraction and assignment, referenced
  feature/evidence/gap IDs, local files/anchors and roadmap checklist checked.
- `git diff --check` passed. Existing unrelated edits preserved.

No engine builds, runtime fixture execution, sanitizers, Windows cross-builds or
interactive verification were performed for this documentation goal. Source and
fixture presence do not imply those checks passed. Goal 1 completed the inventory
reconciliation. Later goals record their own behavioral and platform evidence;
milestone 4 is now complete by user acceptance with the exceptions at the top of
this document.


**C12 complete — explicit motion authoring and requested whole-body control:**

Rigid bodies, standalone particles, and individual soft nodes expose Dynamic /
Static / Kinematic. Applicable homogeneous and mixed bulk selections show their
current mode or mixed state and use the shared undo transaction. Static keeps
stored mass, motion, gravity, response masks, and dormant explicit intent;
selecting Dynamic clears explicit intent. Zero-mass Dynamic rigid bodies and
particles remain automatically kinematic and display “Zero mass: behaves
kinematically.” Positive mass restores automatic dynamic simulation. Explicit
Kinematic survives mass changes. Soft-node authoring/creation still requires
positive mass. At final review the user requested whole-soft-body motion and
accepted C12 conditional on that addition. The body Physics selector now applies
motion to all existing nodes and stores defaults for newly added nodes. Individual
node overrides produce Mixed Motion; later body edits affect only motion mode.
Empty bodies show their default. Whole-body collision and gravity remain C13.

`kinematic_driven` is part of authored bodies/nodes, object descriptions, particle
and node full descriptions, catalogs, and generated constructors. Description
getters report explicit intent, including while Static, rather than the effective
zero-mass mode. Full replacement can clear intent at zero mass by using the
existing low-level Static/removal/Dynamic transitions; those low-level APIs and
the physics pipeline retain their semantics. Generated direct rigid constructors
now retain stored Static mass, motion, gravity, and rotation-lock settings.
Mode changes do not modify directional response masks.

Project format 13 writes the boolean flag; format 12 accepts it optionally and
validates supplied values as booleans. Missing intent defaults false. Editor-only
soft-body `node_static_default` and `node_kinematic_driven_default` also persist as
validated optional booleans; missing defaults are false in formats 12/13. Runtime
construction uses resolved node values, without adding runtime container modes. C11 still
migrates project 11 → 12, after which ordinary loading/saving writes 13. Runtime
format remains 10 with its existing explicit-intent storage. Developer-owned
`main.c` files were not changed. The three bundled generated examples were
refreshed from their authored data.

Acceptance evidence (2026-10-23):

1. **Authoring/history:** `editor_motion_authoring` exercises each selector through
   pointer input, all three entity types plus whole soft bodies, mixed bulk display and editing,
   undo/redo, duplication, and preservation of stored values. The zero-mass notice
   adds measured layout space and disappears when mass becomes positive. Whole-body
   tests cover all three selectors, mixed nodes, empty bodies, defaults for new
   nodes, Static's per-node dormant intent, subsequent body overwrite, duplication,
   saved defaults, and exact preservation of unrelated node settings.
2. **Automatic/explicit:** full description replacement, getters, mass edits,
   Static/dynamic restoration, zero-mass particles, and independent spawned
   instances distinguish inferred behavior from explicit intent.
3. **Physics:** all three collider types move with prescribed velocity, ignore
   gravity/contact impulses while Kinematic, influence responding dynamic peers,
   remain fixed while Static, and regain dynamic contact response after clearing
   intent with positive mass. Existing collision-response regressions also pass.
4. **Cross-path parity:** CLI mode/static commands and serialization, project
   12/13 defaults and malformed flags, save/reload, runtime snapshot restoration,
   runtime project import, catalogs, and repeated spawning pass. Published CLI
   whole-body mode/static commands also preserve saved node values and defaults.
   The installed-SDK
   `editor_motion_generated` consumer executes direct generated constructors,
   generated catalogs, and file-loaded objects, including nodes configured by a
   whole-body motion command. Migration regressions pass.
5. **Delivery:** all 125 Linux tests passed after the whole-body addition. The
   existing soft-mass pointer test was adjusted for the new Motion row and passed
   with its mass/history assertions intact. Six ASan/UBSan suites passed with leak
   detection: motion authoring, particle authoring, collision migration, runtime
   persistence, collision response, and object instances. The Windows engine,
   editor, CLI, focused tests, and generated fixture cross-built without compiler
   warnings. COM, player-controller, and soft-body examples independently built
   against Linux/Windows SDKs; Linux startup and interrupt checks ran from `/tmp`.
   Native Windows execution was not performed. User editor review accepted the
   result conditional on adding whole-body motion; that addition is now complete
   with pointer/state regression coverage. No image analysis was used.

Reproduce the focused fixture with
`build/tests/editor_motion_authoring_test /tmp/c12_editor_review` using a new
or empty destination and dummy SDL video/audio drivers for fixture generation.
The generated test is `ctest --test-dir build -R editor_motion_generated` with an
up-to-date `ROHR_SDK_INSTALL_PREFIX`. The review editor is open on that fixture.
Open `Motion`; `body_0`/`particle_0`/`node_0` are Dynamic, suffix `_1` is explicit
Kinematic, `_2` is Static with dormant intent, and `_3` is zero-mass Dynamic for
bodies/particles (positive-mass Dynamic for nodes).

The user reviewed `/tmp/c12_editor_review` and requested the missing whole-body
motion control: “Just that functionality then PASS.” The rebuilt editor exposes
it under the soft body's Physics accordion. The existing review window must be
restarted to use the rebuilt executable. Reproduced fixtures also include
`driven_soft`, whose nodes receive explicit Kinematic through the body command.

C12 is complete. C7/C9's earlier manual review exceptions remain unchanged.
C13 verification follows. C14 has not started.

**C13 complete — whole-soft-body node physics controls:**

The parent Physics panel and compatible bulk selections expose Gravity,
Collision, Category, Responds To, Friction, and Restitution. Body edits update
existing nodes and defaults for future nodes; each command changes only its
property, or its selected mask bit. Individual node overrides produce Mixed
values, while empty parents display defaults. Materials commit on Enter. Beam
settings remain independent and beam collision stays opt-in. The built-in
`default` collision mask now resolves by its stored name before C-identifier
normalization, so toggling it works in UI and CLI.

Project format remains 13 with optional, validated defaults accepted in 12/13;
missing fields retain historical new-node values and never overwrite existing
nodes. Runtime format remains 10. Resolved node values flow through generated
constructors, catalogs, file import, snapshots, and independent spawns. Direct
generated constructors now restore disabled collision after the existing
restitution setter and preserve stored category/response masks. No physics
pipeline semantics or developer-owned `main.c` files changed.

Acceptance evidence (2026-10-23):

1. **Authoring and history:** `editor_soft_physics` exercises parent and mixed
   bulk pointer controls, Enter-only material commits, invalid input, undo/redo,
   empty bodies, new-node inheritance, and object data duplication. Exact node
   comparisons prove unrelated properties remain unchanged; beam data remains
   unchanged by parent edits.
2. **Mixed values and defaults:** independent node overrides produce mixed
   gravity, collision, materials, and mask bits; subsequent property edits
   preserve other overrides. Adding/toggling a named mask preserves unrelated
   bits and shares one history transaction. JSON round trips preserve defaults;
   malformed supplied defaults fail, and missing defaults in both supported
   project versions preserve existing node records.
3. **Physics behavior:** deterministic catalog-spawned nodes verify gravity
   acceleration, collision-disabled prescribed movement, restitution bounce,
   and tangential friction response. Directional response and motion regression
   suites retain coverage of masks and Dynamic/Static/Kinematic behavior.
4. **Cross-path parity:** raw command parsing/serialization and the published
   CLI exercise all six properties. Runtime project import and snapshot restore
   preserve node settings. `editor_soft_physics_generated` builds an installed-SDK
   application, scene, and assertion consumer; the consumer executes direct
   generated constructors, generated catalogs, and file-loaded catalogs with
   repeated independent instances. C11 migration regressions remain passing.
5. **Delivery and user review:** all 127 Linux tests pass. Eight scene/process
   launch tests initially failed under sandbox restrictions and passed when
   rerun outside the sandbox. Both documentation checks pass after the updates.
   The focused seven ASan/UBSan suites pass with
   leak detection. The Windows editor, CLI, focused tests, and installed-SDK
   application/scene/consumer cross-build without compiler warnings. COM,
   player-controller, and soft-body examples regenerate without tracked changes,
   build independently against both SDKs, and pass Linux startup/interrupt checks
   outside their source directories. Native Windows execution was not performed.
   The user reviewed `/tmp/c13_editor_review` in “C13 review - soft-body physics”
   and reported PASS. The user explicitly deferred duplication UI to later work;
   the existing object-data duplication path is tested. No image analysis was used.

Reproduce the focused test with `ctest --test-dir build -R '^editor_soft_physics'`
and an up-to-date `ROHR_SDK_INSTALL_PREFIX`. Create a fresh manual fixture with
`build/tests/editor_soft_physics_test /tmp/c13_editor_review_new` under dummy SDL
video/audio drivers; open `SoftPhysics` → `cloth` → Physics. Check parent settings,
node overrides and Mixed display, category/response bits, new-node inheritance,
compatible bulk selections, and undo/redo. Keep beams unchanged. Do not regenerate
over an in-progress user review project.

Verification logs are `/tmp/c13_final_tests.log`, `/tmp/c13_launch_retests.log`,
`/tmp/c13_sanitizer_tests.log`, `/tmp/c13_windows_build.log`,
`/tmp/c13_generated_windows.log`, and `/tmp/c13_examples.log`.
C7/C9 review exceptions remain unchanged. C14 implementation and review follow.


**C14 complete — immediate joint placement and spring editing aids:**

Editor and CLI joint configuration commands resolve complete pin/weld and
explicitly locked spring connections immediately. Physical transforms move
connected geometry atomically, including soft-body node attachments and selected
connected groups. Configuration failures retain the setting and report the
constraint without changing geometry. Impossible pointer steps are silent;
diagonal dragging retains its allowed projection onto custom shifted axis lines.
Static has no effect on editor movement. Runtime low-level setters and creation,
explicit runtime placement restrictions, and simulation behavior remain unchanged.

The spring Parameters panel exposes **Lock Editor Rest Length** (default false),
with command/history and CLI `lock-editor-restlength`. Project format 13 remains
current and format 12 remains accepted; the optional boolean
`editor_rest_length_locked` defaults false and rejects nonboolean input. Runtime
format stays 10 and has no editor lock. Generated constructors/catalogs and file
spawns retain saved poses and ordinary spring dynamics. Editor-only guides show a
solid centered rest segment plus dotted stretched extensions, or a compressed
spring plus a dotted full-rest ghost. Guides do not enlarge the hit region.

Acceptance evidence (2026-10-26):

1. **Immediate authoring:** `editor_joint_authoring` covers configuration,
   endpoint completion, incompatible constraints, unchanged geometry on failure,
   Static editing, body/node/anchor connections, and grouped transforms.
2. **History and input:** state and pointer tests cover connected dragging,
   one transaction per gesture, cancellation, undo/redo, rest-lock checkbox,
   custom-axis projection, and atomic conflict refusal. Existing core/history
   regressions cover selected connected rotation and unrelated orphan joints.
3. **Spring behavior:** geometry assertions cover stretched/compressed/coincident
   and zero-rest-length guides. Unlocked springs remain freely editable; enabling
   the editor lock places immediately. Object-data duplication and project 12/13
   round trips retain the flag without changing runtime motion.
4. **Cross-path delivery:** `editor_joint_generated` builds an installed-SDK
   application, scene, and assertion consumer. Direct constructors, generated
   catalogs, and file catalogs spawn the saved rest-distance pose; a zero-stiffness
   spring then changes length under prescribed velocity, proving the editor lock
   does not constrain runtime physics. Repeated instances are independent.
5. **Editor review:** PASS for joint/spring behavior in `/tmp/c14_editor_review_projected`, window
   “C14 review - projected dragging”. Check JointPins/JointWeld immediate edits,
   connected dragging and undo/redo; JointStatic editor movement; JointSpring
   guides, lock, and save/reopen; and JointAxisLocked diagonal projection and
   silent refusal. No image analysis is used. C15 error presentation and C16
   numeric property locks are not implemented. Body-relative axis following is
   explicitly deferred by the user; current arbitrary direction/point controls
   are retained. The user requested a simpler axis UI after accepting the other
   checks: Angle clockwise from +Y plus Shift X/Y replaces the vector-component
   fields. Final review of `/tmp/c14_editor_review_commit` in “C14 final review -
   axis commits”: **PASS**. Enter and focus loss commit and immediately place
   connected geometry; typing drafts does not move geometry, and Escape cancels
   the draft. `editor_configuration_controls` verifies these input paths and
   undo/redo of both configuration and poses.

Reproduce with `ctest --test-dir build -R '^editor_joint'` and an up-to-date
installed SDK. Generate a fresh review project using dummy SDL video/audio and
`build/tests/editor_joint_authoring_test /tmp/c14_editor_review_new`; never
regenerate over an in-progress user review project.

The final revision passed all 129 Linux tests and seven ASan/UBSan suites with
leak detection, including the shared numeric controls and soft-body regressions. The Windows editor/CLI/tests and installed-SDK
application/scene/consumer cross-built without compiler warnings. Four examples
(COM, player-controller, soft-body, joints) built independently against both SDKs
and passed Linux startup/interrupt checks outside their source directories.
The generated Linux application and scene also passed startup/interrupt checks.
Native Windows execution was not performed. Regeneration preserved developer
main.c files and produced no tracked example changes; temporary generated
sidecars were archived under `/tmp/c14_generation_sidecars`.

Logs: `/tmp/c14_final_tests.log`, `/tmp/c14_sanitizer_tests.log`,
`/tmp/c14_windows_build.log`, `/tmp/c14_generated_windows.log`,
`/tmp/c14_generated_startup.log`, `/tmp/c14_examples.log`, and
`/tmp/c14_cli_check.log`. The published CLI also verifies spring locks, Static
editing, and immediate custom axis/shift placement with saved values. Documentation
checks and `git diff --check` pass. At C14 closure, C15/C16 remained unstarted and
C7/C9's existing review exceptions remained unchanged.

**C15 complete — shared current-error presentation:**

The editor derives diagnostics from the current authored configuration. Affected
geometry gets red exterior outlines where it can be rendered; affected items and
ancestor rows, fields, and accordion headers use shared red indicators. Collapsed
accordions retain the indicator. Reasons appear in the measured, scrollable
properties panel. Fixing an error clears current styling; historical notifications
remain in the log. Rejected input that leaves valid stored data unchanged creates
a contextual notification without persistent red styling. Pointer refusals stay
silent.

Opening a project logs its existing errors immediately. Unchanged snapshots do
not generate duplicates; resolution followed by recurrence, including undo, logs
the returning error. Messages retain project, object/item, and field context;
joint command failures also identify endpoints and their owners. Diagnostic
snapshots own their text and are released with the editor. They are derived state,
not serialized project or runtime data.

Acceptance evidence (2026-10-26):

1. **Current state and recovery:** `editor_error_presentation` covers retained
   blocked weld configuration, missing references, independent errors,
   correction, undo/redo, and project round trips. Read-only joint status reuses
   existing satisfaction rules without moving geometry. Ordinary spring stretch
   remains valid.
2. **Shared presentation:** regression checks cover exact and ancestor targets,
   field/accordion mapping, collapsed/expanded accordion drawing, object/layout
   ID separation, soft-node/area and camera errors, and viewport drawing. Inline
   reasons participate in normal content measurement and scrolling.
3. **Notifications:** tests cover initial population, unchanged-frame deduplication,
   reopening, correction and recurrence, retained history, contextual rejected
   numeric commits, and silent pointer rejection. Rejected edits do not falsely
   mark valid stored properties as errors.
4. **Delivery:** all 130 Linux tests and five focused ASan/UBSan suites pass with
   leak detection (`editor_error_presentation`, `editor_panel_lifecycle`,
   `editor_configuration_controls`, `editor_joint_authoring`, and
   `editor_reference_picker`). Windows editor, CLI, and affected tests cross-build.
   The joints example builds independently against both installed SDKs and passes
   Linux startup/interrupt checks outside its source directory. Final builds have
   no compiler warnings; native Windows execution was not performed.
5. **Editor review:** the user reports **PASS** in “C15 review - accordions and
   error log”, using their sandbox copy of the fixture. The accepted checks
   include startup log entries without repetition, red collapsed Parameters/
   Connections headers, clearing on correction, retained log history, and
   styling/log recurrence through undo. No image analysis was used, and the
   user's sandbox copy was not overwritten.

Reproduce the focused regression with
`ctest --test-dir build --output-on-failure -R '^editor_error_presentation$'`.
Create a fresh manual fixture with dummy SDL video/audio drivers and
`build/tests/editor_error_presentation_test --fixture <new-project-directory>`;
never regenerate over an active user review. `BlockedWeld` has an incompatible
rest angle; setting it to 0 resolves it. `MissingEndpoint` has an unset Anchor B;
its right body's anchor is `anchor_13` in the original fixture.

Verification logs are retained in ignored `build/c15_delivery/`: the
`c15_full_build.log`, `c15_full_tests.log`, `c15_sanitizer_*.log`, and
`c15_windows_*.log` files record the final suites; `linux_joints_build.log`,
`windows_joints_build.log`, and `linux_joints_run.log` record independent SDK
delivery. Project format 13 (accepting 12), runtime format 10, CLI/generation,
runtime physics, and developer-owned `main.c` are unchanged. C15 is complete;
C16's subsequent completion is recorded below. Goal 5 is now closed by user
acceptance with the recorded exceptions, including C7/C9.

**C16 complete — numeric property locks:**

Numeric locks are sparse, project-owned authoring metadata. The canonical
registry covers object/entity transforms and settings, soft-body children,
geometry, forces/torques, sprites/animation, camera/layout/UI controls, shared
fonts, layers, input bindings, audio, application window, and project physics.
Temporary tool inputs, editor preferences, and read-only derived displays are
excluded. Field and accordion padlocks use the same command/history path;
mixed groups lock all before unlocking all. Symbol-size sliders and bulk color
opacity share those controls. Locked field text remains selectable/copyable.

Locks protect local/stored values, independently of Static or runtime motion
mode. Parent movement can carry a child without changing its locked local
coordinates. Direct GUI/CLI edits retain locked members of compound settings;
connected joint placement is validated together, while independent selection
groups can still change. Pointer drags project onto permitted directions and
summarize changed/blocked items on release. Deletion remains permitted;
duplication and undo/redo retain lock metadata. UI vertex insertion/removal
remaps surviving child locks. Shared UI definitions/fonts retain shared identity.

Project format **14** writes `numeric_locks`; formats **12/13/14** load, with
missing metadata defaulting to no locks. Invalid targets/properties, malformed
entries, and duplicates reject the document. C11's migration still emits 12,
which subsequently saves as 14. Runtime format remains **10**. Constructors,
catalogs, spawning, runtime setters, and simulation do not enforce authoring
locks. Developer-owned generated-project `main.c` is unchanged.

Verification sources:

1. `editor_numeric_locks` covers projected single/connected movement, partial
   independent transforms, compound settings, local child coordinates,
   whole-body material edits retaining node overrides, UI range invariants,
   topology remapping, history, duplication, persistence, and accordion scope.
2. `editor_configuration_controls`, `ui_field_selection`, and `ui_reports`
   cover pointer-operated field/group/slider locks, mixed bulk opacity locks,
   history, selectable/copyable locked fields, and sliders refusing input and
   snapping. The reusable UI defaults remain editable.
3. `editor_cli_numeric_locks` executes the CLI, covering positional dispatch,
   selector ordering, rejection without file changes, unlock/edit, formats
   12/13/14, and malformed metadata. Additional installed-CLI checks cover
   exact uint64 lifetime values and independent RGB edits with locked opacity.
4. `editor_motion_generated` authors position/mass locks, then exercises
   constructors, catalogs, and object-file spawning against the installed SDK.
   Runtime mass and position setters remain unrestricted. Windows builds the
   same generated application, scene, and probe against its installed SDK.
5. User manual review of the five C16 checks: **PASS** (2026-10-27).
   Review project: `sandbox/milestone_1/c16/project/`; checklist beside it.
   Final added-control review: **PASS**, covering symbol-size and Configuration
   accordion locks, undo/redo, and mixed bulk Outline Color opacity locks.

Final delivery results (2026-10-27): all **132 Linux tests** and **114 ASan/UBSan
tests** pass, with leak detection and halt-on-error enabled. Windows
editor/CLI/tests and the installed-SDK generated application, scene, and probe
cross-build successfully. The UI example builds independently against both
installed SDKs and passes Linux startup/interrupt checks from outside its source
directory. Final builds have no compiler warnings or sanitizer findings. Native
Windows execution was not performed. No screenshot analysis was used.

Local evidence is organized under ignored `sandbox/milestone_1/c16/`:
`project/` and `checklist.md` hold manual review, `automated/` holds disposable
checks/generated consumers, and `logs/` holds verification output. Existing
configured build trees and installed SDKs remain under `build/`; the sandbox
links to them. Never regenerate over an active manual review project.

The `logs/final_linux_tests.log`, `logs/final_sanitizer_tests.log`,
`logs/final_*incremental.log`, `logs/generated_windows.log`,
`logs/final_cli_checks.log`, and `logs/*user_interface*.log` files retain the
results. `verification.txt` indexes the completed checks. Reproduce the shared
lock and CLI regressions with
`ctest --test-dir build --output-on-failure -R 'editor_numeric_locks|editor_cli_numeric_locks|editor_configuration_controls|ui_field_selection|ui_reports'`.
C16 is complete. Milestone 4 and goal 5 are now complete by user acceptance;
the existing C7/C9 exceptions and other unconfirmed goal 5 checks remain
unverified. No subsequent implementation goal was started.
