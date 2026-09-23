/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "audio.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define STB_VORBIS_HEADER_ONLY
#include "lib/vorbis/stb_vorbis.c"

#define AUDIO_MIX_FREQUENCY 48000
#define AUDIO_MIX_CHANNELS 2
#define AUDIO_MIX_BUFFER_FRAMES 1024
#define AUDIO_MUSIC_DECODE_FRAMES 4096
#define AUDIO_HANDLE_SLOT_MASK UINT32_C(0xffff)
#define AUDIO_HANDLE_GENERATION_SHIFT 16

typedef struct AudioSound {
    Sound id;
    float *samples;
    size_t frame_count;
    double cursor;
    float volume;
    float pan;
    float playback_rate;
    bool loop;
    bool playing;
    bool used;
} AudioSound;

typedef struct AudioMusic {
    Music id;
    stb_vorbis *decoder;
    float *decode_buffer;
    size_t decode_frame_count;
    size_t decode_frame_index;
    unsigned int source_frequency;
    unsigned int source_frame_count;
    unsigned int current_frame_position;
    int source_channels;
    double phase;
    float current_frame[AUDIO_MIX_CHANNELS];
    float next_frame[AUDIO_MIX_CHANNELS];
    float volume;
    float playback_rate;
    bool loop;
    bool playing;
    bool paused;
    bool current_frame_ready;
    bool end_pending;
    bool used;
} AudioMusic;

static SDL_AudioStream *audio_stream = NULL;
static AudioSound audio_sounds[ROHR_AUDIO_SOUND_LIMIT];
static uint16_t audio_sound_generations[ROHR_AUDIO_SOUND_LIMIT];
static AudioMusic audio_music[ROHR_AUDIO_MUSIC_LIMIT];
static uint16_t audio_music_generations[ROHR_AUDIO_MUSIC_LIMIT];
static Music audio_active_music = MUSIC_INVALID;
static float audio_master_volume = 1.0f;
static bool audio_started = false;

static float audio_unit_value_get(float value) {
    if(!(value > 0.0f)) return 0.0f;
    if(value > 1.0f) return 1.0f;
    return value;
}

static float audio_pan_value_get(float value) {
    if(value < -1.0f) return -1.0f;
    if(value > 1.0f) return 1.0f;
    if(!(value >= -1.0f && value <= 1.0f)) return 0.0f;
    return value;
}

static float audio_playback_rate_value_get(float value) {
    if(!(value > 0.0f) || value > FLT_MAX) return 0.0f;
    return value;
}

static Sound audio_sound_id_create(size_t slot) {
    return ((uint32_t)audio_sound_generations[slot]
        << AUDIO_HANDLE_GENERATION_SHIFT) | (uint32_t)(slot + 1);
}

static AudioSound *audio_sound_get(Sound sound) {
    uint32_t stored_slot;
    size_t slot;
    if(sound == SOUND_INVALID) return NULL;
    stored_slot = sound & AUDIO_HANDLE_SLOT_MASK;
    if(stored_slot == 0) return NULL;
    slot = (size_t)(stored_slot - 1);
    if(slot >= ROHR_AUDIO_SOUND_LIMIT || !audio_sounds[slot].used ||
            audio_sounds[slot].id != sound) return NULL;
    return &audio_sounds[slot];
}

static Music audio_music_id_create(size_t slot) {
    return ((uint32_t)audio_music_generations[slot]
        << AUDIO_HANDLE_GENERATION_SHIFT) | (uint32_t)(slot + 1);
}

static AudioMusic *audio_music_get(Music music) {
    uint32_t stored_slot;
    size_t slot;
    if(music == MUSIC_INVALID) return NULL;
    stored_slot = music & AUDIO_HANDLE_SLOT_MASK;
    if(stored_slot == 0) return NULL;
    slot = (size_t)(stored_slot - 1);
    if(slot >= ROHR_AUDIO_MUSIC_LIMIT || !audio_music[slot].used ||
            audio_music[slot].id != music) return NULL;
    return &audio_music[slot];
}

