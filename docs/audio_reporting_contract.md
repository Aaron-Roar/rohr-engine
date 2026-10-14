# Audio Update Reporting Contract

Status: approved design, not implemented. This preserves the user-approved
contract for Goal 5 of [System Update Reports](../NEXT_STEPS.md#prerequisite-milestone-system-update-reports).
The documentation prerequisite does not implement or begin audio Goal 5.
Resume that goal only on explicit user instruction.

The [shared report reference](reports.md) describes shipped report behavior.
Apply the [reporting instructions](agent_instructions/system_reporting.md) during
implementation. The [broader reporting contract](physics_reporting_contract.md)
retains milestone context; its physics setter/recovery rules do not prohibit
the explicitly approved deferred audio controls below.

## Current implementation and intended change

Current [audio APIs](../include/audio.h) mutate playback controls synchronously.
The SDL audio callback mixes WAV sound instances and one active streamed Ogg
music track. There is no `audio_update()` entry point yet. The shared report enum
already reserves `ROHR_AUDIO_REPORT`; the child IDs below are not implemented.

Goal 5 adds deferred controls and reporting while preserving callback-driven
playback. It does not move sample generation into the game loop. Existing
playback continues without calling the update function; new requests wait for
submission. This intentionally changes when playback controls take effect and
requires migrating affected callers and documenting that behavior.

## Agreed controls and lifecycle

- All playback controls and property setters queue requests: sound/music play
  and stop, music pause/resume, volume including master volume, pan where
  supported, looping, and playback rate.
- Loading and resource creation remain synchronous. Destruction and audio
  service startup/shutdown also remain synchronous. These lifecycle exceptions
  are distinct from queued playback stop requests.
- A successful control call means the request was accepted into the pipeline's
  queue, not that it was applied or that future playback is guaranteed to work.
  Known invalid handles/arguments and a full queue return immediate errors;
  execution-dependent failures belong to the report when observed. Preserve
  existing clamping/normalization unless explicitly revised.
- Preserve FIFO ordering. `play` followed by `pause` is evaluated in that order,
  rather than rejecting pause solely because play has not executed yet.
  `play` followed by `stop` ends stopped. Replaying an existing sound restarts
  that instance; this does not implicitly create another playback instance.
  Music retains the single-active-track behavior.
- Getters and checks return applied state independently of pending requests.
  A successful play request can coexist with `playing_check()` returning false
  until the callback applies it. Setter acceptance does not immediately alter
  getter results.
- Destruction invalidates the resource handle synchronously. Pending requests
  for a destroyed resource are skipped and reported without reviving the handle
  or affecting a replacement in the reused slot.
- Service shutdown stops playback and clears pending requests safely before
  resource teardown. Previously returned reports remain independently valid.
- Queues use named fixed capacities. Queue exhaustion is explicit; accepted
  requests are not silently discarded. Shutdown intentionally cancels pending
  work as described above.

## Update and report contract

Planned public/direct entry points, not declarations of shipped APIs:

```c
Report rohr_audio_update(void);
Report audio_update(void);
```

The update submits queued requests for the callback to apply at an audio-buffer
boundary. It returns request-processing results plus completed callback work
collected since the previous update. It does not wait for newly submitted
requests to execute. Execution failures can therefore appear in a later report
than the update that submitted the request.

Approved report ownership:

```text
ROHR_AUDIO_REPORT
├── ROHR_AUDIO_COMMANDS_REPORT
├── ROHR_AUDIO_SOUNDS_REPORT
├── ROHR_AUDIO_MUSIC_REPORT
└── ROHR_AUDIO_OUTPUT_REPORT
```

- Commands describe request processing; sounds describe sound mixing; music
  describes streamed music processing; output describes sample submission.
  Individual sounds/tracks and internal decoding helpers do not get full reports.
  Finalize runnable child interfaces and their prerequisites before implementing
  them; the report tree alone does not define safe manual execution.
- Reports describe work actually performed, independently of request state.
  `NOT_RUN` means the subsystem did not run during the collection interval.
  A successful music processing invocation can be reported while a new play
  request is still pending; neither status claims that request completed.
- If a request submitted several updates earlier causes a failure now, report
  that failure on the next collection, identifying the affected sound or music
  resource and copying its diagnostic details. Do not invent exact request
  causality when only the resource and failed operation are known.
- Callback evidence accumulates with bounded storage between collections, using
  shared aggregation, visible overflow, and reserved first-fatal preservation.
  Collect each occurrence once; later independent failures are new evidence.
  There is no unbounded history or per-request completion-query API in this goal.
- A failed sound or music track does not stop unrelated playback. Normal end of
  playback, intentional pauses, and looping are not warnings by themselves.
  Define the precise handling of decode/output failures before implementation.
- Collection never prints or serializes in the callback. Report collection and
  reading must not be required for existing playback to continue.
- Callback work and update-call wall time cover different execution intervals;
  document their timing boundaries rather than summing them into a misleading
  parent duration.

This callback handoff is required for Goal 5. It is distinct from Goal 10's
future `rohr_report_get(id)` latest-report copies and automatic composition;
Goal 5 does not implement that retrieval API or change its priority.

## Implementation details still to resolve for Goal 5

These are open details, not additional approved API or behavior:

- Exact queue capacity/name, pending versus submitted storage, and backpressure
  handling that preserves accepted requests and FIFO order.
- Supported calling threads, synchronization and report storage strategy,
  callback budget, and an exact collection boundary that prevents duplicate or
  lost evidence during concurrent callbacks and shutdown.
- Directly runnable child signatures, buffer ownership, and prerequisites that
  prevent manual calls from advancing live playback twice or racing the callback.
- Stage-specific statistics and timing definitions, output/decode error severity,
  retry versus stopping affected playback, and inactive-service update behavior.
- Delivery or explicit cancellation of uncollected diagnostics at shutdown and
  the reset boundary on restart. No guarantee to return a shutdown report has
  been agreed for the existing void shutdown API.

## Acceptance criteria for audio implementation

- Verify FIFO control sequences, delayed applied-state getters, queue saturation,
  stale handles/slot reuse, destruction, shutdown/restart, and public/direct parity.
- Deterministically submit a request, defer callback execution across multiple
  updates, then inject failure. Verify the later collection reports the affected
  resource and earlier reports remain unchanged.
- Verify completed work is collected once, later failures remain observable,
  unrelated playback continues, and normal EOF/pause is not diagnosed as failure.
- Verify reports survive destruction and later updates; test overflow and the
  first fatal arriving after ordinary diagnostics are full.
- Verify callback handoff and independent pipeline composition without duplicate
  work, and that ongoing playback does not require game-loop updates.
- Migrate affected callers/examples, verify relevant sanitizer and Linux/Windows
  builds and installed-SDK consumers, and document any unverified runtime platform.
