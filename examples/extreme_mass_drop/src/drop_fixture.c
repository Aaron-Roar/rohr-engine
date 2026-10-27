/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "drop_fixture.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const float offsets[DROP_CASE_COUNT] = {0.0f, 0.0025f, -0.0025f, 0.01f, -0.01f};
static const char *body_names[] = {"floor", "tiny", "large"};

bool drop_result_check(EngineResult result) {
    if(!rohr_error_check(result)) return true;
    fprintf(stderr, "Engine error: %s\n", rohr_error_message_get(result));
    return false;
}

static bool fixture_halt(DropFixture *fixture, const char *message) {
    fixture->halted = true;
    snprintf(fixture->failure, sizeof(fixture->failure), "%.*s",
        (int)sizeof(fixture->failure) - 1, message);
    fprintf(stderr, "HALTED at t=%.9f: %s\n", fixture->time, message);
    return false;
}

/* Independent double-precision SAT on the current rectangles, not cached
 * contact depth. Projection radii also handle full containment correctly.
 * Clockwise degrees in the engine's Y-up coordinates. */
double drop_rect_depth_get(const DropBody *a, const DropBody *b) {
    const double radians = 3.14159265358979323846 / 180.0;
    double ac = cos(a->angle * radians), as = sin(a->angle * radians);
    double bc = cos(b->angle * radians), bs = sin(b->angle * radians);
    double axes[4][2] = {{ac, -as}, {as, ac}, {bc, -bs}, {bs, bc}};
    double dx = (double)b->position.x - a->position.x;
    double dy = (double)b->position.y - a->position.y;
    double depth = DBL_MAX;
    for(size_t i = 0; i < 4; ++i) {
        double x = axes[i][0], y = axes[i][1];
        double ar = a->half_width * fabs(x * ac - y * as) +
            a->half_height * fabs(x * as + y * ac);
        double br = b->half_width * fabs(x * bc - y * bs) +
            b->half_height * fabs(x * bs + y * bc);
        double overlap = ar + br - fabs(dx * x + dy * y);
        if(overlap <= 0.0) return 0.0;
        depth = fmin(depth, overlap);
    }
    return depth;
}

bool drop_fixture_observe(DropFixture *fixture) {
    const EntityInfoFlags flags = ROHR_ENTITY_INFO_PHYSICS_POSITION |
        ROHR_ENTITY_INFO_PHYSICS_ORIENTATION | ROHR_ENTITY_INFO_PHYSICS_VELOCITY |
        ROHR_ENTITY_INFO_PHYSICS_ANGULAR_VELOCITY;
    DropBody observed[DROP_BODY_COUNT];
    memcpy(observed, fixture->bodies, sizeof(observed));
    for(size_t i = 0; i < DROP_BODY_COUNT; ++i) {
        EntityInfoResult result = rohr_entity_info_get(observed[i].entity, flags);
        if(rohr_error_check(result))
            return fixture_halt(fixture, "Unable to inspect body state; restart required");
        EntityInfo info = result.result.value;
        bool valid = rohr_entity_info_check(&info, flags) &&
            isfinite(info.position.x) && isfinite(info.position.y) &&
            isfinite(info.velocity.x) && isfinite(info.velocity.y) &&
            isfinite(info.orientation) && isfinite(info.angular_velocity);
        observed[i].position = info.position;
        observed[i].velocity = info.velocity;
        observed[i].angle = info.orientation;
        observed[i].angular_velocity = info.angular_velocity;
        if(!drop_result_check(rohr_entity_info_release(&info)))
            return fixture_halt(fixture, "Unable to release body observation");
        if(!valid)
            return fixture_halt(fixture, "Invalid/nonfinite body state; last finite view retained");
    }
    memcpy(fixture->bodies, observed, sizeof(observed));
    fixture->depths[0] = drop_rect_depth_get(&observed[0], &observed[1]);
    fixture->depths[1] = drop_rect_depth_get(&observed[1], &observed[2]);
    fixture->depths[2] = drop_rect_depth_get(&observed[0], &observed[2]);
    fixture->floor_moved |= observed[0].position.x != 0.0f ||
        observed[0].position.y != -0.05f || observed[0].angle != 0.0f;
    for(size_t i = 0; i < DROP_PAIR_COUNT; ++i)
        fixture->maximum_depth = fmax(fixture->maximum_depth, fixture->depths[i]);
    if(fixture->maximum_depth > DROP_PENETRATION_LIMIT && fixture->first_violation_time < 0) {
        fixture->first_violation_time = fixture->time;
        puts("OBSERVED penetration limit violation (not a demo failure or a passing physics result).");
        drop_fixture_print(fixture);
    }
    return true;
}

