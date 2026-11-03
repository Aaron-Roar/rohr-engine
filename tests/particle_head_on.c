/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>

#define PARTICLE_RADIUS 2.5f
#define PARTICLE_SPEED 1000.0f
#define PARTICLE_PENETRATION_LIMIT 0.5
#define RUN_SECONDS 2.5

static volatile sig_atomic_t interrupted;
static void interrupt_test(int number) { (void)number; interrupted = 1; }

typedef struct HeadOnFixture {
    Entity entities[2];
    EntityIndex indices[2];
    double time, maximum_depth, maximum_energy_ratio;
    uint64_t substeps;
    bool valid, contact, crossed;
    const char *stop_reason;
} HeadOnFixture;

static bool result_check(EngineResult result) {
    if(!rohr_error_check(result)) return true;
    fprintf(stderr, "%s\n", rohr_error_message_get(result));
    return false;
}

static void observe(double dt, void *context) {
    HeadOnFixture *f = context;
    f->time += dt;
    ++f->substeps;
    double energy = 0;
    for(unsigned i = 0; i < 2; ++i) {
        EntityIndex n = f->indices[i];
        Position p = positions[n];
        Velocity v = velocities[n];
        if(!isfinite(p.x) || !isfinite(p.y) || !isfinite(v.x) || !isfinite(v.y) ||
                !isfinite(orientations[n]) || !isfinite(angular_velocities[n])) {
            f->valid = false;
            f->stop_reason = "invalid_state";
            return;
        }
        energy += (double)v.x*v.x + (double)v.y*v.y;
        if(fabs(p.y) > .0001 || fabs(v.y) > .0001 ||
                fabs(orientations[n]) > .0001 || fabs(angular_velocities[n]) > .0001)
            f->valid = false;
    }
    Position a = positions[f->indices[0]], b = positions[f->indices[1]];
    /* Independent circle geometry at every completed substep. Checking order
     * also catches a complete pass-through that endpoint overlap alone misses. */
    double depth = fmax(0, 2.0*PARTICLE_RADIUS - hypot((double)b.x-a.x, (double)b.y-a.y));
    f->maximum_depth = fmax(f->maximum_depth, depth);
    f->maximum_energy_ratio = fmax(f->maximum_energy_ratio,
        energy/(2.0*PARTICLE_SPEED*PARTICLE_SPEED));
    f->crossed |= a.x >= b.x;
    f->contact |= rohr_physics_contact_check(f->entities[0], f->entities[1]);
    if(depth > PARTICLE_PENETRATION_LIMIT || f->crossed || f->maximum_energy_ratio > 1.001)
        f->valid = false;
}

