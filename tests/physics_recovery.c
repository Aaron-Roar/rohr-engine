/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"

#include <math.h>
#include "physics/physics_step_internal.h"
#include <assert.h>
#include <float.h>
#include <string.h>

static ForceDescription force_vector_description(Entity target, Force vector) {
    return (ForceDescription){.target = target,
        .location.reference = {.kind = ROHR_FORCE_COM_POSITION, .entity = target},
        .orientation.offset = vector.x == 0 && vector.y == 0 ? 0 :
            math_radians_to_degrees(atan2f(vector.x, vector.y)),
        .magnitude = hypotf(vector.x, vector.y)};
}


#define OK(v) assert(!rohr_error_check(v))
#define BAD(v) assert(rohr_error_check(v))

static Entity entity(void) {
    EntityResult r = rohr_entity_add(); OK(r); return r.result.value;
}
static EntityIndex index_of(Entity e) {
    EntityIndexResult r = rohr_entity_index_get(e); OK(r); return r.result.value;
}
static EntityInfo inspect(Entity e, EntityInfoFlags flags) {
    EntityInfoResult r = rohr_entity_info_get(e, flags); OK(r);
    assert(rohr_entity_info_check(&r.result.value, flags));
    return r.result.value;
}
static void no_issues(const EntityInfo *info) {
    EntityInfoIssueCountResult r = entity_info_issues_count_get(info, info->requested); OK(r);
    assert(r.result.value == 0);
}
static void stored_equal(const Report *report, ReportFlags flags) {
    ReportResult r = rohr_report_get(flags); OK(r);
    assert(memcmp(report, &r.result.value, sizeof(*report)) == 0);
}
static Entity body(float x, float y, bool collider) {
    Entity e = entity();
    OK(rohr_physics_position_set(e, (Position){x, y}));
    OK(rohr_physics_orientation_set(e, 0));
    OK(rohr_physics_mass_set(e, 1));
    OK(rohr_physics_dynamic_set(e));
    if(collider) {
        OK(rohr_physics_hitbox_set(e, rohr_math_square_create(2, 2)));
        OK(rohr_entity_components_add(e, ROHR_COLLISION));
    }
    return e;
}
static const ReportDiagnostic *warning(const Report *r, PhysicsReportStage stage, Entity source) {
    assert(r->header.status == ROHR_REPORT_COMPLETE && !r->has_fatal);
    assert(rohr_report_valid_check(r));
    for(size_t n = 0; n < r->diagnostic_count; ++n) {
        const ReportDiagnostic *d = &r->diagnostics[n];
        if(d->internal_stage == (uint32_t)stage && d->source.id == source) {
            assert(d->severity == ROHR_REPORT_SEVERITY_WARNING);
            assert(d->recovery == ROHR_REPORT_CONTRIBUTION_SKIPPED);
            return d;
        }
    }
    assert(!"missing operation rejection");
    return NULL;
}

