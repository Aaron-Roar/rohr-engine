/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <assert.h>
#include <string.h>
static unsigned initialized, cleaned, updated;
static bool fail_initialize, fail_update;
static Entity entity;
static EngineResult initialize(void *context) {
    assert(context == &entity); ++initialized;
    EntityResult created = entity_add(); assert(!error_check(created)); entity = created.result.value;
    return fail_initialize ? error_result_error_detail(ERROR_ENGINE_STATE_INVALID, "scene init fixture") : error_result_value(true);
}
static void shutdown_scene(void *context) {
    assert(context == &entity); ++cleaned;
    assert(!error_check(entity_delete(entity)));
}
static Report update(void *context, Tick ticks, Time time) {
    (void)context; (void)ticks; assert(time >= 0); ++updated;
    Report report = report_create(); report.header = (ReportHeader){ROHR_REPORT_CUSTOM_ID_BEGIN, ROHR_REPORT_COMPLETE, 0};
    if(fail_update) {
        ReportDiagnostic diagnostic = {.report_id = ROHR_REPORT_CUSTOM_ID_BEGIN,
            .code = ERROR_ENGINE_STATE_INVALID, .severity = ROHR_REPORT_SEVERITY_FATAL,
            .recovery = ROHR_REPORT_UPDATE_ABORTED, .occurrence_count = 1};
        assert(!error_check(report_diagnostic_detail_set(&diagnostic, "scene update fixture")));
        assert(!error_check(report_diagnostic_add(&report, diagnostic)));
    } else {
        SDL_Event event; SDL_zero(event); event.type = SDL_EVENT_QUIT; assert(SDL_PushEvent(&event));
    }
    return report;
}
int main(void) {
    char *argv[] = {"scene_runner_test", NULL};
    SceneCallbacks callbacks = {&entity, initialize, update, shutdown_scene};
    Report invalid = rohr_scene_run(1, argv, (SceneCallbacks){0});
    assert(invalid.header.status == ROHR_REPORT_FAILED && !initialized && !cleaned);
    char *bad_argv[] = {"scene", "--rohr-scene-host", NULL, NULL};
    assert(scene_run(4, bad_argv, callbacks).header.status == ROHR_REPORT_FAILED);
    assert(!error_check(engine_start()));
    assert(scene_run(1, argv, callbacks).header.status == ROHR_REPORT_FAILED);
    assert(engine_update().header.status == ROHR_REPORT_COMPLETE); engine_stop();
    fail_initialize = true;
    Report failed = scene_run(1, argv, callbacks);
    assert(failed.header.status == ROHR_REPORT_FAILED && initialized == 1 && cleaned == 1 && updated == 0);
    assert(strstr(failed.first_fatal.detail, "scene init fixture"));
    fail_initialize = false; fail_update = true;
    Report gameplay = rohr_scene_run(1, argv, callbacks);
    assert(gameplay.header.status == ROHR_REPORT_FAILED && initialized == 2 && cleaned == 2 && updated == 1);
    assert(strstr(gameplay.first_fatal.detail, "scene update fixture"));
    fail_update = false;
    Report success = scene_run(1, argv, callbacks);
    assert(success.header.status == ROHR_REPORT_COMPLETE && initialized == 3 && cleaned == 3 && updated == 2);
    assert(report_valid_check(&success) && report_valid_check(&failed) && report_valid_check(&gameplay));
    ReportResult stored = rohr_report_get(ROHR_SCENE_REPORT); assert(!error_check(stored));
    assert(stored.result.value.header.status == ROHR_REPORT_COMPLETE);
    assert(engine_update().header.status == ROHR_REPORT_FAILED);
    return 0;
}
