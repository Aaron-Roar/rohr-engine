/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "project_objects.h"
#include <math.h>
#include <stdio.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "generated COM line %d: %s\n", __LINE__, #c); return false; } } while(0)
#define OK(c) CHECK(!rohr_error_check(c))
typedef struct Snapshot { Position origin, com; float angle, inertia; } Snapshot;
static bool point_equal(Position a, Position b) {
    return fabsf(a.x-b.x) < 0.002f && fabsf(a.y-b.y) < 0.002f;
}
static bool scenario(bool generated, Snapshot *snapshot) {
    Fleet fleet = {0};
    Entity body;
    OK(rohr_engine_start());
    if(generated) {
        EngineResult result = fleet_create(&fleet, (Position){0});
        if(rohr_error_check(result)) fprintf(stderr, "fleet_create: %s\n",
            rohr_error_message_get(result));
        OK(result);
        body = fleet.offset_body;
        CHECK(rohr_physics_center_of_mass_automatic_check(fleet.auto_body).result.value);
        CHECK(point_equal(rohr_physics_center_of_mass_local_position_get(fleet.auto_body).result.value,
            (Position){4,4}));
        CHECK(point_equal(rohr_physics_center_of_mass_local_position_get(fleet.empty_body).result.value,
            (Position){7,-3}));
        CHECK(!rohr_entity_components_check(fleet.empty_body, ROHR_HIT_BOX));
        CHECK(rohr_error_check(rohr_physics_moment_of_inertia_get(fleet.empty_body)));
        CHECK(rohr_error_check(rohr_physics_center_of_mass_local_position_set(fleet.pure_particle,
            (Position){1,2})));
        CHECK(point_equal(rohr_physics_center_of_mass_local_position_get(fleet.pure_particle).result.value,
            (Position){4,2}));
    } else {
        EntityResult created = rohr_entity_add(); OK(created);
        body = created.result.value;
        OK(rohr_physics_position_set(body, (Position){10,20}));
        OK(rohr_physics_orientation_set(body, -450.0f));
        OK(rohr_physics_mass_set(body, 12));
        OK(rohr_physics_hitbox_set(body, ((Shape){.amount_of_vertices=4,
            .vertices={{2,3},{6,3},{6,5},{2,5}}})));
        OK(rohr_physics_velocity_set(body, (Velocity){2,-1}));
        OK(rohr_physics_angular_velocity_set(body, 810.0f));
        OK(rohr_physics_dynamic_set(body));
        OK(rohr_physics_center_of_mass_local_position_set(body, (Position){3,-2}));
    }
    CHECK(!rohr_physics_center_of_mass_automatic_check(body).result.value);
    EntityIndex index = rohr_entity_index_get(body).result.value;
    CHECK(orientations[index] == -450.0f && angular_velocities[index] == 810.0f);
    Position initial = rohr_physics_center_of_mass_world_position_get(body).result.value;
    CHECK(point_equal(initial, (Position){12,23}));
    for(int i=0;i<100;i+=1) OK(rohr_system_physics_update(0.01));
    snapshot->origin = rohr_physics_position_get(body).result.value;
    snapshot->com = rohr_physics_center_of_mass_world_position_get(body).result.value;
    snapshot->angle = orientations[rohr_entity_index_get(body).result.value];
    snapshot->inertia = rohr_physics_moment_of_inertia_get(body).result.value;
    CHECK(point_equal(snapshot->com, (Position){initial.x+2,initial.y-1}));
    CHECK(fabsf(snapshot->angle - 360.0f) < 0.01f);
    CHECK(fabsf(snapshot->inertia-464) < 0.002f);
    if(generated) fleet_destroy(&fleet);
    rohr_engine_stop();
    return true;
}
int main(void) {
    Snapshot generated, direct;
    if(!scenario(true, &generated) || !scenario(false, &direct)) return 1;
    if(!point_equal(generated.origin, direct.origin) ||
            !point_equal(generated.com, direct.com) ||
            fabsf(generated.angle-direct.angle) > 0.002f ||
            fabsf(generated.inertia-direct.inertia) > 0.002f) return 1;
    return 0;
}