static bool audio_lock(void) {
    return audio_stream != NULL && SDL_LockAudioStream(audio_stream);
}

static void audio_unlock(void) {
    if(audio_stream != NULL) (void)SDL_UnlockAudioStream(audio_stream);
}

static void audio_sound_cursor_resolve(AudioSound *sound) {
    if(sound->frame_count == 0) {
        sound->playing = false;
        sound->cursor = 0.0;
        return;
    }
    if(sound->cursor < (double)sound->frame_count) return;
    if(sound->loop) {
        sound->cursor = fmod(sound->cursor, (double)sound->frame_count);
    } else {
        sound->playing = false;
        sound->cursor = (double)sound->frame_count;
    }
}

static void audio_sound_mix(
        AudioSound *sound,
        float *destination,
        size_t frame_count) {
    float left_pan = sound->pan > 0.0f ? 1.0f - sound->pan : 1.0f;
    float right_pan = sound->pan < 0.0f ? 1.0f + sound->pan : 1.0f;
    float gain = sound->volume * audio_master_volume;

    if(!sound->playing || sound->playback_rate == 0.0f) return;
    for(size_t frame = 0; frame < frame_count; frame += 1) {
        size_t sample_frame;
        size_t next_frame;
        float fraction;
        float left;
        float right;
        audio_sound_cursor_resolve(sound);
        if(!sound->playing) break;
        sample_frame = (size_t)sound->cursor;
        next_frame = sample_frame + 1;
        if(next_frame >= sound->frame_count)
            next_frame = sound->loop ? 0 : sample_frame;
        fraction = (float)(sound->cursor - (double)sample_frame);
        left = sound->samples[sample_frame * AUDIO_MIX_CHANNELS] +
            (sound->samples[next_frame * AUDIO_MIX_CHANNELS] -
                sound->samples[sample_frame * AUDIO_MIX_CHANNELS]) * fraction;
        right = sound->samples[sample_frame * AUDIO_MIX_CHANNELS + 1] +
            (sound->samples[next_frame * AUDIO_MIX_CHANNELS + 1] -
                sound->samples[sample_frame * AUDIO_MIX_CHANNELS + 1]) * fraction;
        destination[frame * AUDIO_MIX_CHANNELS] += left * gain * left_pan;
        destination[frame * AUDIO_MIX_CHANNELS + 1] += right * gain * right_pan;
        sound->cursor += sound->playback_rate;
    }
}

static void audio_music_decode_state_reset(AudioMusic *music) {
    music->decode_frame_count = 0;
    music->decode_frame_index = 0;
    music->phase = 0.0;
    music->current_frame_position = 0;
    music->current_frame_ready = false;
    music->end_pending = false;
}

static void audio_music_playback_clear(AudioMusic *music) {
    music->playing = false;
    music->paused = false;
    music->current_frame_ready = false;
    music->end_pending = false;
    if(audio_active_music == music->id) audio_active_music = MUSIC_INVALID;
}

static bool audio_music_decode_refill(AudioMusic *music) {
    for(size_t attempt = 0; attempt < 2; attempt += 1) {
        int decoded = stb_vorbis_get_samples_float_interleaved(
            music->decoder,
            music->source_channels,
            music->decode_buffer,
            AUDIO_MUSIC_DECODE_FRAMES * music->source_channels);
        if(decoded > 0) {
            music->decode_frame_count = (size_t)decoded;
            music->decode_frame_index = 0;
            return true;
        }
        if(!music->loop || attempt > 0 ||
                !stb_vorbis_seek_start(music->decoder)) break;
    }
    music->decode_frame_count = 0;
    music->decode_frame_index = 0;
    return false;
}

static bool audio_music_source_frame_get(
        AudioMusic *music,
        float output[AUDIO_MIX_CHANNELS]) {
    size_t offset;
    if(music->decode_frame_index >= music->decode_frame_count &&
            !audio_music_decode_refill(music)) return false;
    offset = music->decode_frame_index * (size_t)music->source_channels;
    output[0] = music->decode_buffer[offset];
    output[1] = music->source_channels == 1 ? output[0] :
        music->decode_buffer[offset + 1];
    music->decode_frame_index += 1;
    return true;
}

