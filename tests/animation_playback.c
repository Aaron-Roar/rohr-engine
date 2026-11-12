/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "test_png.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OK(call) assert(!rohr_error_check(call))
#define BAD(call) assert(rohr_error_check(call))
static Entity take(EntityResult result) { OK(result); return result.result.value; }
static size_t frame(Entity entity) {
    AnimationFrameIndexResult result = rohr_graphics_animated_sprite_frame_index_get(entity);
    OK(result); return result.result.value;
}
static void update(Tick tick, Time time) {
    Report report = rohr_graphics_sprite_frames_update(tick, time);
    assert(report.header.status != ROHR_REPORT_FAILED && !report.has_fatal);
}

static void standalone(AnimationAsset asset) {
    AnimationPlayerDescription description = rohr_graphics_animation_player_description_default_get();
    assert(description.playback_mode == ROHR_ANIMATION_PLAYBACK_LOOP);
    description.animation = asset; description.ticks_per_frame = 2;
    description.playback_mode = ROHR_ANIMATION_PLAYBACK_ONCE;
    AnimationPlayerResult created = rohr_graphics_animation_player_create(description); OK(created);
    AnimationPlayer p = created.result.value;
    rohr_graphics_animation_player_update(&p, 100, 10);
    OK(rohr_graphics_animation_player_restart(&p));
    graphics_animation_player_update(&p, 101, 10);
    assert(p.frame_index == 0 && !p.completed);
    graphics_animation_player_update(&p, 102, 10);
    assert(p.frame_index == 1 && !p.completed);
    graphics_animation_player_update(&p, 104, 10);
    assert(p.frame_index == 2 && !p.completed); /* Last frame must finish its interval. */
    graphics_animation_player_update(&p, 105, 10);
    assert(!p.completed);
    graphics_animation_player_update(&p, 106, 10);
    assert(p.frame_index == 2 && graphics_animation_player_completed_check(&p));
    graphics_animation_player_update(&p, 200, 20);
    assert(p.frame_index == 2 && rohr_graphics_animation_player_completed_check(&p));
    OK(rohr_graphics_animation_player_loop_set(&p, true));
    assert(graphics_animation_player_loop_check(&p) && rohr_graphics_animation_player_loop_check(&p));
    graphics_animation_player_update(&p, 300, 30);
    assert(p.completed && p.frame_index == 2);
    OK(rohr_graphics_animation_player_completed_set(&p, false));
    graphics_animation_player_update(&p, 301, 30);
    assert(p.frame_index == 2 && !p.completed);
    graphics_animation_player_update(&p, 302, 30);
    assert(p.frame_index == 0 && !p.completed);
    OK(graphics_animation_player_completed_set(&p, true));
    assert(p.frame_index == 0); /* Manual completion never seeks. */
    graphics_animation_player_update(&p, 400, 40);
    OK(graphics_animation_player_restart(&p));
    assert(!p.completed && p.frame_index == 0 && p.last_update_tick == 400);

    /* Mode-only description replacement preserves all progress and completion. */
    OK(graphics_animation_player_completed_set(&p, true));
    AnimationPlayerDescriptionResult queried = graphics_animation_player_description_get(&p); OK(queried);
    queried.result.value.playback_mode = ROHR_ANIMATION_PLAYBACK_ONCE;
    OK(graphics_animation_player_description_set(&p, queried.result.value));
    assert(p.completed && p.last_update_tick == 400);
    AnimationPlayer saved = p;
    queried.result.value.playback_mode = (AnimationPlaybackMode)99;
    BAD(rohr_graphics_animation_player_description_set(&p, queried.result.value));
    assert(memcmp(&p, &saved, sizeof(p)) == 0);

    /* Existing either-clock and one-step behavior, including large clock jumps. */
    OK(graphics_animation_player_restart(&p));
    p.time_per_frame = .5; p.ticks_per_frame = 100;
    graphics_animation_player_update(&p, 400, 40.5);
    assert(p.frame_index == 1 && !p.completed);
    graphics_animation_player_update(&p, 500, 40.5);
    assert(p.frame_index == 2 && !p.completed);
    graphics_animation_player_update(&p, 500, 41);
    assert(p.completed);
    OK(graphics_animation_player_restart(&p));
    graphics_animation_player_update(&p, 10000, 1000);
    assert(p.frame_index == 1 && !p.completed);
    p.frame_count = 1;
    OK(graphics_animation_player_restart(&p));
    graphics_animation_player_update(&p, 10000, 1000.25);
    assert(!p.completed);
    graphics_animation_player_update(&p, 10000, 1000.5);
    assert(p.completed && p.frame_index == 0);
    OK(graphics_animation_player_loop_set(&p, true));
    OK(graphics_animation_player_completed_set(&p, false));
    graphics_animation_player_update(&p, 10000, 1001);
    assert(!p.completed && p.frame_index == 0);
    OK(graphics_animation_player_loop_set(&p, false));
    p.frame_count = 0;
    OK(graphics_animation_player_restart(&p));
    graphics_animation_player_update(&p, 20000, 2000);
    assert(!p.completed);
    p.frame_count = 3; p.ticks_per_frame = 0; p.time_per_frame = 0;
    graphics_animation_player_update(&p, 30000, 3000);
    assert(!p.completed && p.frame_index == 0);
    BAD(graphics_animation_player_restart(NULL));
    BAD(rohr_graphics_animation_player_completed_set(NULL, true));
    BAD(graphics_animation_player_loop_set(NULL, false));
    assert(!graphics_animation_player_loop_check(NULL));
    assert(!rohr_graphics_animation_player_completed_check(NULL));
}