static void movement_retry(void) {
    OK(rohr_engine_start());
    Entity a = body(ROHR_WORLD_COORDINATE_MAX, 0, true);
    Entity b = body(0, 0, false);
    EntityIndex i = index_of(a), j = index_of(b);
    OK(rohr_physics_velocity_set(a, (Velocity){16, 0}));
    OK(rohr_physics_velocity_set(b, (Velocity){4, 0}));
    OK(rohr_physics_substeps_set(4));
    EntityInfoFlags fields = ROHR_ENTITY_INFO_PHYSICS_POSITION | ROHR_ENTITY_INFO_PHYSICS_VELOCITY |
        ROHR_ENTITY_INFO_PHYSICS_BODY_MODE | ROHR_ENTITY_INFO_PHYSICS_HITBOX_LOCAL;
    EntityInfo before = inspect(a, fields); no_issues(&before);
    Report r = rohr_physics_dt_update(1);
    const ReportDiagnostic *d = warning(&r, ROHR_PHYSICS_POSITION, a);
    assert(d->occurrence_count == 4 && d->first_substep == 0 && d->last_substep == 3);
    assert(positions[i].x == ROHR_WORLD_COORDINATE_MAX && velocities[i].x == 16);
    assert(positions[j].x == 4 && rohr_entity_components_check(a, ROHR_HIT_BOX));
    assert(!rohr_entity_components_check(a, ROHR_HOLD));
    EntityInfo rejected = inspect(a, fields); no_issues(&rejected);
    assert(rejected.position.x == before.position.x && rejected.velocity.x == before.velocity.x);
    assert(rejected.body_mode.is_dynamic && !rejected.body_mode.is_held);
    assert(memcmp(&rejected.hitbox_local, &before.hitbox_local, sizeof(Shape)) == 0);
    stored_equal(&r, ROHR_PHYSICS_REPORT); /* Inspection neither executes nor replaces evidence. */
    Report saved = r;
    ReportResult stored = rohr_report_get(ROHR_PHYSICS_REPORT); OK(stored);
    assert(stored.result.value.version == r.version);
    r = rohr_physics_dt_update(1);
    assert(warning(&r, ROHR_PHYSICS_POSITION, a)->occurrence_count == 4);
    OK(rohr_physics_velocity_set(a, (Velocity){-16, 0}));
    r = rohr_physics_dt_update(1);
    assert(r.diagnostic_count == 0 && positions[i].x == ROHR_WORLD_COORDINATE_MAX - 16);
    EntityInfo recovered = inspect(a, fields); no_issues(&recovered);
    assert(recovered.position.x == positions[i].x && recovered.velocity.x == -16);
    assert(rejected.position.x == ROHR_WORLD_COORDINATE_MAX && rejected.velocity.x == 16);
    assert(positions[j].x == 12); /* Independent work completed on every attempt. */
    assert(saved.diagnostics[0].occurrence_count == 4);
    rohr_engine_stop();
    OK(rohr_engine_start());
    assert(before.position.x == rejected.position.x && saved.diagnostics[0].source.id == a);
    OK(entity_info_release(&before)); OK(entity_info_release(&rejected)); OK(entity_info_release(&recovered));
    rohr_engine_stop();
}

static void integration_and_locks(void) {
    OK(rohr_engine_start());
    Entity a = body(0, 0, false), b = body(10, 0, false);
    OK(rohr_physics_orientation_set(b, 0));
    EntityIndex i = index_of(a);
    OK(rohr_physics_velocity_set(a, (Velocity){1, 0}));
    OK(rohr_physics_acceleration_set(a, (Acceleration){FLT_MAX, 0}));
    Report r = rohr_physics_pipeline_integrate(2);
    warning(&r, ROHR_PHYSICS_VELOCITY, a);
    assert(velocities[i].x == 1 && positions[i].x == 2); /* Old valid velocity still usable. */
    OK(rohr_physics_acceleration_set(a, (Acceleration){0}));
    OK(rohr_physics_position_set(a, (Position){ROHR_WORLD_COORDINATE_MAX, 0}));
    OK(rohr_physics_velocity_set(a, (Velocity){0}));
    OK(rohr_physics_center_of_mass_local_position_set(a, (Position){10, 0}));
    OK(rohr_physics_angular_velocity_set(a, 180));
    r = rohr_physics_pipeline_integrate(1);
    warning(&r, ROHR_PHYSICS_ORIENTATION, a);
    assert(orientations[i] == 0 && positions[i].x == ROHR_WORLD_COORDINATE_MAX);
    assert(angular_velocities[i] == 180);
    OK(rohr_physics_angular_velocity_set(a, 0));
    OK(rohr_physics_transform_lock_set(a, b, (Vec2D){FLT_MAX, FLT_MAX}, 30, true, true, true));
    r = rohr_physics_pipeline_integrate(0);
    warning(&r, ROHR_PHYSICS_TRANSFORM_LOCK, a);
    assert(positions[i].x == ROHR_WORLD_COORDINATE_MAX && orientations[i] == 0);
    assert(velocities[i].x == 0);
    transform_locks[i].driver = UINT32_MAX; /* Invalid existing reference. */
    r = rohr_physics_pipeline_integrate(0);
    warning(&r, ROHR_PHYSICS_TRANSFORM_LOCK, a);
    assert(rohr_entity_components_check(a, ROHR_TRANSFORM_LOCK));
    EntityInfo broken = inspect(a, ROHR_ENTITY_INFO_PHYSICS_TRANSFORM_LOCK);
    EntityInfoIssue issue;
    EntityInfoIssueReadResult issues = entity_info_issues_get(&broken, broken.requested, &issue, 1); OK(issues);
    assert(issues.result.value.total == 1 && issue.reason == ROHR_ENTITY_INFO_REFERENCE_MISSING);
    assert(issue.reference.id == UINT32_MAX && issue.field == ROHR_ENTITY_INFO_FIELD_TARGET);
    stored_equal(&r, ROHR_PHYSICS_INTEGRATION_REPORT);
    OK(rohr_physics_transform_lock_set(a, b, (Vec2D){0}, 0, true, false, false));
    r = rohr_physics_pipeline_integrate(0);
    assert(r.diagnostic_count == 0 && positions[i].x == 10);
    EntityInfo repaired = inspect(a, ROHR_ENTITY_INFO_PHYSICS_TRANSFORM_LOCK); no_issues(&repaired);
    assert(repaired.transform_lock.driver == b && broken.transform_lock.driver == UINT32_MAX);
    rohr_engine_stop();
    OK(entity_info_release(&broken)); OK(entity_info_release(&repaired));
}

