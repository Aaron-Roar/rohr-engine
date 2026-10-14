/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "audio/audio_internal.h"

#include <SDL3/SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "audio_fixture.h"

static bool result_error_is(EngineResult result, EngineError error) {
    return result.kind == ERROR_RESULT_ERROR && result.result.error == error;
}

static bool stats_check(
        size_t sound_players,
        size_t sound_resources,
        size_t sound_references,
        size_t music_instances,
        const char *context) {
    AudioOwnershipStats stats = audio_ownership_stats_get();
    if(stats.sound_players == sound_players &&
            stats.sound_resources == sound_resources &&
            stats.sound_references == sound_references &&
            stats.music_instances == music_instances) return true;
    fprintf(stderr,
        "%s: audio stats were players=%zu resources=%zu references=%zu "
        "music=%zu; expected %zu/%zu/%zu/%zu\n",
        context, stats.sound_players, stats.sound_resources,
        stats.sound_references, stats.music_instances, sound_players,
        sound_resources, sound_references, music_instances);
    return false;
}

static int fail(const char *message) {
    fprintf(stderr, "audio test failed: %s\n", message);
    rohr_audio_stop();
    (void)remove(AUDIO_TEST_PATH);
    (void)remove(AUDIO_TEST_MUSIC_PATH);
    return 1;
}

/* Preserve this test's applied-state assertions while migrating controls to
 * asynchronous execution. Completion is observed through reports, not a delay. */
static EngineResult control_complete(EngineResult accepted) {
    if(error_check(accepted)) return accepted;
    Report report = rohr_audio_update();
    uint64_t deadline = SDL_GetTicks() + 2000;
    for(;;) {
        for(size_t i = 0; i < report.diagnostic_count; i += 1)
            if(report.diagnostics[i].report_id == ROHR_AUDIO_COMMANDS_REPORT)
                return error_result_error(report.diagnostics[i].code);
        for(size_t i = 0; i < report.statistic_count; i += 1)
            if(report.statistics[i].report_id == ROHR_AUDIO_COMMANDS_REPORT &&
                    report.statistics[i].metric_id == ROHR_REPORT_PROCESSED_COUNT &&
                    report.statistics[i].value > 0) return accepted;
        if(SDL_GetTicks() >= deadline)
            return error_result_error(ERROR_ENGINE_AUDIO_OPERATION_FAILED);
        SDL_Delay(1);
        report = audio_update();
    }
}