static void entities(AnimatedSpriteDescription d) {
    Entity body = take(rohr_entity_description_add((EntityDescription){.name="body"}));
    d.player.playback_mode = ROHR_ANIMATION_PLAYBACK_ONCE;
    d.player.ticks_per_frame = 2;
    Entity a = take(rohr_graphics_animated_sprite_attachment_add(body, "once", d));
    d.player.playback_mode = ROHR_ANIMATION_PLAYBACK_LOOP;
    Entity b = take(graphics_animated_sprite_attachment_add(body, "loop", d));
    Tick start = rohr_engine_tick_get(); Time time = rohr_engine_time_get();
    update(start + 2, time); assert(frame(a) == 1 && frame(b) == 1);
    OK(rohr_graphics_animated_sprite_loop_set(a, true));
    OK(graphics_animated_sprite_loop_set(a, false)); /* No reset on either change. */
    assert(frame(a) == 1);
    AnimatedSpriteDescriptionResult current = rohr_graphics_animated_sprite_description_get(a); OK(current);
    current.result.value.visible = false;
    OK(rohr_graphics_animated_sprite_description_set(a, current.result.value));
    update(start + 4, time); assert(frame(a) == 2 && !rohr_graphics_animated_sprite_completed_check(a));
    update(start + 6, time);
    assert(frame(a) == 2 && frame(b) == 0 && graphics_animated_sprite_completed_check(a));
    assert(!rohr_graphics_animated_sprite_completed_check(b));
    current.result.value.direction = DIRECTION_LEFT;
    current.result.value.player.playback_mode = ROHR_ANIMATION_PLAYBACK_LOOP;
    OK(graphics_animated_sprite_description_set(a, current.result.value));
    assert(rohr_graphics_animated_sprite_loop_check(a) && graphics_animated_sprite_completed_check(a));
    OK(rohr_graphics_animated_sprite_frame_index_set(a, 1));
    assert(graphics_animated_sprite_completed_check(a));
    update(start + 100, time); assert(frame(a) == 1);
    OK(rohr_graphics_animated_sprite_completed_set(a, false));
    update(start + 1, time); assert(frame(a) == 1); /* Entity reset uses engine clocks. */
    update(start + 2, time); assert(frame(a) == 2);
    OK(graphics_animated_sprite_restart(a)); assert(frame(a) == 0);
    OK(graphics_animated_sprite_completed_set(a, true));
    AnimatedSpriteDescription invalid = current.result.value;
    invalid.player.playback_mode = (AnimationPlaybackMode)-1;
    BAD(graphics_animated_sprite_description_set(a, invalid));
    assert(graphics_animated_sprite_completed_check(a));
    current.result.value.frames[0].id = 99; /* Replacing current identity restarts. */
    OK(graphics_animated_sprite_description_set(a, current.result.value));
    assert(!graphics_animated_sprite_completed_check(a) && frame(a) == 0);
    OK(graphics_animated_sprite_completed_set(a, true));
    current.result.value.frames[0].id = 100;
    OK(rohr_graphics_animated_sprite_frame_description_set(a, 0, current.result.value.frames[0]));
    assert(!graphics_animated_sprite_completed_check(a));
    BAD(rohr_graphics_animated_sprite_restart(body));
    BAD(rohr_graphics_animated_sprite_loop_set(ENTITY_INVALID, true));
    assert(!graphics_animated_sprite_completed_check(body) && !graphics_animated_sprite_loop_check(body));
    OK(rohr_entity_delete(a));
    Entity reused = take(rohr_entity_add()); assert(reused != a);
    BAD(graphics_animated_sprite_completed_set(a, false));
    assert(!rohr_graphics_animated_sprite_loop_check(a));
    assert(rohr_entity_alive_check(b));
    OK(rohr_entity_delete(body)); OK(rohr_entity_delete(reused));
}

