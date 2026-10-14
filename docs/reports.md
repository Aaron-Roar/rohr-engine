# Shared reports

The shared report API is implemented independently of engine initialization.
Physics updates return this type as described below. Console, JSON, and text-file
output are implemented; other system integrations and latest-report retrieval
remain scheduled work.

Include `rohr.h` for the public API, or `report.h` for the direct `report_*`
functions. Both paths use the same implementation and have the same behavior.

## Build a report

```c
Report report = rohr_report_create();
EngineResult result = rohr_report_header_set(&report, (ReportHeader){
    .id = ROHR_PHYSICS_JOINTS_REPORT,
    .status = ROHR_REPORT_COMPLETE,
    .total_ms = measured_ms
});
/* Check result before continuing; check subsequent helper results too. */

ReportDiagnostic diagnostic = {
    .report_id = ROHR_PHYSICS_JOINTS_REPORT,
    .severity = ROHR_REPORT_SEVERITY_WARNING,
    .code = ERROR_ENGINE_COMPONENT_MISSING,
    .recovery = ROHR_REPORT_CONTRIBUTION_SKIPPED,
    .source = {ROHR_REPORT_OBJECT_ENTITY, joint},
    .target = {ROHR_REPORT_OBJECT_ENTITY, target},
    .occurrence_count = 1
};
result = rohr_report_diagnostic_detail_set(&diagnostic, "target has no required component");
result = rohr_report_diagnostic_add(&report, diagnostic);
```

The example illustrates collection, not an implemented joint recovery policy.
The producer measures its own elapsed time and chooses the outcome of its work.
Reporting never simulates, queries handles, changes entity state, or reads engine
time. Header status describes completion; warnings do not turn it into failure.
Adding a fatal diagnostic or failed child makes failure sticky. Initialize a
new report to start another operation rather than clearing an existing failure.

`report_create()` returns the same empty value as zero initialization: no ID,
NOT_RUN, no stored records, and no fatal. Set a real ID with `header_set()` before
adding records. A completed empty operation still has its ID and COMPLETE status.
No explicit lifetime or destruction operation is required. Returned reports and
ordinary C copies own all their bytes, including diagnostic text.

All malformed arguments return `ERROR_ENGINE_REPORT_INVALID` and preserve the
destination. `report_valid_check()` checks populated fields, enum ranges, finite
nonnegative elapsed times, duplicate keys, and reserved-fatal consistency. Unused
array slots do not matter. Prefer helpers over editing counts or collected slots.
`NONE` severity denotes empty/default values; it is invalid as a collected issue.

## Add entries and statistics

An entry summarizes a directly runnable child: its ID, status, time, and number
of invocations. `report_entry_add()` combines equal IDs, sums time and invocation
count, and propagates the strongest status. It does not modify the parent's time.

`report_statistic_add()` combines `(report_id, metric_id)` keys. Engine metrics
have fixed rules:

| Metric | Rule |
| --- | --- |
| PROCESSED, SKIPPED, CANDIDATE, OVERLAP, CONTACT, QUARANTINED counts | SUM |
| SUBSTEPS_ATTEMPTED and SUBSTEPS_COMPLETED counts | SUM |
| TREE_HEIGHT_PEAK | MAX |
| TREE_NODES_CURRENT | LATEST |

The names above omit the common `ROHR_REPORT_` prefix for readability. Provide
the corresponding `ROHR_REPORT_AGGREGATION_*` rule when adding a statistic.
Wrong built-in rules and conflicting rules for an existing key are rejected.
LATEST means last collected/merged, not greatest timestamp. Statistics are
unsigned integer work measurements; elapsed milliseconds are doubles in headers.

Engine report IDs name the system/pipeline, such as `ROHR_PHYSICS_REPORT` and
`ROHR_PHYSICS_JOINTS_REPORT`, without `STAGE` in their names. The inclusive custom
report and metric ranges run from `0x10000` through `0x7fffffff`, exposed through
`ROHR_REPORT_CUSTOM_ID_BEGIN/END` and
`ROHR_REPORT_CUSTOM_METRIC_ID_BEGIN/END`. Applications assign their own unique IDs
within these ranges and explicitly choose aggregation rules for custom metrics.
Unknown engine-range IDs are rejected. There is no registration or mutable global
name table; output helpers display numeric custom IDs.

## Merge completed work

