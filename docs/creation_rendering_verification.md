# Creation and automatic-rendering verification

Milestone 15 Goal 6 integrates the descriptor, generated-project and automatic
viewport workflow. Physics settings remain unchanged; historical failures are
retained, with the obsolete forces timing assertion corrected below.
**Goal 6 is complete.** The user accepted the generated
application's independent scenes, inset screens and runtime-added toggle.

## Integration corrections

- Portable Linux SDK archives now compile as position-independent code. The
  Rocky Linux build previously succeeded locally but its SDL archive failed to
  link into a default-PIE consumer on the host toolchain. Installed-SDK tests now
  request PIE explicitly on Linux, so packaging also exercises that combination.
- The installed automatic-viewport consumer now includes the shared example
  helper used by its control tests.
- Automatic body/particle rendering omits outlines with zero alpha. The initial
  controlled comparison found that these invisible commands consumed useful
  frame budget. Changing alpha back to any nonzero value restores the outline;
  a regression check covers both transitions across shared screens.
- By explicit user approval, the forces test now permits the adopted fixed-step
  schedule as well as legacy adaptive refinement. Its minimum completed-substep
  check is `>= configured` instead of `> configured`; every force/torque numerical
  check is unchanged. This removes the obsolete packaging blocker without a
  physics implementation or settings change.

## Lifecycle and workflow

The new [generated workflow consumer](../tests/installed_sdk_consumer/generated_workflow.c)
uses the checked-in center-of-mass project's `project.h`. It creates two
independent object instances, explicitly binds each camera, changes only one
instance's appearance, and adds a rotated, clipped inset and a button after
viewport creation. Another viewport mounts the inset's screen; the test checks
three screen renders and one presentation for the batch. A separately created
screen survives removal of its mount. Viewport-owned screens and UI become stale
on destruction; borrowed cameras remain live until their objects are deleted.
A directly created member survives deletion of both object roots.

Each of 1,000 cycles creates, uses, extends and destroys the entire arrangement.
Existing automatic-viewport tests additionally check live font/text resources,
rollback, allocation failures, visibility/layers, stale cameras, restart, direct
and public reporting, invalid selections and callback recursion. Descriptor
creation checks cover dependencies, ownership, failed creation and independent
instances. The preview mode lets the user operate the runtime-added toggle.

The generic SDK is built using the cached Rocky Linux builder. Its relocated CLI
creates a fresh starter without the engine source mounted. Generated and
handwritten consumers then build with the host Linux toolchain against that
installed SDK. All thirteen bundled examples are configured independently and
launched from `/tmp`, with staged assets beside their executables.

Initial full packaging stopped at the previously recorded `rohr_forces` failure,
`tests/forces.c:95: completed > substeps`. That historical failure remains in the
logs. After the user-approved timing-contract correction, the unmodified packaging
script passes its full consumer gate and creates `rohr-sdk-linux-x86_64.tar.gz`.
All fourteen consumers also pass against the generic SDK with the host toolchain.
No consumer is excluded from the final gate. Other known physics failures remain
outside this milestone's acceptance and are not claimed resolved.

Final verification:

- Twenty focused Release and twenty ASan/UBSan checks pass: creation, ownership,
  automatic rendering, reports/layers, cameras/window presentation, authored
  examples, assets, timing, deterministic components and the corrected forces
  contract. Asset/timing executables are relinked to the final engine.
- All fourteen installed generic Linux consumers pass. Fourteen sanitizer SDK
  consumers pass across the thirteen-consumer run and the corrected forces run,
  including 1,000 complete generated-object/viewport lifecycle cycles. No
  sanitizer memory or undefined-behavior findings occurred.
- All thirteen independently configured examples build and launch from `/tmp`
  against the generic SDK. Its CLI creates a fresh starter without engine-source
  access; that starter compiles and launches outside its source directory.
- Updated SDK consumers, affected engine tests, the extended frame-pacing fixture
  and all four comparison executables cross-build for Windows. Goal 5's thirteen
  Windows example cross-builds remain the unchanged example-source baseline.
  Native Windows execution and timing are unverified.
