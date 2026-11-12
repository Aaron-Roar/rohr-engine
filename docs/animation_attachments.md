# Independent animation attachments

Multiple animations can follow the
same body and render simultaneously, with independent frames, timing, visibility,
layers and transforms. Editor attachments and generated creation use the same
independent Entity identities; the body no longer doubles as its first animation.

## Editor and generated projects

Attach any number of animations to the same body using each animation's
Attachment section. The editor stores them dynamically and supports more than
32 animation rows and property panels. Visibility, transforms, layers and
playback remain per animation. Body detachment preserves its world pose and
removes only that animation's former hitbox bindings. Undo restores the
attachment, driver selection and bindings together.

Authored animations create distinct named entities in project and object
creation. Connections use authored identities within that creation, so repeated
instances remain isolated. Drawing attachment and lifetime ownership are
separate; generated animations are owned by their authored parent. Their
transforms resolve when drawn without adding physics transform locks.

Game code that previously controlled the first animation through the body's ID
must look up the animation's runtime name or enumerate the body's attachments.
Developer-edited `main.c` files are preserved and require this manual migration.
Individual creation leaves omitted optional links unbound; provide explicit
`ObjectCreateBinding` entries to attach an individually created animation.

## Hitbox animation driver

One animation may drive a body's active hitbox. In its Attachment section,
enable **Drives Hitboxes**. Choosing a sibling replaces the driver; clearing
the checkbox disables automatic selection. The body's **Bind Frames** control
edits the selected driver's frame bindings. Switching drivers retains sibling
bindings, playback, visibility and layers. Editor saves record this choice;
older documents without the field select their previously bound animation.

The authoring canvas draws all visible hitbox variants; its animation preview
does not switch the displayed hitbox. Use **Simulate Scene** or the generated
application to observe the driver selecting the active physics hitbox. Bind
each desired frame to its variant; an unbound frame leaves the last active
hitbox unchanged.

The authored `ObjectDescription.hitbox_animation_driver` member contains the
chosen animation's description ID. Generated named views expose the same field,
for example `project.actor.body.hitbox_animation_driver`. Generated catalogs
remain immutable creation recipes; use the runtime API to change a live body:

```c
EntityResult driver = rohr_entity_by_name_get("walk");
if(!rohr_error_check(driver)) {
    EngineResult result = rohr_physics_hitbox_animation_driver_set(body, driver.result.value);
    /* Check result before relying on the change. */
}
EntityResult selected = rohr_physics_hitbox_animation_driver_get(body);
/* ENTITY_INVALID disables selection while retaining bindings. */
EngineResult disabled = rohr_physics_hitbox_animation_driver_set(body, ENTITY_INVALID);
```