static bool audio_music_frames_prepare(AudioMusic *music) {
    if(music->current_frame_ready) return true;
    if(!audio_music_source_frame_get(music, music->current_frame)) return false;
    if(!audio_music_source_frame_get(music, music->next_frame)) {
        memcpy(music->next_frame, music->current_frame,
            sizeof(music->next_frame));
        music->end_pending = true;
    }
    music->current_frame_ready = true;
    return true;
}

static bool audio_music_position_set(
        AudioMusic *music,
        unsigned int position) {
    if(!stb_vorbis_seek(music->decoder, position)) return false;
    music->decode_frame_count = 0;
    music->decode_frame_index = 0;
    music->current_frame_position = position;
    music->current_frame_ready = false;
    music->end_pending = false;
    return audio_music_frames_prepare(music);
}

static bool audio_music_frame_advance(AudioMusic *music) {
    if(music->end_pending) return false;
    memcpy(music->current_frame, music->next_frame,
        sizeof(music->current_frame));
    music->current_frame_position += 1;
    if(music->current_frame_position >= music->source_frame_count)
        music->current_frame_position = 0;
    if(!audio_music_source_frame_get(music, music->next_frame)) {
        memcpy(music->next_frame, music->current_frame,
            sizeof(music->next_frame));
        music->end_pending = true;
    }
    return true;
}

static bool audio_music_frames_advance(AudioMusic *music, uint64_t frames) {
    if(frames > AUDIO_MUSIC_DECODE_FRAMES) {
        uint64_t target = (uint64_t)music->current_frame_position + frames;
        if(!music->loop && target >= music->source_frame_count) return false;
        if(music->loop) target %= music->source_frame_count;
        return audio_music_position_set(music, (unsigned int)target);
    }
    while(frames > 0) {
        if(!audio_music_frame_advance(music)) return false;
        frames -= 1;
    }
    return true;
}

static void audio_music_mix(
        AudioMusic *music,
        float *destination,
        size_t frame_count) {
    double step = ((double)music->source_frequency /
        (double)AUDIO_MIX_FREQUENCY) * (double)music->playback_rate;
    float gain = music->volume * audio_master_volume;

    if(!music->playing || music->paused || music->playback_rate == 0.0f)
        return;
    if(!audio_music_frames_prepare(music)) {
        audio_music_playback_clear(music);
        return;
    }
    for(size_t frame = 0; frame < frame_count; frame += 1) {
        float fraction = (float)music->phase;
        destination[frame * AUDIO_MIX_CHANNELS] +=
            (music->current_frame[0] +
                (music->next_frame[0] - music->current_frame[0]) * fraction) *
            gain;
        destination[frame * AUDIO_MIX_CHANNELS + 1] +=
            (music->current_frame[1] +
                (music->next_frame[1] - music->current_frame[1]) * fraction) *
            gain;
        music->phase += step;
        if(music->phase >= 1.0) {
            uint64_t whole_frames;
            if(music->phase >= (double)UINT64_MAX) {
                if(!music->loop) {
                    audio_music_playback_clear(music);
                    break;
                }
                music->phase = fmod(music->phase,
                    (double)music->source_frame_count);
            }
            whole_frames = (uint64_t)music->phase;
            music->phase -= (double)whole_frames;
            if(!audio_music_frames_advance(music, whole_frames)) {
                audio_music_playback_clear(music);
                break;
            }
        }
    }
}

