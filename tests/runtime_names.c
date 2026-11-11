/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#define AUDIO_TEST_PATH "runtime_names.wav"
#define AUDIO_TEST_MUSIC_PATH "runtime_names.ogg"
#include "audio_fixture.h"
#include <assert.h>
#include <string.h>

#define OK(call) assert(!error_check(call))
#define BAD(call) assert(error_check(call))

static void namespaces(void) {
    const char *bases[] = {"box", "box_1", "box", "box_1"};
    const char *expected[] = {"box", "box_1", "box_2", "box_1_1"};
    Entity entities[4]; ViewportId viewports[4]; InputControllerId controllers[4];
    Sound sounds[4]; Music music[4];
    for(size_t i = 0; i < 4; ++i) {
        EntityDescription e = {0}; ViewportDescription v = graphics_viewport_description_default_get();
        InputControllerDescription c = {.enabled = true};
        SoundDescription s = {.path = AUDIO_TEST_PATH}; MusicDescription m = {.path = AUDIO_TEST_MUSIC_PATH};
        strcpy(e.name, bases[i]); strcpy(v.name, bases[i]); strcpy(c.name, bases[i]);
        strcpy(s.name, bases[i]); strcpy(m.name, bases[i]);
        EntityResult er = rohr_entity_description_add(e); OK(er); entities[i] = er.result.value;
        ViewportIdResult vr = rohr_viewport_create(v); OK(vr); viewports[i] = vr.result.value;
        InputControllerIdResult cr = rohr_controller_create(c); OK(cr); controllers[i] = cr.result.value;
        SoundResult sr = rohr_audio_sound_create(s); OK(sr); sounds[i] = sr.result.value;
        MusicResult mr = rohr_audio_music_create(m); OK(mr); music[i] = mr.result.value;
        assert(!strcmp(rohr_entity_name_get(entities[i]).result.value.value, expected[i]));
        assert(!strcmp(rohr_viewport_name_get(viewports[i]).result.value.value, expected[i]));
        assert(!strcmp(rohr_input_controller_name_get(controllers[i]).result.value.value, expected[i]));
        assert(!strcmp(rohr_audio_sound_name_get(sounds[i]).result.value.value, expected[i]));
        assert(!strcmp(rohr_audio_music_name_get(music[i]).result.value.value, expected[i]));
        assert(entity_by_name_get(expected[i]).result.value == entities[i]);
        assert(graphics_viewport_by_name_get(expected[i]).result.value == viewports[i]);
        assert(input_controller_by_name_get(expected[i]).result.value == controllers[i]);
        assert(audio_sound_by_name_get(expected[i]).result.value == sounds[i]);
        assert(audio_music_by_name_get(expected[i]).result.value == music[i]);
    }
    ViewportName saved_viewport = graphics_viewport_name_get(viewports[1]).result.value;
    InputControllerName saved_controller = input_controller_name_get(controllers[1]).result.value;
    SoundName saved_sound = audio_sound_name_get(sounds[1]).result.value;
    MusicName saved_music = audio_music_name_get(music[1]).result.value;
    Entity old_entity = entities[1]; ViewportId old_viewport = viewports[1];
    InputControllerId old_controller = controllers[1]; Sound old_sound = sounds[1]; Music old_music = music[1];
    OK(entity_delete(entities[1])); OK(graphics_viewport_destroy(viewports[1]));
    OK(input_controller_destroy(controllers[1])); OK(audio_sound_destroy(&sounds[1])); OK(audio_music_destroy(&music[1]));
    EntityResult er = entity_description_add((EntityDescription){.name = "box"}); OK(er); entities[1] = er.result.value;
    ViewportIdResult vr = graphics_viewport_create((ViewportDescription){.name = "box"}); OK(vr); viewports[1] = vr.result.value;
    InputControllerIdResult cr = input_controller_create((InputControllerDescription){.name = "box"}); OK(cr); controllers[1] = cr.result.value;
    SoundResult sr = audio_sound_create((SoundDescription){.name = "box", .path = AUDIO_TEST_PATH}); OK(sr); sounds[1] = sr.result.value;
    MusicResult mr = audio_music_create((MusicDescription){.name = "box", .path = AUDIO_TEST_MUSIC_PATH}); OK(mr); music[1] = mr.result.value;
    assert(!entity_alive_check(old_entity)); BAD(graphics_viewport_name_get(old_viewport));
    BAD(input_controller_name_get(old_controller)); BAD(audio_sound_name_get(old_sound)); BAD(audio_music_name_get(old_music));
    assert(rohr_entity_by_name_get("box_1").result.value == entities[1]);
    assert(rohr_viewport_by_name_get("box_1").result.value == viewports[1]);
    assert(rohr_input_controller_by_name_get("box_1").result.value == controllers[1]);
    assert(rohr_audio_sound_by_name_get("box_1").result.value == sounds[1]);
    assert(rohr_audio_music_by_name_get("box_1").result.value == music[1]);
    assert(!strcmp(saved_viewport.value, "box_1") && !strcmp(saved_controller.value, "box_1"));
    assert(!strcmp(saved_sound.value, "box_1") && !strcmp(saved_music.value, "box_1"));

    SoundDescription sd = audio_sound_description_get(sounds[1]).result.value;
    MusicDescription md = audio_music_description_get(music[1]).result.value;
    assert(!strcmp(sd.name, "box_1") && !strcmp(md.name, "box_1"));
    OK(audio_sound_description_set(sounds[1], sd)); OK(audio_music_description_set(music[1], md));
    strcpy(sd.name, "rename"); strcpy(md.name, "rename");
    BAD(audio_sound_description_set(sounds[1], sd)); BAD(audio_music_description_set(music[1], md));
    sd.name[0] = md.name[0] = 0;
    OK(audio_sound_description_set(sounds[1], sd)); OK(audio_music_description_set(music[1], md));
    (void)audio_update(); SDL_Delay(40);
    assert(audio_sound_by_name_get("box_1").result.value == sounds[1]);
    assert(audio_music_by_name_get("box_1").result.value == music[1]);
    for(size_t i = 0; i < 4; ++i) {
        OK(entity_delete(entities[i])); OK(graphics_viewport_destroy(viewports[i]));
        OK(input_controller_destroy(controllers[i])); OK(audio_sound_destroy(&sounds[i])); OK(audio_music_destroy(&music[i]));
    }
}

