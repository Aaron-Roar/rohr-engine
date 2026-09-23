/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define AUDIO_TEST_PATH "rohr_audio_test.wav"
#define AUDIO_TEST_SAMPLE_RATE 48000
#define AUDIO_TEST_SAMPLE_COUNT 4800

static void write_u16_le(FILE *file, uint16_t value) {
    fputc((int)(value & UINT16_C(0xff)), file);
    fputc((int)((value >> 8) & UINT16_C(0xff)), file);
}

static void write_u32_le(FILE *file, uint32_t value) {
    fputc((int)(value & UINT32_C(0xff)), file);
    fputc((int)((value >> 8) & UINT32_C(0xff)), file);
    fputc((int)((value >> 16) & UINT32_C(0xff)), file);
    fputc((int)((value >> 24) & UINT32_C(0xff)), file);
}

static bool audio_test_wav_write(void) {
    const uint32_t data_size = AUDIO_TEST_SAMPLE_COUNT * sizeof(int16_t);
    FILE *file = fopen(AUDIO_TEST_PATH, "wb");
    if(file == NULL) return false;
    fwrite("RIFF", 1, 4, file);
    write_u32_le(file, 36 + data_size);
    fwrite("WAVEfmt ", 1, 8, file);
    write_u32_le(file, 16);
    write_u16_le(file, 1);
    write_u16_le(file, 1);
    write_u32_le(file, AUDIO_TEST_SAMPLE_RATE);
    write_u32_le(file, AUDIO_TEST_SAMPLE_RATE * sizeof(int16_t));
    write_u16_le(file, sizeof(int16_t));
    write_u16_le(file, 16);
    fwrite("data", 1, 4, file);
    write_u32_le(file, data_size);
    for(size_t sample = 0; sample < AUDIO_TEST_SAMPLE_COUNT; sample += 1) {
        double phase = (double)sample * 440.0 * 6.283185307179586 /
            AUDIO_TEST_SAMPLE_RATE;
        int16_t value = (int16_t)(sin(phase) * 4096.0);
        write_u16_le(file, (uint16_t)value);
    }
    if(fclose(file) != 0) return false;
    return true;
}

static bool result_error_is(EngineResult result, EngineError error) {
    return result.kind == ERROR_RESULT_ERROR && result.result.error == error;
}

static int fail(const char *message) {
    fprintf(stderr, "audio test failed: %s\n", message);
    rohr_audio_stop();
    (void)remove(AUDIO_TEST_PATH);
    return 1;
}

int main(void) {
    SoundConfig config = rohr_audio_sound_config_default_get();
    SoundResult first;
    SoundResult second;
    AudioValueResult value;

    if(config.path != NULL || config.volume != 1.0f || config.pan != 0.0f ||
            config.loop || config.playback_rate != 1.0f)
        return fail("unexpected sound defaults");
    first = rohr_audio_sound_create(config);
    if(first.kind != ERROR_RESULT_ERROR ||
            first.result.error != ERROR_ENGINE_AUDIO_NOT_STARTED)
        return fail("sound creation before audio start was accepted");
    if(!audio_test_wav_write()) return fail("could not create WAV fixture");
    (void)SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    if(error_check(rohr_audio_start())) return fail("audio start failed");
    if(!rohr_audio_started_check()) return fail("started state was not retained");
    if(!result_error_is(rohr_audio_start(), ERROR_ENGINE_AUDIO_ALREADY_STARTED))
        return fail("duplicate audio start was accepted");

    if(error_check(rohr_audio_volume_set(-2.0f)) ||
            rohr_audio_volume_get() != 0.0f)
        return fail("negative master volume was not clamped");
    if(error_check(rohr_audio_volume_set(2.0f)) ||
            rohr_audio_volume_get() != 1.0f)
        return fail("high master volume was not clamped");

    config.path = "missing_audio_test.wav";
    first = rohr_audio_sound_create(config);
    if(first.kind != ERROR_RESULT_ERROR ||
            first.result.error != ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED)
        return fail("missing WAV did not report a load failure");

    config.path = AUDIO_TEST_PATH;
    config.volume = 2.0f;
    config.pan = 2.0f;
    config.loop = true;
    config.playback_rate = -3.0f;
    first = rohr_audio_sound_create(config);
    if(error_check(first)) return fail("valid WAV could not be loaded");
    value = rohr_audio_sound_volume_get(first.result.value);
    if(error_check(value) || value.result.value != 1.0f)
        return fail("sound volume was not clamped");
    value = rohr_audio_sound_pan_get(first.result.value);
    if(error_check(value) || value.result.value != 1.0f)
        return fail("sound pan was not clamped");
    value = rohr_audio_sound_playback_rate_get(first.result.value);
    if(error_check(value) || value.result.value != 0.0f)
        return fail("negative playback rate was not normalized to zero");
    if(!rohr_audio_sound_loop_check(first.result.value))
        return fail("sound loop state was not retained");
    if(error_check(rohr_audio_sound_play(first.result.value)))
        return fail("zero-rate sound could not be armed");
    if(rohr_audio_sound_playing_check(first.result.value))
        return fail("zero-rate sound reported active playback");
    if(error_check(rohr_audio_sound_playback_rate_set(first.result.value, 1.0f)) ||
            !rohr_audio_sound_playing_check(first.result.value))
        return fail("raising playback rate did not resume the sound");
    if(error_check(rohr_audio_sound_stop(first.result.value)) ||
            rohr_audio_sound_playing_check(first.result.value))
        return fail("sound stop did not clear playback");

    config.volume = 1.0f;
    config.pan = -1.0f;
    config.playback_rate = 1.0f;
    second = rohr_audio_sound_create(config);
    if(error_check(second) || second.result.value == first.result.value)
        return fail("independent sound instance was not created");
    if(error_check(rohr_audio_sound_play(first.result.value)) ||
            error_check(rohr_audio_sound_play(second.result.value)))
        return fail("overlapping sound instances could not be started");
    if(error_check(rohr_audio_sound_destroy(first.result.value)))
        return fail("sound destroy failed");
    if(!result_error_is(rohr_audio_sound_stop(first.result.value),
            ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND))
        return fail("stale sound handle remained valid");

    rohr_audio_stop();
    if(rohr_audio_started_check()) return fail("audio stop retained started state");
    if(!result_error_is(rohr_audio_volume_set(1.0f),
            ERROR_ENGINE_AUDIO_NOT_STARTED))
        return fail("audio mutation after stop was accepted");

    (void)SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    if(error_check(rohr_engine_start())) return fail("engine restart failed");
    if(error_check(rohr_audio_start())) {
        rohr_engine_stop();
        return fail("audio start under engine failed");
    }
    rohr_engine_stop();
    if(rohr_audio_started_check())
        return fail("engine stop did not clean up audio");

    if(remove(AUDIO_TEST_PATH) != 0) return fail("could not remove WAV fixture");
    return 0;
}
