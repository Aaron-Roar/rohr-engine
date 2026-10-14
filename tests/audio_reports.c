/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#define AUDIO_TEST_PATH "rohr_audio_reports_test.wav"
#define AUDIO_TEST_MUSIC_PATH "rohr_audio_reports_test.ogg"
#include "audio_fixture.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define OK(call) assert(!error_check(call))
static uint64_t metric(const Report *report, ReportId owner, ReportMetricId id) {
    assert(report_valid_check(report));
    for(size_t i = 0; i < report->statistic_count; i += 1)
        if(report->statistics[i].report_id == owner && report->statistics[i].metric_id == id)
            return report->statistics[i].value;
    return 0;
}

#ifdef ROHR_TEST_AUDIO_WRAP
#define STB_VORBIS_HEADER_ONLY
#include "../lib/vorbis/stb_vorbis.c"
static SDL_AudioStreamCallback callback;
static void *callback_context;
static SDL_AudioStream *test_stream;
static bool real_playback, fail_output, fail_decode, fail_seek;
static unsigned int output_calls;
static uint64_t sample_frames;
static double sample_energy;
static bool timing_probe, fail_stream_lock;
static Uint64 probe_ns;
Uint64 __real_SDL_GetTicksNS(void);
Uint64 __wrap_SDL_GetTicksNS(void) {
    return timing_probe ? probe_ns : __real_SDL_GetTicksNS();
}
void __real_SDL_LockMutex(SDL_Mutex *);
void __wrap_SDL_LockMutex(SDL_Mutex *mutex) {
    __real_SDL_LockMutex(mutex);
    if(timing_probe) probe_ns += 7000000;
}
bool __real_SDL_LockAudioStream(SDL_AudioStream *);
bool __wrap_SDL_LockAudioStream(SDL_AudioStream *stream) {
    if(fail_stream_lock) return SDL_SetError("injected stream lock failure");
    return __real_SDL_LockAudioStream(stream);
}

SDL_AudioStream *__real_SDL_OpenAudioDeviceStream(SDL_AudioDeviceID, const SDL_AudioSpec *, SDL_AudioStreamCallback, void *);
SDL_AudioStream *__wrap_SDL_OpenAudioDeviceStream(SDL_AudioDeviceID id, const SDL_AudioSpec *spec,
        SDL_AudioStreamCallback fn, void *context) {
    callback = fn;
    callback_context = context;
    test_stream = __real_SDL_OpenAudioDeviceStream(id, spec, fn, context);
    return test_stream;
}
bool __real_SDL_ResumeAudioStreamDevice(SDL_AudioStream *);
bool __wrap_SDL_ResumeAudioStreamDevice(SDL_AudioStream *stream) {
    return real_playback ? __real_SDL_ResumeAudioStreamDevice(stream) : true;
}
bool __real_SDL_PutAudioStreamData(SDL_AudioStream *, const void *, int);
bool __wrap_SDL_PutAudioStreamData(SDL_AudioStream *stream, const void *data, int length) {
    if(real_playback) return __real_SDL_PutAudioStreamData(stream, data, length);
    output_calls += 1;
    if(fail_output) { fail_output = false; return SDL_SetError("injected audio output failure"); }
    const float *samples = data;
    for(int i = 0; i < length / (int)sizeof(float); i += 1) sample_energy += fabs(samples[i]);
    sample_frames += (uint64_t)length / (2 * sizeof(float));
    return true;
}
int __real_stb_vorbis_get_samples_float_interleaved(stb_vorbis *, int, float *, int);
int __wrap_stb_vorbis_get_samples_float_interleaved(stb_vorbis *v, int c, float *b, int n) {
    return fail_decode ? 0 : __real_stb_vorbis_get_samples_float_interleaved(v,c,b,n);
}
int __real_stb_vorbis_get_error(stb_vorbis *);
int __wrap_stb_vorbis_get_error(stb_vorbis *v) {
    if(fail_decode) { fail_decode = false; return VORBIS_invalid_stream; }
    return __real_stb_vorbis_get_error(v);
}
int __real_stb_vorbis_seek_start(stb_vorbis *);
int __wrap_stb_vorbis_seek_start(stb_vorbis *v) {
    if(fail_seek) { fail_seek = false; return 0; }
    return __real_stb_vorbis_seek_start(v);
}
static void pump(size_t frames) {
    assert(SDL_LockAudioStream(test_stream));
    callback(callback_context, test_stream, (int)(frames * 2 * sizeof(float)), 0);
    SDL_UnlockAudioStream(test_stream);
}

