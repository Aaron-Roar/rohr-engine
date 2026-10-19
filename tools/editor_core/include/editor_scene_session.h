/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_SCENE_SESSION_H
#define ROHR_EDITOR_SCENE_SESSION_H
#include "report.h"
#include <SDL3/SDL.h>

typedef struct EditorSceneSession EditorSceneSession;
typedef void (*EditorSceneOutput)(void *context, const char *bytes, size_t count);
typedef void (*EditorSceneReportOutput)(void *context, const Report *report);
/* Borrowed sink; receives each chunk once during update, including shutdown.
 * Keep context alive until after session destruction. */
void editor_scene_session_output_set(EditorSceneSession *session, EditorSceneOutput output, void *context);
/* Optional observer, including updates performed by destroy. Borrowed report. */
void editor_scene_session_report_output_set(EditorSceneSession *session,
    EditorSceneReportOutput output, void *context);
typedef enum EditorSceneState {
    EDITOR_SCENE_STOPPED, EDITOR_SCENE_STARTING, EDITOR_SCENE_RUNNING,
    EDITOR_SCENE_PAUSED, EDITOR_SCENE_STOPPING, EDITOR_SCENE_FAILED
} EditorSceneState;
typedef struct EditorSceneFrame {
    const unsigned char *pixels; /* RGBA32, borrowed until next update/launch/destroy. */
    int width, height, pitch;
    uint64_t sequence, tick;
    double time;
} EditorSceneFrame;
typedef struct EditorSceneInputMode {
    bool text_active, relative_mouse;
    SDL_Rect text_area; /* Scene logical coordinates. */
    int text_cursor;
} EditorSceneInputMode;
/* Single caller thread. Explicit allocation; destroy stops and reaps an owned
 * process (up to two seconds grace). Normal UI teardown uses stop then update. */
EditorSceneSession *editor_scene_session_create(void);
void editor_scene_session_destroy(EditorSceneSession *session);
/* Acceptance only: launch creates the child; update observes startup/runtime
 * failures. An active process must finish stopping before another launch. */
EngineResult editor_scene_session_launch(EditorSceneSession *session, const char *executable);
EngineResult editor_scene_session_pause_set(EditorSceneSession *session, bool paused);
EngineResult editor_scene_session_focus_set(EditorSceneSession *session, bool focused);
EngineResult editor_scene_session_input_add(EditorSceneSession *session, const SDL_Event *event);
void editor_scene_session_stop(EditorSceneSession *session);
/* Nonblocking socket/log pumps, each with a fixed byte/packet budget. Reports
 * own evidence collected since the previous update; elapsed time measures this
 * call, excluding child simulation. Failed diagnostics are delivered once. */
Report editor_scene_session_update(EditorSceneSession *session);
EditorSceneState editor_scene_session_state_get(const EditorSceneSession *session);
bool editor_scene_session_active_check(const EditorSceneSession *session);
EditorSceneFrame editor_scene_session_frame_get(const EditorSceneSession *session);
EditorSceneInputMode editor_scene_session_input_mode_get(const EditorSceneSession *session);
const char *editor_scene_session_error_get(const EditorSceneSession *session);
/* Last 16 KiB of combined stdout/stderr, borrowed until next update/launch.
 * Tail-discarded bytes are counted, and reported only without an output sink.
 * Logs can contain arbitrary UTF-8 fragments. */
const char *editor_scene_session_log_get(const EditorSceneSession *session);
uint64_t editor_scene_session_log_dropped_get(const EditorSceneSession *session);
/* Map an editor point through the centered, aspect-preserving scene rectangle.
 * Returns false outside it; coordinates are still supplied for pointer capture. */
bool editor_scene_session_pointer_map(const EditorSceneFrame *frame, SDL_FRect viewport,
    SDL_FPoint point, SDL_FPoint *logical);
#endif