```c
Report combined = rohr_report_create();
EngineResult result = rohr_report_header_set(&combined, (ReportHeader){
    .id = ROHR_PHYSICS_REPORT,
    .status = ROHR_REPORT_COMPLETE,
    .total_ms = parent_elapsed_ms
});
result = rohr_report_merge(&combined, &child_report);
```

Merging adds the child header as one invocation when the child ran, then its
already-aggregated entries, statistics, and diagnostics. A source's NOT_RUN header
does not invent an invocation. Merge each child once: do not separately add its
header, re-merge its descendants, or count an inclusive parent timing as more
child work. Re-merging deliberately counts another observation; this API is not
an event deduplication service. Self-merge is rejected.

The source is immutable; destination identity and elapsed time are preserved.
The caller measures parent wall-clock duration, which cannot be inferred from
possibly nested child timings. A failed child makes the destination failed and
later successful children cannot clear it. First fatal and LATEST statistics
follow merge order. Merge validates both reports and all existing metric-rule
conflicts before mutation, including conflicts found after otherwise valid entries.

Reports have fixed capacities, defined in `report.h`:

- `ROHR_REPORT_ENTRY_CAPACITY`: 32.
- `ROHR_REPORT_STATISTIC_CAPACITY`: 64.
- `ROHR_REPORT_DIAGNOSTIC_CAPACITY`: 256.
- `ROHR_REPORT_DETAIL_CAPACITY`: 256 bytes including the terminating NUL.

Overflow is successful bounded collection, not simulation failure. Missing new
entries/statistics increment their loss counts; missing diagnostics increment
lost occurrence count. These are losses at collection/merge time, not an estimate
of unique unseen keys. Existing stored keys can still accumulate when full.
Source loss counts carry forward through merging. Counters saturate at UINT64_MAX;
elapsed-time accumulation saturates at DBL_MAX. Both set `counters_saturated`.

## Diagnostics and first fatal

Diagnostics aggregate on report ID, pipeline-local `internal_stage`, severity,
error code, recovery action, source, and target. Zero internal stage means no
additional stage context. Occurrence counts add; known substep ranges widen.
For unknown substep context, use `has_substep = false` and zero indices. Source
and target are copied tagged 64-bit IDs, preserving entity generations without
owning or resolving the object. NONE object references use ID zero.

`report_diagnostic_detail_set()` copies text; NULL clears it. Overlong input is
NUL-terminated and sets `detail_truncated`. It may copy from the diagnostic's own
buffer. Repeated diagnostics preserve the first message and its truncation flag.
The helper only edits text; a diagnostic must have valid context/severity/count
before adding it to a report.

The first fatal is also stored in `first_fatal` with `has_fatal = true`. If the
ordinary list is full, this dedicated slot still records the failure and its
original message. That retained fatal is not counted as dropped. Matching repeats
accumulate in the reserved record; later distinct critical messages cannot replace
it. If both list and reserved slot contain it, they describe the same occurrences,
not two failures. Merge handles a mirrored or reserved-only fatal exactly once.
Later distinct fatal records can overflow the ordinary list, but never conceal
the first reason the operation failed.

## Cost and boundaries

On the verified native build, `sizeof(Report)` is 88,728 bytes. Layout is a C ABI,
not a portable serialized byte format. Keep retained reports in deliberately
budgeted storage; avoid large arrays on small stacks. Validation uses bounded
linear/quadratic scans over populated slots and collection uses no heap allocation.
Collection helpers are independent of SDL, console output, engine state, and
resource lifetimes; callers synchronize concurrent writes to the same report
themselves.

Milestone requirements and remaining integrations/recovery are recorded in the
[reporting contract](physics_reporting_contract.md) and [roadmap](../NEXT_STEPS.md).

## Physics reports

Physics updates now return `Report` through both direct and `rohr_` APIs. This
breaks the former `EngineResult` return type and removes `PhysicsUpdateReport`
and the old physics-specific latest-report getters. Check the returned status:

```c
Report report = rohr_physics_update(ticks);
if(report.header.status == ROHR_REPORT_FAILED) {
    /* report.first_fatal contains the reason and any source/target evidence. */
}
```

`rohr_error_check()` continues to accept ordinary result objects; it does not
accept a report. Setters and creators retain their existing result types.
Existing developer-owned project entry points need this explicit call-site
migration. Newly generated entry points and bundled examples use the new return.

The report tree is:

