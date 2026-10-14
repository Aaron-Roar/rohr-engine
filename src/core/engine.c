/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include "core/report_internal.h"
#include <SDL3/SDL.h>
#include "engine.h"
#include "audio.h"
#include "core/engine_internal.h"
#include "entity_components.h"
#include "physics.h"
#include "graphics.h"
#include "input/input_internal.h"

SDL_Event sdl_event;

bool engine_running = false;
bool engine_paused = false;

Tick engine_tick_count = 0;
static Tick engine_tick_delta = 0;

Time engine_time = 0.0;   // simulated engine time in seconds
Time engine_time_per_tick = 1.0 / 60.0;
Time engine_tick_accumulator = 0.0;

SDLTime sdl_prev_counter = 0;
SDLTime sdl_frequency = 0;

EngineResult engine_tables_ensure_capacity(size_t capacity) {
    EngineResult result;

    result = entity_tables_ensure_capacity(capacity);
    if(result.kind != ERROR_RESULT_ERROR) {
        result = physics_tables_ensure_capacity(capacity);
    }
    if(result.kind != ERROR_RESULT_ERROR) {
        result = graphics_tables_ensure_capacity(capacity);
    }
    return result;
}

EngineResult engine_start(void) {
    EngineResult result;

    if(engine_running) {
        return error_result_error(ERROR_ENGINE_ALREADY_RUNNING);
    }

    SDL_SetMainReady();
    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        return error_result_error_detail(ERROR_ENGINE_SDL_INIT_FAILED,
            SDL_GetError());
    }
    result = entity_tables_init();
    if(result.kind == ERROR_RESULT_ERROR) {
        SDL_Quit();
        return result;
    }
    result = physics_tables_init();
    if(result.kind == ERROR_RESULT_ERROR) {
        entity_tables_destroy();
        SDL_Quit();
        return result;
    }
    result = graphics_tables_init();
    if(result.kind == ERROR_RESULT_ERROR) {
        physics_tables_destroy();
        entity_tables_destroy();
        SDL_Quit();
        return result;
    }
    result = physics_broadphase_init();
    if(result.kind == ERROR_RESULT_ERROR) {
        graphics_tables_destroy();
        physics_tables_destroy();
        entity_tables_destroy();
        SDL_Quit();
        return result;
    }
    game_state_runtime_reset();

    sdl_frequency = SDL_GetPerformanceFrequency();
    sdl_prev_counter = SDL_GetPerformanceCounter();

    engine_time = 0.0;
    engine_time_per_tick = 1.0 / 60.0;
    engine_tick_accumulator = 0.0;
    engine_tick_count = 0;
    engine_tick_delta = 0;

    input_start();
    engine_paused = false;
    engine_running = true;
    return error_result_value(true);
}

void engine_pause(void) {
    engine_paused = true;
}
bool engine_paused_get(void) {
    if(engine_paused) {
        return true;
    }
    return false;
}

void engine_resume(void) {
    sdl_prev_counter = SDL_GetPerformanceCounter();
    engine_paused = false;
}

void engine_time_update(void) {
    SDLTime current_counter = SDL_GetPerformanceCounter();

    if(engine_paused || !engine_running) {
        sdl_prev_counter = current_counter;
        return;
    }
    Time real_dt = (Time)(current_counter - sdl_prev_counter) / (Time)sdl_frequency;
    sdl_prev_counter = current_counter;
    engine_time += real_dt;
    engine_tick_accumulator += real_dt;
}

Tick engine_tick_update(void) {
    Tick ticks_advanced;
    engine_tick_delta = 0;
    engine_time_update();
    if(engine_paused || !engine_running) {
        return 0;
    }
    ticks_advanced = (Tick)(engine_tick_accumulator / engine_time_per_tick);
    if(ticks_advanced > 0) {
        engine_tick_accumulator -= (Time)ticks_advanced * engine_time_per_tick;
        engine_tick_count += ticks_advanced;
    }
    engine_tick_delta = ticks_advanced;
    return ticks_advanced;
}

Tick engine_tick_delta_get(void) {
    return engine_tick_delta;
}

Tick engine_tick_get(void) {
    return engine_tick_count;
}

Time engine_time_get(void) {
    return engine_time;
}
void engine_clock_reset(void) {
    sdl_prev_counter = SDL_GetPerformanceCounter();
    engine_tick_accumulator = 0.0;
    engine_tick_delta = 0;
}

EngineResult engine_time_per_tick_set(Time value) {
    if(value <= 0.0) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    engine_time_per_tick = value;
    return error_result_value(true);
}

Time engine_time_per_tick_get(void) { return engine_time_per_tick; }

void engine_stop(void) {
    audio_stop();
    input_stop();
    game_state_runtime_reset();
    physics_broadphase_destroy();
    graphics_tables_destroy();
    physics_tables_destroy();
    entity_tables_destroy();
    engine_running = false;
    engine_tick_delta = 0;
    SDL_Quit();
}