static void SDLCALL audio_mix_callback(
        void *userdata,
        SDL_AudioStream *stream,
        int additional_amount,
        int total_amount) {
    float mixed[AUDIO_MIX_BUFFER_FRAMES * AUDIO_MIX_CHANNELS];
    const size_t frame_bytes = sizeof(float) * AUDIO_MIX_CHANNELS;
    size_t frames_remaining;
    (void)userdata;
    (void)total_amount;

    if(additional_amount <= 0) return;
    frames_remaining = ((size_t)additional_amount + frame_bytes - 1) /
        frame_bytes;
    while(frames_remaining > 0) {
        size_t frames = frames_remaining > AUDIO_MIX_BUFFER_FRAMES ?
            AUDIO_MIX_BUFFER_FRAMES : frames_remaining;
        size_t sample_count = frames * AUDIO_MIX_CHANNELS;
        memset(mixed, 0, sample_count * sizeof(*mixed));
        for(size_t slot = 0; slot < ROHR_AUDIO_SOUND_LIMIT; slot += 1)
            if(audio_sounds[slot].used)
                audio_sound_mix(&audio_sounds[slot], mixed, frames);
        if(audio_active_music != MUSIC_INVALID) {
            AudioMusic *music = audio_music_get(audio_active_music);
            if(music != NULL) audio_music_mix(music, mixed, frames);
        }
        for(size_t sample = 0; sample < sample_count; sample += 1) {
            if(mixed[sample] > 1.0f) mixed[sample] = 1.0f;
            else if(mixed[sample] < -1.0f) mixed[sample] = -1.0f;
        }
        if(!SDL_PutAudioStreamData(
                stream, mixed, (int)(sample_count * sizeof(*mixed)))) return;
        frames_remaining -= frames;
    }
}

EngineResult audio_start(void) {
    SDL_AudioSpec mix_spec = {
        .format = SDL_AUDIO_F32,
        .channels = AUDIO_MIX_CHANNELS,
        .freq = AUDIO_MIX_FREQUENCY,
    };
    if(audio_started) return error_result_error(ERROR_ENGINE_AUDIO_ALREADY_STARTED);

    SDL_SetMainReady();
    if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_INIT_FAILED, SDL_GetError());
    memset(audio_sounds, 0, sizeof(audio_sounds));
    memset(audio_music, 0, sizeof(audio_music));
    audio_active_music = MUSIC_INVALID;
    audio_master_volume = 1.0f;
    audio_stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &mix_spec,
        audio_mix_callback,
        NULL);
    if(audio_stream == NULL) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return error_result_error_detail(ERROR_ENGINE_AUDIO_INIT_FAILED,
            SDL_GetError());
    }
    if(!SDL_ResumeAudioStreamDevice(audio_stream)) {
        char detail[256];
        SDL_snprintf(detail, sizeof(detail), "%s", SDL_GetError());
        SDL_DestroyAudioStream(audio_stream);
        audio_stream = NULL;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return error_result_error_detail(ERROR_ENGINE_AUDIO_INIT_FAILED, detail);
    }
    audio_started = true;
    return error_result_value(true);
}