static bool run_check(unsigned hz, unsigned delays) {
    HeadOnFixture f = {.valid = true};
    bool started = result_check(rohr_engine_start());
    if(!started) {
        puts("FAIL particle_head_on completion condition=time target=2.5 reached=0 reason=engine_start_failed");
        return false;
    }
    if(!result_check(rohr_physics_gravity_set((Acceleration){0})) ||
            !result_check(rohr_physics_substeps_set(1)) ||
            !result_check(rohr_physics_solver_iterations_set(8))) f.stop_reason = "setup_failed";
    for(unsigned i = 0; i < 2 && !f.stop_reason; ++i) {
        ParticleDescription d = rohr_physics_particle_description_default_get();
        d.radius = PARTICLE_RADIUS; d.rigid_vertices = 16; d.mass_value = 1;
        d.friction = .5f; d.restitution = .5f; d.gravity_enabled = false;
        /* Each travels 250 units before the nominal impact at 0.25 s. */
        d.position = (Position){(i ? 1 : -1)*(PARTICLE_RADIUS + PARTICLE_SPEED*.25f), 0};
        d.velocity = (Velocity){i ? -PARTICLE_SPEED : PARTICLE_SPEED, 0};
        EntityResult entity = rohr_physics_particle_add(d);
        if(rohr_error_check(entity)) {
            fprintf(stderr, "%s\n", rohr_error_message_get(entity));
            f.stop_reason = "setup_failed";
            break;
        }
        f.entities[i] = entity.result.value;
        EntityIndexResult index = rohr_entity_index_get(f.entities[i]);
        if(rohr_error_check(index)) {
            fprintf(stderr, "%s\n", rohr_error_message_get(index));
            f.stop_reason = "setup_failed";
            break;
        }
        f.indices[i] = index.result.value;
    }
    if(!f.stop_reason) physics_substep_observer_set(observe, &f);
    double requested = 0;
    bool first = false, second = false;
    while(requested < RUN_SECONDS-1e-12 && !f.stop_reason && !interrupted) {
        double dt = fmin(1.0/hz, RUN_SECONDS-requested);
        if(delays && !first && requested >= .2-1e-12) {
            dt = delays == 1 ? .1 : 1; first = true;
        } else if(delays && !second && requested >= 1.3-1e-12) {
            dt = delays == 1 ? 1 : .1; second = true;
        } else if(delays) {
            if(!first && requested < .2) dt = fmin(dt, .2-requested);
            if(!second && requested < 1.3) dt = fmin(dt, 1.3-requested);
        }
        dt = fmin(dt, RUN_SECONDS-requested);
        uint64_t before = f.substeps, reported = 0;
        Report report = rohr_system_physics_update(dt);
        requested += dt;
        for(size_t i = 0; i < report.statistic_count; ++i)
            if(report.statistics[i].report_id == ROHR_PHYSICS_REPORT_ID &&
                    report.statistics[i].metric_id == ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT)
                reported = report.statistics[i].value;
        if(report.diagnostic_count) f.valid = false;
        if(report.has_fatal || report.header.status == ROHR_REPORT_FAILED) f.stop_reason = "engine_failure";
        else if(!reported || reported != f.substeps-before || fabs(f.time-requested) > 1e-9)
            f.stop_reason = "time_or_substep_mismatch";
    }
    physics_substep_observer_set(NULL, NULL);
    if(interrupted) f.stop_reason = "interrupted";
    bool complete = !f.stop_reason && fabs(f.time-RUN_SECONDS) <= 1e-9;
    double rebound_error = 0;
    if(complete) for(unsigned i = 0; i < 2; ++i)
        rebound_error = fmax(rebound_error, fabs(velocities[f.indices[i]].x - (i ? 500 : -500)));
    f.valid &= complete && f.contact && rebound_error <= 5 && (!delays || (first && second));
    printf("%s particle_head_on update_hz=%u delays=%u speed_each=1000 diameter=5 restitution=0.5 "
        "penetration_limit=%.9g time=%.9f substeps=%llu depth=%.9g contact=%d crossed=%d rebound_error=%.9g energy_ratio=%.9g\n",
        f.valid ? "PASS" : "FAIL", hz, delays, PARTICLE_PENETRATION_LIMIT, f.time,
        (unsigned long long)f.substeps, f.maximum_depth, f.contact, f.crossed, rebound_error, f.maximum_energy_ratio);
    printf("completion condition=time target=2.5 reached=%d reason=%s measurements=%s\n", complete,
        f.stop_reason ? f.stop_reason : complete ? "endpoint_reached" : "endpoint_not_reached",
        complete ? "full_run" : "partial_run");
    fflush(stdout);
    rohr_engine_stop();
    return f.valid;
}

int main(void) {
    signal(SIGINT, interrupt_test); signal(SIGTERM, interrupt_test);
    const unsigned rates[] = {30, 60, 120, 60, 60};
    bool valid = true;
    for(unsigned i = 0; i < 5 && !interrupted; ++i)
        valid = run_check(rates[i], i < 3 ? 0 : i-2) && valid;
    return valid && !interrupted ? 0 : 1;
}
