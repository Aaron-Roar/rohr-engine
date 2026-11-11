# Entity Ids {#entity_ids}

`Entity` values are stable ids, not component table indexes.

Systems that need direct table access resolve an entity id through
`entity_index_get()`. This keeps ids decoupled from storage location and avoids
using raw array indexes as public handles.

`EntityIndex` values still correspond to component table indexes. That property
is important for compute-oriented loops and direct table access inside engine
systems.

When an entity is deleted, its id slot and table index can be reused. The
generation bits in the entity id prevent stale ids from resolving as live
entities after deletion and reuse.

## Caller-owned entity lists

`EntityList` contains an `Entity *data`, `size_t capacity`, and `size_t count`.
`EntityNameList` has the same fields with an `EntityName *data`. Allocate storage
in game code using `rohr_entity_alive_count_get()` as the required element count,
then call `rohr_entity_list_get(&list)` or `rohr_entity_name_list_get(&list)`.
The direct APIs are `entity_list_get` and `entity_name_list_get`; both return
`EngineResult` with identical behavior to the public wrappers.

The getters copy every live entity in discovery order. They do not allocate or
sort, and do not promise a stable order across creation/deletion. Each unnamed
entity contributes an empty `EntityName`, so ID and name entries correspond if
no entities are created or deleted between calls. Serialize these calls between
updates and entity mutations, including renames; they are not concurrent reads.

For every non-NULL list, `count` is overwritten with the required live count,
even on failure. `data` and `capacity` are never changed. The error contract is:

| Request | Result |
|---|---|
| NULL list | `ERROR_ENGINE_ENTITY_LIST_INVALID` |
| NULL data with nonzero capacity | `ERROR_ENGINE_ENTITY_LIST_INVALID` |
| Capacity less than live count | `ERROR_ENGINE_ENTITY_LIST_CAPACITY_EXCEEDED` |
| NULL data and zero capacity, no live entities | Success, count zero |
| Sufficient valid storage | Success, exactly count elements copied |

Validation precedes copying: errors leave every array element untouched. A NULL
data pointer with zero capacity is also a valid capacity query; with live
entities it returns the capacity error and required count. Success leaves any
unused array tail untouched. If creation has increased the live count since
sizing storage, resize it explicitly and retry between mutations.

The caller owns and releases only its arrays. Snapshots neither own nor retain
entities. Deleted IDs become stale, and copied names remain unchanged after
renaming or deletion. Validate a captured ID with `rohr_entity_alive_check`
before using it after mutations. Names are copied values, not durable identity.
These lightweight lists are separate from the allocating `EntityInfo` inspection
snapshots and do not require `rohr_entity_info_release`.

## Runtime ID and name access

Authored object/project structures are creation recipes. Created resources have
typed IDs and runtime names in independent namespaces:

| Resource | Global lookup | Copied runtime name |
|---|---|---|
| Entity | `rohr_entity_by_name_get` | `rohr_entity_name_get` (`EntityName`) |
| Viewport | `rohr_viewport_by_name_get` | `rohr_viewport_name_get` (`ViewportName`) |
| Controller | `rohr_input_controller_by_name_get` | `rohr_input_controller_name_get` (`InputControllerName`) |
| Sound player | `rohr_audio_sound_by_name_get` | `rohr_audio_sound_name_get` (`SoundName`) |
| Music stream | `rohr_audio_music_by_name_get` | `rohr_audio_music_name_get` (`MusicName`) |

Each lookup/getter returns its typed result or an error. Direct equivalents omit
`rohr_`; viewport direct equivalents use `graphics_viewport_`. Names are copied
values; they retain no resource. Lookups only find live IDs, and name getters
reject stale IDs. A name may be reused after deletion: retain the ID when access
must continue referring to the same resource.

Creation tries the authored/requested name first, then appends `_1`, `_2`, and
so on, choosing the lowest available suffix. Existing suffixes are part of the
base: another `box_1` becomes `box_1_1`. Deletion frees a name for reuse. Entity
names no longer include project or object prefixes. The same rule applies to
native descriptors, standalone authored creation, and whole-project creation.
Entity kind defaults (`entity`, `particle`, `node`, etc.), `viewport`, `sound`,
and `music` are used when their descriptors omit names; controllers still require
a nonempty name. All five families currently allow 63 bytes plus a terminator.
Overlong/unterminated names, or a required suffix that cannot fit, fail without
truncation or partial creation. Entities/controllers use their existing
name-length errors; viewports use `ERROR_ENGINE_STATE_INVALID`, and audio uses
the corresponding sound/music configuration error.

`ViewportDescription`, `SoundDescription`, and `MusicDescription` now contain a
copied `name` field. Generated catalogs carry the authored names for standalone
creation as well. Rebuild consumers and regenerate catalogs with the updated SDK;
the descriptor layouts have changed. Existing developer-owned mains remain
untouched. Controller/entity metadata setters still require an exact unique
name; they do not choose suffixes. Viewport setters preserve names when empty
and reject occupied replacements. Audio runtime names are immutable: full audio
setters accept an empty name or the current name, preserving existing queued
playback/source-update semantics. Audio names survive source replacement.