void audio_stop(void) {
    if(!audio_started) return;
    if(audio_stream != NULL) {
        SDL_DestroyAudioStream(audio_stream);
        audio_stream = NULL;
    }
    for(size_t slot = 0; slot < ROHR_AUDIO_SOUND_LIMIT; slot += 1) {
        if(audio_sounds[slot].samples != NULL)
            SDL_free(audio_sounds[slot].samples);
        audio_sounds[slot] = (AudioSound){0};
    }
    for(size_t slot = 0; slot < ROHR_AUDIO_MUSIC_LIMIT; slot += 1) {
        if(audio_music[slot].decoder != NULL)
            stb_vorbis_close(audio_music[slot].decoder);
        if(audio_music[slot].decode_buffer != NULL)
            SDL_free(audio_music[slot].decode_buffer);
        audio_music[slot] = (AudioMusic){0};
    }
    audio_active_music = MUSIC_INVALID;
    audio_master_volume = 1.0f;
    audio_started = false;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

bool audio_started_check(void) { return audio_started; }

EngineResult audio_volume_set(float volume) {
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    audio_master_volume = audio_unit_value_get(volume);
    audio_unlock();
    return error_result_value(true);
}

float audio_volume_get(void) {
    float result = audio_master_volume;
    if(audio_started && audio_lock()) {
        result = audio_master_volume;
        audio_unlock();
    }
    return result;
}

SoundConfig audio_sound_config_default_get(void) {
    return (SoundConfig){
        .path = NULL,
        .volume = 1.0f,
        .pan = 0.0f,
        .loop = false,
        .playback_rate = 1.0f,
    };
}

SoundResult audio_sound_create(SoundConfig config) {
    SDL_AudioSpec source_spec;
    SDL_AudioSpec mix_spec = {
        .format = SDL_AUDIO_F32,
        .channels = AUDIO_MIX_CHANNELS,
        .freq = AUDIO_MIX_FREQUENCY,
    };
    Uint8 *source_samples = NULL;
    Uint8 *converted_samples = NULL;
    Uint32 source_length = 0;
    int converted_length = 0;
    size_t slot;

    if(!audio_started) return ERROR_RESULT_MAKE_ERROR(
        SoundResult, ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(config.path == NULL || config.path[0] == '\0')
        return ERROR_RESULT_MAKE_ERROR(
            SoundResult, ERROR_ENGINE_AUDIO_SOUND_CONFIG_INVALID);
    for(slot = 0; slot < ROHR_AUDIO_SOUND_LIMIT; slot += 1)
        if(!audio_sounds[slot].used) break;
    if(slot == ROHR_AUDIO_SOUND_LIMIT) return ERROR_RESULT_MAKE_ERROR(
        SoundResult, ERROR_ENGINE_AUDIO_SOUND_CAPACITY_EXCEEDED);
    if(!SDL_LoadWAV(config.path, &source_spec, &source_samples, &source_length))
        return (error_detail_set(
            ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED, SDL_GetError()),
            ERROR_RESULT_MAKE_ERROR(
                SoundResult, ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED));
    if(source_length > (Uint32)INT_MAX || !SDL_ConvertAudioSamples(
            &source_spec,
            source_samples,
            (int)source_length,
            &mix_spec,
            &converted_samples,
            &converted_length)) {
        const char *detail = SDL_GetError();
        SDL_free(source_samples);
        error_detail_set(ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED, detail);
        return ERROR_RESULT_MAKE_ERROR(
            SoundResult, ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED);
    }
    SDL_free(source_samples);
    if(converted_length <= 0 ||
            converted_length % (int)(sizeof(float) * AUDIO_MIX_CHANNELS) != 0) {
        SDL_free(converted_samples);
        return ERROR_RESULT_MAKE_ERROR(
            SoundResult, ERROR_ENGINE_AUDIO_SOUND_CONFIG_INVALID);
    }

    if(!audio_lock()) {
        SDL_free(converted_samples);
        return (error_detail_set(ERROR_ENGINE_AUDIO_OPERATION_FAILED,
            SDL_GetError()), ERROR_RESULT_MAKE_ERROR(
                SoundResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED));
    }
    audio_sound_generations[slot] += 1;
    if(audio_sound_generations[slot] == 0) audio_sound_generations[slot] = 1;
    audio_sounds[slot] = (AudioSound){
        .id = audio_sound_id_create(slot),
        .samples = (float *)converted_samples,
        .frame_count = (size_t)converted_length /
            (sizeof(float) * AUDIO_MIX_CHANNELS),
        .volume = audio_unit_value_get(config.volume),
        .pan = audio_pan_value_get(config.pan),
        .playback_rate = audio_playback_rate_value_get(config.playback_rate),
        .loop = config.loop,
        .used = true,
    };
    audio_unlock();
    return ERROR_RESULT_MAKE_VALUE(SoundResult, audio_sounds[slot].id);
}

EngineResult audio_sound_destroy(Sound sound) {
    AudioSound *value;
    float *samples;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_sound_get(sound);
    if(value == NULL) {
        audio_unlock();
        return error_result_error(ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND);
    }
    samples = value->samples;
    *value = (AudioSound){0};
    audio_unlock();
    SDL_free(samples);
    return error_result_value(true);
}

EngineResult audio_sound_play(Sound sound) {
    AudioSound *value;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_sound_get(sound);
    if(value == NULL) {
        audio_unlock();
        return error_result_error(ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND);
    }
    value->cursor = 0.0;
    value->playing = true;
    audio_unlock();
    return error_result_value(true);
}

EngineResult audio_sound_stop(Sound sound) {
    AudioSound *value;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_sound_get(sound);
    if(value == NULL) {
        audio_unlock();
        return error_result_error(ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND);
    }
    value->cursor = 0.0;
    value->playing = false;
    audio_unlock();
    return error_result_value(true);
}

EngineResult audio_sound_volume_set(Sound sound, float volume) {
    AudioSound *value;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_sound_get(sound);
    if(value != NULL) value->volume = audio_unit_value_get(volume);
    audio_unlock();
    return value == NULL ? error_result_error(ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND) :
        error_result_value(true);
}

AudioValueResult audio_sound_volume_get(Sound sound) {
    AudioSound *value;
    float result;
    if(!audio_started) return ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED);
    value = audio_sound_get(sound);
    result = value == NULL ? 0.0f : value->volume;
    audio_unlock();
    return value == NULL ? ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND) :
        ERROR_RESULT_MAKE_VALUE(AudioValueResult, result);
}

