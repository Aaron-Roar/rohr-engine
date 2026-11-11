/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "runtime_fixture.h"
#include "editor_navigation.h"
#include <editor_soft_area.h>
#include "editors/render/editor_camera.h"
#include "entity/object_internal.h"
#include "physics/physics_internal.h"
#include "graphics/marker_geometry.h"
#include "entity/object_visuals.h"
#include <math.h>
#include "yyjson/yyjson.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float editor_viewport_width=1024, editor_window_width=1280, editor_window_height=720, editor_viewport_bottom=720;
static void begin(const EditorProject *p,const EditorCommand *c,void *h) { editor_history_command_begin(h,p,c); }
static void finish(const EditorCommand *c,const EditorCommandResult *r,void *h) { editor_history_command_finish(h,c,r); }
static unsigned frames;
static void render(CameraId camera,void *context) {
    (void)camera;
    ++frames;
    assert(rohr_entity_object_draw(*(Entity *)context));
}
static void runtime_check(EditorProject *p, bool draws) {
    ObjectDescriptionResult description=object_description_project_read(p, "");
    assert(!rohr_error_check(description));
    const ObjectDescription *object=description.result.value->children[0];
    assert(object->children[0]->visible == !p->objects[0].rigid_bodies[0].app_hidden);
    EntityResult instance=rohr_entity_object_description_add(object,(ObjectSpawnDescription){.flags=ROHR_ENTITY_OBJECT_ALL});
    assert(!rohr_error_check(instance));
    EntityResult body=fixture_entity_get(instance.result.value,"body");
    assert(!rohr_error_check(body));
    assert(rohr_entity_components_check(body.result.value,ROHR_MASS));
    EntityResult camera_entity=fixture_entity_get(instance.result.value,"camera");
    assert(!rohr_error_check(camera_entity));
    CameraIdResult camera=rohr_entity_object_camera_get(camera_entity.result.value);
    assert(!rohr_error_check(camera));
    assert(!rohr_error_check(rohr_camera_render_callback_set(camera.result.value,render,&instance.result.value)));
    ScreenIdResult screen=rohr_screen_create((ScreenDescription){camera.result.value,320,240});
    assert(!rohr_error_check(screen));
    ViewportIdResult viewport=rohr_viewport_create((ViewportDescription){.rectangle={0,0,320,240}});
    assert(!rohr_error_check(viewport));
    assert(!rohr_error_check(rohr_viewport_screen_add(viewport.result.value,screen.result.value,
        (ViewportItemDescription){.rectangle={0,0,320,240},.content_scale={1,1},.visible=true})));
    assert(!rohr_error_check(rohr_viewport_enable_set(viewport.result.value)));
    frames=0; rohr_graphics_show(); assert((frames>0)==draws);
    assert(!rohr_error_check(rohr_viewport_destroy(viewport.result.value)));
    assert(!rohr_error_check(rohr_screen_destroy(screen.result.value)));
    assert(!rohr_error_check(rohr_entity_delete(instance.result.value)));
    object_description_tree_destroy((ObjectDescription *)description.result.value);
}
static void legacy(yyjson_mut_val *v) {
    if(yyjson_mut_is_obj(v)) {
        yyjson_mut_obj_remove_str(v,"visible_in_app");
        yyjson_mut_obj_remove_str(v,"visuals");
        size_t i,max; yyjson_mut_val *key,*value;
        yyjson_mut_obj_foreach(v,i,max,key,value) legacy(value);
    } else if(yyjson_mut_is_arr(v)) {
        size_t i,max; yyjson_mut_val *value;
        yyjson_mut_arr_foreach(v,i,max,value) legacy(value);
    }
}
typedef struct GeometryCapture { unsigned count; Position a,b; bool seen_a,seen_b; } GeometryCapture;
static bool capture_polygon(const Position *points,size_t count,Color color,void *context) {
    GeometryCapture *capture=context; ++capture->count;
    Shape shape={.amount_of_vertices=(uint16_t)count}; Position center={0};
    for(size_t i=0;i<count;++i) { shape.vertices[i]=points[i]; center.x+=points[i].x/count; center.y+=points[i].y/count; }
    if(color.red==255 && color.green==0 && color.blue==0) { capture->a=center; capture->seen_a=true; }
    if(color.red==0 && color.green==255 && color.blue==0) { capture->b=center; capture->seen_b=true; }
    bool ok=rohr_graphics_screen_shape_filled_draw(shape,color);
    if(!ok) fprintf(stderr,"marker polygon failed with %zu points and color %u,%u,%u\n",count,color.red,color.green,color.blue);
    return ok;
}
static void marker_geometry_check(void) {
    MarkerGeometry g={.kind=OBJECT_JOINT,.joint=JOINT_SPRING,.endpoint_a={-50,0},.endpoint_b={50,0},.magnitude=1};
    g.visuals.main=object_visual_style_get((ObjectVisualStyle){0},OBJECT_JOINT);
    g.visuals.endpoint_a=g.visuals.endpoint_b=g.visuals.main;
    g.visuals.endpoint_a.fill_color=0xff0000ff; g.visuals.endpoint_b.fill_color=0x00ff00ff;
    g.visuals.endpoint_a.offset=(Position){10,20}; g.visuals.endpoint_b.offset=(Position){-5,-10};
    g.visuals.endpoint_a.scale=(Scale){2,0.5f}; g.visuals.endpoint_a.rotation=45;
    g.visuals.main.scale=(Scale){2,3}; g.visuals.main.rotation=90; g.visuals.main.offset=(Position){7,11};
    GeometryCapture capture={0}; assert(graphics_marker_geometry_draw(&g,capture_polygon,&capture));
    assert(capture.seen_a && capture.seen_b);
    assert(fabsf(capture.a.x-67)<0.001f && fabsf(capture.a.y-91)<0.001f);
    assert(fabsf(capture.b.x+23)<0.001f && fabsf(capture.b.y+79)<0.001f);
    assert(g.endpoint_a.x==-50 && g.endpoint_b.x==50);
    for(unsigned kind=0;kind<4;++kind) {
        g=(MarkerGeometry){.kind=kind<2?OBJECT_FORCE:OBJECT_TORQUE,.magnitude=kind%2?-1:1};
        capture=(GeometryCapture){0}; assert(graphics_marker_geometry_draw(&g,capture_polygon,&capture)); assert(capture.count>0);
        g.visuals.main=object_visual_style_get(g.visuals.main,g.kind); g.visuals.main.scale.x=0;
        capture=(GeometryCapture){0}; assert(graphics_marker_geometry_draw(&g,capture_polygon,&capture)); assert(capture.count==0);
    }
    g=(MarkerGeometry){.kind=OBJECT_JOINT,.joint=JOINT_SPRING,.magnitude=1,.endpoint_b={20,0}};
    capture=(GeometryCapture){0}; assert(graphics_marker_geometry_draw(&g,capture_polygon,&capture));
    (void)rohr_graphics_pipeline_commands_execute();
}
static void visual_persistence_check(void) {
    EditorProject p={0},loaded={0}; editor_project_init(&p); editor_project_init(&loaded);
    EditorObject *o=editor_project_object_add(&p,(Position){0}); assert(o); strcpy(o->name,"styled");
    EditorForce *f=editor_project_force_add(&p,o); assert(f); strcpy(f->name,"force");
    EditorTorque *t=editor_project_torque_add(&p,o); assert(t); strcpy(t->name,"torque");
    EditorJoint *j=editor_project_joint_add(&p,o,EDITOR_JOINT_SPRING); assert(j);
    const char *path=NULL; EditorCommand command; char output[512];
    char *force_args[]={"rohr-cli","--project","/tmp/rohr_styles.json","--object","styled",
        "--force","force","--property","visual-fill-color","set","#11223344"};
    assert(!editor_result_check(editor_command_cli_standard_parse(&p,11,force_args,&path,&command)));
    assert(editor_command_execute(&p,&command).kind==ERROR_RESULT_VALUE);
    assert(f->visuals.main.fill_color==0x11223344 && f->visuals.main.scale.x==1);
    assert(!editor_result_check(editor_command_cli_standard_write(&p,&command,NULL,path,output,sizeof(output))));
    assert(strstr(output,"--force-id") && strstr(output,"visual-fill-color"));
    FILE *replay=fopen("/tmp/rohr_visual_command.txt","w"); assert(replay);
    assert(fputs(output,replay)>=0 && fclose(replay)==0);
    char *torque_args[]={"rohr-cli","--project","/tmp/rohr_styles.json","--object","styled",
        "--torque","torque","--property","visual-scale-x","set","-2"};
    assert(!editor_result_check(editor_command_cli_standard_parse(&p,11,torque_args,&path,&command)));
    assert(editor_command_execute(&p,&command).kind==ERROR_RESULT_VALUE);
    assert(t->visuals.main.scale.x==-2);
    char oid[24],jid[24]; snprintf(oid,sizeof(oid),"%u",o->id); snprintf(jid,sizeof(jid),"%u",j->id);
    char *joint_args[]={"rohr-cli","--project","/tmp/rohr_styles.json","--object-id",oid,
        "--joint-id",jid,"--property","visual-a-offset-x","12"};
    assert(!editor_result_check(editor_command_cli_standard_parse(&p,10,joint_args,&path,&command)));
    assert(editor_command_execute(&p,&command).kind==ERROR_RESULT_VALUE);
    assert(j->visuals.endpoint_a.offset.x==12 && !j->visuals.endpoint_b.specified);
    command.data.property_set.property=EDITOR_PROPERTY_VISUAL_BORDER_THICKNESS;
    command.data.property_set.value.number=-1;
    assert(editor_command_execute(&p,&command).kind==ERROR_RESULT_ERROR && !j->visuals.main.specified);
    command.data.property_set.value.number=NAN;
    assert(editor_command_execute(&p,&command).kind==ERROR_RESULT_ERROR && !j->visuals.main.specified);
    assert(editor_project_save(&p,"/tmp/rohr_styles.json"));
    assert(!editor_result_check(editor_project_load(&loaded,"/tmp/rohr_styles.json")));
    assert(loaded.objects[0].force_items[0].visuals.main.fill_color==0x11223344);
    assert(loaded.objects[0].torque_items[0].visuals.main.scale.x==-2);
    assert(loaded.objects[0].joint_items[0].visuals.endpoint_a.offset.x==12);
    yyjson_doc *doc=yyjson_read_file("/tmp/rohr_styles.json",0,NULL,NULL); assert(doc);
    yyjson_mut_doc *copy=yyjson_doc_mut_copy(doc,NULL); yyjson_doc_free(doc); assert(copy);
    yyjson_mut_val *object=yyjson_mut_arr_get_first(yyjson_mut_obj_get(yyjson_mut_doc_get_root(copy),"objects"));
    yyjson_mut_val *force=yyjson_mut_arr_get_first(yyjson_mut_obj_get(object,"forces")); assert(force);
    yyjson_mut_val *style=yyjson_mut_obj_get(yyjson_mut_obj_get(force,"visuals"),"main"); assert(style);
    yyjson_mut_obj_put(style,yyjson_mut_str(copy,"border_thickness"),yyjson_mut_real(copy,-1));
    assert(yyjson_mut_write_file("/tmp/rohr_styles_invalid.json",copy,0,NULL,NULL));
    assert(editor_result_check(editor_project_load(&loaded,"/tmp/rohr_styles_invalid.json")));
    assert(loaded.objects[0].force_items[0].visuals.main.fill_color==0x11223344);
    legacy(yyjson_mut_doc_get_root(copy));
    assert(yyjson_mut_write_file("/tmp/rohr_styles_legacy.json",copy,0,NULL,NULL));
    assert(!editor_result_check(editor_project_load(&loaded,"/tmp/rohr_styles_legacy.json")));
    assert(!loaded.objects[0].force_items[0].visuals.main.specified);
    assert(!loaded.objects[0].joint_items[0].visuals.endpoint_a.specified);
    yyjson_mut_doc_free(copy); editor_project_destroy(&loaded); editor_project_destroy(&p);
}
static void visual_color_open(void *context,uint32_t *color,EditorItemKind kind,
        EditorObjectId object,uint32_t parent,uint32_t item,EditorPropertyKind property) {
    (void)color;
    EditorCommand command={.type=EDITOR_COMMAND_PROPERTY_SET,.data.property_set={
        .kind=kind,.object=object,.parent=parent,.item=item,.property=property,
        .value_kind=EDITOR_PROPERTY_VALUE_UINT,.value.integer=0x11223344}};
    assert(editor_command_execute(context,&command).kind==ERROR_RESULT_VALUE);
}
static uint64_t drawn(Entity entity) {
    (void)rohr_graphics_pipeline_commands_execute();
    assert(rohr_entity_object_draw(entity));
    Report report=rohr_graphics_pipeline_commands_execute();
    assert(rohr_report_valid_check(&report) && !report.has_fatal);
    uint64_t count=0;
    for(size_t i=0;i<report.statistic_count;++i)
        if(report.statistics[i].report_id==ROHR_GRAPHICS_COMMANDS_REPORT_ID &&
                report.statistics[i].metric_id==ROHR_REPORT_PROCESSED_COUNT)
            count=report.statistics[i].value;
    return count;
}
static void source_runtime_check(void) {
    EditorProject p={0}; editor_project_init(&p);
    EditorObject *o=editor_project_object_add(&p,(Position){0}); assert(o);
    EditorRigidBody *b=editor_project_rigid_body_add(&p,o); assert(b);
    strcpy(b->name,"target"); b->app_hidden=true;
    EditorAnchor *a=editor_project_anchor_add(&p,o,(Position){0},b->id); assert(a);
    uint32_t anchor=a->id;
    a=editor_project_anchor_add(&p,o,(Position){20,0},0); assert(a);
    EditorJoint *j=editor_project_joint_add(&p,o,EDITOR_JOINT_SPRING); assert(j);
    j->anchor_a=anchor; j->anchor_b=a->id; j->visible=false; strcpy(j->name,"joint");
    EditorForce *f=editor_project_force_add(&p,o); assert(f); strcpy(f->name,"force");
    f->visible=false;
    f->description=(ObjectForceDescription){.target={OBJECT_SOURCE_BODY,b->id,0},.magnitude=-10,
        .location={.kind=ROHR_FORCE_ORIGIN_POSITION,.reference={OBJECT_SOURCE_BODY,b->id,0},.offset={2,3}},
        .orientation={.kind=ROHR_FORCE_BODY_ORIENTATION,.reference={OBJECT_SOURCE_BODY,b->id,0}}};
    EditorTorque *t=editor_project_torque_add(&p,o); assert(t); strcpy(t->name,"torque");
    t->description=(ObjectTorqueDescription){.target={OBJECT_SOURCE_BODY,b->id,0},.magnitude=-20};
    f->visuals.main=object_visual_style_get((ObjectVisualStyle){0},OBJECT_FORCE);
    f->visuals.main.scale=(Scale){2,3}; f->visuals.main.offset=(Position){8,9};
    j->visuals.endpoint_b=object_visual_style_get((ObjectVisualStyle){0},OBJECT_JOINT);
    j->visuals.endpoint_b.offset=(Position){4,5}; j->visuals.endpoint_b.fill_color=0xff0000ff;
    ObjectDescriptionResult d=object_description_project_read(&p,""); assert(!rohr_error_check(d));
    EntityResult root=rohr_entity_object_description_add(d.result.value->children[0],
        (ObjectSpawnDescription){.flags=ROHR_ENTITY_OBJECT_ALL}); assert(!rohr_error_check(root));
    Entity body=fixture_entity_get(root.result.value,"target").result.value;
    const char *names[]={"force","torque","joint"}; Entity entities[3];
    uint64_t total=0;
    for(size_t i=0;i<3;++i) {
        EntityResult e=fixture_entity_get(root.result.value,names[i]); assert(!rohr_error_check(e));
        entities[i]=e.result.value;
        ObjectAppearance appearance; assert(object_instance_appearance_get(e.result.value,&appearance));
        assert(appearance.enabled && appearance.visible && appearance.color.alpha);
        uint64_t count=drawn(e.result.value); assert(count>0); total+=count;
        appearance.visible=false; assert(!rohr_error_check(object_instance_appearance_set(e.result.value,appearance)));
        assert(drawn(e.result.value)==0);
        appearance.visible=true; assert(!rohr_error_check(object_instance_appearance_set(e.result.value,appearance)));
    }
    assert(drawn(root.result.value)==total);
    assert(!rohr_error_check(rohr_physics_force_description_get(entities[0])));
    assert(!rohr_error_check(rohr_physics_torque_description_get(entities[1])));
    Position point; Vec2D direction; float magnitude;
    assert(!rohr_error_check(rohr_physics_position_set(body,(Position){10,20})));
    assert(!rohr_error_check(rohr_physics_orientation_set(body,90)));
    assert(physics_source_geometry_get(entities[0],false,&point,&direction,&magnitude));
    assert(fabsf(point.x-13)<0.001f && fabsf(point.y-18)<0.001f);
    assert(fabsf(direction.x+1)<0.001f && fabsf(direction.y)<0.001f && magnitude==-10);
    assert(physics_source_geometry_get(entities[1],true,&point,&direction,&magnitude) && magnitude==-20);
    ObjectAppearance parent; assert(object_instance_appearance_get(root.result.value,&parent));
    parent.visible=false; assert(!rohr_error_check(object_instance_appearance_set(root.result.value,parent)));
    assert(drawn(root.result.value)==0 && drawn(entities[0])>0);
    EntityNameResult root_name=rohr_entity_name_get(root.result.value); assert(!rohr_error_check(root_name));
    assert(!rohr_error_check(rohr_game_state_file_save("/tmp/rohr_source_visibility_runtime.json")));
    assert(!rohr_error_check(rohr_entity_delete(root.result.value)));
    assert(!rohr_error_check(rohr_game_state_file_load("/tmp/rohr_source_visibility_runtime.json")));
    root=rohr_entity_by_name_get(root_name.result.value.value); assert(!rohr_error_check(root));
    for(size_t i=0;i<3;++i) entities[i]=fixture_entity_get(root.result.value,names[i]).result.value;
    assert(drawn(root.result.value)==0 && drawn(entities[0])>0 && drawn(entities[1])>0);
    ObjectAppearance restored; assert(object_instance_appearance_get(entities[0],&restored));
    assert(restored.visuals.main.scale.x==2 && restored.visuals.main.offset.y==9);
    assert(object_instance_appearance_get(entities[2],&restored));
    assert(restored.visuals.endpoint_b.offset.y==5 && restored.visuals.endpoint_b.fill_color==0xff0000ff);
    assert(!rohr_error_check(rohr_entity_delete(root.result.value)));
    for(size_t i=0;i<3;++i) assert(!rohr_entity_alive_check(entities[i]));
    root=rohr_entity_object_description_add(d.result.value->children[0],
        (ObjectSpawnDescription){.flags=ROHR_ENTITY_OBJECT_PHYSICS}); assert(!rohr_error_check(root));
    assert(drawn(root.result.value)==0);
    assert(!rohr_error_check(rohr_entity_delete(root.result.value)));
    object_description_tree_destroy((ObjectDescription *)d.result.value); editor_project_destroy(&p);
}
int main(void) {
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    marker_geometry_check();
    visual_persistence_check();
    source_runtime_check();
    EditorProject p={0}; editor_project_init(&p);
    EditorObject *o=editor_project_object_add(&p,(Position){0}); assert(o);
    EditorRigidBody *body=editor_project_rigid_body_add(&p,o); assert(body);
    snprintf(body->name,sizeof(body->name),"body");
    EditorCamera *camera=editor_project_camera_add(&p,o); assert(camera);
    snprintf(camera->name,sizeof(camera->name),"camera");
    o->visible=body->visible=camera->visible=false;
    runtime_check(&p,true); /* Eyes cannot disable rendering or physics. */
    camera->app_hidden=true; runtime_check(&p,false);
    camera->app_hidden=false; o->app_hidden=true; runtime_check(&p,false);
    assert(!camera->app_hidden); o->app_hidden=false;
    EditorLayoutViewport *vp=editor_project_layout_viewport_add(&p); assert(vp);
    EditorViewportCameraItem *screen=editor_viewport_camera_add(&p,vp,o->id,camera->id); assert(screen);
    EditorViewportUiItem *ui=editor_viewport_ui_add(&p,vp,EDITOR_VIEWPORT_UI_SHAPE); assert(ui);
    EditorSoftBody *soft=editor_project_soft_body_add(&p,o); assert(soft);
    EditorSoftNode *a=editor_project_soft_node_add(&p,soft,(Position){0}); assert(a); uint32_t aid=a->id;
    EditorSoftNode *b=editor_project_soft_node_add(&p,soft,(Position){20,0}); assert(b);
    EditorSoftBeam *beam=editor_project_soft_beam_add(&p,soft,aid,b->id); assert(beam);
    EditorSprite *sprite=editor_project_sprite_add(&p,o,"sprite","placeholder.png"); assert(sprite);
    EditorAnimatedSprite *animation=editor_project_animated_sprite_add(&p,o); assert(animation);
    EditorJoint *joint=editor_project_joint_add(&p,o,EDITOR_JOINT_REVOLUTE); assert(joint);
    EditorSoftArea *area=editor_soft_area_add(soft); assert(area);
    EditorForce *force=editor_project_force_add(&p,o); assert(force);
    EditorTorque *torque=editor_project_torque_add(&p,o); assert(torque);
    EditorSelectionRef refs[]={
        {EDITOR_SELECTION_JOINT,o->id,0,0,joint->id},
        {EDITOR_SELECTION_SOFT_AREA,o->id,soft->id,0,area->id},
        {EDITOR_SELECTION_OBJECT,o->id,0,0,o->id},
        {EDITOR_SELECTION_RIGID_BODY,o->id,0,0,body->id},
        {EDITOR_SELECTION_CAMERA,o->id,0,0,camera->id},
        {EDITOR_SELECTION_SOFT_BODY,o->id,0,0,soft->id},
        {EDITOR_SELECTION_SOFT_NODE,o->id,soft->id,0,aid},
        {EDITOR_SELECTION_SOFT_BEAM,o->id,soft->id,0,beam->id},
        {EDITOR_SELECTION_SPRITE,o->id,0,0,sprite->id},
        {EDITOR_SELECTION_ANIMATED_SPRITE,o->id,0,0,animation->id},
        {EDITOR_SELECTION_LAYOUT_VIEWPORT,0,0,0,vp->id},
        {EDITOR_SELECTION_LAYOUT_VIEWPORT,vp->id,1,0,screen->id},
        {EDITOR_SELECTION_UI_SHAPE,vp->id,0,0,ui->id},
        {EDITOR_SELECTION_FORCE,o->id,0,0,force->id}};
    EditorHistory history={0}; assert(editor_history_init(&history,&p));
    editor_command_executing_callback_set(begin,&history); editor_command_finished_callback_set(finish,&history);
    EditorVisualControls visuals={0}; FontAsset visual_font=rohr_graphics_font_default_get();
    assert(editor_visual_controls_create(&visuals,&visual_font));
    EditorModeContext visual_context={.project=&p,.history=&history,.width=300,.color_open=visual_color_open,.color_context=&p};
    for(unsigned step=0;step<2;++step) {
        EditorModeAccordionLayoutCursor cursor=editor_mode_accordion_layout_cursor_get(0,300,0);
        rohr_ui_input_set((UIInput){{200,50},step?MOUSE_BUTTON_STATE_RELEASED:MOUSE_BUTTON_STATE_PRESSED});
        (void)editor_visual_controls_draw(&visuals,&visual_context,&cursor,EDITOR_ITEM_FORCE,o->id,force->id,false);
        rohr_ui_update();
    }
    assert(force->visuals.main.specified && force->visuals.main.fill_color==0x11223344);
    assert(editor_history_undo(&history)); o=editor_project_selected_get(&p); force=&o->force_items[0];
    assert(!force->visuals.main.specified);
    assert(editor_history_redo(&history)); o=editor_project_selected_get(&p); force=&o->force_items[0];
    assert(force->visuals.main.fill_color==0x11223344);
    /* Aggregate history can replace child arrays; reacquire all borrowed model pointers. */
    body=&o->rigid_bodies[0]; camera=&o->cameras[0]; torque=&o->torque_items[0];
    editor_visual_controls_destroy(&visuals);
    EditorViewportState state; editor_viewport_state_init(&state);
    state.selected_camera_entity = camera->id;
    state.mode = EDITOR_VIEWPORT_CAMERA_ENTITY;
    state.selection = EDITOR_SELECTION_CAMERA;
    EditorSelectionRef eye_ref;
    assert(editor_navigation_panel_visibility_ref_get(&p,&state,&eye_ref));
    assert(eye_ref.kind==EDITOR_SELECTION_CAMERA && eye_ref.item==camera->id);
    /* Header eyes use the open editor even when child selection changes. */
    state.selection=EDITOR_SELECTION_RIGID_BODY;
    assert(editor_navigation_panel_visibility_ref_get(&p,&state,&eye_ref));
    assert(eye_ref.kind==EDITOR_SELECTION_CAMERA);
    assert(editor_navigation_selection_visibility_set(&p,eye_ref,true));
    assert(camera->visible && !camera->app_hidden);
    assert(editor_history_undo(&history)); assert(!camera->visible);
    state.selection=EDITOR_SELECTION_CAMERA;
    FontAsset font = rohr_graphics_font_default_get();
    EditorCameraEditor panel = {0}; assert(editor_camera_editor_create(&panel, &font));
    EditorModeContext context = {.project=&p, .viewport=&state, .history=&history, .width=300};
    /* The app checkbox is directly below the name, even with sections collapsed. */
    panel.transform_section.expanded = panel.view_section.expanded = panel.attachment_section.expanded = false;
    rohr_ui_input_set((UIInput){{20,90},MOUSE_BUTTON_STATE_PRESSED});
    (void)editor_camera_editor_draw(&panel,&context); rohr_ui_update();
    rohr_ui_input_set((UIInput){{20,90},MOUSE_BUTTON_STATE_RELEASED});
    (void)editor_camera_editor_draw(&panel,&context); rohr_ui_update();
    assert(camera->app_hidden && !camera->visible);
    assert(editor_history_undo(&history)); assert(!camera->app_hidden && !camera->visible);
    editor_camera_editor_destroy(&panel); editor_viewport_state_destroy(&state);
    assert(editor_history_transaction_begin(&history));
    for(size_t i=0;i<sizeof(refs)/sizeof(*refs);++i) {
        EditorCommand c; assert(editor_navigation_app_visibility_command_get(&p,refs[i],&c));
        assert(c.data.app_visibility.visible); c.data.app_visibility.visible=false;
        assert(editor_command_execute(&p,&c).kind==ERROR_RESULT_VALUE);
    }
    EditorCommand torque_visibility={.type=EDITOR_COMMAND_APP_VISIBILITY,
        .data.app_visibility={EDITOR_ITEM_TORQUE,o->id,0,torque->id,false}};
    assert(editor_command_execute(&p,&torque_visibility).kind==ERROR_RESULT_VALUE);
    assert(torque->app_hidden);
    assert(editor_history_transaction_end(&history));
    assert(editor_history_undo(&history)); assert(!torque->app_hidden);
    for(size_t i=0;i<sizeof(refs)/sizeof(*refs);++i) {
        EditorCommand c; assert(editor_navigation_app_visibility_command_get(&p,refs[i],&c));
        assert(c.data.app_visibility.visible);
    }
    assert(editor_history_redo(&history));
    assert(!o->visible && !body->visible && !camera->visible);
    /* No-op assignments must undo to their actual previous state. */
    EditorCommand c; assert(editor_navigation_app_visibility_command_get(&p,refs[4],&c));
    assert(!c.data.app_visibility.visible);
    size_t undo_count=history.undo_count;
    assert(editor_command_execute(&p,&c).kind==ERROR_RESULT_VALUE);
    assert(history.undo_count==undo_count && camera->app_hidden);
    assert(editor_project_save(&p,"/tmp/rohr_visibility_test.json"));
    EditorProject loaded={0}; editor_project_init(&loaded);
    assert(!editor_result_check(editor_project_load(&loaded,"/tmp/rohr_visibility_test.json")));
    for(size_t i=0;i<sizeof(refs)/sizeof(*refs);++i) {
        assert(editor_navigation_app_visibility_command_get(&loaded,refs[i],&c));
        assert(!c.data.app_visibility.visible);
    }
    assert(loaded.objects[0].torque_items[0].app_hidden);
    EditorProject clone={0}; assert(editor_project_clone(&clone,&loaded));
    assert(clone.objects[0].app_hidden && clone.objects[0].cameras[0].app_hidden);
    EditorObject *duplicate=editor_project_object_duplicate(&clone,clone.objects[0].id); assert(duplicate);
    assert(duplicate->app_hidden && duplicate->cameras[0].app_hidden && !duplicate->cameras[0].visible);
    duplicate->app_hidden=false; assert(clone.objects[0].app_hidden);
    clone.objects[0].app_hidden=false; assert(loaded.objects[0].app_hidden);
    editor_project_destroy(&clone); editor_project_destroy(&loaded);
    yyjson_doc *doc=yyjson_read_file("/tmp/rohr_visibility_test.json",0,NULL,NULL); assert(doc);
    yyjson_mut_doc *mutable=yyjson_doc_mut_copy(doc,NULL); assert(mutable); yyjson_doc_free(doc);
    legacy(yyjson_mut_doc_get_root(mutable));
    yyjson_mut_obj_put(yyjson_mut_doc_get_root(mutable),yyjson_mut_str(mutable,"format_version"),yyjson_mut_uint(mutable,EDITOR_PROJECT_FORMAT_VERSION));
    assert(yyjson_mut_write_file("/tmp/rohr_visibility_legacy_test.json",mutable,0,NULL,NULL));
    editor_project_init(&loaded);
    assert(!editor_result_check(editor_project_load(&loaded,"/tmp/rohr_visibility_legacy_test.json")));
    for(size_t i=0;i<sizeof(refs)/sizeof(*refs);++i) {
        assert(editor_navigation_app_visibility_command_get(&loaded,refs[i],&c)); assert(c.data.app_visibility.visible);
    }
    assert(!loaded.objects[0].cameras[0].visible);
    yyjson_mut_val *objects=yyjson_mut_obj_get(yyjson_mut_doc_get_root(mutable),"objects");
    yyjson_mut_obj_add_str(mutable,yyjson_mut_arr_get_first(objects),"visible_in_app","invalid");
    assert(yyjson_mut_write_file("/tmp/rohr_visibility_invalid_test.json",mutable,0,NULL,NULL));
    assert(editor_result_check(editor_project_load(&loaded,"/tmp/rohr_visibility_invalid_test.json")));
    assert(loaded.object_count==1 && !loaded.objects[0].app_hidden);
    yyjson_mut_doc_free(mutable);
    char kind[16],object[16],item[16],text[512]; const char *path=NULL;
    snprintf(kind,sizeof(kind),"%u",EDITOR_ITEM_CAMERA); snprintf(object,sizeof(object),"%u",o->id); snprintf(item,sizeof(item),"%u",camera->id);
    char *args[]={"rohr-cli","--project","/tmp/project.json","--app-visibility",kind,object,"0",item,"set","false"};
    assert(!editor_result_check(editor_command_cli_standard_parse(&p,10,args,&path,&c)));
    assert(c.type==EDITOR_COMMAND_APP_VISIBILITY && c.data.app_visibility.item==camera->id);
    assert(!editor_result_check(editor_command_cli_standard_write(&p,&c,NULL,path,text,sizeof(text))));
    assert(strstr(text,"--app-visibility") && strstr(text,"set false"));
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&history); editor_project_destroy(&loaded); editor_project_destroy(&p);
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
