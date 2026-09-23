/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <stdio.h>

static void error_print(EngineError error) {
    fprintf(stderr, "audio error %d: %s\n", (int)error,
        rohr_error_code_message_get(error));
}

int main(int argc, char **argv) {
    Sound sound = SOUND_INVALID;
    Music music = MUSIC_INVALID;
    SoundResult sound_result;
    MusicResult music_result;
    EngineResult result;

    if(argc != 3) {
        fprintf(stderr, "usage: %s <sound.wav> <music.ogg>\n", argv[0]);
        return 1;
    }
    result = rohr_audio_start();
    if(rohr_error_check(result)) {
        error_print(result.result.error);
        return 1;
    }
    sound_result = rohr_audio_sound_create((SoundConfig){
        .path = argv[1],
        .volume = 1.0f,
        .pan = 0.0f,
        .playback_rate = 1.0f,
    });
    if(rohr_error_check(sound_result)) {
        error_print(sound_result.result.error);
        goto fail;
    }
    sound = sound_result.result.value;
    music_result = rohr_audio_music_create((MusicConfig){
        .path = argv[2],
        .volume = 0.7f,
        .loop = true,
        .playback_rate = 1.0f,
    });
    if(rohr_error_check(music_result)) {
        error_print(music_result.result.error);
        goto fail;
    }
    music = music_result.result.value;
    result = rohr_audio_music_play(music);
    if(rohr_error_check(result)) {
        error_print(result.result.error);
        goto fail;
    }
    result = rohr_audio_sound_play(sound);
    if(rohr_error_check(result)) {
        error_print(result.result.error);
        goto fail;
    }
    puts("Playing one WAV sound over one looping Ogg Vorbis music stream.");
    rohr_tools_delay(3);
    (void)rohr_audio_sound_destroy(sound);
    (void)rohr_audio_music_destroy(music);
    rohr_audio_stop();
    return 0;

fail:
    if(sound != SOUND_INVALID) (void)rohr_audio_sound_destroy(sound);
    if(music != MUSIC_INVALID) (void)rohr_audio_music_destroy(music);
    rohr_audio_stop();
    return 1;
}