EngineResult audio_sound_pan_set(Sound sound, float pan) {
    AudioSound *value;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_sound_get(sound);
    if(value != NULL) value->pan = audio_pan_value_get(pan);
    audio_unlock();
    return value == NULL ? error_result_error(ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND) :
        error_result_value(true);
}

AudioValueResult audio_sound_pan_get(Sound sound) {
    AudioSound *value;
    float result;
    if(!audio_started) return ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED);
    value = audio_sound_get(sound);
    result = value == NULL ? 0.0f : value->pan;
    audio_unlock();
    return value == NULL ? ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND) :
        ERROR_RESULT_MAKE_VALUE(AudioValueResult, result);
}

EngineResult audio_sound_loop_set(Sound sound, bool loop) {
    AudioSound *value;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_sound_get(sound);
    if(value != NULL) value->loop = loop;
    audio_unlock();
    return value == NULL ? error_result_error(ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND) :
        error_result_value(true);
}

bool audio_sound_loop_check(Sound sound) {
    AudioSound *value;
    bool result;
    if(!audio_started || !audio_lock()) return false;
    value = audio_sound_get(sound);
    result = value != NULL && value->loop;
    audio_unlock();
    return result;
}

EngineResult audio_sound_playback_rate_set(Sound sound, float playback_rate) {
    AudioSound *value;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_sound_get(sound);
    if(value != NULL)
        value->playback_rate = audio_playback_rate_value_get(playback_rate);
    audio_unlock();
    return value == NULL ? error_result_error(ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND) :
        error_result_value(true);
}

AudioValueResult audio_sound_playback_rate_get(Sound sound) {
    AudioSound *value;
    float result;
    if(!audio_started) return ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED);
    value = audio_sound_get(sound);
    result = value == NULL ? 0.0f : value->playback_rate;
    audio_unlock();
    return value == NULL ? ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND) :
        ERROR_RESULT_MAKE_VALUE(AudioValueResult, result);
}

bool audio_sound_playing_check(Sound sound) {
    AudioSound *value;
    bool result;
    if(!audio_started || !audio_lock()) return false;
    value = audio_sound_get(sound);
    result = value != NULL && value->playing && value->playback_rate > 0.0f;
    audio_unlock();
    return result;
}

MusicConfig audio_music_config_default_get(void) {
    return (MusicConfig){
        .path = NULL,
        .volume = 1.0f,
        .loop = false,
        .playback_rate = 1.0f,
    };
}

