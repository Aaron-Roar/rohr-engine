# Architecture {#architecture}

Rohr Engine is a C and SDL3 engine built around explicit systems and visible
data ownership. The public facade is `include/rohr.h`; applications should not
depend on headers under `src/`.

## Runtime organization

```text
include/                 public engine and tools APIs
src/
├── core/                lifecycle, facade, errors, systems, timing
├── entity/              stable entity handles and component metadata
├── graphics/            renderer, deferred draw layers, UI, presentation
├── input/               keyboard, mouse, and controller input
├── math/                vectors, polygons, projections, and AABBs
├── physics/
│   ├── body/            body state, forces, and motion properties
│   ├── broadphase/      dynamic AABB tree
│   ├── collision/       filtering, decomposition, overlap, manifolds
│   ├── constraints/     iterative constraint orchestration
│   ├── joints/          anchors and joint constraints
│   ├── particles/       particle-circle geometry and response helpers
│   ├── pipeline/        standard and individually callable stages
│   ├── rigid_body/      integration and rigid contact response
│   └── soft_body/       nodes, beams, areas, and boundary contacts
└── state/               JSON runtime state and authored definitions
tools/
├── editor_core/         editor document commands and CLI grammar
├── cli/                 rohr-cli frontend
├── editor/              rohr-gui frontend
└── terminal/            reusable terminal emulator
```

Domain modules own their state and cleanup. Cross-domain private contracts are
declared in narrowly scoped internal headers such as
`src/core/engine_internal.h` and `src/physics/physics_internal.h`.

## Entities and components

`Entity` is a stable, generation-checked handle. `EntityIndex` is an internal
index into component pools. Systems resolve handles before direct table access;
applications should retain `Entity`, not `EntityIndex`.

Engine component pools grow with entity capacity. A pool owns a value array and
occupancy information, so optional data does not need a valid sentinel value.
Entity deletion clears owned component state before its index can be reused.

Engine component masks use `ROHR_` prefixes. Generated game components use a
separate `GameComponentMask` and typed sparse-set pools. Generated component
addresses are invalidated when their pool grows or swap-removes an item.

## Error flow

Fallible APIs return small generated result types. Errors are propagated to the
application boundary; the application decides whether to log, notify, recover,
or terminate. `rohr_error_message_get(result)` combines the stable Rohr error
message with a captured lower-level cause such as `SDL_GetError()`.

See [Error handling](errors.md).

## Physics

The standard physics update is assembled from public stages under
`src/physics/pipeline`. The default pipeline remains the behavioral reference,
while advanced applications may call stages directly.

Rigid contact constraints and active joints are gathered once per substep and
iterated together. Polygon collision uses an AABB-tree broadphase, SAT over
cached convex pieces, contact manifolds, accumulated impulses, positional
correction, friction, and restitution. Concave decomposition is internal.

See [Physics](physics.md) for ordering, collision modes, and limitations.

## Graphics

Drawing is deferred until presentation. Each active signed layer owns an
insertion-ordered command buffer. Only layers used during the frame exist in
the sparse active-layer list; layers are ordered before execution, while calls
within one layer retain submission order. Buffers retain capacity between
frames.

### Texture ownership

Loaded texture pixels are owned by the graphics texture registry and addressed
through generation-checked handles rather than public SDL pointers. The
resolved, normalized path identifies the registry resource: loading the same
path shares one GPU texture while each returned `TextureAsset` keeps its own
logical drawing size. A successful load returns one owning reference. Copying
the value does not retain it, so every independently owned copy requires a
successful `rohr_graphics_texture_retain` and one matching release.

Successful release consumes one reference and clears the supplied asset.
Releasing a zero handle is idempotent; null pointers and nonzero stale handles
are rejected without changing registry ownership. A texture is publicly valid
only while it has an owning reference. Sprite value construction is non-owning,
while successful component insertion retains the texture. Replacement retains
the new texture before releasing the old one, and component removal or entity
deletion releases the component reference. Failed loading and failed component
insertion leave no acquired reference behind.

Queued drawing uses a separate internal command reference. It can keep the
native texture alive after the final owning release, but it does not make the
handle publicly valid and a later load cannot revive or share that unowned
resource. Executing or discarding the command releases this deferred reference.
Graphics renderer shutdown discards queued commands, destroys the animation
and texture registries regardless of remaining public owners, and invalidates
every prior texture handle. Engine-table teardown separately clears component
ownership.

