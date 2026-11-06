# Baseline pit optimization

Milestone 13 Goal 2 optimizes the opt-in simpler baseline before adding collision
protection back. The [roadmap](../NEXT_STEPS.md#13-physics-rebuild-from-the-simpler-baseline)
owns the goal order. Production defaults and public APIs remain unchanged.
**First-attempt outcome: Failed against the performance gates.** The retained four-pass
candidate improves pit CPU time by 43.3%, but its 7.664-second median misses the
six-second target; all three rendered runs also miss the strict real-time gates.
The user accepted the preview's behavior, not those performance failures.
**Rebased outcome: Failed against the strict performance gates.** All seven
investigations and measured combinations are reported in [the new results](#rebased-investigation-results).
The proposed geometry/solver patch reduces the matched pit median from 7.240 s
to 6.387 s with identical sampled trajectories. Experimental two-pass response
reaches 5.533 s and sleeping 3.970 s, but normal rendered playback still misses
the timing contract. Prior evidence remains preserved; no quality/default
selection or restoration goal has begun.

## Rebased Goal 2 investigation plan

Investigate all seven levers before choosing combinations. This is the revised
scope of Goal 2, not seven new milestone goals; Goals 3–7 keep their order.
The user has instructed execution. The implementation parent is
`0fc3c5b1`; the retained comparison implementation is
`bdb622c6eb8d41829ee893ee6ac17bfa268ab07f`. Full source/executable provenance and
new runs are preserved under `sandbox/milestone_13/goal_2/rebased/`. Keep its 60 Hz,
four-substep, four-pass behavior as the fixed comparison control. Also retain
Goal 1 and the frozen references; exclude `main`.

1. [x] **30 Hz physics, first.** Change only the outer physics tick from 60 to
   30 Hz, keeping four configured substeps, four solver passes, authored inputs
   timed in simulation seconds, and 60 FPS normal rendering. Each integration
   substep consequently doubles in duration; this is a behavior experiment,
   not just less scheduling overhead. Compare both complete diagnostic workloads
   for CPU time, ms per simulated second, ms per tick, p95/max tick and rendered
   frame work, backlog and simulated/elapsed time. Inspect input response,
   visible stepping, car deformation, pile settling, overlap and tunneling.
   Record any existing interpolation; do not add rendering smoothing silently.
   A 30 Hz tick may burst on alternate rendered frames, so average cost alone
   is insufficient. Do not promote 30 Hz to a default before user selection.
2. [x] **Refresh particle polygon geometry only when a consumer needs it.**
   Trace dirty writes and consumers, including mixed/beam contacts, transform
   locks, queries, publication and standalone pipeline boundaries. Test direct
   circle data and deferred polygon refresh while ensuring each consumer sees
   current geometry. Count flushes/vertices avoided and new checks performed.
   Aim for unchanged response and trajectories where execution order is held
   fixed; classify any numerical/order changes explicitly.
3. [x] **Simplify particle solver storage and execution.** Measure compact
   particle constraints, prepared invariant masses/material/filter data and
   reduced generic dispatch. Preserve directional response, nonrotation,
   sequential shared-body updates and generation-safe identity. Distinguish
   data valid for one solve from data requiring invalidation on motion or API
   mutation. Include setup and memory costs; do not remove safety checks without
   preserving their guarantees. Keep solver effort fixed for this screen.
4. [x] **Reduce particle-pair response effort.** Reverify two particle-pair
   passes against four while keeping particle–rigid and rigid–rigid effort
   unchanged. Include eligible soft-body nodes and measure their coupled car
   behavior. The earlier 5.493-second/two-pass pit with 0.330803 worst overlap
   is an exploratory lead, not verified acceptance. Keep the known one-pass
   1.791743 overlap failure visible; do not repeat unchanged rejected candidates.
   Record bounce, residual overlap, settling and energy as well as worst overlap.
5. [x] **Separate particle collision cadence from elastic integration.** Test
   fewer particle-pair discoveries and response rounds while retaining necessary
   elastic integration steps. Measure discovery reuse and response cadence
   separately before combining them. Explicitly account for newly approaching
   pairs, correction-created contacts, fast motion and node/rigid coupling.
   Keep existing rigid-contact effort; specify how the motion interval will be
   covered by later wall protection rather than treating skipped checks as safe.
6. [x] **Reuse candidates and previous velocity impulses.** First test bounded,
   conservative candidate reuse with current geometry checks; then test velocity
   impulse warm starting as a separate variable. Measure fewer searches and
   passes against cache maintenance. Cover entity deletion/reuse, radius and
   shape changes, filtering, teleports, external forces, moving obstacles and
   corrections outside retained bounds. Reject stale contact identity or normals;
   do not blindly reuse positional corrections or turn missed contacts into passes.
7. [x] **Sleep settled particle groups.** Test standalone piles first, with
   support checks and waking on impact, loads, edits, support removal and moving
   obstacles. Evaluate beam/joint-coupled nodes separately; retain active handling
   if safe group wake propagation is not worthwhile. Report time to sleep,
   actually avoided visits, maintenance/wake spikes and changes in slow motion,
   settling and response to disturbance. Eligible samples alone are not savings.

For items 2–7, use isolated branches/overlays against the retained 60 Hz control
first so a tick-rate change cannot explain an algorithm's apparent saving. Then
screen the useful changes at 30 Hz as well; retain 60 Hz as an option if its
quality or tick-spike behavior is preferable. Settings remain at 60 Hz or below.
Every investigation needs a measured verdict, not an obligation to ship each
mechanism. Any experiment that cannot produce meaningful measurements must be
identified as untested/incomplete with its reason, not marked investigated.

After all seven screens, rank savings and quality in **both** workloads, select
a small set of complementary candidates and measure combinations. Do not add
individual speedups arithmetically. Preserve API overrides already supported;
label private experimental controls separately. Public API redesign, production
default changes and final tolerance acceptance require a separate user decision.

### Measurement, protection and completion

- Preserve the unchanged ten-second car diagnostic and full twelve-second pit;
  car acceptance still uses the inherited full sequence. Keep the one-minute
  wall cap per execution, including shutdown. Preserve incomplete results and
  do not repeat unchanged known-slow candidates or shorten endpoints silently.
- Keep source/settings provenance and unsuccessful trials under a new
  `sandbox/milestone_13/goal_2/rebased/` evidence directory. Do not overwrite the
  first attempt's executables, manifests, source overlays or logs.
- Screen clean timings separately from exclusive instrumentation. Report total
  full-run CPU time, stage and pair-type costs, operations, cache/flush costs,
  tick spikes, frame spikes and full-run worst errors. At 30 Hz, normalize cost
  per simulated second and per 60 Hz presentation frame as well as per tick;
  do not compare unlike tick counts or infer rendered smoothness from throughput.
- Retain the 0.5-unit particle-pair review threshold for pit/head-on cases and
  0.1-unit Milestone 6 stress penetration checks. These are reporting thresholds,
  not solver correction slack. Report failures with measured value, threshold,
  excess and supported explanation. Continue endpoints despite numerical failure.
- Include particle/node–rigid thick-wall and head-on diagnostics early for each
  behavior-changing candidate, alongside rigid–rigid and relevant soft/joint
  regressions. Preserve existing failures and identify new ones. The baseline
  already omits sweeps/adaptive subdivision; their removal supplies no new saving.
  Goal 3 still owns blocking restoration. Lower particle-pile accuracy and
  stronger wall blocking are distinct properties; future protection does not
  automatically restore settling or momentum accuracy lost here.
- Verify finalists with three serial clean runs per workload/version on AC,
  streaming closed and no concurrent builds; then separate rendered trials and
  single-window behavior review. Require relevant Linux regressions, ASan/UBSan,
  Windows cross-builds and independent installed-SDK examples. Ownership,
  nonfinite-state handling, manual pipelines and truthful reporting remain mandatory.
- Keep the approved five-second car / six-second pit CPU targets and rendered
  60 FPS, p95 work at most 16.67 ms, simulation within 1% of elapsed and no
  accumulating backlog. Completing the screens does not turn an unmet performance
  gate into a pass. Report distance to the proposed 4 ms average / 5 ms p95 physics
  budget separately, including actual 30 Hz spikes; it is not a newly approved
  gate. Present costs still needed for blocking and structural restoration.
- Deliver a per-investigation cost/quality table, combined-candidate evidence,
  retained/rejected decisions and unresolved risks. Report Complete, Failed or
  Stuck according to the roadmap contract, then stop for review and the user's
  commit before any restoration goal begins.

## Rebased investigation results

**Outcome: Failed against the performance gates.** All seven investigations
have measured verdicts. The proposed engine patch retains only deferred geometry
refresh and the simpler one-point particle response path. It reproduces every
sampled control state at both rates and leaves production defaults, public APIs,
four solver passes and configured substeps unchanged. Sleeping, reduced particle
effort, reduced cadence and caches remain isolated experiments. No new physics
quality or production configuration has been accepted. The user selected retaining
these trajectory-preserving changes and preserving sleeping for later. The failed
outcome reflects measured performance gates, not missing user feedback.

The implementation parent is `0fc3c5b16e2ddcccaadfe1892b4af66d7587034c`;
the frozen retained control is `bdb622c6eb8d41829ee893ee6ac17bfa268ab07f`.
`sleep_retry` in the new evidence means that control library linked to the updated
diagnostic harness; it does **not** enable sleeping. `retained` is the proposed
geometry/solver patch. `sleeping_hardened` adds the isolated sleeping prototype.
The original first-attempt report and artifacts below remain historical records.

All new artifacts are under `sandbox/milestone_13/goal_2/rebased/`. Settings and
binary hashes accompany each execution. AC power and closed streaming were
confirmed by the user; controlled runs were serial with no concurrent builds.
Car diagnostics keep the authored 200 particles plus 100 nodes and ten simulated
seconds; pit diagnostics keep 800 diameter-five particles, restitution 0.5 and
twelve seconds. Every execution has a 59-second interrupt / 60-second hard cap.
No failed numerical endpoint was shortened to make it pass.

### Isolated screens and decisions

The table shows initial single clean CPU screens in seconds, with a separate
per-substep quality oracle. These are screening measurements, not three-run
acceptance. Four configured substeps and four passes remain fixed except for
the explicitly listed response-effort experiment. Thirty-hertz runs retain
60 Hz rendering, drawing the latest state on alternate frames without adding
interpolation. The input sequence uses simulated time.

| Isolated lever | 60 Hz pit / car CPU s | 30 Hz pit / car CPU s | Pit worst overlap, 60 / 30 Hz | Decision |
|---|---:|---:|---:|---|
| Retained control | 7.278 / 2.656 | 3.885 / 1.469 | 0.266 / 0.818 | Fixed comparison |
| Deferred geometry | 7.452 / 2.648 | 3.949 / 1.483 | 0.266 / 0.818 | Retain after repeated comparison |
| Simpler particle solver | 7.549 / 2.800 | 4.112 / 1.477 | 0.266 / 0.818 | Retain after repeated comparison |
| Two particle-pair passes | 5.499 / 2.686 | 2.803 / 1.560 | 0.331 / 1.574 | Useful 60 Hz experiment; no default selection |
| Alternate-substep particle response | 4.809 / 2.438 | 2.495 / 1.362 | 0.913 / 2.871 | Reject this cadence for the current review limit |
| Conservative candidate reuse | 7.116 / 2.570 | 3.917 / 1.483 | 0.320 / 0.607 | No worthwhile combined benefit; preserve prototype |
| Velocity impulse warm start | 7.464 / 2.591 | 3.836 / 1.500 | 0.307 / 1.207 | Reject this implementation at fixed effort |
| Whole-scene particle sleeping | 4.179 / 2.736 | 3.796 / 1.550 | 0.266 / 0.818 | Promising isolated option; preserve, do not promote |

1. **30 Hz:** nearly halves total work, but doubles the motion interval of each
   substep. Pit overlap exceeds 0.5 by 0.318; car node overlap grows from 1.290 to
   3.297. The 1,000-units/s head-on case misses the collision at 30 Hz despite
   reporting zero overlap: the particles cross between samples. At 60 Hz it
   catches the collision with 0.000227 overlap. Existing delayed-update failures
   remain. Thirty hertz is not selected; its lower average cost does not establish
   protection or smoothness. The 30 Hz pit preview measured 58.71 FPS,
   17.36 ms frame-work p95 and a 0.97850 simulation/elapsed ratio; the car prefix
   measured 59.11 FPS and 8.05 ms p95, also outside the 1% timing gate.
2. **Geometry:** separate three-run isolated medians were 6.817 s for the pit
   versus 7.592 s for its matched control. Skip redundant per-iteration flushes
   only during complete baseline solves without beams/joints; rigid consumers
   refresh on demand and finalization receives current geometry. Coupled and
   manually composed stages retain their existing flush behavior. All sampled
   car/pit state hashes match at 30 and 60 Hz. No geometry cache survives the
   existing public-call boundary. Counters below distinguish fewer flush calls
   from actual polygon translations avoided.
3. **Solver:** the isolated repeated pit median was 7.037 s against that same
   7.592 s control. The one-point nonrotating particle path avoids generic
   manifold preparation, point matching and angular dispatch. It keeps the same
   rebound target, directional inverse masses, sequential normal/friction
   updates and validated pair commits. There is no persistent material cache,
   new allocation, contact-storage redesign or public API. Control state hashes
   match; manual/complete parity now covers one, two, four and eight passes.
4. **Two particle-pair passes:** the diagnostic wrapper skips only eligible
   particle-pair response, including nonrotating nodes; particle–rigid and
   rigid–rigid response retain four passes. It still traverses the existing
   constraint lists, so this is a measured policy experiment, not a shipped
   independent-pass API. At 60 Hz the pit stays below 0.5, but final overlap
   rises from 0.04929 to 0.13385 and RMS speed from 0.57574 to 0.64135. Mean pile
   height falls by 1.090 units. Car node overlap is 1.276 versus 1.290, and beam
   error 8.214 versus 9.827; trajectories change, so these numbers do not imply
   universally improved physics. At 30 Hz the 1.574 overlap exceeds 0.5 by 1.074.
5. **Collision cadence:** skipping particle response on alternating configured
   substeps retains force/integration, gathering and all rigid response. At
   60 Hz it exceeds the pit overlap limit by 0.413, and at 30 Hz by 2.371. The
   car node error also grows to 2.940 at 60 Hz. Candidate reuse below independently
   tests reduced discovery. Neither experiment provides wall protection over
   skipped intervals; restoration must cover those intervals explicitly.
6. **Reuse and warm starting:** the bounded one-unit-skin candidate cache reused
   2,746 lists and rebuilt 134 at 60 Hz, with no overflow. It rechecks current
   geometry/filters and invalidates on membership/radius changes or displacement
   beyond half the skin. Wider candidate coverage and changed pair order raise
   checks from 3.971 million to 6.346 million; total saving is small despite 95.3%
   list reuse. Pair and membership/circle arrays consume about 1.209 MB. The
   separate warm-start cache uses 16,384 full-identity slots (2.753 MB), only the
   preceding substep, matching duration/material/mass/radii, compatible normal,
   small displacement and normal speed at most two. It never reuses positional
   correction. Maintenance costs and changed settling outweigh the saving here.
   Six existing gathering, mutation, lifetime, recovery and response checks pass
   for each prototype. They are rejected experiments, not fully qualified new
   persistent-cache APIs.
7. **Sleeping:** a conservative contact-group eligibility audit first found
   essentially no benefit at speed 0.1 or 0.5. At speed one and a 0.5-second dwell,
   the actual prototype sleeps the **entire eligible standalone-particle scene**
   together, requiring static support for every connected group. It refuses
   beam/joint/moving-rigid scenes and more than 1,024 colliders. Exact snapshots
   of identity, shape, pose, material, filter and prepared loads wake the scene
   on mutation; nonfinite values cannot qualify. Snapshots alone occupy 1.212 MB.
   The 60 Hz pit sleeps at 6.7375 s, skips 8,643,972 response visits over 1,263
   substeps and removes 41.905 units of residual kinetic energy. Gathering and
   publication continue. Final RMS speed becomes zero, final overlap 0.05322,
   and mean pile height is 0.787 units above the active control. Thus slow settling
   is frozen. Ten load/impact/support/edit/lifecycle wake cases pass in release
   and sanitizers. The car remains active; the 30 Hz pile never qualifies. This
   is not independent island sleeping for a busy game. A new object wakes the
   whole scene, and continuous activity can eliminate the saving. The user chose
   to preserve this experiment for later; no sleeping engine option is added
   and no default is changed.

Earlier 60 Hz combination screens were geometry+two passes 5.171 s, reuse+two
5.911 s, warm+two 5.994 s and sleeping+two 5.811 s for the pit. In the last case
the noisier two-pass pile never slept. Savings are not additive. The final
comparisons below test geometry+solver, that pair with two-pass response, and
that pair with four-pass sleeping.

### Final three-run CPU comparisons

Medians below use three serial clean full-endpoint runs per workload/version.
All use 60 Hz and four configured substeps. Quality oracles and exclusive
profilers are separate and do not supply these CPU acceptance numbers.

| Candidate | Car CPU s / 10 s | Pit CPU s / 12 s | Car / pit physics headroom | Pit ms per 60 Hz frame |
|---|---:|---:|---:|---:|
| Control | 2.592 | 7.240 | 3.86× / 1.66× | 10.056 |
| Retained geometry + solver | 2.582 | 6.387 | 3.87× / 1.88× | 8.871 |
| Retained + experimental PP2 | 2.609 | 5.533 | 3.83× / 2.17× | 7.685 |
| Retained + experimental sleep | 2.756 | 3.970 | 3.63× / 3.02× | 5.514 |

The retained pit patch saves 11.79% versus the matched control, but exceeds the
six-second budget by 0.387 s (6.45%). Its car change is only about 0.4%, within
run-to-run variation. Two-pass and sleeping experiments meet the average physics
CPU targets; neither establishes strict rendered acceptance. The proposed
four-ms-average/five-ms-p95 full-game physics budget remains unmet: retained pit
average is 8.871 ms per 60 Hz frame, PP2 7.685 ms and sleep 5.514 ms. Their
respective clean tick p95 ranges are 11.783–12.672, 9.814–10.442 and
11.543–12.204 ms. Sleeping improves the settled tail, not the expensive active
part of the drop. The car retained average is 4.303 ms, with p95 5.714–6.164 ms.

### Exclusive subsystem measurements

These are instrumented **full-run milliseconds**, not clean CPU acceptance
values. Each nested interval is charged once; all eight final profiles have
`accounting_valid=1`, no lost report updates and complete elapsed simulation time.
The pair breakdown is a subset of rigid solving and must not be added to it.

| Pit stage | Control | Retained | Retained + PP2 | Retained + sleep |
|---|---:|---:|---:|---:|
| Force preparation | 90.4 | 89.0 | 87.6 | 86.8 |
| Integration | 196.2 | 194.4 | 191.2 | 269.5 |
| Contact gathering | 1418.8 | 1374.8 | 1398.3 | 1331.6 |
| Rigid solving, all pairs | 5760.4 | 5629.7 | 4537.7 | 3292.1 |
| Joint/lock solving | 83.6 | 82.1 | 82.3 | 82.6 |
| Geometry flush/publication | 1408.0 | 461.4 | 441.0 | 247.1 |
| Orchestration | 144.3 | 141.4 | 142.6 | 191.1 |
| ↳ Particle–particle response | 3581.7 | 3411.3 | 2272.3 | 1672.3 |
| ↳ Particle–static response | 927.7 | 985.0 | 968.8 | 447.9 |

The retained car spends 987.3 ms gathering, 955.4 ms in all rigid solving,
217.7 ms in joint/lock solving, 111.1 ms in soft solving and 284.5 ms in
finalization. Its rigid response includes 258.0 ms particle-particle,
287.6 ms particle-static and 85.9 ms particle-moving. The pit remains dominated
by repeated response plus gathering. The car has a broader cost distribution;
sleeping a standalone pile does not solve its structural workload.

Pit candidate counts are 3,971,047 control/retained, 4,283,479 with PP2 and
3,852,579 with sleep. Configured substeps stay 2,880 in every 60 Hz pit run and
2,400 in every car run; there are no sweeps/adaptive subdivisions in this
baseline. Fewer particle passes change trajectories and future discovery, so
candidate counts need not fall. Observed cross-object dirty flush calls fall
from 11,520 to 2,880 in the pit; car calls rise from 9,600 to 12,000 because
the final boundary flush is added while coupled per-pass handling remains.
The diagnostic geometry counters report actual complete-substep translations:

- pit: 9,389,956 → 4,900,979 translations; 150,135,616 → 78,311,984 vertices.
- car: 2,750,288 → 2,750,288 translations; 21,846,141 → 21,846,141 vertices.

### Rendered playback and pacing

Three normal runs per control/retained/PP2 workload use a verified 1280×720 client,
60 FPS limiter, debug drawing off and no slow motion. Car rows are the ten-second
diagnostic prefix. All endpoints complete; every strict playback verdict fails.
Work excludes pacing, and simulation/elapsed time includes pacing.

| Version / workload | FPS range | Frame-work p95 ms range | Simulation / elapsed range |
|---|---:|---:|---:|
| Retained + experimental PP2 / car | 59.08–59.14 | 6.65–6.79 | 0.98464–0.98569 |
| Retained + experimental PP2 / pit | 58.79–58.86 | 12.64–12.70 | 0.97978–0.98095 |
| Retained geometry + solver / car | 59.09–59.10 | 7.09–7.16 | 0.98489–0.98506 |
| Retained geometry + solver / pit | 58.70–58.85 | 14.54–14.66 | 0.97839–0.98077 |
| Control / car | 59.10–59.14 | 7.08–7.09 | 0.98495–0.98562 |
| Control / pit | 58.56–58.75 | 15.97–16.98 | 0.97603–0.97923 |

Retained and PP2 frame work fits 16.67 ms, but playback remains below 60 FPS
and more than 1% slow with accumulating lag. The earlier single sleeping preview
also failed: 58.88 FPS, 14.33 ms p95 and ratio 0.98126. It is not a three-run
rendered finalist or an accepted quality option. Single-window previews were
shown; the user reported not seeing enough to judge. New motion/settling quality
is therefore unassessed, not rejected. This does not cause the measured
performance-gate failure.
The prior first-attempt preview acceptance does not apply to these new policies.

Inspection found that `graphics_pipeline_present` waits before `SDL_RenderPresent`
and starts the next pacing interval after presentation. Presentation time is
therefore additional to its nominal frame interval. A separate wrapped diagnostic
measures that cost; it does not modify engine rendering or normal acceptance runs.
The pit audit recorded 249.820 ms in 721 presentations (including startup),
about 0.346 ms each, versus 239.872 ms of final schedule lag. Requested waits
totaled 3,911.844 ms and actual waits 3,914.693 ms. These observations support
presentation being the main added pacing cost; they are not a repaired-limiter
acceptance run.
The existing `--uncapped` request sets limit zero, which actually selects the
engine's 120 FPS fallback. Those six legacy-request runs are preserved under
`uncapped_*` and must not be described as unlimited throughput. Corrected
`actual_unpaced_*` diagnostics bypass only `SDL_DelayPrecise`, with vsync off,
and explicitly log `bypass=1`; they are separate instrumented diagnostics.

- True unpaced diagnostic, retained, PP2, car: 184.39 FPS, simulation/elapsed 3.073.
- True unpaced diagnostic, retained, PP2, pit: 104.06 FPS, simulation/elapsed 1.734.
- True unpaced diagnostic, retained, PP4, car: 181.42 FPS, simulation/elapsed 3.024.
- True unpaced diagnostic, retained, PP4, pit: 84.81 FPS, simulation/elapsed 1.413.
- True unpaced diagnostic, sleep_retry, PP4, car: 175.53 FPS, simulation/elapsed 2.925.
- True unpaced diagnostic, sleep_retry, PP4, pit: 79.51 FPS, simulation/elapsed 1.325.

The full sixty-second authored car acceptance run, window-switch review and
final quality selection remain Goal 7. The one-minute wall cap was not waived.
Neither a ten-second prefix nor uncapped throughput is presented as that gate.

### Safety, regressions and remaining protection

- Linux retained regression suite: 20 pass, five existing failures (head-on,
  sweeps, joints, soft bodies, wall blocking). Default option-OFF suite: 24/24.
  The joint aggregate assertion is not a completed numerical inventory; separate
  continuing joint/soft/sweep inventories preserve endpoint results and errors.
- ASan/UBSan retained suite has the same five numerical failures with no reported
  sanitizer findings. Full twelve-second pit and ten-second car diagnostics
  complete under sanitizers, as do the sleeping candidate's workloads and ten
  focused wake scenarios. No safety pass is inferred for unexecuted endpoints.
- All **228 original-setting stress endpoints** complete with the same 52 passes
  and 176 failures as the retained control. The separate four-substep/four-pass
  release screen reaches 226 of 228 endpoints before the soft-body batch cap
  (227 result lines include one interrupted case). The analogous sanitizer
  screen reaches 189 of 228; stack and soft-body batches hit the cap. These
  incomplete runs are preserved and not retried unchanged or called passes.
- Each behavior-changing screen and the retained candidate has particle/node
  thick-wall checks at 30 and 60 Hz plus the original head-on schedules. Each
  50-case wall run reaches every endpoint: 26 pass, 24 fail, 24 crossings,
  no fatal/nonfinite state. The largest one-sided wall violation is about
  985.5 units; that measures travel beyond the entry face, not a 985-unit
  intersection with the ten-unit-thick wall. Its threshold is 0.1. Missed
  collisions remain separate from overlap; zero overlap cannot excuse tunneling.
- Public API/ownership, entity deletion/reuse, variable radii, grid outliers,
  recovery, manual pipelines, directional response, nonrotation and reporting
  remain covered by existing checks. The expanded baseline test compares
  complete/manual particle response and current world geometry across four
  effort settings. Final car/pit state hashes equal control for all 600/720
  updates at 60 Hz and 300/360 updates at 30 Hz.
- Linux builds and Windows cross-builds pass without compiler warnings.
  Independent installed-SDK soft-body examples build for both platforms;
  the Linux example launches from `/tmp` and exits cleanly after its requested
  60-frame smoke. Its built-in sixty-second acceptance reporter correctly marks
  that short run incomplete; it is launch verification only, not full-run
  physics evidence. Native Windows execution was not performed.

The 0.5 particle-pair review threshold and 0.1 stress penetration criteria are
unchanged; no solver correction slack was added. The retained car still has
5.992 worst standalone overlap, 1.290 node overlap and 9.827 absolute beam error.
The pit's acceptable overlap does not establish restored wall/structural quality.

### Review and subsequent work

The proposed commit contains the two trajectory-preserving changes and their
verification/reporting, as selected by the user. Sleeping stays preserved for
later investigation. This retention decision does not accept new physics quality
or waive the performance gates. At 60 Hz, PP2 is the simpler measured quality/cost option
for future selection; sleeping offers a larger isolated-pile saving with a much
narrower workload benefit. Neither is silently enabled. Tick rate, global
substeps and global solver iterations retain their existing APIs. Per-pair
effort/cadence, cache skins and sleeping controls are private experiment settings;
a public independent-effort/sleep API requires a separate decision.

Before spending the remaining budget on protection, keep separate measurements
for active-pile response, discovery and presentation pacing. For the PP2 option,
the six-second physics budget leaves only 0.467 s over twelve simulated seconds
(0.65 ms per 60 Hz frame) for added work if 2× average headroom is to remain.
Independent supported islands could generalize sleeping, but require coupling,
wake propagation and lifecycle work; the whole-scene result does not predict
their game-level saving. Cache reuse needs fewer total checks, not merely fewer
rebuilds. Goal 3's wall protection and Goal 4's structural restoration must
measure their added costs and cannot be assumed to recover cheaper settling
accuracy for free. Presentation pacing is a separately identified issue; no
graphics behavior is changed in this goal. Goals 3–7 have not begun.

Reproduction entry points: `build_final.py`, `verify_final.py`, `finish_checks.py`,
`measure_final.py`, `render_final.py`, `run_geometry_audit.py` and
`render_unpaced.py`, run serially inside `nix develop`. Per-variant source
overlays/patches, SDK archives and build caches preserve rejected experiments.
`final_summary.json`, `analysis.json`, individual run metadata and the final
artifact manifest provide the audit trail. An initial geometry-counter build
used an incorrect SDK path and failed to link; the corrected diagnostic uses
the frozen retained SDK explicitly. Failed logs remain preserved.

## First-attempt scope and provenance

- Immediate parent: `7e337fc2cb4f5ed93e4d2f4d7f27cee24b7fa6df`.
- Comparison control: Goal 1's simpler baseline, preserved SDK archive SHA-256
  `6c5605856e08c86e846ea1049fe0c3b4d451aa660d4192a4cbb949b5df6e2d85`.
- Historical schedule: `a8a7940a6e6d039c80aed20c6f6b2624169e1ee3`.
- Pre-rebuild checkpoint: `0b254c045b806fb13c022cd7470f063a03a130db`.
- Frozen accuracy reference: `6f283268aa4bf038c4ed78271baeb9d5986bd7d9`.
  `main` is excluded; previous evidence stays in place.

Artifacts are under `sandbox/milestone_13/goal_2/`: parent source archive,
preserved control SDK, candidate archives/source patches, independent installed-SDK
consumers, scripts, build caches, logs and individual run records. The final
manifest pins the retained implementation and executables. Earlier experimental
archives and failed attempts remain identified rather than silently replaced.

Both first-attempt workloads use 60 physics ticks/s, four configured substeps and four
solver passes. The authored car contains 200 standalone particles and 100 nodes;
its ten-second prefix is diagnostic, not the final sixty-second acceptance run.
The unchanged pit contains 800 particles, diameter 5 and restitution 0.5, over
twelve simulated seconds. Geometry, materials, body counts and input sequences
are unchanged. Existing runtime setters still control tick duration, substeps
and solver iterations; there is no new particle-pass API or hidden iteration cap.

The user preferred retaining physics quality over sacrificing substantial
behavior to save another second near the six-second target. Four passes are
retained for review. The user found the single four-pass pit preview acceptable;
this is provisional motion feedback, not final Milestone 13 quality acceptance.
The six-second budget, rendered gates and 0.5-unit pit overlap limit are still
reported as measured criteria. The overlap limit is not solver correction slack.

## Retained implementation

Changes activate only within `ROHR_PHYSICS_SIMPLE_BASELINE=ON` complete updates
and complete-substep calls. The option remains OFF by default. Manual stages
retain their existing algorithms and explicit composition/ownership contracts.

1. Reuse validated, nonrotating particle geometry during response. Particle-pair
   correction computes the live circle without repeatedly translating every
   polygon vertex. Mixed contacts and the existing dirty flush still update
   world polygons. Invalid/extreme geometry falls back to full validation; the
   cached polygon's pose stamp is never falsely advanced.
2. Specialize point response for two nonrotating particles. Preserve restitution,
   accumulated impulses, friction, directional response masks, effective pair
   masses and sequential validated velocity commits, while omitting zero angular
   lever-arm work. Rotating circles and ordinary rigid contacts retain the
   existing response path.
3. Construct rigid constraints directly in existing owned storage. Initialize
   identity, response, contact, counts and validity flags without clearing/copying
   unused manifold payloads. Readers remain gated by those counts/flags. Existing
   allocation failure, destruction and generation checks remain in force.
4. Use a spatial hash for particle-pair discovery in beam-free complete substeps.
   Polygon interactions still use the tree. Mixed-size particles use a cell width
   at least the largest diameter and inspect neighboring cells conservatively.
   When the largest diameter exceeds four times the mean, use the existing tree
   instead. That threshold is a measured heuristic, not a claim of universal
   optimality. Sizes are reconsidered each gather; no cross-step topology cache
   or sleeping state is introduced.

Cells are not clamped to scene boundaries. Full signed cell coordinates
distinguish unrelated cells sharing one of the 4,096 hash buckets. The engine's
world-coordinate validation still applies; this does not create an infinite
numeric world. Out-of-range cell conversions use an explicit all-pairs fallback
instead of overflowing or dropping candidates. Grid scratch storage is bounded
by the existing entity capacity and does not allocate per query.

Beam-coupled scenes retain tree discovery and its original contact order. An
initial grid experiment omitted particle leaves needed by beam gathering; its
car results are rejected. Restoring those leaves fixed the beam regression, but
grid order still worsened node overlap without a worthwhile car speed benefit.
The retained tree path avoids that tradeoff. Manual gathering always builds the
complete tree. The isolated response/geometry changes preserve sampled car
trajectories; the grid deliberately changes pit contact order, so its physics
results are compared numerically rather than claimed bit-identical.

The grid can still degrade with crowded cells, adversarial hash aliases or
unfavorable size distributions. The outlier fallback addresses the measured
large-particle case, not every possible distribution. A multilevel grid or
separate oversized-particle structure would require its own cost/coverage study.

## Investigated alternatives

Initial clean screens were single runs, not final acceptance medians:

| Experiment | Pit CPU seconds / 12 simulated | Worst particle overlap | Decision |
| --- | ---: | ---: | --- |
| Goal 1 control, four passes | 13.489 | 0.303774 | Comparison |
| Two particle passes only | 9.633 | 0.490913 | Near overlap limit; retain evidence |
| One particle pass | 7.135 | 1.791743 | Reject; exceeds 0.5 by 1.291743 |
| Geometry reuse, four passes | 10.751 | 0.303774 | Retain; sampled states unchanged |
| Geometry plus specialized response, four passes | 9.622 | 0.303774 | Retain; sampled states unchanged |
| Same response changes, two passes | 6.892 | 0.490913 | Optional reduced-effort candidate |
| Add in-place construction, two passes | 6.673 | 0.490913 | Retain construction; not the pass reduction |
| Velocity response only on first pass | 8.697 | 0.976530 | Reject; exceeds 0.5 by 0.476530 |
| Tree topology reuse, bounds refreshed, two passes | 6.644 | 0.521108 | Reject; exceeds 0.5 by 0.021108 |
| Initial grid, two passes | 5.493 | 0.330803 | Exploratory pit result; original car path rejected |
| Initial grid, four passes | 7.339 | 0.266482 | Exploratory pit result; corrected and reverified below |

In those grid pit runs, two passes increased the final remaining overlap from
0.049292 to 0.133850 and final RMS speed from 0.575739 to 0.641348. Worst overlap
increased from 0.266482 to 0.330803; both remained below 0.5. Mean particle height
at the endpoint was 39.755395 versus 38.665855, showing a different pile. Floor
depth was lower with two passes (0.063147 versus 0.129438). These metrics do not
make quality a single scalar or establish equivalent motion. No reduced-pass
configuration is installed as a default in this goal.

Sleeping was screened as an upper bound, not implemented without wake-up and
support correctness. In the two-pass tree candidate, only 3.94% of particle
samples had stayed below speed 0.1 for half a second; 4.49% stayed below 0.5.
The four-pass grid settled differently: 0.40% below 0.1, but 52.59% below 0.5.
These are eligible samples, not avoided solver work or validated sleep savings.
Impacts, forces, moving support, filter/shape changes and coupled nodes would
require additional wake-up/maintenance work. No significant quality change was
introduced merely to claim those theoretical savings.

An isolated 800-particle, 120-gather sensor diagnostic tested discovery separately
from dynamics. Before the outlier fallback, one radius-250 particle among radius-2.5
particles cost 237.6 ms versus the control's 85.3 ms. The retained fallback screen
cost 98.8 ms. Equal-size and distant-cell screens cost 46.6/43.1 ms versus
75.6/77.2 ms; varied radii cost 62.5 ms versus 87.2 ms. These single-run small-scene
screens justify the fallback and expose overhead; they are not workload headroom
acceptance or a universal speedup claim.

## Verification and reproduction

Run scripts inside `nix develop`. `build.py baseline [sanitizers|windows]` builds
the selected engine and independent SDK consumers; `build.py control` uses the
preserved Goal 1 archive, and `build.py default` verifies the option-disabled
engine. `final_trials.py retained` runs the three serial clean comparisons;
`profile_trials.py` separates exclusive profiles, numerical oracles and the
pair-work audit. `rendered_one.py` checks the actual Sway client dimensions
before releasing a diagnostic-only start gate. One early window-tracking error
ended at zero simulated seconds; its incomplete record is preserved, and the
fixed attempt has a separate label. User interruption stops the visual sequence.

Each runtime execution has a one-minute wall cap, including shutdown. Final runs
use AC power, streaming last confirmed closed, and no concurrent builds/tests.
The host CPU governor was `powersave`; no governor or machine settings were
changed. Report all three measurements rather than selecting the fastest one.
Profile/oracle instrumentation is excluded from clean CPU timing. Stage tables
sum exclusive time only; pair response rows are subsets of rigid-solving time.

The expanded baseline regression covers dynamic/static/one-way/sensor pairs,
100:1 mass ratio, friction, offset circles, synchronized final hitboxes,
mixed polygon/particle discovery, changing radii, deletion/slot reuse,
far-away negative coordinates, aliased hash buckets and guarded extreme offsets.
Ownership, elapsed-time consumption and release of complete-substep policy before
callbacks/manual calls remain covered. Relevant Linux and ASan/UBSan suites
retain 20 passes and five known numerical failures. The option-disabled suite
passes all 24 tests. A six-second sanitizer drop/pile and one-second car diagnostic
complete without sanitizer findings. Windows engine/consumer cross-builds pass;
native Windows execution was not performed. The ordinary standalone car also
builds independently against the installed SDK on Linux and Windows; its short
Linux smoke is not the final sixty-second scenario.

All 228 inherited stress endpoints complete: 52 numerical passes and 176
failures, the same verdict counts as Goal 1. `comparison.json` preserves each
failure line with measured values and available limits. Existing failures include:

- Four of five original head-on schedules miss the 1,000-units/s impact, with
  rebound error 1,500 despite zero sampled overlap. The original fixture uses
  one configured substep; zero overlap does not prove collision protection.
- Thick-wall cases at 10,000 units/s still cross with no contact; recorded depth
  is about 975.5 against 0.1 (about 975.4 excess). The 1,000-units/s and initially
  embedded cases remain distinct in the retained logs.
- Pin/weld inventory spin errors remain about 665.48/614.34 against 0.1;
  current anchor errors can be small while motion quality fails.
- Correction-created beam contact is still absent in its regression. The bounded
  gather-once schedule delays rediscovery; it is not fixed by faster searching.
- Fixed-step stack/elastic fixtures retain penetration, energy and stability
  failures. Those require the later blocking/structural goals, not a silent
  tolerance change in this optimization.

The ordinary joint regression aborts on its existing numerical assertion; the
separate inventory replay reaches its endpoint and records the remaining cases.
Numerical failures are not relabeled passes. The 0.5 pit/head-on review and 0.1
stress penetration criteria are unchanged. Final quality and production-default
selection remain Milestone 13 Goal 7 work.

## Final measurements

Three serial clean runs per workload/version, with all declared diagnostic
endpoints complete:

| Workload | Goal 1 control CPU seconds | Retained candidate CPU seconds | Candidate median | Budget |
| --- | --- | --- | ---: | ---: |
| Ten-second car | 3.159, 2.883, 3.337 | 3.032, 3.105, 3.159 | 3.105 | 5.000 |
| Twelve-second pit | 13.239, 15.338, 13.511 | 7.652, 7.682, 7.664 | 7.664 | 6.000 |

The pit uses 43.3% less CPU, but still needs 21.7% less work to reach its
six-second budget. It runs at 1.57× real-time physics throughput. The car has no
convincing material speed improvement and retains all 600 sampled state hashes
and numerical measurements from Goal 1. Option-disabled car/pit runs match all
600/720 historical sampled states and report counts. Within each measured
version, trace/oracle states match and exclusive timing accounting closes exactly.

| Full-run pit quality | Control | Retained candidate |
| --- | ---: | ---: |
| Worst particle-pair overlap | 0.303774 | 0.266482 |
| Final particle-pair overlap | 0.051446 | 0.049292 |
| Worst floor-circle depth | 0.134687 | 0.129438 |
| Escape samples | 0 | 0 |
| Final RMS speed | 0.508867 | 0.575739 |
| Final mean particle height | 38.674769 | 39.755395 |

The inherited car errors remain: maximum standalone-pair overlap 5.992004,
node-pair overlap 1.289857, absolute beam-length error 9.826794 and relative error
1.049554. Matching that trajectory does not make those errors acceptable.
At the runtime's four configured substeps, the separate head-on replay completes
all five cases with three passes/two failures: normal 60 Hz detects the collision
with 0.000227 overlap, but the 30 Hz and larger-delay cases still tunnel with
rebound error 1,500. These are separate from the original one-substep stress run.

### Exclusive physics costs

Milliseconds over one full instrumented pit run, not clean CPU acceptance:

| Stage | Control | Candidate |
| --- | ---: | ---: |
| Force preparation | 90.9 | 92.9 |
| Integration | 194.3 | 201.7 |
| Contact gathering | 2769.5 | 1461.4 |
| Joint gathering | 17.8 | 18.0 |
| Rigid/particle solving, including dispatch | 10076.7 | 6008.8 |
| Joint solving | 83.0 | 87.5 |
| Finalization, including geometry flush | 1256.7 | 1418.1 |
| Reporting | 58.8 | 56.8 |
| Orchestration | 163.0 | 159.5 |

There are no subdivision sweeps or soft contacts in the pit. Within candidate
rigid solving, particle-pair response costs 3721.9 ms, particle–static response
980.1 ms, polygon response 10.5 ms and remaining solver/dispatch work 1296.3 ms.
Within finalization, instrumented dirty-hitbox flushing costs 1284.7 ms.
These are subsets, not additional costs to add to the table. Geometry-flush cost
did not disappear into a claimed response saving; it is explicitly included.
The car profile records 1043.0 ms gathering, 1027.9 ms rigid response, 231.7 ms
joint response, 119.3 ms soft response and 301.7 ms finalization; the full stage
and function breakdown is in `analysis.json`.

### What the millions of visits mean

| Particle-pair work over the full pit | Control | Candidate |
| --- | ---: | ---: |
| Distinct entity pairs seen during the run | 2,253 | 2,006 |
| First solver visits to gathered pair occurrences | 3,644,445 | 3,531,811 |
| Total response visits, four per occurrence | 14,577,780 | 14,127,244 |
| Visits with retained solved state | 10,916,999 | 10,585,596 |
| Visits changing a position | 13,727,740 | 12,702,820 |
| Visits changing a velocity | 12,214,002 | 13,521,517 |
| Visits changing neither | 339,309 | 169,171 |

Position/velocity rows overlap. Most repeated visits change state; they cannot
simply be deleted as exact no-ops. Control particle response costs about 0.539 µs
per instrumented visit (Goal 1's earlier 8033.5 ms was about 0.551 µs); the
candidate costs about 0.263 µs. The primary gain is cheaper work per visit, plus
cheaper discovery, rather than eliminating most response visits.

Both versions complete 2,880 pit substeps and 11,520 solver rounds. Reported
candidates fall from 4,275,022 to 3,971,047. The inherited `CONTACT_COUNT` counter
is not a count of all particle contacts: the scalar particle-pair branch does
not increment it before returning. Its retained values (153,047/148,745) must not
be presented as total physical contacts. The independent pair audit above supplies
the particle visit/occurrence counts. This pre-existing counter limitation is
preserved visibly rather than changing counter semantics in these comparisons.
Link wrappers count instrumented call boundaries; same-object calls can remain
inside their parent timing. In particular, joint processing also invokes an
already-empty second dirty flush; the 11,520 instrumented flush calls are not a
claim that no other empty calls occur.

### Rendered results

Actual client size is 1280×720 in each accepted measurement. All six paced runs
reach twelve simulated seconds, but none passes the strict gates:

| Version/run | FPS | p95 frame work, ms | Simulation / elapsed | End lag, ms |
| --- | ---: | ---: | ---: | ---: |
| Control 1 | 42.72 | 28.98 | 0.7120 | 4853.1 |
| Control 2 | 39.68 | 30.99 | 0.6613 | 6144.9 |
| Control 3 | 40.22 | 30.97 | 0.6703 | 5901.6 |
| Candidate 1 | 58.69 | 17.22 | 0.9782 | 267.3 |
| Candidate 2 | 58.20 | 17.98 | 0.9701 | 370.5 |
| Candidate 3 | 58.80 | 17.10 | 0.9801 | 244.2 |

Candidate p95 exceeds 16.67 ms by 0.43–1.31 ms. Simulation falls 2.0–3.0% behind
elapsed time, outside the 1% limit, with accumulated lag. The separate uncapped
run reaches 72.93 FPS, 16.30 ms p95 work and 1.216× simulation throughput; it is
not paced acceptance. It spends 7470.5 ms CPU in physics and 1574.0 ms wall time
in drawing/presentation, excluding pacing, over 720 frames: about 10.4 ms and
2.2 ms per frame respectively. Graphics is secondary to physics in this scene.
CPU and wall figures have different measurement boundaries and are not an exact
additive decomposition. No intentional slow motion or debug drawing is enabled.

## Review direction

Retain the verified reductions for review, but do not advance into restoration
as though sufficient headroom were established. The user explicitly stressed
that these isolated tests must leave room for a full game. A proposed future
budget is 4 ms average physics per 60 Hz frame with p95 below 5 ms; this is an
engineering proposal, not a universal industry standard or an approved change
to this goal's gates. Four milliseconds corresponds to 2.88 CPU seconds for this
pit, requiring about another 2.66× speedup from the retained candidate.

The user subsequently rebased Goal 2 into the seven-investigation plan above,
with 30 Hz first, followed by geometry refresh, particle solver data/execution,
reduced passes, collision cadence, reuse/warm starting and group sleeping.
Near-zero idle
visits mean simple no-op skipping is unlikely to supply the needed reduction.
The current profile alone does not prove how much any proposal will save.

Reliable particle/node–rigid and rigid–rigid blocking must eventually fit inside
the budget. Additional collision-risk protection should be selective; making
every particle interaction trigger world-wide work would repeat the earlier
problem. Final performance must be measured with protection enabled, rather than
accepting a fast tunneling baseline. A separate graphics/frame-pacing study may
be useful, but these measurements do not justify making it the main next
optimization goal. No later roadmap goal has been started or reordered.
