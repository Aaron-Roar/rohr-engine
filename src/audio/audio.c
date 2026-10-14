/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "audio.h"
#include "audio/audio_internal.h"
#include "core/report_internal.h"
#include "graphics/asset_path.h"

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

typedef struct AudioSoundResource {
    float *samples;
    char *path;
    size_t frame_count;
    size_t references;
    bool used;
} AudioSoundResource;

typedef struct AudioSound {
    AudioSoundResource *resource;
    Sound id;
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
    char *path;
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
    bool decode_failed;
    bool current_frame_ready;
    bool end_pending;
    bool used;
} AudioMusic;

static SDL_AudioStream *audio_stream = NULL;
static AudioSound audio_sounds[ROHR_AUDIO_SOUND_LIMIT];
static AudioSoundResource audio_sound_resources[ROHR_AUDIO_SOUND_LIMIT];
static uint16_t audio_sound_generations[ROHR_AUDIO_SOUND_LIMIT];
static AudioMusic audio_music[ROHR_AUDIO_MUSIC_LIMIT];
static uint16_t audio_music_generations[ROHR_AUDIO_MUSIC_LIMIT];
static Music audio_active_music = MUSIC_INVALID;
static float audio_master_volume = 1.0f;
static bool audio_started = false;

/* The application gate protects service lifetime, including slow resource loads.
 * It is process-lived and never acquired by the SDL callback. SDL's stream lock
 * protects the queue, applied state, and active evidence bank. */
static SDL_InitState audio_gate_init;
static SDL_Mutex *audio_gate;

static bool audio_gate_lock(void) {
    if(SDL_ShouldInit(&audio_gate_init)) {
        SDL_Mutex *created = SDL_CreateMutex();
        if(created == NULL) {
            SDL_SetInitialized(&audio_gate_init, false);
            return false;
        }
        audio_gate = created;
        SDL_SetInitialized(&audio_gate_init, true);
    }
    SDL_LockMutex(audio_gate);
    return true;
}

static void audio_gate_unlock(void) { SDL_UnlockMutex(audio_gate); }

static Report audio_failure_report(const char *detail) {
    Report report = report_create();
    report.header = (ReportHeader){ROHR_AUDIO_REPORT, ROHR_REPORT_FAILED, 0.0};
    ReportDiagnostic diagnostic = {.report_id = ROHR_AUDIO_REPORT,
        .severity = ROHR_REPORT_SEVERITY_FATAL, .code = ERROR_ENGINE_AUDIO_OPERATION_FAILED,
        .recovery = ROHR_REPORT_UPDATE_ABORTED, .occurrence_count = 1};
    (void)report_diagnostic_detail_set(&diagnostic, detail);
    (void)report_diagnostic_add(&report, diagnostic);
    return report;
}

typedef enum AudioCommandKind {
    AUDIO_MASTER_VOLUME,
    AUDIO_SOUND_PLAY, AUDIO_SOUND_STOP, AUDIO_SOUND_VOLUME, AUDIO_SOUND_PAN,
    AUDIO_SOUND_LOOP, AUDIO_SOUND_RATE,
    AUDIO_MUSIC_PLAY, AUDIO_MUSIC_STOP, AUDIO_MUSIC_PAUSE, AUDIO_MUSIC_RESUME,
    AUDIO_MUSIC_VOLUME, AUDIO_MUSIC_LOOP, AUDIO_MUSIC_RATE
} AudioCommandKind;

typedef struct AudioCommand {
    AudioCommandKind kind;
    uint32_t handle;
    float value;
} AudioCommand;

static AudioCommand audio_commands[ROHR_AUDIO_REQUEST_CAPACITY];
static size_t audio_command_head, audio_command_count, audio_submitted_count;

enum { AUDIO_COMMANDS, AUDIO_SOUNDS, AUDIO_MUSIC, AUDIO_OUTPUT, AUDIO_PIPELINE_COUNT };
static const ReportId audio_report_ids[AUDIO_PIPELINE_COUNT] = {
    ROHR_AUDIO_COMMANDS_REPORT, ROHR_AUDIO_SOUNDS_REPORT,
    ROHR_AUDIO_MUSIC_REPORT, ROHR_AUDIO_OUTPUT_REPORT
};

typedef struct AudioPipelineEvidence {
    uint64_t invocations, nanoseconds, processed, skipped, frames;
    bool failed;
} AudioPipelineEvidence;

typedef struct AudioEvidence {
    /* Only diagnostics use the trusted shared collector in the callback. Pipeline
     * counters are O(1); complete report construction happens on the caller. */
    Report issues;
    AudioPipelineEvidence pipeline[AUDIO_PIPELINE_COUNT];
} AudioEvidence;

static AudioEvidence audio_evidence[2];
static unsigned int audio_evidence_index;
static Report audio_shutdown_report;
static bool audio_shutdown_pending;

static AudioEvidence *audio_evidence_current(void) {
    return &audio_evidence[audio_evidence_index];
}

static void audio_count_add(uint64_t *value, uint64_t amount) {
    if(UINT64_MAX - *value < amount) {
        *value = UINT64_MAX;
        audio_evidence_current()->issues.counters_saturated = true;
    } else *value += amount;
}

static void audio_pipeline_finish(unsigned int pipeline, uint64_t start,
        uint64_t processed, uint64_t frames) {
    AudioPipelineEvidence *value = &audio_evidence_current()->pipeline[pipeline];
    audio_count_add(&value->invocations, 1);
    audio_count_add(&value->nanoseconds, SDL_GetTicksNS() - start);
    audio_count_add(&value->processed, processed);
    audio_count_add(&value->frames, frames);
}

