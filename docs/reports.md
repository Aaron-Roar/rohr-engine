# Shared reports

The shared report API is implemented independently of engine initialization.
Engine, physics, graphics, audio, and UI updates return this type as described below.
Console, JSON, and text-file output are implemented. Input reporting is explicitly
deferred; latest-report retrieval and simulation-failure recovery remain scheduled work.

For implementation work, follow the
[reporting instructions](agent_instructions/system_reporting.md). The
[audio Goal 5 contract](audio_reporting_contract.md) records deferred-control
and callback-reporting semantics.

Include `rohr.h` for the public API, or `report.h` for the direct `report_*`
functions. Both paths use the same implementation and have the same behavior.

## System coverage inventory

This is the current Goal 9 inventory. Names omit the optional public `rohr_`
prefix; every listed public operation has an equivalent direct API. Report IDs
omit `ROHR_`. Entries aggregate by ID, rather than recording execution order.

| Entry points | Report owner and children | Execution and collection boundary |
| --- | --- | --- |
| `engine_update()` | ENGINE_REPORT → ENGINE_LIFECYCLE_REPORT | Requires engine startup. Advances ticks then cleans lifetimes even while paused or with zero ticks. COMPLETE for empty work; FAILED before startup. Parent measures both operations. |
| `engine_lifecycle_update()` | ENGINE_LIFECYCLE_REPORT | Independent sweep using current time/ticks. Individual resolution/deletion failures warn; startup failure is fatal. |
| `physics_update(ticks)`, `physics_dt_update(dt)`, `system_physics_update(dt)`, `physics_pipeline_update(dt)` | PHYSICS_REPORT → PHYSICS_SUBSTEP_REPORT and descendants | Requires engine startup. First three include animated-hitbox preparation when executing; raw pipeline does not. Zero ticks and finite nonpositive high-level duration are NOT_RUN. Lower-level zero duration executes; negative/nonfinite lower-level duration fails. |
| `physics_pipeline_substep(dt)` | PHYSICS_SUBSTEP_REPORT → LOADS, INTEGRATION, CONTACTS, JOINTS, CONSTRAINTS reports | Requires prepared interaction state. Runs stages in order, stopping dependent work after fatal failure. Child invocations aggregate across substeps. |
| `physics_pipeline_forces_apply()`, `physics_pipeline_integrate(dt)` | PHYSICS_LOADS_REPORT, PHYSICS_INTEGRATION_REPORT | Requires prepared acceleration/state. Independently runnable spring/beam loads and integration. |
| `physics_pipeline_contacts_gather()`, `physics_pipeline_joints_gather()`, `physics_pipeline_constraints_solve(iterations)` | PHYSICS_CONTACTS_REPORT, PHYSICS_JOINTS_REPORT, PHYSICS_CONSTRAINTS_REPORT | Clear constraint buffers before gathering; solve after successful gathering. Zero iterations are NOT_RUN. |
| `graphics_show()` | GRAPHICS_REPORT → CAMERAS, COMMANDS, VIEWPORTS, RECORDING, PRESENTATION reports | Requires graphics startup. Inclusive frame timing. Stops dependent work after fatal failure; recording failure still permits presentation. |
| `graphics_pipeline_cameras_update()`, `graphics_pipeline_commands_execute()`, `graphics_pipeline_viewports_draw()` | GRAPHICS_CAMERAS_REPORT, GRAPHICS_COMMANDS_REPORT, GRAPHICS_VIEWPORTS_REPORT | Independent stages. Commands consume pending submission evidence once. Callback draws belong to commands. Missing renderer/recursive frame calls fail. Empty valid work is COMPLETE. |
| `graphics_pipeline_recording_update()`, `graphics_pipeline_present()` | GRAPHICS_RECORDING_REPORT, GRAPHICS_PRESENTATION_REPORT | Disabled recording is NOT_RUN after graphics initialization. Presentation includes frame-limit waiting. |
| `graphics_sprite_frames_update(tick, time)` | GRAPHICS_ANIMATIONS_REPORT | Independent entity animation sweep after engine startup. Not implicitly run or merged by frame updates, even if invoked inside a camera callback. |
| `audio_update()`, `audio_stop()` | AUDIO_REPORT → COMMANDS, SOUNDS, MUSIC, OUTPUT reports | Callback-owned children; no public manual execution. Update submits requests and collects completed evidence, possibly from older requests. Active empty updates are COMPLETE; inactive updates without shutdown evidence are NOT_RUN. |
| `ui_update()` | UI_REPORT; no children | Automatic first use. Collects preceding events/widgets, finalizes the frame and consumes evidence once. Empty frames are COMPLETE. Timing covers update only. Graphics draw failures stay in graphics reports. |