- The generated workflow preview exits cleanly and the user accepts its behavior.
  The only example warning under the extra verification flags is the already
  recorded unused helper in Pong's generated component code.

## Controlled rendering comparison

The [comparison tool](../tools/automatic_rendering/README.md) links both paths to
the same generic SDK. AC power and closed heavy workloads were confirmed by the
user. Builds finish before serial mapped-window runs at 1280×720. Each path runs
three complete repetitions, alternating pair order, for moving bars (600 frames),
car (600 frames), 800-particle pit (720 frames), and two viewports (600 frames).
The latter mounts a shared screen alongside a distinct screen. First presentation
and startup are separate from subsequent-frame means and nearest-rank p95s.

The car comparison uses callback-era source `8934dc85` and the migrated source
on one current engine archive. The pit reuses the same physics fixture and uses
identical persistent HUD text in both modes. Its callback constructs screen-space
geometry from validated poses; automatic mode projects retained runtime visuals.
The tests preserve geometry, appearance, camera selection and physics counts.

Before the transparent-outline correction, automatic car drawing used about
1.28 ms versus 0.98 ms of work, excluding backend presentation and limiter waits.
Automatic pit drawing used about 4.4 ms versus 3.0 ms. All three automatic pit
runs reached the endpoint but failed their real-time gate (about 59.05–59.47 FPS).
These are retained failures, not discarded warm-ups or incomplete simulations.
The extra diagnostic first attempt also reached the endpoint and failed the gate.

An initial graphical launch could not locate the generic SDK's dynamically loaded
video libraries on NixOS and produced zero frames. Subsequent runs explicitly
expose the development environment's graphics/audio library directories through
`LD_LIBRARY_PATH`. That failed launch is excluded from timing; ordinary generic
Linux loader paths and Nix runtime integration are different environments.

### Corrected measurements

Numbers below are the median of three run means / median of three run p95s, in
milliseconds. Rendering work excludes software-limit waits and backend present
time. CPU is process time before presentation, including renderer worker threads.

| Workload | Callback work avg / p95 | Automatic work avg / p95 | Callback CPU avg / p95 | Automatic CPU avg / p95 |
| --- | ---: | ---: | ---: | ---: |
| Moving bars | 0.180 / 0.230 | 0.192 / 0.255 | 0.201 / 0.294 | 0.219 / 0.322 |
| Car | 0.987 / 1.081 | 1.045 / 1.124 | 0.998 / 1.099 | 1.056 / 1.151 |
| 800-particle pit | 2.921 / 3.014 | 3.257 / 3.385 | 2.914 / 3.007 | 3.248 / 3.376 |
| Two viewports | 0.185 / 0.220 | 0.197 / 0.227 | 0.198 / 0.269 | 0.210 / 0.295 |

The correction reduces automatic pit drawing work by about 1.16 ms (26%) versus
the initial automatic version. Remaining overhead versus callbacks is about
0.06 ms for the car and 0.34 ms for the pit. This preserves the existing
within-budget pacing behavior without changing physics or relaxing performance
gates; it does not establish a general renderer speedup.

| Workload / path | Software wait avg / p95 | Backend present avg / p95 | Present interval avg / p95 |
| --- | ---: | ---: | ---: |
| Bars / callback | 16.284 / 16.401 | 0.185 / 0.300 | 16.668 / 16.669 |
| Bars / automatic | 16.228 / 16.378 | 0.219 / 0.341 | 16.668 / 16.669 |
| Car / callback | 10.079 / 12.596 | 0.176 / 0.287 | 16.668 / 16.668 |
| Car / automatic | 10.091 / 12.497 | 0.134 / 0.284 | 16.668 / 16.669 |
| Pit / callback | 4.101 / 11.644 | 0.219 / 0.372 | 16.684 / 16.733 |
| Pit / automatic | 3.768 / 11.389 | 0.209 / 0.363 | 16.686 / 16.732 |
| Two views / callback | 16.251 / 16.372 | 0.186 / 0.338 | 16.668 / 16.669 |
| Two views / automatic | 16.211 / 16.362 | 0.240 / 0.362 | 16.668 / 16.669 |