void drop_fixture_destroy(DropFixture *fixture) {
    for(size_t i = DROP_BODY_COUNT; i > 0; --i) {
        Entity *entity = &fixture->bodies[i - 1].entity;
        if(*entity != 0) (void)drop_result_check(rohr_entity_delete(*entity));
        *entity = 0;
    }
}

bool drop_fixture_reset(DropFixture *fixture, unsigned case_index, unsigned rate) {
    const float widths[] = {0.01f, 0.01f, 100.0f};
    const float heights[] = {0.1f, 0.01f, 100.0f};
    const float masses[] = {0.0001f, 0.0001f, 10000000.0f};
    if(case_index >= DROP_CASE_COUNT || (rate != 30 && rate != 60 && rate != 120)) return false;
    drop_fixture_destroy(fixture);
    *fixture = (DropFixture){.rate = rate, .case_index = case_index,
        .first_violation_time = -1.0};
    if(!drop_result_check(rohr_physics_gravity_set((Acceleration){0, -9.8f})) ||
            !drop_result_check(rohr_physics_substeps_set(1)) ||
            !drop_result_check(rohr_physics_solver_iterations_set(8))) goto fail;
    for(size_t i = 0; i < DROP_BODY_COUNT; ++i) {
        EntityResult created = rohr_entity_add();
        if(rohr_error_check(created)) goto fail;
        Entity e = created.result.value;
        DropBody *body = &fixture->bodies[i];
        body->entity = e;
        /* Use the actual stored float dimensions in the independent observer. */
        body->half_width = (double)widths[i] * 0.5;
        body->half_height = (double)heights[i] * 0.5;
        Position p = i == 0 ? (Position){0, -0.05f} :
            i == 1 ? (Position){0, 0.005f} : (Position){offsets[case_index], 1050.01f};
        if(!drop_result_check(rohr_physics_hitbox_set(e,
                    rohr_math_square_create(widths[i], heights[i]))) ||
                !drop_result_check(rohr_physics_position_set(e, p)) ||
                !drop_result_check(rohr_physics_orientation_set(e, 0)) ||
                !drop_result_check(rohr_physics_velocity_set(e, (Velocity){0, 0})) ||
                !drop_result_check(rohr_physics_angular_velocity_set(e, 0)) ||
                !drop_result_check(rohr_physics_mass_set(e, masses[i])) ||
                !drop_result_check(rohr_physics_friction_set(e, 0.8f)) ||
                !drop_result_check(rohr_physics_restitution_set(e, 0)) ||
                !drop_result_check(rohr_physics_collision_response_all_set(e)) ||
                !drop_result_check(rohr_entity_components_add(e, ROHR_COLLISION)) ||
                !drop_result_check(i == 0 ? rohr_physics_static_set(e) :
                    rohr_physics_dynamic_set(e)) ||
                (i != 0 && !drop_result_check(rohr_physics_gravity_enable(e)))) goto fail;
    }
    return drop_fixture_observe(fixture);
fail:
    drop_fixture_destroy(fixture);
    return fixture_halt(fixture, "Fixture creation failed");
}