MusicResult audio_music_create(MusicConfig config) {
    stb_vorbis *decoder;
    stb_vorbis_info info;
    float *decode_buffer;
    unsigned int source_frame_count;
    int decoder_error = 0;
    size_t slot;
    char detail[96];

    if(!audio_started) return ERROR_RESULT_MAKE_ERROR(
        MusicResult, ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(config.path == NULL || config.path[0] == '\0')
        return ERROR_RESULT_MAKE_ERROR(
            MusicResult, ERROR_ENGINE_AUDIO_MUSIC_CONFIG_INVALID);
    for(slot = 0; slot < ROHR_AUDIO_MUSIC_LIMIT; slot += 1)
        if(!audio_music[slot].used) break;
    if(slot == ROHR_AUDIO_MUSIC_LIMIT) return ERROR_RESULT_MAKE_ERROR(
        MusicResult, ERROR_ENGINE_AUDIO_MUSIC_CAPACITY_EXCEEDED);

    decoder = stb_vorbis_open_filename(config.path, &decoder_error, NULL);
    if(decoder == NULL) {
        snprintf(detail, sizeof(detail), "stb_vorbis error %d", decoder_error);
        error_detail_set(ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED, detail);
        return ERROR_RESULT_MAKE_ERROR(
            MusicResult, ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED);
    }
    info = stb_vorbis_get_info(decoder);
    source_frame_count = stb_vorbis_stream_length_in_samples(decoder);
    if(info.sample_rate == 0 || (info.channels != 1 && info.channels != 2) ||
            source_frame_count == 0) {
        stb_vorbis_close(decoder);
        error_detail_set(ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED,
            "music must contain mono or stereo Vorbis samples");
        return ERROR_RESULT_MAKE_ERROR(
            MusicResult, ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED);
    }
    decode_buffer = SDL_malloc(AUDIO_MUSIC_DECODE_FRAMES *
        (size_t)info.channels * sizeof(*decode_buffer));
    if(decode_buffer == NULL) {
        stb_vorbis_close(decoder);
        error_detail_set(ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED,
            "music decode buffer allocation failed");
        return ERROR_RESULT_MAKE_ERROR(
            MusicResult, ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED);
    }
    if(!audio_lock()) {
        SDL_free(decode_buffer);
        stb_vorbis_close(decoder);
        error_detail_set(ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
        return ERROR_RESULT_MAKE_ERROR(
            MusicResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED);
    }
    audio_music_generations[slot] += 1;
    if(audio_music_generations[slot] == 0) audio_music_generations[slot] = 1;
    audio_music[slot] = (AudioMusic){
        .id = audio_music_id_create(slot),
        .decoder = decoder,
        .decode_buffer = decode_buffer,
        .source_frequency = info.sample_rate,
        .source_frame_count = source_frame_count,
        .source_channels = info.channels,
        .volume = audio_unit_value_get(config.volume),
        .playback_rate = audio_playback_rate_value_get(config.playback_rate),
        .loop = config.loop,
        .used = true,
    };
    audio_unlock();
    return ERROR_RESULT_MAKE_VALUE(MusicResult, audio_music[slot].id);
}

EngineResult audio_music_destroy(Music music) {
    AudioMusic *value;
    stb_vorbis *decoder;
    float *decode_buffer;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_music_get(music);
    if(value == NULL) {
        audio_unlock();
        return error_result_error(ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND);
    }
    if(audio_active_music == music) audio_active_music = MUSIC_INVALID;
    decoder = value->decoder;
    decode_buffer = value->decode_buffer;
    *value = (AudioMusic){0};
    audio_unlock();
    stb_vorbis_close(decoder);
    SDL_free(decode_buffer);
    return error_result_value(true);
}

EngineResult audio_music_play(Music music) {
    AudioMusic *value;
    AudioMusic *active;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_music_get(music);
    if(value == NULL) {
        audio_unlock();
        return error_result_error(ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND);
    }
    if(!stb_vorbis_seek_start(value->decoder)) {
        audio_unlock();
        return error_result_error_detail(ERROR_ENGINE_AUDIO_OPERATION_FAILED,
            "could not seek music to its beginning");
    }
    active = audio_music_get(audio_active_music);
    if(active != NULL) audio_music_playback_clear(active);
    audio_music_decode_state_reset(value);
    value->playing = true;
    value->paused = false;
    audio_active_music = music;
    audio_unlock();
    return error_result_value(true);
}

EngineResult audio_music_pause(Music music) {
    AudioMusic *value;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_music_get(music);
    if(value == NULL) {
        audio_unlock();
        return error_result_error(ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND);
    }
    if(audio_active_music != music || !value->playing) {
        audio_unlock();
        return error_result_error(ERROR_ENGINE_AUDIO_MUSIC_NOT_ACTIVE);
    }
    value->paused = true;
    audio_unlock();
    return error_result_value(true);
}

EngineResult audio_music_resume(Music music) {
    AudioMusic *value;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_music_get(music);
    if(value == NULL) {
        audio_unlock();
        return error_result_error(ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND);
    }
    if(audio_active_music != music || !value->playing) {
        audio_unlock();
        return error_result_error(ERROR_ENGINE_AUDIO_MUSIC_NOT_ACTIVE);
    }
    value->paused = false;
    audio_unlock();
    return error_result_value(true);
}

EngineResult audio_music_stop(Music music) {
    AudioMusic *value;
    bool seeked;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_music_get(music);
    if(value == NULL) {
        audio_unlock();
        return error_result_error(ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND);
    }
    seeked = stb_vorbis_seek_start(value->decoder) != 0;
    audio_music_playback_clear(value);
    audio_music_decode_state_reset(value);
    audio_unlock();
    return seeked ? error_result_value(true) : error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED,
        "could not reset music to its beginning");
}

EngineResult audio_music_volume_set(Music music, float volume) {
    AudioMusic *value;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_music_get(music);
    if(value != NULL) value->volume = audio_unit_value_get(volume);
    audio_unlock();
    return value == NULL ? error_result_error(ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND) :
        error_result_value(true);
}

AudioValueResult audio_music_volume_get(Music music) {
    AudioMusic *value;
    float result;
    if(!audio_started) return ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED);
    value = audio_music_get(music);
    result = value == NULL ? 0.0f : value->volume;
    audio_unlock();
    return value == NULL ? ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND) :
        ERROR_RESULT_MAKE_VALUE(AudioValueResult, result);
}

