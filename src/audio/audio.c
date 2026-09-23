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
#include <string.h>

#define AUDIO_MIX_FREQUENCY 48000
#define AUDIO_MIX_CHANNELS 2
#define AUDIO_MIX_BUFFER_FRAMES 1024
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

static SDL_AudioStream *audio_stream = NULL;
static AudioSound audio_sounds[ROHR_AUDIO_SOUND_LIMIT];
static uint16_t audio_sound_generations[ROHR_AUDIO_SOUND_LIMIT];
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
