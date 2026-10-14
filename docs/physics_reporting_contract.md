# Physics Update Reporting Contract

Status: design contract for the System Update Reports milestone, including its
physics integration. This document distinguishes
verified current behavior, agreed requirements, and proposed implementation
details. Shared collection, merging, and physics integration are implemented as documented
in [Shared reports](reports.md). That page is the current API reference; the
original physics inventory and proposed refinements below retain design context.
Console, JSON, text-file output, graphics reports, and audio reports are also implemented. Other system
integrations and retrieval remain scheduled work. The
capacity, shared-type, return-ownership, and output decisions below are settled;
finalize runnable boundaries during each integration goal. Shared infrastructure
is Goal 1, physics integration Goal 2, console/JSON output Goal 3, and
[graphics integration](reports.md#graphics-reports) Goal 4.
Simulation-failure recovery is Goal 11, after all systems have reporting.

The separate Reporting Workflow Standards and Audio Contract documentation
milestone precedes audio Goal 5 without renumbering this milestone. Follow the
[reporting instructions](agent_instructions/system_reporting.md); the
[audio contract](audio_reporting_contract.md) preserves the approved deferred
controls and asynchronous report delivery implemented in Goal 5. Its
explicitly approved audio behavior does not change physics setter or recovery
semantics. Public manual audio mixing/output remains deferred.

## Scope and decisions

Build the shared report system first. Physics is the first integrated report
owner, followed by console/JSON output, graphics, audio, input, UI, and
engine-tick cleanup. Share diagnostic
concepts without making physics depend on another system or a global queue.
The force/torque milestone will consume this reporting mechanism later.
Goal 9 verifies reporting coverage; Goal 11 then addresses simulation-failure
recovery. Do not change recovery policy during a reporting integration goal.

Agreed requirements:

- A physics update directly returns a self-contained report containing status,
  timings, statistics, warnings, and fatal details. Developers retain that value
  themselves; the normal update path does not require a report getter.
- Success means processing completed, not that every contribution was applied.
  Recoverable problems skip affected work and appear as warnings; fatal
  failures stop work that cannot safely continue.
- Only directly runnable system/pipeline entry points generate reports. Each
  such pipeline returns its own report; its internal stages contribute to that
  report without creating a report per helper. The full physics update combines
  the reports of pipelines it runs. Custom callers can inspect the same reports
  directly or combine selected reports themselves. Other systems remain independent.
- Previously returned report values are independent of subsequent updates and
  engine lifetime; there are no borrowed diagnostic buffers.
- Equivalent warnings within an update aggregate with occurrence counts.
  Report overflow is visible, never silently discarded.
- Invalid setter arguments return immediately and preserve previous state.
  They do not become deferred pipeline errors merely because a report exists.
- An intentionally incomplete future force source is suspended normally.
  Unexpected runtime invalidity is reported when its contribution is skipped.
  A one-tick failure does not extend the request's expiry.

Settled decisions:

1. **Capacities:** 32 report entries, 64 statistics, and 256 distinct diagnostics,
   defined by public named constants. Aggregate repeats, expose overflow counts,
   and reserve first-fatal details even when ordinary diagnostic storage is full.
2. **Report ownership:** self-contained value returned directly by the update,
   including diagnostic records, that callers can retain after subsequent updates.
   No pointers into mutable engine storage or borrowed diagnostic strings.
3. **Simulation recovery is deferred:** currently quarantining an entity also
   fails the update. Preserve that behavior during reporting implementation.
   Goal 11, after all reporting integrations and their audit, will decide failure flags,
   per-update retries, restoration, and state preservation. Neither a particular
   recovery policy nor rollback behavior is approved yet. Setter/creator errors
   remain immediate and are not the simulation-recovery problem being deferred.
4. **Shared type:** all systems and runnable pipelines use the same fixed-size
   `Report`, not separate physics/graphics types. A fixed header and enum IDs
   identify the operation; bounded entries/statistics avoid system-specific
   named struct fields. `ReportId` values follow `ROHR_PHYSICS_REPORT`,
   `ROHR_PHYSICS_JOINTS_REPORT`, and `ROHR_GRAPHICS_REPORT` naming, without
   `STAGE` in the identifier.
5. **Output:** explicit console printing and JSON-file writing are part of the
   reporting API. Use the engine console and existing yyjson dependency. Increase
   the console's named line buffer to 1,024 bytes and print reports line by line,
   detecting truncation. Collection itself does not print or allocate JSON.
6. **Goal 1 collection API:** provide public constructors and helpers to add
   entries, statistics, and diagnostics and to merge reports. Game developers
   can build reports for custom pipelines through the same API as engine code.
   Collection and merging perform no allocation or simulation work.
7. **Custom IDs:** reserve documented numeric ranges for developer-owned report
   and metric IDs alongside engine-defined enum values. Custom statistics must
   specify their aggregation rule explicitly; do not silently assume summation.
8. **Critical failures:** critical means fatal in the API. Preserve the first
   fatal diagnostic and its original detail message in the dedicated slot,
   regardless of ordinary-list capacity. A failure captured in that slot is
   retained, not counted as dropped merely because the ordinary list is full.
   Later successes or fatal errors cannot erase or replace that first failure.

The remaining system integration definitions below are the proposed contract for review.
They must not be treated as already shipped API or approval to change runtime
behavior before its relevant implementation goal is explicitly selected.

## Pre-integration entry-point inventory (historical)

| Entry point | Current behavior | Reporting responsibility |
| --- | --- | --- |
| `physics_update(ticks)` | Zero ticks returns success without entering the pipeline; otherwise converts ticks to a single elapsed duration. | One top-level report for the invocation that actually performs physics, not one report per engine tick. |
| `physics_dt_update(dt)` | Nonpositive duration returns success without entering the pipeline. | Same owner as tick-driven updates. |
| `system_physics_update(dt)` | Updates animated-hitbox bindings, then calls the pipeline. | Include preparation as part of the report; currently preparation occurs before report reset. |
| `physics_pipeline_update(dt)` | Resets the report; runs configured substeps; breaks on an error; records elapsed time. | Return an aggregate report for the pipelines/substeps actually executed. |
| `physics_pipeline_step_begin()` | Resets report timestamp/tick and advances interaction-transition state. | Preparation helper; not a report-generating update on its own. |
| `physics_pipeline_substep(dt)` | Runs the standard stage sequence, returning integration failures. | Return its report when exposed as a directly runnable pipeline; the full update combines it. |
| Individual `physics_pipeline_*` stages | Some return `EngineResult`; most return `void`. | Classify by actual runnable contract, not name or visibility: runnable pipelines return reports, internal/preparation helpers contribute to their owner. |
| `physics_update_report_get()` and `system_physics_update_report_get()` | Return a copied `PhysicsUpdateReport` containing metrics only. | Replace latest-report access with directly returned reports in the new API. |
| Public `rohr_` wrappers | Forward to the corresponding direct APIs. | Preserve identical results, report contents, and lifetime semantics. |

Sources: [physics_config.c](../src/physics/pipeline/physics_config.c),
[physics_step.c](../src/physics/physics_step.c),
[physics_pipeline.c](../src/physics/pipeline/physics_pipeline.c),
[physics_stages.c](../src/physics/pipeline/physics_stages.c),
[physics.h](../include/physics.h), and [rohr.c](../src/core/rohr.c).

Physics does not own engine clock advancement or lifetime cleanup; those remain
in `system_tick_update()` and later receive their own report. A timing report
must not cause extra physics steps, advance engine time, or change source expiry.

## Audited stage inventory

| Stage identity | Work covered | Current observation |
| --- | --- | --- |
| Preparation | Animated-hitbox binding update and interaction-step initialization | Binding update precedes the current report reset. |
| Substep begin | Clear transient contacts/joints and force-derived acceleration | Must not clear update-level warnings. |
| Gravity | Apply opted-in global gravity | Currently a void stage. |
| Spring and beam loads | Spring-joint and soft-body-beam contributions | Separate from persistent force/torque source application. |
| Force sources | Resolve/apply force and torque sources | Currently inside `physics_rigid_integrate()`, not `physics_pipeline_forces_apply()`. |
| Integration | Velocities, angular motion, positions, limits, and transform locks | Out-of-range positions quarantine affected entities and return an error. |
| Broad phase | Build spatial index and enumerate candidates | Existing timing and tree/candidate counters. |
| Narrow phase/contact gathering | Rigid and soft-boundary geometry queries and contact allocation | Contact-list growth can fail; collectors must surface allocation failures. |
| Joint gathering | Assemble pin/weld constraints | Some current append results are ignored; audit and propagate them. |
| Constraint response | Iterative rigid/soft contacts and joint solves | One shared iterative solver; no report per solver iteration. |
| Finalization | Contact publication and required transform/hitbox refresh | Report failures before marking the update complete. |

The stage inventory is observational, not permission to reorder physics or
generate reports for every listed stage. Final API review must classify directly
runnable pipelines and document their prerequisites, including a joints-only
workflow for editor/custom callers. Merely being a public function does not
make a helper a complete runnable pipeline.
Timings may subdivide a stage without introducing an additional simulation pass.
Relevant sources: [rigid_body_step.c](../src/physics/rigid_body/rigid_body_step.c),
[soft_body_solver.c](../src/physics/soft_body/soft_body_solver.c),
[joint_solver.c](../src/physics/joints/joint_solver.c),
[contact_constraint.c](../src/physics/collision/contact_constraint.c), and
[constraint_solver.c](../src/physics/constraints/constraint_solver.c).

## Proposed data and API shape

Change top-level update return types from `EngineResult` to the shared `Report`.
The report contains completion/failure status and a fatal `EngineError` when
failed. A completed report may contain warnings. Setters and creators retain
their existing result types; this change concerns physics update boundaries.

Replace the physics-specific report layout with a common header, report entries,
statistics identified by metric IDs, and diagnostics. Preserve existing metric
meaning through this new representation, not through physics-only field names.

```c
#define ROHR_REPORT_ENTRY_CAPACITY 32u
#define ROHR_REPORT_STATISTIC_CAPACITY 64u
#define ROHR_REPORT_DIAGNOSTIC_CAPACITY 256u
#define ROHR_REPORT_DETAIL_CAPACITY 256u

typedef enum ReportSeverity {
    ROHR_REPORT_SEVERITY_NONE = 0,
    ROHR_REPORT_SEVERITY_WARNING,
    ROHR_REPORT_SEVERITY_FATAL,
} ReportSeverity;

typedef struct Report {
    ReportHeader header;

    size_t entry_count;
    ReportEntry entries[ROHR_REPORT_ENTRY_CAPACITY];

    size_t statistic_count;
    ReportStatistic statistics[ROHR_REPORT_STATISTIC_CAPACITY];

    size_t diagnostic_count;
    ReportDiagnostic diagnostics[ROHR_REPORT_DIAGNOSTIC_CAPACITY];

    uint64_t dropped_entry_count;
    uint64_t dropped_statistic_count;
    uint64_t dropped_diagnostic_occurrences;
    bool counters_saturated;

    bool has_fatal;
    ReportDiagnostic first_fatal;
} Report;
```

The common header identifies the report, completion status, and elapsed time.
Entries summarize runnable child operations and invocation counts. Statistics
identify their report and metric via enums. Diagnostics describe issues; `NONE`
supports empty/default values, not appended no-issue records. The remaining
record definitions must carry the context below without introducing pointers or
system-specific top-level report layouts:

- An update sequence number independent of engine tick, because manual updates
  and paused engine time can share a tick.
- A status: not run, complete, or failed. Internal collection state need not be
  exposed as an additional status on a finalized return value.
- Attempted and completed substep counts.
- Stored diagnostic count, total warning occurrences, and dropped occurrences.
- Inline storage for 256 distinct diagnostics, with self-contained records.
- A first-fatal record with an explicit presence flag; this survives collection
  overflow and agrees with the error returned by the update.

Each diagnostic contains:

- Severity (warning or fatal), originating report ID, and `EngineError` code.
  Internal-stage context may further identify where the issue occurred without
  making that helper a report-producing operation.
- Source and related-target context with explicit presence/type information.
  For physics, store full generation-checked entity handles, not pool indexes.
  Copy handles as evidence; do not retain or dereference deleted entities.
- Recovery action, such as contribution skipped, entity quarantined, or update
  aborted. Severity alone does not explain what happened to the simulation.
- First and last affected substep, with explicit absence for preparation errors.
- Occurrence count. Solver iteration is optional context, not a separate owner.
- An optional copied detail string using `ROHR_REPORT_DETAIL_CAPACITY` (256 bytes,
  including the terminating NUL), with a truncation flag. Use stable error-code
  messages where no additional detail is needed. Preserve the first fatal
  message even when later errors supply different text.

Enum constants use `ROHR_` prefixes. Exact enum entries follow this inventory;
reuse `EngineError` where its meaning matches, adding specific codes where needed.
Do not store pointers returned by the shared last-error message API: later errors
overwrite those buffers. Collection must not invoke logging, create text assets,
or depend on graphics. See [error.h](../include/error.h) and
[error.c](../src/core/error.c).

Proposed public signatures implementing the agreed direct-return design:

```c
Report rohr_physics_update(Tick ticks);
Report rohr_physics_dt_update(Time dt);
Report rohr_physics_pipeline_update(double dt);
Report rohr_system_physics_update(double dt);
```

Directly runnable subordinate pipelines also return self-contained reports.
The exact runnable entry-point inventory remains part of final API review.
Do not require a global reporting begin/end session merely to obtain the report
of one runnable pipeline. Existing physics preparation/reset operations retain
their simulation purpose; reporting must not silently add extra resets.

Provide one public report-combination helper, shared by the full update and
custom callers. Internal stages use a collector owned by the pipeline executing
them; ordinary helpers keep appropriate immediate return values and do not
materialize reports themselves. A pipeline stops dependent work on fatal failure
and returns its collected details. Normal usage has no drain operation or
requirement to fetch a retained global last report.

Combining reports performs no physics work. Preserve diagnostic pipeline/stage
identity and execution context, aggregate matching occurrences, carry forward
source overflow counts, and count additional losses when the destination fills.
Propagate failure monotonically: a failed child makes the destination failed;
a later successful child cannot clear it. Preserve the first fatal in merge
order, including its detail text, even when ordinary storage is full. Overflow
alone does not make an otherwise completed report failed. Do not double-count
a fatal record stored both in the ordinary list and
the reserved fatal slot. Sources remain unchanged and independently retainable.
Combine each executed child report once; do not combine both an aggregate and
its children into the same parent. Distinguish parent elapsed time from child
work timings so inclusive totals and substep counts are not added twice.

## Boundaries and partial work

Proposed lifecycle rules:

1. Each runnable pipeline owns collection for its invocation. The outermost
   update combines the reports it receives and includes its own preparation work,
   including animated-hitbox preparation. Internal helpers never reset the
   enclosing collector.
2. Each executed substep retains context and timings in the combined report.
   A fatal error stops unsafe dependent work; recoverable errors continue only
   work independent of the failure. Report combination never chooses recovery.
3. Custom callers run the pipelines they need, save the directly returned values,
   and optionally combine them. A pipeline's documented prerequisites still
   apply; a report does not make joint solving independent of required setup.
4. Low-level stage sequences must run inside an owning runnable pipeline to
   obtain an aggregate report. Final API review must document how existing
   direct-stage callers migrate without creating a report per internal helper.
5. A legitimate no-op invocation returns a fresh NOT_RUN
   report with zero work counts; an executed empty pipeline returns COMPLETE. Preserve existing nonpositive-duration no-op
   behavior where it exists; reject nonfinite durations before simulation.
   Document that this fresh-report behavior differs from today's early returns.
6. Before the first update, there is no report to fetch. A zero-initialized
   caller-owned report may use not-started status. Engine teardown releases
   internal collection state; previously returned values remain valid. Restart
   begins a new reporting lifetime.

Reports are observations, not transactions. An update may have advanced earlier
entities or substeps before a fatal failure. Do not imply rollback or automatically
retry a failed update. Report completed/attempted substeps and the failure stage
so callers can make informed decisions.

## Recovery and compatibility

| Condition | Proposed behavior |
| --- | --- |
| Invalid setter argument | Immediate error; previous successful value preserved; no deferred warning. |
| Deliberately disabled, held, collision-filtered, or incomplete source | Expected behavior; no warning solely for inactivity. |
| A runtime contribution becomes unrepresentable | Warning plus skipped contribution when safe; other sources continue. Future force behavior is implemented in its own milestone. |
| Out-of-range integrated position | Preserve existing fatal/quarantine behavior during reporting integration. Goal 11 determines its replacement recovery policy. |
| Contact/joint storage allocation failure | Fatal; do not simulate as if missing constraints were a complete valid solve. Retain first failure and partial progress. |
| Diagnostic storage full | Count dropped occurrences and keep processing according to the underlying condition; always preserve first-fatal details. |
| Nonfinite update duration | Fatal validation failure before simulation; report identifies validation stage. |

Update return types and the public report layout change: rebuild SDK consumers
together. Replace latest-report getter usage with locally retained return values;
remove obsolete report getters rather than leave deprecated duplicate access.
Migrate both direct APIs and `rohr_` wrappers and all first-party callers.
Do not change saved project formats solely for transient reports. Generated
runtime sources and examples must demonstrate checking update results and
inspecting warnings without overwriting developer-owned project entry points.

Changing ignored allocation failures into reported fatal errors is an intentional
error-propagation correction. It must be covered by failure injection and called
out in the implementation commit. Reclassifying existing fatal simulation errors
as warnings is a separate behavioral decision, not an automatic reporting change.

## Timing and statistics

Preserve the information represented by existing `timestamp`, `tick`, `total_ms`,
broad-phase build/query, narrow-phase and response timings, plus current
collision/tree/entity counters, using the common header/identified entries and
metric representation. Existing metric meanings do not require retaining the
old physics-specific C field names.
Document existing counters as work counts where they accumulate per substep,
rather than claiming they count unique entities for an entire update.

Add preparation, load-application, integration, joint-gathering, and finalization
timings where needed to account for missing stages. All times use a monotonic
clock and milliseconds; timings are nonnegative, including failed updates.
Identify inclusive vs exclusive timings so totals are not double-counted.
Each runnable pipeline's `total_ms` measures its own elapsed time. A caller-built
aggregate must distinguish summed child work from overall wall-clock duration;
combining reports cannot infer caller work or waiting between pipeline calls.

Report correctness cannot depend on performance measurement being enabled.
Warnings, status, overflow, and fatal details remain available even if timing
collection is disabled. Avoid hardware-specific timing thresholds in correctness
tests; measure collection overhead in Goal 1 and integration overhead in Goal 2.

Metric definitions specify aggregation: work counters sum, peak measurements
take the maximum, and end-state measurements use the latest value. Do not sum
tree heights or final collection sizes as if they were event counts. Latest
means last in merge order, not greatest timestamp. Engine metric definitions
provide the rule; custom metrics specify it explicitly. Matching child entries
accumulate elapsed time and invocation count, but merging never invents or
overwrites the parent's independently measured elapsed time.

## Agreed storage and aggregation

Allocate no memory while collecting or merging reports. Bounded inline storage
provides predictable failure behavior.
The update returns the report by value; saving that value introduces no hidden
heap ownership. Measure report size and return/copy cost before finalizing layout.

Aggregate by originating report ID, internal stage, severity, error code,
recovery action, source, and related target. First/last substeps and count update
on a match. Preserve the first detail message for an aggregated entry, especially
the original critical failure message. Distinct targets remain distinguishable.
Repeated failures in later physics updates appear in those later reports too;
there is no global suppression hiding a still-invalid source.

When full, existing entries can still accumulate repeats. Capture the first
fatal in its reserved slot before testing ordinary-list capacity. If the list
cannot also hold it, that first fatal is still retained and is not a dropped
occurrence. Matching repeats can update its reserved occurrence count without
overwriting its original message. For other unmatched diagnostics that cannot
be retained, increment dropped-occurrence count; do not claim it measures
distinct lost errors. Saturate counters on overflow and expose saturation
rather than wrap. Merging applies the same rules to both ordinary source records
and a source's reserved fatal record; retain each occurrence only once logically.

Custom report/metric ranges are part of the public API and must not collide
with engine-defined IDs. Implement named range boundaries and validate IDs
through the public helpers. Numeric custom IDs remain usable without registering
global mutable names; output can use a numeric fallback. Developer naming
registries are not required for Goal 1.

## Human-readable output and JSON

Output is explicit and separate from collecting or combining reports. Physics
updates neither print automatically nor construct JSON. A caller can inspect a
report, print it, write it to a file, or retain it without affecting simulation.

Implemented public output APIs:

```c
EngineResult rohr_report_console_write(const Report *report);
EngineResult rohr_report_json_write(
    const Report *report, const char *path, bool pretty);
EngineResult rohr_report_text_write(const Report *report, const char *path);
```

- Console printing goes through the engine console interface, not a parallel
  report-specific terminal backend. The engine already exposes
  `rohr_console_write()`; see [console.h](../include/console.h),
  [console.c](../src/core/console.c), and [rohr.h](../include/rohr.h).
- Increase `MAX_LOG_STR` from 100 to 1,024 bytes. Keep its definition centralized;
  report output must not scatter literal buffer limits through formatting code.
  Preserve the existing 100-entry console history: text storage grows from about
  10 KB to 100 KB, while retained entry count stays unchanged.
- Print the header, entries, statistics, and diagnostics as separate formatted
  lines. Never try to fit an entire report in one console write. The terminal
  receives the full sequence even when it exceeds retained console history;
  history replay only contains its most recent 100 entries.
- Detect truncation rather than silently clipping. A shortened line must have
  a visible truncation indication, and the report-print call must signal that
  output was incomplete. Preserve the source report unchanged. Check actual
  formatted length, including console prefix, newline, and terminating NUL.
  The existing void console-write API can retain its signature; the checked
  report-print path must propagate output failures through `EngineResult`.
- JSON writing uses the existing yyjson dependency internally. `pretty` selects
  readable indentation versus compact output. JSON file output is independent
  of the console line-buffer size and its history ring.
- Both forms include readable ID/status/severity/metric names, error codes and
  messages, timings, work counters, applicable source/target and substep context,
  repetitions, overflow/saturation, and reserved fatal details. Include stable
  numeric identifiers alongside names in JSON; serialize only populated slots.
  Never emit `NONE` diagnostic entries for unused storage.
- Serialize copied diagnostic detail rather than consult mutable last-error
  text. yyjson handles JSON escaping. Do not inject console category prefixes
  into serialized JSON. Validate counts and pointers before reading arrays.
- Output functions return their own errors for invalid arguments, allocation,
  truncation, and I/O failures. They do not rewrite the report's status or append
  failures into that same report. Any temporary JSON allocation is confined to
  the explicit output operation and released on success and failure.
- This is a diagnostic export, not project persistence or a report-loading API.
  It does not change saved project formats or introduce automatic log files.

Example (both output results must be checked by callers):

```c
Report report = rohr_physics_update(ticks);
EngineResult printed = rohr_report_console_write(&report);
EngineResult written = rohr_report_json_write(&report, "physics_report.json", true);
```

Text files share the console formatter without console prefixes. The general
file API is deferred. See [the version 1 export contract](reports.md#json-format-version-1)
for enum objects, exact integer strings, diagnostics, and failure behavior.

## Acceptance criteria for implementation

Shared layout/storage/merge checks belong to Goal 1; physics pipeline and caller
migration checks belong to Goal 2; console/JSON checks belong to Goal 3. Verify
each goal's requirements when implemented rather than deferring them to the
system-wide coverage audit.

1. Standard tick/dt/system/pipeline entry points produce equivalent diagnostics
   for equivalent work; report composition includes preparation and counts each
   executed child once. Directly runnable pipelines also return useful reports
   without running the full physics update or querying global state.
2. Warning-only updates return complete reports and still simulate unaffected entities.
   Assertions verify what was skipped, not merely that a warning exists.
3. Fatal updates return failed reports with the first fatal code, retain earlier warnings/timings,
   stop unsafe dependent stages, and expose partial progress without rollback.
4. Test every inventoried stage's diagnostic path where a real failure exists;
   use focused fault injection for allocation errors rather than leaving silent
   constraint omissions. Do not invent warnings for normal filtered/no-op work.
5. During reporting implementation, preserve the current quarantine outcome and entity
   hold/hitbox/motion semantics. Existing `tests/engine_lifecycle.c` requires a
   failed update and records the affected entity count; changes must be explicit.
6. Verify aggregation across substeps/iterations, distinct entities and reasons,
   overflow, saturated counters, and fatal details after capacity exhaustion.
   Specifically fill ordinary diagnostics before the first fatal arrives:
   verify the dedicated slot keeps its message, status becomes failed, and the
   retained fatal is not counted as dropped. Later merges must not replace it.
7. Verify report copies survive later updates, source deletion/slot reuse, and
   engine restart under the agreed ownership contract; details never alias
   the shared last-error buffer.
8. Verify zero-work calls, nonfinite duration, standalone runnable pipelines,
   their documented prerequisites, custom composition, and early failure.
   Merge tests cover source immutability, repeat aggregation, source/destination
   overflow, preserved fatal details, and non-duplicated metrics.
9. Verify all public/direct access paths agree and unrelated systems do not
   reset physics reports. Assert useful metrics without brittle exact timings.
10. Verify no allocations during collection and no reliance on renderer/fonts
    or logging. Run focused sanitizers, relevant examples/generated consumers,
    and Linux/Windows compilation. Report separately whether Windows executed.
11. Verify the same shared report type, named capacities, zero/default severity,
    and metric aggregation rules across standalone and combined physics reports.
    Shared-library tests cover public construction/collection/merging, custom ID
    ranges, explicit custom metric rules, copied/truncated detail strings, and
    preserved first-critical text without requiring a physics update.
12. Verify console output for reports larger than one line and larger than its
    history ring, prefix-aware line truncation, preserved source reports, and
    1,024-byte console buffering. Parse pretty and compact JSON back with yyjson
    to verify fields, escaping, counts, overflow, and fatal details. Cover invalid
    counts/pointers, allocation and output failures, and temporary-buffer cleanup.

## Deferred simulation-failure recovery

Goal 11 must distinguish rejecting an invalid attempted step from mutating or
rolling back already-committed state. Decide whether a reserved failure flag
excludes an entity until explicit repair/reset, or whether later updates retry
it automatically and report each repeated failure. Define contacts, joints,
soft-body dependencies, and developer restoration before implementing either.
Do not implement a new flag, destructive quarantine replacement, or automatic
retry as part of reporting. Setters/creators keep immediate validation semantics.

Goals 1 through 5 implement shared report values, collection/merging, physics
pipeline return types, console/JSON/text-file output, graphics pipelines, and audio reporting. Other systems,
stored-report retrieval, and recovery remain scheduled work. Later-goal acceptance criteria are not claims
of implementation.

## Agreed latest-report retrieval (Goal 10)

After reporting coverage, add `Report rohr_report_get(ReportId id)`. Each runnable
pipeline stores its latest report, overwritten by its next update, including
individually runnable children. Return independent copies; do not retain history.
`ROHR_REPORT` requests automatic composition of the latest available reports
without counting parent-included child work twice. Systems may have updated at
different times. Never-run pipelines yield NOT_RUN; updates keep returning reports
directly. Resolve invocation provenance, independent child updates, and lifecycle
and callback synchronization in that goal; no retrieval API ships in Goal 2.