static void authored(void) {
    ObjectFrame frames[] = {{.id=1,.path="animation_playback.png",.scale={1,1}},
        {.id=2,.path="animation_playback.png",.scale={1,1}}};
    ObjectDescription d = {.id=1,.name="authored",.kind=OBJECT_ANIMATION,
        .categories=ROHR_ENTITY_OBJECT_ALL,.frames=frames,.frame_count=2,.scale={1,1},
        .visible=true,.playing=true,.ticks_per_frame=1,.starting_frame=1,
        .playback_mode=ROHR_ANIMATION_PLAYBACK_ONCE};
    Entity e = take(rohr_entity_create(d)); assert(frame(e) == 1);
    update(rohr_engine_tick_get()+1, rohr_engine_time_get());
    assert(rohr_graphics_animated_sprite_completed_check(e) && frame(e)==1);
    OK(rohr_graphics_animated_sprite_restart(e)); assert(frame(e)==0);
    OK(rohr_entity_delete(e));
    d.playback_mode = (AnimationPlaybackMode)99;
    BAD(rohr_entity_create(d)); assert(rohr_entity_alive_count_get()==0);
}

/* Mutate only saved playback fields to exercise old saves and malformed input. */
static char *field_replace(const char *text, const char *key, const char *replacement) {
    char needle[64]; snprintf(needle,sizeof(needle),"\"%s\"",key);
    const char *first = strstr(text,needle); assert(first);
    const char *end = strchr(first,','); assert(end); ++end;
    size_t prefix = (size_t)(first-text), suffix = strlen(end), n = strlen(replacement);
    char *out = malloc(prefix+n+suffix+1); assert(out);
    memcpy(out,text,prefix); memcpy(out+prefix,replacement,n); memcpy(out+prefix+n,end,suffix+1);
    return out;
}
static void persistence_and_driver(AnimatedSpriteDescription d) {
    Entity body = take(rohr_entity_description_add((EntityDescription){.name="saved_body"}));
    Shape first = {.amount_of_vertices=3,.vertices={{0,0},{1,0},{0,1}}};
    Shape last = {.amount_of_vertices=3,.vertices={{0,0},{2,0},{0,2}}};
    OK(rohr_physics_hitbox_add(body, first)); OK(rohr_physics_hitbox_add(body, last));
    d.player.playback_mode = ROHR_ANIMATION_PLAYBACK_ONCE; d.player.ticks_per_frame = 1;
    Entity e = take(rohr_graphics_animated_sprite_attachment_add(body,"saved_animation",d));
    OK(rohr_physics_hitbox_animation_driver_set(body,e));
    OK(rohr_physics_hitbox_animation_binding_at_set(body,0,0));
    OK(rohr_physics_hitbox_animation_binding_at_set(body,2,1));
    OK(rohr_engine_time_per_tick_set(.001));
    for(unsigned step=0;step<100 && !rohr_graphics_animated_sprite_completed_check(e);++step) {
        SDL_Delay(2); (void)rohr_engine_update();
        update(rohr_engine_tick_get(),rohr_engine_time_get());
        rohr_physics_hitbox_animation_bindings_update();
    }
    assert(rohr_graphics_animated_sprite_completed_check(e) && frame(e)==2);
    assert(rohr_physics_hitbox_active_index_get(body).result.value==1);
    update(rohr_engine_tick_get(),rohr_engine_time_get());
    rohr_physics_hitbox_animation_bindings_update();
    assert(rohr_physics_hitbox_active_index_get(body).result.value==1);
    OK(rohr_game_state_file_save("animation_playback.json"));
    OK(rohr_entity_delete(body));
    OK(rohr_game_state_file_load("animation_playback.json"));
    body=take(rohr_entity_by_name_get("saved_body")); e=take(rohr_entity_by_name_get("saved_animation"));
    assert(graphics_animated_sprite_completed_check(e) && !graphics_animated_sprite_loop_check(e) && frame(e)==2);
    assert(rohr_physics_hitbox_animation_driver_get(body).result.value==e);
    OK(rohr_graphics_animated_sprite_completed_set(e,false));
    update(rohr_engine_tick_get(),rohr_engine_time_get()); assert(!graphics_animated_sprite_completed_check(e));
    OK(rohr_graphics_animated_sprite_restart(e));
    OK(rohr_game_state_file_save("animation_playback_running.json"));
    OK(rohr_entity_delete(body));
    OK(rohr_game_state_file_load("animation_playback_running.json"));
    body=take(rohr_entity_by_name_get("saved_body")); e=take(rohr_entity_by_name_get("saved_animation"));
    assert(!graphics_animated_sprite_completed_check(e) && !graphics_animated_sprite_loop_check(e) && frame(e)==0);
    OK(rohr_entity_delete(body));
    char *json=SDL_LoadFile("animation_playback.json",NULL); assert(json);
    const char *keys[]={"completed","playback_mode","playback_mode"};
    const char *values[]={"\"completed\":42,","\"playback_mode\":99,","\"playback_mode\":null,"};
    for(size_t i=0;i<3;++i) {
        char *bad=field_replace(json,keys[i],values[i]);
        assert(SDL_SaveFile("animation_playback_bad.json",bad,strlen(bad))); free(bad);
        BAD(rohr_game_state_file_load("animation_playback_bad.json")); assert(rohr_entity_alive_count_get()==0);
    }
    char *legacy1=field_replace(json,"completed","");
    char *legacy2=field_replace(legacy1,"playback_mode",""); free(legacy1);
    assert(SDL_SaveFile("animation_playback_legacy.json",legacy2,strlen(legacy2))); free(legacy2); SDL_free(json);
    OK(rohr_game_state_file_load("animation_playback_legacy.json"));
    e=take(rohr_entity_by_name_get("saved_animation"));
    assert(graphics_animated_sprite_loop_check(e) && !graphics_animated_sprite_completed_check(e));
    OK(rohr_entity_delete(take(rohr_entity_by_name_get("saved_body"))));
}
int main(void) {
    assert(SDL_SaveFile("animation_playback.png",test_png,sizeof(test_png)));
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    AnimationAssetResult asset=rohr_graphics_animation_load((AnimationDescription){.id=71,
        .frame_count=3,.frame_ids={1,2,3},.frame_files={"animation_playback.png","animation_playback.png","animation_playback.png"}}); OK(asset);
    AnimatedSpriteDescriptionResult description=rohr_graphics_animated_sprite_description_from_animation_get(asset.result.value); OK(description);
    standalone(asset.result.value); entities(description.result.value); authored(); persistence_and_driver(description.result.value);
    OK(rohr_graphics_animation_release(&asset.result.value));
    assert(rohr_entity_alive_count_get()==0);
    rohr_graphics_stop(); rohr_engine_stop();
    const char *files[]={"animation_playback.png","animation_playback.json","animation_playback_running.json","animation_playback_bad.json","animation_playback_legacy.json"};
    for(size_t i=0;i<sizeof(files)/sizeof(files[0]);++i) assert(SDL_RemovePath(files[i]));
    puts("Animation playback modes, completion, restart, descriptions, driver and persistence passed");
    return 0;
}
