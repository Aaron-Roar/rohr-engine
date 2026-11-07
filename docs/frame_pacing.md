# Frame pacing before game development

Milestone 14 Goal 1 corrects software frame limiting before the small C game
checkpoint. The comparison parent is
`178711a263d81824136ac5c332b6621c19391206`. Physics algorithms, settings, scenes,
and public signatures are unchanged. Further physics development is paused by
user direction, with its known failures preserved.

## Cause and change

The software limiter waited before `SDL_RenderPresent`, then started the next
interval after presentation returned. Every frame therefore took the nominal
interval **plus presentation time**, even when rendering and physics fit the
budget. Previous car/pit runs consistently reached only about 58–59 FPS.

The interval now begins immediately before `SDL_RenderPresent`. Presentation
cost consumes part of the next frame's budget. The limiter still spaces actual
presentation starts; a late frame presents immediately and establishes a fresh
interval, without accumulating catch-up frames. It does not compensate for
scheduler oversleep by shortening subsequent intervals.

VSync retains ownership of pacing when enabled. The software cap applies only
with VSync off. Zero still means the existing 120 FPS fallback, not unlimited
rendering. Successful VSync/limit changes reset the interval; invalid or failed
changes preserve existing state. Presentation failures retain their reporting
and recovery behavior. Wait metrics measure actual software waiting, while the
presentation report includes both waiting and backend presentation work.

This fixes avoidable limiter delay, not arbitrary overload or display/compositor
jitter. Frames whose actual work exceeds the budget still run late. It does not
complete the physics milestone's performance or numerical acceptance gates.

## Reproduction and verification

`tests/graphics_reports.c` uses a simulated clock and presentation backend to
verify timing independently of scheduler jitter. The old engine fails on the
second frame with nonzero presentation cost. The fix verifies 120 consecutive
frames, direct/composed/public paths, long stalls, over-budget presentation,
oversleep accounting, failed presentation, VSync ownership and failed changes,
limit changes, zero fallback, and graphics restart.

The independent SDK consumer in `tools/frame_pacing/` shows three moving bars
with different speeds and records 600 frame intervals. Its normal mode caps at
60 FPS. `--vsync` follows the display; `--stall` injects one 200 ms stall;
`--toggle` changes VSync at frames 200 and 400. Escape, window close and terminal
interrupt stop the run and report an incomplete endpoint. VSync waiting remains
inside its `nonwait_ms` field: that field excludes only software limiter waits
and must not be read as exclusive CPU/render work.

Build against an installed SDK inside `nix develop`:

```sh
cmake -S tools/frame_pacing -B build/frame_pacing \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PWD/build/example-sdk"
cmake --build build/frame_pacing --parallel 4
build/frame_pacing/frame_pacing
build/frame_pacing/frame_pacing --stall
build/frame_pacing/frame_pacing --vsync
build/frame_pacing/frame_pacing --toggle
```

Evidence is retained in `sandbox/frame_pacing/`, including frozen control SDK,
build/check logs, commands, executable hashes, display/window information and
per-frame lightweight samples. Serial before/after comparisons use AC power,
closed heavy workloads and mapped 1280×720 windows. Window sizing settles after
mapping; startup remains included in the workload timing. The 600-frame car is
an explicitly declared ten-second diagnostic, not its inherited sixty-second
acceptance endpoint. The pit retains all 800 particles for twelve seconds.

An interrupted control car run and an early window-sizing race remain separate
incomplete records. They are excluded from completed-run comparisons. An initial
sandboxed sanitizer run reached its scene endpoint but leak checking failed
under ptrace; it is not a successful sanitizer verification.

## Results

All 18 comparison endpoints complete: three serial runs per workload/version.
Observed FPS ranges are:

| Workload | Control | Corrected |
|---|---:|---:|
| Moving bars, 600 frames | 58.987–59.366 | 60.005–60.012 |
| Car, ten-second prefix | 59.029–59.124 | 60.061–60.064 |
| Pit, twelve seconds | 58.973–58.992 | 60.046–60.047 |

The lightweight frame-interval p95 improves from 17.003–17.052 ms to
16.820–16.872 ms. Corrected car/pit frame-work p95 remains 6.407–6.542 ms and
10.151–10.311 ms respectively, excluding software waits. This is a pacing
correction, not a physics throughput optimization.

Control car accumulates 148–164 ms of lag over ten seconds; control pit accumulates
205–209 ms over twelve seconds. Corrected runs retain small initial offsets
(car about −10 to −11 ms, pit about −9 ms), with no positive accumulating lag in
these runs. Slightly above-60 averages include that initial offset, not a higher
target rate. All three pit runs pass their existing playback gate. The car still
correctly reports its sixty-second acceptance incomplete; only the declared
ten-second pacing diagnostic is complete.

Every car run retains identical 2,400 substeps, 701,387 candidates and 108,585
contacts; every pit run retains 2,880 substeps, 4,283,479 candidates and 152,816
contacts. The user accepted the initial corrected previews ("looks really good").
That feedback does not waive any previously recorded physics accuracy failures.

All three additional 600-frame review endpoints complete:

- A deliberate stall and focus-away/return run records one 200.604 ms interval
  with no software wait, followed by 16.818 ms and normal pacing. Focus loss and
  regain are observed; there is no accumulated catch-up burst.
- VSync plus focus switching averages 120.060 FPS on the 120 Hz display, with
  median/p95 intervals 8.332/8.815 ms and no software limiter waits.
- Switching VSync on and back off reaches the endpoint and returns to 60 FPS
  spacing. Backend synchronization produces short transition intervals when
  enabling and one 25.008 ms interval when disabling, followed by 16.640 ms.
  Seamless refresh-mode changes are not claimed.

These rendered checks used Linux/XWayland under Sway on the laptop display.
No native Wayland or native Windows timing result is claimed.

Four relevant checks pass on Linux and ASan/UBSan: graphics reports (including
the injected-clock regression), cameras, window presentation and engine timing.
The standalone sanitizer stall workload completes all 600 frames with leak
detection enabled and no findings when rerun outside the ptrace restriction.
Windows engine/check targets and independent pacing/car/pit SDK consumers
cross-build successfully. Builds emit no C compiler warnings. The normal Linux
SDK and relevant example binaries are refreshed with the corrected engine.
The refreshed normal car also completes its ten-second headless smoke with
matching physics counts. Its SDK archive exactly matches the measured candidate.

**Outcome: complete.** Keep the pacing fix, retain the current physics behavior,
and stop for user review and commit. The next checkpoint is agreeing the small
C game's concept and completion criteria; it has not begun.