static void setters(void) {
    OK(rohr_engine_start());
    Entity a = body(0, 0, false), b = body(2, 0, false);
    EntityIndex i = index_of(a);
    RohrComponentMask mask = entity_mask[i];
    uint32_t count = entity_alive_count_get();
    BAD(rohr_physics_velocity_set(a, (Velocity){NAN, 0}));
    BAD(rohr_physics_acceleration_set(a, (Acceleration){0, INFINITY}));
    BAD(rohr_physics_force_description_set(a, (ForceDescription){.magnitude = INFINITY}));
    BAD(rohr_physics_torque_description_set(a, (TorqueDescription){.magnitude = NAN}));
    BAD(rohr_physics_force_add(force_vector_description(a, (Force){NAN, 0})));
    BAD(rohr_physics_torque_add((TorqueDescription){.target = a, .magnitude = INFINITY}));
    BAD(rohr_physics_impulse_apply(a, (Vec2D){NAN, 0}));
    BAD(rohr_physics_axis_lock_set(a, (Axis){NAN, 1}, (Position){0}));
    BAD(rohr_physics_transform_lock_set(a, b, (Vec2D){INFINITY, 0}, 0, true, true, true));
    BAD(rohr_physics_joint_anchor_create(a, (Vec2D){NAN, 0}));
    assert(entity_alive_count_get() == count && entity_mask[i] == mask);
    assert(velocities[i].x == 0 && accelerations[i].y == 0);
    OK(rohr_physics_velocity_set(a, (Velocity){FLT_MAX, 0}));
    BAD(rohr_physics_impulse_apply(a, (Vec2D){FLT_MAX, 0}));
    assert(velocities[i].x == FLT_MAX);
    rohr_engine_stop();
}

static void joints_retry(void) {
    OK(rohr_engine_start());
    Entity a = body(0, 0, false), b = body(10, 0, false), joint = entity();
    JointAnchorIdResult aa = rohr_physics_joint_anchor_create(a, (Vec2D){0}); OK(aa);
    JointAnchorIdResult bb = rohr_physics_joint_anchor_create(b, (Vec2D){0}); OK(bb);
    OK(rohr_physics_joint_pin_set(joint, aa.result.value, bb.result.value));
    EntityIndex k = index_of(joint);
    joints[k].anchor_a = UINT64_MAX;
    rohr_physics_pipeline_substep_begin();
    (void)rohr_physics_pipeline_joints_gather();
    Report r = rohr_physics_pipeline_constraints_solve(3);
    assert(warning(&r, ROHR_PHYSICS_JOINT_TARGET, joint)->occurrence_count == 3);
    assert(entity_alive_check(joint) && joints[k].anchor_a == UINT64_MAX);
    OK(rohr_physics_joint_pin_set(joint, aa.result.value, bb.result.value));
    r = rohr_physics_pipeline_constraints_solve(3);
    assert(r.diagnostic_count == 0 && positions[index_of(a)].x == positions[index_of(b)].x);
    OK(rohr_entity_delete(a));
    assert(entity_alive_check(joint) && joints[k].a == ENTITY_INVALID);
    rohr_engine_stop();
}