Actions stay scoped to controller IDs, and viewport items to viewport IDs.
Screens are accessed through placements, cameras through their entities, and
loaded assets through their existing typed handles. Named layers continue to
share names and values; they do not receive instance suffixes.

Authored connections resolve through creation-local identity mappings and bound
instance membership, independently of runtime names or temporary legacy member
aliases. Explicit bindings remain required for dependencies outside a creation
operation. Multiple copies may share an owner; legacy authored-name lookup
reports ambiguity when more than one member matches. Hierarchy access APIs
remain temporarily available until Milestone 18 Goal 3 migrates their callers.

Projects retain their resource IDs solely for ownership and grouped cleanup.
Individual deletion is allowed: later project cleanup skips stale IDs and
preserves replacement resources occupying the old slots. Failed creation rolls
back only resources from that attempt. Entity, controller, viewport, sound and
music slots are retired when their ID generation is exhausted instead of
reviving an old ID. Entity IDs remain scoped to the engine lifetime; do not
retain them across engine shutdown/restart.

## Milestone 18 Goal 1 verification

`tests/entity_lists.c` exercises both direct and public APIs, including mixed
ID/name call pairs. It covers empty/exact/insufficient and excess capacity,
NULL list/storage validation, required counts on failure, unchanged arrays on
error, untouched tails, unnamed entities, discovery order, copied-name
independence, and deletion followed by slot reuse. Linux allocation wrappers
reject any allocation during the getters. New errors also have readable
messages and are accepted in report diagnostics.

The six focused tests (`entity_lists`, `entity_info`, `entity_info_adversarial`,
`engine_lifecycle`, `report_output`, and `report`) pass in Release and ASan/UBSan
with leak detection. The same list test passes as an independent installed-SDK
consumer in both Linux configurations. All six tests and the consumer
cross-build for Windows; native Windows execution was not performed.

All 13 examples compile independently against the installed Release Linux SDK.
Center-of-mass, player-controller, and viewport examples pass three-second
headless startup and clean interrupt checks from `/tmp`. These checks do not
claim real visual acceptance; rendering is unchanged. No compiler warnings or
sanitizer findings remain. The final example checks passed after removing this
goal's disposable sanitizer/Windows SDK copies to recover disk space.

Local build/test logs and the verification driver are retained under
`sandbox/milestone_18/goal_1/`. Reproduce focused verification with CMake targets
`entity_lists_test entity_info_test entity_info_adversarial_test
engine_lifecycle_test report_output_test report_test`, then run the corresponding
CTest names. Naming and example/API migration are separate goals; the following
section records Goal 2's subsequent verification.

## Milestone 18 Goal 2 verification

`tests/runtime_names.c` covers direct/facade lookups and copied getters across
all five namespaces, occupied and already-suffixed names, lowest-suffix reuse,
stale IDs, maximum-length and unterminated names, failed audio-load retry, and
audio name preservation. Repeated entity creation/deletion crosses the 16-bit
generation boundary and verifies that the original ID never becomes live again.

`tests/project_creation.c` checks repeated projects, instance-local camera
wiring despite changed runtime names and legacy aliases, and grouped cleanup
after individually deleted resources have had their slots reused. Existing
allocation-failure, rollback, capacity, lifecycle and persistence tests remain
active. Object tests cover repeated members under one owner and legacy lookup
ambiguity without changing the authored identity used for connections.

All 12 focused tests pass in Linux Release and ASan/UBSan with leak detection:
`runtime_names`, `descriptor_creation`, `controller_creation`, `project_creation`,
`input_actions`, `object_instances`, `viewport_descriptions`, `audio`,
`audio_reports`, `entity_lists`, `engine_lifecycle`, and `runtime_persistence`.
The same 12 targets cross-build for Windows. Native Windows execution remains
unverified. No compiler warnings or sanitizer findings remain in these checks.

The installed Release SDK passes independent naming and persistence consumers.
Regenerated configuration-parity and project consumers compile and execute,
including authored viewport/audio names and repeated project instances. All 13
standalone examples compile against that SDK; center-of-mass, player-controller
and viewport examples pass headless startup and clean interrupt checks from
outside their source directories. No real visual acceptance is claimed.

The 14 new direct/facade query declarations are recorded in the API inventory.
Its audit still fails on pre-existing missing entries and duplicate facade
declarations: current diagnostics exactly match an audit against committed
headers and inventory. Logs for both audits, builds, tests and the verification
drivers are retained under `sandbox/milestone_18/goal_2/`. Disposable SDK and
consumer builds were removed between phases because disk space is limited.
Goal 3 retains the complete example/main migration, generic Linux SDK workflows,
hierarchy API removal, and real Linux preview acceptance.