### Animation ownership

Animations are immutable registry resources addressed through
generation-checked `AnimationAsset` handles. Registry identity includes the
normalized stable animation ID, ordered stable frame IDs, resolved frame
textures and their logical sizes, frame center offsets and clockwise degree
rotations, and tick/time timing. Zero IDs normalize to
their documented compatibility defaults. Identical definitions share one
resource; changing any identity input creates a distinct animation.

Each animation resource owns one texture reference per ordered frame until its
final animation reference is released. Loading an identical animation adds an
animation owner without adding another persistent set of frame references.
Animation information is returned by value, while each immutable frame value
contains a borrowed texture. Callers must not release a borrowed frame texture
unless they first retain their own copy. A successful animation load or
explicit retain creates one owner, and a successful release consumes one owner
and clears the supplied asset. Zero release is idempotent; null pointers and
nonzero stale handles are rejected.

Mutable playback lives in `AnimationPlayer`, separate from shared animation
data. Player and animated-sprite value construction are non-owning. Every player
has an independent frame index, update timestamps, and effective tick/time
durations initialized from the asset defaults. Successful animated-sprite
component insertion retains the animation; replacement, removal, entity
deletion, and graphics teardown release component ownership.

Invalid definitions and partial frame-load failures release every texture
acquired during the attempt. Releasing the final animation owner releases its
frame textures. A queued draw may then defer a frame texture's native
destruction according to the texture command contract, but it does not retain
the animation. Graphics renderer shutdown destroys every remaining animation,
releases its frame textures, and invalidates every prior animation handle.
Engine-table teardown separately clears animated-sprite ownership.

### Shared resource ownership {#remaining-resource-ownership}

Texture and animation ownership established the engine-wide distinction between
immutable shared assets, mutable instances, borrowed values, internal
dependencies, and subsystem infrastructure. Font, text, and audio follow the
same rules:

- Shared assets use engine-owned registries, stable cache identities,
  generation-checked handles, and explicit owning references.
- Mutable instances use generation-checked handles with one public owner and
  explicit destruction. Copying an instance handle creates only a borrowed
  alias, not another owner.
- Internal dependency and queued-command references may defer native-resource
  destruction, but they do not transfer public ownership or make a stale caller
  value reusable.
- Successful release or destruction clears the supplied owner. Zero values are
  idempotent; null pointers and nonzero stale handles are rejected without
  changing the supplied value.
- Forced subsystem shutdown discards queued work, releases dependencies,
  destroys remaining resources, and invalidates every prior handle. Handle
  generations survive subsystem restarts.

These are binding contracts for the ownership implementation. Texture,
animation, font, rendered text, sound, and music ownership follow them.

#### Audit boundary

The audit covers direct and `rohr_` font, text, sound, and music functions;
state and project definitions; generated viewport resources; editor font
validation and preview caches; examples; existing tests; and graphics/audio
shutdown. Fonts and text already cross state, editor, and generated-project
paths. Audio currently has no authored JSON, editor, or generated-C model, so
its ownership scope is limited to the direct API, wrappers, documentation,
example, and tests unless that scope changes explicitly. The completed audit
verified these boundaries:

| Resource | Current boundary | Required boundary |
| --- | --- | --- |
| Custom font | `FontAsset` is a generation-checked handle; resolved path and point size identify shared immutable registry resources with explicit owners. | Complete. |
| Built-in font | The built-in font is an engine-pinned generation-checked entry for each graphics lifetime. | Complete. |
| Rendered text | `TextAsset` is a unique generation-checked handle plus logical size; private payload revisions retain font, queued-command, and persistent-UI dependencies. | Complete. |
| WAV sound | Independent generation-checked players share immutable decoded samples by normalized resolved path. | Complete. |
| Streamed music | Unique generation-checked instances own independent paths, decoders, buffers, cursors, and playback settings. | Complete. |

The following allocations do not need shared-asset registries:

- The SDL window, renderer, SDL_ttf text engine, audio device stream, and audio
  callback are private subsystem infrastructure. Their global isolation belongs
  to the later global-state priority.
- Screen render targets are unique mutable GPU resources already hidden behind
  generation-checked `ScreenId` values. They are not cacheable file assets.
- Graphics UI definitions remain unique generation-checked objects. Their text
  dependencies are part of the text migration, not a new shared UI registry.
- Frame-capture and font-rendering surfaces, WAV source and conversion buffers,
  and similar scratch values are transient allocations owned by one operation.
