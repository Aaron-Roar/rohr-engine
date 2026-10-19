/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#define AUDIO_TEST_PATH "sdk_audio_descriptions.wav"
#define AUDIO_TEST_MUSIC_PATH "sdk_audio_descriptions.ogg"
#include "../audio_fixture.h"
#include <assert.h>
#include <string.h>

int main(void) {
    (void)SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    assert(audio_test_wav_write() && audio_test_ogg_write());
    assert(!rohr_error_check(rohr_audio_start()));
    SoundDescription sound = rohr_audio_sound_description_default_get();
    MusicDescription music = rohr_audio_music_description_default_get();
    assert(sound.volume == 0 && music.volume == 0);
    sound.path = AUDIO_TEST_PATH; music.path = AUDIO_TEST_MUSIC_PATH;
    SoundResult s = rohr_audio_sound_create(sound);
    MusicResult m = rohr_audio_music_create(music);
    assert(!rohr_error_check(s) && !rohr_error_check(m));
    SoundDescriptionResult current_sound = rohr_audio_sound_description_get(s.result.value);
    MusicDescriptionResult current_music = rohr_audio_music_description_get(m.result.value);
    assert(!rohr_error_check(current_sound) && !rohr_error_check(current_music));
    assert(strstr(current_sound.result.value.path, AUDIO_TEST_PATH));
    assert(strstr(current_music.result.value.path, AUDIO_TEST_MUSIC_PATH));
    current_sound.result.value.volume = 0.25f;
    current_music.result.value.volume = 0.5f;
    assert(!rohr_error_check(rohr_audio_sound_description_set(s.result.value, current_sound.result.value)));
    assert(!rohr_error_check(rohr_audio_music_description_set(m.result.value, current_music.result.value)));
    assert(rohr_audio_sound_volume_get(s.result.value).result.value == 0);
    assert(rohr_audio_music_volume_get(m.result.value).result.value == 0);
    (void)rohr_audio_update();
    Uint64 deadline = SDL_GetTicks() + 2000;
    while(rohr_audio_music_volume_get(m.result.value).result.value != 0.5f && SDL_GetTicks() < deadline)
        SDL_Delay(1);
    assert(rohr_audio_sound_volume_get(s.result.value).result.value == 0.25f);
    assert(rohr_audio_music_volume_get(m.result.value).result.value == 0.5f);
    assert(!rohr_error_check(rohr_audio_sound_destroy(&s.result.value)));
    assert(!rohr_error_check(rohr_audio_music_destroy(&m.result.value)));
    (void)rohr_audio_stop();
    assert(remove(AUDIO_TEST_PATH) == 0 && remove(AUDIO_TEST_MUSIC_PATH) == 0);
    return 0;
}
