/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_scene_session.h"
#include "scene/scene_transport.h"
#include "core/report_internal.h"
#include <math.h>
#include <string.h>

#define SCENE_CONTROL_CAPACITY 256u
#define SCENE_LOG_CAPACITY 16384u
#define SCENE_LOG_PUMP_BYTES 65536u

typedef struct SceneControl {
    SceneMessage kind;
    size_t size;
    unsigned char data[SCENE_INPUT_BYTES];
} SceneControl;
struct EditorSceneSession {
    SDL_Process *process;
    SceneSocket listener;
    SceneWire wire;
    EditorSceneState state;
    bool transport, focused, stopping, failed, failure_pending;
    bool stop_sent, killed;
    char token[SCENE_TOKEN_TEXT_SIZE], error[256];
    uint64_t deadline, log_dropped, log_pending;
    char log[SCENE_LOG_CAPACITY + 1];
    size_t log_used;
    SceneControl controls[SCENE_CONTROL_CAPACITY];
    size_t control_start, control_count;
    EditorSceneFrame frame;
    size_t pixels_capacity;
    EditorSceneInputMode input_mode;
};
static EngineResult invalid(const char *detail) {
    return error_result_error_detail(ERROR_ENGINE_STATE_INVALID, detail);
}
static void session_fail(EditorSceneSession *session, const char *detail) {
    if(!session->failed) {
        session->failed = session->failure_pending = true;
        SDL_strlcpy(session->error, detail, sizeof(session->error));
    }
    if(session->process && !session->stopping) {
        session->stopping = true;
        session->deadline = SDL_GetTicks() + SCENE_STOP_TIMEOUT_MS;
        session->control_count = 0;
    }
    session->state = EDITOR_SCENE_FAILED;
}
static void connection_close(EditorSceneSession *session) {
    scene_socket_close(session->listener); session->listener = SCENE_SOCKET_INVALID;
    scene_wire_destroy(&session->wire);
    if(session->transport) scene_transport_stop();
    session->transport = false;
    session->input_mode = (EditorSceneInputMode){0};
}
EditorSceneSession *editor_scene_session_create(void) {
    EditorSceneSession *session = SDL_calloc(1, sizeof(*session));
    if(session) { session->listener = SCENE_SOCKET_INVALID; scene_wire_init(&session->wire, SCENE_SOCKET_INVALID); }
    return session;
}
static bool log_collect(EditorSceneSession *session) {
    SDL_IOStream *output = SDL_GetProcessOutput(session->process);
    if(!output) { session_fail(session, "Scene log stream is unavailable"); return true; }
    unsigned char buffer[4096];
    for(size_t budget = SCENE_LOG_PUMP_BYTES; budget;) {
        size_t count = SDL_ReadIO(output, buffer, SDL_min(budget, sizeof(buffer)));
        if(!count) {
            SDL_IOStatus status = SDL_GetIOStatus(output);
            if(status == SDL_IO_STATUS_ERROR) session_fail(session, SDL_GetError());
            return status == SDL_IO_STATUS_EOF || status == SDL_IO_STATUS_ERROR;
        }
        budget -= count;
        size_t drop = session->log_used + count > SCENE_LOG_CAPACITY ?
            session->log_used + count - SCENE_LOG_CAPACITY : 0;
        if(drop) {
            memmove(session->log, session->log + drop, session->log_used - drop);
            session->log_used -= drop;
            session->log_dropped += drop; session->log_pending += drop;
        }
        memcpy(session->log + session->log_used, buffer, count);
        session->log_used += count; session->log[session->log_used] = 0;
    }
    return false;
}
EngineResult editor_scene_session_launch(EditorSceneSession *session, const char *executable) {
    if(!session || !executable || !*executable || session->process) return invalid("Scene launch needs an idle session and executable path");
    connection_close(session);
    SDL_free((void *)session->frame.pixels);
    session->frame = (EditorSceneFrame){0}; session->pixels_capacity = 0;
    session->control_count = session->control_start = 0;
    session->focused = session->stopping = session->failed = session->failure_pending = false;
    session->stop_sent = session->killed = false;
    session->log_used = 0; session->log[0] = session->error[0] = 0;
    session->log_dropped = session->log_pending = 0;
    uint16_t port;
    if(!(session->transport = scene_transport_start()) || !scene_token_create(session->token) ||
            (session->listener = scene_socket_listen(&port)) == SCENE_SOCKET_INVALID) {
        session_fail(session, "Could not create the loopback scene listener"); connection_close(session);
        return invalid(session->error);
    }
    char port_text[8]; SDL_snprintf(port_text, sizeof(port_text), "%u", port);
    const char *args[] = {executable, "--rohr-scene-host", port_text, session->token, NULL};
    SDL_PropertiesID properties = SDL_CreateProperties();
    bool configured = properties && SDL_SetPointerProperty(properties, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, (void *)args) &&
        SDL_SetNumberProperty(properties, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP) &&
        SDL_SetBooleanProperty(properties, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);
    if(configured) session->process = SDL_CreateProcessWithProperties(properties);
    SDL_DestroyProperties(properties);
    if(!session->process) {
        session_fail(session, SDL_GetError()); connection_close(session); return invalid(session->error);
    }
    session->state = EDITOR_SCENE_STARTING;
    session->deadline = SDL_GetTicks() + SCENE_START_TIMEOUT_MS;
    return error_result_value(true);
}
static EngineResult control_add(EditorSceneSession *session, SceneMessage kind, const void *data, size_t size) {
    if(!session || !session->process || session->stopping) return invalid("Scene is not accepting controls");
    if(session->control_count == SCENE_CONTROL_CAPACITY) {
        /* A lost release would leave a stuck key/button. Stop explicitly. */
        session_fail(session, "Scene input/control queue overflow; simulation stopped to release input");
        return invalid(session->error);
    }
    SceneControl *control = &session->controls[(session->control_start + session->control_count++) % SCENE_CONTROL_CAPACITY];
    control->kind = kind; control->size = size;
    if(size) memcpy(control->data, data, size);
    return error_result_value(true);
}
EngineResult editor_scene_session_pause_set(EditorSceneSession *session, bool paused) {
    return control_add(session, paused ? SCENE_PAUSE : SCENE_RESUME, NULL, 0);
}
EngineResult editor_scene_session_focus_set(EditorSceneSession *session, bool focused) {
    unsigned char data[4]; scene_u32_write(data, focused);
    EngineResult result = control_add(session, SCENE_FOCUS, data, sizeof(data));
    if(!error_check(result)) session->focused = focused;
    return result;
}
EngineResult editor_scene_session_input_add(EditorSceneSession *session, const SDL_Event *event) {
    if(!session || !session->focused || session->state != EDITOR_SCENE_RUNNING || session->stopping)
        return invalid("Scene input requires focus and running simulation");
    unsigned char data[SCENE_INPUT_BYTES];
    if(!scene_input_encode(event, data)) return invalid("Unsupported or invalid scene input event");
    return control_add(session, SCENE_INPUT, data, sizeof(data));
}
void editor_scene_session_stop(EditorSceneSession *session) {
    if(!session || !session->process || session->stopping) return;
    session->stopping = true; session->focused = false;
    session->control_count = 0;
    session->deadline = SDL_GetTicks() + SCENE_STOP_TIMEOUT_MS;
    session->state = EDITOR_SCENE_STOPPING;
}
static bool packet_apply(EditorSceneSession *session) {
    SceneWire *wire = &session->wire;
    if(!wire->authenticated) {
        if(wire->kind != SCENE_HELLO || memcmp(wire->rx, session->token, SCENE_TOKEN_TEXT_SIZE - 1)) return false;
        if(!scene_wire_send(wire, SCENE_READY, session->token, SCENE_TOKEN_TEXT_SIZE - 1)) return false;
        wire->authenticated = true;
        scene_socket_close(session->listener); session->listener = SCENE_SOCKET_INVALID;
        return true;
    }
    switch(wire->kind) {
        case SCENE_FRAME: {
            uint32_t width = scene_u32_read(wire->rx), height = scene_u32_read(wire->rx + 4);
            if(!width || !height || width > SCENE_FRAME_SIDE_MAX || height > SCENE_FRAME_SIDE_MAX ||
                    wire->size != SCENE_FRAME_HEADER_SIZE + (size_t)width * height * 4 ||
                    !isfinite(scene_double_read(wire->rx + 16)) ||
                    scene_u64_read(wire->rx + 24) <= session->frame.sequence) return false;
            size_t size = (size_t)width * height * 4;
            if(size > session->pixels_capacity) {
                void *pixels = SDL_realloc((void *)session->frame.pixels, size);
                if(!pixels) { session_fail(session, "Scene frame allocation failed"); return false; }
                session->frame.pixels = pixels; session->pixels_capacity = size;
            }
            memcpy((void *)session->frame.pixels, wire->rx + SCENE_FRAME_HEADER_SIZE, size);
            session->frame.width = (int)width; session->frame.height = (int)height;
            session->frame.pitch = (int)width * 4;
            session->frame.tick = scene_u64_read(wire->rx + 8);
            session->frame.time = scene_double_read(wire->rx + 16);
            session->frame.sequence = scene_u64_read(wire->rx + 24);
            return true;
        }
        case SCENE_STATE: {
            uint32_t flags = scene_u32_read(wire->rx);
            if(flags & ~7u || !isfinite(scene_double_read(wire->rx + 16))) return false;
            if(!session->stopping) session->state = flags & 1u ? EDITOR_SCENE_PAUSED : EDITOR_SCENE_RUNNING;
            session->input_mode = (EditorSceneInputMode){.text_active = (flags & 2u) != 0,
                .relative_mouse = (flags & 4u) != 0,
                .text_area = {(int32_t)scene_u32_read(wire->rx + 24), (int32_t)scene_u32_read(wire->rx + 28),
                    (int32_t)scene_u32_read(wire->rx + 32), (int32_t)scene_u32_read(wire->rx + 36)},
                .text_cursor = (int32_t)scene_u32_read(wire->rx + 40)};
            return session->input_mode.text_area.w >= 0 && session->input_mode.text_area.h >= 0;
        }
        case SCENE_FAILURE:
            if(!memchr(wire->rx, 0, wire->size)) return false;
            session_fail(session, (const char *)wire->rx); return true;
        default: return false;
    }
}
static void controls_flush(EditorSceneSession *session, size_t *budget) {
    SceneWire *wire = &session->wire;
    if(!scene_wire_flush(wire, budget)) return;
    if(!wire->authenticated) return;
    for(unsigned i = 0; i < SCENE_CONTROL_CAPACITY && *budget && !scene_wire_busy_check(wire); ++i) {
        if(session->stopping) {
            if(!session->stop_sent) session->stop_sent = scene_wire_send(wire, SCENE_STOP, NULL, 0);
            (void)scene_wire_flush(wire, budget); return;
        }
        if(!session->control_count) break;
        SceneControl *control = &session->controls[session->control_start];
        if(!scene_wire_send(wire, control->kind, control->data, control->size)) return;
        session->control_start = (session->control_start + 1) % SCENE_CONTROL_CAPACITY;
        --session->control_count;
        if(!scene_wire_flush(wire, budget)) return;
    }
}
static void diagnostic_add(Report *report, bool fatal, const char *detail, uint64_t count) {
    ReportDiagnostic diagnostic = {.report_id = ROHR_SCENE_SESSION_REPORT_ID,
        .code = ERROR_ENGINE_STATE_INVALID,
        .severity = fatal ? ROHR_REPORT_SEVERITY_FATAL : ROHR_REPORT_SEVERITY_WARNING,
        .recovery = fatal ? ROHR_REPORT_UPDATE_ABORTED : ROHR_REPORT_CONTRIBUTION_SKIPPED,
        .occurrence_count = count};
    (void)report_diagnostic_detail_set(&diagnostic, detail);
    (void)report_diagnostic_add(report, diagnostic);
}
Report editor_scene_session_update(EditorSceneSession *session) {
    uint64_t started = SDL_GetTicksNS();
    Report report = report_create(); report.header.id = ROHR_SCENE_SESSION_REPORT_ID;
    if(!session) { diagnostic_add(&report, true, "Scene session is null", 1); goto done; }
    report.header.status = session->process || session->failure_pending ? ROHR_REPORT_COMPLETE : ROHR_REPORT_NOT_RUN;
    if(session->process) {
        bool logs_finished = log_collect(session);
        if(session->wire.socket == SCENE_SOCKET_INVALID && session->listener != SCENE_SOCKET_INVALID) {
            SceneSocket accepted = scene_socket_accept(session->listener);
            if(accepted != SCENE_SOCKET_INVALID) scene_wire_init(&session->wire, accepted);
        }
        if(session->wire.socket != SCENE_SOCKET_INVALID && !session->wire.closed && !session->wire.failed) {
            size_t budget = SCENE_PUMP_BYTES;
            for(unsigned i = 0; i < 64 && budget; ++i) {
                int received = scene_wire_receive(&session->wire, &budget);
                if(received <= 0) break;
                if(!packet_apply(session)) { session_fail(session, "Invalid scene handshake or message"); break; }
            }
            budget = SCENE_PUMP_BYTES;
            controls_flush(session, &budget);
        }
        if((session->wire.failed || session->wire.closed) && !session->stopping)
            session_fail(session, session->wire.failed ? session->wire.error : "Scene process disconnected");
        if(session->state == EDITOR_SCENE_STARTING && SDL_GetTicks() >= session->deadline)
            session_fail(session, "Scene startup timed out; rebuild scene.c with rohr_scene_run callbacks");
        if(session->stopping && !session->killed && SDL_GetTicks() >= session->deadline) {
            if(SDL_KillProcess(session->process, true)) {
                session->killed = true;
                session_fail(session, "Scene did not stop within two seconds and was terminated");
            } else session_fail(session, SDL_GetError());
        }
        int exit_code;
        if(SDL_WaitProcess(session->process, false, &exit_code) && logs_finished) {
            if(!session->stopping || (exit_code != 0 && !session->killed)) {
                char detail[128]; SDL_snprintf(detail, sizeof(detail), "Scene exited with code %d", exit_code);
                session_fail(session, detail);
            }
            SDL_DestroyProcess(session->process); session->process = NULL;
            connection_close(session);
            session->state = session->failed ? EDITOR_SCENE_FAILED : EDITOR_SCENE_STOPPED;
        }
    }
    if(session->failure_pending) {
        diagnostic_add(&report, true, session->error, 1); session->failure_pending = false;
    }
    if(session->log_pending) {
        diagnostic_add(&report, false, "Scene log capacity exceeded; oldest bytes discarded", session->log_pending);
        session->log_pending = 0;
    }
done:
    report.header.total_ms = (double)(SDL_GetTicksNS() - started) / 1e6;
    report_publish_internal(&report); return report;
}
void editor_scene_session_destroy(EditorSceneSession *session) {
    if(!session) return;
    editor_scene_session_stop(session);
    uint64_t deadline = SDL_GetTicks() + SCENE_STOP_TIMEOUT_MS;
    while(session->process && SDL_GetTicks() < deadline) {
        (void)editor_scene_session_update(session);
        if(session->process) SDL_Delay(1);
    }
    if(session->process) {
        (void)SDL_KillProcess(session->process, true);
        (void)SDL_WaitProcess(session->process, true, NULL);
        SDL_DestroyProcess(session->process);
    }
    connection_close(session); SDL_free((void *)session->frame.pixels); SDL_free(session);
}
EditorSceneState editor_scene_session_state_get(const EditorSceneSession *session) { return session ? session->state : EDITOR_SCENE_STOPPED; }
bool editor_scene_session_active_check(const EditorSceneSession *session) { return session && session->process; }
EditorSceneFrame editor_scene_session_frame_get(const EditorSceneSession *session) { return session ? session->frame : (EditorSceneFrame){0}; }
EditorSceneInputMode editor_scene_session_input_mode_get(const EditorSceneSession *session) { return session ? session->input_mode : (EditorSceneInputMode){0}; }
const char *editor_scene_session_error_get(const EditorSceneSession *session) { return session ? session->error : "Scene session is null"; }
const char *editor_scene_session_log_get(const EditorSceneSession *session) { return session ? session->log : ""; }
uint64_t editor_scene_session_log_dropped_get(const EditorSceneSession *session) { return session ? session->log_dropped : 0; }
bool editor_scene_session_pointer_map(const EditorSceneFrame *frame, SDL_FRect viewport, SDL_FPoint point, SDL_FPoint *logical) {
    if(!frame || !logical || frame->width <= 0 || frame->height <= 0 || viewport.w <= 0 || viewport.h <= 0 ||
            !isfinite(viewport.x) || !isfinite(viewport.y) || !isfinite(viewport.w) || !isfinite(viewport.h) ||
            !isfinite(point.x) || !isfinite(point.y)) return false;
    float scale = SDL_min(viewport.w / frame->width, viewport.h / frame->height);
    float x = viewport.x + (viewport.w - frame->width * scale) / 2;
    float y = viewport.y + (viewport.h - frame->height * scale) / 2;
    *logical = (SDL_FPoint){(point.x - x) / scale, (point.y - y) / scale};
    return logical->x >= 0 && logical->y >= 0 && logical->x < frame->width && logical->y < frame->height;
}