- The FFmpeg recording pipe is a singleton process resource closed by recording
  stop and graphics shutdown, not a retainable asset.
- Serialized UI definitions are plain authoring data. JSON documents, editor
  arrays, input handles, and physics handles retain their subsystem ownership.

#### Font contract

Public `FontAsset` values contain a generation-checked handle rather than a
native pointer. Only the font registry may access `TTF_Font`.

A custom font is identified by its resolved normalized path and finite positive
point size. Resolution and normalization follow texture asset rules. Identical
identities share one native font, while the same file at a different point size
is a distinct resource. A successful load returns one owner; copying requires a
successful retain for every independently owned copy and one matching release.
Retain fails atomically for stale handles or reference overflow. Release clears
the supplied value only after consuming a live reference. An explicit validity
check distinguishes live, zero, and stale values.

The built-in font is an engine-pinned registry entry for each graphics
lifetime. Getting or copying it needs no caller reference and never depends on a
path. Generic retain is a successful no-op; generic release may clear a caller
value but cannot destroy the pin. Graphics shutdown invalidates that generation
so a pre-shutdown built-in value cannot become valid after restart.

Every custom-font text instance owns one font reference until the text and its
internal dependents are gone. Releasing the caller's last font owner therefore
cannot invalidate live text. Failed path resolution, loading, duplicate
insertion, capacity reservation, or reference acquisition publishes no resource
and frees every temporary allocation. Native font pointers and mutable font
settings are never exposed through public information queries.

#### Rendered-text contract

`TextAsset` remains mutable per consumer and is never cached by content. Its
public value contains a generation-checked text handle and logical size, never
SDL or SDL_ttf pointers. Successful creation returns the unique public owner;
copying creates only a borrowed alias, and there is no public retain operation
for mutable text.

Creation validates its font, acquires the custom-font dependency when needed,
and publishes a slot only after the complete native payload is ready. Empty
text is a valid zero-sized instance without a drawable payload. Value mutation
is transactional: the new payload and dimensions are prepared before replacing
the current revision, and failure preserves the previous string, payload, size,
and font dependency. Built-in and custom text share this public contract even
when their private rendering backends differ.

Every deferred text draw captures the payload revision current when the draw is
queued and adds an internal command reference. Later mutation or public
destruction cannot change or invalidate that command. Execution or command
discard releases the revision. This removes the current raw `TTF_Text *` and
unmanaged `SDL_Texture *` command lifetime.

Immediate-mode UI borrows text only for the call. Persistent graphics UI
definitions store internal text references instead of public pointers. Creation
and replacement acquire the new dependency before releasing the old one;
destruction and failed insertion release anything acquired; getters return
borrowed text values. Destroying the public text owner prevents further public
mutation and clears its value, while UI and command references may defer native
cleanup without reviving that public handle.

Editor preview teardown and generated code destroy persistent UI definitions
before text, and text before custom fonts, on success and failure paths. Missing
custom fonts continue to identify the path, viewport, and UI element and block
generation or building through an editor notification.

#### Sound contract

Decoded and converted WAV samples become immutable internal assets cached by
resolved normalized path. The current mix format is fixed and need not join the
key; a future option that changes decoded bytes must become part of that
identity. The cache is not public and retains no unused entry after the final
player releases it.

Each `Sound` remains an independently controlled mutable player with its own
generation-checked handle, cursor, volume, pan, loop setting, playback rate, and
playing state. Successful creation acquires one cache reference. Repeated
creation for the same path returns different players that overlap and advance
independently while sharing decoded samples.

Copying `Sound` creates a borrowed alias; sounds have no retain operation. A
pointer-consuming destroy clears a successful owner, accepts zero
idempotently, and rejects a stale nonzero handle without clearing it. A dedicated
validity check distinguishes an invalid player from valid false playback or loop
state.

Creation reserves cache and player ownership transactionally. WAV load,
conversion, allocation, capacity, reference-overflow, or audio-lock failure
releases all temporary storage and references and publishes neither an entry nor
a player. Cache changes, player mutation, destruction, and mixing remain
synchronized with the callback. Audio shutdown destroys the stream before
clearing players and cached samples, then invalidates all handles.

#### Music contract

`Music` remains a unique mutable generation-checked instance owning its resolved
source path, Vorbis decoder, decode buffer, cursor/interpolation data, volume,
loop setting, playback rate, playing state, and pause state. It is deliberately
not cached or reference counted: decoder and file position are mutable, and a
registry that shared only a path or small metadata record would not share the
expensive state. Creating the same path twice opens independent decoders.

