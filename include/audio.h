/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <stdint.h>

#include "error.h"
#include "report.h"

#define ROHR_AUDIO_REQUEST_CAPACITY 256u
#define ROHR_AUDIO_SOUND_LIMIT 128
#define ROHR_AUDIO_MUSIC_LIMIT 16

/** Stable handle for one independently controlled sound playback instance. */
typedef uint32_t Sound;

#define SOUND_INVALID UINT32_C(0)

/** Stable handle for one streamed music resource. */
typedef uint32_t Music;

#define MUSIC_INVALID UINT32_C(0)

/** Values used to load and initialize a WAV sound. */
typedef struct SoundDescription {
    /** WAV file decoded into a cache shared by matching Sound players. */
    const char *path;
    /** Linear gain from zero (silent) to one (full volume). */
    float volume;
    /** Stereo position from -1 (left) through 0 (center) to 1 (right). */
    float pan;
    /** Restart at the beginning when playback reaches the end. */
    bool loop;
    /** Speed and pitch ratio. Non-positive values are stored as zero. */
    float playback_rate;
} SoundDescription;

/** Values used to open and initialize one streamed Ogg Vorbis track. */
typedef struct MusicDescription {
    /** Ogg Vorbis file kept open by the created Music. */
    const char *path;
    /** Linear gain from zero (silent) to one (full volume). */
    float volume;
    /** Restart at the beginning when playback reaches the end. */
    bool loop;
    /** Speed and pitch ratio. Non-positive values are stored as zero. */
    float playback_rate;
} MusicDescription;

ERROR_DECLARE_RESULT_TYPE(SoundResult, Sound);
ERROR_DECLARE_RESULT_TYPE(MusicResult, Music);
ERROR_DECLARE_RESULT_TYPE(AudioValueResult, float);
ERROR_DECLARE_RESULT_TYPE(SoundDescriptionResult, SoundDescription);
ERROR_DECLARE_RESULT_TYPE(MusicDescriptionResult, MusicDescription);

/**
 * All runtime controls enqueue requests, including master volume. Call
 * audio_update() to submit them to the callback. Success means accepted;
 * getters observe applied state. Concurrent callers are serialized in enqueue
 * order. Creation/destruction and service start/stop remain synchronous.
 * Callers must synchronize access to their own handle variables.
 */

/** Start the optional SDL-backed audio service and its shared mixer. */
EngineResult audio_start(void);
/** Stop synchronously and return final evidence without consuming it.
 * The next update returns the same report once. Restart discards that copy. */
Report audio_stop(void);
/** Submit pending controls and collect completed work; never waits for playback. */
Report audio_update(void);
/** Return whether the audio service is currently started. */
bool audio_started_check(void);

/** Set the shared mixer volume, clamped to 0..1. */
EngineResult audio_volume_set(float volume);
/** Return the shared mixer volume. */
float audio_volume_get(void);

/** Return muted defaults: volume zero, centered pan, rate one, no looping. */
SoundDescription audio_sound_description_default_get(void);
/** Load a WAV and create one independently controlled playback instance. */
SoundResult audio_sound_create(SoundDescription config);
/** Prepare a complete replacement and enqueue one atomic update. Same-source
 * edits preserve progress; a changed source is installed stopped. Paths are
 * required and copied; failures leave existing state and queued work intact. */
EngineResult audio_sound_description_set(Sound sound, SoundDescription description);
/** Applied configuration, with a borrowed resolved path. The path remains valid
 * until a subsequent update, creation, destruction, full setter, or shutdown.
 * Synchronize those calls with users of borrowed paths on other threads. */
SoundDescriptionResult audio_sound_description_get(Sound sound);
/** Stop and release one sound owner, clearing it on success. */
EngineResult audio_sound_destroy(Sound *sound);
/** Return whether a sound player handle is currently valid. */
bool audio_sound_valid_check(Sound sound);
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

/** Return muted defaults: volume zero, rate one, no looping. */
MusicDescription audio_music_description_default_get(void);
/** Open an Ogg Vorbis file for incremental decoding during playback. */
MusicResult audio_music_create(MusicDescription config);
/** Prepare a complete replacement and enqueue one atomic update. Same-source
 * edits preserve progress; a changed source is installed stopped. Paths are
 * required and copied; failures leave existing state and queued work intact. */
EngineResult audio_music_description_set(Music music, MusicDescription description);
/** Applied configuration, with a borrowed resolved path. The path remains valid
 * until a subsequent update, creation, destruction, full setter, or shutdown.
 * Synchronize those calls with users of borrowed paths on other threads. */
MusicDescriptionResult audio_music_description_get(Music music);
/** Stop and release one music owner, clearing it on success. */
EngineResult audio_music_destroy(Music *music);
/** Return whether a streamed music handle is currently valid. */
bool audio_music_valid_check(Music music);
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
