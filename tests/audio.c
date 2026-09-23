/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "audio_ogg_fixture.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define AUDIO_TEST_PATH "rohr_audio_test.wav"
#define AUDIO_TEST_MUSIC_PATH "rohr_audio_test.ogg"
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

static int audio_test_base64_value(char value) {
    if(value >= 'A' && value <= 'Z') return value - 'A';
    if(value >= 'a' && value <= 'z') return value - 'a' + 26;
    if(value >= '0' && value <= '9') return value - '0' + 52;
    if(value == '+') return 62;
    if(value == '/') return 63;
    return -1;
}

static bool audio_test_ogg_write(void) {
    FILE *file = fopen(AUDIO_TEST_MUSIC_PATH, "wb");
    uint32_t pending = 0;
    int pending_bits = 0;
    if(file == NULL) return false;
    for(size_t index = 0; audio_test_ogg_base64[index] != '\0'; index += 1) {
        int value;
        if(audio_test_ogg_base64[index] == '=') break;
        value = audio_test_base64_value(audio_test_ogg_base64[index]);
        if(value < 0) {
            fclose(file);
            (void)remove(AUDIO_TEST_MUSIC_PATH);
            return false;
        }
        pending = (pending << 6) | (uint32_t)value;
        pending_bits += 6;
        if(pending_bits >= 8) {
            pending_bits -= 8;
            if(fputc((int)((pending >> pending_bits) & UINT32_C(0xff)), file) ==
                    EOF) {
                fclose(file);
                (void)remove(AUDIO_TEST_MUSIC_PATH);
                return false;
            }
            pending &= pending_bits == 0 ? 0 :
                (UINT32_C(1) << pending_bits) - 1;
        }
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
    (void)remove(AUDIO_TEST_MUSIC_PATH);
    return 1;
}

int main(void) {
    SoundConfig config = rohr_audio_sound_config_default_get();
    SoundResult first;
    SoundResult second;
    MusicConfig music_config = rohr_audio_music_config_default_get();
    MusicResult first_music;
    MusicResult second_music;
    AudioValueResult value;

    if(config.path != NULL || config.volume != 1.0f || config.pan != 0.0f ||
            config.loop || config.playback_rate != 1.0f)
        return fail("unexpected sound defaults");
    first = rohr_audio_sound_create(config);
    if(first.kind != ERROR_RESULT_ERROR ||
            first.result.error != ERROR_ENGINE_AUDIO_NOT_STARTED)
        return fail("sound creation before audio start was accepted");
    if(music_config.path != NULL || music_config.volume != 1.0f ||
            music_config.loop || music_config.playback_rate != 1.0f)
        return fail("unexpected music defaults");
    first_music = rohr_audio_music_create(music_config);
    if(first_music.kind != ERROR_RESULT_ERROR ||
            first_music.result.error != ERROR_ENGINE_AUDIO_NOT_STARTED)
        return fail("music creation before audio start was accepted");
    if(!audio_test_wav_write()) return fail("could not create WAV fixture");
    if(!audio_test_ogg_write()) return fail("could not create Ogg fixture");
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

    first_music = rohr_audio_music_create(music_config);
    if(first_music.kind != ERROR_RESULT_ERROR ||
            first_music.result.error != ERROR_ENGINE_AUDIO_MUSIC_CONFIG_INVALID)
        return fail("empty music path was accepted");
    music_config.path = "missing_audio_test.ogg";
    first_music = rohr_audio_music_create(music_config);
    if(first_music.kind != ERROR_RESULT_ERROR ||
            first_music.result.error != ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED)
        return fail("missing Ogg did not report a load failure");

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

    music_config.path = AUDIO_TEST_MUSIC_PATH;
    music_config.volume = 2.0f;
    music_config.loop = true;
    music_config.playback_rate = -4.0f;
    first_music = rohr_audio_music_create(music_config);
    if(error_check(first_music)) return fail("valid Ogg could not be opened");
    value = rohr_audio_music_volume_get(first_music.result.value);
    if(error_check(value) || value.result.value != 1.0f)
        return fail("music volume was not clamped");
    value = rohr_audio_music_playback_rate_get(first_music.result.value);
    if(error_check(value) || value.result.value != 0.0f)
        return fail("negative music rate was not normalized to zero");
    if(!rohr_audio_music_loop_check(first_music.result.value))
        return fail("music loop state was not retained");
    if(error_check(rohr_audio_music_volume_set(first_music.result.value, -2.0f)))
        return fail("music volume setter failed");
    value = rohr_audio_music_volume_get(first_music.result.value);
    if(error_check(value) || value.result.value != 0.0f)
        return fail("negative music volume was not clamped");
    if(error_check(rohr_audio_music_volume_set(first_music.result.value, 0.75f)) ||
            error_check(rohr_audio_music_loop_set(first_music.result.value, false)) ||
            rohr_audio_music_loop_check(first_music.result.value) ||
            error_check(rohr_audio_music_loop_set(first_music.result.value, true)))
        return fail("music runtime configuration was not applied");
    if(error_check(rohr_audio_music_play(first_music.result.value)))
        return fail("zero-rate music could not be armed");
    if(rohr_audio_music_playing_check(first_music.result.value) ||
            rohr_audio_music_paused_check(first_music.result.value))
        return fail("zero-rate music reported active playback or pause");
    if(error_check(rohr_audio_music_playback_rate_set(
            first_music.result.value, 1.0f)) ||
            !rohr_audio_music_playing_check(first_music.result.value))
        return fail("raising music rate did not resume advancement");
    SDL_Delay(20);
    if(!rohr_audio_music_playing_check(first_music.result.value))
        return fail("looping streamed music stopped during decoding");

    music_config.volume = 0.5f;
    music_config.playback_rate = 1.0f;
    second_music = rohr_audio_music_create(music_config);
    if(error_check(second_music) ||
            second_music.result.value == first_music.result.value)
        return fail("second music resource could not be opened");
    if(error_check(rohr_audio_music_play(second_music.result.value)) ||
            rohr_audio_music_playing_check(first_music.result.value) ||
            !rohr_audio_music_playing_check(second_music.result.value))
        return fail("music replacement did not enforce one active track");
    if(!result_error_is(rohr_audio_music_resume(first_music.result.value),
            ERROR_ENGINE_AUDIO_MUSIC_NOT_ACTIVE))
        return fail("inactive music resume was accepted");
    if(error_check(rohr_audio_music_pause(second_music.result.value)) ||
            !rohr_audio_music_paused_check(second_music.result.value) ||
            rohr_audio_music_playing_check(second_music.result.value))
        return fail("music pause state was not retained");
    if(error_check(rohr_audio_music_resume(second_music.result.value)) ||
            rohr_audio_music_paused_check(second_music.result.value) ||
            !rohr_audio_music_playing_check(second_music.result.value))
        return fail("music resume did not continue the active track");
    if(error_check(rohr_audio_music_stop(second_music.result.value)) ||
            rohr_audio_music_playing_check(second_music.result.value) ||
            rohr_audio_music_paused_check(second_music.result.value))
        return fail("music stop did not reset playback state");
    if(error_check(rohr_audio_music_loop_set(second_music.result.value, false)) ||
            error_check(rohr_audio_music_play(second_music.result.value)))
        return fail("finite music playback could not start");
    SDL_Delay(600);
    if(rohr_audio_music_playing_check(second_music.result.value))
        return fail("finite music remained active after stream end");
    if(error_check(rohr_audio_music_destroy(first_music.result.value)))
        return fail("music destroy failed");
    if(!result_error_is(rohr_audio_music_play(first_music.result.value),
            ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND))
        return fail("stale music handle remained valid");

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
    if(remove(AUDIO_TEST_MUSIC_PATH) != 0)
        return fail("could not remove Ogg fixture");
    return 0;
}
