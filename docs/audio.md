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

`rohr_audio_stop()` destroys every remaining audio resource. Stopping the core
engine also stops audio before SDL shuts down.

## Sounds

A `Sound` is one independently controlled playback instance. The initial
implementation loads WAV files and converts them into the mixer's float stereo
format when the sound is created.

```c
SoundConfig config = rohr_audio_sound_config_default_get();
config.path = "assets/jump.wav";
config.volume = 0.8f;
config.pan = 0.0f;

SoundResult sound = rohr_audio_sound_create(config);
if(!error_check(sound)) rohr_audio_sound_play(sound.result.value);
```

Calling `rohr_audio_sound_play()` restarts that instance from its beginning.
Create multiple `Sound` values from the same configuration when the same effect
must overlap itself. Each created sound owns its decoded samples until
`rohr_audio_sound_destroy()` or service shutdown.

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
if(!error_check(music)) rohr_audio_music_play(music.result.value);
```

Only one music track is active at a time. Playing a track from the beginning
stops the previously active track. Pause and resume preserve the active
track's stream position; stop resets it to the beginning. Destroying active
music stops it before releasing the decoder and stream buffer.

Music volume and playback rate follow the same rules as Sound. A zero playback
rate freezes the stream but does not mark it as explicitly paused. A later
positive rate continues from the frozen position. `rohr_audio_music_playing_check()`
reports only active, unpaused music with a positive rate, while
`rohr_audio_music_paused_check()` reports the explicit pause state.

## Formats and ownership

Use WAV for short reusable sounds and Ogg Vorbis for streamed music. MP3 and
reverse playback are not supported. The game owns each returned `Sound` and
`Music` handle and should destroy it explicitly. Audio service shutdown closes
and frees any resources that remain.