static void timing_boundaries(void) {
    OK(audio_start()); /* Test stream stays paused; no callback clock races. */
    timing_probe = true;
    Report update = rohr_audio_update();
    assert(update.header.status == ROHR_REPORT_COMPLETE && update.header.total_ms == 7.0);
    fail_stream_lock = true;
    Report failure = audio_update();
    fail_stream_lock = false;
    assert(failure.header.status == ROHR_REPORT_FAILED && failure.header.total_ms == 7.0);
    assert(strstr(failure.first_fatal.detail, "injected stream lock failure"));
    Report stopped = rohr_audio_stop();
    assert(stopped.header.total_ms == 7.0);
    Report repeated = audio_stop(), delivered = audio_update();
    assert(memcmp(&stopped, &repeated, sizeof(stopped)) == 0);
    assert(memcmp(&stopped, &delivered, sizeof(stopped)) == 0);
    assert(audio_update().header.status == ROHR_REPORT_NOT_RUN);
    timing_probe = false;
}

static void delayed_and_failures(void) {
    OK(rohr_audio_start());
    SoundConfig config = {.path = AUDIO_TEST_PATH, .volume = 1, .loop = true, .playback_rate = 1};
    SoundResult sound = audio_sound_create(config);
    OK(sound);
    OK(rohr_audio_sound_play(sound.result.value));
    pump(16); /* Accepted, but not submitted. */
    assert(!audio_sound_playing_check(sound.result.value));
    assert(sample_energy == 0);
    Report report = rohr_audio_update();
    assert(metric(&report, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_QUEUED_COMMAND_COUNT) == 1);
    for(int i = 0; i < 3; i += 1) {
        report = audio_update();
        assert(report.entry_count == 0 && report.diagnostic_count == 0);
    }
    pump(64);
    assert(audio_sound_playing_check(sound.result.value) && sample_energy > 0);
    report = audio_update();
    assert(metric(&report, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_PROCESSED_COUNT) == 1);
    assert(metric(&report, ROHR_AUDIO_OUTPUT_REPORT, ROHR_REPORT_AUDIO_FRAMES_COUNT) == 64);
    assert(report.entry_count == 3); /* commands, sounds, output; no music */
    Report copy = report;
    report = audio_update();
    assert(report.entry_count == 0 && report.diagnostic_count == 0);
    assert(metric(&copy, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_PROCESSED_COUNT) == 1);

    OK(audio_sound_volume_set(sound.result.value, 0.25f));
    assert(audio_sound_volume_get(sound.result.value).result.value == 1.0f);
    (void)audio_update();
    pump(1);
    assert(audio_sound_volume_get(sound.result.value).result.value == 0.25f);
    OK(audio_sound_play(sound.result.value));
    OK(audio_sound_stop(sound.result.value));
    (void)audio_update();
    pump(1);
    assert(!audio_sound_playing_check(sound.result.value));

    MusicConfig mc = {.path = AUDIO_TEST_MUSIC_PATH, .volume = 1, .loop = true, .playback_rate = 1};
    MusicResult music = audio_music_create(mc);
    OK(music);
    OK(audio_music_play(music.result.value));
    OK(audio_music_pause(music.result.value));
    (void)audio_update();
    assert(!audio_music_paused_check(music.result.value));
    pump(1);
    assert(audio_music_paused_check(music.result.value));
    report = audio_update();
    assert(report.diagnostic_count == 0);
    OK(audio_music_resume(music.result.value));
    OK(audio_sound_play(sound.result.value));
    (void)audio_update(); pump(1);
    assert(audio_music_playing_check(music.result.value));
    (void)audio_update();
    fail_decode = true;
    pump(20000); /* Exhaust buffered decoded samples; fail after earlier play. */
    report = audio_update();
    assert(report.diagnostic_count == 1 && !report.has_fatal);
    assert(report.diagnostics[0].report_id == ROHR_AUDIO_MUSIC_REPORT);
    assert(report.diagnostics[0].source.id == music.result.value);
    assert(!audio_music_playing_check(music.result.value));
    assert(audio_sound_playing_check(sound.result.value));

    OK(audio_music_play(music.result.value));
    (void)audio_update();
    for(int i = 0; i < 3; i += 1) {
        report = audio_update();
        assert(!report.diagnostic_count);
    }
    fail_seek = true;
    pump(16);
    report = rohr_audio_update();
    assert(report.diagnostic_count == 1 && report.diagnostics[0].report_id == ROHR_AUDIO_COMMANDS_REPORT);
    assert(strstr(report.diagnostics[0].detail, "seek"));
    Report retained = report;

    OK(audio_music_loop_set(music.result.value, false));
    OK(audio_music_play(music.result.value));
    (void)audio_update(); pump(48000);
    report = audio_update();
    assert(!report.diagnostic_count && !audio_music_playing_check(music.result.value));
    assert(audio_sound_playing_check(sound.result.value));

    Sound stale = sound.result.value;
    OK(audio_sound_play(stale));
    (void)audio_update();
    OK(audio_sound_destroy(&sound.result.value));
    sound = audio_sound_create(config); OK(sound);
    assert(sound.result.value != stale);
    pump(1);
    report = audio_update();
    assert(report.diagnostic_count == 1 && report.diagnostics[0].source.id == stale);
    assert(!audio_sound_playing_check(sound.result.value));

    for(size_t i = 0; i < ROHR_AUDIO_REQUEST_CAPACITY; i += 1) OK(audio_volume_set((float)i / 255.0f));
    EngineResult full = rohr_audio_volume_set(0);
    assert(error_check(full) && full.result.error == ERROR_ENGINE_AUDIO_REQUEST_CAPACITY_EXCEEDED);
    report = audio_update();
    assert(metric(&report, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_QUEUED_COMMAND_COUNT) == ROHR_AUDIO_REQUEST_CAPACITY);
    full = audio_volume_set(0); /* Submission does not free capacity. */
    assert(error_check(full));
    pump(1);
    report = audio_update();
    assert(metric(&report, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_PROCESSED_COUNT) == ROHR_AUDIO_REQUEST_CAPACITY);
    assert(audio_volume_get() == 1.0f);

    fail_output = true;
    unsigned int before = output_calls;
    pump(4096);
    report = audio_update();
    assert(report.header.status == ROHR_REPORT_FAILED && report.has_fatal);
    assert(output_calls == before + 1); /* Abort remainder of failed batch. */
    assert(strstr(report.first_fatal.detail, "injected audio output failure"));
    pump(16);
    Report recovered = audio_update();
    assert(recovered.header.status == ROHR_REPORT_COMPLETE && !recovered.has_fatal);
    assert(metric(&recovered, ROHR_AUDIO_OUTPUT_REPORT, ROHR_REPORT_AUDIO_FRAMES_COUNT) == 16);

    OK(audio_sound_play(sound.result.value));
    Report stopped = rohr_audio_stop();
    assert(metric(&stopped, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_CANCELLED_COMMAND_COUNT) == 1);
    assert(!audio_started_check());
    Report again = audio_stop();
    assert(memcmp(&stopped, &again, sizeof(stopped)) == 0);
    again = audio_update();
    assert(memcmp(&stopped, &again, sizeof(stopped)) == 0);
    assert(audio_update().header.status == ROHR_REPORT_NOT_RUN);
    assert(audio_stop().header.status == ROHR_REPORT_NOT_RUN);
    assert(strstr(retained.diagnostics[0].detail, "seek"));
    OK(audio_start());
    assert(audio_update().entry_count == 0);
    assert(!audio_sound_valid_check(stale));
    (void)audio_stop(); (void)audio_update();
}