static void contacts_and_geometry(void) {
    OK(rohr_engine_start());
    Entity a = body(0, 0, true), b = body(1, 0, true);
    EntityIndex i = index_of(a), j = index_of(b);
    OK(rohr_physics_velocity_set(a, (Velocity){1, 1}));
    OK(rohr_physics_velocity_set(b, (Velocity){-1, 0}));
    OK(rohr_physics_friction_set(a, .5f));
    frictions[i] = NAN; /* Inject invalid existing material; normal response remains valid. */
    physics_pipeline_substep_begin();
    Report r = physics_pipeline_contacts_gather();
    assert(!r.has_fatal);
    r = physics_pipeline_constraints_solve(1);
    warning(&r, ROHR_PHYSICS_FRICTION_IMPULSE, a);
    assert(velocities[i].x < 1 && velocities[j].x > -1);
    ContactInfo contact = physics_contact_get(a, b);
    for(uint8_t n = 0; n < contact.point_count; ++n)
        assert(contact.points[n].friction_impulse.x == 0 && contact.points[n].friction_impulse.y == 0);
    EntityInfoResult captured = entity_info_get(a, ROHR_ENTITY_INFO_PHYSICS_FRICTION | ROHR_ENTITY_INFO_PHYSICS_CONTACTS); OK(captured);
    EntityInfo rejected = captured.result.value;
    assert(!entity_info_check(&rejected, ROHR_ENTITY_INFO_PHYSICS_FRICTION) && rejected.friction == 0);
    assert(entity_info_check(&rejected, ROHR_ENTITY_INFO_PHYSICS_CONTACTS) && rejected.contact_count == 1);
    assert(rejected.contacts[0].target == b && rejected.contacts[0].contact.detected);
    EntityInfoIssue issue;
    EntityInfoIssueReadResult issues = entity_info_issues_get(&rejected, rejected.requested, &issue, 1); OK(issues);
    assert(issues.result.value.total == 1 && issue.section == ROHR_ENTITY_INFO_PHYSICS_FRICTION);
    assert(issue.reason == ROHR_ENTITY_INFO_VALUE_INVALID && issue.reference.id == a);
    for(uint8_t n = 0; n < rejected.contacts[0].contact.point_count; ++n)
        assert(rejected.contacts[0].contact.points[n].friction_impulse.x == 0 &&
            rejected.contacts[0].contact.points[n].friction_impulse.y == 0);
    stored_equal(&r, ROHR_PHYSICS_CONSTRAINTS_REPORT);
    OK(rohr_physics_friction_set(a, .5f));
    orientations[i] = NAN;
    r = physics_pipeline_contacts_gather();
    warning(&r, ROHR_PHYSICS_GEOMETRY, a);
    assert(world_hit_boxes[i].amount_of_vertices == 0 && hit_boxes[i].amount_of_vertices != 0);
    assert(entity_components_check(a, ROHR_HIT_BOX));
    OK(rohr_physics_orientation_set(a, 0));
    r = physics_pipeline_contacts_gather();
    assert(r.diagnostic_count == 0 && world_hit_boxes[i].amount_of_vertices != 0);
    rohr_engine_stop();
    assert(!entity_info_check(&rejected, ROHR_ENTITY_INFO_PHYSICS_FRICTION) && rejected.contacts[0].target == b);
    OK(entity_info_release(&rejected));
}

static void soft_body_local_failure(void) {
    OK(rohr_engine_start());
    EntityResult created = rohr_physics_soft_body_add((SoftBodyDescription){0}); OK(created);
    Entity soft = created.result.value;
    Entity nodes[3];
    for(unsigned n = 0; n < 3; ++n) {
        created = rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
            .owner = soft,
            .position = (Position){n * 10.0f, 0},
            .mass = 1,
            .radius = 1
        }); OK(created);
        nodes[n] = created.result.value;
    }
    EntityResult first = rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
        .owner = soft,
        .node_a = nodes[0],
        .node_b = nodes[1],
        .stiffness = 1,
        .damping = 0
    }); OK(first);
    EntityResult second = rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
        .owner = soft,
        .node_a = nodes[1],
        .node_b = nodes[2],
        .stiffness = 1,
        .damping = 0
    }); OK(second);
    EntityIndex i = index_of(nodes[0]), j = index_of(nodes[1]), k = index_of(nodes[2]);
    soft_body_beams[index_of(first.result.value)].stiffness = FLT_MAX;
    soft_body_beams[index_of(first.result.value)].rest_length = 0;
    soft_body_beams[index_of(second.result.value)].rest_length = 0;
    physics_pipeline_accelerations_clear();
    Report r = physics_pipeline_forces_apply();
    warning(&r, ROHR_PHYSICS_BEAM_CONTRIBUTION, first.result.value);
    assert(force_accelerations[i].x == 0 && force_accelerations[j].x == 10 && force_accelerations[k].x == -10);
    r = physics_pipeline_integrate(.1);
    assert(r.diagnostic_count == 0 && velocities[j].x == 1 && velocities[k].x == -1);
    assert(velocities[i].x == 0 && entity_alive_check(first.result.value));
    soft_body_beams[index_of(first.result.value)].stiffness = 1;
    physics_pipeline_accelerations_clear();
    r = physics_pipeline_forces_apply();
    assert(r.diagnostic_count == 0 && force_accelerations[i].x > 0);
    OK(rohr_physics_velocity_set(nodes[0], (Velocity){FLT_MAX, FLT_MAX}));
    Position before = positions[j];
    Velocity previous = velocities[j];
    BAD(rohr_physics_orientation_set(soft, 45));
    assert(orientations[index_of(soft)] == 0);
    assert(positions[j].x == before.x && positions[j].y == before.y);
    assert(velocities[j].x == previous.x && velocities[j].y == previous.y);
    rohr_engine_stop();
}

