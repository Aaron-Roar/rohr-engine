/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "drop_fixture.h"
#include "physics/physics_step_internal.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SETTLING_DEADLINE 2.0
#define SETTLING_HOLD 10.0
#define SETTLING_SPEED_LIMIT 0.0001
static volatile sig_atomic_t interrupted;
static void interrupt_test(int number) { (void)number;interrupted=1; }

typedef struct DropObservation {
    DropFixture fixture;
    uint64_t substeps;
    bool valid;
    const char *stop_reason;
    double impact;
    double maximum_linear_speed;
    double maximum_angular_speed;
    double maximum_tipping_angle;
    double settling_linear_speed;
    double settling_angular_speed;
    double settling_hold;
    unsigned settling_samples;
    double energy_ratio, correction_x, settling_support_gap, settling_support_offset;
} DropObservation;

static double vertical_extent_get(const DropBody *body) {
    double radians = body->angle * 3.14159265358979323846 / 180;
    return fabs(sin(radians)) * body->half_width + fabs(cos(radians)) * body->half_height;
}

static void settling_sample_record(DropObservation *observation, const DropBody *bodies) {
    for(unsigned i = 1; i < DROP_BODY_COUNT; ++i) {
        double linear = hypot(bodies[i].velocity.x, bodies[i].velocity.y);
        double angular = fabs(bodies[i].angular_velocity);
        observation->settling_linear_speed = fmax(observation->settling_linear_speed, linear);
        observation->settling_angular_speed = fmax(observation->settling_angular_speed, angular);
        /* Contact tolerances apply to separation as well as penetration. A
         * body floating above its support cannot pass merely by being still. */
        double gap = bodies[i].position.y - vertical_extent_get(&bodies[i]) -
            bodies[i - 1].position.y - vertical_extent_get(&bodies[i - 1]);
        observation->settling_support_gap=fmax(observation->settling_support_gap,fabs(gap));
        observation->settling_support_offset=fmax(observation->settling_support_offset,
            fabs(bodies[i].position.x-bodies[i-1].position.x));
        if(linear > SETTLING_SPEED_LIMIT || angular > SETTLING_SPEED_LIMIT ||
                fabs(gap) > DROP_PENETRATION_LIMIT ||
                fabs(bodies[i].position.x - bodies[i - 1].position.x) > bodies[i - 1].half_width)
            observation->valid = false;
    }
    ++observation->settling_samples;
}

