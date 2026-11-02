/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 * Diagnostic link wrappers. No simulation operation is skipped or replaced.
 */
#include "rohr.h"
#include "physics/physics_step_internal.h"
#include "physics/soft_body/soft_body_frame.h"
#include "profile_clock.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    M_outer_update, M_observer_validation,
#define VOID(fn, stage, args, call) M_##fn,
#define VALUE(type, fn, stage, args, call) M_##fn,
#include "profile_functions.inc"
#undef VOID
#undef VALUE
    M_COUNT
};
static const char *const names[] = {"outer_update", "observer_validation",
#define VOID(fn, stage, args, call) #fn,
#define VALUE(type, fn, stage, args, call) #fn,
#include "profile_functions.inc"
#undef VOID
#undef VALUE
};
static const char *const stages[] = {"orchestration", "observer_validation",
#define VOID(fn, stage, args, call) #stage,
#define VALUE(type, fn, stage, args, call) #stage,
#include "profile_functions.inc"
#undef VOID
#undef VALUE
};
static MeasurementClock timer;
static MeasurementMetric metrics[M_COUNT];
static bool active, enabled, initialized;
static FILE *trace;
static uint64_t updates, total_ns, substeps, candidates, contacts, attempted;
static uint64_t diagnostics, incomplete_updates;
static uint64_t lost_report_updates, stage_candidates[M_COUNT], stage_contacts[M_COUNT];
static double simulated, update_simulated;
static uint64_t observed, update_observed;
static void (*caller_observer)(double, void *);
static void *caller_context;
void __real_physics_substep_observer_set(void (*observer)(double, void *), void *context);
static void observe(double dt, void *context) {
    (void)context;
    if(active) { update_simulated += dt; ++update_observed; }
    if(caller_observer) {
        if(active && enabled) measurement_enter(&timer, M_observer_validation, SDL_GetTicksNS());
        caller_observer(dt, caller_context);
        if(active && enabled) measurement_leave(&timer, metrics, SDL_GetTicksNS());
    }
}
void __wrap_physics_substep_observer_set(void (*observer)(double, void *), void *context) {
    caller_observer = observer; caller_context = context;
    __real_physics_substep_observer_set(active ? observe : observer, active ? NULL : context);
}

#define VOID(fn, stage, args, call) \
void __real_##fn args; \
void __wrap_##fn args { \
    bool measured = active && enabled; \
    if(measured) measurement_enter(&timer, M_##fn, SDL_GetTicksNS()); \
    __real_##fn call; \
    if(measured) measurement_leave(&timer, metrics, SDL_GetTicksNS()); \
}
#define VALUE(type, fn, stage, args, call) \
type __real_##fn args; \
type __wrap_##fn args { \
    bool measured = active && enabled; \
    if(measured) measurement_enter(&timer, M_##fn, SDL_GetTicksNS()); \
    type result = __real_##fn call; \
    if(measured) measurement_leave(&timer, metrics, SDL_GetTicksNS()); \
    return result; \
}
#include "profile_functions.inc"
#undef VOID
#undef VALUE

/* Count work at its source, not when a child report is merged into a parent. */
void __real_physics_report_counter_add(Report *, ReportMetricId, uint64_t);
void __wrap_physics_report_counter_add(Report *report, ReportMetricId metric, uint64_t value) {
    if(active && enabled && report && timer.depth && !timer.invalid) {
        unsigned owner = timer.stack[timer.depth-1].metric;
        if(metric == ROHR_REPORT_CANDIDATE_COUNT) stage_candidates[owner] += value;
        if(metric == ROHR_REPORT_CONTACT_COUNT) stage_contacts[owner] += value;
    }
    __real_physics_report_counter_add(report, metric, value);
}

static void finish(void) {
    uint64_t exclusive = 0, measured_candidates = 0, measured_contacts = 0;
    for(unsigned i = 0; i < M_COUNT; ++i) {
        exclusive += metrics[i].exclusive_ns;
        measured_candidates += stage_candidates[i]; measured_contacts += stage_contacts[i];
        printf("measurement_stage name=%s stage=%s calls=%llu inclusive_ns=%llu exclusive_ns=%llu candidates=%llu contacts=%llu\n",
            names[i], stages[i], (unsigned long long)metrics[i].calls,
            (unsigned long long)metrics[i].inclusive_ns, (unsigned long long)metrics[i].exclusive_ns,
            (unsigned long long)stage_candidates[i], (unsigned long long)stage_contacts[i]);
    }
    printf("measurement_total enabled=%d updates=%llu wall_ns=%llu exclusive_ns=%llu accounting_valid=%d "
        "attempted=%llu completed=%llu candidates=%llu contacts=%llu diagnostics=%llu incomplete_updates=%llu simulated=%.12f observed=%llu "
        "lost_report_updates=%llu measured_candidates=%llu measured_contacts=%llu\n",
        enabled, (unsigned long long)updates, (unsigned long long)total_ns,
        (unsigned long long)exclusive, !timer.invalid && !timer.depth && (!enabled || exclusive == total_ns),
        (unsigned long long)attempted, (unsigned long long)substeps,
        (unsigned long long)candidates, (unsigned long long)contacts,
        (unsigned long long)diagnostics, (unsigned long long)incomplete_updates,
        simulated, (unsigned long long)observed, (unsigned long long)lost_report_updates,
        (unsigned long long)measured_candidates, (unsigned long long)measured_contacts);
    if(trace && fclose(trace)) { perror("measurement trace close"); fflush(stdout); _Exit(EXIT_FAILURE); }
}
static void begin(void) {
    if(!initialized) {
        const char *option = getenv("ROHR_MEASUREMENT_PROFILE");
        enabled = !option || strcmp(option, "0") != 0;
        const char *path = getenv("ROHR_MEASUREMENT_TRACE");
        if(path && *path) {
            trace = fopen(path, "w");
            if(!trace) { perror("measurement trace"); exit(EXIT_FAILURE); }
            setvbuf(trace, NULL, _IOLBF, 0);
            fputs("update,wall_ns,state_hash,finite,attempted,completed,candidates,contacts,diagnostics,fatal,simulated,observed,report_loss\n", trace);
        }
        atexit(finish); initialized = true;
    }
    active = true;
    update_simulated = 0; update_observed = 0;
    __real_physics_substep_observer_set(observe, NULL);
}
static void hash_bytes(uint64_t *hash, const void *data, size_t size) {
    const unsigned char *bytes = data;
    for(size_t i = 0; i < size; ++i) { *hash ^= bytes[i]; *hash *= UINT64_C(1099511628211); }
}
/* Outside the physics timer. Scalar fields exclude padding and pointer values.
 * This is a same-platform trajectory checksum, not a collision-accuracy oracle. */
