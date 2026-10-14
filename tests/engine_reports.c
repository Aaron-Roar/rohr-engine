/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OK(call) assert(!error_check(call))
#ifdef ROHR_TEST_ENGINE_WRAP
static Uint64 counter = 1024;
static bool fail_deletions, forbid_allocation;
static Entity failed_entity;
static EntityIndex failed_index = UINT32_MAX;
static char deletion_detail[600];
Uint64 __wrap_SDL_GetPerformanceCounter(void) { return counter; }
Uint64 __wrap_SDL_GetPerformanceFrequency(void) { return 1024; }
EngineResult __real_entity_delete(Entity);
EngineResult __wrap_entity_delete(Entity entity) {
    if(fail_deletions || entity == failed_entity)
        return error_result_error_detail(ERROR_ENGINE_STATE_INVALID, deletion_detail);
    return __real_entity_delete(entity);
}
EntityResult __real_entity_from_index_get(EntityIndex);
EntityResult __wrap_entity_from_index_get(EntityIndex index) {
    if(index == failed_index) {
        error_detail_set(ERROR_ENGINE_ENTITY_NOT_FOUND, "injected resolution failure");
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_ENTITY_NOT_FOUND);
    }
    return __real_entity_from_index_get(index);
}
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t n) { assert(!forbid_allocation); return __real_malloc(n); }
void *__wrap_calloc(size_t n, size_t s) { assert(!forbid_allocation); return __real_calloc(n, s); }
void *__wrap_realloc(void *p, size_t n) { assert(!forbid_allocation); return __real_realloc(p, n); }
#endif

static uint64_t metric(const Report *r, ReportId owner, ReportMetricId id) {
    assert(report_valid_check(r));
    for(size_t i = 0; i < r->statistic_count; ++i)
        if(r->statistics[i].report_id == owner && r->statistics[i].metric_id == id)
            return r->statistics[i].value;
    return 0;
}
static void complete(const Report *r, ReportId owner) {
    assert(report_valid_check(r) && r->header.id == owner);
    assert(r->header.status == ROHR_REPORT_COMPLETE && !r->has_fatal);
    assert(isfinite(r->header.total_ms) && r->header.total_ms >= 0);
}
static void lifecycle_counts(const Report *r, uint64_t examined, uint64_t expired,
        uint64_t deleted, uint64_t failed) {
    assert(metric(r, ROHR_ENGINE_LIFECYCLE_REPORT, ROHR_REPORT_CANDIDATE_COUNT) == examined);
    assert(metric(r, ROHR_ENGINE_LIFECYCLE_REPORT, ROHR_REPORT_EXPIRED_ENTITY_COUNT) == expired);
    assert(metric(r, ROHR_ENGINE_LIFECYCLE_REPORT, ROHR_REPORT_PROCESSED_COUNT) == deleted);
    assert(metric(r, ROHR_ENGINE_LIFECYCLE_REPORT, ROHR_REPORT_SKIPPED_COUNT) == failed);
}
static Entity entity(Time time, Tick tick) {
    EntityResult added = rohr_entity_add(); OK(added);
    OK(rohr_entity_life_time_set(added.result.value, time, tick));
    return added.result.value;
}
static void unavailable(void) {
    Report r = rohr_engine_update();
    assert(report_valid_check(&r) && r.header.id == ROHR_ENGINE_REPORT);
    assert(r.header.status == ROHR_REPORT_FAILED && r.has_fatal && r.entry_count == 0);
    assert(r.first_fatal.code == ERROR_ENGINE_STATE_INVALID);
    r = engine_lifecycle_update();
    assert(report_valid_check(&r) && r.header.id == ROHR_ENGINE_LIFECYCLE_REPORT);
    assert(r.header.status == ROHR_REPORT_FAILED && r.has_fatal && r.entry_count == 0);
    assert(rohr_engine_tick_update() == 0 && engine_tick_delta_get() == 0);
    engine_time_update(); /* No uninitialized division. */
}