static void joint_corrections(void) {
    OK(rohr_engine_start());
    Entity a = body(ROHR_WORLD_COORDINATE_MAX, 0, false);
    Entity b = body(ROHR_WORLD_COORDINATE_MAX - 1, 0, false), joint = entity();
    EntityIndex i = index_of(a), j = index_of(b);
    JointAnchorIdResult aa = rohr_physics_joint_anchor_create(a, (Vec2D){-ROHR_WORLD_COORDINATE_MAX, 0}); OK(aa);
    JointAnchorIdResult bb = rohr_physics_joint_anchor_create(b, (Vec2D){ROHR_WORLD_COORDINATE_MAX, 0}); OK(bb);
    OK(rohr_physics_joint_pin_set(joint, aa.result.value, bb.result.value));
    OK(rohr_physics_velocity_set(a, (Velocity){FLT_MAX, 0}));
    OK(rohr_physics_velocity_set(b, (Velocity){-FLT_MAX, 0}));
    physics_pipeline_substep_begin();
    (void)physics_pipeline_joints_gather();
    Report r = physics_pipeline_constraints_solve(1);
    warning(&r, ROHR_PHYSICS_JOINT_POSITION, joint);
    warning(&r, ROHR_PHYSICS_JOINT_VELOCITY, joint);
    assert(positions[i].x == ROHR_WORLD_COORDINATE_MAX && positions[j].x == ROHR_WORLD_COORDINATE_MAX - 1);
    assert(velocities[i].x == FLT_MAX && velocities[j].x == -FLT_MAX);
    rohr_engine_stop();
}

static void rejected_loads_and_limits(void) {
    OK(rohr_engine_start());
    Entity a = body(0, 0, true), b = body(10, 0, false);
    EntityIndex i = index_of(a);
    OK(rohr_physics_mass_set(a, .001f));
    EntityResult load = rohr_physics_force_add(force_vector_description(a, (Force){FLT_MAX, 0})); OK(load);
    Report r = physics_pipeline_integrate(0);
    warning(&r, ROHR_PHYSICS_FORCE_CONTRIBUTION, load.result.value);
    assert(force_accelerations[i].x == 0 && velocities[i].x == 0);
    OK(rohr_entity_delete(load.result.value));
    load = rohr_physics_torque_add((TorqueDescription){.target = a, .magnitude = FLT_MAX}); OK(load);
    r = physics_pipeline_integrate(0);
    warning(&r, ROHR_PHYSICS_TORQUE_CONTRIBUTION, load.result.value);
    assert(torque_angular_accelerations[i] == 0 && angular_velocities[i] == 0);
    OK(rohr_entity_delete(load.result.value));
    OK(rohr_physics_angular_acceleration_set(a, FLT_MAX));
    r = physics_pipeline_integrate(2);
    warning(&r, ROHR_PHYSICS_ANGULAR_VELOCITY, a);
    assert(angular_velocities[i] == 0 && orientations[i] == 0);
    OK(rohr_physics_angular_acceleration_set(a, 0));
    OK(rohr_physics_position_set(a, (Position){ROHR_WORLD_COORDINATE_MAX, 0}));
    OK(rohr_physics_center_of_mass_local_position_set(a, (Position){10, 0}));
    OK(rohr_physics_angle_lock_set(a, 180, 180));
    r = physics_pipeline_integrate(0);
    warning(&r, ROHR_PHYSICS_ANGLE_LOCK, a);
    assert(orientations[i] == 0 && positions[i].x == ROHR_WORLD_COORDINATE_MAX);
    OK(rohr_entity_components_delete(a, ROHR_ANGLE_LOCK));
    OK(rohr_physics_axis_lock_set(a, (Axis){0, 1}, (Position){0}));
    axis_locks[i].point_on_axis = (Position){NAN, 0};
    r = physics_pipeline_integrate(0);
    warning(&r, ROHR_PHYSICS_AXIS_LOCK, a);
    assert(positions[i].x == ROHR_WORLD_COORDINATE_MAX);
    Entity spring = entity();
    Joint joint = {.type = JOINT_SPRING, .a = a, .b = b, .stiffness = FLT_MAX};
    OK(rohr_physics_joint_component_set(spring, joint));
    physics_pipeline_accelerations_clear();
    r = physics_pipeline_forces_apply();
    warning(&r, ROHR_PHYSICS_SPRING_CONTRIBUTION, spring);
    assert(force_accelerations[i].x == 0 && force_accelerations[index_of(b)].x == 0);
    rohr_engine_stop();
}