EngineResult audio_music_loop_set(Music music, bool loop) {
    AudioMusic *value;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_music_get(music);
    if(value != NULL) value->loop = loop;
    audio_unlock();
    return value == NULL ? error_result_error(ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND) :
        error_result_value(true);
}

bool audio_music_loop_check(Music music) {
    AudioMusic *value;
    bool result;
    if(!audio_started || !audio_lock()) return false;
    value = audio_music_get(music);
    result = value != NULL && value->loop;
    audio_unlock();
    return result;
}

EngineResult audio_music_playback_rate_set(Music music, float playback_rate) {
    AudioMusic *value;
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_music_get(music);
    if(value != NULL)
        value->playback_rate = audio_playback_rate_value_get(playback_rate);
    audio_unlock();
    return value == NULL ? error_result_error(ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND) :
        error_result_value(true);
}

AudioValueResult audio_music_playback_rate_get(Music music) {
    AudioMusic *value;
    float result;
    if(!audio_started) return ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED);
    value = audio_music_get(music);
    result = value == NULL ? 0.0f : value->playback_rate;
    audio_unlock();
    return value == NULL ? ERROR_RESULT_MAKE_ERROR(
        AudioValueResult, ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND) :
        ERROR_RESULT_MAKE_VALUE(AudioValueResult, result);
}

bool audio_music_playing_check(Music music) {
    AudioMusic *value;
    bool result;
    if(!audio_started || !audio_lock()) return false;
    value = audio_music_get(music);
    result = value != NULL && audio_active_music == music && value->playing &&
        !value->paused && value->playback_rate > 0.0f;
    audio_unlock();
    return result;
}

bool audio_music_paused_check(Music music) {
    AudioMusic *value;
    bool result;
    if(!audio_started || !audio_lock()) return false;
    value = audio_music_get(music);
    result = value != NULL && audio_active_music == music && value->playing &&
        value->paused;
    audio_unlock();
    return result;
}
