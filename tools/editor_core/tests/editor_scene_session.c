/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_scene_session.h"
#include "scene/scene_transport.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define OK(call) do { EngineResult checked = (call); if(error_check(checked)) { \
    fprintf(stderr, "%s: %s\n", #call, error_result_message_get(checked.kind, checked.result.error)); assert(false); } } while(0)
#define EVENT(value, kind) do { SDL_zero(value); (value).type = (kind); } while(0)
static double poll_peak;
static Report poll_session(EditorSceneSession *session, bool expect_failure) {
    Report report = editor_scene_session_update(session);
    assert(report_valid_check(&report));
    if(report.header.total_ms > poll_peak) poll_peak = report.header.total_ms;
    if(!expect_failure && report.header.status == ROHR_REPORT_FAILED) {
        fprintf(stderr, "Session failure: %s\n%s\n", editor_scene_session_error_get(session), editor_scene_session_log_get(session));
        assert(false);
    }
    return report;
}
static void wait_state(EditorSceneSession *session, EditorSceneState state) {
    uint64_t deadline = SDL_GetTicks() + 10000;
    while(editor_scene_session_state_get(session) != state && SDL_GetTicks() < deadline) {
        poll_session(session, false); SDL_Delay(1);
    }
    assert(editor_scene_session_state_get(session) == state);
}
static EditorSceneFrame wait_frame(EditorSceneSession *session, uint64_t sequence) {
    uint64_t deadline = SDL_GetTicks() + 10000;
    while(editor_scene_session_frame_get(session).sequence <= sequence && SDL_GetTicks() < deadline) {
        poll_session(session, false); SDL_Delay(1);
    }
    EditorSceneFrame frame = editor_scene_session_frame_get(session);
    assert(frame.sequence > sequence); return frame;
}
static void wait_log(EditorSceneSession *session, const char *text) {
    uint64_t deadline = SDL_GetTicks() + 5000;
    while(!strstr(editor_scene_session_log_get(session), text) && SDL_GetTicks() < deadline) {
        poll_session(session, false); SDL_Delay(1);
    }
    if(!strstr(editor_scene_session_log_get(session), text)) fprintf(stderr, "Missing %s in %s\n", text, editor_scene_session_log_get(session));
    assert(strstr(editor_scene_session_log_get(session), text));
}
static void mode_set(const char *mode) { assert(SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "ROHR_SCENE_TEST_MODE", mode, true)); }
static void protocol_checks(void) {
    unsigned char data[SCENE_INPUT_BYTES]; SDL_Event decoded;
    SDL_Event input; EVENT(input, SDL_EVENT_TEXT_EDITING); input.edit.text = "compose"; input.edit.start = -1; input.edit.length = -1;
    assert(scene_input_encode(&input, data) && scene_input_decode(data, &decoded));
    assert(decoded.edit.start == -1 && !strcmp(decoded.edit.text, "compose"));
    memset(data + 48, 'a', 256); assert(!scene_input_decode(data, &decoded));
    EVENT(input, SDL_EVENT_MOUSE_MOTION); input.motion.x = NAN;
    assert(!scene_input_encode(&input, data));
    EVENT(input, SDL_EVENT_KEY_DOWN); input.key.scancode = SDL_SCANCODE_COUNT;
    assert(!scene_input_encode(&input, data));
    assert(!scene_token_check("abc")); char token[SCENE_TOKEN_TEXT_SIZE];
    assert(scene_token_create(token) && scene_token_check(token));
    EditorSceneFrame frame = {.width = 320, .height = 180}; SDL_FPoint point;
    assert(editor_scene_session_pointer_map(&frame, (SDL_FRect){10,20,640,480}, (SDL_FPoint){330,260}, &point));
    assert(fabsf(point.x - 160) < .001f && fabsf(point.y - 90) < .001f);
    assert(!editor_scene_session_pointer_map(&frame, (SDL_FRect){10,20,640,480}, (SDL_FPoint){330,25}, &point));
}
int main(int argc, char **argv) {
    assert(argc >= 2); protocol_checks();
    EditorSceneSession *session = editor_scene_session_create(); assert(session);
    assert(editor_scene_session_update(session).header.status == ROHR_REPORT_NOT_RUN);
    assert(error_check(editor_scene_session_launch(session, "/nonexistent/rohr_scene_fixture")));
    Report missing = editor_scene_session_update(session);
    assert(missing.has_fatal && !editor_scene_session_active_check(session));
    mode_set("normal");
    OK(editor_scene_session_launch(session, argv[1]));
    wait_state(session, EDITOR_SCENE_RUNNING);
    EditorSceneFrame first = wait_frame(session, 0);
    if(argc == 2) {
        assert(first.width == 320 && first.height == 180 && first.pitch == 1280);
        EditorSceneInputMode input = editor_scene_session_input_mode_get(session);
        assert(input.text_active && input.relative_mouse && input.text_cursor == 4 && input.text_area.x == 10);
        OK(editor_scene_session_focus_set(session, true));
        SDL_Event event; EVENT(event, SDL_EVENT_KEY_DOWN); event.key.scancode = SDL_SCANCODE_A;
        OK(editor_scene_session_input_add(session, &event)); wait_log(session, "KEY_DOWN");
        OK(editor_scene_session_focus_set(session, false)); wait_log(session, "KEY_RELEASED");
        assert(error_check(editor_scene_session_input_add(session, &event)));
        EditorSceneFrame unfocused = wait_frame(session, first.sequence + 2);
        assert(unfocused.time > first.time); /* Focus loss keeps running. */
        OK(editor_scene_session_focus_set(session, true));
        EVENT(event, SDL_EVENT_MOUSE_MOTION); event.motion.x = 20; event.motion.y = 20; event.motion.xrel = 3;
        OK(editor_scene_session_input_add(session, &event)); wait_log(session, "RELATIVE"); wait_log(session, "UI_HOVERED");
        EVENT(event, SDL_EVENT_MOUSE_BUTTON_DOWN); event.button.button = SDL_BUTTON_LEFT; event.button.x = 20; event.button.y = 20;
        OK(editor_scene_session_input_add(session, &event)); wait_log(session, "UI_PRESSED");
        event.type = SDL_EVENT_MOUSE_BUTTON_UP; OK(editor_scene_session_input_add(session, &event));
        EVENT(event, SDL_EVENT_MOUSE_WHEEL); event.wheel.y = 2; event.wheel.mouse_x = 20; event.wheel.mouse_y = 20;
        OK(editor_scene_session_input_add(session, &event)); wait_log(session, "WHEEL");
        EVENT(event, SDL_EVENT_TEXT_INPUT); event.text.text = "hello";
        OK(editor_scene_session_input_add(session, &event)); wait_log(session, "TEXT:hello");
        EVENT(event, SDL_EVENT_TEXT_EDITING); event.edit.text = "preedit";
        OK(editor_scene_session_input_add(session, &event)); wait_log(session, "EDIT:preedit");
    }
    OK(editor_scene_session_pause_set(session, true)); wait_state(session, EDITOR_SCENE_PAUSED);
    EditorSceneFrame paused = editor_scene_session_frame_get(session);
    uint64_t deadline = SDL_GetTicks() + 250;
    while(SDL_GetTicks() < deadline) { poll_session(session, false); SDL_Delay(1); }
    assert(editor_scene_session_frame_get(session).sequence == paused.sequence);
    OK(editor_scene_session_pause_set(session, false)); wait_state(session, EDITOR_SCENE_RUNNING);
    EditorSceneFrame resumed = wait_frame(session, paused.sequence);
    assert(resumed.time >= paused.time && resumed.time - paused.time < .2);
    editor_scene_session_stop(session); wait_state(session, EDITOR_SCENE_STOPPED);
    assert(!editor_scene_session_active_check(session));
    if(argc > 2) { editor_scene_session_destroy(session); return 0; }
    assert(strstr(editor_scene_session_log_get(session), "SHUTDOWN"));
    const char *failures[] = {"init_fail", "update_fail", "bad_token", "bad_version", "oversize", "exit"};
    for(size_t i = 0; i < SDL_arraysize(failures); ++i) {
        mode_set(failures[i]); OK(editor_scene_session_launch(session, argv[1]));
        bool failed = false; Report copy = report_create();
        deadline = SDL_GetTicks() + 10000;
        while(editor_scene_session_active_check(session) && SDL_GetTicks() < deadline) {
            Report report = poll_session(session, true);
            if(report.has_fatal) { failed = true; copy = report; }
            SDL_Delay(1);
        }
        assert(failed && !editor_scene_session_active_check(session));
        assert(editor_scene_session_state_get(session) == EDITOR_SCENE_FAILED);
        assert(editor_scene_session_update(session).header.status == ROHR_REPORT_NOT_RUN);
        assert(copy.has_fatal && report_valid_check(&copy));
        if(i < 2) assert(strstr(editor_scene_session_log_get(session), "SHUTDOWN"));
    }
    mode_set("logs"); OK(editor_scene_session_launch(session, argv[1])); wait_state(session, EDITOR_SCENE_RUNNING);
    assert(editor_scene_session_log_dropped_get(session) > 0);
    editor_scene_session_stop(session); wait_state(session, EDITOR_SCENE_STOPPED);
    mode_set("hang"); OK(editor_scene_session_launch(session, argv[1])); editor_scene_session_stop(session);
    deadline = SDL_GetTicks() + 5000; bool killed = false;
    while(editor_scene_session_active_check(session) && SDL_GetTicks() < deadline) {
        Report report = poll_session(session, true); killed |= report.has_fatal; SDL_Delay(1);
    }
    assert(killed && !editor_scene_session_active_check(session));
    /* A developer-owned scene without the runner cannot hang startup forever. */
    OK(editor_scene_session_launch(session, argv[1]));
    deadline = SDL_GetTicks() + 15000; bool timeout = false;
    while(editor_scene_session_active_check(session) && SDL_GetTicks() < deadline) {
        Report report = poll_session(session, true);
        if(report.has_fatal && strstr(report.first_fatal.detail, "startup timed out")) timeout = true;
        SDL_Delay(1);
    }
    assert(timeout && !editor_scene_session_active_check(session));
    mode_set("large"); OK(editor_scene_session_launch(session, argv[1])); wait_state(session, EDITOR_SCENE_RUNNING);
    first = wait_frame(session, 0); assert(first.width == 1920 && first.height == 1080);
    /* Withhold host reads: backpressure must preserve bounded storage and controls. */
    SDL_Delay(100);
    OK(editor_scene_session_pause_set(session, true)); wait_state(session, EDITOR_SCENE_PAUSED);
    OK(editor_scene_session_pause_set(session, false)); wait_state(session, EDITOR_SCENE_RUNNING);
    uint64_t start = SDL_GetTicksNS(), bytes = 0, sequence = editor_scene_session_frame_get(session).sequence;
    for(unsigned i = 0; i < 20; ++i) {
        EditorSceneFrame frame = wait_frame(session, sequence); sequence = frame.sequence; bytes += (uint64_t)frame.pitch * frame.height;
    }
    double seconds = (SDL_GetTicksNS() - start) / 1e9;
    printf("scene transport: 1920x1080 %.1f fps %.1f MiB/s; maximum poll %.3f ms\n", 20 / seconds, bytes / seconds / 1048576, poll_peak);
    editor_scene_session_stop(session); wait_state(session, EDITOR_SCENE_STOPPED);
    mode_set("normal"); OK(editor_scene_session_launch(session, argv[1])); wait_state(session, EDITOR_SCENE_RUNNING);
    OK(editor_scene_session_focus_set(session, true));
    SDL_Event event; EVENT(event, SDL_EVENT_KEY_UP); event.key.scancode = SDL_SCANCODE_A;
    bool overflow = false;
    for(unsigned i = 0; i < 257; ++i) if(error_check(editor_scene_session_input_add(session, &event))) { overflow = true; break; }
    assert(overflow);
    deadline = SDL_GetTicks() + 5000;
    while(editor_scene_session_active_check(session) && SDL_GetTicks() < deadline) { poll_session(session, true); SDL_Delay(1); }
    assert(!editor_scene_session_active_check(session));
    /* Destroy an active child, including graceful callback cleanup and reap. */
    OK(editor_scene_session_launch(session, argv[1])); wait_state(session, EDITOR_SCENE_RUNNING);
    editor_scene_session_destroy(session);
    return 0;
}