static void audio_issue(unsigned int pipeline, EngineError code, bool fatal,
        ReportObjectRef source, const char *detail) {
    ReportDiagnostic diagnostic = {
        .report_id = audio_report_ids[pipeline],
        .severity = fatal ? ROHR_REPORT_SEVERITY_FATAL : ROHR_REPORT_SEVERITY_WARNING,
        .code = code,
        .recovery = fatal ? ROHR_REPORT_UPDATE_ABORTED : ROHR_REPORT_CONTRIBUTION_SKIPPED,
        .source = source, .occurrence_count = 1,
    };
    AudioEvidence *evidence = audio_evidence_current();
    evidence->issues.header.id = ROHR_AUDIO_REPORT;
    (void)report_diagnostic_detail_set(&diagnostic, detail);
    report_diagnostic_collect_internal(&evidence->issues, diagnostic);
    evidence->pipeline[pipeline].failed |= fatal;
    audio_count_add(&evidence->pipeline[pipeline].skipped, 1);
}

static void audio_statistic_add(Report *report, ReportId id,
        ReportMetricId metric, uint64_t value) {
    (void)report_statistic_add(report, (ReportStatistic){
        .report_id = id, .metric_id = metric,
        .aggregation = ROHR_REPORT_AGGREGATION_SUM, .value = value,
    });
}

static Report audio_report_collect(const AudioEvidence *evidence,
        uint64_t accepted, uint64_t cancelled, uint64_t started, bool ran) {
    Report report = evidence->issues;
    report.header.id = ROHR_AUDIO_REPORT;
    report.header.status = report.has_fatal ? ROHR_REPORT_FAILED :
        (ran ? ROHR_REPORT_COMPLETE : ROHR_REPORT_NOT_RUN);
    for(unsigned int i = 0; i < AUDIO_PIPELINE_COUNT; i += 1) {
        const AudioPipelineEvidence *p = &evidence->pipeline[i];
        if(p->invocations == 0) continue;
        (void)report_entry_add(&report, (ReportEntry){
            .header = {audio_report_ids[i],
                p->failed ? ROHR_REPORT_FAILED : ROHR_REPORT_COMPLETE,
                (double)p->nanoseconds / 1000000.0},
            .invocation_count = p->invocations,
        });
        audio_statistic_add(&report, audio_report_ids[i], ROHR_REPORT_PROCESSED_COUNT, p->processed);
        audio_statistic_add(&report, audio_report_ids[i], ROHR_REPORT_SKIPPED_COUNT, p->skipped);
        if(i != AUDIO_COMMANDS)
            audio_statistic_add(&report, audio_report_ids[i], ROHR_REPORT_AUDIO_FRAMES_COUNT, p->frames);
    }
    if(ran) {
        audio_statistic_add(&report, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_QUEUED_COMMAND_COUNT, accepted);
        audio_statistic_add(&report, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_CANCELLED_COMMAND_COUNT, cancelled);
        report.header.total_ms = (double)(SDL_GetTicksNS() - started) / 1000000.0;
    }
    return report;
}


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

static AudioSoundResource *audio_sound_resource_path_get(const char *path) {
    if(path == NULL) return NULL;
    for(size_t slot = 0; slot < ROHR_AUDIO_SOUND_LIMIT; slot += 1) {
        AudioSoundResource *resource = &audio_sound_resources[slot];
        if(resource->used && strcmp(resource->path, path) == 0) return resource;
    }
    return NULL;
}

static AudioSoundResource *audio_sound_resource_free_get(void) {
    for(size_t slot = 0; slot < ROHR_AUDIO_SOUND_LIMIT; slot += 1)
        if(!audio_sound_resources[slot].used)
            return &audio_sound_resources[slot];
    return NULL;
}

static void audio_sound_resource_destroy(AudioSoundResource *resource) {
    if(resource == NULL || !resource->used) return;
    SDL_free(resource->samples);
    SDL_free(resource->path);
    *resource = (AudioSoundResource){0};
}

static size_t audio_sound_free_slot_get(void) {
    size_t slot;
    for(slot = 0; slot < ROHR_AUDIO_SOUND_LIMIT; slot += 1)
        if(!audio_sounds[slot].used) break;
    return slot;
}

static Sound audio_sound_publish(
        size_t slot,
        AudioSoundResource *resource,
        SoundConfig config) {
    audio_sound_generations[slot] += 1;
    if(audio_sound_generations[slot] == 0) audio_sound_generations[slot] = 1;
    audio_sounds[slot] = (AudioSound){
        .resource = resource,
        .id = audio_sound_id_create(slot),
        .volume = audio_unit_value_get(config.volume),
        .pan = audio_pan_value_get(config.pan),
        .playback_rate = audio_playback_rate_value_get(config.playback_rate),
        .loop = config.loop,
        .used = true,
    };
    resource->references += 1;
    return audio_sounds[slot].id;
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
    if(sound->resource == NULL || sound->resource->frame_count == 0) {
        sound->playing = false;
        sound->cursor = 0.0;
        return;
    }
    if(sound->cursor < (double)sound->resource->frame_count) return;
    if(sound->loop) {
        sound->cursor = fmod(sound->cursor,
            (double)sound->resource->frame_count);
    } else {
        sound->playing = false;
        sound->cursor = (double)sound->resource->frame_count;
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
        if(next_frame >= sound->resource->frame_count)
            next_frame = sound->loop ? 0 : sample_frame;
        fraction = (float)(sound->cursor - (double)sample_frame);
        left = sound->resource->samples[sample_frame * AUDIO_MIX_CHANNELS] +
            (sound->resource->samples[next_frame * AUDIO_MIX_CHANNELS] -
                sound->resource->samples[
                    sample_frame * AUDIO_MIX_CHANNELS]) * fraction;
        right = sound->resource->samples[
            sample_frame * AUDIO_MIX_CHANNELS + 1] +
            (sound->resource->samples[next_frame * AUDIO_MIX_CHANNELS + 1] -
                sound->resource->samples[
                    sample_frame * AUDIO_MIX_CHANNELS + 1]) * fraction;
        destination[frame * AUDIO_MIX_CHANNELS] += left * gain * left_pan;
        destination[frame * AUDIO_MIX_CHANNELS + 1] += right * gain * right_pan;
        sound->cursor += sound->playback_rate;
    }
}

