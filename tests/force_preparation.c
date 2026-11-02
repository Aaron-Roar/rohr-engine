/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <assert.h>
#include <math.h>

#define OK(value) assert(!rohr_error_check(value))
static Entity entity_make(void) {
    EntityResult result = rohr_entity_add(); OK(result); return result.result.value;
}
static EntityIndex index_get(Entity entity) {
    EntityIndexResult result = rohr_entity_index_get(entity); OK(result); return result.result.value;
}
static void report_check(Report report) {
    assert(!report.has_fatal && report.diagnostic_count == 0);
}
int main(void) {
    OK(rohr_engine_start());
    Entity owner = entity_make(), a = entity_make(), b = entity_make();
    /* Generic component toggles are public: an empty joint definition still
     * reports unavailable endpoints rather than silently disappearing. */
    OK(rohr_entity_components_add(owner, ROHR_JOINT));
    Report incomplete = rohr_physics_pipeline_forces_apply();
    assert(!incomplete.has_fatal && incomplete.diagnostic_count == 1);
    assert(incomplete.diagnostics[0].source.id == owner);
    OK(rohr_entity_components_delete(owner, ROHR_JOINT));
    Entity bodies[] = {a, b};
    for(unsigned n = 0; n < 2; ++n) {
        OK(rohr_physics_hitbox_set(bodies[n], rohr_math_square_create(2, 2)));
        OK(rohr_physics_mass_set(bodies[n], 2));
        OK(rohr_physics_dynamic_set(bodies[n]));
        OK(rohr_physics_position_set(bodies[n], (Position){4 * n, 0}));
        OK(rohr_physics_orientation_set(bodies[n], 0));
        OK(rohr_physics_velocity_set(bodies[n], (Velocity){0}));
        OK(rohr_physics_gravity_enable(bodies[n]));
    }
    EntityIndex ai = index_get(a), bi = index_get(b);
    /* The spring lies above holes in the entity table. A pool scan must retain
     * live joints at high indices and observe creation/deletion between calls. */
    Entity holes[300];
    for(unsigned n = 0; n < 300; ++n) holes[n] = entity_make();
    JointAnchorIdResult aa = rohr_physics_joint_anchor_add(
        (JointAnchorDescription){.entity = a, .local_offset = {0, 1}}); OK(aa);
    JointAnchorIdResult bb = rohr_physics_joint_anchor_add(
        (JointAnchorDescription){.entity = b, .local_offset = {0, 1}}); OK(bb);
    JointDescription spring = {.type = JOINT_SPRING, .anchor_a = aa.result.value,
        .anchor_b = bb.result.value, .rest_length = 2, .rest_length_explicit = true,
        .stiffness = 6};
    EntityResult joint = rohr_physics_joint_add(spring); OK(joint);
    for(unsigned n = 0; n < 300; ++n) OK(rohr_entity_delete(holes[n]));
    ForceDescription force = {.target = a, .magnitude = 2, .orientation.offset = 90,
        .location.reference = {.kind = ROHR_FORCE_COM_POSITION, .entity = a}};
    force.owner = owner;
    EntityResult source = rohr_physics_force_add(force); OK(source);
    OK(rohr_physics_acceleration_set(a, (Acceleration){0, 2}));
    OK(rohr_physics_gravity_set((Acceleration){0, -3}));

    rohr_physics_pipeline_accelerations_clear();
    report_check(rohr_physics_pipeline_forces_apply());
    assert(force_accelerations[ai].x == 6 && force_accelerations[bi].x == -6);
    assert(force_accelerations[ai].y == 0 && accelerations[ai].y == 2);
    assert(torque_angular_accelerations[ai] > 0 && torque_angular_accelerations[bi] < 0);
    /* Manual force preparation is additive; it neither clears nor applies the
     * explicit source. External loads remain owned by integration. */
    report_check(rohr_physics_pipeline_forces_apply());
    assert(force_accelerations[ai].x == 12 && force_accelerations[bi].x == -12);
    rohr_physics_pipeline_accelerations_clear();
    assert(force_accelerations[ai].x == 0 && torque_angular_accelerations[ai] == 0);
    assert(accelerations[ai].y == 2);
    rohr_physics_pipeline_gravity_apply();
    report_check(rohr_physics_pipeline_forces_apply());
    report_check(rohr_physics_pipeline_integrate(.01));
    assert(fabsf(velocities[ai].x - .07f) < .000001f);
    assert(fabsf(velocities[bi].x + .06f) < .000001f);
    assert(fabsf(velocities[ai].y + .01f) < .000001f);
    assert(fabsf(velocities[bi].y + .03f) < .000001f);

    OK(rohr_entity_delete(joint.result.value));
    Entity reused = entity_make();
    rohr_physics_pipeline_accelerations_clear();
    report_check(rohr_physics_pipeline_forces_apply());
    assert(force_accelerations[ai].x == 0 && force_accelerations[bi].x == 0);
    assert(torque_angular_accelerations[ai] == 0);
    OK(rohr_physics_joint_description_set(reused, spring));
    report_check(rohr_physics_pipeline_forces_apply());
    assert(force_accelerations[ai].x > 0 && force_accelerations[bi].x < 0);
    OK(rohr_entity_components_delete(reused, ROHR_JOINT));
    rohr_physics_pipeline_accelerations_clear();
    report_check(rohr_physics_pipeline_forces_apply());
    assert(force_accelerations[ai].x == 0 && force_accelerations[bi].x == 0);

    OK(rohr_entity_delete(owner));
    assert(!rohr_entity_alive_check(source.result.value));
    assert(rohr_entity_alive_check(a) && rohr_entity_alive_check(b));
    report_check(rohr_physics_pipeline_integrate(0));
    assert(force_accelerations[ai].x == 0);
    rohr_engine_stop();
    return 0;
}