bool drop_fixture_step(DropFixture *fixture) {
    if(fixture->halted) return false;
    Report report = rohr_system_physics_update(1.0 / fixture->rate);
    for(size_t i = 0; i < report.diagnostic_count; ++i)
        fprintf(stderr, "Physics diagnostic: %s\n", report.diagnostics[i].detail);
    if(report.header.status == ROHR_REPORT_FAILED || report.has_fatal)
        return fixture_halt(fixture, report.has_fatal ? report.first_fatal.detail :
            "Physics report failed; restart required");
    uint64_t completed = 0;
    for(size_t i = 0; i < report.statistic_count; ++i)
        if(report.statistics[i].report_id == ROHR_PHYSICS_REPORT_ID &&
                report.statistics[i].metric_id == ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT)
            completed += report.statistics[i].value;
    if(completed == 0)
        return fixture_halt(fixture, "Physics completed no substeps");
    ++fixture->steps;
    fixture->time = (double)fixture->steps / fixture->rate;
    if(!drop_fixture_observe(fixture)) return false;
    if(fixture->steps % fixture->rate == 0) drop_fixture_print(fixture);
    return true;
}

void drop_fixture_print(const DropFixture *fixture) {
    printf("case=%u offset=%+.4f hz=%u t=%.9f steps=%u depth=(%.9g,%.9g,%.9g) peak=%.9g floor_moved=%d floor_width=%.9g\n",
        fixture->case_index + 1, offsets[fixture->case_index], fixture->rate,
        fixture->time, fixture->steps, fixture->depths[0], fixture->depths[1],
        fixture->depths[2], fixture->maximum_depth, fixture->floor_moved,
        2 * fixture->bodies[0].half_width);
    for(size_t i = 0; i < DROP_BODY_COUNT; ++i) {
        const DropBody *b = &fixture->bodies[i];
        printf("  %s p=(%.9g,%.9g) v=(%.9g,%.9g) angle=%.9g omega=%.9g\n",
            body_names[i], b->position.x, b->position.y, b->velocity.x,
            b->velocity.y, b->angle, b->angular_velocity);
    }
    fflush(stdout);
}

static bool fixture_configuration_check(const DropFixture *fixture) {
    const EntityInfoFlags flags = ROHR_ENTITY_INFO_CORE_COMPONENTS |
        ROHR_ENTITY_INFO_PHYSICS_MASS | ROHR_ENTITY_INFO_PHYSICS_BODY_MODE |
        ROHR_ENTITY_INFO_PHYSICS_FRICTION | ROHR_ENTITY_INFO_PHYSICS_RESTITUTION |
        ROHR_ENTITY_INFO_PHYSICS_COM_LOCAL | ROHR_ENTITY_INFO_PHYSICS_INERTIA |
        ROHR_ENTITY_INFO_PHYSICS_HITBOX_LOCAL;
    const float masses[] = {0.0001f, 0.0001f, 10000000.0f};
    for(size_t i = 0; i < DROP_BODY_COUNT; ++i) {
        const DropBody *b = &fixture->bodies[i];
        EntityInfoResult result = rohr_entity_info_get(b->entity, flags);
        if(rohr_error_check(result)) return false;
        EntityInfo info = result.result.value;
        double expected_inertia = masses[i] *
            (b->half_width * b->half_width + b->half_height * b->half_height) / 3;
        bool valid = rohr_entity_info_check(&info, flags) &&
            info.mass_value == masses[i] && info.friction == 0.8f && info.restitution == 0 &&
            fabs(info.center_of_mass_local.x) <= FLT_EPSILON * b->half_width &&
            fabs(info.center_of_mass_local.y) <= FLT_EPSILON * b->half_height &&
            fabs(info.moment_of_inertia - expected_inertia) <= expected_inertia * 1e-6 &&
            info.body_mode.is_static == (i == 0) && info.body_mode.is_dynamic == (i != 0) &&
            info.body_mode.has_gravity == (i != 0) && info.body_mode.has_collision &&
            !info.body_mode.is_held && !info.body_mode.is_kinematic &&
            !(info.components & (ROHR_ANGLE_LOCK | ROHR_AXIS_LOCK | ROHR_TRANSFORM_LOCK)) &&
            info.hitbox_local.amount_of_vertices == 4;
        for(size_t v = 0; v < info.hitbox_local.amount_of_vertices && valid; ++v)
            valid = fabs(info.hitbox_local.vertices[v].x) == b->half_width &&
                fabs(info.hitbox_local.vertices[v].y) == b->half_height;
        if(!valid)
            fprintf(stderr, "Fixture configuration mismatch for %s: available=%llx requested=%llx mass=%.9g inertia=%.9g expected=%.9g COM=(%.9g,%.9g) static=%d dynamic=%d gravity=%d collision=%d vertices=%zu\n",
                body_names[i], (unsigned long long)info.available, (unsigned long long)flags,
                info.mass_value, info.moment_of_inertia, expected_inertia,
                info.center_of_mass_local.x, info.center_of_mass_local.y,
                info.body_mode.is_static, info.body_mode.is_dynamic,
                info.body_mode.has_gravity, info.body_mode.has_collision,
                (size_t)info.hitbox_local.amount_of_vertices);
        if(!drop_result_check(rohr_entity_info_release(&info)) || !valid) return false;
        for(size_t j = 0; j < DROP_BODY_COUNT; ++j)
            if(i != j && !rohr_physics_collision_response_to_check(b->entity,
                        fixture->bodies[j].entity)) return false;
    }
    return true;
}

