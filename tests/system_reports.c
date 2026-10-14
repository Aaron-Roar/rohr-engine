/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define OK(call) assert(!rohr_error_check(call))
#define SYSTEM_COUNT 5
/* Budget retained reports explicitly rather than placing arrays on the stack. */
static Report retained[SYSTEM_COUNT], snapshots[SYSTEM_COUNT], direct[SYSTEM_COUNT];
static Report combined, shutdown_audio;

static uint64_t metric(const Report *r, ReportMetricId id) {
    for(size_t i = 0; i < r->statistic_count; ++i)
        if(r->statistics[i].metric_id == id) return r->statistics[i].value;
    return 0;
}

/* Same checked consumption pattern as the usage guide. Output failure is
 * independent of the system outcome; callers receive both results. */
static EngineResult inspect(const Report *r, bool *failed) {
    *failed = r->header.status == ROHR_REPORT_FAILED;
    if(*failed || r->diagnostic_count || r->dropped_entry_count ||
            r->dropped_statistic_count || r->dropped_diagnostic_occurrences ||
            r->counters_saturated)
        return rohr_report_text_write(r, "system_report.txt");
    return ERROR_RESULT_MAKE_VALUE(EngineResult, true);
}

static void fixture(bool wrapped, Report *reports) {
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    rohr_engine_pause(); OK(rohr_physics_substeps_set(1));
    (void)ui_update(); (void)graphics_pipeline_commands_execute();
    SDL_Event key = {.type = SDL_EVENT_KEY_DOWN};
    key.key.key = SDLK_LEFT;
    rohr_ui_event_add(&key);
    rohr_ui_translation_y_pop(); /* Recoverable unmatched scope. */
    for(unsigned i = 0; i < 2; ++i)
        rohr_graphics_screen_texture_draw((TextureAsset){.handle = 1000},
            (Position){0}, (Scale){1, 1}, 0);

    /* Updating other owners must leave pending UI/graphics evidence intact. */
    reports[0] = wrapped ? rohr_engine_update() : engine_update();
    reports[1] = wrapped ? rohr_physics_dt_update(.01) : physics_dt_update(.01);
    reports[4] = wrapped ? rohr_audio_update() : audio_update();
    engine_clock_reset();
    reports[3] = wrapped ? rohr_graphics_pipeline_commands_execute() :
        graphics_pipeline_commands_execute();
    reports[2] = wrapped ? rohr_ui_update() : ui_update();
    assert(reports[0].entry_count == 1);
    assert(reports[0].entries[0].header.id == ROHR_ENGINE_LIFECYCLE_REPORT);
    assert(reports[1].entry_count == 6);
    assert(reports[2].diagnostic_count == 1);
    assert(reports[2].diagnostics[0].code == ERROR_ENGINE_UI_SCOPE_UNBALANCED);
    assert(metric(&reports[2], ROHR_REPORT_UI_EVENT_RECEIVED_COUNT) == 1);
    assert(reports[3].diagnostic_count == 1);
    assert(reports[3].diagnostics[0].report_id == ROHR_GRAPHICS_COMMANDS_REPORT);
    assert(reports[3].diagnostics[0].occurrence_count == 2);
    assert(reports[4].header.status == ROHR_REPORT_NOT_RUN);
    for(size_t i = 0; i < SYSTEM_COUNT; ++i) {
        assert(rohr_report_valid_check(&reports[i]));
        if(i != 4) assert(reports[i].header.status == ROHR_REPORT_COMPLETE);
        assert(!reports[i].has_fatal);
    }
    assert(ui_update().diagnostic_count == 0);
    assert(graphics_pipeline_commands_execute().diagnostic_count == 0);
    /* Shutdown evidence belongs to audio even while other systems execute. */
    OK(audio_start());
    shutdown_audio = audio_stop();
    (void)engine_update(); (void)physics_dt_update(.01); (void)ui_update();
    (void)graphics_pipeline_commands_execute();
    Report delivery = audio_update();
    assert(memcmp(&delivery, &shutdown_audio, sizeof(delivery)) == 0);
    assert(audio_update().header.status == ROHR_REPORT_NOT_RUN);
    rohr_graphics_stop(); rohr_engine_stop();
}