int main(void) {
    SoundConfig config = rohr_audio_sound_config_default_get();
    SoundResult first;
    SoundResult second;
    Sound stale_sound;
    Sound shutdown_sound;
    Sound zero_sound = SOUND_INVALID;
    MusicConfig music_config = rohr_audio_music_config_default_get();
    MusicResult first_music;
    MusicResult second_music;
    Music stale_music;
    Music shutdown_music;
    Music zero_music = MUSIC_INVALID;
    AudioValueResult value;

    if(config.path != NULL || config.volume != 1.0f || config.pan != 0.0f ||
            config.loop || config.playback_rate != 1.0f)
        return fail("unexpected sound defaults");
    first = rohr_audio_sound_create(config);
    if(first.kind != ERROR_RESULT_ERROR ||
            first.result.error != ERROR_ENGINE_AUDIO_NOT_STARTED)
        return fail("sound creation before audio start was accepted");
    if(rohr_audio_sound_valid_check(SOUND_INVALID) ||
            audio_sound_valid_check(SOUND_INVALID) ||
            error_check(rohr_audio_sound_destroy(&zero_sound)) ||
            zero_sound != SOUND_INVALID)
        return fail("direct or wrapper sound zero-owner contract failed");
    if(music_config.path != NULL || music_config.volume != 1.0f ||
            music_config.loop || music_config.playback_rate != 1.0f)
        return fail("unexpected music defaults");
    first_music = rohr_audio_music_create(music_config);
    if(first_music.kind != ERROR_RESULT_ERROR ||
            first_music.result.error != ERROR_ENGINE_AUDIO_NOT_STARTED)
        return fail("music creation before audio start was accepted");
    if(rohr_audio_music_valid_check(MUSIC_INVALID) ||
            audio_music_valid_check(MUSIC_INVALID) ||
            error_check(rohr_audio_music_destroy(&zero_music)) ||
            zero_music != MUSIC_INVALID)
        return fail("direct or wrapper music zero-owner contract failed");
    if(!audio_test_wav_write()) return fail("could not create WAV fixture");
    if(!audio_test_ogg_write()) return fail("could not create Ogg fixture");
    (void)SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    if(error_check(rohr_audio_start())) return fail("audio start failed");
    if(!rohr_audio_started_check()) return fail("started state was not retained");
    if(!stats_check(0, 0, 0, 0, "initial state"))
        return fail("audio ownership did not start empty");
    if(!result_error_is(rohr_audio_start(), ERROR_ENGINE_AUDIO_ALREADY_STARTED))
        return fail("duplicate audio start was accepted");
    if(!result_error_is(rohr_audio_sound_destroy(NULL),
            ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND) ||
            !result_error_is(rohr_audio_music_destroy(NULL),
                ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND) ||
            !stats_check(0, 0, 0, 0, "null destruction"))
        return fail("null audio destruction changed ownership");

    if(error_check(control_complete(rohr_audio_volume_set(-2.0f))) ||
            rohr_audio_volume_get() != 0.0f)
        return fail("negative master volume was not clamped");
    if(error_check(control_complete(rohr_audio_volume_set(2.0f))) ||
            rohr_audio_volume_get() != 1.0f)
        return fail("high master volume was not clamped");

    config.path = "missing_audio_test.wav";
    first = rohr_audio_sound_create(config);
    if(first.kind != ERROR_RESULT_ERROR ||
            first.result.error != ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED ||
            !stats_check(0, 0, 0, 0, "failed sound creation"))
        return fail("missing WAV did not report a load failure");

    first_music = rohr_audio_music_create(music_config);
    if(first_music.kind != ERROR_RESULT_ERROR ||
            first_music.result.error != ERROR_ENGINE_AUDIO_MUSIC_CONFIG_INVALID)
        return fail("empty music path was accepted");
    music_config.path = "missing_audio_test.ogg";
    first_music = rohr_audio_music_create(music_config);
    if(first_music.kind != ERROR_RESULT_ERROR ||
            first_music.result.error != ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED ||
            !stats_check(0, 0, 0, 0, "failed music creation"))
        return fail("missing Ogg did not report a load failure");

    config.path = AUDIO_TEST_PATH;
    config.volume = 2.0f;
    config.pan = 2.0f;
    config.loop = true;
    config.playback_rate = -3.0f;
    first = rohr_audio_sound_create(config);
    if(error_check(first) || !rohr_audio_sound_valid_check(first.result.value) ||
            !stats_check(1, 1, 1, 0, "first sound creation"))
        return fail("valid WAV could not be loaded");
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
    if(error_check(control_complete(rohr_audio_sound_play(first.result.value))))
        return fail("zero-rate sound could not be armed");
    if(rohr_audio_sound_playing_check(first.result.value))
        return fail("zero-rate sound reported active playback");
    if(error_check(control_complete(rohr_audio_sound_playback_rate_set(first.result.value, 1.0f))) ||
            !rohr_audio_sound_playing_check(first.result.value))
        return fail("raising playback rate did not resume the sound");
    if(error_check(control_complete(rohr_audio_sound_stop(first.result.value))) ||
            rohr_audio_sound_playing_check(first.result.value))
        return fail("sound stop did not clear playback");

    config.path = "./" AUDIO_TEST_PATH;
    config.volume = 1.0f;
    config.pan = -1.0f;
    config.playback_rate = 1.0f;
    second = audio_sound_create(config);
    if(error_check(second) || second.result.value == first.result.value ||
            !audio_sound_valid_check(second.result.value) ||
            !stats_check(2, 1, 2, 0, "shared decoded sound"))
        return fail("shared WAV did not create an independent sound player");
    if(error_check(control_complete(audio_sound_play(first.result.value))) ||
            error_check(control_complete(rohr_audio_sound_play(second.result.value))))
        return fail("overlapping sound instances could not be started");
    if(error_check(control_complete(audio_sound_stop(first.result.value))) ||
            rohr_audio_sound_playing_check(first.result.value) ||
            !audio_sound_playing_check(second.result.value))
        return fail("shared sounds did not retain independent player state");
    stale_sound = first.result.value;
    if(error_check(rohr_audio_sound_destroy(&first.result.value)) ||
            first.result.value != SOUND_INVALID ||
            rohr_audio_sound_valid_check(stale_sound) ||
            !rohr_audio_sound_valid_check(second.result.value) ||
            !stats_check(1, 1, 1, 0, "first sound destruction"))
        return fail("sound destroy failed");
    {
        Sound rejected = stale_sound;
        if(!result_error_is(rohr_audio_sound_destroy(&rejected),
                ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND) ||
                rejected != stale_sound ||
                !result_error_is(control_complete(rohr_audio_sound_stop(stale_sound)),
                    ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND) ||
                !stats_check(1, 1, 1, 0, "stale sound destruction"))
            return fail("stale sound ownership was accepted or cleared");
    }
    if(error_check(audio_sound_destroy(&second.result.value)) ||
            second.result.value != SOUND_INVALID ||
            !stats_check(0, 0, 0, 0, "final sound destruction"))
        return fail("direct sound destruction did not release cached samples");
    config.path = AUDIO_TEST_PATH;
    second = rohr_audio_sound_create(config);
    if(error_check(second) || !stats_check(1, 1, 1, 0,
            "sound recreation after cache release"))
        return fail("sound could not be recreated after cache release");

    music_config.path = AUDIO_TEST_MUSIC_PATH;
    music_config.volume = 2.0f;
    music_config.loop = true;
    music_config.playback_rate = -4.0f;
    first_music = rohr_audio_music_create(music_config);
    if(error_check(first_music) ||
            !rohr_audio_music_valid_check(first_music.result.value) ||
            !stats_check(1, 1, 1, 1, "first music creation"))
        return fail("valid Ogg could not be opened");
    value = rohr_audio_music_volume_get(first_music.result.value);
    if(error_check(value) || value.result.value != 1.0f)
        return fail("music volume was not clamped");
    value = rohr_audio_music_playback_rate_get(first_music.result.value);
    if(error_check(value) || value.result.value != 0.0f)
        return fail("negative music rate was not normalized to zero");
    if(!rohr_audio_music_loop_check(first_music.result.value))
        return fail("music loop state was not retained");
    if(error_check(control_complete(rohr_audio_music_volume_set(first_music.result.value, -2.0f))))
        return fail("music volume setter failed");
    value = rohr_audio_music_volume_get(first_music.result.value);
    if(error_check(value) || value.result.value != 0.0f)
        return fail("negative music volume was not clamped");
    if(error_check(control_complete(rohr_audio_music_volume_set(first_music.result.value, 0.75f))) ||
            error_check(control_complete(rohr_audio_music_loop_set(first_music.result.value, false))) ||
            rohr_audio_music_loop_check(first_music.result.value) ||
            error_check(control_complete(rohr_audio_music_loop_set(first_music.result.value, true))))
        return fail("music runtime configuration was not applied");
    if(error_check(control_complete(rohr_audio_music_play(first_music.result.value))))
        return fail("zero-rate music could not be armed");
    if(rohr_audio_music_playing_check(first_music.result.value) ||
            rohr_audio_music_paused_check(first_music.result.value))
        return fail("zero-rate music reported active playback or pause");
    if(error_check(control_complete(rohr_audio_music_playback_rate_set(
            first_music.result.value, 1.0f))) ||
            !rohr_audio_music_playing_check(first_music.result.value))
        return fail("raising music rate did not resume advancement");
    SDL_Delay(20);
    if(!rohr_audio_music_playing_check(first_music.result.value))
        return fail("looping streamed music stopped during decoding");

    music_config.volume = 0.5f;
    music_config.playback_rate = 1.0f;
    second_music = audio_music_create(music_config);
    if(error_check(second_music) ||
            second_music.result.value == first_music.result.value ||
            !audio_music_valid_check(second_music.result.value) ||
            !stats_check(1, 1, 1, 2, "second music creation"))
        return fail("second music resource could not be opened");
    if(error_check(control_complete(rohr_audio_music_play(second_music.result.value))) ||
            rohr_audio_music_playing_check(first_music.result.value) ||
            !rohr_audio_music_playing_check(second_music.result.value))
        return fail("music replacement did not enforce one active track");
    if(!result_error_is(control_complete(rohr_audio_music_resume(first_music.result.value)),
            ERROR_ENGINE_AUDIO_MUSIC_NOT_ACTIVE))
        return fail("inactive music resume was accepted");
    if(error_check(control_complete(rohr_audio_music_pause(second_music.result.value))) ||
            !rohr_audio_music_paused_check(second_music.result.value) ||
            rohr_audio_music_playing_check(second_music.result.value))
        return fail("music pause state was not retained");
    if(error_check(control_complete(rohr_audio_music_resume(second_music.result.value))) ||
            rohr_audio_music_paused_check(second_music.result.value) ||
            !rohr_audio_music_playing_check(second_music.result.value))
        return fail("music resume did not continue the active track");
    if(error_check(control_complete(rohr_audio_music_stop(second_music.result.value))) ||
            rohr_audio_music_playing_check(second_music.result.value) ||
            rohr_audio_music_paused_check(second_music.result.value))
        return fail("music stop did not reset playback state");
    if(error_check(control_complete(rohr_audio_music_loop_set(second_music.result.value, false))) ||
            error_check(control_complete(rohr_audio_music_play(second_music.result.value))))
        return fail("finite music playback could not start");
    SDL_Delay(600);
    if(rohr_audio_music_playing_check(second_music.result.value))
        return fail("finite music remained active after stream end");
    stale_music = first_music.result.value;
    if(error_check(audio_music_destroy(&first_music.result.value)) ||
            first_music.result.value != MUSIC_INVALID ||
            rohr_audio_music_valid_check(stale_music) ||
            !rohr_audio_music_valid_check(second_music.result.value) ||
            !stats_check(1, 1, 1, 1, "first music destruction"))
        return fail("music destroy failed");
    {
        Music rejected = stale_music;
        if(!result_error_is(rohr_audio_music_destroy(&rejected),
                ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND) ||
                rejected != stale_music ||
                !result_error_is(control_complete(rohr_audio_music_play(stale_music)),
                    ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND) ||
                !stats_check(1, 1, 1, 1, "stale music destruction"))
            return fail("stale music ownership was accepted or cleared");
    }

    shutdown_sound = second.result.value;
    shutdown_music = second_music.result.value;
    rohr_audio_stop();
    if(rohr_audio_started_check()) return fail("audio stop retained started state");
    if(rohr_audio_sound_valid_check(shutdown_sound) ||
            rohr_audio_music_valid_check(shutdown_music) ||
            !stats_check(0, 0, 0, 0, "stopped state"))
        return fail("audio stop retained live ownership");
    if(!result_error_is(control_complete(rohr_audio_volume_set(1.0f)),
            ERROR_ENGINE_AUDIO_NOT_STARTED))
        return fail("audio mutation after stop was accepted");

    (void)SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    if(error_check(rohr_engine_start())) return fail("engine restart failed");
    if(error_check(rohr_audio_start())) {
        rohr_engine_stop();
        return fail("audio start under engine failed");
    }
    config.path = AUDIO_TEST_PATH;
    first = rohr_audio_sound_create(config);
    music_config.path = AUDIO_TEST_MUSIC_PATH;
    first_music = rohr_audio_music_create(music_config);
    if(error_check(first) || error_check(first_music) ||
            first.result.value == shutdown_sound ||
            first_music.result.value == shutdown_music ||
            rohr_audio_sound_valid_check(shutdown_sound) ||
            rohr_audio_music_valid_check(shutdown_music) ||
            !stats_check(1, 1, 1, 1, "restart ownership")) {
        rohr_engine_stop();
        return fail("audio restart revived stale ownership");
    }
    rohr_engine_stop();
    if(rohr_audio_started_check())
        return fail("engine stop did not clean up audio");
    if(rohr_audio_sound_valid_check(first.result.value) ||
            rohr_audio_music_valid_check(first_music.result.value) ||
            !stats_check(0, 0, 0, 0, "engine shutdown"))
        return fail("engine shutdown retained audio ownership");

    if(remove(AUDIO_TEST_PATH) != 0) return fail("could not remove WAV fixture");
    if(remove(AUDIO_TEST_MUSIC_PATH) != 0)
        return fail("could not remove Ogg fixture");
    return 0;
}
