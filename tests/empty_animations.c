/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "graphics/animation_assets.h"
#include "graphics/texture_assets.h"
#include "test_png.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define OK(value) assert(!rohr_error_check(value))
static Entity member(Entity root, const char *name) {
    EntityResult result = rohr_entity_object_member_by_name_get(root, name);
    OK(result); return result.result.value;
}
static EntityInfo empty_info(Entity entity) {
    EntityInfoResult result = rohr_entity_info_get(entity, ROHR_ENTITY_INFO_GRAPHICS_ANIMATIONS);
    OK(result);
    EntityInfo info = result.result.value;
    assert(info.animation_count == 1 && info.animations[0].frame_count == 0);
    EntityInfoIssue issue;
    EntityInfoIssueReadResult issues = rohr_entity_info_issues_get(&info,
        ROHR_ENTITY_INFO_GRAPHICS_ANIMATIONS, &issue, 1); OK(issues);
    assert(issues.result.value.total == 1);
    assert(issue.kind == ROHR_ENTITY_INFO_INFORMATION && issue.reason == ROHR_ENTITY_INFO_ANIMATION_EMPTY);
    assert(issue.reference.id == entity && issue.has_index && issue.index == 0);
    assert(rohr_entity_info_check(&info, ROHR_ENTITY_INFO_GRAPHICS_ANIMATIONS));
    return info;
}
static uint64_t metric(Report r, ReportMetricId id) {
    for(size_t i = 0; i < r.statistic_count; ++i)
        if(r.statistics[i].metric_id == id) return r.statistics[i].value;
    return 0;
}
static void reports_and_persistence(void) {
    EntityResult added = rohr_entity_add(); OK(added); Entity entity = added.result.value;
    OK(rohr_entity_name_set(entity, "empty_animation"));
    AnimatedSprite empty = rohr_graphics_animated_sprite_create((AnimationAsset){0}, (Scale){2,3});
    empty.player.ticks_per_frame = 7; empty.player.time_per_frame = .25;
    empty.body_offset = (Position){4,5}; empty.orientation_offset = 33;
    OK(rohr_graphics_animated_sprite_add(entity, empty));
    OK(rohr_graphics_layer_animation_set(entity, 12));
    EntityInfo snapshot = empty_info(entity);
    Report report = rohr_graphics_sprite_frames_update(100, 20);
    assert(rohr_report_valid_check(&report) && report.header.status == ROHR_REPORT_COMPLETE && !report.has_fatal);
    assert(metric(report, ROHR_REPORT_PROCESSED_COUNT) == 0 && metric(report, ROHR_REPORT_SKIPPED_COUNT) == 1);
    assert(report.diagnostic_count == 1 && report.diagnostics[0].severity == ROHR_REPORT_SEVERITY_INFORMATION);
    assert(report.diagnostics[0].code == ERROR_NONE && report.diagnostics[0].source.id == entity);
    ReportResult stored = rohr_report_get(ROHR_GRAPHICS_ANIMATIONS_REPORT); OK(stored);
    assert(memcmp(&stored.result.value, &report, sizeof(report)) == 0);
    EntityInfo current = empty_info(entity);
    assert(memcmp(current.animations, snapshot.animations, sizeof(AnimatedSprite)) == 0);
    OK(rohr_entity_info_release(&current));
    assert(rohr_graphics_animated_sprite_draw(entity));
    Report commands = rohr_graphics_pipeline_commands_execute();
    assert(!commands.has_fatal && !commands.diagnostic_count && metric(commands, ROHR_REPORT_PROCESSED_COUNT) == 0);
    OK(rohr_game_state_file_save("empty_animation_state.json"));
    OK(rohr_entity_delete(entity));
    OK(rohr_game_state_file_load("empty_animation_state.json"));
    added = rohr_entity_by_name_get("empty_animation"); OK(added); entity = added.result.value;
    current = empty_info(entity);
    assert(current.animations[0].scale.y == 3 && current.animations[0].body_offset.x == 4);
    assert(current.animations[0].orientation_offset == 33 && current.animations[0].player.ticks_per_frame == 7);
    assert(current.animations[0].player.time_per_frame == .25);
    assert(rohr_graphics_layer_animation_get(entity).result.value == 12);
    OK(rohr_entity_info_release(&current));
    TextureAssetResult texture = rohr_graphics_texture_load((TextureDescription){"empty_animation_frame.png",{2,2}}); OK(texture);
    AnimationFrame frame = rohr_graphics_animation_frame_create(texture.result.value);
    OK(rohr_graphics_animated_sprite_frame_add(entity, frame));
    OK(rohr_graphics_texture_release(&texture.result.value));
    report = rohr_graphics_sprite_frames_update(101, 21);
    assert(!report.diagnostic_count && metric(report, ROHR_REPORT_PROCESSED_COUNT) == 1);
    /* Replacing the component with its last frame removed releases frame assets. */
    OK(rohr_graphics_animated_sprite_add(entity, empty));
    assert(graphics_texture_assets_stats_get().live_resources == 0);
    current = empty_info(entity); OK(rohr_entity_info_release(&current));
    assert(snapshot.animations[0].frame_count == 0);
    OK(rohr_entity_delete(entity)); OK(rohr_entity_info_release(&snapshot));
    OK(rohr_report_json_write(&stored.result.value, "empty_animation_report.json", true));
    OK(rohr_report_text_write(&stored.result.value, "empty_animation_report.txt"));
    assert(rohr_report_valid_check(&stored.result.value));
}
static void catalog(void) {
    ObjectDescription root = {.id=1,.name="scene",.kind=OBJECT_CONTAINER,.categories=ROHR_ENTITY_OBJECT_ALL,.visible=true};
    ObjectDescription body = {.id=2,.name="body",.kind=OBJECT_BODY,.parent=&root,.categories=ROHR_ENTITY_OBJECT_ALL,.mass=1,.visible=true};
    ObjectDescription attached = {.id=3,.name="attached",.kind=OBJECT_ANIMATION,.parent=&body,
        .target=2,.categories=ROHR_ENTITY_OBJECT_VISUALS,.scale={2,3},.position={4,5},.orientation=25,
        .playing=true,.ticks_per_frame=6,.visible=true,.follow_orientation=true};
    ObjectDescription free_animation = attached;
    free_animation.id=4; free_animation.name="free"; free_animation.target=0; free_animation.parent=&root;
    ObjectFrame frame = {.id=1,.path="empty_animation_frame.png",.scale={1,1}};
    ObjectDescription sibling = attached;
    sibling.id=5; sibling.name="sibling"; sibling.frames=&frame; sibling.frame_count=1;
    const ObjectDescription *body_children[]={&attached,&sibling}, *children[]={&body,&free_animation};
    body.children=body_children; body.child_count=2; root.children=children; root.child_count=2;
    EntityResult a=rohr_entity_object_description_add(&root,(ObjectSpawnDescription){.flags=ROHR_ENTITY_OBJECT_ALL}); OK(a);
    EntityResult b=rohr_entity_object_description_add(&root,(ObjectSpawnDescription){.flags=ROHR_ENTITY_OBJECT_ALL}); OK(b);
    Entity attached_a=member(a.result.value,"attached"), free_a=member(a.result.value,"free");
    assert(attached_a==member(a.result.value,"body"));
    assert(member(a.result.value,"sibling")!=attached_a);
    assert(free_a!=member(b.result.value,"free"));
    EntityInfo info=empty_info(attached_a);
    assert(info.animations[0].body_offset.x==4 && info.animations[0].orientation_offset==25);
    OK(rohr_entity_info_release(&info)); info=empty_info(free_a); OK(rohr_entity_info_release(&info));
    Report report=rohr_graphics_sprite_frames_update(1,1);
    assert(metric(report,ROHR_REPORT_SKIPPED_COUNT)==4 && metric(report,ROHR_REPORT_PROCESSED_COUNT)==2);
    assert(report.diagnostic_count==4 && !report.has_fatal);
    OK(rohr_entity_delete(a.result.value));
    info=empty_info(member(b.result.value,"attached")); OK(rohr_entity_info_release(&info));
    OK(rohr_entity_delete(b.result.value));
    /* Malformed nonempty descriptions must still fail and roll back. */
    free_animation.frame_count=1; free_animation.frames=NULL;
    a=rohr_entity_object_description_add(&root,(ObjectSpawnDescription){.flags=ROHR_ENTITY_OBJECT_ALL});
    assert(rohr_error_check(a));
    assert(graphics_animation_assets_stats_get().live_resources==0);
    assert(graphics_texture_assets_stats_get().live_resources==0);
}
int main(void) {
    assert(SDL_SaveFile("empty_animation_frame.png", test_png, sizeof(test_png)));
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    reports_and_persistence(); catalog();
    rohr_graphics_stop(); rohr_engine_stop();
    puts("Empty animation identity, reports, persistence, assets and independent spawns passed");
    return 0;
}
