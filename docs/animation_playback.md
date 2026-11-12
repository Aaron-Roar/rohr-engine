# Animation playback modes and completion

Milestone 21 adds per-player playback policy and persistent completion. Goal 1
covers runtime configuration and persistence; editor controls and generated mode
settings belong to Goal 2 and are not implemented yet.

## Description configuration

`AnimationPlaybackMode` has two values:

- `ROHR_ANIMATION_PLAYBACK_LOOP` (zero): repeat continuously, preserving legacy
  behavior for zero-initialized descriptions and old saves.
- `ROHR_ANIMATION_PLAYBACK_ONCE`: finish the last frame's full configured interval,
  mark the player completed, and keep rendering that frame.

Use `AnimationPlayerDescription.playback_mode`, including the nested
`AnimatedSpriteDescription.player.playback_mode`. Hand-authored
`ObjectDescription.playback_mode` applies the same setting during creation.
Description getters return the mode; invalid modes fail without changing state.
Mode belongs to the individual player, not the shared animation asset.

```c
AnimatedSpriteDescriptionResult current =
    rohr_graphics_animated_sprite_description_get(animation);
if(!rohr_error_check(current)) {
    current.result.value.player.playback_mode = ROHR_ANIMATION_PLAYBACK_ONCE;
    EngineResult changed = rohr_graphics_animated_sprite_description_set(
        animation, current.result.value);
    /* Handle changed before relying on the new mode. */
}
```

`completed` belongs to runtime `AnimationPlayer` state, not the description.
Compatible description changes preserve completion, frame and clocks. Existing
content-change reset cases (changed catalog or removal/replacement of the current
frame's identity) clear completion and reset to frame zero. Changing a frame's
transform or texture while keeping its ID preserves progress. Explicit frame-index
assignment changes the frame without clearing completion or resetting clocks.

## Runtime controls

The entity APIs below have equivalent direct `graphics_` functions. Standalone
players expose the same suffixes through `rohr_graphics_animation_player_*` and
`graphics_animation_player_*`, taking an `AnimationPlayer *` instead of an Entity;
checks accept a const pointer.

| Entity API | Behavior |
| --- | --- |
| `rohr_graphics_animated_sprite_loop_set(animation, loop)` | Select loop or once; preserve frame, clocks and completion. |
| `rohr_graphics_animated_sprite_loop_check(animation)` | Check the configured mode, even if currently completed. |
| `rohr_graphics_animated_sprite_completed_set(animation, true)` | Freeze at the current frame; do not seek to the last frame. |
| `rohr_graphics_animated_sprite_completed_set(animation, false)` | Clear completion and resume the current frame with a fresh interval. |
| `rohr_graphics_animated_sprite_completed_check(animation)` | Read persistent completion without consuming it. |
| `rohr_graphics_animated_sprite_restart(animation)` | Clear completion, select frame zero, and begin a fresh interval. |

Setters and restart return `EngineResult`. Invalid/stale IDs and missing animation
components fail; null standalone pointers fail. Boolean checks return false for
invalid targets. A completed looping animation is valid after manual completion
or changing the mode of a completed one-shot. Enabling looping never resumes it
implicitly: clear completion or restart explicitly.

Entity controls use current engine clocks when clearing completion/restarting;
standalone controls use the last clocks supplied to their update function. Restart
preserves configured mode and timing, and starts at zero regardless of an authored
starting-frame setting. Clearing completion on the last frame in once mode gives
that frame another interval, then completes again.

## Timing, visibility and ownership

Tick and time thresholds retain the existing either-threshold behavior. An update
advances at most one frame, even after a large clock jump. A one-frame one-shot
completes after one interval; empty animations never complete automatically.
Disabling both clocks prevents advancement without setting completion. Hiding an
animation does not stop its updates. Restart does not enable disabled clocks.

Attachments keep independent completion and timing. These controls do not allocate,
retain/release assets or change ownership. Deleting/reusing a slot never makes a
stale animation ID valid. A completed hitbox driver retains its current frame and
corresponding active hitbox; the physics pipeline is unchanged.

## Persistence and compatibility

Runtime animation state stores `playback_mode` and `completed` alongside the frame
and elapsed clocks. Old states without these fields load as looping/not completed.
Malformed mode or completion values reject loading and use existing rollback.
Completed and running one-shots retain their states through save/load.

Rebuild the engine and SDK consumers together: public player/description layouts
have changed. Existing source using defaults or designated initializers continues
looping. Editor mode selection, JSON authoring, and emitted non-default mode
configuration remain Goal 2 work. Developer-edited mains remain developer-owned.

## Goal 1 verification

Verification artifacts are under `sandbox/milestone_21/goal_1/`:

- Release: seven animation, attachment, hitbox, object and persistence regressions
  pass, plus five graphics reporting, entity information, editor core and generated
  attachment compatibility regressions. Engine, editor and CLI compile successfully.
- ASan/UBSan with leak detection: all seven runtime regressions pass without
  findings. The installed sanitizer SDK's public playback consumer also passes.
- Installed Nix-built Linux SDK: the public playback consumer builds and runs;
  game-state and viewport examples build independently and run from `/tmp`, with
  bounded smoke runs and clean interrupt shutdown. Existing generated attachment
  workflows still compile and run all three creation paths with default looping.
- Windows: engine, editor, CLI, seven runtime test executables, the installed SDK
  playback consumer and both affected examples cross-build. Windows executables
  were not run on this Linux host.
- No compiler warnings were reported in the verification builds. The API inventory
  includes all twenty new public/direct declarations; its 77 existing diagnostics
  are unchanged from Milestone 20. `git diff --check` passes.

Editor mode authoring, generated non-default modes, generic Linux packaging and
real editor/application preview acceptance remain Goal 2 verification. Goal 1
does not claim sanitizer verification of the editor workflow.
