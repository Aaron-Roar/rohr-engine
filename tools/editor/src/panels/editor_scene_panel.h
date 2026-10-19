/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_SCENE_PANEL_H
#define ROHR_EDITOR_SCENE_PANEL_H
#include "editor_scene_session.h"
#include "editor_notification_panel.h"
#include "editor_terminal_panel.h"
#include "graphics/scene_image.h"

#define EDITOR_SCENE_TAB_HEIGHT 15.0f
#define EDITOR_SCENE_CONTROLS_HEIGHT 36.0f
#define EDITOR_SCENE_DIAGNOSTIC_GROUPS 100
typedef struct EditorSceneDiagnosticGroup {
    ReportDiagnostic diagnostic;
    uint64_t notification;
} EditorSceneDiagnosticGroup;
typedef enum EditorSceneAction {
    EDITOR_SCENE_ACTION_NONE, EDITOR_SCENE_ACTION_RUN,
    EDITOR_SCENE_ACTION_STOP, EDITOR_SCENE_ACTION_EDITOR
} EditorSceneAction;
typedef enum EditorSceneTab {
    EDITOR_SCENE_TAB_EDITOR, EDITOR_SCENE_TAB_SIMULATION, EDITOR_SCENE_TAB_COUNT,
    EDITOR_SCENE_TAB_NONE = EDITOR_SCENE_TAB_COUNT
} EditorSceneTab;
typedef struct EditorScenePanel {
    bool tab_visible[EDITOR_SCENE_TAB_COUNT];
    EditorSceneTab tab_order[EDITOR_SCENE_TAB_COUNT], active_tab;
    EditorSceneTab pressed_tab, hovered_tab, drop_before;
    Position tab_press;
    bool close_pressed, close_hovered, tab_dragging;
    bool focused, focus_on_running, control_pending;
    bool cleared, image_failed;
    uint32_t captured_buttons;
    EditorSceneState previous_state;
    EditorSceneSession *session; /* borrowed; must outlive attachment */
    EditorTerminalPanel *terminal;
    EditorNotificationPanel *notifications;
    GraphicsSceneImage *image;
    uint64_t sequence, omitted, overflow_notification;
    size_t group_count;
    EditorSceneDiagnosticGroup groups[EDITOR_SCENE_DIAGNOSTIC_GROUPS];
    unsigned char utf8[4];
    size_t utf8_count, utf8_expected;
    TextAsset editor_label, scene_label, run_label, pause_label, resume_label,
        stop_label, waiting_label, release_label, close_label, empty_label;
} EditorScenePanel;
bool editor_scene_panel_create(EditorScenePanel *panel, const FontAsset *font,
    EditorTerminalPanel *terminal, EditorNotificationPanel *notifications);
/* Session-only layout; project transitions preserve visibility and order. */
void editor_scene_panel_project_reset(EditorScenePanel *panel);
/* Caller handles STOP by also cancelling simulation-owned build preparation. */
EditorSceneAction editor_scene_panel_tab_visible_set(EditorScenePanel *panel,
    EditorSceneTab tab, bool visible);
/* Consumes tab pointer gestures before either viewport receives them. */
bool editor_scene_panel_tabs_event_add(EditorScenePanel *panel, const SDL_Event *event,
    SDL_FRect viewport, bool blocked, EditorSceneAction *action);
void editor_scene_panel_destroy(EditorScenePanel *panel);
void editor_scene_panel_attach(EditorScenePanel *panel, EditorSceneSession *session);
void editor_scene_panel_run_begin(EditorScenePanel *panel);
void editor_scene_panel_stop(EditorScenePanel *panel);
void editor_scene_panel_update(EditorScenePanel *panel);
void editor_scene_panel_focus_set(EditorScenePanel *panel, bool focused);
/* Returns true only for events routed to the scene. Blocked overlays release focus. */
bool editor_scene_panel_event_add(EditorScenePanel *panel, const SDL_Event *event,
    SDL_FRect viewport, bool blocked);
void editor_scene_panel_input_mode_apply(EditorScenePanel *panel, SDL_FRect viewport);
EditorSceneAction editor_scene_panel_draw(EditorScenePanel *panel,
    SDL_FRect viewport, bool building);
void editor_scene_panel_output(void *context, const char *bytes, size_t count);
void editor_scene_panel_report(void *context, const Report *report);
#endif