static uint64_t state_hash_get(bool *finite) {
    uint64_t hash = UINT64_C(14695981039346656037);
    for(uint32_t i = 0; i < entity_alive_count_get(); ++i) {
        EntityResult entity = entity_alive_at_get(i);
        if(entity.kind == ERROR_RESULT_ERROR) { *finite = false; continue; }
        EntityIndex n;
        if(!entity_index_get(entity.result.value, &n)) { *finite = false; continue; }
        hash_bytes(&hash, &entity.result.value, sizeof(entity.result.value));
        hash_bytes(&hash, &entity_mask[n], sizeof(entity_mask[n]));
#define SCALAR(pool, table, field) \
        if(n < pool.capacity && pool.used[n]) { \
            double value = (double)(table[n] field); \
            *finite &= isfinite(value); hash_bytes(&hash, &value, sizeof(value)); \
        }
        SCALAR(positions_pool, positions, .x) SCALAR(positions_pool, positions, .y)
        SCALAR(velocities_pool, velocities, .x) SCALAR(velocities_pool, velocities, .y)
        SCALAR(accelerations_pool, accelerations, .x) SCALAR(accelerations_pool, accelerations, .y)
        SCALAR(orientations_pool, orientations, ) SCALAR(angular_velocities_pool, angular_velocities, )
#undef SCALAR
    }
    return hash;
}
static void record(const Report *report, uint64_t elapsed) {
    active = false; ++updates; total_ns += elapsed;
    __real_physics_substep_observer_set(caller_observer, caller_context);
    simulated += update_simulated; observed += update_observed;
    uint64_t a = 0, s = 0, c = 0, k = 0;
    for(size_t i = 0; i < report->statistic_count; ++i) {
        ReportStatistic metric = report->statistics[i];
        if(metric.metric_id == ROHR_REPORT_SUBSTEPS_ATTEMPTED_COUNT) a += metric.value;
        if(metric.metric_id == ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT) s += metric.value;
        if(metric.metric_id == ROHR_REPORT_CANDIDATE_COUNT) c += metric.value;
        if(metric.metric_id == ROHR_REPORT_CONTACT_COUNT) k += metric.value;
    }
    bool finite = true;
    uint64_t hash = state_hash_get(&finite);
    attempted += a; substeps += s; candidates += c; contacts += k;
    diagnostics += report->diagnostic_count;
    bool loss = report->dropped_entry_count || report->dropped_statistic_count ||
        report->dropped_diagnostic_occurrences || report->counters_saturated;
    lost_report_updates += loss;
    incomplete_updates += report->has_fatal || report->header.status == ROHR_REPORT_FAILED || !finite || a != s || s != update_observed;
    if(trace) {
        fprintf(trace, "%llu,%llu,%016llx,%d,%llu,%llu,%llu,%llu,%zu,%d,%.12f,%llu,%d\n",
            (unsigned long long)updates, (unsigned long long)elapsed, (unsigned long long)hash, finite,
            (unsigned long long)a, (unsigned long long)s, (unsigned long long)c,
            (unsigned long long)k, report->diagnostic_count, report->has_fatal,
            update_simulated, (unsigned long long)update_observed, loss);
        if(ferror(trace)) { perror("measurement trace write"); exit(EXIT_FAILURE); }
    }
}
#define ROOT(fn, type) \
Report __real_##fn(type value); \
Report __wrap_##fn(type value) { \
    if(active) return __real_##fn(value); \
    begin(); uint64_t started = SDL_GetTicksNS(); \
    if(enabled) measurement_enter(&timer, M_outer_update, started); \
    Report report = __real_##fn(value); \
    uint64_t ended = SDL_GetTicksNS(); \
    if(enabled) measurement_leave(&timer, metrics, ended); \
    record(&report, ended - started); return report; \
}
ROOT(physics_update, Tick)
ROOT(physics_dt_update, Time)
ROOT(system_physics_update, double)
ROOT(physics_pipeline_update, double)
ROOT(physics_pipeline_substep, double)