static bool measured_metric(ReportMetricId id) {
    return id == ROHR_REPORT_BROADPHASE_BUILD_NS || id == ROHR_REPORT_BROADPHASE_QUERY_NS ||
        id == ROHR_REPORT_NARROWPHASE_NS || id == ROHR_REPORT_FRAME_LIMIT_WAIT_NS;
}

static void parity(const Report *a, const Report *b) {
    assert(a->header.id == b->header.id && a->header.status == b->header.status);
    assert(a->entry_count == b->entry_count && a->statistic_count == b->statistic_count);
    assert(a->diagnostic_count == b->diagnostic_count && a->has_fatal == b->has_fatal);
    for(size_t i = 0; i < a->entry_count; ++i) {
        assert(a->entries[i].header.id == b->entries[i].header.id);
        assert(a->entries[i].header.status == b->entries[i].header.status);
        assert(a->entries[i].invocation_count == b->entries[i].invocation_count);
    }
    for(size_t i = 0; i < a->statistic_count; ++i) {
        const ReportStatistic *x = &a->statistics[i], *y = &b->statistics[i];
        assert(x->report_id == y->report_id && x->metric_id == y->metric_id);
        assert(x->aggregation == y->aggregation);
        if(!measured_metric(x->metric_id)) assert(x->value == y->value);
    }
    for(size_t i = 0; i < a->diagnostic_count; ++i) {
        const ReportDiagnostic *x = &a->diagnostics[i], *y = &b->diagnostics[i];
        assert(x->report_id == y->report_id && x->code == y->code);
        assert(x->severity == y->severity && x->recovery == y->recovery);
        assert(x->occurrence_count == y->occurrence_count && strcmp(x->detail, y->detail) == 0);
    }
}

int main(void) {
    fixture(true, retained);
    memcpy(snapshots, retained, sizeof(snapshots));
    fixture(false, direct);
    assert(memcmp(snapshots, retained, sizeof(snapshots)) == 0);
    for(size_t i = 0; i < SYSTEM_COUNT; ++i) parity(&retained[i], &direct[i]);

    combined = rohr_report_create();
    OK(rohr_report_header_set(&combined,
        (ReportHeader){ROHR_REPORT_CUSTOM_ID_BEGIN, ROHR_REPORT_COMPLETE, 0}));
    size_t entries = 0;
    for(size_t i = 0; i < SYSTEM_COUNT; ++i) {
        OK(rohr_report_merge(&combined, &retained[i]));
        entries += retained[i].entry_count + (retained[i].header.status != ROHR_REPORT_NOT_RUN);
    }
    assert(combined.entry_count == entries && combined.diagnostic_count == 2);
    for(size_t i = 0; i < combined.entry_count; ++i)
        assert(combined.entries[i].invocation_count == 1);
    assert(combined.header.total_ms == 0 && combined.header.status == ROHR_REPORT_COMPLETE);
    bool failed;
    OK(inspect(&combined, &failed)); assert(!failed); /* COMPLETE with warnings. */

    Report fatal = rohr_engine_update(); /* No engine: fatal with copied details. */
    assert(fatal.has_fatal && fatal.header.status == ROHR_REPORT_FAILED);
    OK(rohr_report_merge(&combined, &fatal));
    OK(rohr_report_merge(&combined, &retained[2])); /* Later success cannot erase it. */
    assert(combined.has_fatal && strcmp(combined.first_fatal.detail, fatal.first_fatal.detail) == 0);
    OK(inspect(&combined, &failed)); assert(failed);
    assert(memcmp(snapshots, retained, sizeof(snapshots)) == 0);
    assert(remove("system_report.txt") == 0);
    return 0;
}
