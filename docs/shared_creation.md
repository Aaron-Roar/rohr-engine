# Shared entity and object creation

Milestone 15 Goal 2 implements the entity/object portion of the
[creation contract](creation_and_viewport_contract.md). Viewport creation/show
changes and the unified generated `project.h` remain later goals. Existing
generated catalogs work with the new calls now.

## Using descriptions

Include `rohr.h` and your existing generated catalog header. For example, the
bundled center-of-mass catalog exposes these descriptions:

```c
#include "rohr.h"
#include "project_objects.h"

/* Alternative creation operations; check each result before using its value. */
EntityResult instance = rohr_object_create(&objects.mass_demo);
EntityResult body = rohr_entity_create(&objects.mass_demo.automatic_body);
```

The first operation creates the whole object, including its bodies, anchors,
and camera. The second creates only the selected body and its configured
components. It does not create the body's separately authored anchor children
or any siblings. Container and soft-body aggregates require object creation.
Both operations return `EntityResult`; `rohr_entity_delete` releases the created
root and its owned contents. `rohr_object_destroy` still releases a loaded or
copied **description**, not a runtime instance.

Descriptions are borrowed during creation and never mutated. Each call copies
runtime state and acquires its own asset references. Releasing a loaded/copied
description after creation is safe. A const compiled catalog needs no release.
`EntityCreateDescription` names the existing common catalog record for a member;
the metadata-only `EntityDescription` API remains unchanged.

## Finding created members

After checking `instance`, find its immediate members with:

```c
EntityResult body = rohr_object_member_get(instance.result.value, "automatic_body");
/* After checking body: */
EntityResult anchor = rohr_object_member_get(body.result.value, "automatic_tip");
```

Lookup uses authored member names, independent of runtime name suffixes or later
runtime renaming. It does not recursively select a member from another nested
object. Missing or ambiguous names return an error. The older recursive
`rohr_entity_object_member_by_name_get` retains its original semantics.

Returned member handles are borrowed aliases. Some attached sprite/animation
descriptions resolve to their body's entity; deleting that handle deletes the
body. Remove the visual component when only that visual should be removed.

## Placement, ownership, and dependencies

Use `rohr_entity_create_options_default_get()` or
`rohr_object_create_options_default_get()` and change the required fields.
Both options support `owner`, a world-space translation `offset`, and a
`bindings` pointer with `binding_count`. Object options also expose the existing
physics/visual `flags`; zero flags are invalid, and the default getter selects
both categories. Supplying a lifetime owner does not apply its transform again.

Internal references resolve within the new object instance. For individual
creation, bind each referenced description to the intended live entity:

```c
/* live_body is a previously created compatible entity. */
ObjectCreateBinding binding = {
    .description = rohr_object_get(objects.mass_demo.automatic_body),
    .entity = live_body
};
EntityCreateOptions options = rohr_entity_create_options_default_get();
options.bindings = &binding;
options.binding_count = 1;
EntityResult anchor = rohr_entity_create_with_options(
    &objects.mass_demo.automatic_body.automatic_tip, options);
```

Binding descriptions must belong to the same catalog as the selected template.
Missing nonzero references, duplicate/conflicting identities, wrong kinds, stale
handles, cycles, and invalid topology ownership fail creation. A joint binding
names an anchor entity; its owned anchor resource is resolved automatically.
Individual creation of a node, beam, or area requires the intended soft-body
owner. Force/torque members require their runtime owner and referenced targets.
Zero references retain the existing disconnected/world semantics.

Binding an attached visual to an existing body creates a separate attached
entity; it does not overwrite that body's existing components. Newly created
members within one object can still use the established visual alias behavior.

Creation allocates temporary bounded traversal storage and may allocate runtime
components/load assets. Dependencies are created before their users even when
listed later in the catalog. Existing owners receive the completed root only
after all other fallible work succeeds. Failure deletes the attempt's entities,
anchors, cameras, aliases, and acquired asset references. Initial force/torque
sources retain the existing next-update lifetime; creation does not apply a
force or advance simulation.

## Verification

Verification results and inherited failures are recorded in the milestone 15
entry in [NEXT_STEPS.md](../NEXT_STEPS.md#15-descriptor-based-creation-and-automatic-viewport-rendering).
Detailed local logs and staged SDKs are under `sandbox/milestone_15/goal_2/`.
The focused creation suite covers public/direct calls, generated-layout views,
independent instances, released description copies, nested member access,
explicit bindings, forward references, ownership, malformed catalogs, allocation
failure rollback, and 1,000 creation/destruction cycles with camera invalidation
and texture ownership accounting.

The eight focused suites (`descriptor_creation`, `object_instances`,
`runtime_persistence`, `shared_sources`, `soft_body`, `texture_assets`,
`animation_assets`, and `authored_car`) pass in Release and under ASan/UBSan with
leak detection. Allocation injection covers entity/component allocation,
external anchors, owner attachment, and texture-reference rollback. The latest
creation tests also pass after adding explicit external torque and anchor cases.

All installed consumer targets and independent car/flies examples build against
isolated Linux and Windows SDK installations. Three Linux consumers execute
successfully, including the generated-catalog creation case. Consumers and
examples launch from a temporary directory outside the source tree with dummy
SDL drivers. The car completes the requested 600-frame diagnostic prefix with
2,400 substeps, 701,387 candidates, and 108,585 contacts. Its own report correctly
marks the full 60-second benchmark incomplete; this is neither full benchmark
acceptance nor a rendered timing claim. Flies starts and exits cleanly on SIGINT.
No compiler warnings were observed in these builds. Native Windows execution
was not performed. These are Nix-built installed SDK checks; generic Linux
distribution packaging and the integrated game workflow remain Goal 6.

Two broader physics checks remain failing: `joints` exits with SIGSEGV in Release
and fails the angular-velocity assertion at `tests/joints.c:340` in the sanitizer
configuration; `forces` fails `completed > substeps` at `tests/forces.c:95`.
Both reproduce with matched builds of the pre-change implementation from
`409041e2` and with the new implementation. The comparison substitutes the two
modified entity implementation translation units in an otherwise identical
engine archive, then relinks the unchanged tests. Neither sanitizer run reports
a memory or undefined-behavior finding for those assertion failures. The focused
eight-suite result does not imply the full suite is clean; physics defaults and
these inherited failures remain unchanged.
