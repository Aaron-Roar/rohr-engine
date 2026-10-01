/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editors/soft_body/editor_soft_area.h"
#include <editor_soft_area.h>
#include "editor_navigation.h"
#include "editor_layout.h"
#include <assert.h>

float editor_viewport_width = 1024.0f;
float editor_window_width = 1280.0f;
float editor_window_height = 720.0f;
float editor_viewport_bottom = 720.0f;

static void panel(EditorSoftAreaEditor *ui, EditorModeContext *context,
        bool list, Position pointer, MouseButtonState button) {
    rohr_ui_frame_begin((UIInput){pointer, button});
    editor_mode_accordion_layout_measure_reset();
    if(list) (void)editor_soft_area_list_draw(ui, context,
        &context->project->objects[0].soft_body_items[0], 80);
    else (void)editor_soft_area_editor_draw(ui, context);
    rohr_ui_frame_end();
}
static void click(EditorSoftAreaEditor *ui, EditorModeContext *context,
        bool list, Position pointer) {
    panel(ui, context, list, pointer, MOUSE_BUTTON_STATE_PRESSED);
    panel(ui, context, list, pointer, MOUSE_BUTTON_STATE_RELEASED);
    panel(ui, context, list, pointer, MOUSE_BUTTON_STATE_UP);
}
static bool viewport(EditorProject *project, EditorViewportState *state,
        Position pointer, MouseButtonState button) {
    return editor_viewport_update(state, project, pointer, button,
        MOUSE_BUTTON_STATE_UP, false, 0, false);
}
int main(void) {
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    FontAsset font = rohr_graphics_font_default_get();
    EditorSoftAreaEditor ui;
    assert(editor_soft_area_editor_create(&ui, &font));
    ui.appearance_section.expanded = false;
    EditorProject project;
    EditorViewportState state;
    editor_project_init(&project);
    editor_viewport_state_init(&state);
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
        .data.item_add = {.kind = EDITOR_ITEM_SOFT_BODY, .object = object->id}};
    assert(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    EditorSoftBody *body = &object->soft_body_items[0];
    state.selected_soft_body = body->id;
    state.mode = EDITOR_VIEWPORT_SOFT_BODY;
    EditorModeContext context = {.project = &project, .viewport = &state, .x = 0, .width = 400};
    /* The accordion's add button opens a draft; a single row click only selects. */
    click(&ui, &context, true, (Position){100,128});
    assert(body->area_count == 2 && state.mode == EDITOR_VIEWPORT_SOFT_AREA);
    EditorSoftAreaId area_id = state.selected_soft_area;
    EditorSoftArea *area = editor_soft_area_get(body, area_id);
    assert(area != NULL && area->outer.node_count == 0);
    rohr_ui_field_focus_clear();
    state.mode = EDITOR_VIEWPORT_SOFT_BODY;
    click(&ui, &context, true, (Position){100,192});
    assert(state.mode == EDITOR_VIEWPORT_SOFT_BODY && state.selection == EDITOR_SELECTION_SOFT_AREA);
    assert(editor_navigation_selected_open(&project, &state));
    /* Collapsed appearance leaves the picker box at y=184. */
    click(&ui, &context, false, (Position){100,200});
    assert(state.soft_area_picking);
    project.viewport_local_view = false;
    project.viewport_camera_zoom = 1;
    Position before = body->nodes[0].position;
    Position pointer = {512 + before.x,
        EDITOR_MENU_HEIGHT + (720 - EDITOR_MENU_HEIGHT) * 0.5f - before.y};
    assert(viewport(&project, &state, pointer, MOUSE_BUTTON_STATE_PRESSED));
    assert(area->outer.node_count == 1 && area->outer.nodes[0] == body->nodes[0].id);
    assert(viewport(&project, &state, (Position){pointer.x + 30, pointer.y + 25}, MOUSE_BUTTON_STATE_DOWN));
    assert(viewport(&project, &state, pointer, MOUSE_BUTTON_STATE_RELEASED));
    assert(state.mode == EDITOR_VIEWPORT_SOFT_AREA && !state.dragged_soft_node && !state.dragged_soft_body);
    assert(body->nodes[0].position.x == before.x && body->nodes[0].position.y == before.y);
    assert(editor_viewport_soft_area_node_toggle(&project, &state, body->nodes[1].id));
    /* Reorder and remove operate on membership, leaving the physical nodes untouched. */
    click(&ui, &context, false, (Position){332,230});
    assert(area->outer.nodes[0] == body->nodes[1].id);
    click(&ui, &context, false, (Position){370,230});
    assert(area->outer.node_count == 1 && body->node_count == 4);
    click(&ui, &context, false, (Position){100,200});
    assert(!state.soft_area_picking);
    /* Outside picking a node opens its own editor without changing the loop. */
    assert(viewport(&project, &state, pointer, MOUSE_BUTTON_STATE_PRESSED));
    assert(state.mode == EDITOR_VIEWPORT_SOFT_NODE && area->outer.node_count == 1);
    (void)viewport(&project, &state, pointer, MOUSE_BUTTON_STATE_RELEASED);
    EditorSelectionRef ref = {EDITOR_SELECTION_SOFT_AREA, object->id, body->id, 0, area_id};
    assert(editor_viewport_selection_set(&project, &state, ref, false));
    assert(editor_navigation_selected_open(&project, &state));
    /* One node row puts Add Hole after the draft status at y=284. */
    click(&ui, &context, false, (Position){100,297});
    assert(area->hole_count == 1 && state.mode == EDITOR_VIEWPORT_SOFT_HOLE);
    assert(!state.soft_area_picking);
    rohr_ui_field_focus_clear();
    click(&ui, &context, false, (Position){100,126});
    assert(state.soft_area_picking);
    assert(viewport(&project, &state, pointer, MOUSE_BUTTON_STATE_PRESSED));
    assert(viewport(&project, &state, pointer, MOUSE_BUTTON_STATE_RELEASED));
    assert(area->holes[0].loop.node_count == 1 && area->outer.node_count == 1);
    editor_viewport_back(&state);
    assert(!state.soft_area_picking && state.mode == EDITOR_VIEWPORT_SOFT_HOLE);
    editor_viewport_back(&state);
    assert(state.mode == EDITOR_VIEWPORT_SOFT_AREA && state.selected_item_count == 0);
    editor_soft_area_editor_destroy(&ui);
    editor_viewport_state_destroy(&state);
    editor_project_destroy(&project);
    editor_viewport_assets_destroy();
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