static void independent_cleanup(void) {
    OK(rohr_engine_start()); rohr_engine_pause();
    Entity expired = entity(-1, 0), future = entity(0, 100), permanent = entity(0, 0);
    Time before = rohr_engine_time_get(); Tick tick = rohr_engine_tick_get();
    Report r = rohr_engine_lifecycle_update(); complete(&r, ROHR_ENGINE_LIFECYCLE_REPORT);
    assert(r.entry_count == 0 && r.diagnostic_count == 0);
    lifecycle_counts(&r, 3, 1, 1, 0);
    assert(!rohr_entity_alive_check(expired));
    assert(rohr_entity_alive_check(future) && rohr_entity_alive_check(permanent));
    assert(rohr_engine_time_get() == before && rohr_engine_tick_get() == tick);
    r = rohr_engine_update(); complete(&r, ROHR_ENGINE_REPORT);
    assert(r.entry_count == 1 && r.entries[0].header.id == ROHR_ENGINE_LIFECYCLE_REPORT);
    assert(r.entries[0].invocation_count == 1);
    assert(metric(&r, ROHR_ENGINE_REPORT, ROHR_REPORT_TICKS_ADVANCED_COUNT) == 0);
    lifecycle_counts(&r, 2, 0, 0, 0);
    Report saved = r, snapshot = r;
    rohr_engine_stop();
    assert(memcmp(&saved, &snapshot, sizeof(saved)) == 0);
    unavailable();
}

#ifdef ROHR_TEST_ENGINE_WRAP
static Report clock_path(bool combined) {
    OK(engine_start()); OK(engine_time_per_tick_set(0.25));
    Entity by_tick = entity(0, 2), by_time = entity(0.5, 0);
    Entity future = entity(0, 3), permanent = entity(0, 0);
    counter += 640; /* 0.625 seconds: two ticks, half a tick left. */
    Report r;
    forbid_allocation = true;
    if(combined) r = rohr_engine_update();
    else {
        assert(engine_tick_update() == 2);
        assert(rohr_entity_alive_check(by_tick) && rohr_entity_alive_check(by_time));
        r = engine_lifecycle_update();
    }
    forbid_allocation = false;
    complete(&r, combined ? ROHR_ENGINE_REPORT : ROHR_ENGINE_LIFECYCLE_REPORT);
    assert(engine_tick_get() == 2 && rohr_engine_tick_get() == 2);
    assert(engine_tick_delta_get() == 2 && rohr_engine_tick_delta_get() == 2);
    assert(engine_time_get() == 0.625);
    assert(!entity_alive_check(by_tick) && !entity_alive_check(by_time));
    assert(entity_alive_check(future) && entity_alive_check(permanent));
    lifecycle_counts(&r, 4, 2, 2, 0);
    Report again = rohr_engine_lifecycle_update(); lifecycle_counts(&again, 2, 0, 0, 0);
    assert(engine_tick_delta_get() == 2);
    counter += 128;
    engine_time_update(); /* Accumulate without consuming ticks or clearing delta. */
    assert(engine_tick_get() == 2 && engine_tick_delta_get() == 2);
    assert(entity_alive_check(future));
    assert(rohr_engine_tick_update() == 1 && engine_tick_delta_get() == 1);
    assert(entity_alive_check(future));
    again = rohr_engine_lifecycle_update(); lifecycle_counts(&again, 2, 1, 1, 0);
    assert(!entity_alive_check(future));
    assert(engine_tick_update() == 0 && engine_tick_delta_get() == 0);
    counter += 256;
    assert(engine_tick_update() == 1 && engine_tick_delta_get() == 1);
    engine_clock_reset(); assert(engine_tick_delta_get() == 0 && engine_tick_get() == 4);
    counter += 256; assert(engine_tick_update() == 1);
    engine_pause(); counter += 2048;
    assert(engine_tick_update() == 0 && engine_tick_delta_get() == 0);
    Entity while_paused = entity(-1, 0);
    again = engine_update(); assert(!entity_alive_check(while_paused));
    assert(engine_tick_get() == 5 && engine_time_get() == 1.25);
    counter += 1024; engine_resume();
    assert(engine_tick_update() == 0); /* No paused-time catch-up. */
    counter += 256; assert(engine_tick_update() == 1);
    engine_stop(); assert(engine_tick_delta_get() == 0);
    return r;
}

