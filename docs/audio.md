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

Music will use the same mixer while exposing streaming controls suited to Ogg
Vorbis tracks. Its decoder and public API are not part of the WAV Sound
foundation yet.