bool drop_fixture_verify(void) {
    /* Analytic independent cases: separation, touching, shallow overlap,
     * containment, rotation, symmetry, and the extreme feature sizes. */
    DropBody a = {.half_width = 1, .half_height = 1};
    DropBody b = a;
    b.position.x = 3;
    if(drop_rect_depth_get(&a, &b) != 0) return false;
    b.position.x = 2;
    if(drop_rect_depth_get(&a, &b) != 0) return false;
    b.position.x = 1.75f;
    if(fabs(drop_rect_depth_get(&a, &b) - 0.25) > 1e-12) return false;
    b.position.x = 0;
    b.half_width = b.half_height = 0.25;
    if(fabs(drop_rect_depth_get(&a, &b) - 1.25) > 1e-12) return false;
    b = a;
    b.position.x = 2;
    b.angle = 45;
    if(fabs(drop_rect_depth_get(&a, &b) - (sqrt(2.0) - 1.0)) > 1e-12 ||
            drop_rect_depth_get(&a, &b) != drop_rect_depth_get(&b, &a)) return false;
    a = (DropBody){.half_width = 500, .half_height = 0.05,
        .position = {0, -0.05f}};
    b = (DropBody){.half_width = 0.005, .half_height = 0.005,
        .position = {0, 0.00495f}};
    if(fabs(drop_rect_depth_get(&a, &b) - 0.00005) > 1e-8) return false;

    DropFixture fixture = {0};
    const unsigned rates[] = {30, 60, 120};
    bool ok = true;
    for(unsigned c = 0; c < DROP_CASE_COUNT && ok; ++c) {
        for(size_t r = 0; r < 3 && ok; ++r) {
            ok = drop_fixture_reset(&fixture, c, rates[r]);
            if(!ok) break;
            if(!fixture_configuration_check(&fixture) || fixture.steps != 0 || fixture.time != 0 || fixture.halted ||
                    fixture.maximum_depth > 1e-8 || fixture.floor_moved ||
                    fixture.bodies[0].half_width != (double)0.01f * 0.5 ||
                    fixture.bodies[2].position.x != offsets[c] ||
                    fixture.bodies[2].position.y != 1050.01f ||
                    fixture.bodies[2].velocity.y != 0 ||
                    fixture.bodies[2].angle != 0) { ok = false; break; }
            for(unsigned step = 0; step < rates[r] * 20 && ok; ++step)
                ok = drop_fixture_step(&fixture);
            MassResult mass = rohr_physics_mass_get(fixture.bodies[0].entity);
            if(rohr_error_check(mass) || mass.result.value != 0.0001f ||
                    fixture.floor_moved || fixture.steps != rates[r] * 20) ok = false;
        }
    }
    drop_fixture_destroy(&fixture);
    puts(ok ? "PASS: observer, reset, and fifteen finite observation runs; physics acceptance NOT established." :
        "FAIL: observation harness verification");
    return ok;
}