static void overflow_test(void) {
    OK(audio_start());
    SoundConfig config = {.path = AUDIO_TEST_PATH, .volume = 1, .loop = true, .playback_rate = 1};
    MusicConfig mc = {.path = AUDIO_TEST_MUSIC_PATH, .volume = 1, .loop = true, .playback_rate = 100};
    MusicResult music = audio_music_create(mc); OK(music);
    OK(audio_music_play(music.result.value));
    (void)audio_update(); pump(1); (void)audio_update();
    for(size_t i = 0; i < ROHR_AUDIO_REQUEST_CAPACITY; i += 1) {
        SoundResult sound = audio_sound_create(config); OK(sound);
        OK(audio_sound_play(sound.result.value));
        OK(audio_sound_destroy(&sound.result.value));
    }
    (void)audio_update();
    fail_decode = true;
    fail_output = true;
    uint64_t start = SDL_GetTicksNS();
    pump(1024);
    printf("full diagnostic callback: %.3f us\n", (double)(SDL_GetTicksNS() - start) / 1000.0);
    /* All ordinary slots are full before output's first fatal arrives. */
    fail_output = true;
    pump(1);
    Report report = audio_update();
    assert(report.diagnostic_count == ROHR_REPORT_DIAGNOSTIC_CAPACITY);
    assert(report.has_fatal && report.first_fatal.occurrence_count == 2);
    assert(report.dropped_diagnostic_occurrences == 1);
    assert(report.header.status == ROHR_REPORT_FAILED);
    assert(metric(&report, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_SKIPPED_COUNT) == 256);
    assert(report_valid_check(&report));
    OK(rohr_report_json_write(&report, "audio_report.json", false));
    OK(rohr_report_text_write(&report, "audio_report.txt"));
    FILE *file = fopen("audio_report.json", "rb"); assert(file);
    assert(fseek(file, 0, SEEK_END) == 0);
    long size = ftell(file); assert(size > 0); rewind(file);
    char *bytes = malloc((size_t)size + 1); assert(bytes);
    assert(fread(bytes, 1, (size_t)size, file) == (size_t)size); bytes[size] = 0;
    assert(strstr(bytes, "ROHR_AUDIO_REPORT"));
    assert(strstr(bytes, "ROHR_AUDIO_COMMANDS_REPORT"));
    assert(strstr(bytes, "ROHR_AUDIO_SOUNDS_REPORT"));
    assert(strstr(bytes, "ROHR_AUDIO_MUSIC_REPORT"));
    assert(strstr(bytes, "ROHR_AUDIO_OUTPUT_REPORT"));
    assert(strstr(bytes, "ROHR_REPORT_AUDIO_FRAMES_COUNT"));
    assert(strstr(bytes, "ROHR_REPORT_CANCELLED_COMMAND_COUNT"));
    free(bytes); fclose(file);
    assert(remove("audio_report.json") == 0 && remove("audio_report.txt") == 0);
    (void)audio_stop(); (void)audio_update();
}

