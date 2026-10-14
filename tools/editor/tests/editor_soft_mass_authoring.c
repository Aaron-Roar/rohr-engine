/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editors/soft_body/editor_soft_body.h"
#include "editor_mass_properties.h"
#include "editor_navigation.h"
#include "editor_workspace.h"
#include "editor_layout.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

float editor_viewport_width = 1024, editor_window_width = 1280;
float editor_window_height = 720, editor_viewport_bottom = 720;
static bool near(float a, float b) { return fabsf(a - b) < .002f; }
static EditorSoftBody *soft_get(EditorProject *p) { return &p->objects[0].soft_body_items[0]; }
static void begin(const EditorProject *p, const EditorCommand *c, void *h) {
    editor_history_command_begin(h, p, c);
}
static void finish(const EditorCommand *c, const EditorCommandResult *r, void *h) {
    editor_history_command_finish(h, c, r);
}
static void panel(EditorSoftBodyEditor *ui, EditorAutoShapeEditor *shape,
        EditorModeContext *context, bool dedicated, Position pointer, MouseButtonState button) {
    rohr_ui_input_set((UIInput){pointer, button});
    editor_mode_accordion_layout_measure_reset();
    if(dedicated) (void)editor_center_of_mass_panel_draw(&ui->mass_properties, context);
    else (void)editor_soft_body_editor_draw(ui, shape, context);
    rohr_ui_update();
}
static void number(EditorSoftBodyEditor *ui, EditorAutoShapeEditor *shape,
        EditorModeContext *context, bool dedicated, float y, const char *text) {
    Position pointer = {200, y + 10};
    panel(ui,shape,context,dedicated,pointer,MOUSE_BUTTON_STATE_PRESSED);
    panel(ui,shape,context,dedicated,pointer,MOUSE_BUTTON_STATE_RELEASED);
    panel(ui,shape,context,dedicated,pointer,MOUSE_BUTTON_STATE_UP);
    SDL_Event event = {.type = SDL_EVENT_KEY_DOWN};
    event.key.key = SDLK_A; event.key.mod = SDL_KMOD_CTRL;
    rohr_ui_field_event_add(&event); event.key.mod = 0;
    for(const char *at = text; *at; at++) {
        event.key.key = (SDL_Keycode)*at; rohr_ui_field_event_add(&event);
    }
    panel(ui,shape,context,dedicated,pointer,MOUSE_BUTTON_STATE_UP);
    rohr_ui_field_focus_clear();
}
static void fixture_remove(const char *root) {
    const char *files[] = {"project.rohr.json", "objects/project.rohr.json", "src/main.c",
        "src/generated/project_objects.c", "src/generated/project_objects.h",
        "src/generated/project_viewports.c", "src/generated/project_viewports.h",
        "CMakeLists.txt", ".gitignore", "editor.lua", "assets/tutorial_frame_1.png",
        "assets/tutorial_frame_2.png", "src/generated", "src", "assets", "objects", ""};
    for(size_t i = 0; i < sizeof(files)/sizeof(files[0]); i++) {
        char path[2048]; snprintf(path,sizeof(path),"%s/%s",root,files[i]);
        (void)SDL_RemovePath(path);
    }
}
static void marker_check(void) {
    const float zooms[] = {.1f,.5f,1,2,10};
    for(size_t z = 0; z < 5; z++) for(int attempt = 0; attempt < 5; attempt++) {
        EditorProject p; editor_project_init(&p);
        EditorObject *object = editor_project_object_add(&p,(Position){1,2});
        EditorSoftBody *soft = editor_project_soft_body_add(&p,object);
        const Position points[] = {{-100,-100},{100,-100},{100,100},{-100,100}};
        for(size_t i = 0; i < 4; i++) assert(editor_project_soft_node_add(&p,soft,points[i]) != NULL);
        soft->position = (Position){3,4}; soft->rotation = 90;
        assert(editor_soft_center_of_mass_set(&p,object->id,soft->id,(Position){10,20}).kind == ERROR_RESULT_VALUE);
        EditorSprite *cover = editor_project_sprite_add(&p,object,"cover","unused.png");
        assert(cover != NULL); cover->position = (Position){23,-6};
        cover->size = (Scale){400,400}; cover->graphics_layer.value = 1000000;
        EditorViewportState state = {0}; editor_viewport_state_init(&state);
        state.mode = EDITOR_VIEWPORT_SOFT_BODY; state.selection = EDITOR_SELECTION_SOFT_BODY;
        state.selected_soft_body = soft->id;
        p.viewport_local_view = false; p.viewport_camera_zoom = zooms[z];
        state.selection_modifier = attempt == 2; soft->visible = attempt != 3;
        Position pointer = {512 + 24 * zooms[z] + (attempt == 4 ? 14 : 12),
            EDITOR_MENU_HEIGHT+(720-EDITOR_MENU_HEIGHT)*.5f + 4 * zooms[z]};
        (void)editor_viewport_update(&state,&p,pointer,MOUSE_BUTTON_STATE_PRESSED,
            MOUSE_BUTTON_STATE_UP,false,0,attempt == 1);
        if(attempt == 0) {
            assert(state.dragged_center_of_mass && state.mode == EDITOR_VIEWPORT_CENTER_OF_MASS);
            pointer.x += 5;
            assert(editor_viewport_update(&state,&p,pointer,MOUSE_BUTTON_STATE_DOWN,
                MOUSE_BUTTON_STATE_UP,false,0,false));
            EditorSoftMassProperties value = editor_soft_mass_properties_get(soft);
            assert(near(value.center.x,10) && near(value.center.y,20+5/zooms[z]));
            /* An infeasible continuation preserves the last valid distribution. */
            Mass previous[4]; for(size_t i = 0; i < 4; i++) previous[i] = soft->nodes[i].node_mass;
            pointer.x += 200 * zooms[z];
            assert(!editor_viewport_update(&state,&p,pointer,MOUSE_BUTTON_STATE_DOWN,
                MOUSE_BUTTON_STATE_UP,false,0,false));
            for(size_t i = 0; i < 4; i++) assert(previous[i] == soft->nodes[i].node_mass);
            editor_viewport_draw(&p,&state,false);
            (void)editor_viewport_update(&state,&p,pointer,MOUSE_BUTTON_STATE_RELEASED,
                MOUSE_BUTTON_STATE_UP,false,0,false);
            assert(!editor_viewport_transform_active_check(&state));
            EditorNavigationState navigation = editor_navigation_state_get(&p,&state);
            EditorCommand remove = {.type=EDITOR_COMMAND_ITEM_REMOVE,
                .data.item_remove={EDITOR_ITEM_SOFT_BODY,object->id,0,soft->id,0}};
            assert(editor_command_execute(&p,&remove).kind == ERROR_RESULT_VALUE);
            editor_navigation_state_apply(&p,&state,&navigation);
            assert(state.mode == EDITOR_VIEWPORT_OBJECT);
        } else assert(!state.dragged_center_of_mass && state.selection != EDITOR_SELECTION_CENTER_OF_MASS);
        editor_viewport_state_destroy(&state); editor_project_destroy(&p);
    }
}