static void completed_substep(double dt, void *context) {
    DropObservation *observation = context;
    bool was_valid=observation->valid;
    DropFixture *fixture = &observation->fixture;
    DropBody previous[DROP_BODY_COUNT];
    memcpy(previous, fixture->bodies, sizeof(previous));
    double previous_time = fixture->time;
    fixture->time += dt;
    ++observation->substeps;
    if(!drop_fixture_observe(fixture)) {
        observation->valid=false;observation->stop_reason="invalid_observation";return;
    }
    const DropBody *tiny = &fixture->bodies[1], *large = &fixture->bodies[2];
    observation->correction_x=fmax(observation->correction_x,
        fabs(tiny->position.x-previous[1].position.x)-fabs(previous[1].velocity.x)*dt);
    /* A correction may repair overlap, but must not teleport the tiny support
     * sideways. Allow integrated motion plus one full tiny-body width. */
    if(fabs(tiny->position.x - previous[1].position.x) >
            fabs(previous[1].velocity.x) * dt + 0.01)
        observation->valid = false;
    if(fixture->maximum_depth > DROP_PENETRATION_LIMIT || fixture->floor_moved)
        observation->valid = false;
    /* The centered support must not be expelled or passed through. Offset
     * cases retain unrestricted motion, subject to collision and energy. */
    if(fixture->case_index == 0 && (fabs(tiny->position.x) > 0.005 ||
            tiny->position.y < -DROP_PENETRATION_LIMIT || tiny->position.y > 0.015 ||
            (fixture->time > 14.5 &&
                fabs(large->position.y - 50.01) > 0.001))) observation->valid = false;
    double energy = 0;
    for(unsigned i = 1; i < DROP_BODY_COUNT; ++i) {
        const DropBody *body = &fixture->bodies[i];
        double m = i == 1 ? 0.0001 : 10000000;
        double inertia = m * (body->half_width * body->half_width +
            body->half_height * body->half_height) / 3;
        double omega = body->angular_velocity * 3.14159265358979323846 / 180;
        energy += 0.5 * m * ((double)body->velocity.x * body->velocity.x +
            (double)body->velocity.y * body->velocity.y) + 0.5 * inertia * omega * omega +
            m * 9.8 * body->position.y;
        if(fixture->time >= 15) {
            observation->maximum_linear_speed = fmax(observation->maximum_linear_speed,
                hypot(body->velocity.x, body->velocity.y));
            observation->maximum_angular_speed = fmax(observation->maximum_angular_speed,
                fabs(body->angular_velocity));
        }
    }
    if(!isfinite(energy) || energy > 1.001 * (10000000.0 * 9.8 * 1050.01))
        observation->valid = false;
    if(!isfinite(energy))observation->stop_reason="invalid_energy";
    observation->energy_ratio=fmax(observation->energy_ratio,energy/(10000000.0*9.8*1050.01));
    if(observation->impact == 0 && large->position.y < 50.011 &&
            fabs(large->velocity.y) < 1) observation->impact = fixture->time;
    if(observation->impact > 0 && fixture->case_index == 0) {
        double deadline = observation->impact + SETTLING_DEADLINE;
        if(fixture->time >= deadline && previous_time < deadline + SETTLING_HOLD) {
            /* Check both endpoints of the substep crossing the deadline, so
             * sampling cannot grant an extra substep to settle. Include the
             * endpoint crossing the end of the hold for the same reason. */
            if(observation->settling_samples == 0) settling_sample_record(observation, previous);
            settling_sample_record(observation, fixture->bodies);
            observation->settling_hold = fmin(SETTLING_HOLD, fixture->time - deadline);
        }
    }
    if(observation->impact > 0 && fixture->case_index >= 3)
        observation->maximum_tipping_angle = fmax(observation->maximum_tipping_angle,
            fixture->case_index == 3 ? large->angle : -large->angle);
    if(was_valid && !observation->valid) {
        fprintf(stderr, "Collision/support/energy/settling failure at completed substep %llu\n",
            (unsigned long long)observation->substeps);
        drop_fixture_print(fixture);
    }
}

static int time_compare(const void *first, const void *second) {
    double a = *(const double *)first, b = *(const double *)second;
    return (a > b) - (a < b);
}