SDL_Event engine_event_poll(void) {
    while (SDL_PollEvent(&sdl_event)) {
        input_event_add(&sdl_event);
        return sdl_event;
    }
    return (SDL_Event) {0};
}

/* Each report owns its evidence; no report collection occurs in tick-only calls. */
static void engine_report_statistic_add(Report *report, ReportMetricId metric,
        uint64_t value) {
    (void)report_statistic_add(report, (ReportStatistic){.report_id = report->header.id, .metric_id = metric, .aggregation = ROHR_REPORT_AGGREGATION_SUM, .value = value});
}

static void engine_report_issue(Report *report, uint32_t stage, EngineError code,
        bool fatal, Entity entity, const char *detail) {
    ReportDiagnostic diagnostic = {.report_id = report->header.id,
        .internal_stage = stage, .code = code, .occurrence_count = 1,
        .severity = fatal ? ROHR_REPORT_SEVERITY_FATAL : ROHR_REPORT_SEVERITY_WARNING,
        .recovery = fatal ? ROHR_REPORT_UPDATE_ABORTED : ROHR_REPORT_CONTRIBUTION_SKIPPED,
        .source = entity == ENTITY_INVALID ? (ReportObjectRef){0} :
            (ReportObjectRef){ROHR_REPORT_OBJECT_ENTITY, entity}};
    (void)report_diagnostic_detail_set(&diagnostic, detail);
    report_diagnostic_collect_internal(report, diagnostic);
}

static void engine_report_finish(Report *report, Uint64 started) {
    report->header.total_ms = (double)(SDL_GetPerformanceCounter() - started) *
        1000.0 / (double)SDL_GetPerformanceFrequency();
    report_publish_internal(report);
}

Report engine_lifecycle_update(void) {
    Uint64 started = SDL_GetPerformanceCounter();
    Report report = {.header = {ROHR_ENGINE_LIFECYCLE_REPORT, ROHR_REPORT_COMPLETE, 0}};
    uint64_t examined = 0, expired = 0, deleted = 0, failed = 0;
    if(!engine_running) {
        engine_report_issue(&report, 0, ERROR_ENGINE_STATE_INVALID, true,
            ENTITY_INVALID, "Engine lifecycle update requires engine startup");
        engine_report_finish(&report, started);
        return report;
    }
    for(EntityIndex index = 0; index < MAX_ENTITIES; index += 1) {
        if(!entity_index_alive_check(index) ||
                !entity_index_components_check(index, ROHR_LIFETIME)) continue;
        examined += 1;
        if((life_times[index].expirey_time != 0 &&
                    life_times[index].expirey_time <= engine_time_get()) ||
                (life_times[index].expirey_tick != 0 &&
                    life_times[index].expirey_tick <= engine_tick_get())) {
            expired += 1;
            EntityResult entity = entity_from_index_get(index);
            if(error_check(entity)) {
                char detail[ROHR_REPORT_DETAIL_CAPACITY * 3];
                snprintf(detail, sizeof(detail), "Expired entity slot %" PRIu64 ": %s",
                    (uint64_t)index, error_result_message_get(entity.kind, entity.result.error));
                engine_report_issue(&report, 1, entity.result.error, false, ENTITY_INVALID, detail);
                failed += 1;
                continue;
            }
            EngineResult result = entity_delete(entity.result.value);
            if(error_check(result)) {
                engine_report_issue(&report, 2, result.result.error, false, entity.result.value,
                    error_result_message_get(result.kind, result.result.error));
                failed += 1;
            } else deleted += 1;
        }
    }
    engine_report_statistic_add(&report, ROHR_REPORT_CANDIDATE_COUNT, examined);
    engine_report_statistic_add(&report, ROHR_REPORT_EXPIRED_ENTITY_COUNT, expired);
    engine_report_statistic_add(&report, ROHR_REPORT_PROCESSED_COUNT, deleted);
    engine_report_statistic_add(&report, ROHR_REPORT_SKIPPED_COUNT, failed);
    engine_report_finish(&report, started);
    return report;
}

Report engine_update(void) {
    Uint64 started = SDL_GetPerformanceCounter();
    Report report = {.header = {ROHR_ENGINE_REPORT, ROHR_REPORT_COMPLETE, 0}};
    if(!engine_running) {
        engine_report_issue(&report, 0, ERROR_ENGINE_STATE_INVALID, true,
            ENTITY_INVALID, "Engine update requires engine startup");
    } else {
        engine_report_statistic_add(&report, ROHR_REPORT_TICKS_ADVANCED_COUNT, engine_tick_update());
        Report lifecycle = engine_lifecycle_update();
        (void)report_merge(&report, &lifecycle);
    }
    engine_report_finish(&report, started);
    return report;
}