Detailed prerequisites, stage timing, and failure policies appear in the
system sections below. Every producer returns an owned value without requiring
another system to run or a global error queue to be read.

Report-free boundaries are deliberate:

- Creation, loading, setters, direct force/impulse application, validation, and
  startup keep immediate results. Audio control acceptance is immediate;
  later callback execution has separate reporting.
- `engine_time_update()` and `engine_tick_update()` advance the clock without
  cleanup reports; getters only read state.
- Physics `step_begin`, `substep_begin`, `accelerations_clear`, `gravity_apply`,
  and `physics_hitbox_animation_bindings_update()` are preparation helpers.
  Binding failures contribute when invoked by the system update; its standalone
  void API does not return a report.
- Per-value animation helpers update caller-owned player/sprite values; the
  entity sweep owns animation reports. Draw calls and immediate UI widgets
  contribute to their respective next updates.
- Input events, queries, and text/IME truncation keep existing behavior.
  `ROHR_INPUT_REPORT` is reserved and printable, without an input producer.
- Report builders, validation, composition, and output operate on values;
  they do not run systems. Goal 10's retrieval API is not implemented.

### Coverage evidence

| Tests | Contract evidence |
| --- | --- |
| `report`, `report_output` | Validation, aggregation, all built-in output names/rules, overflow, saturation, repeats, reserved first fatal, copied details, output failures. |
| `physics_reports` | Full/standalone composition, substeps, warning/fatal context, allocation failures, recovery preservation and wrapper parity. |
| `graphics_reports`, asset/lifetime tests | Ordering, submission/execution ownership, skipped dependent work, backend failures, recording/presentation, queued-resource lifetime. |
| `audio_reports` | Deterministic delayed callbacks, failures, overflow, concurrent callers, callback continuation, shutdown delivery, timing including gate acquisition. |
| `ui_reports`, UI interaction tests | First use, events, transient-input reset, scopes/capacities, graphics ownership, persistent controls. |
| `engine_reports`, `timing`, `engine_lifecycle` | Combined/split parity, pause/zero/reset, cleanup failures, copies after slot reuse/shutdown. |
| `system_reports`, installed-SDK `rohr_system_reports` | Cross-system pending-evidence isolation, copies through restart, public/direct semantics, manual composition, checked warning/fatal consumption, audio shutdown isolation. |

The [design contract](physics_reporting_contract.md) preserves historical
pre-integration inventories; they do not identify additional missing pipelines.
Tests compare semantic results without exact real-clock timings. Linux failure
injection exercises backend failures; Windows compilation alone does not establish
Windows runtime behavior.

## Inspect and combine returned reports

Check status separately from diagnostics: COMPLETE can contain warnings, FAILED
stops dependent work, and NOT_RUN describes non-execution. Check `has_fatal`
before reading `first_fatal`; developer-built FAILED reports can lack a fatal
record, so do not infer status from that flag alone.

```c
static EngineResult inspect(const Report *r, bool *failed) {
    *failed = r->header.status == ROHR_REPORT_FAILED;
    if(*failed || r->diagnostic_count || r->dropped_entry_count ||
            r->dropped_statistic_count || r->dropped_diagnostic_occurrences ||
            r->counters_saturated)
        return rohr_report_text_write(r, "system_report.txt");
    return ERROR_RESULT_MAKE_VALUE(EngineResult, true);
}
```

Check the returned `EngineResult` for output failure independently of `failed`,
which describes system execution. The file is replaced per write, without history.
Output includes severity, source/target IDs, repeats, copied detail,
`detail_truncated`, overflow counts, and the reserved fatal even when the list
is full. A fatal's ordinary-list mirror describes the same occurrences, not a
second failure. Retained identifiers do not keep objects alive.

Unmatched UI scopes or invalid queued textures produce COMPLETE with warnings;
`engine_update()` before startup produces FAILED with retained startup details.
`tests/system_reports.c` compiles and runs this helper for both outcomes in the
source build and installed SDK.

Combine already returned reports with an application-owned ID. Measure parent
elapsed time independently; zero below means unmeasured.

