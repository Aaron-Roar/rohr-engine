/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <stddef.h>
#include <stdio.h>

#define AUDIO_MUSIC_COUNT 3
#define AUDIO_PREVIEW_SECONDS 2

static const char *audio_music_paths[AUDIO_MUSIC_COUNT] = {
    "assets/audio/ogg/freesound_community-orchestral_2of5-75312.ogg",
    "assets/audio/ogg/freesound_community-water-drop-85731.ogg",
    "assets/audio/ogg/freesound_community-we-have-time-ogg-67849.ogg",
};

static void error_print(EngineError error) {
    fprintf(stderr, "audio error %d: %s\n", (int)error,
        rohr_error_code_message_get(error));
}

int main(void) {
    Sound sound = SOUND_INVALID;
    Music music[AUDIO_MUSIC_COUNT] = {MUSIC_INVALID};
    SoundConfig sound_config = rohr_audio_sound_config_default_get();
    MusicConfig music_config = rohr_audio_music_config_default_get();
    SoundResult sound_result;
    MusicResult music_result;
    EngineResult result;
    int exit_code = 1;

    result = rohr_directory_working_set(rohr_directory_base_get());
    if(rohr_error_check(result)) {
        error_print(result.result.error);
        return 1;
    }
    result = rohr_audio_start();
    if(rohr_error_check(result)) {
        error_print(result.result.error);
        return 1;
    }
    sound_config.path = "assets/audio/wav/water-splash-3.wav";
    sound_result = rohr_audio_sound_create(sound_config);
    if(rohr_error_check(sound_result)) {
        error_print(sound_result.result.error);
        goto cleanup;
    }
    sound = sound_result.result.value;
    music_config.volume = 0.7f;
    music_config.loop = true;
    for(size_t index = 0; index < AUDIO_MUSIC_COUNT; index += 1) {
        music_config.path = audio_music_paths[index];
        music_result = rohr_audio_music_create(music_config);
        if(rohr_error_check(music_result)) {
            error_print(music_result.result.error);
            goto cleanup;
        }
        music[index] = music_result.result.value;
    }

    for(size_t index = 0; index < AUDIO_MUSIC_COUNT; index += 1) {
        printf("Playing %s\n", audio_music_paths[index]);
        result = rohr_audio_music_play(music[index]);
        if(rohr_error_check(result)) {
            error_print(result.result.error);
            goto cleanup;
        }
        result = rohr_audio_sound_play(sound);
        if(rohr_error_check(result)) {
            error_print(result.result.error);
            goto cleanup;
        }
        rohr_tools_delay(AUDIO_PREVIEW_SECONDS);
    }
    exit_code = 0;

cleanup:
    if(sound != SOUND_INVALID) (void)rohr_audio_sound_destroy(sound);
    for(size_t index = 0; index < AUDIO_MUSIC_COUNT; index += 1)
        if(music[index] != MUSIC_INVALID)
            (void)rohr_audio_music_destroy(music[index]);
    rohr_audio_stop();
    return exit_code;
}
