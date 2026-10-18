/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
/* Compile the actual developer-owned tutorial, including its creation path. */
#define main tutorial_application_main
#include "main.c"
#undef main
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
    ProjectViewports viewports = {0};
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    if(tutorial) assert(create_tutorial(&scene));
    else OK(project_objects_create_all(&scene));
    assert(scene.created);
    assert(scene.starter.source_instance && scene.tug.source_instance);
    joint_check(scene.starter.joint_revolute_joint,
        scene.starter.joint_chassis, scene.starter.revolute_wheel);
    joint_check(scene.starter.joint_weld_joint,
        scene.starter.joint_chassis, scene.starter.welded_body);
    joint_check(scene.starter.joint_spring_joint,
        scene.starter.joint_chassis, scene.starter.spring_body);
    tug_check(&scene.tug);
    EntityInfoResult soft = rohr_entity_info_get(scene.starter.soft_body,
        ROHR_ENTITY_INFO_PHYSICS_SOFT_BODY);
    OK(soft);
    OK(rohr_entity_info_release(&soft.result.value));
    OK(project_viewports_create(&viewports, &scene));
    assert(viewports.viewport_count == 1 && viewports.screen_count == 1);

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
    assert(rohr_graphics_show().header.status != ROHR_REPORT_FAILED);
    project_viewports_destroy(&viewports);
    project_objects_destroy_all(&scene);
    assert(rohr_entity_alive_count_get() == 0);
    rohr_graphics_stop(); rohr_engine_stop();
}

static void failure_check(void) {
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    EntityResult added = rohr_entity_add(); OK(added);
    Starter output = {.source_instance = added.result.value};
    Starter original = output;
    assert(rohr_error_check(starter_instance_bind(&output, added.result.value)));
    assert(memcmp(&output, &original, sizeof(output)) == 0);
    assert(rohr_entity_alive_check(added.result.value));
    OK(rohr_entity_delete(added.result.value));
    assert(!own(entity_object_add(objects.starter.box), added.result.value));
    assert(rohr_entity_alive_count_get() == 0);
    /* Fail after several successful spawns and verify tutorial rollback. */
    assert(SDL_RenamePath("assets/tutorial_frame_1.png", "assets/tutorial_frame_1.saved"));
    ProjectObjects scene = {0};
    bool created = create_tutorial(&scene);
    assert(SDL_RenamePath("assets/tutorial_frame_1.saved", "assets/tutorial_frame_1.png"));
    assert(!created);
    project_objects_destroy_all(&scene);
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
