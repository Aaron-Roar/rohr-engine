/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "graphics/animation_assets.h"
#include "graphics/texture_assets.h"
#include "test_png.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OK(call) assert(!rohr_error_check(call))
#define BAD(call) assert(rohr_error_check(call))

static Entity value(EntityResult result) { OK(result); return result.result.value; }
static void near(float a, float b) { assert(fabsf(a - b) < .001f); }
static uint64_t metric(Report report, ReportMetricId id) {
    assert(rohr_report_valid_check(&report) && !report.has_fatal);
    for(size_t i = 0; i < report.statistic_count; ++i)
        if(report.statistics[i].metric_id == id) return report.statistics[i].value;
    return 0;
}

#ifdef ROHR_TEST_ATTACHMENT_WRAP
static int allocation_countdown, retain_countdown;
static bool allocation_failed;
void *__real_calloc(size_t count, size_t size);
void *__real_realloc(void *pointer, size_t size);
static bool allocation_fail(void) {
    if(allocation_countdown > 0 && --allocation_countdown == 0) {
        allocation_failed = true;
        return true;
    }
    return false;
}
void *__wrap_calloc(size_t count, size_t size) {
    return allocation_fail() ? NULL : __real_calloc(count, size);
}
void *__wrap_realloc(void *pointer, size_t size) {
    return allocation_fail() ? NULL : __real_realloc(pointer, size);
}
EngineResult __real_graphics_texture_retain(TextureAsset asset);
EngineResult __wrap_graphics_texture_retain(TextureAsset asset) {
    if(retain_countdown > 0 && --retain_countdown == 0)
        return rohr_error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    return __real_graphics_texture_retain(asset);
}
#endif