static bool run_check(unsigned offset, unsigned rate, bool stalls, bool performance) {
    DropObservation observation = {.valid = true};
    DropFixture *fixture = &observation.fixture;
    if(!drop_fixture_reset(fixture, offset, rate)) return false;
    physics_substep_observer_set(completed_substep, &observation);
    double samples[4096], maximum_stall = 0, requested_time = 0;
    size_t count = 0;
    bool short_stall = false, long_stall = false;
    while(requested_time < 30 && !observation.stop_reason && !interrupted) {
        double dt = fmin(1.0 / rate, 30 - requested_time);
        bool delayed = false;
        if(stalls && !short_stall && requested_time >= 14) {
            dt = 0.1; short_stall = true; delayed = true;
        } else if(stalls && !long_stall && requested_time >= 14.15) {
            dt = 1; long_stall = true; delayed = true;
        }
        uint64_t before = observation.substeps;
        clock_t started = clock();
        Report report = rohr_system_physics_update(dt);
        double cpu_ms = 1000.0 * (clock() - started) / CLOCKS_PER_SEC;
        uint64_t reported = 0;
        for(size_t i = 0; i < report.statistic_count; ++i)
            if(report.statistics[i].report_id == ROHR_PHYSICS_REPORT_ID &&
                    report.statistics[i].metric_id == ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT)
                reported = report.statistics[i].value;
        requested_time += dt;
        if(report.has_fatal || report.header.status == ROHR_REPORT_FAILED)
            observation.stop_reason="engine_failure";
        else if(reported != observation.substeps-before || !reported ||
                fabs(fixture->time-requested_time)>1e-9)
            observation.stop_reason="time_or_substep_mismatch";
        if(delayed) maximum_stall = fmax(maximum_stall, cpu_ms);
        else if(count < sizeof(samples) / sizeof(samples[0])) samples[count++] = cpu_ms;
        else observation.valid = false;
    }
    physics_substep_observer_set(NULL, NULL);
    if(interrupted)observation.stop_reason="interrupted";
    bool complete=!observation.stop_reason&&fabs(fixture->time-30)<=1e-9;
    if(!complete)observation.valid=false;
    qsort(samples, count, sizeof(samples[0]), time_compare);
    double p95 = count ? samples[(count - 1) * 95 / 100] : 0;
    if(observation.impact <= 14 || observation.impact >= 14.5 ||
            (stalls && (!short_stall || !long_stall)) ||
            (performance && rate == 60 && (p95 > 2 || maximum_stall > 50))) observation.valid = false;
    if(offset == 0 && (observation.settling_samples == 0 || observation.settling_hold < SETTLING_HOLD))
        observation.valid = false;
    /* Narrow-floor comparisons must visibly rotate toward the overhang by
     * 30 seconds. This is a tipping check, not a settling criterion. */
    if(offset >= 3 && observation.maximum_tipping_angle < 1)
        observation.valid = false;
    /* A supported body remains free to leave and rotate; no fixture lock or
     * automatic freezing is permitted. This is outside timing measurements. */
    DropBody previous = fixture->bodies[2];
    if(complete) {
        if(!drop_result_check(rohr_physics_velocity_set(previous.entity, (Velocity){1, 1})) ||
                !drop_result_check(rohr_physics_angular_velocity_set(previous.entity, 1)))
            observation.valid = false;
        Report released = rohr_system_physics_update(0.001);
        if(released.has_fatal || !drop_fixture_observe(fixture) ||
                fixture->bodies[2].position.x <= previous.position.x + 0.0005f ||
                fixture->bodies[2].position.y <= previous.position.y + 0.0005f ||
                fixture->bodies[2].angle <= previous.angle + 0.0005f) observation.valid = false;
        MassResult stored_mass = rohr_physics_mass_get(previous.entity);
        if(rohr_error_check(stored_mass) || stored_mass.result.value != 10000000.0f)
            observation.valid = false;
    }
    printf("%s offset=%u hz=%u stalls=%d samples=%zu substeps=%llu time=%.9f peak_depth=%.9g impact=%.9f p95_cpu_ms=%.6f stall_cpu_ms=%.6f post15_linear=%.9g post15_angular=%.9g tipping_angle=%.9g final_angle=%.9g settling_samples=%u settling_hold=%.9f settling_linear=%.9g settling_angular=%.9g\n",
        observation.valid ? "PASS" : "FAIL", offset, rate, stalls, count,
        (unsigned long long)observation.substeps, fixture->time, fixture->maximum_depth,
        observation.impact, p95, maximum_stall, observation.maximum_linear_speed,
        observation.maximum_angular_speed, observation.maximum_tipping_angle, previous.angle,
        observation.settling_samples, observation.settling_hold,
        observation.settling_linear_speed, observation.settling_angular_speed);
    printf("worst energy_ratio=%.9g correction_x=%.9g settling_support_gap=%.9g settling_support_offset=%.9g\n",
        observation.energy_ratio,observation.correction_x,observation.settling_support_gap,observation.settling_support_offset);
    printf("completion condition=time target=30 reached=%d reason=%s max_cpu_ms=%.6f\n",
        complete,observation.stop_reason?observation.stop_reason:complete?"endpoint_reached":"endpoint_not_reached",
        fmax(maximum_stall,count?samples[count-1]:0));
    drop_fixture_destroy(fixture);
    return observation.valid;
}

int main(int argc, char **argv) {
    signal(SIGINT,interrupt_test);signal(SIGTERM,interrupt_test);
    bool performance = argc == 2 && strcmp(argv[1], "--performance") == 0;
    if(!drop_result_check(rohr_engine_start())) return 1;
    bool ok = true;
    for(unsigned offset = 0; offset < DROP_CASE_COUNT && !interrupted; ++offset) {
        const unsigned rates[] = {30, 60, 120};
        for(unsigned i = 0; i < 3 && !interrupted; ++i)
            ok = run_check(offset, rates[i], false, performance) && ok;
        if(!interrupted)ok = run_check(offset, 60, true, performance) && ok;
    }
    rohr_engine_stop();
    return ok && !interrupted ? 0 : 1;
}