```c
Report combined = rohr_report_create();
EngineResult result = rohr_report_header_set(&combined,
    (ReportHeader){ROHR_REPORT_CUSTOM_ID_BEGIN, ROHR_REPORT_COMPLETE, 0});
if(rohr_error_check(result)) { /* Handle invalid builder input. */ }
else {
    result = rohr_report_merge(&combined, &engine_report);
    if(!rohr_error_check(result))
        result = rohr_report_merge(&combined, &physics_report);
    if(rohr_error_check(result)) { /* Handle composition failure. */ }
}
```

The variables are reports from work already executed; merging does not invoke
systems or retrieve fresh evidence. Merge each parent once, without separately
merging included descendants. Audio stop and its next-update delivery describe
the same work; do not merge both. Inclusive/asynchronous child times do not sum
to frame duration. Budget retained values explicitly; see [Cost and boundaries](#cost-and-boundaries).

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
| PROCESSED_COUNT, SKIPPED_COUNT, CANDIDATE_COUNT, OVERLAP_COUNT, CONTACT_COUNT, QUARANTINED_COUNT | SUM; work items with owner-specific meaning |
| SUBSTEPS_ATTEMPTED_COUNT, SUBSTEPS_COMPLETED_COUNT | SUM; substeps |
| COLLIDER_COUNT, NARROWPHASE_TEST_COUNT | SUM; collider observations and narrow-phase tests |
| BROADPHASE_BUILD_NS, BROADPHASE_QUERY_NS, NARROWPHASE_NS, FRAME_LIMIT_WAIT_NS | SUM; nanoseconds |
| TREE_HEIGHT_PEAK | MAX; tree height |
| TREE_NODES_CURRENT | LATEST; nodes |
| QUEUED_COMMAND_COUNT, CANCELLED_COMMAND_COUNT | SUM; commands |
| AUDIO_FRAMES_COUNT | SUM; stereo sample frames |
| UI_INTERACTION_EVALUATION_COUNT, UI_EVENT_RECEIVED_COUNT, UI_EVENT_DROPPED_COUNT | SUM; evaluations/events, not unique widgets |
| UI_NAVIGATION_ENTRIES_PEAK, UI_KEY_EVENT_SLOTS_PEAK, UI_TEXT_EVENT_SLOTS_PEAK, UI_SCROLL_RECORDS_PEAK | MAX; occupied entries/slots/records |
| UI_TEXT_BYTES_PEAK | MAX; bytes excluding NUL |
| UI_SCROLL_SCOPE_DEPTH_PEAK, UI_TRANSLATION_SCOPE_DEPTH_PEAK | MAX; simultaneous scopes |
| TICKS_ADVANCED_COUNT, EXPIRED_ENTITY_COUNT | SUM; ticks consumed and expired entities encountered |

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

## Graphics reports

`rohr_graphics_show()` now returns an owned `Report` with ID
`ROHR_GRAPHICS_REPORT`. It runs the following directly callable pipelines in
order; each also has a direct API without `rohr_`:

| API returning `Report` | Report ID | Work measured |
| --- | --- | --- |
| `rohr_graphics_pipeline_cameras_update()` | `ROHR_GRAPHICS_CAMERAS_REPORT` | Camera motion/zoom and callbacks for enabled visible screens. |
| `rohr_graphics_pipeline_commands_execute()` | `ROHR_GRAPHICS_COMMANDS_REPORT` | Layer ordering and execution of queued clears, shapes, lines, text, and textures. |
| `rohr_graphics_pipeline_viewports_draw()` | `ROHR_GRAPHICS_VIEWPORTS_REPORT` | Composition of screens and viewport UI, including transforms and clipping. |
| `rohr_graphics_pipeline_recording_update()` | `ROHR_GRAPHICS_RECORDING_REPORT` | Capture, conversion, and writing/flushing one frame to the recording process. |
| `rohr_graphics_pipeline_present()` | `ROHR_GRAPHICS_PRESENTATION_REPORT` | Frame limiting and window presentation. |

Animation advances only when `rohr_graphics_sprite_frames_update(tick, time)` is
called. It now returns `ROHR_GRAPHICS_ANIMATIONS_REPORT`, independently of the
frame reports. Existing camera callbacks may continue to call this animation
update. `graphics_show()` neither advances animations itself nor merges an
animation report returned earlier or inside a callback.

These return-type changes require recompiling applications against the new SDK.
Ordinary calls that ignore the result remain valid C; function-pointer types
must change from `void` to `Report`. Check `header.status`, not
`rohr_error_check(report)`. Draw primitives, asset loading, and setters retain
their existing immediate return types.

```c
Report frame = rohr_graphics_show();
if(frame.header.status == ROHR_REPORT_FAILED || frame.diagnostic_count != 0) {
    EngineResult output = rohr_report_console_write(&frame);
    /* Check output separately; decide whether to continue the application. */
}
```

### Boundaries, prerequisites, and ownership

Initialize graphics before running frame pipelines. A missing renderer produces
a failed report. Successfully invoked empty pipelines are COMPLETE; recording
is NOT_RUN when disabled. Disabled recording does not create a child entry in
the combined report. Hidden items, paused callbacks, empty text, and collapsed
zero-scale drawings are intentional omissions, not warnings.

Draw calls queue commands before or during camera callbacks. Submission warnings
and accepted-command counts accumulate in bounded, graphics-owned storage until
the next commands execution call. That call consumes this evidence exactly once,
adds execution results, and clears it for the next invocation. Previously returned
reports never change. There is no latest-report cache or automatic report output.

Camera callbacks execute once per visible screen, preserving existing enabled and
pause rules. Camera setup/restoration errors belong to the cameras report; errors
from draw calls inside a callback belong to the commands report. Calling a frame
pipeline or `graphics_show()` recursively from a camera callback returns a failed
report without consuming the outer queue or changing its collector. Animation
updates are independent and remain permitted inside callbacks.

For manual orchestration, run cameras before commands, commands before viewport
composition, recording after composition, and presentation last. Each pipeline
returns only its own invocation; it does not automatically run prerequisites or
merge previously returned reports. Merge each returned child once if building a
custom parent. Repeat calls perform another invocation, not a cached lookup.

Execution or discard releases every queued text/texture reference exactly once,
including on fatal failure. A failed camera pass discards unsafe queued drawing
but keeps its submission evidence and discard counts for the commands report.
The full-frame call collects that evidence even when commands cannot execute.
Graphics shutdown discards pending commands and diagnostics. Engine rendering
remains single-threaded; callbacks must not destroy the renderer mid-pipeline.

### Failure policy and measurements

A failed item draw produces a warning and independent drawing continues. Missing
queued resources and invalid submission geometry retain available asset/entity
and screen evidence. Backend diagnostic text is copied before cleanup or later
backend calls can overwrite it. `GraphicsReportStage` identifies submission,
camera callback/setup, command execution, viewport composition, recording capture
or write, presentation, and restoration context.

A required target, clipping, scale, or other render-state failure is fatal to
its pipeline. Dependent work is not rendered against an unknown state. Restoration
is attempted and queued references are released; successful earlier drawing is
not rolled back. The next invocation retries normally. No persistent failure flag
or physics-recovery policy is introduced.

Recording failure disables recording and fails the recording report. Presentation
still executes; the parent remains FAILED even when presentation succeeds.
Recording start/stop retain their immediate APIs; reports describe capture and
output failures observed during updates, not later encoder exit status at stop.
Viewport interaction behavior remains unchanged; separate UI interaction reports
belong to the later UI goal.

- Header milliseconds measure CPU-side elapsed wall time, not GPU execution time.
  Camera time includes callback work. Command time starts when execution is called;
  it does not include earlier application time spent submitting commands.
- `PROCESSED_COUNT` counts successful callbacks for cameras, successful commands
  for commands, successful visible screen/UI items for viewports, captured frames
  for recording, presented frames for presentation, and visited valid animated
  sprites for animation. A viewport background is included in time/diagnostics,
  but is not a separate item count. Camera motion is included in time.
- `SKIPPED_COUNT` counts failed/aborted work in the corresponding units. Commands
  additionally count rejected submissions; one rejected request may otherwise
  have produced multiple commands. Minimized zero-area recording frames are
  skipped without a warning. Intentionally hidden/paused content is not counted.
- Commands record `QUEUED_COMMAND_COUNT` for successfully accepted commands.
  Discarded commands remain in this count and also contribute to skipped work.
- Presentation records `FRAME_LIMIT_WAIT_NS` for measured deliberate frame-limit
  waiting. It is included in the presentation/header time; do not add it again.
  VSync blocking remains inside the presentation time.
- All these counters sum when reports merge. Missing counters represent zero.
  Parent times are inclusive; never sum parent and child times as separate work.

Rendering order, camera/screen sharing, viewport layers, clipping, and resource
lifetimes retain their existing contracts. No project JSON or generated-object
format changes are required.

## Audio reports

`rohr_audio_update()` / `audio_update()` return `ROHR_AUDIO_REPORT` and submit
pending controls to the independently running SDL callback. Collection swaps
preallocated evidence banks before publishing requests under the stream lock;
report construction happens after unlocking. The result describes callback work
completed before that boundary plus the current submission count. It never
claims that newly submitted requests have already executed. Calls from multiple
application threads are serialized; each update collects a disjoint interval.

| Child report | Invocation and measurements |
| --- | --- |
| `ROHR_AUDIO_COMMANDS_REPORT` | One callback invocation per nonempty submitted batch. PROCESSED counts applied requests; SKIPPED counts failed requests. QUEUED_COMMAND counts requests newly submitted by this update, not completed playback; CANCELLED_COMMAND counts requests discarded by shutdown. |
| `ROHR_AUDIO_SOUNDS_REPORT` | One invocation per mixed chunk, including silence. PROCESSED counts active sound contributions; SKIPPED counts invalid contributions. AUDIO_FRAMES counts stereo frame slots processed once per chunk, not once per sound. |
| `ROHR_AUDIO_MUSIC_REPORT` | One invocation per chunk with an active, unpaused, positive-rate track. PROCESSED counts successful contributions; SKIPPED counts failed contributions. AUDIO_FRAMES counts requested stereo frame slots, including any silence after EOF. |
| `ROHR_AUDIO_OUTPUT_REPORT` | One invocation per submission attempt, including clamping. PROCESSED counts successful submissions; SKIPPED counts failed submissions. AUDIO_FRAMES counts successfully submitted stereo frames. |

Metric names in the table omit `ROHR_REPORT_` and `_COUNT`; all these counters
sum. Child times accumulate measured callback work in milliseconds and retain
actual invocation counts. Parent time starts before application-gate acquisition
and ends after collection (and teardown for stop). It excludes previous callback
execution and is not the sum of child times. Retained shutdown delivery preserves
the original stop timing. Missing child entries mean those pipelines did not run
in that interval. Submission-only calls
can have a QUEUED_COMMAND statistic without a completed command invocation.
An active-service update is COMPLETE even when no callback ran; an inactive
service with no retained shutdown evidence returns NOT_RUN.

These four child boundaries remain callback-owned; public manual mixing/output
is explicitly deferred. They are not a public independently callable pipeline
API. Helpers and individual sounds/tracks contribute to their owning report.
The future latest-report retrieval API remains Goal 10.

Commands referencing destroyed resources are skipped with warnings identifying
the original generation-checked handle. Seek/decode failures report commands or
music according to where they occur; unrelated sounds continue. Normal EOF,
pauses, and looping do not generate warnings. An SDL output failure retains a
fatal backend message and aborts the remaining batch; subsequent callbacks retry
without rewinding consumed samples. Callback diagnostics use the trusted internal
collector, with bounded linear lookup rather than public whole-report validation.
Shared aggregation, dropped-occurrence counts, saturation, and the reserved first
fatal still apply. Collection and merging never print or serialize.

`rohr_audio_stop()` / `audio_stop()` return a final owned report after synchronizing
shutdown, releasing resources, and counting cancelled requests. Stop retains a
copy without consuming it; the next update returns it once, then stopped updates
return NOT_RUN. Repeated stop calls also leave that copy unconsumed. Stop and the
first subsequent update report the same work: do not merge both. Successful
restart discards the uncollected copy and starts fresh; previously returned
values remain valid. This is bounded delivery, not a report history.

The current Vorbis backend still performs incremental decoding/file access in
the callback. Added reporting introduces no callback allocation, application
mutex acquisition, logging, or serialization; this does not promise that existing
SDL/decoder work is entirely free of allocation or blocking I/O.

## UI reports

`rohr_ui_update()` (direct API: `ui_update()`) finalizes immediate UI work and
returns one owned `ROHR_UI_REPORT`, with no child pipelines. The UI is ready on
first use, including before input is supplied. Each update consumes evidence
collected since the preceding update, including events received before widgets
or input setting. An empty update still runs, clears focus for missing controls,
and returns COMPLETE.

```c
rohr_ui_input_set(input);
/* Forward relevant SDL events with rohr_ui_event_add(), before widgets. */
if(rohr_ui_button("save", label, bounds, NULL).clicked) {
    save_document(); /* Widget results remain immediate. */
}
bool pointer_consumed = rohr_ui_pointer_consumed_get();
Report ui = rohr_ui_update();
/* Inspect ui here; graphics submission/execution diagnostics remain separate. */
```

This is a breaking API change: replace `rohr_ui_frame_begin(input)` with
`rohr_ui_input_set(input)` and `rohr_ui_frame_end()` with `rohr_ui_update()`;
the direct APIs change identically. There are no compatibility aliases.
Input setting replaces the snapshot and prepares input-dependent capture, but
never clears events, widget registration, pointer consumption, or UI scopes.
Repeated setters do not create frame boundaries. Read pointer consumption
before update; it describes only the current frame.

Update draws pending dropdown overlays after ordinary controls, finalizes
navigation/focus, and resets temporary registration, modal state, buffered keys,
text, wheel input, navigation activation, and the pointer snapshot. Presses and
releases cannot replay without fresh input. Focused fields, open dropdowns, and
ongoing drag identity persist while the relevant controls remain present.
Applications should supply a fresh snapshot before evaluating widgets each frame.
The report's `total_ms` measures execution inside `ui_update()` only, including
scope recovery, overlays and finalization. It excludes earlier widget execution
and time spent in application code; it is not total UI CPU time for the frame.

All metrics use the `ROHR_REPORT_` prefix and the UI report ID:

| Metric | Meaning and aggregation |
| --- | --- |
| `UI_INTERACTION_EVALUATION_COUNT` | SUM of interaction evaluations, including filtered controls, dropdown options, slider step controls and eligible scrollbar evaluations; repeated IDs count repeatedly, not as unique widgets. Disabled drawing and invalid controls rejected before interaction do not count. |
| `UI_EVENT_RECEIVED_COUNT` | SUM of key-down, text-input and wheel events submitted, including events later dropped. Unrelated events and null event pointers are ignored. |
| `UI_EVENT_DROPPED_COUNT` | SUM of relevant events rejected for UI buffer capacity. Text events are accepted or dropped whole. |
| `UI_NAVIGATION_ENTRIES_PEAK` | MAX registered navigation entries (capacity 512). |
| `UI_TEXT_BYTES_PEAK` | MAX buffered text-input bytes, excluding NUL (capacity `UI_FIELD_EDIT_MAX - 1`). |
| `UI_KEY_EVENT_SLOTS_PEAK`, `UI_TEXT_EVENT_SLOTS_PEAK` | MAX occupied slots in the two independent event buffers (each `UI_FIELD_KEY_EVENT_MAX`). |
| `UI_SCROLL_RECORDS_PEAK` | MAX scroll records retained for next-frame wheel targeting (capacity 64). |
| `UI_SCROLL_SCOPE_DEPTH_PEAK`, `UI_TRANSLATION_SCOPE_DEPTH_PEAK` | MAX simultaneous scopes of each kind (each capacity 8). |

Absent metrics mean zero work. Navigation, event, dropdown-visible-option (512),
scroll-record and scope capacity failures produce recoverable
`ERROR_ENGINE_UI_CAPACITY_EXCEEDED` warnings. The combined UI scope stack has
64 entries. Normal filtering, disabled controls, field length/type validation,
and unchanged values are not failures.

Close UI clip, scroll, translation and modal-control scopes in reverse opening
order; call clip end only after a successful clip begin. Unmatched/out-of-order
ends produce `ERROR_ENGINE_UI_SCOPE_UNBALANCED` and leave open scopes unchanged.
Update warns for each unclosed scope and unwinds UI-owned scopes before drawing
overlays. Surrounding graphics clips remain owned by the application. Do not
mutate the graphics clip stack directly inside an open UI clip/scroll scope.
Failed graphics clip pushes are never popped by UI cleanup. Graphics failures
remain in graphics reports and are not copied into UI reports.

Collection allocates, prints and serializes nothing. Explicit text/JSON output
supports these metric/error names. Shared aggregation, saturation, diagnostic
overflow, first-fatal preservation and copied-message ownership rules apply.
Retained report copies remain independent after later updates and shutdown.
Latest stored-report retrieval remains a separate pending milestone goal.


## Engine reports

Use the combined update or call its two operations independently:

```c
Report engine = rohr_engine_update();
if(engine.header.status == ROHR_REPORT_FAILED) {
    /* Handle failure before performing dependent work. */
} else {
    Tick ticks = rohr_engine_tick_delta_get();
    Report physics = rohr_physics_update(ticks);
    /* Inspect engine and physics independently. */
}
```

```c
Tick ticks = rohr_engine_tick_update();
Report lifecycle = rohr_engine_lifecycle_update();
/* The same clock/cleanup sequence, with no combined report. */
```

The direct APIs omit `rohr_` and have identical semantics. `engine_update()`
returns `ROHR_ENGINE_REPORT`, advances the clock once, then merges one
`ROHR_ENGINE_LIFECYCLE_REPORT` from cleanup. Its elapsed time independently
covers both operations and collection. The lifecycle report measures its own
sweep. Physics, graphics, audio and UI are never run or merged here. There is no
separate clock report and no implicit latest-report storage.

`engine_tick_get()` returns the accumulated tick count. `engine_tick_delta_get()`
returns the latest tick-update result, replaced even when zero; repeated reads
never consume it. Startup, shutdown and clock reset clear the delta. Clock reset
preserves accumulated time and tick count. Time-only updates accumulate elapsed
time without consuming ticks or changing the delta. Lifecycle-only calls use
current time/ticks without changing either or the delta. Paused tick calls store
zero, while combined updates still perform cleanup. Resume excludes paused
elapsed time as before.

Combined and lifecycle updates require a started engine. Unstarted calls return
FAILED with a fatal `ERROR_ENGINE_STATE_INVALID` and perform no cleanup; the
combined report has no child when rejected. Tick-only calls safely return/store
zero while unstarted. Started empty/paused/zero-tick updates return COMPLETE;
ordinary inactivity is not a warning.

Metrics use the `ROHR_REPORT_` prefix and SUM aggregation:

| Owner | Metric | Meaning |
| --- | --- | --- |
| Engine | `TICKS_ADVANCED_COUNT` | Ticks consumed by this combined update. |
| Lifecycle | `CANDIDATE_COUNT` | Live lifetime-bearing entities examined, including unexpired ones. |
| Lifecycle | `EXPIRED_ENTITY_COUNT` | Expired entities encountered for resolution/deletion. |
| Lifecycle | `PROCESSED_COUNT` | Successful direct deletions. |
| Lifecycle | `SKIPPED_COUNT` | Failed resolution/deletion attempts. |

Expiration still uses time **or** tick deadlines; zero disables that deadline.
Cleanup traverses entity slots in the existing order. Cascading destruction may
remove other entities before their slots are examined; these are not counted as
additional direct deletions. Unexpired entities are not skipped failures.

An individual entity-resolution or deletion failure is a recoverable warning:
cleanup continues, preserving existing deletion behavior. Diagnostics own their
error text and entity ID when resolved; resolution failures include the slot in
the detail. Internal stage 1 identifies resolution and stage 2 deletion. Warning
reports remain COMPLETE. Deletion may already have detached some relationships
before failing; reporting adds no rollback, quarantine, or retry policy.

Collection adds no allocation, printing or serialization. Shared diagnostic
capacity, aggregation, overflow, saturation and first-fatal preservation apply;
ordinary copies remain valid after subsequent updates, deletion, slot reuse and
shutdown. Text and JSON output recognize the new IDs and metrics.

### Breaking migration

Replace `rohr_system_tick_update()` with `rohr_engine_update()` followed by
`rohr_engine_tick_delta_get()` to preserve its combined behavior. For explicit
separation, call `rohr_engine_tick_update()` then `rohr_engine_lifecycle_update()`.
Replace `rohr_system_entities_past_lifetime_clean()` with the latter report-returning
API. The direct names migrate identically; old functions have no aliases.
The unused `ROHR_ENGINE_TICK_REPORT` enum becomes `ROHR_ENGINE_REPORT` at the same
numeric value; the lifecycle ID is appended without renumbering existing IDs.

New generated applications use the combined update and check its status before
physics. Regeneration upgrades an exact match of the preceding generated entry
point; developer-edited `main.c` files remain unchanged and require manual API
migration. Generated modules remain editor-owned.

The top-level audit leaves startup and configuration operations on their immediate
results, low-level time advancement on its existing API, and input reporting
deferred by user choice. Existing physics, graphics, audio and UI report owners
remain independent. The [coverage inventory](#system-coverage-inventory) records
their boundaries. Latest-report retrieval/composition remains Goal 10.