static void controls(AnimatedSpriteDescription description) {
    Entity body = value(rohr_entity_add());
    OK(rohr_physics_position_set(body, (Position){10,20}));
    OK(rohr_physics_orientation_set(body, 90));
    description.body_offset = (Position){3,0};
    description.orientation_offset = 12;
    description.player.ticks_per_frame = 1;
    Entity a = value(rohr_graphics_animated_sprite_attachment_add(body, "animation", description));
    description.player.ticks_per_frame = 3;
    description.body_offset = (Position){0,4};
    description.follow_entity_rotation = false;
    Entity b = value(graphics_animated_sprite_attachment_add(body, "animation", description));
    assert(a != b && a != body && b != body);
    assert(value(rohr_entity_by_name_get("animation")) == a);
    assert(value(rohr_entity_by_name_get("animation_1")) == b);
    assert(value(rohr_entity_owner_get(a)) == body);
    assert(value(graphics_animated_sprite_attachment_get(b)) == body);
    assert(!rohr_entity_components_check(body, ROHR_ANIMATED_SPRITE));

    AnimatedSpritePoseResult pose = rohr_graphics_animated_sprite_pose_get(a); OK(pose);
    near(pose.result.value.position.x, 10); near(pose.result.value.position.y, 17);
    near(pose.result.value.orientation, 102);
    pose = graphics_animated_sprite_pose_get(b); OK(pose);
    near(pose.result.value.position.x, 14); near(pose.result.value.position.y, 20);
    near(pose.result.value.orientation, 12);
    OK(rohr_physics_position_set(body, (Position){20,30}));
    pose = rohr_graphics_animated_sprite_pose_get(a); OK(pose);
    near(pose.result.value.position.x, 20); near(pose.result.value.position.y, 27);

    Entity ids[2] = {123,456};
    EntityList list = {0};
    BAD(rohr_graphics_animated_sprite_attachment_list_get(body, &list)); assert(list.count == 2);
    list = (EntityList){ids,1,0};
    BAD(graphics_animated_sprite_attachment_list_get(body, &list));
    assert(list.count == 2 && ids[0] == 123 && ids[1] == 456);
    list = (EntityList){NULL,2,0};
    BAD(graphics_animated_sprite_attachment_list_get(body, &list)); assert(list.count == 2);
    BAD(graphics_animated_sprite_attachment_list_get(body, NULL));
    list = (EntityList){ids,2,0}; OK(rohr_graphics_animated_sprite_attachment_list_get(body, &list));
    assert(list.count == 2 && ((ids[0] == a && ids[1] == b) || (ids[0] == b && ids[1] == a)));

    Tick tick = rohr_engine_tick_get();
    Report report = rohr_graphics_sprite_frames_update(tick + 1, rohr_engine_time_get());
    assert(metric(report, ROHR_REPORT_PROCESSED_COUNT) == 2);
    assert(rohr_graphics_animated_sprite_frame_index_get(a).result.value == 1);
    assert(rohr_graphics_animated_sprite_frame_index_get(b).result.value == 0);
    OK(rohr_graphics_animated_sprite_frame_scale_set(a, 0, (Scale){2,3}));
    assert(rohr_graphics_animated_sprite_frame_scale_get(b, 0).result.value.x == 1);
    OK(rohr_graphics_layer_animation_set(a, 2)); OK(rohr_graphics_layer_animation_set(b, 5));
    assert(rohr_graphics_layer_animation_get(a).result.value == 2);
    assert(rohr_graphics_layer_animation_get(b).result.value == 5);
    assert(rohr_graphics_animated_sprite_draw(a) && rohr_graphics_animated_sprite_draw(b));
    report = rohr_graphics_pipeline_commands_execute();
    assert(metric(report, ROHR_REPORT_PROCESSED_COUNT) == 2);
    AnimatedSpriteDescription edited = rohr_graphics_animated_sprite_description_get(a).result.value;
    edited.visible = false;
    OK(rohr_graphics_animated_sprite_description_set(a, edited));
    assert(value(rohr_graphics_animated_sprite_attachment_get(a)) == body);
    assert(rohr_graphics_animated_sprite_frame_index_get(a).result.value == 1);
    assert(!rohr_graphics_animated_sprite_draw(a) && rohr_graphics_animated_sprite_draw(b));
    report = rohr_graphics_pipeline_commands_execute(); assert(metric(report, ROHR_REPORT_PROCESSED_COUNT) == 1);
    EntityInfoResult info = rohr_entity_info_get(a, ROHR_ENTITY_INFO_GRAPHICS_ANIMATIONS); OK(info);
    assert(info.result.value.animation_count == 1 && info.result.value.animations[0].attachment_target == body);
    OK(rohr_entity_info_release(&info.result.value));

    /* Self/cyclic bindings fail without changing ownership or playback. */
    BAD(rohr_graphics_animated_sprite_attachment_set(a, a));
    OK(rohr_graphics_animated_sprite_attachment_set(b, a));
    BAD(rohr_graphics_animated_sprite_attachment_set(a, b));
    assert(value(rohr_entity_owner_get(a)) == body && value(rohr_entity_owner_get(b)) == a);
    pose = rohr_graphics_animated_sprite_pose_get(b); OK(pose);
    near(pose.result.value.orientation, 12);
    OK(rohr_graphics_animated_sprite_attachment_set(b, body));

    /* The original single-component operation remains separate and does not
     * replace attachments or change their transform origin. */
    description.body_offset = (Position){100,100};
    OK(rohr_graphics_animated_sprite_add(body, description));
    pose = rohr_graphics_animated_sprite_pose_get(a); OK(pose);
    near(pose.result.value.position.x, 20); near(pose.result.value.position.y, 27);
    assert(value(rohr_graphics_animated_sprite_attachment_get(body)) == ENTITY_INVALID);

    OK(rohr_entity_delete(a));
    Entity replacement = value(rohr_entity_add());
    BAD(rohr_graphics_animated_sprite_attachment_get(a));
    OK(rohr_entity_delete(body));
    assert(!rohr_entity_alive_check(b) && rohr_entity_alive_check(replacement));
    OK(rohr_entity_delete(replacement));
}

static void stale_target(AnimatedSpriteDescription description) {
    Entity target = value(rohr_entity_add());
    Entity child = value(rohr_graphics_animated_sprite_attachment_add(target, NULL, description));
    /* Lifetime ownership is independently editable; a stale drawing target must
     * not resolve to the next occupant of the physical slot. */
    OK(rohr_entity_owner_set(child, ENTITY_INVALID));
    OK(rohr_entity_delete(target));
    Entity replacement = value(rohr_entity_add());
    BAD(rohr_graphics_animated_sprite_pose_get(child));
    assert(value(rohr_graphics_animated_sprite_attachment_get(child)) == target);
    BAD(rohr_graphics_animated_sprite_attachment_set(child, target));
    OK(rohr_graphics_animated_sprite_attachment_set(child, replacement));
    OK(rohr_graphics_animated_sprite_pose_get(child));
    OK(rohr_entity_delete(replacement)); assert(!rohr_entity_alive_check(child));
}