static void limits(void) {
    EntityDescription e = {0}; ViewportDescription v = {0}; InputControllerDescription c = {0};
    SoundDescription s = {.path = AUDIO_TEST_PATH}; MusicDescription m = {.path = AUDIO_TEST_MUSIC_PATH};
    memset(e.name, 'x', sizeof(e.name) - 1); strcpy(v.name, e.name); strcpy(c.name, e.name);
    strcpy(s.name, e.name); strcpy(m.name, e.name);
    EntityResult er = entity_description_add(e); OK(er);
    ViewportIdResult vr = graphics_viewport_create(v); OK(vr);
    InputControllerIdResult cr = input_controller_create(c); OK(cr);
    SoundResult sr = audio_sound_create(s); OK(sr); MusicResult mr = audio_music_create(m); OK(mr);
    /* The exact maximum-length name fits; appending a suffix must not truncate it. */
    BAD(entity_description_add(e)); BAD(graphics_viewport_create(v)); BAD(input_controller_create(c));
    BAD(audio_sound_create(s)); BAD(audio_music_create(m));
    assert(!strcmp(entity_name_get(er.result.value).result.value.value, e.name));
    assert(!strcmp(graphics_viewport_name_get(vr.result.value).result.value.value, e.name));
    assert(!strcmp(input_controller_name_get(cr.result.value).result.value.value, e.name));
    assert(!strcmp(audio_sound_name_get(sr.result.value).result.value.value, e.name));
    assert(!strcmp(audio_music_name_get(mr.result.value).result.value.value, e.name));
    OK(entity_delete(er.result.value)); OK(graphics_viewport_destroy(vr.result.value));
    OK(input_controller_destroy(cr.result.value)); OK(audio_sound_destroy(&sr.result.value)); OK(audio_music_destroy(&mr.result.value));
    e.name[sizeof(e.name)-1] = v.name[sizeof(v.name)-1] = c.name[sizeof(c.name)-1] =
        s.name[sizeof(s.name)-1] = m.name[sizeof(m.name)-1] = 'x';
    BAD(entity_description_add(e)); BAD(graphics_viewport_create(v)); BAD(input_controller_create(c));
    BAD(audio_sound_create(s)); BAD(audio_music_create(m));
    BAD(entity_by_name_get(NULL)); BAD(graphics_viewport_by_name_get(NULL)); BAD(input_controller_by_name_get(NULL));
    BAD(audio_sound_by_name_get(NULL)); BAD(audio_music_by_name_get(NULL));
    BAD(rohr_entity_by_name_get("")); BAD(rohr_viewport_by_name_get("")); BAD(rohr_input_controller_by_name_get(""));
    BAD(rohr_audio_sound_by_name_get("")); BAD(rohr_audio_music_by_name_get(""));
    BAD(audio_sound_create((SoundDescription){.name="retry",.path="missing_runtime_names.wav"}));
    BAD(audio_music_create((MusicDescription){.name="retry",.path="missing_runtime_names.ogg"}));
    sr = audio_sound_create((SoundDescription){.name="retry",.path=AUDIO_TEST_PATH}); OK(sr);
    mr = audio_music_create((MusicDescription){.name="retry",.path=AUDIO_TEST_MUSIC_PATH}); OK(mr);
    assert(!strcmp(audio_sound_name_get(sr.result.value).result.value.value,"retry"));
    assert(!strcmp(audio_music_name_get(mr.result.value).result.value.value,"retry"));
    OK(audio_sound_destroy(&sr.result.value)); OK(audio_music_destroy(&mr.result.value));
}

static void generation_retirement(void) {
    EntityResult first = entity_add(); OK(first);
    Entity stale = first.result.value;
    OK(entity_delete(stale));
    for(unsigned i = 0; i < UINT16_MAX; ++i) {
        EntityResult next = entity_add(); OK(next);
        assert(next.result.value != stale && !entity_alive_check(stale));
        OK(entity_delete(next.result.value));
    }
}

int main(void) {
    assert(audio_test_wav_write() && audio_test_ogg_write());
    OK(rohr_engine_start()); OK(rohr_graphics_start()); OK(rohr_audio_start());
    namespaces(); limits(); generation_retirement();
    (void)rohr_audio_stop(); rohr_graphics_stop(); rohr_engine_stop();
    remove(AUDIO_TEST_PATH); remove(AUDIO_TEST_MUSIC_PATH);
    return 0;
}
