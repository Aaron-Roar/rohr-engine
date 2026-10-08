/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editors/physics/editor_center_of_mass.h"
#include "editors/editor_mode_controls.h"
#include "editor_mass_properties.h"
#include "editor_navigation.h"
#include "editor_layout.h"
#include <assert.h>
#include <math.h>

float editor_viewport_width = 1024, editor_window_width = 1280;
float editor_window_height = 720, editor_viewport_bottom = 720;
static void begin(const EditorProject *project, const EditorCommand *command, void *context) {
    editor_history_command_begin(context, project, command);
}
static void finish(const EditorCommand *command, const EditorCommandResult *result, void *context) {
    editor_history_command_finish(context, command, result);
}
static void panel(EditorCenterOfMassEditor *ui, EditorModeContext *context,
        bool inline_panel, Position pointer, MouseButtonState button) {
    rohr_ui_frame_begin((UIInput){pointer,button});
    if(inline_panel) {
        EditorObject *object = editor_project_selected_get(context->project);
        (void)editor_center_of_mass_editor_draw(ui,context,object,
            editor_project_rigid_body_get(object,context->viewport->selected_rigid_body),42);
    } else (void)editor_center_of_mass_panel_draw(ui,context);
    rohr_ui_frame_end();
}
static void click(EditorCenterOfMassEditor *ui, EditorModeContext *context,
        bool inline_panel, Position pointer) {
    panel(ui,context,inline_panel,pointer,MOUSE_BUTTON_STATE_PRESSED);
    panel(ui,context,inline_panel,pointer,MOUSE_BUTTON_STATE_RELEASED);
    panel(ui,context,inline_panel,pointer,MOUSE_BUTTON_STATE_UP);
}
int main(void) {
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    FontAsset font = rohr_graphics_font_default_get();
    EditorCenterOfMassEditor ui;
    assert(editor_center_of_mass_editor_create(&ui,&font));
    EditorProject project; EditorViewportState state = {0}; EditorHistory history;
    editor_project_init(&project); editor_viewport_state_init(&state);
    EditorObject *object = editor_project_object_add(&project,(Position){0});
    EditorRigidBody *body = editor_project_rigid_body_add(&project,object);
    assert(body != NULL);
    EditorRigidBodyId body_id = body->id;
    EditorObjectId object_id = object->id;
    project.viewport_local_view = false;
    project.viewport_camera_zoom = 1;
    state.mode = EDITOR_VIEWPORT_RIGID_BODY;
    state.selection = EDITOR_SELECTION_RIGID_BODY;
    state.selected_rigid_body = body_id;
    Position center = editor_mass_properties_get(body).center;
    Position pointer = {512+center.x, EDITOR_MENU_HEIGHT+(720-EDITOR_MENU_HEIGHT)*.5f-center.y};
    assert(editor_viewport_update(&state,&project,pointer,MOUSE_BUTTON_STATE_PRESSED,
        MOUSE_BUTTON_STATE_UP,false,0,false));
    assert(state.selection == EDITOR_SELECTION_CENTER_OF_MASS && state.mode == EDITOR_VIEWPORT_CENTER_OF_MASS);
    assert(!state.dragged_center_of_mass);
    assert(editor_navigation_pointer_selection_finish(&project,&state,(EditorSelectionRef){0},false,false));
    (void)editor_viewport_update(&state,&project,pointer,MOUSE_BUTTON_STATE_RELEASED,
        MOUSE_BUTTON_STATE_UP,false,0,false);
    assert(state.selection == EDITOR_SELECTION_CENTER_OF_MASS && state.mode == EDITOR_VIEWPORT_CENTER_OF_MASS);
    EditorSelectionRef com;
    assert(editor_viewport_selection_ref_get(&project,&state,&com));
    assert(!editor_viewport_selection_set(&project,&state,com,true));
    assert(state.selected_item_count == 1);
    project.navigation = editor_navigation_state_get(&project,&state);
    assert(project.navigation.mode == EDITOR_VIEWPORT_CENTER_OF_MASS);
    /* Existing navigation fields round-trip the dedicated mode without new fields. */
    assert(editor_project_save(&project,"editor_com_navigation.json"));
    EditorProject loaded; editor_project_init(&loaded);
    assert(!editor_result_check(editor_project_load(&loaded,"editor_com_navigation.json")));
    assert(loaded.navigation.mode == EDITOR_VIEWPORT_CENTER_OF_MASS &&
        loaded.navigation.selection == EDITOR_SELECTION_CENTER_OF_MASS);
    editor_project_destroy(&loaded); (void)SDL_RemovePath("editor_com_navigation.json");
    assert(editor_history_init(&history,&project));
    editor_command_executing_callback_set(begin,&history);
    editor_command_finished_callback_set(finish,&history);
    EditorModeContext context = {.project=&project,.history=&history,.viewport=&state,.width=400};
    /* Click the actual mode dropdown in the dedicated panel. */
    click(&ui,&context,false,(Position){200,54});
    click(&ui,&context,false,(Position){200,107});
    assert(body->center_of_mass_explicit && history.undo_count == 1);
    assert(state.mode == EDITOR_VIEWPORT_CENTER_OF_MASS);
    assert(editor_history_undo(&history));
    editor_navigation_state_apply(&project,&state,&project.navigation);
    body = editor_project_rigid_body_get(&project.objects[0],body_id);
    assert(!body->center_of_mass_explicit && state.mode == EDITOR_VIEWPORT_CENTER_OF_MASS &&
        state.selection == EDITOR_SELECTION_CENTER_OF_MASS);
    assert(editor_history_redo(&history));
    editor_navigation_state_apply(&project,&state,&project.navigation);
    body = editor_project_rigid_body_get(&project.objects[0],body_id);
    assert(body->center_of_mass_explicit && state.mode == EDITOR_VIEWPORT_CENTER_OF_MASS);
    click(&ui,&context,false,(Position){200,86});
    SDL_Event select_all = {.type=SDL_EVENT_KEY_DOWN};
    select_all.key.key = SDLK_A; select_all.key.mod = SDL_KMOD_CTRL;
    rohr_ui_field_event_add(&select_all);
    SDL_Event digit = {.type=SDL_EVENT_KEY_DOWN};
    digit.key.key = SDLK_1; rohr_ui_field_event_add(&digit);
    digit.key.key = SDLK_7; rohr_ui_field_event_add(&digit);
    panel(&ui,&context,false,(Position){200,86},MOUSE_BUTTON_STATE_UP);
    assert(fabsf(body->center_of_mass_offset.x-17) < .001f);
    rohr_ui_field_focus_clear();
    panel(&ui,&context,true,(Position){0},MOUSE_BUTTON_STATE_UP);
    assert(editor_mass_properties_get(body).center.x == 17);
    /* The same dropdown in the inline controls updates the dedicated editor. */
    click(&ui,&context,true,(Position){200,54});
    click(&ui,&context,true,(Position){200,80});
    assert(!body->center_of_mass_explicit && body->center_of_mass_offset.x == 0);
    panel(&ui,&context,false,(Position){0},MOUSE_BUTTON_STATE_UP);
    editor_viewport_back(&state);
    assert(state.mode == EDITOR_VIEWPORT_RIGID_BODY && state.selection == EDITOR_SELECTION_RIGID_BODY);
    assert(editor_viewport_selection_set(&project,&state,com,false));
    assert(editor_navigation_selected_open(&project,&state));
    /* Autoshape has its own input entry point; its visible COM has the same priority. */
    state.mode = EDITOR_VIEWPORT_AUTO_SHAPE;
    state.auto_shape_parent_mode = EDITOR_VIEWPORT_HITBOX;
    state.selected_hitbox = body->hitboxes[0].id;
    EditorAutoShapeConfig config = {0};
    assert(editor_viewport_auto_shape_update(&state,&project,&config,pointer,
        MOUSE_BUTTON_STATE_PRESSED,MOUSE_BUTTON_STATE_UP,false,0,false));
    assert(state.mode == EDITOR_VIEWPORT_CENTER_OF_MASS && state.selection == EDITOR_SELECTION_CENTER_OF_MASS);
    /* Automatic COM without geometry has unavailable readouts, but a valid editor. */
    size_t boxes = body->hitbox_count;
    body->hitbox_count = 0;
    panel(&ui,&context,false,(Position){0},MOUSE_BUTTON_STATE_UP);
    assert(!editor_mass_properties_get(body).center_available && state.mode == EDITOR_VIEWPORT_CENTER_OF_MASS);
    body->hitbox_count = boxes;
    /* Removing the owner exits safely, including when restored navigation is stale. */
    EditorNavigationState navigation = editor_navigation_state_get(&project,&state);
    EditorCommand remove = {.type=EDITOR_COMMAND_ITEM_REMOVE,
        .data.item_remove={EDITOR_ITEM_RIGID_BODY,object_id,0,body_id,0}};
    assert(editor_command_execute(&project,&remove).kind == ERROR_RESULT_VALUE);
    panel(&ui,&context,false,(Position){0},MOUSE_BUTTON_STATE_UP);
    assert(state.mode == EDITOR_VIEWPORT_OBJECT && state.selected_item_count == 0);
    editor_navigation_state_apply(&project,&state,&navigation);
    assert(state.mode == EDITOR_VIEWPORT_OBJECT && state.selection != EDITOR_SELECTION_CENTER_OF_MASS);
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&history); editor_viewport_state_destroy(&state); editor_project_destroy(&project);
    editor_center_of_mass_editor_destroy(&ui); editor_viewport_assets_destroy();
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