Copying `Music` creates a borrowed alias. Pointer-consuming destruction clears a
successful owner, accepts zero idempotently, and safely rejects stale nonzero
handles. A dedicated validity check disambiguates invalid music from legitimate
false playing or paused results.

Starting a track seeks and prepares it before replacing the active track. The
previous track is stopped only after the replacement succeeds. Configuration,
open, validation, decode-buffer allocation, seek, and lock failures close and
free partial state while leaving the slot and active track unchanged. Destroying
active music removes it from the mixer before closing its decoder. Shutdown
destroys the stream first, then all decoders and buffers, while preserving the
single-active-track policy.

#### Cross-path and verification contract

Direct functions and `rohr_` wrappers expose the same ownership and errors.
Boolean state queries may return false for invalid handles, but explicit
validity checks and result-returning operations distinguish stale values. State
and project files store paths, sizes, colors, and playback settings rather than
runtime handles; loading acquires owners and unwinds them in reverse dependency
order.

The public ownership shape follows existing naming conventions:

- Fonts add `graphics_font_retain`, `graphics_font_release`, and
  `graphics_font_valid_check`, with matching `rohr_` wrappers. Release consumes
  and clears a `FontAsset *`.
- Text keeps `graphics_text_create`, `graphics_text_value_set`, and
  `graphics_text_destroy`, adds `graphics_text_valid_check`, and makes destroy a
  result-returning, pointer-consuming operation. There is intentionally no text
  retain API. Persistent `ViewportUiTextConfig` values carry a `TextAsset`
  handle value instead of a caller-owned pointer; zero means no text, and UI
  creation or replacement acquires the private dependency.
- Sound and music keep their existing create, playback, mutation, and query
  names. Their destroy functions consume `Sound *` and `Music *`, and explicit
  `audio_sound_valid_check` and `audio_music_valid_check` functions are added.
  There are intentionally no public decoded-sound, music-source, or player
  retain APIs.
- Boolean conditions continue using `_check`, stored values use `_get`, and
  mutations use `_set`. Direct and `rohr_` declarations change together.

Editor font validation and viewport previews use the same registries and release
their owners when projects close or change. Generated projects create fonts
before text and persistent UI, then destroy UI before text and fonts. Focused
tests cover cache identity, independent instance state, internal dependencies,
replacement, stale handles, partial failure, queued work, shutdown, and restart.
Logical registry counters complement ASan, LeakSanitizer, and UBSan.

The implementation order is:

1. Add shared custom-font handles, adapting text only enough to own its font.
   Complete.
2. Add unique text handles, transactional mutation, persistent UI references,
   safe command payloads, and editor/generated-project migration. Complete.
3. Add the decoded-WAV cache and harden sound/music destruction, validity,
   rollback, and shutdown without changing music playback policy. Complete.
4. Extend deterministic ownership and Linux sanitizer coverage to fonts, text,
   sound, and music; then complete the roadmap priority if no gaps remain.
   Complete.

The UI is composed from primitive interactions, surfaces, clipping, text,
fields, sliders, dropdowns, and scroll regions. Higher-level tools use the same
public primitives available to applications.

## Input

The engine event poller automatically feeds an engine-owned per-frame keyboard,
pointer, and text snapshot. Raw device state remains separate from named logical
controllers. Controller-owned actions combine copied tagged bindings into Button,
Axis 1D, or Axis 2D
values; generated project defaults and runtime user overrides occupy separate
binding lists. See [Input](input.md) for lifecycle and authoring details.

## Editor and generated projects

The editor owns an authoring model separate from runtime ECS state. Editor ids
are stable references inside `EditorProject`, not live engine entities. JSON
stores editable objects and metadata; generated C creates runtime entities and
returns their handles through typed generated structs.

Saving JSON and generating C are separate operations. Generation replaces only
files under `src/generated/`; developer-owned `src/main.c` is created with a
new project and is not overwritten afterward.

See [Editor guide](editor.md) and [Editor architecture](editor_architecture.md).

## Build and distribution

CMake is authoritative. `dev.sh` and `dev.bat` are convenience frontends. SDK
installs export a `Rohr` CMake package, headers, static libraries, `rohr-cli`,
`rohr-gui`, configuration defaults, project templates, and licenses.

See [Building and SDK usage](building.md).
