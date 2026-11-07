# Automatic rendering comparison

This installed-SDK consumer compares callback rendering with automatic discovery
on the same engine archive. It changes no physics settings. GNU-compatible linker
wrapping measures presentation boundaries; this is a diagnostic, not a new engine
pipeline or a general profiler.

Prepare a source snapshot of the last callback-based car, then build against the
SDK to be measured:

```sh
mkdir -p sandbox/rendering_callback
git archive 8934dc85 examples/soft-body examples/cmake | tar -x -C sandbox/rendering_callback
nix develop
cmake -S tools/automatic_rendering -B build/automatic_rendering \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$PWD/dist/linux" \
  -DROHR_CALLBACK_SOURCE_DIR="$PWD/sandbox/rendering_callback"
cmake --build build/automatic_rendering --parallel 4
```

Finish compilation, connect AC, close heavy workloads, and run each pair serially
three times, alternating pair order. Use stable, visible 1280×720 windows. Closing
a window interrupts that run; do not include it as a completed endpoint.

| Workload | Callback | Automatic | Endpoint |
| --- | --- | --- | --- |
| Three moving bars | `light` | `light --automatic` | 600 frames after one warm-up |
| Car | `car_callback --benchmark=rendered --frames=600` | `car_automatic --benchmark=rendered --frames=600` | 600 frames, 10 simulated seconds |
| 800-particle pit | `pit --benchmark=rendered --seconds=12` | `ROHR_AUTOMATIC=1 pit --benchmark=rendered --seconds=12` | 720 frames, 12 simulated seconds |
| Two viewports | `light --multiple` | `light --multiple --automatic` | 600 frames after one warm-up |

Commands in the table refer to executables under the build directory. Run them
from outside the source tree. The two-viewport fixture has two distinct screens
and another mount of the first screen, with different placement, rotation and
clipping. It checks that only two screens render. The pit uses the unchanged
physics fixture and identical persistent HUD controls on both paths; only camera
geometry submission differs. The car uses the pre-migration and current entry
points with their corresponding generated descriptions and appearance setup.

`light` also accepts `--stall` (200 ms at frame 120), `--vsync`, and `--toggle`
(switch VSync at frames 200 and 400). Exercise focus/window switching separately
from controlled comparisons. For NixOS, the generic Linux SDK's dynamically loaded
Wayland/X11, GL and audio libraries need their Nix library directories available
through `LD_LIBRARY_PATH`; the Nix SDK supplies its own runtime integration.

## Measurement boundaries

All samples are buffered until the application exits; no timed-frame CSV I/O or
physics profiling is added. Each `sample` row records:

- Time between starts of consecutive `SDL_RenderPresent` calls.
- Full show-call wall time minus reported software-limit wait and time spent
  inside `SDL_RenderPresent`.
- Process CPU consumed between show entry and presentation entry, including any
  renderer worker threads. This is separate from elapsed rendering work.
- Time inside `SDL_RenderPresent`, including any backend/VSync blocking.
- Reported software-limit wait, and full show-call wall time.

`startup_ms` is time from main entry to the first show, including initialization,
scene creation and the first physics step where applicable. Report the first
show separately. Summarize every subsequent sample with mean and nearest-rank
p95; retain maxima and raw samples rather than trimming outliers. Pit HUD text
mutation precedes show; its existing `render_total_ms` includes that work too.
Car and pit retain their existing complete-frame-work and physics summaries.

`render_samples failures=0` requires exactly one presentation per frame and no
graphics errors/diagnostics. Endpoint completion and acceptance are separate:
the pit can reach its full endpoint and still return failure for its existing
performance gate. The short car always reports an incomplete **60-second physics
scenario**; reaching 600 frames is sufficient only for this rendering diagnostic.
Keep the established physics failure inventory visible.

The integrated milestone results are in
[the verification report](../../docs/creation_rendering_verification.md).