static void benchmark(void) {
    OK(audio_start());
    uint64_t start = SDL_GetTicksNS();
    for(int i = 0; i < 1000; i += 1) pump(1024);
    double callback_ms = (double)(SDL_GetTicksNS() - start) / 1000000.0;
    Report report = audio_update();
    assert(metric(&report, ROHR_AUDIO_OUTPUT_REPORT, ROHR_REPORT_AUDIO_FRAMES_COUNT) == 1024000);
    start = SDL_GetTicksNS();
    for(int i = 0; i < 1000; i += 1) (void)audio_update();
    printf("audio callback incl reporting: %.3f us/buffer; empty update: %.3f us; Report: %zu bytes\n",
        callback_ms, (double)(SDL_GetTicksNS() - start) / 1000000.0, sizeof(Report));
    (void)audio_stop(); (void)audio_update();
}
#endif

/* Real callback concurrency runs on every platform, without production hooks. */
typedef struct Producer { Sound sound; SDL_AtomicInt *go, *ready; unsigned accepted, full, stopped, stale; } Producer;
static int SDLCALL produce(void *context) {
    Producer *p = context;
    EngineResult own_error = error_result_error_detail(ERROR_ENGINE_AUDIO_OPERATION_FAILED, "producer detail");
    SDL_AddAtomicInt(p->ready, 1);
    while(!SDL_GetAtomicInt(p->go)) SDL_Delay(1);
    assert(strstr(rohr_error_message_get(own_error), "producer detail"));
    for(unsigned int i = 0; i < 1000; i += 1) {
        if(i == 500) {
            SDL_AddAtomicInt(p->ready, 1);
            while(SDL_GetAtomicInt(p->go) < 2) SDL_Delay(1);
        }
        if(i >= 500 && i % 8 == 0) SDL_Delay(1);
        EngineResult r = rohr_audio_sound_volume_set(p->sound, (float)(i % 10) / 10.0f);
        if(!error_check(r)) p->accepted += 1;
        else if(r.result.error == ERROR_ENGINE_AUDIO_REQUEST_CAPACITY_EXCEEDED) p->full += 1;
        else if(r.result.error == ERROR_ENGINE_AUDIO_NOT_STARTED) p->stopped += 1;
        else if(r.result.error == ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND) p->stale += 1;
        else assert(false);
    }
    return 0;
}
static void concurrent_test(void) {
#ifdef ROHR_TEST_AUDIO_WRAP
    real_playback = true;
#endif
    OK(audio_start());
    SoundConfig config = {.path = AUDIO_TEST_PATH, .volume = 1, .loop = true, .playback_rate = 1};
    SoundResult sound = audio_sound_create(config); OK(sound);
    SDL_AtomicInt go = {0}, ready = {0};
    Producer producers[4] = {0};
    SDL_Thread *threads[4];
    for(size_t i = 0; i < 4; i += 1) {
        producers[i].sound = sound.result.value; producers[i].go = &go; producers[i].ready = &ready;
        threads[i] = SDL_CreateThread(produce, "audio producer", &producers[i]); assert(threads[i]);
    }
    while(SDL_GetAtomicInt(&ready) < 4) SDL_Delay(1);
    EngineResult own_error = error_result_error_detail(ERROR_ENGINE_AUDIO_OPERATION_FAILED, "main detail");
    SDL_SetAtomicInt(&go, 1);
    assert(strstr(rohr_error_message_get(own_error), "main detail"));
    uint64_t processed = 0, cancelled = 0, skipped = 0;
    for(int i = 0; i < 30; i += 1) {
        Report r = audio_update();
        processed += metric(&r, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_PROCESSED_COUNT);
        skipped += metric(&r, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_SKIPPED_COUNT);
        SDL_Delay(1);
    }
    while(SDL_GetAtomicInt(&ready) < 8) SDL_Delay(1);
    SDL_SetAtomicInt(&go, 2); /* Producers are live across destroy and stop. */
    OK(audio_sound_destroy(&sound.result.value));
    Report stopped = audio_stop();
    processed += metric(&stopped, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_PROCESSED_COUNT);
    skipped += metric(&stopped, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_SKIPPED_COUNT);
    cancelled += metric(&stopped, ROHR_AUDIO_COMMANDS_REPORT, ROHR_REPORT_CANCELLED_COMMAND_COUNT);
    unsigned int accepted = 0;
    for(size_t i = 0; i < 4; i += 1) {
        SDL_WaitThread(threads[i], NULL);
        assert(producers[i].accepted + producers[i].full + producers[i].stopped + producers[i].stale == 1000);
        accepted += producers[i].accepted;
    }
    assert(accepted > 0 && accepted == processed + skipped + cancelled);
    assert(audio_update().header.status == stopped.header.status);
    assert(audio_update().header.status == ROHR_REPORT_NOT_RUN);
    OK(audio_start());
    SoundResult fresh = audio_sound_create(config); OK(fresh);
    assert(fresh.result.value != producers[0].sound);
    OK(audio_sound_play(fresh.result.value));
    (void)audio_update();
    uint64_t deadline = SDL_GetTicks() + 2000;
    while(!audio_sound_playing_check(fresh.result.value) && SDL_GetTicks() < deadline) SDL_Delay(1);
    assert(audio_sound_playing_check(fresh.result.value));
    SDL_Delay(30); /* Playback must advance without another update. */
    Report continued = audio_update();
    assert(metric(&continued, ROHR_AUDIO_OUTPUT_REPORT, ROHR_REPORT_AUDIO_FRAMES_COUNT) > 0);
    (void)audio_stop(); (void)audio_update();
}
int main(void) {
    (void)SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    assert(audio_update().header.status == ROHR_REPORT_NOT_RUN);
    assert(audio_test_wav_write() && audio_test_ogg_write());
#ifdef ROHR_TEST_AUDIO_WRAP
    timing_boundaries();
    delayed_and_failures();
    overflow_test();
    benchmark();
#endif
    concurrent_test();
    assert(remove(AUDIO_TEST_PATH) == 0);
    assert(remove(AUDIO_TEST_MUSIC_PATH) == 0);
    return 0;
}
