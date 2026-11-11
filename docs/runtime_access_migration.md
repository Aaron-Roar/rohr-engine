# Runtime access migration

Authored `project` and `objects` catalogs are immutable creation recipes. Game
state stores typed runtime IDs. Regenerate catalogs and rebuild all consumers
against the matching SDK; entity, viewport, controller, sound and music names
use separate global namespaces. See [entity IDs and names](entity_ids.md) for
the complete snapshot, naming and lifetime contracts.

## Migrate developer-owned mains

Generation preserves edited `src/main.c` byte for byte. Only exact recognized
old templates are upgraded automatically. A preserved main using removed APIs
requires manual changes; regeneration cannot infer your gameplay ownership.

| Previous access | Current access |
|---|---|
| `project_root_get`, `project_viewports_get`, project-local named lookups | Keep the `ProjectId` for destruction; use typed global runtime-name lookups and retain their returned IDs. |
| `object_member_get`, `entity_object_members_get`, `entity_object_member_by_name_get`, `entity_object_member_name_get` | Retain creator IDs, resolve a known runtime name, or enumerate `EntityList`/`EntityNameList` snapshots. |
| Generated mutable object structs, `<object>_instance_bind`, per-object create/draw/destroy wrappers, `ProjectObjects`, `ProjectControllers` | Game-owned typed IDs, shared engine creators, explicit updates and automatic viewport rendering. |
| Generated `project_audio` convenience catalog | Named immutable sound/music recipes in `project`. |

Both direct and `rohr_` hierarchy lookup spellings are removed from installed
headers. Creation, grouped destruction, ownership/parent operations, object
transforms, camera relationships, controller actions and viewport placements
remain available. Internal membership exists for wiring and safe cleanup; it
is not an application access hierarchy.

The starter main keeps three complete paths with clear dividers: whole project
active, whole object and individual entities commented out. Select one main.
Each shows initialization, checks, frame updates and teardown directly, without
helper functions or macros. The individual path creates a useful independent
subset; required joint/topology dependencies still need explicit bindings.
Upgraded generic mains also retain all three alternatives. Their object path
creates the project entity hierarchy with a caller-owned camera/viewport; their
individual path uses an independent native particle recipe so it remains valid
after arbitrary authored object renames or deletions. Replace that recipe with
an authored member and explicit dependencies when needed.
Global `project_settings_apply()` remains separate from creation. The disposable
editor scene entry point continues to use the scene runner's lifecycle callbacks
for transport, pause and subprocess control.

## Names discover resources; IDs retain identity

Creation uses the authored name if available, then the lowest available suffix:
`box`, `box_1`, `box_2`. Existing suffixes are part of the base. Names have no
project/object prefix and become available again after deletion. In a fresh
world the first project can resolve its authored names directly:

```c
EntityResult found = rohr_entity_by_name_get("box");
if(rohr_error_check(found)) goto cleanup;
Entity box = found.result.value; /* Store this ID in game state. */
```

For repeated instances, keep returned IDs and inspect snapshots around creation
when you need to discover additional created entities. Compare IDs, not array
positions or guessed suffixes. The caller must prevent concurrent creation or
deletion during discovery. Ownership relationships can identify a retained
object's members when inspecting a snapshot; they do not restore authored-name
hierarchy lookup semantics. `EntityNameList` contains copied current runtime
names, not authored member aliases.

Never use global names to wire two authored instances. Supply the existing typed
creation bindings; internal identity mappings select the correct instance even
when names collide or change. Actions are scoped to their controller ID and
viewport items to their viewport ID. Access screens through placements, cameras
through entities, assets through typed handles. Named layers keep shared-name
semantics.

## Snapshot storage and cleanup

Allocate storage explicitly using `rohr_entity_alive_count_get()`, set `.data`
and `.capacity`, then check `rohr_entity_list_get(&list)`. The getter allocates
nothing. On insufficient capacity it copies no elements and sets `.count` to
the required count; resize and retry between mutations. Zero entities need no
allocation. Free your array separately from destroying runtime resources.

Snapshots own no entities. Deleted IDs become stale even if their slots are
reused, and copied names remain unchanged after renaming. Validate IDs before
using a snapshot after mutations. Whole-project cleanup uses the retained
`ProjectId`; standalone object cleanup uses its root `Entity`. Destroy separately
created viewports/controllers/audio by their own IDs. Grouped cleanup skips
individually deleted resources and preserves replacements in reused slots.

The migrated [player example](../examples/player_controller/src/main.c),
[center-of-mass main](../examples/center_of_mass/src/main.c) and
[soft-body main](../examples/soft-body/src/main.c) show these patterns.

## Milestone 18 Goal 3 verification

The Release engine, CLI and editor build successfully. All 24 focused runtime and
generator tests pass. Installed SDK consumers pass 18/18 on both Nix Linux and
Rocky Linux 8, including positive typed-access and negative removed-API compile
contracts. The generic Linux SDK is rebuilt in the existing packaging container;
verification output is isolated from the user's distribution directory.

All three starter mains and all three generic mains produced by exact legacy
migration compile and run against Release, ASan/UBSan and generic Linux SDKs,
and cross-build against the Windows SDK. Each has direct initialization, checks,
updates and cleanup. Starter report-injection checks cover seven cases per path;
bounded runs include pause/resume. Exact preceding templates upgrade, customized
mains remain byte-for-byte intact, and repeated generation is deterministic.

All 13 examples compile and run independently against the installed Release SDK
from outside their source directories, and all 13 cross-build for Windows. The
three migrated authored examples also run with ASan/UBSan and leak detection.
The car completes 3,600 frames/60 simulated seconds in both configurations with
matching 14,400 substeps, 7,222,164 candidates and 1,195,541 contacts. These are
completion and migration checks, not real-time performance acceptance: the
headless logs report `realtime_pass=0`. No physics or pacing behavior changed.

Eight focused runtime tests pass with ASan/UBSan and cross-build for Windows.
Six migrated SDK consumers also pass under sanitizers and cross-build. Generated
configuration, construction, incomplete joints, collision migration, partial
spawning, motion, soft-body properties, center-of-mass, areas, lifecycle and
scaffold-preservation checks pass. Scene probes cover full selection, a selected
object, empty selection, no viewport and saved editor selection. Viewports with
no selected camera omit unused object bindings. Assertion-based visibility and
car-angle tests now retain their checks in Release builds.

The user accepted the real Linux preview of the freshly generated application
and its normal close behavior. Native Windows execution remains unverified.
No compiler warnings or sanitizer findings remain in the final checks.
The CLI/editor are refreshed in the Release, sanitized, generic Linux and
Windows SDKs. The installed sanitized CLI also passes fresh creation and
deterministic regeneration with leak detection.

The API inventory audit retains 77 pre-existing diagnostics (missing inventory
entries and duplicate facade declarations), compared with 84 in the committed
baseline snapshot. This goal adds none; seven missing-entry diagnostics disappear
with removal of their public APIs. The obsolete documented member queries and
their inventory counts are updated. This verification does not claim that the
whole historical inventory audit passes.

Local commands, sources and logs are retained under
`sandbox/milestone_18/goal_3/`. The drivers cover Release (`verify_release.py`,
`broad_release.py`, `extras.py`, `scenes.py`, `final_checks.py`), SDK/platforms
(`platforms.py`, `examples.py`, `generic_mains.py`, `tool_parity.py`) and generic
Linux packaging (`generic_linux.sh`, `generic_final.sh`). Disposable sanitizer
build outputs may be removed after verification to conserve disk space.
