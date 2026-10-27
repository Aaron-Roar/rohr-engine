# Extreme mass/size drop review

This direct-C demo prepares the visual review for **Milestone 6, Physics Goal 1**.
It uses the existing solver with one substep and eight solver iterations. It
does not implement robustness fixes or establish physics acceptance.

## Fixture

| Body | Width × height | Stored mass | Initial center |
|---|---|---|---|
| Static floor | 1,000 × 0.1 | 0.0001 | (0, -0.05) |
| Tiny dynamic body | 0.01 × 0.01 | 0.0001 | (0, 0.005) |
| Large dynamic body | 100 × 100 | 10,000,000 | (offset, 1050.01) |

The large body's bottom starts 1,000 units above the tiny body's top. Positions
and dimensions use the engine's float representation; the requested coordinates
are rounded to representable values. Both dynamic bodies start at rest, receive
gravity `(0, -9.8)`, use automatic COM/inertia, and have unrestricted rotation.
All bodies have friction 0.8, restitution zero, and mutual collision response.
The floor's stored mass does not make it dynamic.

The three cases use large-body horizontal offsets of 0, +0.0025, and -0.0025.
The overview shows true scale, so the tiny body is smaller than a pixel. The
contact view is fixed on the initial support region and shows 0.08 × 0.05 world
units. Yellow is the tiny body, blue the large body, and gray the floor.

## Run and controls

The installed-SDK review executable is in the milestone's ignored sandbox:

```sh
nix develop -c sandbox/milestone_6/goal_1/bin/linux/extreme_mass_drop
```

| Key | Action |
|---|---|
| Space | Pause or run |
| N | Advance one update while paused |
| R | Restart the current case, paused |
| 1 / 2 / 3 | Restart centered / positive offset / negative offset |
| F1 / F2 / F3 | Restart at 30 / 60 / 120 updates per second |
| Escape | Exit |

The demo starts paused. Running updates retain elapsed simulation time; paused
wall time is excluded. The display and terminal show body positions, velocities,
angles, angular velocities, pair penetration depths, and the maximum depth since
restart. The first violation above 0.0001 world units stays flagged until restart;
it does not stop or alter motion. No settling timer or automatic freezing is used.

Fatal physics reports or nonfinite observations halt advancement and preserve
the last finite snapshot for display. Restart recreates the owned fixture.
Presentation/API failures report an error and exit. Exit destroys viewports,
screens, cameras, text, and fixture entities before engine shutdown.

## Observation and verification

Penetration is measured from current rectangle transforms using independent
double-precision separating-axis projections, including rotation and full
containment. Dimensions come from the actual stored float geometry. These are
post-update geometric measurements, not cached solver contact depths. The demo
checks that each observed update reports exactly one completed substep. Samples
can miss a crossing between updates and therefore do not prove no tunneling.

`--verify` checks the observer against analytic cases, validates the fixture's
geometry/materials/mass/motion/COM/inertia, and runs all nine case/rate combinations
for 20 simulated seconds each. It checks reset and finite state while recording
penetration violations as baseline findings. **A passing harness is not a passing
physics result.** Twenty seconds is an observation interval, not a settling limit.

`--smoke` uses SDL events to exercise all cases and rates, pause/run, single step,
restart, fatal-state recovery, all three camera callbacks, composition, and exit.
Dummy-SDL checks cannot establish real desktop visual correctness.

For an independent installed-SDK build, supply the SDK location and keep all
delivery artifacts in the same milestone sandbox:

```sh
nix develop -c cmake -S examples/extreme_mass_drop \
  -B sandbox/milestone_6/goal_1/automated/linux \
  -DCMAKE_PREFIX_PATH=/absolute/path/to/installed/sdk \
  -DROHR_EXAMPLE_OUTPUT_DIRECTORY="$PWD/sandbox/milestone_6/goal_1/bin/linux"
nix develop -c cmake --build sandbox/milestone_6/goal_1/automated/linux --parallel 4
nix develop -c ctest --test-dir sandbox/milestone_6/goal_1/automated/linux --output-on-failure
```

Use separate `automated/windows` and `bin/windows` directories with the Windows
toolchain and SDK. Use `automated/sanitizers` and `bin/sanitizers` with an
ASan/UBSan SDK and matching compiler/link flags. The demo uses the engine's
built-in font and primitive geometry; no assets, new dependencies, or source-tree
working directory are needed at runtime.

For a source build, configure the engine with `ROHR_BUILD_EXAMPLES=ON`, then
build the `extreme_mass_drop` target. Its in-tree default output is also the
milestone's `bin/linux` directory (`bin/windows` or `bin/sanitizers` for those
builds). `ROHR_EXAMPLE_OUTPUT_DIRECTORY` can select another sandbox destination;
the isolated source-build verification uses `automated/source` and `bin/source`.

## Review boundary

Review the centered and both offset cases, including the first updates and the
large-body arrival, using the overview, magnified view, stepping, and terminal
observations. Record existing failures separately from desired behavior. Agree
settling time, residual linear/angular motion limits, observation duration, and
measurement tolerances with the user before planning solver changes.

All five physics goals remain open. Stall/catch-up acceptance, player-floor
regressions, performance budgets, and the final robustness criteria remain in
the [roadmap](../../NEXT_STEPS.md#6-physics-robustness); this preparation does not
claim those gates passed.
