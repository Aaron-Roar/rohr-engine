/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "graphics/scene_image.h"
#include "panels/editor_scene_panel.h"
#include "input/input_internal.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

float editor_viewport_width=800, editor_window_width=1000, editor_window_height=720,
    editor_viewport_bottom=600, editor_viewport_top=49;
static EditorScenePanel panel;
static EditorNotificationPanel notifications;
static EditorTerminalPanel terminal;
static const SDL_FRect viewport = {0,49,800,551};
/* The bundled dummy backend supports keyboard focus without a desktop. */
extern bool SDL_SetKeyboardFocus(SDL_Window *window);
static void poll_scene(void) {
    (void)editor_scene_session_update(panel.session);
    editor_scene_panel_update(&panel);
}
static void wait_state(EditorSceneState state) {
    uint64_t deadline = SDL_GetTicks() + 10000;
    while(editor_scene_session_state_get(panel.session) != state && SDL_GetTicks() < deadline) {
        poll_scene(); SDL_Delay(1);
    }
    assert(editor_scene_session_state_get(panel.session) == state);
}
static EditorSceneAction frame(Position point, MouseButtonState button) {
    rohr_ui_input_set((UIInput){.pointer=point, .primary_button=button});
    EditorSceneAction action = editor_scene_panel_draw(&panel, viewport, false);
    rohr_ui_update(); assert(graphics_editor_show().header.status != ROHR_REPORT_FAILED);
    return action;
}
static EditorSceneAction tab_event(Uint32 type, Position point, bool blocked) {
    SDL_FPoint window = graphics_scene_window_point_get((SDL_FPoint){point.x, point.y});
    SDL_Event event = {.type = type};
    if(type == SDL_EVENT_MOUSE_MOTION) { event.motion.x = window.x; event.motion.y = window.y; }
    else { event.button.button = SDL_BUTTON_LEFT; event.button.x = window.x; event.button.y = window.y; }
    EditorSceneAction action;
    (void)editor_scene_panel_tabs_event_add(&panel, &event, viewport, blocked, &action);
    return action;
}
static EditorSceneAction click(Position point) {
    if(point.y < viewport.y) {
        (void)tab_event(SDL_EVENT_MOUSE_BUTTON_DOWN, point, false);
        EditorSceneAction action = tab_event(SDL_EVENT_MOUSE_BUTTON_UP, point, false);
        (void)frame(point, MOUSE_BUTTON_STATE_UP);
        return action;
    }
    EditorSceneAction first = frame(point, MOUSE_BUTTON_STATE_PRESSED);
    EditorSceneAction second = frame(point, MOUSE_BUTTON_STATE_RELEASED);
    (void)frame(point, MOUSE_BUTTON_STATE_UP);
    return second == EDITOR_SCENE_ACTION_NONE ? first : second;
}
static Report warning_report(void) {
    Report report = report_create(); report.header.id = ROHR_SCENE_SESSION_REPORT_ID;
    ReportDiagnostic d = {.report_id=ROHR_REPORT_CUSTOM_ID_BEGIN, .internal_stage=5,
        .severity=ROHR_REPORT_SEVERITY_WARNING, .code=ERROR_ENGINE_STATE_INVALID,
        .recovery=ROHR_REPORT_CONTRIBUTION_SKIPPED, .occurrence_count=2,
        .source={ROHR_REPORT_OBJECT_ENTITY,123}};
    strcpy(d.detail, "retained issue");
    assert(!rohr_error_check(report_diagnostic_add(&report,d))); return report;
}
int main(int argc, char **argv) {
    assert(argc == 2);
    assert(!rohr_error_check(rohr_engine_start())); assert(!rohr_error_check(rohr_graphics_start()));
    FontAsset font=rohr_graphics_font_default_get();
    assert(editor_notification_panel_create(&notifications,&font,&font));
    assert(editor_scene_panel_create(&panel,&font,&terminal,&notifications));
    assert(EDITOR_SCENE_TAB_HEIGHT == 15 && EDITOR_SCENE_CONTROLS_HEIGHT == 36);
    assert(panel.tab_visible[0] && panel.tab_visible[1] && panel.active_tab == EDITOR_SCENE_TAB_EDITOR);
    assert(panel.tab_order[0] == EDITOR_SCENE_TAB_EDITOR);
    assert(click((Position){200,41}) == EDITOR_SCENE_ACTION_NONE && panel.active_tab == EDITOR_SCENE_TAB_SIMULATION);
    /* Close never selects its tab; View uses the same visibility state. */
    assert(click((Position){93,41}) == EDITOR_SCENE_ACTION_NONE);
    assert(!panel.tab_visible[0] && panel.active_tab == EDITOR_SCENE_TAB_SIMULATION);
    assert(click((Position){153,41}) == EDITOR_SCENE_ACTION_STOP);
    assert(!panel.tab_visible[1] && panel.active_tab == EDITOR_SCENE_TAB_NONE);
    assert(click((Position){50,41}) == EDITOR_SCENE_ACTION_NONE && panel.active_tab == EDITOR_SCENE_TAB_NONE);
    assert(editor_scene_panel_tab_visible_set(&panel, EDITOR_SCENE_TAB_EDITOR, true) == EDITOR_SCENE_ACTION_STOP);
    assert(panel.active_tab == EDITOR_SCENE_TAB_EDITOR);
    assert(editor_scene_panel_tab_visible_set(&panel, EDITOR_SCENE_TAB_SIMULATION, true) == EDITOR_SCENE_ACTION_NONE);
    /* A sub-threshold move still selects; dragging does not change the active identity. */
    (void)tab_event(SDL_EVENT_MOUSE_BUTTON_DOWN, (Position){50,41}, false);
    (void)tab_event(SDL_EVENT_MOUSE_MOTION, (Position){52,41}, false);
    assert(!panel.tab_dragging);
    assert(tab_event(SDL_EVENT_MOUSE_BUTTON_UP, (Position){52,41}, false) == EDITOR_SCENE_ACTION_EDITOR);
    (void)tab_event(SDL_EVENT_MOUSE_BUTTON_DOWN, (Position){50,41}, false);
    (void)tab_event(SDL_EVENT_MOUSE_MOTION, (Position){240,120}, false);
    assert(panel.tab_dragging && panel.drop_before == EDITOR_SCENE_TAB_NONE);
    (void)frame((Position){240,120}, MOUSE_BUTTON_STATE_UP);
    (void)tab_event(SDL_EVENT_MOUSE_BUTTON_UP, (Position){240,120}, false);
    assert(panel.tab_order[0] == EDITOR_SCENE_TAB_SIMULATION && panel.active_tab == EDITOR_SCENE_TAB_EDITOR);
    (void)tab_event(SDL_EVENT_MOUSE_BUTTON_DOWN, (Position){200,41}, false);
    (void)tab_event(SDL_EVENT_MOUSE_MOTION, (Position){20,41}, false);
    assert(panel.drop_before == EDITOR_SCENE_TAB_SIMULATION);
    (void)tab_event(SDL_EVENT_MOUSE_BUTTON_UP, (Position){20,41}, false);
    assert(panel.tab_order[0] == EDITOR_SCENE_TAB_EDITOR);
    /* Close dragging, overlays, focus loss, and Escape must not reorder or select. */
    (void)tab_event(SDL_EVENT_MOUSE_BUTTON_DOWN, (Position){93,41}, false);
    (void)tab_event(SDL_EVENT_MOUSE_MOTION, (Position){240,41}, false);
    assert(!panel.tab_dragging);
    (void)tab_event(SDL_EVENT_MOUSE_BUTTON_UP, (Position){240,41}, false);
    assert(panel.tab_visible[0] && panel.tab_order[0] == EDITOR_SCENE_TAB_EDITOR);
    for(int cancellation = 0; cancellation < 3; ++cancellation) {
        (void)tab_event(SDL_EVENT_MOUSE_BUTTON_DOWN, (Position){50,41}, false);
        (void)tab_event(SDL_EVENT_MOUSE_MOTION, (Position){240,41}, false);
        SDL_Event cancel = {.type = cancellation == 1 ? SDL_EVENT_WINDOW_FOCUS_LOST : SDL_EVENT_KEY_DOWN};
        cancel.key.scancode = SDL_SCANCODE_ESCAPE;
        EditorSceneAction ignored;
        (void)editor_scene_panel_tabs_event_add(&panel, &cancel, viewport, cancellation == 0, &ignored);
        assert(panel.pressed_tab == EDITOR_SCENE_TAB_NONE && !panel.tab_dragging);
        (void)tab_event(SDL_EVENT_MOUSE_BUTTON_UP, (Position){240,41}, false);
        assert(panel.tab_order[0] == EDITOR_SCENE_TAB_EDITOR);
    }
    assert(click((Position){200,41}) == EDITOR_SCENE_ACTION_NONE && panel.active_tab == EDITOR_SCENE_TAB_SIMULATION);
    assert(click((Position){60,65}) == EDITOR_SCENE_ACTION_RUN);
    assert(click((Position){170,65}) == EDITOR_SCENE_ACTION_STOP);
    Report warning=warning_report();
    editor_scene_panel_report(&panel,&warning);
    uint64_t id=panel.groups[0].notification;
    uint64_t created=notifications.store.toast_created_ms[0];
    editor_scene_panel_report(&panel,&warning);
    assert(panel.group_count==1 && panel.groups[0].diagnostic.occurrence_count==4);
    assert(notifications.store.entry_count==1 && notifications.store.toast_count==1 &&
        notifications.store.toast_created_ms[0]==created && panel.groups[0].notification==id);
    assert(strstr(editor_notification_store_id_get(&notifications.store,id)->detail,"Occurrences: 4"));
    for(unsigned i=1;i<EDITOR_SCENE_DIAGNOSTIC_GROUPS+4;++i) {
        warning.diagnostics[0].source.id=i+123;
        editor_scene_panel_report(&panel,&warning);
    }
    assert(panel.omitted==8 && panel.overflow_notification);
    warning.diagnostics[0].severity=ROHR_REPORT_SEVERITY_FATAL;
    strcpy(warning.diagnostics[0].detail,"fatal survives capacity");
    warning.has_fatal=true; warning.first_fatal=warning.diagnostics[0];
    editor_scene_panel_report(&panel,&warning);
    assert(panel.groups[EDITOR_SCENE_DIAGNOSTIC_GROUPS-1].diagnostic.severity==ROHR_REPORT_SEVERITY_FATAL);
    assert(panel.omitted==10);
    editor_scene_panel_output(&panel,"\xe2",1); assert(panel.utf8_count==1);
    editor_scene_panel_output(&panel,"\x82\xac",2); assert(panel.utf8_count==0);
    editor_scene_panel_output(&panel,"\xe2x",2); assert(panel.utf8_count==0);
    size_t retained=notifications.store.entry_count;
    editor_scene_panel_output(&panel,"arbitrary error warning text\n",29);
    assert(notifications.store.entry_count==retained);
    EditorSceneSession *session=editor_scene_session_create(); assert(session);
    editor_scene_panel_attach(&panel,session);
    editor_scene_panel_run_begin(&panel);
    assert(!rohr_error_check(editor_scene_session_launch(session,argv[1])));
    wait_state(EDITOR_SCENE_RUNNING);
    uint64_t deadline=SDL_GetTicks()+10000;
    while(!editor_scene_session_frame_get(session).pixels && SDL_GetTicks()<deadline) { poll_scene(); SDL_Delay(1); }
    assert(panel.focused);
    (void)frame((Position){400,350},MOUSE_BUTTON_STATE_UP);
    assert(panel.image && panel.sequence);
    int window_count;
    SDL_Window **windows=SDL_GetWindows(&window_count); assert(windows && window_count==1);
    SDL_Window *host_window=windows[0]; SDL_free(windows);
    assert(SDL_SetKeyboardFocus(host_window));
    editor_scene_panel_input_mode_apply(&panel,viewport);
    assert(SDL_TextInputActive(host_window));
    SDL_Rect input_area; int cursor;
    assert(SDL_GetTextInputArea(host_window,&input_area,&cursor));
    assert(input_area.w>0 && input_area.h>0 && cursor>0);
    SDL_Event event={.type=SDL_EVENT_KEY_DOWN}; event.key.scancode=SDL_SCANCODE_A;
    input_frame_begin();
    assert(editor_scene_panel_event_add(&panel,&event,viewport,false));
    assert(!input_key_down_check(SDL_SCANCODE_A));
    event.key.scancode=SDL_SCANCODE_ESCAPE;
    assert(editor_scene_panel_event_add(&panel,&event,viewport,false) && !panel.focused);
    assert(!SDL_TextInputActive(host_window));
    event=(SDL_Event){.type=SDL_EVENT_MOUSE_BUTTON_DOWN}; event.button.button=SDL_BUTTON_LEFT;
    /* Convert a known logical editor point to window coordinates. */
    SDL_FPoint window=graphics_scene_window_point_get((SDL_FPoint){400,350});
    event.button.x=window.x; event.button.y=window.y;
    assert(editor_scene_panel_event_add(&panel,&event,viewport,false));
    event.type=SDL_EVENT_MOUSE_BUTTON_UP;
    SDL_FPoint strip = graphics_scene_window_point_get((SDL_FPoint){50,41});
    event.button.x=strip.x; event.button.y=strip.y;
    EditorSceneAction tab_action;
    assert(!editor_scene_panel_tabs_event_add(&panel,&event,viewport,false,&tab_action));
    assert(editor_scene_panel_event_add(&panel,&event,viewport,false));
    assert(!panel.captured_buttons);
    /* Tab gestures are consumed while the child runs, including motion/release over its image. */
    assert(panel.focused);
    event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.x = strip.x; event.button.y = strip.y;
    assert(editor_scene_panel_tabs_event_add(&panel,&event,viewport,false,&tab_action));
    assert(!panel.focused && panel.active_tab == EDITOR_SCENE_TAB_SIMULATION);
    event.type = SDL_EVENT_MOUSE_MOTION; event.motion.x = window.x; event.motion.y = window.y;
    assert(editor_scene_panel_tabs_event_add(&panel,&event,viewport,false,&tab_action));
    event.type = SDL_EVENT_MOUSE_BUTTON_UP; event.button.button = SDL_BUTTON_LEFT;
    event.button.x = window.x; event.button.y = window.y;
    assert(editor_scene_panel_tabs_event_add(&panel,&event,viewport,false,&tab_action));
    assert(tab_action == EDITOR_SCENE_ACTION_NONE && panel.active_tab == EDITOR_SCENE_TAB_SIMULATION);
    assert(!rohr_input_mouse_button_pressed_check(INPUT_MOUSE_BUTTON_LEFT));
    /* Restore tab order after the isolated drag for subsequent control checks. */
    panel.tab_order[0] = EDITOR_SCENE_TAB_EDITOR; panel.tab_order[1] = EDITOR_SCENE_TAB_SIMULATION;
    event.type=SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.x=-20; event.button.y=-20;
    assert(!editor_scene_panel_event_add(&panel,&event,viewport,false) && !panel.focused);
    event.button.x=window.x; event.button.y=window.y;
    assert(editor_scene_panel_event_add(&panel,&event,viewport,false) && panel.focused);
    assert(!editor_scene_panel_event_add(&panel,&event,viewport,true) && !panel.focused);
    assert(click((Position){60,65})==EDITOR_SCENE_ACTION_NONE);
    wait_state(EDITOR_SCENE_PAUSED);
    uint64_t sequence=panel.sequence;
    (void)click((Position){60,65}); wait_state(EDITOR_SCENE_RUNNING);
    assert(panel.focused && !panel.cleared);
    assert(click((Position){50,41})==EDITOR_SCENE_ACTION_EDITOR && panel.active_tab == EDITOR_SCENE_TAB_EDITOR);
    editor_scene_panel_stop(&panel);
    assert(!panel.image && panel.cleared && !panel.focused && panel.tab_visible[EDITOR_SCENE_TAB_SIMULATION]);
    wait_state(EDITOR_SCENE_STOPPED);
    assert(sequence>0);
    /* Hiding the simulation from View or x stops the child, including while paused. */
    for(int paused = 0; paused < 2; ++paused) {
        (void)editor_scene_panel_tab_visible_set(&panel, EDITOR_SCENE_TAB_SIMULATION, false);
        (void)editor_scene_panel_tab_visible_set(&panel, EDITOR_SCENE_TAB_SIMULATION, true);
        assert(editor_scene_session_state_get(session) == EDITOR_SCENE_STOPPED && panel.cleared);
        editor_scene_panel_run_begin(&panel);
        assert(!rohr_error_check(editor_scene_session_launch(session,argv[1])));
        wait_state(EDITOR_SCENE_RUNNING);
        if(paused) { (void)click((Position){60,65}); wait_state(EDITOR_SCENE_PAUSED); }
        EditorSceneAction action = paused ? click((Position){253,41}) :
            editor_scene_panel_tab_visible_set(&panel, EDITOR_SCENE_TAB_SIMULATION, false);
        assert(action == EDITOR_SCENE_ACTION_STOP && panel.active_tab == EDITOR_SCENE_TAB_EDITOR);
        editor_scene_panel_stop(&panel);
        assert(panel.cleared && !panel.focused && !panel.focus_on_running);
        wait_state(EDITOR_SCENE_STOPPED);
    }
    /* Project transitions preserve the layout, but prefer Editor on opening. */
    panel.tab_order[0] = EDITOR_SCENE_TAB_SIMULATION; panel.tab_order[1] = EDITOR_SCENE_TAB_EDITOR;
    (void)editor_scene_panel_tab_visible_set(&panel, EDITOR_SCENE_TAB_SIMULATION, true);
    editor_scene_panel_project_reset(&panel);
    assert(panel.active_tab == EDITOR_SCENE_TAB_EDITOR && panel.tab_order[0] == EDITOR_SCENE_TAB_SIMULATION);
    (void)editor_scene_panel_tab_visible_set(&panel, EDITOR_SCENE_TAB_EDITOR, false);
    editor_scene_panel_project_reset(&panel);
    assert(panel.active_tab == EDITOR_SCENE_TAB_SIMULATION && !panel.tab_visible[0]);
    (void)editor_scene_panel_tab_visible_set(&panel, EDITOR_SCENE_TAB_SIMULATION, false);
    editor_scene_panel_project_reset(&panel);
    assert(panel.active_tab == EDITOR_SCENE_TAB_NONE);
    editor_scene_session_destroy(session); editor_scene_panel_attach(&panel,NULL);
    /* Queued image draws retain resources through owner destruction and resize. */
    GraphicsSceneImage *image=graphics_scene_image_create(); assert(image);
    unsigned char pixels[64]={0};
    assert(!rohr_error_check(graphics_scene_image_update(image,pixels,4,4,16)));
    assert(!rohr_error_check(graphics_scene_image_draw(image,(SDL_FRect){0,0,20,20})));
    assert(rohr_error_check(graphics_scene_image_update(image,pixels,2,2,8)));
    assert(graphics_editor_show().header.status != ROHR_REPORT_FAILED);
    assert(!rohr_error_check(graphics_scene_image_update(image,pixels,2,2,8)));
    assert(!rohr_error_check(graphics_scene_image_draw(image,(SDL_FRect){0,0,20,20})));
    graphics_scene_image_destroy(image);
    assert(graphics_editor_show().header.status != ROHR_REPORT_FAILED);
    editor_scene_panel_destroy(&panel);
    assert(editor_scene_panel_create(&panel,&font,&terminal,&notifications));
    assert(panel.tab_visible[0] && panel.tab_visible[1] && panel.tab_order[0] == EDITOR_SCENE_TAB_EDITOR);
    assert(panel.active_tab == EDITOR_SCENE_TAB_EDITOR);
    editor_scene_panel_destroy(&panel); editor_notification_panel_destroy(&notifications);
    rohr_graphics_stop(); rohr_engine_stop(); return 0;
}
