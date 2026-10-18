/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "scene.h"
#include "scene_runtime.h"
#include "scene_transport.h"
#include "graphics.h"
#include "physics.h"
#include "audio.h"
#include "input.h"
#include "input/input_internal.h"
#include "core/report_internal.h"
#include <stdlib.h>
#include <string.h>

static void scene_issue(Report *report, EngineError error, const char *detail) {
    ReportDiagnostic diagnostic = {.report_id = ROHR_SCENE_REPORT_ID,
        .code = error, .severity = ROHR_REPORT_SEVERITY_FATAL,
        .recovery = ROHR_REPORT_UPDATE_ABORTED, .occurrence_count = 1};
    (void)report_diagnostic_detail_set(&diagnostic, detail);
    (void)report_diagnostic_add(report, diagnostic);
}
static bool scene_result(Report *report, EngineResult result) {
    if(!error_check(result)) return true;
    scene_issue(report, result.result.error, error_result_message_get(result.kind, result.result.error)); return false;
}
static bool scene_child(Report *report, Report child) {
    (void)report_merge(report, &child);
    return child.header.status != ROHR_REPORT_FAILED;
}
typedef struct SceneRun {
    SceneWire wire;
    uint64_t sequence;
    bool paused, focused, stop, state_dirty;
} SceneRun;
static bool scene_capture_ready(void *context) {
    SceneRun *run = context;
    return !scene_wire_busy_check(&run->wire) && !run->state_dirty;
}
static EngineResult scene_capture(void *context, const SDL_Surface *surface) {
    SceneRun *run = context;
    if(scene_wire_busy_check(&run->wire) || run->state_dirty) return error_result_value(true);
    if(surface->w <= 0 || surface->h <= 0 || surface->w > (int)SCENE_FRAME_SIDE_MAX ||
            surface->h > (int)SCENE_FRAME_SIDE_MAX)
        return error_result_error_detail(ERROR_ENGINE_GRAPHICS_STATE_FAILED, "Hosted frame exceeds 4096 by 4096");
    SDL_Surface *rgba = SDL_ConvertSurface((SDL_Surface *)surface, SDL_PIXELFORMAT_RGBA32);
    if(!rgba) return error_result_error_detail(ERROR_ENGINE_GRAPHICS_STATE_FAILED, SDL_GetError());
    unsigned char header[SCENE_FRAME_HEADER_SIZE];
    scene_u32_write(header, (uint32_t)rgba->w); scene_u32_write(header + 4, (uint32_t)rgba->h);
    scene_u64_write(header + 8, engine_tick_get()); scene_double_write(header + 16, engine_time_get());
    scene_u64_write(header + 24, ++run->sequence);
    /* RGBA32 surfaces have a four-byte aligned pitch equal to width * 4. */
    bool sent = rgba->pitch == rgba->w * 4 && scene_wire_send_parts(&run->wire, SCENE_FRAME,
        header, sizeof(header), rgba->pixels, (size_t)rgba->pitch * rgba->h);
    SDL_DestroySurface(rgba);
    return sent ? error_result_value(true) : error_result_error_detail(
        ERROR_ENGINE_GRAPHICS_STATE_FAILED, "Could not queue the hosted frame");
}
static void release_input(void) {
    SDL_Event event = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    input_event_add(&event);
}
static bool scene_commands(SceneRun *run, Report *report) {
    size_t budget = SCENE_PUMP_BYTES;
    for(unsigned i = 0; i < 64; ++i) {
        int received = scene_wire_receive(&run->wire, &budget);
        if(received == 0) break;
        if(received < 0) {
            scene_issue(report, ERROR_ENGINE_STATE_INVALID, run->wire.failed ? run->wire.error : "Scene host disconnected");
            return false;
        }
        switch(run->wire.kind) {
            case SCENE_STOP: run->stop = true; return true;
            case SCENE_PAUSE:
                if(!run->paused) {
                    if(!scene_result(report, audio_scene_pause_set(true))) return false;
                    run->paused = true; engine_pause(); release_input();
                }
                run->state_dirty = true; break;
            case SCENE_RESUME:
                if(run->paused) {
                    if(!scene_result(report, audio_scene_pause_set(false))) return false;
                    run->paused = false; engine_resume();
                }
                run->state_dirty = true; break;
            case SCENE_FOCUS:
                if(scene_u32_read(run->wire.rx) > 1) goto invalid;
                run->focused = scene_u32_read(run->wire.rx) != 0;
                if(!run->focused) release_input();
                run->state_dirty = true; break;
            case SCENE_INPUT: {
                SDL_Event event;
                if(!scene_input_decode(run->wire.rx, &event)) goto invalid;
                if(run->focused && !run->paused) input_event_add(&event);
                break;
            }
            default: goto invalid;
        }
    }
    return true;
invalid:
    scene_issue(report, ERROR_ENGINE_STATE_INVALID, "Unexpected or invalid scene control message"); return false;
}
static bool scene_state_send(SceneRun *run) {
    unsigned char data[48] = {0}; int cursor;
    SDL_Rect area = input_host_text_area_get(&cursor);
    uint32_t flags = (run->paused ? 1u : 0u) | (input_text_state_get().active ? 2u : 0u) |
        (input_mouse_relative_mode_check() ? 4u : 0u);
    scene_u32_write(data, flags); scene_u64_write(data + 8, engine_tick_get());
    scene_double_write(data + 16, engine_time_get());
    scene_u32_write(data + 24, (uint32_t)area.x); scene_u32_write(data + 28, (uint32_t)area.y);
    scene_u32_write(data + 32, (uint32_t)area.w); scene_u32_write(data + 36, (uint32_t)area.h);
    scene_u32_write(data + 40, (uint32_t)cursor);
    return scene_wire_send(&run->wire, SCENE_STATE, data, sizeof(data));
}
static bool scene_connect(SceneRun *run, uint16_t port, const char *token, Report *report) {
    SceneSocket socket = scene_socket_connect(port);
    if(socket == SCENE_SOCKET_INVALID) goto fail;
    scene_wire_init(&run->wire, socket);
    uint64_t deadline = SDL_GetTicks() + SCENE_START_TIMEOUT_MS;
    int connected;
    while((connected = scene_socket_connected_get(socket)) == 0 && SDL_GetTicks() < deadline) SDL_Delay(1);
    if(connected != 1 || !scene_wire_send(&run->wire, SCENE_HELLO, token, SCENE_TOKEN_TEXT_SIZE - 1)) goto fail;
    while(SDL_GetTicks() < deadline) {
        size_t budget = SCENE_PUMP_BYTES;
        if(!scene_wire_flush(&run->wire, &budget)) goto fail;
        int received = scene_wire_receive(&run->wire, &budget);
        if(received < 0) goto fail;
        if(received == 1) {
            if(run->wire.kind != SCENE_READY || memcmp(run->wire.rx, token, SCENE_TOKEN_TEXT_SIZE - 1)) goto fail;
            run->wire.authenticated = true;
            return true;
        }
        SDL_Delay(1);
    }
fail:
    scene_issue(report, ERROR_ENGINE_STATE_INVALID, "Could not authenticate the scene host (protocol, token, connection, or timeout)");
    return false;
}
Report scene_run(int argc, char **argv, SceneCallbacks callbacks) {
    uint64_t started = SDL_GetTicksNS();
    Report report = report_create(); report.header = (ReportHeader){ROHR_SCENE_REPORT_ID, ROHR_REPORT_COMPLETE, 0};
    SceneRun run = {.wire = {.socket = SCENE_SOCKET_INVALID}, .state_dirty = true};
    bool hosted = argc == 4 && argv && argv[1] && argv[2] && argv[3] && strcmp(argv[1], "--rohr-scene-host") == 0;
    bool transport = false, engine = false, initialized = false;
    if(!callbacks.initialize || !callbacks.shutdown || (argc != 1 && !hosted)) {
        scene_issue(&report, ERROR_ENGINE_STATE_INVALID, "Scene runner needs initialization/cleanup callbacks and valid arguments");
        goto done;
    }
    if(hosted) {
        char *end = NULL; unsigned long port = strtoul(argv[2], &end, 10);
        if(!argv[2][0] || !end || *end || port == 0 || port > UINT16_MAX || !scene_token_check(argv[3])) {
            scene_issue(&report, ERROR_ENGINE_STATE_INVALID, "Invalid scene host address or token"); goto done;
        }
        if(!(transport = scene_transport_start())) {
            scene_issue(&report, ERROR_ENGINE_STATE_INVALID, "Could not initialize scene transport"); goto done;
        }
        if(!scene_connect(&run, (uint16_t)port, argv[3], &report)) goto done;
    }
    if(!scene_result(&report, engine_start())) goto done;
    engine = true;
    graphics_scene_configure(hosted, hosted ? scene_capture : NULL, scene_capture_ready, &run);
    input_host_mode_set(hosted);
    if(!scene_result(&report, graphics_start())) goto done;
    initialized = true;
    if(!scene_result(&report, callbacks.initialize(callbacks.context))) goto done;
    engine_clock_reset();
    while(!run.stop) {
        uint64_t frame_start = SDL_GetTicksNS();
        input_frame_begin();
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_EVENT_QUIT) run.stop = true;
            if(!hosted) input_event_add(&event);
        }
        if(hosted && !scene_commands(&run, &report)) break;
        if(run.stop || (!hosted && input_key_pressed_check(SDL_SCANCODE_ESCAPE))) break;
        if(!run.paused) {
            if(!scene_child(&report, engine_update())) break;
            if(callbacks.update && !scene_child(&report, callbacks.update(callbacks.context,
                    engine_tick_delta_get(), engine_time_get()))) break;
            if(!scene_child(&report, physics_update(engine_tick_delta_get())) ||
                    !scene_child(&report, audio_update()) ||
                    !scene_child(&report, graphics_sprite_frames_update(engine_tick_get(), engine_time_get()))) break;
            if(!scene_child(&report, graphics_show())) break;
        }
        if(hosted) {
            size_t budget = SCENE_PUMP_BYTES;
            if(!scene_wire_flush(&run.wire, &budget)) {
                scene_issue(&report, ERROR_ENGINE_STATE_INVALID, run.wire.error); break;
            }
            /* State follows every completed frame, and explicit controls while
             * paused. Never enqueue behind an unbounded frame backlog. */
            if(!scene_wire_busy_check(&run.wire) && (run.state_dirty || !run.paused)) {
                if(!scene_state_send(&run)) { scene_issue(&report, ERROR_ENGINE_STATE_INVALID, run.wire.error); break; }
                run.state_dirty = false;
                (void)scene_wire_flush(&run.wire, &budget);
            }
            uint64_t elapsed = SDL_GetTicksNS() - frame_start;
            if(elapsed < UINT64_C(16666667)) SDL_DelayNS(UINT64_C(16666667) - elapsed);
        }
    }
done:
    if(initialized) callbacks.shutdown(callbacks.context);
    if(engine) {
        (void)scene_child(&report, audio_stop());
        graphics_stop(); engine_stop();
        (void)audio_scene_pause_set(false);
        graphics_scene_configure(false, NULL, NULL, NULL);
    }
    if(hosted && report.header.status == ROHR_REPORT_FAILED && !scene_wire_busy_check(&run.wire) &&
            run.wire.socket != SCENE_SOCKET_INVALID) {
        const char *detail = report.has_fatal ? report.first_fatal.detail : "Scene execution failed";
        (void)scene_wire_send(&run.wire, SCENE_FAILURE, detail, SDL_min(strlen(detail) + 1, 256));
        size_t budget = SCENE_PUMP_BYTES; (void)scene_wire_flush(&run.wire, &budget);
    }
    scene_wire_destroy(&run.wire);
    if(transport) scene_transport_stop();
    report.header.total_ms = (double)(SDL_GetTicksNS() - started) / 1e6;
    report_publish_internal(&report);
    return report;
}
Report rohr_scene_run(int argc, char **argv, SceneCallbacks callbacks) { return scene_run(argc, argv, callbacks); }
