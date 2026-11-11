/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#define AUDIO_TEST_PATH "sdk_project.wav"
#define AUDIO_TEST_MUSIC_PATH "sdk_project.ogg"
#include "../audio_fixture.h"
#include <assert.h>
#include <string.h>
#define OK(call) assert(!rohr_error_check(call))

int main(void) {
    assert(audio_test_wav_write() && audio_test_ogg_write());
    OK(rohr_engine_start()); OK(rohr_graphics_start()); OK(rohr_audio_start());
    ProjectDescription recipe = {.root = {.id=1,.name="world",.kind=OBJECT_CONTAINER,.categories=ROHR_ENTITY_OBJECT_ALL}};
    ObjectDescription child = {.id=2,.name="body",.kind=OBJECT_BODY,.parent=&recipe.root,
        .categories=ROHR_ENTITY_OBJECT_ALL,.mass=1,.position={10,20}};
    const ObjectDescription *children[] = {&child};
    recipe.root.children=children; recipe.root.child_count=1;
    ViewportCreateDescription viewport = {.viewport={.enabled=true}};
    ProjectViewportDescription views[] = {{.name="view",.description=&viewport}};
    ControllerDescription controller = {.controller={.name="player",.enabled=true}};
    ProjectSoundDescription sounds[] = {{.name="sound",.description={.path=AUDIO_TEST_PATH}}};
    ProjectMusicDescription music[] = {{.name="music",.description={.path=AUDIO_TEST_MUSIC_PATH}}};
    recipe.viewports=views; recipe.viewport_count=1; recipe.controllers=&controller; recipe.controller_count=1;
    recipe.sounds=sounds; recipe.sound_count=1; recipe.music=music; recipe.music_count=1;
    ProjectId owners[2]; Entity bodies[2]; ViewportId view_ids[2];
    InputControllerId controllers[2]; Sound sound_ids[2]; Music music_ids[2];
    for(size_t i=0;i<2;++i) {
        ProjectIdResult created=rohr_project_create(recipe); OK(created); owners[i]=created.result.value;
        EntityResult body=rohr_entity_by_name_get(i ? "body_1" : "body"); OK(body); bodies[i]=body.result.value;
        ViewportIdResult view=rohr_viewport_by_name_get(i ? "view_1" : "view"); OK(view); view_ids[i]=view.result.value;
        InputControllerIdResult input=rohr_input_controller_by_name_get(i ? "player_1" : "player"); OK(input); controllers[i]=input.result.value;
        SoundResult sound=rohr_audio_sound_by_name_get(i ? "sound_1" : "sound"); OK(sound); sound_ids[i]=sound.result.value;
        MusicResult track=rohr_audio_music_by_name_get(i ? "music_1" : "music"); OK(track); music_ids[i]=track.result.value;
    }
    assert(bodies[0]!=bodies[1] && view_ids[0]!=view_ids[1]);
    Entity storage[4]; EntityList entities={.data=storage,.capacity=4}; OK(rohr_entity_list_get(&entities));
    assert(entities.count==4);
    OK(rohr_entity_delete(bodies[0])); OK(rohr_viewport_destroy(view_ids[0]));
    OK(rohr_audio_sound_destroy(&sound_ids[0]));
    EntityResult replacement=rohr_entity_description_add((EntityDescription){.name="body"}); OK(replacement);
    ViewportIdResult replacement_view=rohr_viewport_create((ViewportDescription){.name="view"}); OK(replacement_view);
    SoundResult replacement_sound=rohr_audio_sound_create((SoundDescription){.name="sound",.path=AUDIO_TEST_PATH}); OK(replacement_sound);
    OK(rohr_project_destroy(owners[0]));
    assert(rohr_error_check(rohr_project_destroy(owners[0])));
    assert(rohr_entity_alive_check(replacement.result.value) && rohr_entity_alive_check(bodies[1]));
    OK(rohr_viewport_name_get(replacement_view.result.value));
    OK(rohr_audio_sound_name_get(replacement_sound.result.value));
    assert(rohr_error_check(rohr_input_controller_name_get(controllers[0])));
    assert(rohr_error_check(rohr_audio_music_name_get(music_ids[0])));
    OK(rohr_project_destroy(owners[1]));
    assert(!rohr_entity_alive_check(bodies[1]));
    assert(rohr_error_check(rohr_viewport_name_get(view_ids[1])));
    assert(rohr_error_check(rohr_audio_sound_name_get(sound_ids[1])));
    OK(rohr_entity_delete(replacement.result.value)); OK(rohr_viewport_destroy(replacement_view.result.value));
    OK(rohr_audio_sound_destroy(&replacement_sound.result.value));
    assert(rohr_entity_alive_count_get()==0);
    (void)rohr_audio_stop(); rohr_graphics_stop(); rohr_engine_stop();
    assert(remove(AUDIO_TEST_PATH)==0 && remove(AUDIO_TEST_MUSIC_PATH)==0);
    return 0;
}