Car FPS is 60.061–60.062 for callbacks and 60.059–60.062 for automatic rendering.
Pit FPS is 59.972–59.992 versus 59.937–59.976; all six pit runs pass the existing
real-time gate. Automatic pit complete-frame work p95 is 16.240–16.417 ms, so the
heaviest fixture still has little spare frame budget. Final pit lag is
4.74–12.65 ms across the whole 12-second automatic run, versus 107.60–193.13 ms
in the initial three automatic runs. Bars and two-view compositions retain
approximately 60 FPS. All 24 corrected comparison endpoints complete and report
one presentation per frame with no graphics diagnostics.

Every car run preserves 2,400 substeps, 701,387 candidates and 108,585 contacts;
every pit run preserves 2,880 substeps, 4,283,479 candidates and 152,816 contacts.
The pit's recorded maximum floor-circle depth remains 0.063147306 with zero side
escapes. These observations do not resolve the broader known physics failures.
The 10-second car remains a rendering diagnostic, not acceptance of its full
60-second physics scenario.

Startup includes initialization, scene creation and the first physics update
where applicable. First-show cost includes its own waits. Medians are below;
startup ranges across the three runs are in parentheses. These are separate from
the subsequent-frame tables and are not isolated creator microbenchmarks.

| Workload | Callback startup ms | Automatic startup ms | First show ms, callback / automatic |
| --- | ---: | ---: | ---: |
| Bars | 79.17 (74.82–184.55) | 80.08 (76.31–83.71) | 17.61 / 18.02 |
| Car | 183.47 (174.79–185.85) | 173.90 (170.75–178.95) | 5.26 / 5.68 |
| Pit | 582.80 (562.48–597.24) | 592.41 (581.86–601.20) | 8.01 / 8.07 |
| Two views | 90.17 (84.47–91.07) | 86.70 (83.95–92.63) | 17.40 / 17.30 |

### Pacing and review

The automatic 600-frame stall/focus test completes: the intentional 200 ms delay
produces a 200.77 ms interval with no limiter wait; the next intervals return to
16.74 and 16.58 ms without catch-up bursts. Focus leaves and returns successfully.
With VSync enabled on this 120 Hz output, the run reaches 119.46 FPS, has zero
software-limit wait and averages 8.11 ms inside presentation. Toggling VSync at
frames 200 and 400 produces transient 0.81 and 25.26 ms intervals, then recovers
to the selected cadence. Those transition irregularities remain visible, as in
the prior pacing report. The sanitized automatic two-viewport stall test passes.

The user accepts the actual generated application, including independent scene
instances, differently placed/rotated screens and the runtime-added button. The
milestone is complete; game concept/scope selection remains the next user-directed
checkpoint, and further physics development remains paused.

## Reproduction and evidence

Local evidence is under `sandbox/milestone_15/goal_6/`: `build_checks.py`,
`extra_checks.py`, `final_builds.py`, independent example launch scripts,
`render.py`, `render_sequence.py`, `with_runtime.py`, `summarize.py`, and logs.
`before_border_fix/` retains the initial comparison, SDK and executables.
Each rendered run records its command, executable hash, endpoint, exit status,
window geometry, output configuration and raw timing samples. The final run set
uses a separate directory, preserving the original regression evidence.
The final packaging rebuild changes archive header timestamps; relinking all
four benchmark executables against it produces byte-identical executables to
those measured (`archive_executable_parity.json`). Disposable sanitizer/Windows
consumer build directories are removed after verification; SDKs, scripts and
logs remain. The packaged SDK is isolated under this evidence directory; the
user's `dist/` installation is not overwritten.

For the interactive workflow, run the installed-SDK consumer executable
`rohr_generated_workflow --preview`. For a normal game, follow
[the game workflow](../examples/reusable_instances.md): generate `project.h`,
create an object, bind the chosen camera instance, extend the viewport as needed,
update runtime state and call `rohr_graphics_viewport_show()` or its batch variant.
Developer-edited entry points remain preserved. Camera named-layer filters remain
future work; named-layer identity and numeric ordering are already separate.