The driver must be an animation attached directly to that body, or its legacy
animation component (pass the body's ID). A deleted or rebound driver is ignored
and the last active hitbox remains; the getter retains the stored ID and never
follows a reused slot. Index-based binding APIs address the selected driver's
frames. ID-based binding APIs also accept other attached animations so their
bindings can be prepared before switching drivers. Legacy same-entity binding
creation selects that component automatically unless a driver choice was already
stored. Runtime saves retain the driver, hitbox variants and all frame bindings.
For hand-authored mutable descriptions, assign the driver's authored ID before
creation, for example `body.hitbox_animation_driver = walk.id`. A zero member
means disabled. Existing hand-authored recipes with frame bindings must now
select this field explicitly; old editor JSON documents migrate automatically.

This does not add simultaneous collision shapes: only the selected driver's
matching frame changes the existing active hitbox. For movement, a stable body
collider often remains preferable to changing its shape every visual frame.

## Creation and access

`rohr_graphics_animated_sprite_attachment_add(target, name, description)` allocates
an independent entity and returns `EntityResult`. Its direct equivalent is
`graphics_animated_sprite_attachment_add`. The target must be a live entity; it
does not need to have an animation component. NULL or empty names use `animation`.
Creation uses the normal global entity-name namespace and lowest free suffix.

```c
AnimatedSpriteDescription description =
    rohr_graphics_animated_sprite_description_default_get();
/* Supply frames or obtain a description from a loaded AnimationAsset. */
description.body_offset = (Position){0, 12};
EntityResult effect = rohr_graphics_animated_sprite_attachment_add(
    player, "player_effect", description);
if(!rohr_error_check(effect)) {
    Entity animation = effect.result.value;
    rohr_graphics_animated_sprite_body_offset_set(animation, (Position){4, 12});
}
```

Pass the returned Entity to the existing animation description, frame, offset,
orientation, layer, inspection and name APIs. Check result values before using
their payloads. `rohr_graphics_sprite_frames_update` advances every animation;
automatic viewport rendering discovers each attachment exactly as it discovers
other live animation entities. Setting one description's `visible` to false
does not hide its siblings. Existing timing settings control playback separately.

`rohr_graphics_animated_sprite_add(entity, description)` retains its original
meaning: add or replace the one animation component on that entity. It does not
create an attachment or replace other animation entities. Replacing an attached
entity's component through this API preserves its binding and compatible playback.
Removing its animation component removes its binding but leaves the entity and
its lifetime owner intact, like other component removals.

## Storage and lifetime

Component pools and lifetime membership storage grow dynamically during creation.
There is no per-body attachment array or separate fixed attachment limit. Each
attachment consumes one of the engine's existing **10,000 live entity slots**;
this is not unlimited global capacity. Existing per-animation frame limits also
remain unchanged. Allocation/capacity failures are returned, never silently
truncated. Getters, enumeration and drawing do not allocate.

Each attachment retains its own animation catalog and frame texture references.
Failed creation rolls back the new entity and all acquired references. Its target
is initially its lifetime owner: deleting the target deletes the attachments;
`rohr_entity_delete(animation)` deletes just that animation and any members it
owns. Other animations remain usable. IDs include the existing generation checks,
so deleted IDs cannot address later occupants of reused slots. Copied values and
snapshots are not new owners.

`rohr_graphics_animated_sprite_attachment_set(animation, target)` rebinds an
existing animation and transfers its lifetime ownership to the new target.
It preserves its name, local offsets, visibility, layers and playback. Self,
cyclic and stale targets fail. Allocation failure preserves the original binding
and ownership. An ordinary existing animation can be bound this way too.

`rohr_graphics_animated_sprite_attachment_get(animation)` returns its retained
target ID, or `ENTITY_INVALID` for an ordinary unbound animation component.
General entity ownership remains separately editable. If ownership is moved away
and the drawing target is then deleted, pose resolution/drawing rejects the stale
target. It never follows a different entity that reuses the target's slot.

## Enumeration and transforms

`rohr_graphics_animated_sprite_attachment_list_get(target, &list)` copies direct
attachment Entity IDs into caller-owned `EntityList` storage. A zero-capacity
query reports the required `list.count` and returns the capacity error when
nonempty. Insufficient capacity writes no array elements. NULL storage with
nonzero capacity is invalid. Empty lists succeed. Results have no sorting
guarantee and own no entities; deletion makes captured IDs stale.

`rohr_graphics_animated_sprite_pose_get(animation)` returns `AnimatedSpritePoseResult`
with the resolved drawing position and clockwise-degree orientation. It reads
the target's current origin and orientation immediately, without simulation or
physics updates. Body offsets rotate with the target; `follow_entity_rotation`
controls whether its orientation is inherited by the image. Frame offsets and
transforms are applied afterward by the existing frame drawing code.

For nested attachments, the parent attachment contributes its resolved drawing
pose. An ordinary target entity contributes its physical origin/orientation,
without any unrelated legacy sprite's visual offset. An attached entity's own
physics position is not its drawing position; use the pose getter for that.
This is a graphics relationship and does not add bodies, locks or physics forces.

Every new API has an equivalent direct `graphics_*` entry point. Entity inspection
of an attachment still returns one animation component, including its retained
target in `AnimatedSprite.attachment_target`. Enumerate the target's attachments
to inspect all of them. The target field is managed by the attachment API; it is
not part of `AnimatedSpriteDescription` or an owning reference in copied values.

## Persistence and verification

Runtime state saves preserve target names alongside each animation's complete
configuration and playback state. Reload resolves new live IDs and restores
lifetime ownership independently, including attachments whose owner was moved
away from their drawing target. Existing states without an attachment target
remain compatible. Rebuild consumers against the updated SDK: `AnimatedSprite`
has gained a field. Goal 2 also extends `ObjectDescription` and
`HitboxAnimationBindingList`; previously compiled struct layouts are not ABI
compatible.

The focused `animation_attachments` suite covers independent playback, visibility,
render-command submission, layers, transforms, naming, snapshot capacity errors,
256 attachments on one body, nested/rebound targets, legacy component coexistence,
individual and grouped deletion, slot reuse, runtime save/load and asset lifetime.
Linux allocation/retain injection checks rollback during table growth, creation
and ownership transfer. The installed SDK consumer uses only public headers.

Goal 1 verification (2026-11-12):

- All ten Release regressions pass: `animation_attachments`, `animation_assets`,
  `empty_animations`, `runtime_persistence`, `automatic_viewports`, `entity_info`,
  `entity_info_adversarial`, `hitbox_animation_binding`, `graphics_reports` and
  `origin_offsets`.
- The attachment suite passes ASan/UBSan with leak detection, including allocation
  failure injection and independent lifetime-owner persistence.
- The public-header consumer builds and runs against the installed Nix/Linux SDK.
  The engine, focused suite and installed-SDK consumer cross-build for Windows.
  Native Windows execution is not verified.
- The game-state and view-port examples build against the installed Linux SDK
  and run from `/tmp`. Game-state exits successfully; view-port completes a
  three-second headless smoke run and exits successfully on SIGINT. Both examples
  also cross-build against the installed Windows SDK.
- Final builds have no compiler warnings or sanitizer findings. The API inventory
  audit still reports its 77 pre-existing diagnostics; the ten new public/direct
  declarations are classified and add no diagnostics. `git diff --check` passes.

Logs and reproduction scripts are in `sandbox/milestone_20/goal_1/`. The disk-full
link failure and tool paths invalidated by Nix garbage collection were resolved
before the final runs. Generic Linux SDK parity and real editor/generated-app
preview acceptance remain scheduled for Goal 3 after editor integration.

Goal 2 verification (2026-11-12):

- Sixteen focused Release regressions pass, including object instance lifecycle,
  runtime names, editor history, authored JSON, configuration and construction
  parity, snapshots, animation assets, runtime persistence and viewport discovery.
- The history fixture authors 40 attachments on one body and verifies independent
  settings, driver changes, detach/undo, sibling bindings, JSON round trips,
  legacy driver migration and explicit disabling. History assertions now run in
  Release as well as Debug.
- A freshly generated installed-SDK consumer builds and runs project, object and
  individual-entity creation with 40 attachments. It verifies distinct runtime
  IDs, selected-driver behavior, frame/layer/visibility independence, immediate
  poses, runtime snapshots, simultaneous project instances and grouped cleanup.
  The fixture checks deterministic generation and edited-main preservation.
- The attachment, hitbox binding and editor history suites pass ASan/UBSan with
  leak detection. Driver tests cover switching, disabling, deleted IDs, slot
  reuse and runtime persistence of driver, variants and bindings.
- Engine, editor and the affected animation tests cross-build for Windows.
  Game-state and view-port examples build against the installed Nix/Linux SDK
  and run from `/tmp`, with the latter limited to a three-second smoke run and
  clean SIGINT exit. Native Windows execution and full preview acceptance remain
  Goal 3 work.
- Final builds have no compiler warnings or sanitizer findings. The API audit
  still reports the same 77 pre-existing diagnostics; the four added declarations
  are classified. Logs are under `sandbox/milestone_20/goal_2/`.

Goal 3 verification (2026-11-12):

- All 73 selected Release editor, animation, instance, persistence, naming and
  rendering regressions pass. The final run passed 71; the two obsolete
  relocation fixtures passed after migrating their developer mains to current
  creation and Entity deletion. Release assertions now remain enabled throughout
  editor tests, so setup and checks actually execute. The particle authoring
  fixture now checks the current generated description format.
- All 23 installed consumers pass against the Nix-built Linux SDK and the
  generic Linux SDK built inside Rocky Linux 8. All 13 standalone examples build
  independently against both SDKs and launch outside their source directories.
  Interactive examples use three-second smoke runs followed by clean SIGINT
  shutdown; these runs do not establish long-duration physics accuracy.
- The Windows engine/editor, all 23 SDK consumers, all 13 examples and both
  generated workflow suites cross-build. Native Windows execution was not
  performed. Cross-built consumers are no longer registered as runnable tests.
- The 40-attachment fixture exercises project, object and individual creation,
  independent playback and visibility, driver switching, instance isolation,
  enumeration and cleanup against Release, sanitizer and generic Linux SDKs;
  it also cross-builds for Windows. Generation is byte-identical after JSON
  save/reload and across the Nix-built and generic Linux CLIs. The fixture uses
  the editor's normal object-name formatting and preserves developer-edited
  mains and custom CMake files.
- All three shipped starter mains compile and run with Release, sanitizer and
  generic Linux SDKs, including pause/resume and report/cleanup cases; they
  cross-build for Windows. The original explicit alternatives remain intact.
- Attachment, hitbox-binding and editor-history suites pass ASan/UBSan with
  leak detection, as do both generated workflow suites linked to the sanitizer
  SDK. No sanitizer findings or final compiler warnings remain. The API audit
  retains its 77 pre-existing diagnostics without changes.
- The generic SDK was archived, extracted into a different directory, and used
  to build/run the public attachment consumer and validate the saved project
  with its relocated CLI.
- The user accepted independent animation and closing in the real 40-attachment
  Linux application, reported that the editor controls worked, and accepted a
  second application visibly switching between square and wide hitboxes while
  all attachments continued playing. Their observation that the authoring canvas
  does not switch variants is documented above; authoring draws all visible
  variants, while simulation and application physics select the active variant.

Logs, SDKs, preview projects and reproduction scripts are under
`sandbox/milestone_20/goal_3/`. The preview projects are temporary review fixtures;
developer changes to the first preview were preserved when creating the second.
