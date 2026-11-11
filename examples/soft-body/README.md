# Authored soft-body car

Open `project.rohr.json` in the editor to inspect the car's chassis, wheels,
soft tires, anchors, and joints. Source definitions live in
`objects/project.rohr.json`. Edit forces through Add Force and Force Properties;
use the CLI for torque definitions until the torque editor is available.

- **A/D:** drive the rear wheel by updating its persistent `rear_drive` torque.
- **Space:** jump with an upward one-tick force at the chassis COM. Both wheels
  must contact external rigid bodies; particle-pit and self-contacts do not count.
  Each press produces at most one jump, and airborne presses are discarded.
- **B:** toggle collision debugging.
- **Escape:** exit.

The source magnitude returns to zero when drive input is released. Jump force retains the magnitude reviewed at the selected 60 Hz physics tick; the existing collision slow-motion
sequence also slows the jump response. An authored zero-magnitude `initial_load`
demonstrates initial-source expiry without changing the car's starting motion.

Regenerate after editing the document:

```sh
rohr-cli --project examples/soft-body generate-c
```

`rohr_object_create(project.car)` creates independent entities on each call.
The game-owned `Car` record retains typed IDs resolved by runtime name after
creation. It is gameplay state, separate from the generated creation recipe.
Deleting the owning root deletes its members and remaining sources. Stored
appearance supplies the car's drawing, and `rohr_graphics_viewport_show()` renders
the scene automatically; no application camera callback is needed.
The normal scene keeps its existing walls, ramp, particle pit, and camera behavior
in developer-owned code. The default project template is unchanged.

Build this example independently with CMake against an installed Rohr SDK. Its
compiled catalog contains the authored data, so launching the executable does
not require the source directory or the JSON document.

For repeatable performance investigation, run `soft_body --benchmark=rendered`.
It uses the same authored car, terrain, 200 pit particles, 60 Hz ticks, four configured substeps, two particle-contact passes and four
other solver passes in the default engine. The 60-second script idles for two seconds, drives right
until 30 seconds, then left, requesting grounded jumps at 5 and 35 seconds.
The rendered run uses a 1280×720 window and a 60 FPS cap, with debug drawing and
collision slow motion disabled. Input cannot change the scripted controls or
debug setting; Escape or a window close interrupts the run.

`--benchmark=headless` isolates physics cost. Add `--debug` or `--slow-motion`
to measure those costs separately; slow motion retains the physics slowdown
without the interactive example's camera zoom sequence. `--frames=N` (1–3600)
limits exploratory runs. Short or interrupted runs print `complete=0` and never
qualify as real-time acceptance. The normal interactive invocation is unchanged.

Output includes physics CPU time, render/frame work excluding frame-cap waits,
simulated and elapsed time, schedule lag, completed substeps, collision work and
pipeline timings. A full normal rendered run exits unsuccessfully if p95 frame
work exceeds 16.67 ms or simulated time differs from elapsed time by over 1%.
Headless/debug/slow-motion results are diagnostic.
[Milestone 13](../../NEXT_STEPS.md#13-physics-rebuild-from-the-simpler-baseline)
retains its strict performance and numerical gates. Goal 2 missed those gates;
the user nevertheless selected its compact scalar implementation as the branch
default after accepting the car/pit behavior. See the
[measured comparison](../../docs/physics_compact_solver.md). The rejected wall
guard and SIMD experiments are not enabled.

The normal example and benchmark now use the tested 60 Hz configuration.
Scripted jump forces are explicitly owned for one physics tick, across all its
substeps, then deleted. Scripted physics does not advance the engine lifetime
clock; explicit cleanup prevents a jump force persisting through the remaining
benchmark. Interactive play continues to use normal engine-clock expiry.
Authored scene data, force magnitudes and the control script are unchanged.
Existing custom SDK builds must be reconfigured with the selected physics
options and rebuilt; the example cannot change its linked engine's build mode.

Add `--uncapped` for a separate throughput/headroom run; it cannot qualify as
normal rendered acceptance. Preserve prior evidence and label diagnostic
prefixes separately. Full acceptance still requires three complete rendered
runs on AC power with heavy workloads closed and visual review.
