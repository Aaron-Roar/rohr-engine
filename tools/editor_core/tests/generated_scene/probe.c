/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "project.h"
#include "../runtime_fixture.h"
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

static void tug_check(Entity tug) {
    EntityResult joint = fixture_entity_get(tug, "tug_spring"); OK(joint);
    EntityResult a = fixture_entity_get(tug, "box_1"); OK(a);
    EntityResult b = fixture_entity_get(tug, "box_2"); OK(b);
    joint_check(joint.result.value, a.result.value, b.result.value);
    EntityResult force = fixture_entity_get(tug, "pull_1"); OK(force);
    EntityResult target = rohr_physics_force_target_get(force.result.value); OK(target);
    assert(target.result.value == a.result.value);
    force = fixture_entity_get(tug, "pull_2"); OK(force);
    target = rohr_physics_force_target_get(force.result.value); OK(target);
    assert(target.result.value == b.result.value);
}

static void run_check(bool tutorial) {
    Entity root = ENTITY_INVALID, example = ENTITY_INVALID, tug = ENTITY_INVALID;
    OK(rohr_engine_start()); OK(rohr_graphics_start()); OK(project_settings_apply());
    if(tutorial) {
        EntityResult created = rohr_object_create(project); OK(created); root = created.result.value;
        EntityResult found = rohr_entity_by_name_get("Example"); OK(found); example = found.result.value;
        found = rohr_entity_by_name_get("Tug"); OK(found); tug = found.result.value;
    } else {
        EntityResult created = rohr_object_create(project.example); OK(created); example = created.result.value;
        created = rohr_object_create(project.tug); OK(created); tug = created.result.value;
    }
    const char *joint_names[] = {"revolute_joint", "weld_joint", "spring_joint"};
    const char *bodies[] = {"revolute_wheel", "welded_body", "spring_body"};
    EntityResult chassis = fixture_entity_get(example, "joint_chassis"); OK(chassis);
    for(size_t i = 0; i < 3; ++i) {
        EntityResult joint = fixture_entity_get(example, joint_names[i]); OK(joint);
        EntityResult body = fixture_entity_get(example, bodies[i]); OK(body);
        joint_check(joint.result.value, chassis.result.value, body.result.value);
    }
    tug_check(tug);
    EntityResult soft_entity = fixture_entity_get(example, "soft_body"); OK(soft_entity);
    EntityInfoResult soft = rohr_entity_info_get(soft_entity.result.value, ROHR_ENTITY_INFO_PHYSICS_SOFT_BODY); OK(soft);
    OK(rohr_entity_info_release(&soft.result.value));
    assert(project.viewport_count == 1 && project.initial_viewport.item_count >= 1);
    EntityResult camera_entity = fixture_entity_get(example, "main_camera"); OK(camera_entity);
    CameraIdResult camera = rohr_entity_object_camera_get(camera_entity.result.value); OK(camera);
    ViewportCameraBinding binding = {.item_name=project.initial_viewport.items[0].name, .camera=camera.result.value};
    ViewportCreateDescription description = {.template=&project.initial_viewport._description,.cameras=&binding,.camera_count=1};
    ViewportIdResult viewport = rohr_viewport_create(description); OK(viewport);
    EntityResult added = rohr_object_create(project.tug); OK(added);
    tug_check(added.result.value);
    EntityResult second_body = fixture_entity_get(added.result.value, "box_1"); OK(second_body);
    EntityResult first_body = fixture_entity_get(tug, "box_1"); OK(first_body);
    EntityResult other_body = fixture_entity_get(tug, "box_2"); OK(other_body);
    assert(second_body.result.value != first_body.result.value);
    OK(rohr_entity_delete(added.result.value));
    assert(!rohr_entity_alive_check(second_body.result.value));
    tug_check(tug);

    PositionResult before_a = rohr_physics_position_get(first_body.result.value);
    PositionResult before_b = rohr_physics_position_get(other_body.result.value);
    OK(before_a); OK(before_b);
    for(unsigned tick = 0; tick < 120; ++tick) {
        assert(rohr_physics_update(1).header.status != ROHR_REPORT_FAILED);
        assert(rohr_graphics_sprite_frames_update(tick, tick / 60.0).header.status != ROHR_REPORT_FAILED);
    }
    PositionResult after_a = rohr_physics_position_get(first_body.result.value);
    PositionResult after_b = rohr_physics_position_get(other_body.result.value);
    OK(after_a); OK(after_b);
    assert(after_a.result.value.x < before_a.result.value.x);
    assert(after_b.result.value.x > before_b.result.value.x);
    assert(rohr_graphics_viewport_show(viewport.result.value).header.status != ROHR_REPORT_FAILED);
    OK(rohr_viewport_destroy(viewport.result.value));
    if(root) OK(rohr_entity_delete(root));
    else { OK(rohr_entity_delete(tug)); OK(rohr_entity_delete(example)); }
    assert(rohr_entity_alive_count_get() == 0);
    rohr_graphics_stop(); rohr_engine_stop();
}

static void failure_check(void) {
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    EntityResult added = rohr_entity_add(); OK(added);
    OK(rohr_entity_delete(added.result.value));
    assert(rohr_error_check(rohr_entity_name_get(added.result.value)));
    EntityCreateOptions options = {.owner=added.result.value};
    assert(rohr_error_check(rohr_entity_create_with_options(project.example.box, options)));
    assert(rohr_entity_alive_count_get() == 0);
    /* Fail after several successful spawns and verify tutorial rollback. */
    assert(SDL_RenamePath("assets/example_frame_1.png", "assets/example_frame_1.saved"));
    EntityResult created = rohr_object_create(project);
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
