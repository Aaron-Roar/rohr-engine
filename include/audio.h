/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stdint.h>

#include "error.h"

#define ROHR_AUDIO_SOUND_LIMIT 128
#define ROHR_AUDIO_MUSIC_LIMIT 16

/** Stable handle for one independently controlled sound playback instance. */
typedef uint32_t Sound;

#define SOUND_INVALID UINT32_C(0)

/** Stable handle for one streamed music resource. */
typedef uint32_t Music;

#define MUSIC_INVALID UINT32_C(0)

/** Values used to load and initialize a WAV sound. */
typedef struct SoundConfig {
    /** WAV file loaded and owned by the created Sound. */
    const char *path;
    /** Linear gain from zero (silent) to one (full volume). */
    float volume;
    /** Stereo position from -1 (left) through 0 (center) to 1 (right). */
    float pan;
    /** Restart at the beginning when playback reaches the end. */
    bool loop;
    /** Speed and pitch ratio. Non-positive values are stored as zero. */
    float playback_rate;
} SoundConfig;

/** Values used to open and initialize one streamed Ogg Vorbis track. */
typedef struct MusicConfig {
    /** Ogg Vorbis file kept open by the created Music. */
    const char *path;
    /** Linear gain from zero (silent) to one (full volume). */
    float volume;
    /** Restart at the beginning when playback reaches the end. */
    bool loop;
    /** Speed and pitch ratio. Non-positive values are stored as zero. */
    float playback_rate;
} MusicConfig;

ERROR_DECLARE_RESULT_TYPE(SoundResult, Sound);
ERROR_DECLARE_RESULT_TYPE(MusicResult, Music);
ERROR_DECLARE_RESULT_TYPE(AudioValueResult, float);

/** Start the optional SDL-backed audio service and its shared mixer. */
EngineResult audio_start(void);
/** Destroy all audio resources and stop the audio service. */
void audio_stop(void);
/** Return whether the audio service is currently started. */
bool audio_started_check(void);

/** Set the shared mixer volume, clamped to 0..1. */
EngineResult audio_volume_set(float volume);
/** Return the shared mixer volume. */
float audio_volume_get(void);

/** Return defaults for one WAV sound playback instance. */
SoundConfig audio_sound_config_default_get(void);
/** Load a WAV and create one independently controlled playback instance. */
SoundResult audio_sound_create(SoundConfig config);
/** Stop and release one sound and its decoded samples. */
EngineResult audio_sound_destroy(Sound sound);
/** Restart one sound from its beginning. */
EngineResult audio_sound_play(Sound sound);
/** Stop one sound and reset it to its beginning. */
EngineResult audio_sound_stop(Sound sound);

EngineResult audio_sound_volume_set(Sound sound, float volume);
AudioValueResult audio_sound_volume_get(Sound sound);
EngineResult audio_sound_pan_set(Sound sound, float pan);
AudioValueResult audio_sound_pan_get(Sound sound);
EngineResult audio_sound_loop_set(Sound sound, bool loop);
bool audio_sound_loop_check(Sound sound);
EngineResult audio_sound_playback_rate_set(Sound sound, float playback_rate);
AudioValueResult audio_sound_playback_rate_get(Sound sound);
/** Return true only while the sound is actively advancing. */
bool audio_sound_playing_check(Sound sound);

/** Return defaults for one streamed Ogg Vorbis track. */
MusicConfig audio_music_config_default_get(void);
/** Open an Ogg Vorbis file for incremental decoding during playback. */
MusicResult audio_music_create(MusicConfig config);
/** Stop and release one music decoder and its stream buffer. */
EngineResult audio_music_destroy(Music music);
/** Start one track from the beginning, replacing any active track. */
EngineResult audio_music_play(Music music);
/** Pause the active track without changing its playback position. */
EngineResult audio_music_pause(Music music);
/** Resume an explicitly paused active track. */
EngineResult audio_music_resume(Music music);
/** Stop one track and reset it to the beginning. */
EngineResult audio_music_stop(Music music);

EngineResult audio_music_volume_set(Music music, float volume);
AudioValueResult audio_music_volume_get(Music music);
EngineResult audio_music_loop_set(Music music, bool loop);
bool audio_music_loop_check(Music music);
/** Set speed and pitch; zero freezes and negative values normalize to zero. */
EngineResult audio_music_playback_rate_set(Music music, float playback_rate);
AudioValueResult audio_music_playback_rate_get(Music music);
/** Return true only while the active track is advancing. */
bool audio_music_playing_check(Music music);
/** Return whether the active track was explicitly paused. */
bool audio_music_paused_check(Music music);

#endif