```text
ROHR_PHYSICS_REPORT                   full update, including preparation when applicable
└── ROHR_PHYSICS_SUBSTEP_REPORT        each executed substep, aggregated by ID
    ├── ROHR_PHYSICS_LOADS_REPORT      spring and beam loads
    ├── ROHR_PHYSICS_INTEGRATION_REPORT
    ├── ROHR_PHYSICS_CONTACTS_REPORT   broadphase and contact gathering
    ├── ROHR_PHYSICS_JOINTS_REPORT     joint gathering
    └── ROHR_PHYSICS_CONSTRAINTS_REPORT  iterative solve and interaction publication
```

| Runnable API (prefix `rohr_` optional) | Prerequisites and behavior |
| --- | --- |
| `physics_update(ticks)` | Converts ticks to one total duration; zero ticks is NOT_RUN. |
| `physics_dt_update(dt)` | Finite nonpositive duration is NOT_RUN. |
| `system_physics_update(dt)` | Updates animated hitboxes, then runs all substeps. |
| `physics_pipeline_update(dt)` | Advances interactions and runs all substeps; caller handles animated-hitbox preparation. |
| `physics_pipeline_substep(dt)` | Clears constraints/acceleration, applies gravity and loads, integrates, gathers, solves. Call `step_begin()` first when beginning a new interaction update. |
| `physics_pipeline_forces_apply()` | Applies spring/beam loads to prepared acceleration buffers. Persistent force sources remain in integration, as before. |
| `physics_pipeline_integrate(dt)` | Integrates current state and prepared loads; preserves existing world-range quarantine. |
| `physics_pipeline_contacts_gather()` | Refreshes hitboxes/tree and appends contacts. Call `substep_begin()` first to clear old constraints. |
| `physics_pipeline_joints_gather()` | Replaces gathered joint constraints; can run without contacts or integration. |
| `physics_pipeline_constraints_solve(iterations)` | Requires successful gathering. Zero iterations is NOT_RUN; contact and joint iterations retain their existing ordering. |

Initialize the engine before simulation. Preparation helpers `step_begin`,
`substep_begin`, `accelerations_clear`, and `gravity_apply` remain report-free;
using them does not create child report entries. Internal solver iterations and
per-entity helpers contribute to their owning report. All nonfinite durations
fail before simulation. Direct system/pipeline/substep/integration zero-duration
calls still execute their existing zero-time work, including collision response;
negative durations fail at these lower-level entry points.

For a joints-only pass, call `substep_begin()`, gather joints, check that report,
then solve and check that report. Merge both returned reports if desired. Do not
solve partially gathered constraints after a fatal gather. Call `step_begin()`
when this pass should also start a new interaction-history boundary.

The full update records attempted/completed substep counts. Failed integration
retains each quarantined entity's full handle, copied message, and zero-based
substep index. Its existing HOLD, cleared motion, and removed-hitbox behavior is
unchanged. Subsequent stages/substeps stop on fatal failure; prior work is not
rolled back. Broken-joint cleanup and failed animated-hitbox selection produce
warnings while independent work continues. Preparation issues have no substep.
Allocation failures in tree construction, rigid/beam contact gathering, joint
gathering, legacy joint force creation, and interaction publication are fatal.

Metrics retain the previous meanings, with explicit owner and aggregation:

- Integration PROCESSED and QUARANTINED counts count entity visits per substep.
- Contacts COLLIDER, CANDIDATE, NARROWPHASE_TEST, and OVERLAP counts describe the
  rigid broadphase path, including node colliders; beam-query pairs are not added
  to these legacy counters. Work counts sum over substeps.
- Contacts TREE_NODES_CURRENT takes the latest tree size; TREE_HEIGHT_PEAK keeps
  the maximum height observed across substeps.
- Contacts BROADPHASE_BUILD_NS, BROADPHASE_QUERY_NS, and NARROWPHASE_NS are summed
  nanoseconds. Query time excludes separately measured build and narrowphase
  time and includes hitbox refresh and soft boundary gathering. Divide by
  1,000,000 to compare with the former millisecond fields.
- Constraints CONTACT_COUNT counts rigid response pairs once per substep, not
  once per solver iteration. Its header elapsed time replaces `response_ms`.
- Joints PROCESSED_COUNT counts gathered pin/weld joints; SKIPPED_COUNT records
  skipped contributions where a warning is emitted.

Parent/header milliseconds are inclusive wall-clock measurements. Do not sum
parent and child elapsed times. Missing statistics represent zero observed work.
Report collection receives explicit invocation-local pointers; there is no
mutable latest-report collector and no report pointer retained in constraints.
The engine's simulation state remains subject to its existing single-threaded
physics execution requirement.

