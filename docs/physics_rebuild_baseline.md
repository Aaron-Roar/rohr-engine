# Physics rebuild baseline

Milestone 13 Goal 1 evaluates the historical execution schedule inside the
current engine. It is an experimental baseline, not a production selection.
**Outcome: Complete.** The declared diagnostic endpoints and mandatory safety
verification are satisfied; numerical quality and rendered/headroom failures
remain comparison results under this goal's approved contract.
The authoritative scope and later goals are in [the roadmap](../NEXT_STEPS.md#13-physics-rebuild-from-the-simpler-baseline).

## Provenance and reproduction

- Immediate parent: `f3e385fc850c8a9b8f2172a306094e180d9c6cc2`.
- Historical execution model: `a8a7940a6e6d039c80aed20c6f6b2624169e1ee3`.
- Current comparison checkpoint: `0b254c045b806fb13c022cd7470f063a03a130db`.
  Physics implementation matches the immediate parent; intervening changes
  document the comparison and new roadmap.
- Frozen accuracy reference: `6f283268aa4bf038c4ed78271baeb9d5986bd7d9`.
  `main` is excluded. Existing reference SDKs, binaries, sources and evidence
  remain in their Milestone 11/12 locations.

New artifacts live under `sandbox/milestone_13/goal_1/`: `provenance.json`,
`source/parent.tar`, separate `sdk_control` and `sdk_baseline` directories,
independent consumer builds, scripts, logs and individual trial records.
The provenance inventory pins retained historical evidence by path and hash;
it does not rebuild into or overwrite the frozen SDKs.

Configure the engine with `-DROHR_PHYSICS_SIMPLE_BASELINE=ON` to build the
candidate. This private compile definition is OFF by default and is not a new
runtime API. The installed SDK contains the selected implementation without
requiring client compile flags. Existing runtime APIs still select tick duration,
configured substeps and solver iterations. Baseline measurements use 60 Hz,
four configured substeps and four solver passes.
The option applies to complete updates and complete-substep entry points;
calling individual manual stages does not activate the bounded schedule.

`build.py baseline` builds the engine with CMake, copies its archive into a
separate SDK and builds independent consumers. `build.py control` uses the
preserved control archive. Optional `sanitizers` and `windows` arguments select
the corresponding builds. Commands run inside `nix develop`; exact compiler,
configuration and link commands are retained in the build logs and caches.

`run_one.py <variant> <car|pit> <clean|trace|oracle|rendered> <unique_label>`
runs exactly one declared workload from `/tmp`. Car means the ten-second prefix
of the unchanged current authored scene with 200 particles and jump requests;
pit means the complete twelve-second 800-particle fixture. Each execution has
a 59-second interrupt / 60-second kill deadline, checks AC power and preserves
incomplete evidence. No automatic replacement visual windows are launched.

The historical example remains a separate behavioral reference: older authored
content/control implementation, no jump, older boundary representation and older
physics kernels. Its previously measured 7.178-second car median is not a matched
modern-scene performance result. No historical pit parity is asserted.

## Execution and feature inventory

| System | Candidate behavior | Reduced protection or deferred work |
| --- | --- | --- |
| Update and elapsed time | Consume the full requested time in exactly the configured number of steps; preserve attempted/completed reports and observer durations | No speed-, joint-, elastic- or collision-triggered additional subdivision |
| Force preparation | Current acceleration clearing, gravity, external source ownership, spring/beam forces and soft-body frame updates | No removal or retiming of force ownership |
| Integration | Ordinary current semi-implicit rigid/node integration of prepared loads, with finite-state and coordinate checks | Complete substeps bypass special joint/elastic integration and swept duration selection |
| Rigid/particle gathering | Current broadphase, filtering and geometry, once after integration per step | Correction-created pairs wait until the next configured step |
| Joints | Current anchor validation, pin/weld assembly, response and transform locks | No special analytical joint motion or joint-driven duration limit |
| Springs | Current spring forces and constraints | No adaptive spring stability protection; stiff systems and long delayed updates can become inaccurate |
| Soft boundaries | Current independent beams, node–rigid and beam–rigid response | No mid-solve global rediscovery after rigid corrections |
| Beam/self contacts | Current beam-pair eligibility, filtering, gather and response remain present | Newly touching beams may wait for the next step; fast crossing protection is deferred |
| Solver | Exactly the configured rounds, historical position fractions `1 / remaining_rounds`; current friction, restitution, block response and per-contact geometry refresh | No early convergence exit; no global regather between rounds |
| Interaction publication | Current overlap/contact finalization, handle orientation and generation checks | Gather-once can delay newly created interactions; it does not claim continuous collision detection |
| Manual pipelines | Existing explicit preparation, integration, gathering and solving contracts; manual solves retain current rediscovery | The bounded policy is scoped to complete substeps and released before observer callbacks and on failure |
| Safety and compatibility | Current APIs, ownership, validation, entity generations, reporting, editor/project integration | No repository rollback or silent removal of supported component types |

The historical order is clear/apply forces → integrate → gather contacts →
gather joints → repeated contact/joint solve → finalize. The candidate restores
that schedule using modern kernels and checks. It is intentionally not a copy of
historical engine internals. The control still selects adaptive durations,
uses special joint/elastic motion, and regathers inside constraint rounds.

## Diagnostic corrections and interpretation

Direct scripted physics ticks do not advance the engine's wall-clock lifetime
system. The diagnostic harness now explicitly owns accepted one-tick jump force
sources across all configured substeps and deletes them after that tick. It does
not repeatedly apply them because the wall clock stands still. A dedicated check
verifies one tick of acceleration followed by unchanged velocity on the next
tick. Interactive execution retains the engine's normal lifetime behavior.
Historical and earlier faulty harness traces remain preserved and identified.

The car harness declares its requested ten-second endpoint explicitly rather
than labelling every run shorter than the production sixty-second sequence
incomplete. This does not shorten or satisfy the final sixty-second acceptance
test. Completion and numerical/performance verdicts are separate.

Clean measurements time only physics CPU work. Trace measurements use nested
exclusive wall-time accounting; parent inclusive times must not be added to
their children. Oracle runs separately observe every completed step, track
finite state, particle overlap, beam stretch and motion, and keep their overhead
out of clean timings. State hashes compare instrumented trajectories within a
version; they do not establish cross-version physical accuracy.

The pit's particle-pair review limit remains 0.5 units, and extreme-fixture
penetration review remains 0.1. Neither changes the solver's correction slop.
Missed contacts, tunneling, rebound, energy and joint errors remain separate
criteria. Numerical failures are retained to full endpoints unless invalid
state, fatal errors, inability to advance, interruption or the wall cap prevents
completion.

## Measured workloads

Three serial clean runs per workload/version, AC checked, streaming last confirmed closed, no concurrent builds. All twelve clean runs and eight trace/oracle runs completed; no fatal/nonfinite state, report loss or time-accounting mismatch was observed. These are diagnostic CPU results, not final rendered acceptance.

| Workload | Control CPU seconds (three runs) | Baseline CPU seconds (three runs) | Median speedup | Baseline throughput | 2× budget |
| --- | --- | --- | --- | --- | --- |
| car | 49.455, 48.175, 51.855 | 2.700, 2.912, 2.919 | 16.99× | 3.43× real time | 5 s |
| pit | 23.220, 25.030, 25.091 | 12.755, 13.317, 12.271 | 1.96× | 0.94× real time | 6 s |

Car passes the diagnostic physics headroom budget; pit needs another **2.13× speedup** (about **53% less CPU work**) to meet its six-second budget. This is not acceptance of the candidate's numerical quality.

### Exclusive stage wall time

Milliseconds over each complete instrumented workload; one profiled run each. Instrumentation adds overhead, particularly to cheap particle calls, so use clean CPU medians for speedup. Every trace closes its accounting exactly; trace/oracle state hashes match at every update within each version.

| Stage | Control car | Baseline car | Control pit | Baseline pit |
| --- | ---: | ---: | ---: | ---: |
| force preparation | 864.2 | 82.8 | 152.4 | 93.0 |
| integration | 1866.7 | 121.4 | 341.9 | 197.6 |
| subdivision sweeps | 10314.2 | 0.0 | 4202.9 | 0.0 |
| contact gathering | 22226.2 | 998.2 | 7809.4 | 2829.3 |
| joint gathering | 158.5 | 14.7 | 30.2 | 18.2 |
| rigid solving | 13797.1 | 1126.4 | 12832.6 | 10366.0 |
| joint solving | 2389.1 | 221.6 | 147.8 | 85.2 |
| soft solving | 923.9 | 111.5 | 0.0 | 0.0 |
| finalization | 601.3 | 271.1 | 1596.1 | 1288.4 |
| reporting | 525.4 | 42.8 | 111.1 | 60.6 |
| orchestration | 1011.1 | 104.4 | 265.3 | 169.5 |

### Operation counts

| Work | Control car | Baseline car | Control pit | Baseline pit |
| --- | ---: | ---: | ---: | ---: |
| completed steps | 25,099 | 2,400 | 4,778 | 2,880 |
| rigid gathers | 225,878 | 2,400 | 22,039 | 2,880 |
| soft gathers | 125,495 | 2,400 | 4,778 | 2,880 |
| contact solver rounds | 100,396 | 9,600 | 19,112 | 11,520 |
| joint solver rounds | 100,396 | 9,600 | 19,112 | 11,520 |
| reported candidates | 9,808,179 | 659,843 | 6,825,238 | 4,275,022 |
| reported contacts | 870,703 | 99,243 | 241,318 | 153,047 |

Reported candidate/contact work counts are accumulated events, not unique pairs. Response-call counts include retained constraints whose current geometry may no longer need a response. Counts are taken at execution, not again during report merging. Shared broadphase/traversal cost is reported in gathering rather than arbitrarily apportioned to pair types.

### Pair-type response costs

Exclusive milliseconds, with response invocation counts in parentheses. Block-solver and common dispatch overhead remain in the rigid-solving stage total; these rows must not be added to that total.

| Pair type | Control car | Baseline car | Control pit | Baseline pit |
| --- | ---: | ---: | ---: | ---: |
| particle particle | 5795.0 (15,146,881) | 427.8 (1,133,060) | 9623.9 (19,689,142) | 8033.5 (14,577,780) |
| node particle | 2.0 (4,389) | 0.3 (1,388) | 0.0 (0) | 0.0 (0) |
| rotating circle | 0.0 (0) | 0.0 (0) | 0.0 (0) | 0.0 (0) |
| particle static | 2148.8 (2,090,299) | 288.2 (219,612) | 1371.1 (829,026) | 1000.0 (534,884) |
| particle moving | 191.9 (104,375) | 86.0 (61,484) | 0.0 (0) | 0.0 (0) |
| polygon | 288.2 (376,740) | 31.2 (40,572) | 16.9 (32,656) | 8.5 (18,088) |
| overlap only | 1597.6 (2,538,973) | 147.1 (245,640) | 0.0 (0) | 0.0 (0) |

The baseline pit spends 8,033.5 ms on 14,577,780 particle-pair response invocations.
This is the largest remaining measured cost. Following baseline review, the user
prioritized pit optimization as Goal 2, before blocking restoration (now Goal 3).
Separate redundant visits from effective responses, then evaluate fewer searches
and response visits, bounded reuse and sleeping with correct wake-up. The target
is six physics CPU seconds per twelve-second pit, retaining the 0.5-unit overlap
review limit, separate tunneling/wall diagnostics and an unchanged car comparison.
The [roadmap](../NEXT_STEPS.md#13-physics-rebuild-from-the-simpler-baseline) defines
verification and stop conditions. These are planned investigations, not measured
savings or a guarantee that restored protections will fit the resulting budget.

### Workload behavior

| Full-run diagnostic | Control | Baseline | Interpretation |
| --- | ---: | ---: | --- |
| Pit worst particle-pair penetration | 0.101639 | 0.303774 | Both below the 0.5 review limit |
| Pit floor-circle depth | 0.045247 | 0.134687 | Record separately; no generic pit wall limit was invented |
| Pit escape samples | 0 | 0 | Neither run escaped the pit |
| Car worst standalone particle-pair penetration | 0.098565 | 5.992004 | Major degradation; near-full overlap of diameter-six car particles |
| Car worst pair penetration involving a node | 0.000964 | 1.289857 | Major contact-precision degradation |
| Car maximum absolute beam length error | 13.962458 | 9.826794 | Both have substantial transient deformation; this metric did not worsen |
| Car maximum relative beam length error | 1.132047 | 1.049554 | Maximum absolute length error divided by that beam's rest length |

The car's final node centroid differs: control `(602.709, -259.617)`, baseline `(704.840, -282.290)`. These are different trajectories. The five-second jump request was not accepted in either ten-second run; explicit temporary-force expiry is exercised separately by the harness regression. Old no-jump traces remain distinct.

## Error inventory

The original stress schedules remain unchanged, including their one-substep
cases, higher diagnostic rates and delayed updates. They are not all the
60 Hz/four-substep car/pit configuration. All **228 declared stress endpoints
and calibrations** finish; **52 pass and 176 fail numerically**. Full logs and
per-case verdicts are in `runs/baseline_fixtures/fixtures.json`. CPU budgets were
not enabled as acceptance gates. No threshold was loosened.

| Fixture | Pass / total | Representative full-run failure and excess | Supported explanation |
| --- | ---: | --- | --- |
| Particle head-on | 1 / 5 | Four missed collisions/order crossings; rebound error 1,500 units/s despite measured endpoint overlap of zero | Discrete endpoints can completely skip a collision; overlap alone is insufficient |
| Thin barriers and rebound calibrations | 20 / 124 | Worst signed barrier violation 22,500.0227 versus 0.1, excess 22,499.9227 | No swept protection; particles and polygons can cross a barrier between fixed steps |
| Extreme mass drop | 13 / 20 | Settling speed up to 257.737701 versus 0.0001, excess 257.737601; missed delayed impacts | Small sampled contact depth does not prove sustained support or prevent tunneling |
| Persistent stack | 0 / 4 | Depth 2.049881 versus 0.1, excess 1.949881; post-motion speed/angular drift failures also retained | Coarse motion and no discovery of correction-created pairs degrade support |
| Pin stability | 4 / 8 | Relative energy error 0.999543 versus 0.01, excess 0.989543 | Ordinary integration plus constraint projection loses high-rotation energy without the special joint integrator |
| Weld stability | 1 / 8 | Swept barrier violation 28,227.0913 versus 0.1; energy error 0.140439 versus 0.01; momentum error 0.971057 versus 0.01 | Coarse motion misses impacts; constraint correction does not reproduce protected joint motion |
| Spring stability and calibrations | 1 / 11 | Energy error 554.950065 versus 0.01, excess 554.940065; position-reference error up to 26,188.7137 | Explicit elastic integration is unstable at some original delayed/stiff schedules; collision subdivision no longer limits it |
| Soft-body stability | 12 / 48 | Depth 989.524292 versus 0.1, excess 989.424292; energy error 635,124.172 versus 0.01 | Missing continuous/refresh protection and coarse elastic stepping; per-case tunneling and required-contact failures are retained |

Large signed barrier violations above measure travel through/beyond the barrier,
not a physically overlapping thickness of that many units. The logs preserve
separate contact, crossing, energy, reference-motion and support criteria.
Some explanations involve several deliberately reduced systems; this combined
baseline does not isolate each one's individual contribution.

The separate fifty-case thick-wall diagnostic uses four substeps/four passes.
Its 26 cases at 1,000 units/s or initially embedded pass; all 24 cases at
10,000 units/s fail by crossing a ten-unit-thick wall without a contact. This
covers standalone particles and nodes, static/moving/rotating walls, oblique
motion and delayed updates. Maximum signed violation is 985.500209 versus 0.1.
This is not proof that every 1,000-units/s phase or configuration is protected.

A further head-on replay at four substeps/four passes completes all five cases:
normal 60 Hz passes with worst overlap 0.00022745, and the 120 Hz diagnostic and
one delayed 60 Hz case pass. The 30 Hz and other delayed-phase case miss the
collision. The original one-substep stress failures above remain unchanged.

Original numerical regressions remain failed: `particle_head_on`,
`particle_sweep`, `joints`, `soft_body`, and `particle_blocking`. The older
`joints` and sweep tests abort/exit at their first numerical failure, and the
soft-body test stops at the missing correction-created beam contact. Preserved
diagnostic copies collect those numerical failures to their endpoints, return
failure, and then exercise the remaining API/lifecycle checks. Safety, setup and
finite-state checks remain active. They do not replace the original verdicts.

The independent joint replay reaches all three two-second endpoints. Pin spin
error peaks at 665.479128 degrees/s versus 0.1, and speed error at 42.278534
versus 0.02; weld peaks are 614.337221 and 16.083322 respectively. Anchor error
remains below 0.001. The small spring replay passes. The soft-body replay finds
the node contact but misses the newly created beam contact, then completes the
remaining lifecycle checks. All eleven sweep replay endpoints complete with
missed-contact failures, including turning-path/grazing cases that cannot be
judged by endpoint penetration alone.

## Ranked additions for blocking restoration (now Goal 3)

1. **Reliable rigid-wall blocking, including particles and nodes.** Start with
   the observed missed thick/thin-wall and delayed rigid impacts. Evaluate
   conservative local path tests and response that consumes the remaining time.
   Keep moving/rotating obstacles and initially embedded cases in the contract;
   do not restore unconditional world-wide repetition to fix one body.
2. **Rigid support and correction-created contacts.** Restore required stack
   support and discover pairs affected by corrections. Measure an affected-body
   refresh or a bounded fallback against a full rediscovery pass, keeping rigid
   quality distinct from cheaper particle-pair precision.
3. **Track particle-pair protection as a separate tradeoff.** The pit passes its
   0.5 overlap review, while high-speed head-on and dense car contacts fail.
   Retain these failures visibly and measure any targeted protection separately
   from rigid-wall safety. No relaxed final tolerance has been accepted.

Carry the joint/elastic energy failures and beam-contact refresh failure into
Goals 4–5. The baseline proves they matter but does not authorize implementing
those goals now. The pit's 14.6 million particle response calls are the immediate
focus of the new Goal 2; Goal 6 later optimizes the restored, composed result.
Each restored system must state the failures
it fixes and the milliseconds it adds; the car's current headroom is not a
budget to spend blindly.

## Verification and visual evidence

- Linux Release baseline: 20 selected checks pass after correcting the
  diagnostic test's body setup; the five numerical regressions listed above
  remain failed. Initial build/test mistakes and the corrected results are
  preserved separately. The baseline contract test verifies elapsed-time
  consumption and reports, prepared manual/full composition, persistent-source
  ownership, policy cleanup on invalid input, and generation-safe deletion/reuse.
- ASan/UBSan Debug: the same 20 checks pass, with the same five numerical
  failures. Full numerical replays complete, including remaining joint and
  soft-body API/lifecycle checks. Independent car/pit one-second smoke endpoints
  and a separate four-second 800-particle dense-impact endpoint complete with
  leak detection enabled. No sanitizer finding is reported. These shorter
  sanitizer checks are not substitutes for the full clean workload timings.
- Windows: engine and independent installed-SDK consumers, including baseline
  and diagnostic tests, cross-build successfully without new compiler warnings.
  Native Windows execution was not performed.
- Option OFF: all 24 selected Linux regressions pass. Full car/pit oracle runs
  match the preserved control at every one of 600/720 updates: state hashes,
  step/candidate/contact counts, durations and diagnostics. See
  `default_parity.json`. Production defaults remain unchanged.
- All eight primary control/baseline trace/oracle runs have complete endpoints,
  no lost reports or nonfinite/fatal updates, exact exclusive-time closure and
  matching within-version trajectory hashes. Every test execution stayed below
  the one-minute wall cap. No capped measurement was reused as a full result.

Both baseline rendered previews run individually to their endpoints and close
automatically. Screenshots at three and seven wall seconds are retained with
window IDs/geometry under `runs/baseline_car_rendered_1/` and
`runs/baseline_pit_rendered_1/`. The car wheels remain connected while the car
rotates into the particle bed; this visual observation does not excuse the
measured penetration. The pit forms a contained pile; endpoint escape checks
remain zero. User motion/quality acceptance is separate from this observation.

| Preview | p95 frame work | Displayed FPS | Simulated / elapsed | Final lag | Verdict |
| --- | ---: | ---: | ---: | ---: | --- |
| Car, 10 simulated seconds | 7.853 ms | 58.941 | 0.98236 | 179.6 ms | Not rendered acceptance |
| Pit, 12 simulated seconds | 28.810 ms | 42.164 | 0.70273 | 5,076.2 ms | Not real time |

Pacing is excluded from frame work. The compositor tiled both previews at
1420×884 logical window bounds rather than the requested 1280×720, so they are
visual diagnostics, not the final resolution-controlled gate. The car also
misses the 1% timing condition; the pit misses both timing and frame-work gates.
Final window-switch review, three rendered trials, the full sixty-second car
sequence, resolution/pacing control and explicit quality acceptance remain in
Goal 7. Goal 2, baseline pit optimization, has not started. The user subsequently
reviewed a single car replay and said it looked good; this records baseline
visual feedback, not final physics quality acceptance or a new performance pass.
