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
CTest names. Milestone 18's naming changes and example/API migration remain
separate, unstarted goals.
