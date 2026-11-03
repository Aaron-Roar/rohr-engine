# Extreme mass/size drop review

This direct-C demo reproduces the accepted fixture for **Milestone 6, Physics Goal 1**.
It uses one minimum substep, collision-aware subdivision, and eight solver
iterations. The user accepted the motion, window-switch behavior, and centered
settling limits; the engine regression enforces those limits.

## Fixture

| Body | Width × height | Stored mass | Initial center |
|---|---|---|---|
| Static floor | 0.01 × 0.1 | 0.0001 | (0, -0.05) |
| Tiny dynamic body | 0.01 × 0.01 | 0.0001 | (0, 0.005) |
| Large dynamic body | 100 × 100 | 10,000,000 | (offset, 1050.01) |

The large body's bottom starts 1,000 units above the tiny body's top. Positions
and dimensions use the engine's float representation; the requested coordinates
are rounded to representable values. Both dynamic bodies start at rest, receive
gravity `(0, -9.8)`, use automatic COM/inertia, and have unrestricted rotation.
All bodies have friction 0.8, restitution zero, and mutual collision response.
The floor's stored mass does not make it dynamic.

The original three cases use large-body horizontal offsets of 0, +0.0025, and
-0.0025. The user also requested tipping comparisons at +0.01 and -0.01. These
put the large body's COM beyond the tiny support's initial ±0.005 footprint.
The user revised the floor width from 1,000 to 0.01 in **all five cases**, matching
the yellow body's width. The overhang now has room to tip without meeting a
broad floor. The original offsets can balance without tipping; cases 4 and 5
must rotate at least 1° toward the overhang within the 30-second observation.
Watch the LARGE angle readout after impact (about 14.3 seconds). Neither body is
locked, so subsequent sliding, rolling, and falling off the narrow floor remain
possible. The centered stack must remain supported.
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
| 1 / 2 / 3 | Restart centered / +0.0025 / -0.0025 offset |
| 4 / 5 | Restart +0.01 / -0.01 tipping comparison (same narrow floor) |
| F1 / F2 / F3 | Restart at 30 / 60 / 120 updates per second |
| Escape | Exit |

The demo starts paused. Running updates retain elapsed simulation time; paused
wall time is excluded. The display and terminal show body positions, velocities,
angles, angular velocities, pair penetration depths, and the maximum depth since
restart. The first violation above 0.1 world units stays flagged until restart;
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
checks that each observed update reports completed substeps. Display samples
can miss a crossing between updates and therefore do not prove no tunneling.

`--verify` checks the observer against analytic cases, validates the fixture's
geometry/materials/mass/motion/COM/inertia, and runs all fifteen case/rate combinations
for 20 simulated seconds each. It checks reset and finite state while recording
penetration violations as baseline findings. **A passing harness is not a passing
physics result.** Twenty seconds is an observation interval, not a settling limit.

`--smoke` uses SDL events to exercise all cases and rates, pause/run, single step,
restart, fatal-state recovery, all three camera callbacks, composition, and exit.
Dummy-SDL checks cannot establish real desktop visual correctness.

The separate `extreme_mass_drop_test` engine regression observes **every completed
substep** through an internal synchronous observer. It runs all fifteen combinations
and five 60 Hz stall cases for 30 simulated seconds, checks the 0.1 penetration
limit, centered support, finite state, fixed floor, energy growth, actual report
counts and elapsed-time consumption, then verifies the giant can leave support
and rotate. It rejects sideways support correction jumps exceeding integrated
motion plus one tiny-body width, and requires the new comparisons to rotate at
least 1° toward the overhang after contact. Offset bodies may fall below the
floor's top after moving off its finite footprint; actual overlaps still obey
the penetration limit. No final resting angle is imposed.
The 100 ms stall occurs after 14 seconds and the 1 s stall after
14.15 seconds, crossing impact. The energy guard allows 0.1% above initial energy;
elapsed-time accounting tolerance is 1e-9 seconds. Thirty seconds is a collision
observation interval. For the centered case, the user-approved requirement is
settling within 2 seconds after impact, followed by 10 seconds of support with
each body's linear speed ≤0.0001 units/s and angular speed ≤0.0001 degrees/s.
The regression checks every completed substep in that hold, including both
endpoints of the substep crossing the settling deadline and the endpoint
crossing the hold's end. Positive support gaps retain a separate 0.0001-unit
separation limit, so a stationary but unsupported body cannot pass. The centered
support-position guards also remain unchanged. The 0.1-unit penetration limit
is a provisional review threshold requested during milestone 12; the original
0.0001-unit penetration criterion remains recorded in historical evidence.
Neither threshold changes solver correction settings. It reports hold
duration, sample count, and peak speeds. Residual motion after 15 seconds is
also recorded for all cases; offset motion has no settling requirement.

`extreme_mass_drop_test --performance` additionally checks the roadmap's 2 ms
normal 60 Hz p95 and 50 ms delayed-update limits. Use a Release build on the
reference laptop on AC. Measurements use process CPU time around the complete
physics update, including the independent substep observer, excluding setup,
rendering, and printing. Each run reports update sample and substep counts and
records timings separately for delayed updates.

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
the current isolated Release verification uses `automated/source` and `bin/release`.

## Review boundary

The user approved fixing support and collision before settling review, revised
the floor width in all five cases, and accepted the corrected tipping and
window-switch behavior. They then accepted the centered settling limits above.
The observation harness is a visual aid; completion evidence comes from the
substep regression, regression/sanitizer suites, platform builds, performance
measurements, and that actual desktop review. See the
[roadmap](../../NEXT_STEPS.md#6-physics-robustness) for goal status and later goals.
