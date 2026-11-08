/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "project.h"
#include <assert.h>
#include <math.h>
#include <string.h>

#define OK(value) assert(!rohr_error_check(value))

static void joint_check(Entity entity, Entity a, Entity b) {
    EntityInfoResult result = rohr_entity_info_get(entity, ROHR_ENTITY_INFO_PHYSICS_JOINT);
    OK(result);
    assert(rohr_entity_info_check(&result.result.value, ROHR_ENTITY_INFO_PHYSICS_JOINT));
    assert(result.result.value.joint.a == a && result.result.value.joint.b == b);
    assert(result.result.value.joint.anchor_a && result.result.value.joint.anchor_b);
    OK(rohr_entity_info_release(&result.result.value));
}

static void tug_check(const Tug *tug) {
    joint_check(tug->joint_tug_spring, tug->box_1, tug->box_2);
    EntityResult target = rohr_physics_force_target_get(tug->force_pull_1);
    OK(target); assert(target.result.value == tug->box_1);
    target = rohr_physics_force_target_get(tug->force_pull_2);
    OK(target); assert(target.result.value == tug->box_2);
}

static void run_check(bool tutorial) {
    ProjectObjects scene = {0};
    Entity root = ENTITY_INVALID;
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    if(tutorial) {
        OK(project_settings_apply());
        EntityResult created = rohr_object_create(&project); OK(created); root = created.result.value;
        EntityResult example = rohr_object_member_get(root,project.example.name); OK(example);
        EntityResult tug = rohr_object_member_get(root,project.tug.name); OK(tug);
        OK(example_instance_bind(&scene.example,example.result.value));
        OK(tug_instance_bind(&scene.tug,tug.result.value));
        scene.created = true;
    }
    else OK(project_objects_create_all(&scene));
    assert(scene.created);
    assert(scene.example.source_instance && scene.tug.source_instance);
    joint_check(scene.example.joint_revolute_joint,
        scene.example.joint_chassis, scene.example.revolute_wheel);
    joint_check(scene.example.joint_weld_joint,
        scene.example.joint_chassis, scene.example.welded_body);
    joint_check(scene.example.joint_spring_joint,
        scene.example.joint_chassis, scene.example.spring_body);
    tug_check(&scene.tug);
    EntityInfoResult soft = rohr_entity_info_get(scene.example.soft_body,
        ROHR_ENTITY_INFO_PHYSICS_SOFT_BODY);
    OK(soft);
    OK(rohr_entity_info_release(&soft.result.value));
    assert(project.viewport_count == 1 && project.initial_viewport.item_count >= 1);
    ViewportCameraBinding binding = {.item_name=project.initial_viewport.items[0].name,
        .camera=scene.example.camera_main_camera};
    ViewportCreateDescription description = {.template=&project.initial_viewport,.cameras=&binding,.camera_count=1};
    ViewportIdResult viewport = rohr_viewport_create(&description); OK(viewport);

    /* Another whole object must have entirely independent bindings. */
    Tug second = {0};
    EntityResult added = entity_object_add(objects.tug); OK(added);
    OK(tug_instance_bind(&second, added.result.value));
    assert(second.box_1 != scene.tug.box_1);
    tug_check(&second);
    Entity deleted = second.box_1;
    tug_destroy(&second);
    assert(!rohr_entity_alive_check(deleted));
    tug_check(&scene.tug);

    PositionResult before_a = rohr_physics_position_get(scene.tug.box_1);
    PositionResult before_b = rohr_physics_position_get(scene.tug.box_2);
    OK(before_a); OK(before_b);
    for(unsigned tick = 0; tick < 120; ++tick) {
        assert(rohr_physics_update(1).header.status != ROHR_REPORT_FAILED);
        assert(rohr_graphics_sprite_frames_update(tick, tick / 60.0).header.status != ROHR_REPORT_FAILED);
    }
    PositionResult after_a = rohr_physics_position_get(scene.tug.box_1);
    PositionResult after_b = rohr_physics_position_get(scene.tug.box_2);
    OK(after_a); OK(after_b);
    assert(after_a.result.value.x < before_a.result.value.x);
    assert(after_b.result.value.x > before_b.result.value.x);
    assert(rohr_graphics_viewport_show(viewport.result.value).header.status != ROHR_REPORT_FAILED);
    OK(rohr_viewport_destroy(viewport.result.value));
    if(root) OK(rohr_entity_delete(root));
    else project_objects_destroy_all(&scene);
    assert(rohr_entity_alive_count_get() == 0);
    rohr_graphics_stop(); rohr_engine_stop();
}

static void failure_check(void) {
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    EntityResult added = rohr_entity_add(); OK(added);
    Example output = {.source_instance = added.result.value};
    Example original = output;
    assert(rohr_error_check(example_instance_bind(&output, added.result.value)));
    assert(memcmp(&output, &original, sizeof(output)) == 0);
    assert(rohr_entity_alive_check(added.result.value));
    OK(rohr_entity_delete(added.result.value));
    EntityCreateOptions options = {.owner=added.result.value};
    assert(rohr_error_check(rohr_entity_create_with_options(&project.example.box, options)));
    assert(rohr_entity_alive_count_get() == 0);
    /* Fail after several successful spawns and verify tutorial rollback. */
    assert(SDL_RenamePath("assets/example_frame_1.png", "assets/example_frame_1.saved"));
    EntityResult created = rohr_object_create(&project);
    assert(SDL_RenamePath("assets/example_frame_1.saved", "assets/example_frame_1.png"));
    assert(rohr_error_check(created));
    assert(rohr_entity_alive_count_get() == 0);
    rohr_graphics_stop(); rohr_engine_stop();
}

int main(void) {
    OK(rohr_directory_working_set(rohr_directory_base_get()));
    run_check(true);
    run_check(false);
    failure_check();
    return 0;
}