static void audio_music_decode_state_reset(AudioMusic *music) {
    music->decode_failed = false;
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
        if(stb_vorbis_get_error(music->decoder) != VORBIS__no_error) {
            music->decode_failed = true;
            break;
        }
        if(!music->loop) break;
        if(attempt > 0 || !stb_vorbis_seek_start(music->decoder)) {
            music->decode_failed = true;
            break;
        }
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
    if(!stb_vorbis_seek(music->decoder, position)) {
        music->decode_failed = true;
        return false;
    }
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

static EngineResult audio_command_enqueue(AudioCommandKind kind,
        uint32_t handle, float value) {
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    EngineError error = ERROR_NONE;
    if(kind >= AUDIO_SOUND_PLAY && kind <= AUDIO_SOUND_RATE && audio_sound_get(handle) == NULL)
        error = ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND;
    else if(kind >= AUDIO_MUSIC_PLAY && audio_music_get(handle) == NULL)
        error = ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND;
    else if(audio_command_count == ROHR_AUDIO_REQUEST_CAPACITY)
        error = ERROR_ENGINE_AUDIO_REQUEST_CAPACITY_EXCEEDED;
    if(error == ERROR_NONE) {
        size_t tail = (audio_command_head + audio_command_count) % ROHR_AUDIO_REQUEST_CAPACITY;
        audio_commands[tail] = (AudioCommand){kind, handle, value};
        audio_command_count += 1;
    }
    audio_unlock();
    return error == ERROR_NONE ? error_result_value(true) : error_result_error(error);
}

/* Called only by the callback, already holding SDL's stream lock. */
static bool audio_command_apply(AudioCommand command) {
    AudioSound *sound = NULL;
    AudioMusic *music = NULL;
    ReportObjectRef source = {0};
    EngineError error = ERROR_NONE;
    const char *detail = NULL;
    if(command.kind >= AUDIO_SOUND_PLAY && command.kind <= AUDIO_SOUND_RATE) {
        source = (ReportObjectRef){ROHR_REPORT_OBJECT_SOUND, command.handle};
        sound = audio_sound_get(command.handle);
        if(sound == NULL) error = ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND;
    } else if(command.kind >= AUDIO_MUSIC_PLAY) {
        source = (ReportObjectRef){ROHR_REPORT_OBJECT_MUSIC, command.handle};
        music = audio_music_get(command.handle);
        if(music == NULL) error = ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND;
    }
    if(error != ERROR_NONE) {
        audio_issue(AUDIO_COMMANDS, error, false, source, "queued resource no longer exists");
        return false;
    }
    switch(command.kind) {
        case AUDIO_MASTER_VOLUME: audio_master_volume = command.value; break;
        case AUDIO_SOUND_PLAY: sound->cursor = 0.0; sound->playing = true; break;
        case AUDIO_SOUND_STOP: sound->cursor = 0.0; sound->playing = false; break;
        case AUDIO_SOUND_VOLUME: sound->volume = command.value; break;
        case AUDIO_SOUND_PAN: sound->pan = command.value; break;
        case AUDIO_SOUND_LOOP: sound->loop = command.value != 0.0f; break;
        case AUDIO_SOUND_RATE: sound->playback_rate = command.value; break;
        case AUDIO_MUSIC_PLAY: {
            if(!stb_vorbis_seek_start(music->decoder)) {
                detail = "could not seek music to its beginning";
                break;
            }
            audio_music_decode_state_reset(music);
            if(!audio_music_frames_prepare(music) || music->decode_failed) {
                audio_music_playback_clear(music);
                detail = "could not prepare music for playback";
                break;
            }
            AudioMusic *previous = audio_music_get(audio_active_music);
            if(previous != NULL && previous != music) audio_music_playback_clear(previous);
            music->playing = true;
            music->paused = false;
            audio_active_music = music->id;
            break;
        }
        case AUDIO_MUSIC_STOP:
            if(!stb_vorbis_seek_start(music->decoder)) {
                detail = "could not reset music to its beginning";
                break;
            }
            audio_music_playback_clear(music);
            audio_music_decode_state_reset(music);
            break;
        case AUDIO_MUSIC_PAUSE:
        case AUDIO_MUSIC_RESUME:
            if(audio_active_music != music->id || !music->playing) {
                error = ERROR_ENGINE_AUDIO_MUSIC_NOT_ACTIVE;
                detail = "queued pause/resume requires an active track";
            } else music->paused = command.kind == AUDIO_MUSIC_PAUSE;
            break;
        case AUDIO_MUSIC_VOLUME: music->volume = command.value; break;
        case AUDIO_MUSIC_LOOP: music->loop = command.value != 0.0f; break;
        case AUDIO_MUSIC_RATE: music->playback_rate = command.value; break;
    }
    if(detail != NULL) {
        audio_issue(AUDIO_COMMANDS, error == ERROR_NONE ? ERROR_ENGINE_AUDIO_OPERATION_FAILED : error,
            false, source, detail);
        return false;
    }
    return true;
}

static void audio_commands_apply(void) {
    if(audio_submitted_count == 0) return;
    uint64_t start = SDL_GetTicksNS(), applied = 0;
    while(audio_submitted_count > 0) {
        AudioCommand command = audio_commands[audio_command_head];
        audio_command_head = (audio_command_head + 1) % ROHR_AUDIO_REQUEST_CAPACITY;
        audio_command_count -= 1;
        audio_submitted_count -= 1;
        if(audio_command_apply(command)) applied += 1;
    }
    audio_pipeline_finish(AUDIO_COMMANDS, start, applied, 0);
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
    audio_commands_apply();
    frames_remaining = ((size_t)additional_amount + frame_bytes - 1) /
        frame_bytes;
    while(frames_remaining > 0) {
        size_t frames = frames_remaining > AUDIO_MIX_BUFFER_FRAMES ?
            AUDIO_MIX_BUFFER_FRAMES : frames_remaining;
        size_t sample_count = frames * AUDIO_MIX_CHANNELS;
        memset(mixed, 0, sample_count * sizeof(*mixed));
        uint64_t start = SDL_GetTicksNS(), processed = 0;
        for(size_t slot = 0; slot < ROHR_AUDIO_SOUND_LIMIT; slot += 1) {
            AudioSound *sound = &audio_sounds[slot];
            if(!sound->used || !sound->playing || sound->playback_rate == 0.0f) continue;
            if(sound->resource == NULL || sound->resource->frame_count == 0) {
                sound->playing = false;
                audio_issue(AUDIO_SOUNDS, ERROR_ENGINE_AUDIO_OPERATION_FAILED, false,
                    (ReportObjectRef){ROHR_REPORT_OBJECT_SOUND, sound->id}, "sound has no samples");
                continue;
            }
            audio_sound_mix(sound, mixed, frames);
            processed += 1;
        }
        audio_pipeline_finish(AUDIO_SOUNDS, start, processed, frames);
        if(audio_active_music != MUSIC_INVALID) {
            AudioMusic *music = audio_music_get(audio_active_music);
            if(music != NULL && music->playing && !music->paused && music->playback_rate > 0.0f) {
                start = SDL_GetTicksNS();
                music->decode_failed = false;
                audio_music_mix(music, mixed, frames);
                if(music->decode_failed) {
                    audio_music_playback_clear(music);
                    audio_issue(AUDIO_MUSIC, ERROR_ENGINE_AUDIO_OPERATION_FAILED, false,
                        (ReportObjectRef){ROHR_REPORT_OBJECT_MUSIC, music->id}, "music decode or seek failed");
                }
                audio_pipeline_finish(AUDIO_MUSIC, start, music->decode_failed ? 0 : 1, frames);
            }
        }
        start = SDL_GetTicksNS();
        for(size_t sample = 0; sample < sample_count; sample += 1) {
            if(mixed[sample] > 1.0f) mixed[sample] = 1.0f;
            else if(mixed[sample] < -1.0f) mixed[sample] = -1.0f;
        }
        if(!SDL_PutAudioStreamData(stream, mixed, (int)(sample_count * sizeof(*mixed)))) {
            audio_issue(AUDIO_OUTPUT, ERROR_ENGINE_AUDIO_OPERATION_FAILED, true,
                (ReportObjectRef){0}, SDL_GetError());
            audio_pipeline_finish(AUDIO_OUTPUT, start, 0, 0);
            return;
        }
        audio_pipeline_finish(AUDIO_OUTPUT, start, 1, frames);
        frames_remaining -= frames;
    }
}

static EngineResult audio_start_impl(void) {
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
    memset(audio_sound_resources, 0, sizeof(audio_sound_resources));
    memset(audio_music, 0, sizeof(audio_music));
    audio_active_music = MUSIC_INVALID;
    audio_master_volume = 1.0f;
    audio_command_head = audio_command_count = audio_submitted_count = 0;
    memset(audio_evidence, 0, sizeof(audio_evidence));
    audio_evidence_index = 0;
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
    audio_shutdown_pending = false;
    audio_shutdown_report = report_create();
    return error_result_value(true);
}

static Report audio_stop_impl(void) {
    uint64_t start = SDL_GetTicksNS();
    if(!audio_started) {
        if(audio_shutdown_pending) return audio_shutdown_report;
        Report empty = report_create();
        empty.header.id = ROHR_AUDIO_REPORT;
        return empty;
    }
    if(audio_stream != NULL) {
        SDL_DestroyAudioStream(audio_stream);
        audio_stream = NULL;
    }
    for(size_t slot = 0; slot < ROHR_AUDIO_SOUND_LIMIT; slot += 1)
        audio_sounds[slot] = (AudioSound){0};
    for(size_t slot = 0; slot < ROHR_AUDIO_SOUND_LIMIT; slot += 1)
        audio_sound_resource_destroy(&audio_sound_resources[slot]);
    for(size_t slot = 0; slot < ROHR_AUDIO_MUSIC_LIMIT; slot += 1) {
        if(audio_music[slot].decoder != NULL)
            stb_vorbis_close(audio_music[slot].decoder);
        if(audio_music[slot].decode_buffer != NULL)
            SDL_free(audio_music[slot].decode_buffer);
        SDL_free(audio_music[slot].path);
        audio_music[slot] = (AudioMusic){0};
    }
    audio_shutdown_report = audio_report_collect(audio_evidence_current(), 0,
        audio_command_count, start, true);
    audio_shutdown_pending = true;
    audio_command_head = audio_command_count = audio_submitted_count = 0;
    audio_active_music = MUSIC_INVALID;
    audio_master_volume = 1.0f;
    audio_started = false;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    audio_shutdown_report.header.total_ms = (double)(SDL_GetTicksNS() - start) / 1000000.0;
    return audio_shutdown_report;
}

static Report audio_update_impl(void) {
    uint64_t start = SDL_GetTicksNS();
    if(!audio_started) {
        if(audio_shutdown_pending) {
            audio_shutdown_pending = false;
            return audio_shutdown_report;
        }
        Report empty = report_create();
        empty.header.id = ROHR_AUDIO_REPORT;
        return empty;
    }
    unsigned int previous = audio_evidence_index;
    unsigned int next = previous ^ 1u;
    memset(&audio_evidence[next], 0, sizeof(audio_evidence[next]));
    if(!audio_lock()) {
        return audio_failure_report(SDL_GetError());
    }
    audio_evidence_index = next;
    uint64_t accepted = audio_command_count - audio_submitted_count;
    audio_submitted_count = audio_command_count;
    audio_unlock();
    return audio_report_collect(&audio_evidence[previous], accepted, 0, start, true);
}

static bool audio_started_check_impl(void) { return audio_started; }

static AudioOwnershipStats audio_ownership_stats_get_impl(void) {
    AudioOwnershipStats stats = {0};
    if(!audio_started || !audio_lock()) return stats;
    for(size_t slot = 0; slot < ROHR_AUDIO_SOUND_LIMIT; slot += 1) {
        if(audio_sounds[slot].used) stats.sound_players += 1;
        if(!audio_sound_resources[slot].used) continue;
        stats.sound_resources += 1;
        stats.sound_references += audio_sound_resources[slot].references;
    }
    for(size_t slot = 0; slot < ROHR_AUDIO_MUSIC_LIMIT; slot += 1)
        if(audio_music[slot].used) stats.music_instances += 1;
    audio_unlock();
    return stats;
}

static EngineResult audio_volume_set_impl(float volume) {
    return audio_command_enqueue(AUDIO_MASTER_VOLUME, 0, audio_unit_value_get(volume));
}

static float audio_volume_get_impl(void) {
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

static SoundResult audio_sound_create_impl(SoundConfig config) {
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
    AudioSoundResource *resource;
    char *path;
    size_t slot;
    Sound sound;

    if(!audio_started) return ERROR_RESULT_MAKE_ERROR(
        SoundResult, ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(config.path == NULL || config.path[0] == '\0')
        return ERROR_RESULT_MAKE_ERROR(
            SoundResult, ERROR_ENGINE_AUDIO_SOUND_CONFIG_INVALID);
    path = graphics_asset_path_resolve(config.path);
    if(path == NULL) {
        error_detail_set(ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED,
            "sound path resolution failed");
        return ERROR_RESULT_MAKE_ERROR(
            SoundResult, ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED);
    }
    if(!audio_lock()) {
        SDL_free(path);
        return (error_detail_set(ERROR_ENGINE_AUDIO_OPERATION_FAILED,
            SDL_GetError()), ERROR_RESULT_MAKE_ERROR(
                SoundResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED));
    }
    slot = audio_sound_free_slot_get();
    resource = audio_sound_resource_path_get(path);
    if(slot == ROHR_AUDIO_SOUND_LIMIT) {
        audio_unlock();
        SDL_free(path);
        return ERROR_RESULT_MAKE_ERROR(
            SoundResult, ERROR_ENGINE_AUDIO_SOUND_CAPACITY_EXCEEDED);
    }
    if(resource != NULL) {
        if(resource->references == SIZE_MAX) {
            audio_unlock();
            SDL_free(path);
            return ERROR_RESULT_MAKE_ERROR(
                SoundResult, ERROR_ENGINE_AUDIO_SOUND_CAPACITY_EXCEEDED);
        }
        sound = audio_sound_publish(slot, resource, config);
        audio_unlock();
        SDL_free(path);
        return ERROR_RESULT_MAKE_VALUE(SoundResult, sound);
    }
    audio_unlock();

    if(!SDL_LoadWAV(path, &source_spec, &source_samples, &source_length)) {
        const char *detail = SDL_GetError();
        SDL_free(path);
        return (error_detail_set(
            ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED, detail),
            ERROR_RESULT_MAKE_ERROR(
                SoundResult, ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED));
    }
    if(source_length > (Uint32)INT_MAX || !SDL_ConvertAudioSamples(
            &source_spec,
            source_samples,
            (int)source_length,
            &mix_spec,
            &converted_samples,
            &converted_length)) {
        const char *detail = SDL_GetError();
        SDL_free(source_samples);
        SDL_free(path);
        error_detail_set(ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED, detail);
        return ERROR_RESULT_MAKE_ERROR(
            SoundResult, ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED);
    }
    SDL_free(source_samples);
    if(converted_length <= 0 ||
            converted_length % (int)(sizeof(float) * AUDIO_MIX_CHANNELS) != 0) {
        SDL_free(converted_samples);
        SDL_free(path);
        return ERROR_RESULT_MAKE_ERROR(
            SoundResult, ERROR_ENGINE_AUDIO_SOUND_CONFIG_INVALID);
    }

    if(!audio_lock()) {
        SDL_free(converted_samples);
        SDL_free(path);
        return (error_detail_set(ERROR_ENGINE_AUDIO_OPERATION_FAILED,
            SDL_GetError()), ERROR_RESULT_MAKE_ERROR(
                SoundResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED));
    }
    slot = audio_sound_free_slot_get();
    resource = audio_sound_resource_path_get(path);
    if(slot == ROHR_AUDIO_SOUND_LIMIT ||
            (resource != NULL && resource->references == SIZE_MAX)) {
        audio_unlock();
        SDL_free(converted_samples);
        SDL_free(path);
        return ERROR_RESULT_MAKE_ERROR(
            SoundResult, ERROR_ENGINE_AUDIO_SOUND_CAPACITY_EXCEEDED);
    }
    if(resource == NULL) {
        resource = audio_sound_resource_free_get();
        if(resource == NULL) {
            audio_unlock();
            SDL_free(converted_samples);
            SDL_free(path);
            return ERROR_RESULT_MAKE_ERROR(
                SoundResult, ERROR_ENGINE_AUDIO_SOUND_CAPACITY_EXCEEDED);
        }
        *resource = (AudioSoundResource){
            .samples = (float *)converted_samples,
            .path = path,
            .frame_count = (size_t)converted_length /
                (sizeof(float) * AUDIO_MIX_CHANNELS),
            .used = true,
        };
        converted_samples = NULL;
        path = NULL;
    }
    sound = audio_sound_publish(slot, resource, config);
    audio_unlock();
    SDL_free(converted_samples);
    SDL_free(path);
    return ERROR_RESULT_MAKE_VALUE(SoundResult, sound);
}

static EngineResult audio_sound_destroy_impl(Sound *sound) {
    AudioSound *value;
    AudioSoundResource *resource;
    if(sound == NULL)
        return error_result_error(ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND);
    if(*sound == SOUND_INVALID) {
        *sound = SOUND_INVALID;
        return error_result_value(true);
    }
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_sound_get(*sound);
    if(value == NULL) {
        audio_unlock();
        return error_result_error(ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND);
    }
    resource = value->resource;
    *value = (AudioSound){0};
    if(resource != NULL && resource->references > 0) {
        resource->references -= 1;
        if(resource->references == 0) audio_sound_resource_destroy(resource);
    }
    audio_unlock();
    *sound = SOUND_INVALID;
    return error_result_value(true);
}

static bool audio_sound_valid_check_impl(Sound sound) {
    bool valid;
    if(!audio_started || !audio_lock()) return false;
    valid = audio_sound_get(sound) != NULL;
    audio_unlock();
    return valid;
}

static EngineResult audio_sound_play_impl(Sound sound) {
    return audio_command_enqueue(AUDIO_SOUND_PLAY, sound, 0.0f);
}

static EngineResult audio_sound_stop_impl(Sound sound) {
    return audio_command_enqueue(AUDIO_SOUND_STOP, sound, 0.0f);
}

static EngineResult audio_sound_volume_set_impl(Sound sound, float volume) {
    return audio_command_enqueue(AUDIO_SOUND_VOLUME, sound, audio_unit_value_get(volume));
}

static AudioValueResult audio_sound_volume_get_impl(Sound sound) {
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

static EngineResult audio_sound_pan_set_impl(Sound sound, float pan) {
    return audio_command_enqueue(AUDIO_SOUND_PAN, sound, audio_pan_value_get(pan));
}

static AudioValueResult audio_sound_pan_get_impl(Sound sound) {
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

static EngineResult audio_sound_loop_set_impl(Sound sound, bool loop) {
    return audio_command_enqueue(AUDIO_SOUND_LOOP, sound, loop ? 1.0f : 0.0f);
}

static bool audio_sound_loop_check_impl(Sound sound) {
    AudioSound *value;
    bool result;
    if(!audio_started || !audio_lock()) return false;
    value = audio_sound_get(sound);
    result = value != NULL && value->loop;
    audio_unlock();
    return result;
}

static EngineResult audio_sound_playback_rate_set_impl(Sound sound, float playback_rate) {
    return audio_command_enqueue(AUDIO_SOUND_RATE, sound, audio_playback_rate_value_get(playback_rate));
}

static AudioValueResult audio_sound_playback_rate_get_impl(Sound sound) {
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

static bool audio_sound_playing_check_impl(Sound sound) {
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

static MusicResult audio_music_create_impl(MusicConfig config) {
    stb_vorbis *decoder;
    stb_vorbis_info info;
    float *decode_buffer;
    char *path;
    unsigned int source_frame_count;
    int decoder_error = 0;
    Music music;
    size_t slot;
    char detail[96];

    if(!audio_started) return ERROR_RESULT_MAKE_ERROR(
        MusicResult, ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(config.path == NULL || config.path[0] == '\0')
        return ERROR_RESULT_MAKE_ERROR(
            MusicResult, ERROR_ENGINE_AUDIO_MUSIC_CONFIG_INVALID);
    path = graphics_asset_path_resolve(config.path);
    if(path == NULL) {
        error_detail_set(ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED,
            "music path resolution failed");
        return ERROR_RESULT_MAKE_ERROR(
            MusicResult, ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED);
    }
    if(!audio_lock()) {
        SDL_free(path);
        return (error_detail_set(ERROR_ENGINE_AUDIO_OPERATION_FAILED,
            SDL_GetError()), ERROR_RESULT_MAKE_ERROR(
                MusicResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED));
    }
    for(slot = 0; slot < ROHR_AUDIO_MUSIC_LIMIT; slot += 1)
        if(!audio_music[slot].used) break;
    audio_unlock();
    if(slot == ROHR_AUDIO_MUSIC_LIMIT) {
        SDL_free(path);
        return ERROR_RESULT_MAKE_ERROR(
            MusicResult, ERROR_ENGINE_AUDIO_MUSIC_CAPACITY_EXCEEDED);
    }

    decoder = stb_vorbis_open_filename(path, &decoder_error, NULL);
    if(decoder == NULL) {
        snprintf(detail, sizeof(detail), "stb_vorbis error %d", decoder_error);
        error_detail_set(ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED, detail);
        SDL_free(path);
        return ERROR_RESULT_MAKE_ERROR(
            MusicResult, ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED);
    }
    info = stb_vorbis_get_info(decoder);
    source_frame_count = stb_vorbis_stream_length_in_samples(decoder);
    if(info.sample_rate == 0 || (info.channels != 1 && info.channels != 2) ||
            source_frame_count == 0) {
        stb_vorbis_close(decoder);
        SDL_free(path);
        error_detail_set(ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED,
            "music must contain mono or stereo Vorbis samples");
        return ERROR_RESULT_MAKE_ERROR(
            MusicResult, ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED);
    }
    decode_buffer = SDL_malloc(AUDIO_MUSIC_DECODE_FRAMES *
        (size_t)info.channels * sizeof(*decode_buffer));
    if(decode_buffer == NULL) {
        stb_vorbis_close(decoder);
        SDL_free(path);
        error_detail_set(ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED,
            "music decode buffer allocation failed");
        return ERROR_RESULT_MAKE_ERROR(
            MusicResult, ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED);
    }
    if(!audio_lock()) {
        SDL_free(decode_buffer);
        stb_vorbis_close(decoder);
        SDL_free(path);
        error_detail_set(ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
        return ERROR_RESULT_MAKE_ERROR(
            MusicResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED);
    }
    for(slot = 0; slot < ROHR_AUDIO_MUSIC_LIMIT; slot += 1)
        if(!audio_music[slot].used) break;
    if(slot == ROHR_AUDIO_MUSIC_LIMIT) {
        audio_unlock();
        SDL_free(decode_buffer);
        stb_vorbis_close(decoder);
        SDL_free(path);
        return ERROR_RESULT_MAKE_ERROR(
            MusicResult, ERROR_ENGINE_AUDIO_MUSIC_CAPACITY_EXCEEDED);
    }
    audio_music_generations[slot] += 1;
    if(audio_music_generations[slot] == 0) audio_music_generations[slot] = 1;
    audio_music[slot] = (AudioMusic){
        .id = audio_music_id_create(slot),
        .decoder = decoder,
        .decode_buffer = decode_buffer,
        .path = path,
        .source_frequency = info.sample_rate,
        .source_frame_count = source_frame_count,
        .source_channels = info.channels,
        .volume = audio_unit_value_get(config.volume),
        .playback_rate = audio_playback_rate_value_get(config.playback_rate),
        .loop = config.loop,
        .used = true,
    };
    music = audio_music[slot].id;
    audio_unlock();
    return ERROR_RESULT_MAKE_VALUE(MusicResult, music);
}

static EngineResult audio_music_destroy_impl(Music *music) {
    AudioMusic *value;
    stb_vorbis *decoder;
    float *decode_buffer;
    char *path;
    if(music == NULL)
        return error_result_error(ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND);
    if(*music == MUSIC_INVALID) {
        *music = MUSIC_INVALID;
        return error_result_value(true);
    }
    if(!audio_started) return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    if(!audio_lock()) return error_result_error_detail(
        ERROR_ENGINE_AUDIO_OPERATION_FAILED, SDL_GetError());
    value = audio_music_get(*music);
    if(value == NULL) {
        audio_unlock();
        return error_result_error(ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND);
    }
    if(audio_active_music == *music) audio_active_music = MUSIC_INVALID;
    decoder = value->decoder;
    decode_buffer = value->decode_buffer;
    path = value->path;
    *value = (AudioMusic){0};
    audio_unlock();
    stb_vorbis_close(decoder);
    SDL_free(decode_buffer);
    SDL_free(path);
    *music = MUSIC_INVALID;
    return error_result_value(true);
}

static bool audio_music_valid_check_impl(Music music) {
    bool valid;
    if(!audio_started || !audio_lock()) return false;
    valid = audio_music_get(music) != NULL;
    audio_unlock();
    return valid;
}

static EngineResult audio_music_play_impl(Music music) {
    return audio_command_enqueue(AUDIO_MUSIC_PLAY, music, 0.0f);
}

static EngineResult audio_music_pause_impl(Music music) {
    return audio_command_enqueue(AUDIO_MUSIC_PAUSE, music, 0.0f);
}

static EngineResult audio_music_resume_impl(Music music) {
    return audio_command_enqueue(AUDIO_MUSIC_RESUME, music, 0.0f);
}

static EngineResult audio_music_stop_impl(Music music) {
    return audio_command_enqueue(AUDIO_MUSIC_STOP, music, 0.0f);
}

static EngineResult audio_music_volume_set_impl(Music music, float volume) {
    return audio_command_enqueue(AUDIO_MUSIC_VOLUME, music, audio_unit_value_get(volume));
}

static AudioValueResult audio_music_volume_get_impl(Music music) {
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

static EngineResult audio_music_loop_set_impl(Music music, bool loop) {
    return audio_command_enqueue(AUDIO_MUSIC_LOOP, music, loop ? 1.0f : 0.0f);
}

static bool audio_music_loop_check_impl(Music music) {
    AudioMusic *value;
    bool result;
    if(!audio_started || !audio_lock()) return false;
    value = audio_music_get(music);
    result = value != NULL && value->loop;
    audio_unlock();
    return result;
}

static EngineResult audio_music_playback_rate_set_impl(Music music, float playback_rate) {
    return audio_command_enqueue(AUDIO_MUSIC_RATE, music, audio_playback_rate_value_get(playback_rate));
}

static AudioValueResult audio_music_playback_rate_get_impl(Music music) {
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

static bool audio_music_playing_check_impl(Music music) {
    AudioMusic *value;
    bool result;
    if(!audio_started || !audio_lock()) return false;
    value = audio_music_get(music);
    result = value != NULL && audio_active_music == music && value->playing &&
        !value->paused && value->playback_rate > 0.0f;
    audio_unlock();
    return result;
}

static bool audio_music_paused_check_impl(Music music) {
    AudioMusic *value;
    bool result;
    if(!audio_started || !audio_lock()) return false;
    value = audio_music_get(music);
    result = value != NULL && audio_active_music == music && value->playing &&
        value->paused;
    audio_unlock();
    return result;
}

/* Application entry points: lock order is gate, then SDL stream. */
EngineResult audio_start(void) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_start_impl();
    audio_gate_unlock();
    return result;
}

Report audio_stop(void) {
    if(!audio_gate_lock()) { return audio_failure_report(SDL_GetError()); }
    Report result = audio_stop_impl();
    audio_gate_unlock();
    return result;
}

Report audio_update(void) {
    if(!audio_gate_lock()) { return audio_failure_report(SDL_GetError()); }
    Report result = audio_update_impl();
    audio_gate_unlock();
    return result;
}

bool audio_started_check(void) {
    if(!audio_gate_lock()) { return false; }
    bool result = audio_started_check_impl();
    audio_gate_unlock();
    return result;
}

AudioOwnershipStats audio_ownership_stats_get(void) {
    if(!audio_gate_lock()) { return (AudioOwnershipStats){0}; }
    AudioOwnershipStats result = audio_ownership_stats_get_impl();
    audio_gate_unlock();
    return result;
}

EngineResult audio_volume_set(float volume) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_volume_set_impl(volume);
    audio_gate_unlock();
    return result;
}

float audio_volume_get(void) {
    if(!audio_gate_lock()) { return 0.0f; }
    float result = audio_volume_get_impl();
    audio_gate_unlock();
    return result;
}

SoundResult audio_sound_create(SoundConfig config) {
    if(!audio_gate_lock()) { return ERROR_RESULT_MAKE_ERROR(SoundResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    SoundResult result = audio_sound_create_impl(config);
    audio_gate_unlock();
    return result;
}

EngineResult audio_sound_destroy(Sound *sound) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_sound_destroy_impl(sound);
    audio_gate_unlock();
    return result;
}

bool audio_sound_valid_check(Sound sound) {
    if(!audio_gate_lock()) { return false; }
    bool result = audio_sound_valid_check_impl(sound);
    audio_gate_unlock();
    return result;
}

EngineResult audio_sound_play(Sound sound) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_sound_play_impl(sound);
    audio_gate_unlock();
    return result;
}

EngineResult audio_sound_stop(Sound sound) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_sound_stop_impl(sound);
    audio_gate_unlock();
    return result;
}

EngineResult audio_sound_volume_set(Sound sound, float volume) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_sound_volume_set_impl(sound, volume);
    audio_gate_unlock();
    return result;
}

AudioValueResult audio_sound_volume_get(Sound sound) {
    if(!audio_gate_lock()) { return ERROR_RESULT_MAKE_ERROR(AudioValueResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    AudioValueResult result = audio_sound_volume_get_impl(sound);
    audio_gate_unlock();
    return result;
}

EngineResult audio_sound_pan_set(Sound sound, float pan) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_sound_pan_set_impl(sound, pan);
    audio_gate_unlock();
    return result;
}

AudioValueResult audio_sound_pan_get(Sound sound) {
    if(!audio_gate_lock()) { return ERROR_RESULT_MAKE_ERROR(AudioValueResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    AudioValueResult result = audio_sound_pan_get_impl(sound);
    audio_gate_unlock();
    return result;
}

EngineResult audio_sound_loop_set(Sound sound, bool loop) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_sound_loop_set_impl(sound, loop);
    audio_gate_unlock();
    return result;
}

bool audio_sound_loop_check(Sound sound) {
    if(!audio_gate_lock()) { return false; }
    bool result = audio_sound_loop_check_impl(sound);
    audio_gate_unlock();
    return result;
}

EngineResult audio_sound_playback_rate_set(Sound sound, float playback_rate) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_sound_playback_rate_set_impl(sound, playback_rate);
    audio_gate_unlock();
    return result;
}

AudioValueResult audio_sound_playback_rate_get(Sound sound) {
    if(!audio_gate_lock()) { return ERROR_RESULT_MAKE_ERROR(AudioValueResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    AudioValueResult result = audio_sound_playback_rate_get_impl(sound);
    audio_gate_unlock();
    return result;
}

bool audio_sound_playing_check(Sound sound) {
    if(!audio_gate_lock()) { return false; }
    bool result = audio_sound_playing_check_impl(sound);
    audio_gate_unlock();
    return result;
}

MusicResult audio_music_create(MusicConfig config) {
    if(!audio_gate_lock()) { return ERROR_RESULT_MAKE_ERROR(MusicResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    MusicResult result = audio_music_create_impl(config);
    audio_gate_unlock();
    return result;
}

EngineResult audio_music_destroy(Music *music) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_music_destroy_impl(music);
    audio_gate_unlock();
    return result;
}

bool audio_music_valid_check(Music music) {
    if(!audio_gate_lock()) { return false; }
    bool result = audio_music_valid_check_impl(music);
    audio_gate_unlock();
    return result;
}

EngineResult audio_music_play(Music music) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_music_play_impl(music);
    audio_gate_unlock();
    return result;
}

EngineResult audio_music_pause(Music music) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_music_pause_impl(music);
    audio_gate_unlock();
    return result;
}

EngineResult audio_music_resume(Music music) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_music_resume_impl(music);
    audio_gate_unlock();
    return result;
}

EngineResult audio_music_stop(Music music) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_music_stop_impl(music);
    audio_gate_unlock();
    return result;
}

EngineResult audio_music_volume_set(Music music, float volume) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_music_volume_set_impl(music, volume);
    audio_gate_unlock();
    return result;
}

AudioValueResult audio_music_volume_get(Music music) {
    if(!audio_gate_lock()) { return ERROR_RESULT_MAKE_ERROR(AudioValueResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    AudioValueResult result = audio_music_volume_get_impl(music);
    audio_gate_unlock();
    return result;
}

EngineResult audio_music_loop_set(Music music, bool loop) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_music_loop_set_impl(music, loop);
    audio_gate_unlock();
    return result;
}

bool audio_music_loop_check(Music music) {
    if(!audio_gate_lock()) { return false; }
    bool result = audio_music_loop_check_impl(music);
    audio_gate_unlock();
    return result;
}

EngineResult audio_music_playback_rate_set(Music music, float playback_rate) {
    if(!audio_gate_lock()) { return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    EngineResult result = audio_music_playback_rate_set_impl(music, playback_rate);
    audio_gate_unlock();
    return result;
}

AudioValueResult audio_music_playback_rate_get(Music music) {
    if(!audio_gate_lock()) { return ERROR_RESULT_MAKE_ERROR(AudioValueResult, ERROR_ENGINE_AUDIO_OPERATION_FAILED); }
    AudioValueResult result = audio_music_playback_rate_get_impl(music);
    audio_gate_unlock();
    return result;
}

bool audio_music_playing_check(Music music) {
    if(!audio_gate_lock()) { return false; }
    bool result = audio_music_playing_check_impl(music);
    audio_gate_unlock();
    return result;
}

bool audio_music_paused_check(Music music) {
    if(!audio_gate_lock()) { return false; }
    bool result = audio_music_paused_check_impl(music);
    audio_gate_unlock();
    return result;
}
