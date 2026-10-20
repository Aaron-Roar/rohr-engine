/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "panels/editor_scene_settings_panel.h"
#include "editor_workspace.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

float editor_viewport_width=1024, editor_window_width=1280, editor_window_height=720, editor_viewport_bottom=720;
static void begin(const EditorProject *p, const EditorCommand *c, void *h) { editor_history_command_begin(h,p,c); }
static void finish(const EditorCommand *c, const EditorCommandResult *r, void *h) { editor_history_command_finish(h,c,r); }
static void execute(EditorProject *p, EditorCommand c) { assert(editor_command_execute(p,&c).kind != ERROR_RESULT_ERROR); }
static void frame(EditorSceneSettingsPanel *panel, EditorProject *project, EditorHistory *history,
        Position pointer, MouseButtonState button) {
    UIRect bounds = {100,100,600,500};
    rohr_ui_input_set((UIInput){.pointer=pointer,.primary_button=button});
    rohr_ui_modal_set(bounds);
    assert(!rohr_ui_button("underlying", &panel->all_label, (UIRect){0,0,80,80},NULL).clicked);
    editor_scene_settings_panel_draw(panel,project,history,NULL,bounds); rohr_ui_update();
}
static void click(EditorSceneSettingsPanel *panel, EditorProject *project, EditorHistory *history, Position at) {
    frame(panel,project,history,at,MOUSE_BUTTON_STATE_PRESSED);
    frame(panel,project,history,at,MOUSE_BUTTON_STATE_RELEASED);
    frame(panel,project,history,at,MOUSE_BUTTON_STATE_UP);
}
int main(void) {
    assert(!rohr_error_check(rohr_engine_start())); assert(!rohr_error_check(rohr_graphics_start()));
    EditorProject project={0}; editor_project_init(&project);
    EditorObjectId first=editor_project_object_add(&project,(Position){0})->id;
    EditorObjectId second=editor_project_object_add(&project,(Position){0})->id;
    EditorLayoutViewportId viewport=editor_project_layout_viewport_add(&project)->id;
    EditorHistory history; assert(editor_history_init(&history,&project));
    editor_command_executing_callback_set(begin,&history); editor_command_finished_callback_set(finish,&history);
    EditorNavigationState navigation=project.navigation;
    execute(&project,(EditorCommand){.type=EDITOR_COMMAND_SIMULATION_OBJECT_SET,.data.simulation_object={first,false}});
    assert(project.objects[0].simulation_excluded && !project.objects[1].simulation_excluded);
    assert(editor_history_undo(&history) && !project.objects[0].simulation_excluded);
    assert(editor_history_redo(&history) && project.objects[0].simulation_excluded);
    size_t count=history.undo_count;
    assert(editor_scene_settings_all_set(&project,&history,false)); assert(history.undo_count==count+1);
    assert(project.objects[0].simulation_excluded && project.objects[1].simulation_excluded);
    assert(editor_history_undo(&history)); assert(project.objects[0].simulation_excluded && !project.objects[1].simulation_excluded);
    assert(editor_history_redo(&history));
    assert(editor_scene_settings_all_set(&project,&history,true));
    assert(!project.objects[0].simulation_excluded && !project.objects[1].simulation_excluded);
    execute(&project,(EditorCommand){.type=EDITOR_COMMAND_SIMULATION_VIEWPORT_SET,.data.simulation_viewport={viewport}});
    assert(editor_history_ui_change_begin(&history));
    assert(editor_project_layout_viewport_remove(&project,viewport)); assert(!project.simulation_viewport);
    assert(editor_history_ui_change_finish(&history));
    assert(editor_history_undo(&history)); assert(project.simulation_viewport==viewport);
    assert(editor_history_redo(&history)); assert(!project.simulation_viewport);
    viewport=editor_project_layout_viewport_add(&project)->id; assert(!project.simulation_viewport);
    execute(&project,(EditorCommand){.type=EDITOR_COMMAND_SIMULATION_OBJECT_SET,.data.simulation_object={first,false}});
    EditorObject *copy=editor_project_object_duplicate(&project,first); assert(copy && !copy->simulation_excluded);
    assert(editor_project_object_add(&project,(Position){0}) && !project.objects[project.object_count-1].simulation_excluded);
    strcpy(project.objects[0].name,"Renamed"); assert(project.objects[0].simulation_excluded);
    const char *path="scene_settings_test.json";
    assert(editor_project_save(&project,path));
    EditorProject loaded={0}; editor_project_init(&loaded);
    assert(!editor_result_check(editor_project_load(&loaded,path)));
    assert(!loaded.simulation_viewport && loaded.objects[0].simulation_excluded);
    /* Older files lack both fields and select all objects plus their first viewport. */
    size_t size; char *json=SDL_LoadFile(path,&size); assert(json);
    char *field=strstr(json,"\"simulation_viewport\""); assert(field); field[1]='_';
    while((field=strstr(json,"\"simulation_excluded\""))) field[1]='_';
    FILE *file=fopen(path,"wb"); assert(file); assert(fwrite(json,1,size,file)==size); assert(!fclose(file)); SDL_free(json);
    assert(!editor_result_check(editor_project_load(&loaded,path)));
    assert(loaded.simulation_viewport==viewport);
    for(size_t i=0;i<loaded.object_count;++i) assert(!loaded.objects[i].simulation_excluded);
    editor_project_destroy(&loaded); assert(!remove(path));
    FontAsset font=rohr_graphics_font_default_get(); EditorSceneSettingsPanel panel={0};
    assert(editor_scene_settings_panel_create(&panel,&font)); panel.open=true;
    click(&panel,&project,&history,(Position){25,25}); /* Modal blocks underlying controls. */
    click(&panel,&project,&history,(Position){140,258}); assert(!project.objects[0].simulation_excluded);
    click(&panel,&project,&history,(Position){240,218});
    for(size_t i=0;i<project.object_count;++i) assert(project.objects[i].simulation_excluded);
    click(&panel,&project,&history,(Position){140,218});
    for(size_t i=0;i<project.object_count;++i) assert(!project.objects[i].simulation_excluded);
    click(&panel,&project,&history,(Position){300,174});
    click(&panel,&project,&history,(Position){300,230}); assert(project.simulation_viewport==viewport);
    click(&panel,&project,&history,(Position){300,174});
    click(&panel,&project,&history,(Position){300,202}); assert(!project.simulation_viewport);
    for(int i=0;i<25;++i) assert(editor_project_object_add(&project,(Position){0}));
    SDL_Event wheel={.type=SDL_EVENT_MOUSE_WHEEL}; wheel.wheel.y=-4; rohr_ui_field_event_add(&wheel);
    frame(&panel,&project,&history,(Position){350,400},MOUSE_BUTTON_STATE_UP); assert(panel.scroll>0);
    assert(!memcmp(&navigation,&project.navigation,sizeof(navigation)));
    (void)second;
    editor_scene_settings_panel_destroy(&panel);
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&history); editor_project_destroy(&project);
    rohr_graphics_stop(); rohr_engine_stop(); return 0;
}
