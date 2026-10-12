# Shared reports

The shared report API is implemented independently of engine initialization.
Physics and other system updates do not yet return this type: their integration
is scheduled in later System Update Reports goals. Console printing and JSON
export are likewise not implemented by the shared-library goal.

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
name table; later output helpers can display numeric custom IDs.

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
All helpers are independent of SDL, console output, engine state, and resource
lifetimes; callers synchronize concurrent writes to the same report themselves.

Future physics integration, output, and recovery requirements remain in the
[reporting contract](physics_reporting_contract.md) and [roadmap](../NEXT_STEPS.md).