static void failures(void) {
    OK(rohr_engine_start()); rohr_engine_pause();
    failed_entity = entity(-1, 0);
    Entity resolution = entity(-1, 0), healthy = entity(-1, 0);
    failed_index = rohr_entity_index_get(resolution).result.value;
    snprintf(deletion_detail, sizeof(deletion_detail), "injected relationship detach failure");
    Report r = engine_update(), saved = r, snapshot = r;
    complete(&r, ROHR_ENGINE_REPORT); lifecycle_counts(&r, 3, 3, 1, 2);
    assert(r.entry_count == 1 && r.entries[0].invocation_count == 1);
    assert(r.diagnostic_count == 2 && r.dropped_diagnostic_occurrences == 0);
    assert(entity_alive_check(failed_entity) && entity_alive_check(resolution));
    assert(!entity_alive_check(healthy));
    bool found_source = false, found_slot = false;
    for(size_t i = 0; i < r.diagnostic_count; ++i) {
        ReportDiagnostic *d = &r.diagnostics[i];
        assert(d->report_id == ROHR_ENGINE_LIFECYCLE_REPORT);
        assert(d->severity == ROHR_REPORT_SEVERITY_WARNING && d->occurrence_count == 1);
        assert(d->recovery == ROHR_REPORT_CONTRIBUTION_SKIPPED);
        if(d->source.id == failed_entity) {
            assert(strstr(d->detail, "injected relationship detach failure")); found_source = true;
        } else { assert(strstr(d->detail, "Expired entity slot")); found_slot = true; }
    }
    assert(found_source && found_slot);
    failed_entity = ENTITY_INVALID; failed_index = UINT32_MAX;
    memset(deletion_detail, 'x', sizeof(deletion_detail) - 1);
    r = engine_lifecycle_update(); lifecycle_counts(&r, 2, 2, 2, 0);
    assert(memcmp(&saved, &snapshot, sizeof(saved)) == 0);
    for(size_t i = 0; i < ROHR_REPORT_DIAGNOSTIC_CAPACITY + 4; ++i) (void)entity(-1, 0);
    fail_deletions = true;
    r = rohr_engine_update(); complete(&r, ROHR_ENGINE_REPORT);
    lifecycle_counts(&r, ROHR_REPORT_DIAGNOSTIC_CAPACITY + 4,
        ROHR_REPORT_DIAGNOSTIC_CAPACITY + 4, 0, ROHR_REPORT_DIAGNOSTIC_CAPACITY + 4);
    assert(r.diagnostic_count == ROHR_REPORT_DIAGNOSTIC_CAPACITY);
    assert(r.dropped_diagnostic_occurrences == 4 && r.diagnostics[0].detail_truncated);
    Report full = r;
    fail_deletions = false;
    r = engine_lifecycle_update(); lifecycle_counts(&r, ROHR_REPORT_DIAGNOSTIC_CAPACITY + 4,
        ROHR_REPORT_DIAGNOSTIC_CAPACITY + 4, ROHR_REPORT_DIAGNOSTIC_CAPACITY + 4, 0);
    engine_stop();
    Report fatal = engine_update();
    OK(report_merge(&full, &fatal));
    assert(full.has_fatal && full.header.status == ROHR_REPORT_FAILED);
    assert(full.dropped_diagnostic_occurrences == 4);
    assert(strstr(full.first_fatal.detail, "requires engine startup"));
    OK(report_merge(&full, &r)); assert(full.header.status == ROHR_REPORT_FAILED);
    assert(memcmp(&saved, &snapshot, sizeof(saved)) == 0);
}
#endif

static void formatting(void) {
    OK(engine_start()); rohr_engine_pause();
    (void)entity(-1, 0);
    Report r = engine_update();
    OK(rohr_report_text_write(&r, "engine_report.txt"));
    OK(rohr_report_json_write(&r, "engine_report.json", true));
    const char *paths[] = {"engine_report.txt", "engine_report.json"};
    for(size_t i = 0; i < 2; ++i) {
        char buffer[12000] = {0}; FILE *f = fopen(paths[i], "rb"); assert(f);
        assert(fread(buffer, 1, sizeof(buffer) - 1, f) > 0); fclose(f);
        assert(strstr(buffer, "ENGINE_LIFECYCLE") && strstr(buffer, "TICKS_ADVANCED_COUNT"));
        assert(strstr(buffer, "EXPIRED_ENTITY_COUNT")); assert(remove(paths[i]) == 0);
    }
    OK(report_statistic_add(&r, (ReportStatistic){ROHR_ENGINE_REPORT,
        ROHR_REPORT_TICKS_ADVANCED_COUNT, ROHR_REPORT_AGGREGATION_SUM, UINT64_MAX}));
    OK(report_statistic_add(&r, (ReportStatistic){ROHR_ENGINE_REPORT,
        ROHR_REPORT_TICKS_ADVANCED_COUNT, ROHR_REPORT_AGGREGATION_SUM, 1}));
    assert(r.counters_saturated);
    engine_stop();
}

int main(void) {
    unavailable(); independent_cleanup();
#ifdef ROHR_TEST_ENGINE_WRAP
    Report combined = clock_path(true), separate = clock_path(false);
    assert(combined.entry_count == 1 && separate.entry_count == 0);
    assert(combined.statistic_count == separate.statistic_count + 1);
    for(size_t i = 0; i < separate.statistic_count; ++i)
        assert(metric(&combined, separate.statistics[i].report_id,
            separate.statistics[i].metric_id) == separate.statistics[i].value);
    failures();
#endif
    formatting();
    return 0;
}