static void paired_contact_rejection(void) {
    OK(rohr_engine_start());
    Entity a = body(ROHR_WORLD_COORDINATE_MAX - 1, 0, true);
    Entity b = body(ROHR_WORLD_COORDINATE_MAX, 0, true);
    EntityIndex i = index_of(a), j = index_of(b);
    Position prior_a = positions[i], prior_b = positions[j];
    physics_pipeline_substep_begin();
    (void)physics_pipeline_contacts_gather();
    Report r = physics_pipeline_constraints_solve(2);
    warning(&r, ROHR_PHYSICS_POSITION_CORRECTION, a);
    assert(positions[i].x == prior_a.x && positions[j].x == prior_b.x);
    /* The valid overlap is still published despite rejected separation. */
    assert(rohr_physics_contact_check(a, b));
    rohr_engine_stop();

    OK(rohr_engine_start());
    EntityResult soft = rohr_physics_soft_body_add((SoftBodyDescription){0}); OK(soft);
    EntityResult n1 = rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .owner = soft.result.value,
        .position = (Position){-10, 0},
        .mass = 1,
        .radius = 1
    }); OK(n1);
    EntityResult n2 = rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .owner = soft.result.value,
        .position = (Position){10, 0},
        .mass = 1,
        .radius = 1
    }); OK(n2);
    EntityResult beam = rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){
        .owner = soft.result.value,
        .node_a = n1.result.value,
        .node_b = n2.result.value,
        .stiffness = 0,
        .damping = 0
    }); OK(beam);
    OK(rohr_physics_soft_body_beam_collision_thickness_set(beam.result.value, 2));
    ParticleConfig config = rohr_physics_particle_config_default_get();
    config.position = (Position){0, 1.5f}; config.velocity = (Velocity){0, -FLT_MAX};
    EntityResult particle = rohr_physics_particle_create(config); OK(particle);
    OK(rohr_physics_velocity_set(n1.result.value, (Velocity){0, FLT_MAX}));
    OK(rohr_physics_velocity_set(n2.result.value, (Velocity){0, FLT_MAX}));
    physics_pipeline_substep_begin();
    (void)physics_pipeline_contacts_gather();
    r = physics_pipeline_constraints_solve(2);
    warning(&r, ROHR_PHYSICS_NORMAL_IMPULSE, beam.result.value);
    assert(velocities[index_of(n1.result.value)].y == FLT_MAX);
    assert(velocities[index_of(n2.result.value)].y == FLT_MAX);
    assert(velocities[index_of(particle.result.value)].y == -FLT_MAX);
    ContactInfo contact = rohr_physics_contact_get(beam.result.value, particle.result.value);
    assert(contact.point_count == 1 && contact.points[0].normal_impulse.y == 0);
    rohr_engine_stop();
}

int main(void) {
    movement_retry();
    integration_and_locks();
    setters();
    joints_retry();
    contacts_and_geometry();
    soft_body_local_failure();
    rejected_loads_and_limits();
    paired_contact_rejection();
    joint_corrections();
    return 0;
}
