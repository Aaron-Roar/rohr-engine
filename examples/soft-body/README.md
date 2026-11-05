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

The source magnitude returns to zero when drive input is released. Jump force is
tuned for the normal 1/240-second physics tick; the existing collision slow-motion
sequence also slows the jump response. An authored zero-magnitude `initial_load`
demonstrates initial-source expiry without changing the car's starting motion.

Regenerate after editing the document:

```sh
rohr-cli --project examples/soft-body generate-c
```

`car_create()` uses the shared object spawner and returns independent entities on
each call. `car_destroy()` deletes the owning instance and its remaining sources.
The normal scene keeps its existing walls, ramp, particle pit, and camera behavior
in developer-owned code. The default project template is unchanged.

Build this example independently with CMake against an installed Rohr SDK. Its
compiled catalog contains the authored data, so launching the executable does
not require the source directory or the JSON document.

For repeatable performance investigation, run `soft_body --benchmark=rendered`.
It uses the same authored car, terrain, 200 pit particles, 240 Hz ticks and four
configured substeps. The 60-second script idles for two seconds, drives right
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
[Milestone 13](../../NEXT_STEPS.md#13-physics-rebuild-from-the-simpler-baseline) additionally
requires sustained 60 FPS without accumulating backlog, three serial repetitions
per workload and engine version on AC power, with streaming applications closed
and no concurrent builds, plus visual/window-switch review. Report uncapped
throughput/headroom separately; one successful process does not establish
milestone completion. The same rendered gates also apply to the unchanged
800-particle pit's 12-second sequence.

[Milestone 13](../../NEXT_STEPS.md#13-physics-rebuild-from-the-simpler-baseline)
is now the active roadmap, with all implementation goals unstarted. Milestones
11 and 12 are superseded, not complete, and their evidence remains preserved.
The rebuild uses the historical `a8a7940a` execution model inside the current
engine, retaining `6f283268` as the frozen accuracy reference and excluding
`main` from comparisons. Its staged runtime candidates stay at 60 Hz or below;
this roadmap update does not change this example's existing 240 Hz settings or
command-line behavior. Preserve authored scene data and input sequences, and
label diagnostic prefixes/no-jump historical comparisons separately. Keep new
milestone evidence under `sandbox/milestone_13/goal_<n>/`. Milestone 12 Goal 1's
[measurement contract](../../docs/physics_measurement_contract.md) defines the
reference and diagnostic consumers. Add `--uncapped` to a rendered benchmark
for a separate throughput/headroom run; it cannot qualify as normal rendered
acceptance. Fatal benchmark failures now print an explicit incomplete summary.
