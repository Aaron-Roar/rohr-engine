# Audio Update Reporting Contract

Goal 5 of [System Update Reports](../NEXT_STEPS.md#prerequisite-milestone-system-update-reports)
implements this contract. The user deferred manual sample generation/output;
those APIs are not part of this goal. See [Audio](audio.md) for usage and
[Audio reports](reports.md#audio-reports) for metrics and collection boundaries.
Follow the [reporting instructions](agent_instructions/system_reporting.md).

## Controls and ownership

- All runtime playback controls and property setters enqueue requests, including
  master volume. Creation/loading, destruction, and service startup/shutdown are
  synchronous. Preserve WAV sharing, unique music decoder ownership, and one
  active music track.
- Success means accepted into the queue. Invalid handles and a full queue return
  immediate errors; applied-state failures appear later in reports. Preserve
  existing normalization. Getters return applied state, not requested state.
- Multiple application threads may submit requests; successful enqueue order
  determines FIFO order. The engine creates no extra request-processing thread.
  Resource lifecycle and updates are serialized with application calls; SDL's
  stream lock synchronizes them with callback state. Callers still synchronize
  their own handle variables and obey SDL platform startup requirements.
- `ROHR_AUDIO_REQUEST_CAPACITY` is 256 outstanding requests, shared by pending
  and submitted work. Submission does not free capacity. No accepted request is
  silently discarded; shutdown intentionally cancels remaining work.
- `audio_update()` publishes pending requests for the callback. Requests remain
  ineligible until submitted. The callback applies eligible work in order at
  its next buffer boundary; play followed by pause is valid even if the track
  was stopped when both calls were made.
- Destruction invalidates the handle immediately. Submitted references to a
  destroyed owner produce a warning when executed and cannot act on a reused
  slot. Loading/setup remains outside the callback.

## Execution and reports

Public APIs have equivalent direct `audio_*` entry points:

```c
Report rohr_audio_update(void);
Report rohr_audio_stop(void);
```

```text
ROHR_AUDIO_REPORT
├── ROHR_AUDIO_COMMANDS_REPORT
├── ROHR_AUDIO_SOUNDS_REPORT
├── ROHR_AUDIO_MUSIC_REPORT
└── ROHR_AUDIO_OUTPUT_REPORT
```

The child boundaries are callback-owned. Unlike the public physics/graphics
pipelines, they are not independently exposed for manual execution in this goal.
Individual resources and internal decoder helpers do not produce full reports.
This is the user-approved audio exception to exposing each report owner directly.

Updates exchange preallocated evidence banks under the stream lock, then
construct their report outside that lock. Collection precedes publishing newly
submitted requests at that boundary. Reports contain completed callback work
since the last collection, plus the current submission count; an update never
waits for its newly submitted requests to execute. Callback playback continues
without another game-loop update.

Failures from requests made several updates earlier appear when observed, with
copied details and resource identity. Do not invent exact request causality.
`NOT_RUN` describes actual execution, independently of pending requests. Reports
retain shared overflow, saturation, aggregation, and reserved first-fatal rules.
No callback logging, serialization, or reporting allocation is introduced.

Failed commands are skipped with warnings; invalid sounds and failed music
processing stop the affected contribution without stopping unrelated playback.
Normal EOF, looping, and intentional pauses are not warnings. Output submission
failure fails that invocation and aborts the remaining callback batch. Later
callbacks retry output without rewinding already-consumed samples. Existing
streamed Vorbis decoding/file access remains in the callback; this goal does
not claim to make that backend fully real-time safe.

Parent time measures the update/stop call, not previous callback work. Child
times and invocation counts accumulate across the collected interval. Metrics
are specified in the report reference rather than inferred from parent time.

## Shutdown and restart

Stop synchronously destroys the stream before releasing resources, counts
cancelled requests, and returns all uncollected evidence plus shutdown results.
It retains that final report without consuming it. The next update returns the
same copy once; subsequent stopped updates return `NOT_RUN`. Repeated stop calls
before collection return the retained copy and do not consume it.

The stop result and the following update result describe the same work. Merging
both counts it twice. Returned values remain valid after resource teardown.
Successful restart begins a fresh reporting lifetime and discards any uncollected
shutdown copy; failed startup does not consume that copy.

This bounded callback handoff is distinct from Goal 10's future
`rohr_report_get(id)` latest-report API. No report history, per-request completion
query, or manual sample-buffer API is implemented here.

## Verification requirements

Verify deferred state, FIFO order, concurrent producers, saturation of pending
and submitted queues, destruction/slot reuse, stop/restart, and public/direct
parity. Hold callback execution across multiple updates, then inject command,
decode, and output failures and check later delivery. Verify unaffected playback,
normal EOF, no duplicate collection, independent copies, diagnostic overflow,
and first-fatal retention after capacity exhaustion. Exercise automatic playback
without further updates, measure reporting cost, migrate affected examples and
SDK consumers, and run native/sanitizer/Linux/Windows checks. Record platform
execution separately from compilation.