int main(int argc, char **argv) {
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    FontAsset font = rohr_graphics_font_default_get();
    EditorSoftBodyEditor ui; EditorAutoShapeEditor shape;
    assert(editor_soft_body_editor_create(&ui,&font));
    assert(editor_auto_shape_editor_create(&shape,&font));
    EditorProject project = {0}; EditorWorkspace workspace = {0};
    char fixture[1024];
    snprintf(fixture,sizeof(fixture),"soft_mass_%llu",(unsigned long long)SDL_GetTicksNS());
    if(argc == 2) snprintf(fixture,sizeof(fixture),"%s",argv[1]);
    assert(!editor_result_check(editor_workspace_create(&workspace,&project,fixture)));
    editor_project_destroy(&project); editor_project_init(&project);
    EditorObject *object = editor_project_object_add(&project,(Position){0});
    EditorSoftBody *soft = editor_project_soft_body_add(&project,object);
    assert(soft != NULL);
    editor_project_object_name_format(object->name,sizeof(object->name),"cloth");
    snprintf(soft->name,sizeof(soft->name),"fabric");
    Position corners[] = {{-100,-100},{100,-100},{100,100},{-100,100}};
    for(size_t i = 0; i < 4; i++) {
        EditorSoftNode *node = editor_project_soft_node_add(&project,soft,corners[i]);
        assert(node != NULL); node->node_mass = 1;
    }
    EditorViewportState state = {0}; editor_viewport_state_init(&state);
    assert(editor_viewport_selection_set(&project,&state,
        (EditorSelectionRef){EDITOR_SELECTION_SOFT_BODY,object->id,0,0,soft->id},false));
    state.mode = EDITOR_VIEWPORT_SOFT_BODY;
    project.viewport_local_view = false; project.viewport_camera_zoom = 1;
    project.navigation = editor_navigation_state_get(&project,&state);
    EditorHistory history; assert(editor_history_init(&history,&project));
    editor_command_executing_callback_set(begin,&history);
    editor_command_finished_callback_set(finish,&history);
    EditorModeContext context = {.project=&project,.history=&history,.viewport=&state,.width=400};
    /* Exercise the actual inline Physics accordion and shared field controls. */
    assert(!ui.physics_section.expanded);
    editor_mode_accordion_section_expanded_set(&ui.physics_section,true);
    editor_mode_accordion_section_expanded_set(&ui.transform_section,false);
    panel(&ui,&shape,&context,false,(Position){0},MOUSE_BUTTON_STATE_UP);
    /* Transform header starts at 118, Physics header follows at 154; first row 190. */
    number(&ui,&shape,&context,false,190,"8");
    assert(near(editor_soft_mass_properties_get(soft).total,8));
    for(size_t i = 0; i < 4; i++) assert(near(soft->nodes[i].node_mass,2));
    assert(editor_history_undo(&history)); soft = soft_get(&project);
    assert(near(editor_soft_mass_properties_get(soft).total,4));
    assert(editor_history_redo(&history)); soft = soft_get(&project);
    /* COM has top selection priority and opens its own panel. */
    Position pointer = {512,EDITOR_MENU_HEIGHT+(720-EDITOR_MENU_HEIGHT)*.5f};
    assert(editor_viewport_update(&state,&project,pointer,MOUSE_BUTTON_STATE_PRESSED,
        MOUSE_BUTTON_STATE_UP,false,0,false));
    assert(state.mode == EDITOR_VIEWPORT_CENTER_OF_MASS && state.selected_rigid_body == 0);
    assert(state.dragged_center_of_mass && state.selected_soft_body == soft->id);
    (void)editor_viewport_update(&state,&project,pointer,MOUSE_BUTTON_STATE_RELEASED,
        MOUSE_BUTTON_STATE_UP,false,0,false);
    project.navigation = editor_navigation_state_get(&project,&state);
    number(&ui,&shape,&context,true,74,"25");
    soft = soft_get(&project);
    assert(near(editor_soft_mass_properties_get(soft).center.x,25));
    assert(near(editor_soft_mass_properties_get(soft).total,8));
    for(size_t i = 0; i < 4; i++) {
        assert(soft->nodes[i].position.x == corners[i].x && soft->nodes[i].position.y == corners[i].y);
        assert(near(soft->nodes[i].node_mass,corners[i].x > 0 ? 2.5f : 1.5f));
    }
    size_t before = history.undo_count;
    number(&ui,&shape,&context,true,74,"150");
    assert(ui.mass_properties.soft_error && history.undo_count == before);
    assert(near(editor_soft_mass_properties_get(soft).center.x,25));
    number(&ui,&shape,&context,true,42,"-1");
    assert(history.undo_count == before && near(editor_soft_mass_properties_get(soft).total,8));
    /* One drag is one undo item; canceled transactions restore all masses. */
    assert(editor_history_transaction_begin(&history));
    pointer.x = 512 + 25 + 5;
    assert(editor_viewport_update(&state,&project,pointer,MOUSE_BUTTON_STATE_PRESSED,
        MOUSE_BUTTON_STATE_UP,false,0,false));
    pointer.x += 10;
    assert(editor_viewport_update(&state,&project,pointer,MOUSE_BUTTON_STATE_DOWN,
        MOUSE_BUTTON_STATE_UP,false,0,false));
    pointer.x += 10;
    assert(editor_viewport_update(&state,&project,pointer,MOUSE_BUTTON_STATE_DOWN,
        MOUSE_BUTTON_STATE_UP,false,0,false));
    (void)editor_viewport_update(&state,&project,pointer,MOUSE_BUTTON_STATE_RELEASED,
        MOUSE_BUTTON_STATE_UP,false,0,false);
    assert(editor_history_transaction_end(&history));
    assert(history.undo_count == before + 1 && near(editor_soft_mass_properties_get(soft).center.x,45));
    assert(editor_history_undo(&history)); soft = soft_get(&project);
    assert(near(editor_soft_mass_properties_get(soft).center.x,25));
    assert(editor_history_redo(&history)); soft = soft_get(&project);
    assert(near(editor_soft_mass_properties_get(soft).center.x,45));
    assert(editor_history_transaction_begin(&history));
    assert(editor_soft_center_of_mass_set(&project,1,1,(Position){0,0}).kind == ERROR_RESULT_VALUE);
    editor_history_transaction_cancel(&history); soft = soft_get(&project);
    assert(near(editor_soft_mass_properties_get(soft).center.x,45));
    editor_navigation_state_apply(&project,&state,&project.navigation);
    assert(state.mode == EDITOR_VIEWPORT_CENTER_OF_MASS && state.selected_soft_body == 1);
    editor_viewport_back(&state);
    assert(state.mode == EDITOR_VIEWPORT_SOFT_BODY && state.selection == EDITOR_SELECTION_SOFT_BODY);
    /* Named CLI, command recording and execution use the same atomic mutation. */
    char *args[] = {"rohr-cli","--project","project.rohr.json","--object","cloth","--soft-body","fabric",
        "--property","center-of-mass","25","0"};
    EditorCommand command; const char *path;
    EditorResult parsed = editor_command_cli_standard_parse(&project,11,args,&path,&command);
    assert(!editor_result_check(parsed));
    assert(editor_command_execute(&project,&command).kind == ERROR_RESULT_VALUE);
    char output[2048];
    assert(!editor_result_check(editor_command_cli_write(&command,"project.rohr.json",output,sizeof(output))));
    assert(strstr(output,"center-of-mass 25 0") != NULL && strstr(output,"explicit") == NULL);
    char *mass_args[] = {"rohr-cli","soft-body","set","project.rohr.json","1","1","mass","16"};
    assert(!editor_result_check(editor_command_cli_parse(8,mass_args,&path,&command)));
    assert(editor_command_execute(&project,&command).kind == ERROR_RESULT_VALUE);
    soft = soft_get(&project);
    assert(near(editor_soft_mass_properties_get(soft).total,16));
    EditorSoftBody clone = {0}; assert(editor_project_soft_body_clone(&clone,soft));
    assert(near(editor_soft_mass_properties_get(&clone).center.x,25));
    clone.nodes[0].node_mass = 100;
    assert(near(soft->nodes[0].node_mass,3));
    editor_project_soft_body_destroy(&clone);
    soft->position = (Position){7,9}; soft->rotation = 90;
    assert(editor_workspace_save(&workspace,&project));
    char state_path[2048]; snprintf(state_path,sizeof(state_path),"%s/objects/project.rohr.json",fixture);
    EditorProject loaded; editor_project_init(&loaded);
    assert(!editor_result_check(editor_project_load(&loaded,state_path)));
    assert(near(editor_soft_mass_properties_get(soft_get(&loaded)).total,16));
    assert(near(editor_soft_mass_properties_get(soft_get(&loaded)).center.x,25));
    editor_navigation_state_apply(&loaded,&state,&loaded.navigation);
    assert(state.mode == EDITOR_VIEWPORT_CENTER_OF_MASS && state.selected_rigid_body == 0);
    assert(editor_workspace_c_generate(&workspace,&loaded));
    editor_project_destroy(&loaded);
    char source_path[2048]; snprintf(source_path,sizeof(source_path),"%s/src/generated/project_objects.c",fixture);
    char *source = SDL_LoadFile(source_path,NULL); assert(source != NULL);
    assert(strstr(source,"rohr_physics_position_set(object->fabric, (Position){position.x + 7.00000000f, position.y + 9.00000000f})") != NULL);
    assert(strstr(source,"rohr_physics_orientation_set(object->fabric, 90.0000000f)") != NULL);
    assert(strstr(source,"}, 3.00000000f, 4.00000000f)") != NULL);
    assert(strstr(source,"}, 5.00000000f, 4.00000000f)") != NULL);
    SDL_free(source);
    /* Derived readouts react immediately; empty and zero-total bodies have no COM. */
    soft->nodes[0].position.x -= 16;
    assert(near(editor_soft_mass_properties_get(soft).center.x,22));
    for(size_t i = 0; i < 4; i++) soft->nodes[i].node_mass = 0;
    assert(!editor_soft_mass_properties_get(soft).center_available);
    soft->node_count = 0;
    assert(editor_soft_mass_properties_get(soft).mass_available && editor_soft_mass_properties_get(soft).total == 0);
    panel(&ui,&shape,&context,true,(Position){0},MOUSE_BUTTON_STATE_UP);
    soft->node_count = 4;
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&history); editor_viewport_state_destroy(&state);
    editor_project_destroy(&project); editor_soft_body_editor_destroy(&ui);
    editor_auto_shape_editor_destroy(&shape); editor_viewport_assets_destroy();
    if(argc != 2) fixture_remove(fixture);
    marker_check(); editor_viewport_assets_destroy();
    rohr_graphics_stop(); rohr_engine_stop(); return 0;
}