The later stored-report goal will add `rohr_report_get(id)` and the combined
`rohr_report_get(ROHR_REPORT)`. Each runnable pipeline will retain only its latest
copy, overwritten by its next update. Updates will still return reports directly.
That retrieval API is agreed but is not implemented in this physics goal.

## Console and file output

Output accepts an existing report; it neither runs an update nor fetches stored
reports. All three functions work before engine startup and preserve the source.
The direct APIs have the same signatures without the `rohr_` prefix:

```c
EngineResult rohr_report_console_write(const Report *report);
EngineResult rohr_report_json_write(const Report *report, const char *path, bool pretty);
EngineResult rohr_report_text_write(const Report *report, const char *path);
```

Check each returned `EngineResult`: it describes output success or failure,
independently of the report's simulation status. A failed physics report can be
written successfully. Invalid reports and null/empty paths return
`ERROR_ENGINE_REPORT_INVALID`; I/O, formatting, and console truncation return
`ERROR_ENGINE_OUTPUT_IO_FAILED`, `ERROR_ENGINE_OUTPUT_FORMAT_FAILED`, and
`ERROR_ENGINE_OUTPUT_TRUNCATED`, respectively. Allocation failure returns
`ERROR_MEMORY_POOL_ALLOCATION_FAILED`. An already-truncated diagnostic detail
is visibly marked but does not itself make output fail.

Files contain one report, replacing existing contents. Relative paths use the
current working directory; parent directories are not created. An I/O failure
can leave a partial file. JSON is completely serialized before opening its
file, so invalid UTF-8 or serialization allocation failures preserve the prior
file. JSON's temporary document and output buffer are released on every path.
The general file API remains deferred; these functions own their file handles.

Console and text files share a formatter. They include the header, child summaries,
statistics with aggregation rules and units in metric names, the first fatal,
additional diagnostics, and all overflow/saturation fields. Enum names are
shortened for readability; custom IDs retain numeric fallback labels. Optional
object/substep context is shown only when present. Diagnostic control characters
are escaped (`\n`, `\r`, `\t`, `\xNN`) and literal backslashes are doubled.
The first fatal is printed once, even when mirrored in the ordinary list.

Each console line goes through the existing engine console backend with its
`[#]` prefix. Files omit those prefixes. `MAX_LOG_STR` is 1,024 bytes including
prefix and NUL; the console history still keeps 100 entries. Every report line
is sent to the terminal even when earlier lines age out of history. Lines that
exceed capacity end with `... [truncated]`, and subsequent report lines are still
written before returning the truncation error. An I/O or formatting failure
stops further output. Text files do not inherit the console's line limit.
Existing void console APIs keep their signatures and use the same checked
backend internally, discarding its return value.

### JSON format version 1

The root object contains `format_version`, `header`, `entries`, `statistics`,
`diagnostics`, `first_fatal`, `dropped_entry_count`, `dropped_statistic_count`,
`dropped_diagnostic_occurrences`, and `counters_saturated`. Arrays contain only
populated records in collection order, so redundant counts/capacities are omitted.
A child entry remains a summary; export does not invent nested child reports.

- Enums use `{ "value": number, "name": "EXACT_C_ENUM_NAME" }`. Unknown custom
  names are `null`. Numeric values already exported by version 1 must not be
  renumbered when extending enums.
- Every `uint64_t` field uses a decimal string, including statistic values,
  invocation/occurrence/drop counts, and object IDs. This preserves values larger
  than JavaScript's exact-integer range. Enum/stage/substep IDs are numbers.
- Header `total_ms` is a number. Boolean fields remain booleans. No engine tick,
  timestamp, object name, or other live state is inferred at export time.
- Diagnostic fields match `ReportDiagnostic`, with enum objects for
  `report_id`, `internal_stage`, `severity`, `code`, and `recovery`. Stage names
  are resolved only within their owning system; unknown or zero stages have a
  null name. `message` adds the error code's static human-readable explanation;
  `detail` is the report's copied text, never mutable last-error text.
- `source` and `target` contain `kind` (enum object) and `id` (decimal string),
  including NONE/zero references. `has_substep`, `first_substep`, `last_substep`,
  and `detail_truncated` are retained explicitly.
- `first_fatal` is `null` when absent; otherwise it is a complete diagnostic.
  Its ordinary-list mirror is retained in JSON. Both refer to the same failure
  and must not be added together when counting occurrences.
- Pretty and compact output differ only in whitespace. Invalid UTF-8 is rejected
  rather than replaced. This is diagnostic export, not a report-loading or
  project-persistence schema.
