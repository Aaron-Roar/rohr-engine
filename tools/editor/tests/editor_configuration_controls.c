/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editors/editor_entity_settings_panel.h"
#include "panels/editor_application_settings_panel.h"
#include "editor_navigation.h"
#include <assert.h>
#include <math.h>
#include <SDL3/SDL.h>
#include <string.h>

float editor_viewport_width = 1024, editor_window_width = 1280;
float editor_window_height = 720, editor_viewport_bottom = 720;
static void begin(const EditorProject *p, const EditorCommand *c, void *context) {
    editor_history_command_begin(context, p, c);
}
static void finish(const EditorCommand *c, const EditorCommandResult *r, void *context) {
    editor_history_command_finish(context, c, r);
}
static void entity_frame(EditorEntitySettingsPanel *panel, EditorModeContext *context,
        Position pointer, MouseButtonState button) {
    rohr_ui_input_set((UIInput){pointer, button});
    editor_mode_accordion_layout_measure_reset();
    (void)editor_entity_settings_panel_draw(panel, context, 0);
    rohr_ui_update();
}
static void entity_click(EditorEntitySettingsPanel *panel, EditorModeContext *context, Position p) {
    entity_frame(panel, context, p, MOUSE_BUTTON_STATE_PRESSED);
    entity_frame(panel, context, p, MOUSE_BUTTON_STATE_RELEASED);
    entity_frame(panel, context, p, MOUSE_BUTTON_STATE_UP);
}
static void app_frame(EditorApplicationSettingsPanel *panel, EditorProject *project,
        Position pointer, MouseButtonState button) {
    UIRect bounds = {420, 0, 800, 720};
    rohr_ui_input_set((UIInput){pointer, button}); rohr_ui_modal_set(bounds);
    editor_application_settings_panel_draw(panel, project, bounds);
    rohr_ui_update();
}
static void app_click(EditorApplicationSettingsPanel *panel, EditorProject *project, Position p) {
    app_frame(panel, project, p, MOUSE_BUTTON_STATE_PRESSED);
    app_frame(panel, project, p, MOUSE_BUTTON_STATE_RELEASED);
    app_frame(panel, project, p, MOUSE_BUTTON_STATE_UP);
}
static void lock_controls_frame(TextAsset *text,EditorModeAccordionSection *section,
        float *number,Position pointer,MouseButtonState button) {
    rohr_ui_input_set((UIInput){pointer,button});
    (void)editor_mode_field("editor.rigid_body.x",(UIFieldBinding){.kind=UI_FIELD_FLOAT,.number=number},
        text,(UIRect){100,100,240,28},NULL);
    (void)editor_mode_accordion_section_draw(section,"editor.rigid_body.section.position",(UIRect){100,200,240,28},0);
    rohr_ui_update();
}
static void numeric_controls_check(FontAsset *font) {
    static EditorProject project;editor_project_init(&project);
    EditorObject *object=editor_project_object_add(&project,(Position){0});assert(object);
    EditorRigidBody *body=editor_project_rigid_body_add(&project,object);assert(body);
    EditorNumericTarget target={.kind=EDITOR_ITEM_RIGID_BODY,.object=object->id,.item=body->id};
    EditorViewportState state={0};editor_viewport_state_init(&state);
    EditorSelectionRef selection={EDITOR_SELECTION_RIGID_BODY,object->id,0,0,body->id};
    editor_mode_numeric_context_set(&project,&state);editor_mode_diagnostics_set(NULL,selection);
    EditorHistory history;assert(editor_history_init(&history,&project));
    editor_command_executing_callback_set(begin,&history);editor_command_finished_callback_set(finish,&history);
    TextAsset text={0};assert(editor_mode_text_create(font,"12",&text));
    EditorModeAccordionSection section;assert(editor_mode_accordion_section_create(&section,font,"Position",false));
    float number=12;
    lock_controls_frame(&text,&section,&number,(Position){328,114},MOUSE_BUTTON_STATE_PRESSED);
    lock_controls_frame(&text,&section,&number,(Position){328,114},MOUSE_BUTTON_STATE_RELEASED);
    assert(editor_numeric_lock_check(&project,target,"position.x"));
    assert(!editor_numeric_lock_check(&project,target,"position.y"));
    lock_controls_frame(&text,&section,&number,(Position){180,114},MOUSE_BUTTON_STATE_PRESSED);
    lock_controls_frame(&text,&section,&number,(Position){180,114},MOUSE_BUTTON_STATE_RELEASED);
    SDL_Event key={.key={.type=SDL_EVENT_KEY_DOWN,.key=SDLK_A,.mod=SDL_KMOD_CTRL}};rohr_ui_field_event_add(&key);
    key.key.key=SDLK_9;key.key.mod=SDL_KMOD_NONE;rohr_ui_field_event_add(&key);
    lock_controls_frame(&text,&section,&number,(Position){180,114},MOUSE_BUTTON_STATE_UP);assert(number==12);
    /* Mixed group click locks all; the next click unlocks all. */
    lock_controls_frame(&text,&section,&number,(Position){328,214},MOUSE_BUTTON_STATE_PRESSED);
    lock_controls_frame(&text,&section,&number,(Position){328,214},MOUSE_BUTTON_STATE_RELEASED);
    assert(editor_numeric_lock_check(&project,target,"position.x") && editor_numeric_lock_check(&project,target,"position.y"));
    assert(!section.expanded);assert(editor_history_undo(&history));
    assert(editor_numeric_lock_check(&project,target,"position.x") && !editor_numeric_lock_check(&project,target,"position.y"));
    assert(editor_history_redo(&history));
    lock_controls_frame(&text,&section,&number,(Position){328,214},MOUSE_BUTTON_STATE_PRESSED);
    lock_controls_frame(&text,&section,&number,(Position){328,214},MOUSE_BUTTON_STATE_RELEASED);
    assert(!editor_numeric_lock_check(&project,target,"position.x") && !editor_numeric_lock_check(&project,target,"position.y"));
    EditorJoint *joint=editor_project_joint_add(&project,object,EDITOR_JOINT_REVOLUTE);assert(joint);
    EditorNumericTarget joint_target={.kind=EDITOR_ITEM_JOINT,.object=object->id,.item=joint->id};
    UISliderConfig slider=rohr_ui_slider_config_default_get();slider.center=(Position){220,300};slider.length=240;
    slider.min_value=0.25f;slider.max_value=3;
    for(int phase=0;phase<2;++phase) {
        rohr_ui_input_set((UIInput){{328,300},phase?MOUSE_BUTTON_STATE_RELEASED:MOUSE_BUTTON_STATE_PRESSED});
        editor_mode_numeric_field_bind(joint_target,"visual_size");
        (void)editor_mode_slider("editor.joint.visual_size",1,&slider);rohr_ui_update();
    }
    assert(editor_numeric_lock_check(&project,joint_target,"visual_size"));
    for(int phase=0;phase<2;++phase) {
        rohr_ui_input_set((UIInput){{280,300},phase?MOUSE_BUTTON_STATE_RELEASED:MOUSE_BUTTON_STATE_PRESSED});
        editor_mode_numeric_field_bind(joint_target,"visual_size");
        UISliderResult result=editor_mode_slider("editor.joint.visual_size",1,&slider);
        assert(!result.changed && result.value==1);rohr_ui_update();
    }
    assert(editor_history_undo(&history));assert(!editor_numeric_lock_check(&project,joint_target,"visual_size"));
    assert(editor_history_redo(&history));assert(editor_numeric_lock_check(&project,joint_target,"visual_size"));
    /* The bulk opacity field uses the same mixed-selection lock button. */
    EditorRigidBody *second=editor_project_rigid_body_add(&project,object);assert(second);
    EditorNumericTarget second_target={.kind=EDITOR_ITEM_RIGID_BODY,.object=object->id,.item=second->id};
    assert(editor_viewport_selection_set(&project,&state,selection,false));
    assert(editor_viewport_selection_set(&project,&state,(EditorSelectionRef){EDITOR_SELECTION_RIGID_BODY,object->id,0,0,second->id},true));
    assert(editor_numeric_lock_set(&project,target,"border_color.alpha",true));
    for(int toggle=0;toggle<2;++toggle) {
        for(int phase=0;phase<2;++phase) {
            rohr_ui_input_set((UIInput){{328,400},phase?MOUSE_BUTTON_STATE_RELEASED:MOUSE_BUTTON_STATE_PRESSED});
            editor_mode_numeric_field_bind(target,"border_color.alpha");
            (void)editor_mode_field("editor.bulk.color_picker.opacity",(UIFieldBinding){.kind=UI_FIELD_FLOAT,.number=&number},
                &text,(UIRect){100,387,240,26},NULL);rohr_ui_update();
        }
        assert(editor_numeric_lock_check(&project,target,"border_color.alpha")==!toggle);
        assert(editor_numeric_lock_check(&project,second_target,"border_color.alpha")==!toggle);
    }
    editor_mode_numeric_context_set(NULL,NULL);editor_mode_diagnostics_set(NULL,(EditorSelectionRef){0});
    editor_command_executing_callback_set(NULL,NULL);editor_command_finished_callback_set(NULL,NULL);
    editor_mode_accordion_section_destroy(&section);rohr_graphics_text_destroy(&text);
    editor_history_destroy(&history);editor_viewport_state_destroy(&state);editor_project_destroy(&project);rohr_ui_field_focus_clear();
}
int main(void) {
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    FontAsset font = rohr_graphics_font_default_get();
    numeric_controls_check(&font);
    static EditorProject p; editor_project_init(&p);
    EditorObject *o = editor_project_object_add(&p, (Position){0}); assert(o);
    EditorRigidBody *body = editor_project_rigid_body_add(&p, o); assert(body);
    EditorViewportState state = {0}; editor_viewport_state_init(&state);
    assert(editor_viewport_selection_set(&p, &state,
        (EditorSelectionRef){EDITOR_SELECTION_RIGID_BODY, o->id, 0, 0, body->id}, false));
    assert(editor_navigation_selected_open(&p, &state));
    p.navigation = editor_navigation_state_get(&p, &state);
    EditorHistory history; assert(editor_history_init(&history, &p));
    editor_command_executing_callback_set(begin, &history);
    editor_command_finished_callback_set(finish, &history);
    EditorModeContext context = {.project = &p, .history = &history, .viewport = &state, .width = 400};
    EditorEntitySettingsPanel entity; assert(editor_entity_settings_panel_create(&entity, &font));
    entity_click(&entity, &context, (Position){100, 15}); assert(entity.sections[0].expanded);
    entity_click(&entity, &context, (Position){20, 50});
    assert(p.objects[0].rigid_bodies[0].entity_settings.lifetime.deadline.time_enabled);
    assert(editor_history_undo(&history));
    assert(!p.objects[0].rigid_bodies[0].entity_settings.lifetime.deadline.time_enabled);
    assert(editor_history_redo(&history));
    assert(p.objects[0].rigid_bodies[0].entity_settings.lifetime.deadline.time_enabled);
    /* Selecting a body row in the object editor must still edit the object. */
    state.mode = EDITOR_VIEWPORT_OBJECT;
    entity_click(&entity, &context, (Position){20, 50});
    assert(p.objects[0].entity_settings.lifetime.deadline.time_enabled);
    assert(p.objects[0].rigid_bodies[0].entity_settings.lifetime.deadline.time_enabled);
    assert(editor_history_undo(&history));
    assert(!p.objects[0].entity_settings.lifetime.deadline.time_enabled);
    assert(p.objects[0].rigid_bodies[0].entity_settings.lifetime.deadline.time_enabled);
    /* The axis UI edits one clockwise angle from +Y, backed by the API vector. */
    assert(editor_navigation_selected_open(&p, &state));
    for(size_t i=0;i<5;++i) editor_mode_accordion_section_expanded_set(&entity.sections[i],false);
    editor_mode_accordion_section_expanded_set(&entity.sections[3],true);
    body=&p.objects[0].rigid_bodies[0];body->entity_settings.axis_lock_enabled=true;
    body->entity_settings.axis_lock.axis=(Axis){0,1};
    body->entity_settings.axis_lock.point_on_axis=(Position){12,34};
    entity_click(&entity,&context,(Position){220,222});
    SDL_Event select={.type=SDL_EVENT_KEY_DOWN};select.key.key=SDLK_A;select.key.mod=SDL_KMOD_CTRL;
    rohr_ui_field_event_add(&select);
    SDL_Event text={.text={.type=SDL_EVENT_TEXT_INPUT,.text="45"}};rohr_ui_field_event_add(&text);
    entity_frame(&entity,&context,(Position){220,222},MOUSE_BUTTON_STATE_UP);
    assert(body->entity_settings.axis_lock.axis.x==0 && body->position.x==0);
    SDL_Event enter={.type=SDL_EVENT_KEY_DOWN};enter.key.key=SDLK_RETURN;enter.key.mod=SDL_KMOD_NONE;rohr_ui_field_event_add(&enter);
    entity_frame(&entity,&context,(Position){220,222},MOUSE_BUTTON_STATE_UP);
    body=&p.objects[0].rigid_bodies[0];
    assert(fabsf(body->entity_settings.axis_lock.axis.x-.70710678f)<.0001f);
    assert(fabsf(body->entity_settings.axis_lock.axis.y-.70710678f)<.0001f);
    assert(body->entity_settings.axis_lock.point_on_axis.x==12 && body->entity_settings.axis_lock.point_on_axis.y==34);
    assert(fabsf(body->position.x+11)<.0001f && fabsf(body->position.y-11)<.0001f);
    assert(editor_history_undo(&history));body=&p.objects[0].rigid_bodies[0];
    assert(body->entity_settings.axis_lock.axis.x==0 && body->entity_settings.axis_lock.axis.y==1);
    assert(body->position.x==0 && body->position.y==0);
    assert(editor_history_redo(&history));
    /* Shift commits on blur; draft typing and Escape do not move geometry. */
    entity_click(&entity,&context,(Position){220,256});
    rohr_ui_field_event_add(&select);text.text.text="56";rohr_ui_field_event_add(&text);
    entity_frame(&entity,&context,(Position){220,256},MOUSE_BUTTON_STATE_UP);
    body=&p.objects[0].rigid_bodies[0];assert(body->entity_settings.axis_lock.point_on_axis.x==12);
    entity_click(&entity,&context,(Position){410,350});
    assert(body->entity_settings.axis_lock.point_on_axis.x==56);
    assert(fabsf(body->position.x-11)<.0001f && fabsf(body->position.y+11)<.0001f);
    assert(editor_history_undo(&history));assert(editor_history_redo(&history));
    body=&p.objects[0].rigid_bodies[0];
    entity_click(&entity,&context,(Position){220,256});
    rohr_ui_field_event_add(&select);text.text.text="99";rohr_ui_field_event_add(&text);
    entity_frame(&entity,&context,(Position){220,256},MOUSE_BUTTON_STATE_UP);
    SDL_Event escape={.key={.type=SDL_EVENT_KEY_DOWN,.key=SDLK_ESCAPE,.mod=SDL_KMOD_NONE}};
    rohr_ui_field_event_add(&escape);entity_frame(&entity,&context,(Position){220,256},MOUSE_BUTTON_STATE_UP);
    entity_click(&entity,&context,(Position){410,350});
    assert(body->entity_settings.axis_lock.point_on_axis.x==56);
    EditorApplicationSettingsPanel app; assert(editor_application_settings_panel_create(&app, &font));
    app.open = true;
    app_click(&app, &p, (Position){460, 74}); assert(p.application_window_enabled);
    assert(editor_history_undo(&history) && !p.application_window_enabled);
    assert(editor_history_redo(&history) && p.application_window_enabled);
    strcpy(app.draft.name, "impact"); strcpy(app.draft.path, "assets/impact.wav");
    app.scroll = 60;
    app_click(&app, &p, (Position){720, 566});
    assert(p.audio_definition_count == 1 && p.audio_definitions[0].volume == 0);
    assert(editor_history_undo(&history) && p.audio_definition_count == 0);
    assert(editor_history_redo(&history) && p.audio_definition_count == 1);
    editor_command_executing_callback_set(NULL, NULL); editor_command_finished_callback_set(NULL, NULL);
    editor_application_settings_panel_destroy(&app); editor_entity_settings_panel_destroy(&entity);
    editor_history_destroy(&history); editor_viewport_state_destroy(&state); editor_project_destroy(&p);
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
