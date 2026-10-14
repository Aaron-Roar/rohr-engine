# Audio

Rohr audio is an optional service backed by one SDL3 playback device and one
engine mixer. Start it explicitly after or independently of the core engine:

```c
EngineResult result = rohr_audio_start();
if(error_check(result)) {
    fprintf(stderr, "%s\n", rohr_error_message_get(result));
    return 1;
}
```

`rohr_audio_stop()` synchronously destroys every remaining audio resource and
returns a final `Report`. Stopping the core engine also stops audio before SDL
shuts down; explicitly stop audio first if you need its final report immediately.

## Queued controls and updates

All play/stop/pause/resume calls and runtime property setters enqueue requests.
Success means **accepted**, not yet applied. Call `rohr_audio_update()` regularly
to submit pending requests and collect completed callback work:

```c
EngineResult accepted = rohr_audio_sound_play(sound_handle);
if(error_check(accepted)) {
    fprintf(stderr, "%s\n", rohr_error_message_get(accepted));
}
Report report = rohr_audio_update();
if(report.diagnostic_count || report.has_fatal) {
    EngineResult printed = rohr_report_console_write(&report);
    if(error_check(printed)) {
        fprintf(stderr, "%s\n", rohr_error_message_get(printed));
    }
}
```

The callback applies submitted controls in FIFO order at its next buffer boundary.
Play followed by pause is valid. Getters show applied state, so `playing_check()`
can remain false after play was accepted. Existing sound/music playback continues
independently of further updates; only new requests wait for submission.

Concurrent callers may enqueue requests from their own threads. Successful
enqueue order determines execution order. Application operations serialize
resource lifetime against shutdown and synchronize with the callback; callers
must still protect their own shared handle variables. Resource loading and
creation/destruction remain synchronous. Service startup must obey the platform's
SDL startup requirements. The first stateful audio API call initializes one
process-lived application mutex; reporting and request submission allocate no
per-request storage.

The named `ROHR_AUDIO_REQUEST_CAPACITY` is 256 outstanding requests. Pending and
submitted requests share that capacity; a full queue returns
`ERROR_ENGINE_AUDIO_REQUEST_CAPACITY_EXCEEDED` without accepting the request.
A queued handle destroyed before execution yields a later warning, never a use
of the replacement resource. A later playback failure appears in a later report,
even when the triggering request was accepted several updates earlier.

Stop returns final evidence without consuming its retained copy. The first
subsequent update returns that same report once; later stopped updates return
`NOT_RUN`. Do not merge both copies. Successful restart discards any uncollected
shutdown copy. See [Audio reports](reports.md#audio-reports).

Manual mixing and direct sample output are deferred; no SDL audio handles or
sample-buffer processing API are exposed.

## Sounds

A `Sound` is one independently controlled playback instance. WAV files are
loaded and converted into the mixer's float stereo format once per resolved,
normalized path. Players created from the same path share those immutable
decoded samples while keeping independent cursors and playback settings.

```c
SoundConfig config = rohr_audio_sound_config_default_get();
config.path = "assets/jump.wav";
config.volume = 0.8f;
config.pan = 0.0f;

SoundResult sound = rohr_audio_sound_create(config);
if(!error_check(sound)) {
    EngineResult accepted = rohr_audio_sound_play(sound.result.value);
    /* Check accepted; submit through the regular audio update above. */
    (void)accepted;
}
```

Executing a `rohr_audio_sound_play()` request restarts that instance from its beginning.
Create multiple `Sound` values from the same configuration when the same effect
must overlap itself. Each created sound owns one cache reference until
`rohr_audio_sound_destroy()` or service shutdown. The final player destruction
releases the cached samples.

Volume uses a linear `0..1` range. Pan uses `-1..1`, where `-1` is fully left,
`0` is centered, and `1` is fully right. Values outside either range are
clamped.

Playback rate is a float ratio: `1` is normal, `0.5` is half speed, and `2` is
double speed. Speed and pitch change together. Zero freezes the playback cursor
and a later positive value resumes it. Negative and non-finite values normalize
to zero; reverse playback is not supported.

## Music

A `Music` resource keeps an Ogg Vorbis file and decoder open, then decodes
small blocks as the shared mixer requests them. Music is not fully decoded
into memory. The initial implementation accepts mono and stereo Vorbis streams.

```c
MusicConfig config = rohr_audio_music_config_default_get();
config.path = "assets/theme.ogg";
config.volume = 0.7f;
config.loop = true;

MusicResult music = rohr_audio_music_create(config);
if(!error_check(music)) {
    EngineResult accepted = rohr_audio_music_play(music.result.value);
    /* Check accepted; submit through the regular audio update above. */
    (void)accepted;
}
```

Only one music track is active at a time. Playing a track from the beginning
stops the previously active track. Pause and resume preserve the active
track's stream position; stop resets it to the beginning. Destroying active
music stops it before releasing the decoder and stream buffer.

Each `Music` is a unique instance with its own decoder and stream position;
creating the same path twice does not share or reference-count decoder state.

Music volume and playback rate follow the same rules as Sound. A zero playback
rate freezes the stream but does not mark it as explicitly paused. A later
positive rate continues from the frozen position. `rohr_audio_music_playing_check()`
reports only active, unpaused music with a positive rate, while
`rohr_audio_music_paused_check()` reports the explicit pause state.

## Formats and ownership

Use WAV for short reusable sounds and Ogg Vorbis for streamed music. MP3 and
reverse playback are not supported. The game owns each returned `Sound` and
`Music` handle and should destroy it explicitly by passing its address. A
successful destroy clears the owning handle, destroying a zero handle is
idempotent, and stale nonzero handles are rejected without being cleared.
`rohr_audio_sound_valid_check()` and `rohr_audio_music_valid_check()` distinguish
invalid handles from valid instances whose boolean playback state is false.
Audio service shutdown closes and frees any resources that remain and
invalidates all previous handles.
