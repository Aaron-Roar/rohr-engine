/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
/* Test-only wrappers exercise the actual inline checks and cleanup in each
 * generated main. They are never emitted into a developer's application. */
#include "project.h"
#include <assert.h>
#include <stdlib.h>

static unsigned mode, diagnostics, graphics_stops, engine_stops, skipped;
static Report injected_report(ReportId id, ReportStatus status) {
    Report report = rohr_report_create();
    assert(!rohr_error_check(rohr_report_header_set(&report, (ReportHeader){id,status,0})));
    return report;
}
static Report checked_engine_update(void) {
    if(mode == 4) return injected_report(ROHR_ENGINE_REPORT_ID, ROHR_REPORT_FAILED);
    if(mode == 2 || mode == 3) rohr_engine_pause();
    return rohr_engine_update();
}
static Report checked_physics_update(Tick ticks) {
    if(mode == 1) return injected_report(ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_FAILED);
    if(mode == 2) return injected_report(ROHR_GRAPHICS_REPORT_ID, ROHR_REPORT_NOT_RUN);
    if(mode == 3) {
        Report report = injected_report(ROHR_PHYSICS_REPORT_ID, ROHR_REPORT_NOT_RUN);
        report.diagnostic_count = ROHR_REPORT_DIAGNOSTIC_CAPACITY + 1;
        return report;
    }
    Report report = rohr_physics_update(ticks);
    if(report.header.status == ROHR_REPORT_NOT_RUN && ticks == 0) ++skipped;
    return report;
}
static Report checked_sprite_update(Tick tick, Time time) {
    if(mode == 5) return injected_report(ROHR_GRAPHICS_REPORT_ID, ROHR_REPORT_FAILED);
    return rohr_graphics_sprite_frames_update(tick,time);
}
static Report checked_show(const ViewportId *views, size_t count) {
    if(mode == 6) return injected_report(ROHR_GRAPHICS_REPORT_ID, ROHR_REPORT_FAILED);
    return rohr_graphics_viewports_show(views,count);
}
static void checked_graphics_stop(void) {
    /* Teardown must release instance ownership before service shutdown could
     * conceal a missed entity/project cleanup. */
    assert(rohr_entity_alive_count_get() == 0);
    ++graphics_stops;
    rohr_graphics_stop();
}
static void checked_engine_stop(void) { ++engine_stops; rohr_engine_stop(); }
static EngineResult checked_report_write(const Report *report) {
    ++diagnostics;
    return rohr_report_console_write(report);
}
#define rohr_engine_update checked_engine_update
#define rohr_physics_update checked_physics_update
#define rohr_graphics_sprite_frames_update checked_sprite_update
#undef rohr_graphics_viewports_show
#define rohr_graphics_viewports_show checked_show
#define rohr_graphics_stop checked_graphics_stop
#define rohr_engine_stop checked_engine_stop
#define rohr_report_console_write checked_report_write
#define main example_main
#include EXAMPLE_PATHWAY_SOURCE
#undef main

int main(int argc, char **argv) {
    assert(argc == 2);
    mode = (unsigned)strtoul(argv[1],NULL,10);
    assert(mode <= 6);
    int result = example_main();
    assert(result == (mode ? 1 : 0));
    assert(graphics_stops == 1 && engine_stops == 1);
    assert(rohr_entity_alive_count_get() == 0);
    assert(mode ? diagnostics > 0 : skipped > 0 && diagnostics == 0);
    return 0;
}