static void growth_and_rollback(AnimatedSpriteDescription description) {
    Entity body = value(rohr_entity_add());
    Entity ids[256];
    GraphicsTextureAssetStats before = graphics_texture_assets_stats_get();
    for(size_t i = 0; i < 256; ++i)
        ids[i] = value(rohr_graphics_animated_sprite_attachment_add(body, "many", description));
    Entity snapshot[256]; EntityList list = {snapshot,256,0};
    OK(rohr_graphics_animated_sprite_attachment_list_get(body, &list)); assert(list.count == 256);
    for(size_t i = 0; i < 256; ++i) {
        bool found = false;
        for(size_t j = 0; j < list.count; ++j) if(ids[i] == snapshot[j]) found = true;
        assert(found);
    }
    OK(rohr_entity_delete(ids[17]));
    Entity unrelated = value(rohr_entity_add());
    OK(rohr_entity_delete(body));
    assert(rohr_entity_alive_check(unrelated));
    for(size_t i = 0; i < 256; ++i) assert(!rohr_entity_alive_check(snapshot[i]));
    assert(graphics_texture_assets_stats_get().owner_references == before.owner_references);
    OK(rohr_entity_delete(unrelated));

    body = value(rohr_entity_add());
    uint32_t count = rohr_entity_alive_count_get();
    AnimatedSpriteDescription invalid = description; invalid.scale.x = NAN;
    BAD(rohr_graphics_animated_sprite_attachment_add(body, "bad", invalid));
    BAD(rohr_graphics_animated_sprite_attachment_add(ENTITY_INVALID, "bad", description));
    char long_name[ENTITY_NAME_MAX + 1]; memset(long_name, 'x', sizeof(long_name)); long_name[ENTITY_NAME_MAX] = 0;
    BAD(rohr_graphics_animated_sprite_attachment_add(body, long_name, description));
    assert(rohr_entity_alive_count_get() == count);
    list = (EntityList){0}; OK(rohr_graphics_animated_sprite_attachment_list_get(body, &list)); assert(!list.count);
    OK(rohr_entity_delete(body));
#ifdef ROHR_TEST_ATTACHMENT_WRAP
    /* Exercise actual table-growth allocation failure, not only ownership
     * membership allocation after existing tables have already grown. */
    body = value(rohr_entity_add());
    Entity fillers[MAX_ENTITIES]; size_t filler_count = 0;
    size_t capacity = animated_sprites_pool.capacity;
    while(rohr_entity_alive_count_get() < capacity)
        fillers[filler_count++] = value(rohr_entity_add());
    count = rohr_entity_alive_count_get();
    allocation_countdown = 1; allocation_failed = false;
    BAD(rohr_graphics_animated_sprite_attachment_add(body, "growth_failure", description));
    allocation_countdown = 0;
    assert(allocation_failed && rohr_entity_alive_count_get() == count);
    assert(graphics_texture_assets_stats_get().owner_references == before.owner_references);
    Entity retry = value(rohr_graphics_animated_sprite_attachment_add(body, "growth_failure", description));
    assert(value(rohr_entity_owner_get(retry)) == body);
    OK(rohr_entity_delete(body));
    for(size_t i = 0; i < filler_count; ++i) OK(rohr_entity_delete(fillers[i]));
    unsigned failures = 0;
    for(int nth = 1; nth < 80; ++nth) {
        body = value(rohr_entity_add()); count = rohr_entity_alive_count_get();
        allocation_failed = false; allocation_countdown = nth;
        EntityResult result = rohr_graphics_animated_sprite_attachment_add(body, "failure", description);
        allocation_countdown = 0;
        if(allocation_failed) {
            BAD(result); ++failures;
            assert(rohr_entity_alive_count_get() == count);
            assert(!rohr_entity_components_check(body, ROHR_OBJECT));
            assert(graphics_texture_assets_stats_get().owner_references == before.owner_references);
        } else OK(result);
        OK(rohr_entity_delete(body));
        if(!allocation_failed) break;
    }
    assert(failures > 0);
    body = value(rohr_entity_add()); count = rohr_entity_alive_count_get();
    for(int nth = 1; nth <= 2; ++nth) {
        retain_countdown = nth;
        BAD(rohr_graphics_animated_sprite_attachment_add(body, "failure", description));
        retain_countdown = 0;
        assert(rohr_entity_alive_count_get() == count);
        assert(graphics_texture_assets_stats_get().owner_references == before.owner_references);
    }
    Entity kept = value(rohr_graphics_animated_sprite_attachment_add(body, "kept", description));
    Entity other = value(rohr_entity_add());
    allocation_countdown = 1; allocation_failed = false;
    BAD(rohr_graphics_animated_sprite_attachment_set(kept, other));
    allocation_countdown = 0;
    assert(allocation_failed && value(rohr_entity_owner_get(kept)) == body);
    assert(value(rohr_graphics_animated_sprite_attachment_get(kept)) == body);
    OK(rohr_entity_delete(other));
    OK(rohr_entity_delete(body));
#endif
}

