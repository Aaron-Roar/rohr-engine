# Independent animation attachments

Milestone 20 Goal 1 adds runtime attachments. Multiple animations can follow the
same body and render simultaneously, with independent frames, timing, visibility,
layers and transforms. Editor controls and authored/generated creation are Goal 2;
the editor's existing single-attachment validation remains until that goal.

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
has gained a field, so previously compiled struct layouts are not ABI compatible.

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