static void persistence(AnimatedSpriteDescription description) {
    Entity body = value(rohr_entity_description_add((EntityDescription){.name="saved_body"}));
    OK(rohr_physics_position_set(body, (Position){23,45}));
    Entity a = value(rohr_graphics_animated_sprite_attachment_add(body, "saved_a", description));
    description.body_offset = (Position){7,9}; description.visible = false;
    Entity b = value(rohr_graphics_animated_sprite_attachment_add(body, "saved_b", description));
    Entity other = value(rohr_entity_description_add((EntityDescription){.name="saved_owner"}));
    Entity c = value(rohr_graphics_animated_sprite_attachment_add(body, "saved_c", description));
    OK(rohr_entity_owner_set(c, other));
    OK(rohr_graphics_animated_sprite_frame_index_set(a, 1));
    OK(rohr_graphics_layer_animation_set(b, 27));
    OK(rohr_game_state_file_save("animation_attachments_state.json"));
    OK(rohr_entity_delete(body));
    OK(rohr_entity_delete(other));
    OK(rohr_game_state_file_load("animation_attachments_state.json"));
    body = value(rohr_entity_by_name_get("saved_body"));
    a = value(rohr_entity_by_name_get("saved_a")); b = value(rohr_entity_by_name_get("saved_b"));
    c = value(rohr_entity_by_name_get("saved_c")); other = value(rohr_entity_by_name_get("saved_owner"));
    assert(value(rohr_entity_owner_get(c)) == other);
    assert(value(rohr_graphics_animated_sprite_attachment_get(c)) == body);
    assert(value(rohr_graphics_animated_sprite_attachment_get(a)) == body);
    assert(value(rohr_graphics_animated_sprite_attachment_get(b)) == body);
    assert(value(rohr_entity_owner_get(b)) == body);
    assert(rohr_graphics_animated_sprite_frame_index_get(a).result.value == 1);
    assert(!rohr_graphics_animated_sprite_description_get(b).result.value.visible);
    assert(rohr_graphics_layer_animation_get(b).result.value == 27);
    AnimatedSpritePoseResult pose = rohr_graphics_animated_sprite_pose_get(b); OK(pose);
    near(pose.result.value.position.x, 30); near(pose.result.value.position.y, 54);
    OK(rohr_entity_delete(body)); assert(!rohr_entity_alive_check(a) && !rohr_entity_alive_check(b));
    assert(rohr_entity_alive_check(c)); BAD(rohr_graphics_animated_sprite_pose_get(c));
    OK(rohr_entity_delete(other)); assert(!rohr_entity_alive_check(c));
}

int main(void) {
    assert(SDL_SaveFile("animation_attachment_frame.png", test_png, sizeof(test_png)));
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    AnimationAssetResult asset = rohr_graphics_animation_load((AnimationDescription){
        .id=1,.frame_count=2,.frame_ids={1,2},
        .frame_files={"animation_attachment_frame.png","animation_attachment_frame.png"},.ticks_per_frame=1});
    OK(asset);
    AnimatedSpriteDescriptionResult description = rohr_graphics_animated_sprite_description_from_animation_get(asset.result.value);
    OK(description);
    controls(description.result.value);
    stale_target(description.result.value);
    growth_and_rollback(description.result.value);
    persistence(description.result.value);
    OK(rohr_graphics_animation_release(&asset.result.value));
    assert(graphics_animation_assets_stats_get().live_resources == 0);
    assert(graphics_texture_assets_stats_get().live_resources == 0);
    assert(rohr_entity_alive_count_get() == 0);
    rohr_graphics_stop(); rohr_engine_stop();
    (void)SDL_RemovePath("animation_attachment_frame.png");
    (void)SDL_RemovePath("animation_attachments_state.json");
    puts("Independent animation attachments: controls, poses, ownership, growth, rollback and persistence passed");
    return 0;
}
