/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_history.h"
#include "editor_workspace.h"
#include "editor_navigation.h"
#include "editor_layout.h"
#include "editors/physics/editor_joint.h"
#include "entity/object_internal.h"
#include "yyjson/yyjson.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define OK(expression) do { assert(!error_check(expression)); } while(0)
float editor_viewport_width=1024,editor_window_width=1280,editor_window_height=720,editor_viewport_bottom=720;
static void begin(const EditorProject *p,const EditorCommand *c,void *h) {editor_history_command_begin(h,p,c);}
static void finish(const EditorCommand *c,const EditorCommandResult *r,void *h) {editor_history_command_finish(h,c,r);}
static void near(float a,float b) {assert(fabsf(a-b)<.001f);}
static EditorObject *pair(EditorProject *p,EditorJointKind kind,const char *name) {
    EditorObject *o=editor_project_object_add(p,(Position){0});assert(o);snprintf(o->name,sizeof(o->name),"%s",name);
    EditorRigidBody *a=editor_project_rigid_body_add(p,o);assert(a);EditorRigidBodyId aid=a->id;
    EditorRigidBody *b=editor_project_rigid_body_add(p,o);assert(b);a=editor_project_rigid_body_get(o,aid);
    snprintf(a->name,sizeof(a->name),"first");snprintf(b->name,sizeof(b->name),"second");
    a->position=(Position){-100,0};b->position=(Position){100,0};
    a->initial_velocity=(Velocity){3,4};a->gravity_enabled=false;b->gravity_enabled=false;
    EditorAnchor *aa=editor_project_anchor_add(p,o,(Position){0},aid);assert(aa);EditorAnchorId aa_id=aa->id;
    EditorAnchor *bb=editor_project_anchor_add(p,o,(Position){0},b->id);assert(bb);
    EditorJoint *j=editor_project_joint_add(p,o,kind);assert(j);snprintf(j->name,sizeof(j->name),"connection");
    assert(editor_project_joint_anchor_set(o,j,0,aa_id));assert(editor_project_joint_anchor_set(o,j,1,bb->id));
    return o;
}
static EditorCommand property(EditorObject *o,EditorPropertyKind kind,float value) {
    return (EditorCommand){.type=EDITOR_COMMAND_PROPERTY_SET,.data.property_set={.kind=EDITOR_ITEM_JOINT,
        .object=o->id,.item=o->joint_items[0].id,.property=kind,.value_kind=EDITOR_PROPERTY_VALUE_FLOAT,.value.number=value}};
}
static EditorCommand move(EditorObject *o,unsigned index,float x,float y,float angle) {
    return (EditorCommand){.type=EDITOR_COMMAND_RIGID_BODY_TRANSFORM,.data.rigid_body_transform={o->id,o->rigid_bodies[index].id,{x,y},angle}};
}
static EditorCommand lock(EditorObject *o,bool value) {
    EditorCommand c=property(o,EDITOR_PROPERTY_EDITOR_REST_LENGTH_LOCKED,0);
    c.data.property_set.value_kind=EDITOR_PROPERTY_VALUE_BOOL;c.data.property_set.value.boolean=value;return c;
}
static void history_check(void) {
    static EditorProject p;editor_project_init(&p);EditorObject *o=pair(&p,EDITOR_JOINT_WELD,"Weld");
    EditorHistory h;assert(editor_history_init(&h,&p));
    editor_command_executing_callback_set(begin,&h);editor_command_finished_callback_set(finish,&h);
    EditorCommand c=property(o,EDITOR_PROPERTY_REST_ANGLE,30);OK(editor_command_execute(&p,&c));
    near(o->rigid_bodies[0].position.x,100);near(o->rigid_bodies[0].rotation,-30);
    near(o->rigid_bodies[0].initial_velocity.x,3);
    assert(editor_history_undo(&h));o=&p.objects[0];near(o->rigid_bodies[0].position.x,-100);near(o->joint_items[0].rest_angle,0);
    assert(editor_history_redo(&h));o=&p.objects[0];near(o->rigid_bodies[0].rotation,-30);
    assert(editor_history_transaction_begin(&h));
    for(int i=1;i<=3;++i) {c=move(o,0,100+i*10,20,10);OK(editor_command_execute(&p,&c));}
    assert(editor_history_transaction_end(&h));near(o->rigid_bodies[1].position.x,130);near(o->rigid_bodies[1].rotation,40);
    assert(editor_history_undo(&h));o=&p.objects[0];near(o->rigid_bodies[0].position.x,100);near(o->rigid_bodies[1].rotation,0);
    assert(editor_history_redo(&h));o=&p.objects[0];near(o->rigid_bodies[1].position.x,130);
    assert(editor_history_transaction_begin(&h));assert(editor_history_transaction_object_track(&h,o->id));
    c=move(o,1,170,40,60);OK(editor_command_execute(&p,&c));editor_history_transaction_cancel(&h);
    o=&p.objects[0];near(o->rigid_bodies[1].position.x,130);
    editor_command_executing_callback_set(NULL,NULL);editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&h);editor_project_destroy(&p);
}
static void constraints_check(void) {
    for(unsigned scenario=0;scenario<6;++scenario) {
        static EditorProject p;editor_project_init(&p);EditorObject *o=pair(&p,EDITOR_JOINT_REVOLUTE,"Pins");
        EditorCommand c=move(o,0,-50,10,0);OK(editor_command_execute(&p,&c));near(o->rigid_bodies[1].position.x,-50);
        EditorRigidBody *b=&o->rigid_bodies[1];
        if(scenario==0) {b->entity_settings.axis_lock_enabled=true;b->entity_settings.axis_lock=(AxisLockDescription){{0,1},{-50,0}};}
        if(scenario==1) {b->entity_settings.axis_lock_enabled=true;b->entity_settings.axis_lock=(AxisLockDescription){{1,0},{0,10}};}
        if(scenario==2) {o->anchors[1].position_follows_body=false;o->anchors[1].position=(Position){-50,10};}
        if(scenario==3) {b->entity_settings.transform_driver=(ObjectEntityReference){OBJECT_CONTAINER,o->id,0};b->entity_settings.transform_position=true;b->entity_settings.transform_offset=b->position;}
        if(scenario==4) {o->joint_items[0].kind=EDITOR_JOINT_WELD;b->rotation_locked=true;}
        if(scenario==5) {
            EditorJoint *other=editor_project_joint_add(&p,o,EDITOR_JOINT_WELD);assert(other);
            other->anchor_a=o->anchors[0].id;other->anchor_b=o->anchors[1].id;other->rest_angle=45;other->rest_angle_explicit=true;
        }
        c=move(o,0,-20,30,20);EditorCommandResult r=editor_command_execute(&p,&c);
        assert(r.kind==ERROR_RESULT_ERROR && r.placement.blocked && r.placement.reason[0]);
        near(o->rigid_bodies[0].position.x,-50);near(b->position.x,-50);near(b->position.y,10);
        editor_project_destroy(&p);
    }
    static EditorProject p;editor_project_init(&p);EditorObject *o=pair(&p,EDITOR_JOINT_WELD,"Blocked");
    o->rigid_bodies[0].rotation_locked=true;EditorCommand c=property(o,EDITOR_PROPERTY_REST_ANGLE,45);
    EditorCommandResult r=editor_command_execute(&p,&c);OK(r);assert(r.placement.blocked);
    near(o->joint_items[0].rest_angle,45);near(o->rigid_bodies[0].position.x,-100);
    o->rigid_bodies[0].rotation_locked=false;r=editor_command_execute(&p,&c);OK(r);assert(!r.placement.blocked);
    near(o->rigid_bodies[0].rotation,-45);
    o->joint_items[0].anchor_b=0;c=property(o,EDITOR_PROPERTY_REST_ANGLE,90);r=editor_command_execute(&p,&c);OK(r);assert(!r.placement.blocked);
    c=(EditorCommand){.type=EDITOR_COMMAND_RELATIONSHIP_SET,.data.relationship_set={EDITOR_RELATIONSHIP_JOINT_ANCHOR,o->id,0,o->joint_items[0].id,1,o->anchors[1].id}};
    OK(editor_command_execute(&p,&c));near(o->rigid_bodies[0].rotation,-90);
    editor_project_destroy(&p);
}
static void static_authoring_check(void) {
    static EditorProject p;editor_project_init(&p);EditorObject *o=pair(&p,EDITOR_JOINT_WELD,"StaticEditing");
    o->rigid_bodies[0].static_body=true;o->rigid_bodies[1].static_body=true;
    EditorCommand c=property(o,EDITOR_PROPERTY_REST_ANGLE,30);EditorCommandResult r=editor_command_execute(&p,&c);OK(r);assert(!r.placement.blocked);
    c=move(o,1,50,20,40);r=editor_command_execute(&p,&c);OK(r);assert(!r.placement.blocked);
    near(o->rigid_bodies[0].position.x,50);near(o->rigid_bodies[0].rotation,10);
    assert(o->rigid_bodies[0].static_body && o->rigid_bodies[1].static_body);
    editor_project_destroy(&p);
}
static void axis_drag_check(void) {
    static EditorProject p;editor_project_init(&p);EditorObject *o=pair(&p,EDITOR_JOINT_REVOLUTE,"Axis");
    EditorCommand c=move(o,0,10,20,0);OK(editor_command_execute(&p,&c));
    ObjectEntitySettings *settings=&o->rigid_bodies[1].entity_settings;
    settings->axis_lock_enabled=true;
    settings->axis_lock=(AxisLockDescription){{2,1},{10,20}};
    /* Projection onto an arbitrary shifted line, applied through the connected peer. */
    c=move(o,0,20,40,0);c.pointer_transform=true;OK(editor_command_execute(&p,&c));
    near(o->rigid_bodies[0].position.x,26);near(o->rigid_bodies[0].position.y,28);
    near(o->rigid_bodies[1].position.x,26);near(o->rigid_bodies[1].position.y,28);
    /* The same exact field/CLI request is rejected, rather than silently altered. */
    c=move(o,0,20,40,0);assert(editor_command_execute(&p,&c).kind==ERROR_RESULT_ERROR);
    settings->axis_lock=(AxisLockDescription){{0,4},{26,0}};
    c.pointer_transform=true;OK(editor_command_execute(&p,&c));
    near(o->rigid_bodies[0].position.x,26);near(o->rigid_bodies[0].position.y,40);
    EditorCommand angle={.type=EDITOR_COMMAND_PROPERTY_SET,.data.property_set={
        .kind=EDITOR_ITEM_RIGID_BODY,.object=o->id,.item=o->rigid_bodies[1].id,
        .property=EDITOR_PROPERTY_AXIS_ANGLE,.value_kind=EDITOR_PROPERTY_VALUE_FLOAT,.value.number=90}};
    OK(editor_command_execute(&p,&angle));near(settings->axis_lock.axis.x,1);near(settings->axis_lock.axis.y,0);
    near(o->rigid_bodies[0].position.y,0);
    angle.data.property_set.value.number=0;OK(editor_command_execute(&p,&angle));
    char *argv[]={"rohr-cli","rigid-body","set","axis.json","1","1","axis-angle","45"};
    const char *path;EditorCommand parsed;OK(editor_command_cli_parse(8,argv,&path,&parsed));
    assert(parsed.data.property_set.property==EDITOR_PROPERTY_AXIS_ANGLE);
    char output[512];OK(editor_command_cli_write(&parsed,path,output,sizeof(output)));assert(strstr(output,"axis-angle"));
    /* Parallel incompatible lines reject the entire step. */
    o->rigid_bodies[0].entity_settings.axis_lock_enabled=true;
    o->rigid_bodies[0].entity_settings.axis_lock=(AxisLockDescription){{0,1},{27,0}};
    c=move(o,0,50,60,0);c.pointer_transform=true;
    assert(editor_command_execute(&p,&c).kind==ERROR_RESULT_ERROR);
    near(o->rigid_bodies[0].position.x,26);near(o->rigid_bodies[0].position.y,0);
    /* Axes also work on an isolated body and keep world-fixed point semantics. */
    o->joint_count=0;o->position=(Position){100,200};
    o->rigid_bodies[0].entity_settings.axis_lock=(AxisLockDescription){{1,0},{0,240}};
    o->rigid_bodies[0].entity_settings.axis_lock_world_fixed=true;
    OK(editor_command_execute(&p,&c));near(o->rigid_bodies[0].position.x,50);near(o->rigid_bodies[0].position.y,40);
    editor_project_destroy(&p);
}
static void batch_check(void) {
    static EditorProject p;editor_project_init(&p);EditorObject *o=pair(&p,EDITOR_JOINT_WELD,"Batch");
    EditorCommand c=move(o,0,0,0,0);OK(editor_command_execute(&p,&c));
    EditorCommand parts[]={move(o,0,20,30,10),move(o,1,20,30,10)};
    c=(EditorCommand){.type=EDITOR_COMMAND_TRANSFORM_BATCH,.data.transform_batch={o->id,parts,2}};
    OK(editor_command_execute(&p,&c));near(o->rigid_bodies[0].position.x,20);near(o->rigid_bodies[1].position.x,20);
    parts[0]=move(o,0,40,30,10);parts[1]=move(o,1,50,30,10);
    EditorCommandResult r=editor_command_execute(&p,&c);assert(r.kind==ERROR_RESULT_ERROR);
    near(o->rigid_bodies[0].position.x,20);near(o->rigid_bodies[1].position.x,20);
    editor_project_destroy(&p);
}
static void soft_check(void) {
    static EditorProject p;editor_project_init(&p);EditorObject *o=pair(&p,EDITOR_JOINT_REVOLUTE,"Soft");
    EditorSoftBody *s=editor_project_soft_body_add(&p,o);assert(s);
    EditorSoftNode *n=editor_project_soft_node_add(&p,s,(Position){10,0});assert(n);EditorSoftNodeId nid=n->id;
    assert(editor_project_soft_node_add(&p,s,(Position){20,0}));
    assert(editor_project_anchor_soft_node_set(o,&o->anchors[1],s->id,nid));
    EditorCommand c=move(o,0,50,30,0);OK(editor_command_execute(&p,&c));
    near(s->position.x,40);near(s->nodes[0].position.x,10);near(s->nodes[1].position.x,20);
    c=(EditorCommand){.type=EDITOR_COMMAND_SOFT_NODE_POSITION,.data.soft_node_position={o->id,s->id,nid,{15,0}}};
    OK(editor_command_execute(&p,&c));near(o->rigid_bodies[0].position.x,55);
    c=(EditorCommand){.type=EDITOR_COMMAND_SOFT_BODY_TRANSFORM,.data.soft_body_transform={o->id,s->id,{70,50},0}};
    OK(editor_command_execute(&p,&c));near(o->rigid_bodies[0].position.x,85);
    editor_project_destroy(&p);
}
static void spring_check(void) {
    static EditorProject p;editor_project_init(&p);EditorObject *o=pair(&p,EDITOR_JOINT_SPRING,"Spring");
    EditorCommand c=property(o,EDITOR_PROPERTY_REST_LENGTH,80);OK(editor_command_execute(&p,&c));near(o->rigid_bodies[0].position.x,-100);
    c=lock(o,true);OK(editor_command_execute(&p,&c));near(o->rigid_bodies[0].position.x,20);
    c=move(o,0,-50,0,0);OK(editor_command_execute(&p,&c));near(o->rigid_bodies[1].position.x,30);
    c=lock(o,false);OK(editor_command_execute(&p,&c));c=move(o,0,-100,0,0);OK(editor_command_execute(&p,&c));near(o->rigid_bodies[1].position.x,30);
    c=lock(o,true);OK(editor_command_execute(&p,&c));assert(editor_project_save(&p,"joint_authoring.json"));
    static EditorProject loaded;OK(editor_project_load(&loaded,"joint_authoring.json"));assert(loaded.objects[0].joint_items[0].editor_rest_length_locked);
    EditorObject *copy=editor_project_object_duplicate(&loaded,loaded.objects[0].id);assert(copy && copy->joint_items[0].editor_rest_length_locked);
    editor_project_destroy(&loaded);
    char *args[]={"rohr-cli","joint","set","joint_authoring.json","1","1","lock-editor-restlength","true"};
    const char *path;OK(editor_command_cli_parse(8,args,&path,&c));char text[512];OK(editor_command_cli_write(&c,path,text,sizeof(text)));assert(strstr(text,"lock-editor-restlength"));
    yyjson_doc *doc=yyjson_read_file("joint_authoring.json",0,NULL,NULL);assert(doc);
    for(unsigned version=12;version<=13;++version) for(unsigned invalid=0;invalid<2;++invalid) {
        yyjson_mut_doc *m=yyjson_doc_mut_copy(doc,NULL);yyjson_mut_val *root=yyjson_mut_doc_get_root(m);
        yyjson_mut_set_uint(yyjson_mut_obj_get(root,"format_version"),version);
        yyjson_mut_val *object=yyjson_mut_arr_get(yyjson_mut_obj_get(root,"objects"),0);
        yyjson_mut_val *joint=yyjson_mut_arr_get(yyjson_mut_obj_get(object,"joints"),0);
        yyjson_mut_obj_remove_key(joint,"editor_rest_length_locked");
        if(invalid) yyjson_mut_obj_add_str(m,joint,"editor_rest_length_locked","bad");
        assert(yyjson_mut_write_file("joint_variant.json",m,0,NULL,NULL));yyjson_mut_doc_free(m);
        EditorResult r=editor_project_load(&loaded,"joint_variant.json");
        if(invalid) assert(error_check(r));else {OK(r);assert(!loaded.objects[0].joint_items[0].editor_rest_length_locked);}
        editor_project_destroy(&loaded);
    }
    yyjson_doc_free(doc);editor_project_destroy(&p);remove("joint_authoring.json");remove("joint_variant.json");
    EditorSpringGuide g=editor_project_spring_guide_get((Position){-100,0},(Position){100,0},80);
    assert(g.stretched && !g.compressed);near(g.rest_start.x,-40);near(g.rest_end.x,40);
    g=editor_project_spring_guide_get((Position){0,-10},(Position){0,10},80);assert(g.compressed);near(g.rest_start.y,-40);near(g.rest_end.y,40);
    g=editor_project_spring_guide_get((Position){0},(Position){0},80);near(g.rest_end.y,40);
    g=editor_project_spring_guide_get((Position){0},(Position){0},0);assert(!g.stretched && !g.compressed);
}
static void joint_panel(EditorJointEditor *ui,EditorModeContext *c,Position point,MouseButtonState button) {
    rohr_ui_input_set((UIInput){point,button});editor_mode_accordion_layout_measure_reset();
    (void)editor_joint_editor_draw(ui,c);rohr_ui_update();
}
static void pointer_update(EditorProject *p,EditorHistory *h,EditorViewportState *v,Position world,MouseButtonState button) {
    Position point={editor_viewport_width*.5f+world.x,
        EDITOR_MENU_HEIGHT+(editor_viewport_bottom-EDITOR_MENU_HEIGHT)*.5f-world.y};
    bool active=editor_viewport_transform_active_check(v);
    (void)editor_viewport_update(v,p,point,button,MOUSE_BUTTON_STATE_UP,false,0,false);
    assert(editor_navigation_viewport_transform_history_update(p,v,h,active));
}
static void pointer_check(void) {
    OK(rohr_graphics_start());FontAsset font=rohr_graphics_font_default_get();
    static EditorProject p;editor_project_init(&p);EditorObject *o=pair(&p,EDITOR_JOINT_SPRING,"Pointer");
    o->joint_items[0].rest_length=80;o->joint_items[0].rest_length_explicit=true;
    EditorHistory h;assert(editor_history_init(&h,&p));
    editor_command_executing_callback_set(begin,&h);editor_command_finished_callback_set(finish,&h);
    EditorJointEditor ui;assert(editor_joint_editor_create(&ui,&font));
    editor_mode_accordion_section_expanded_set(&ui.visuals.outer,false);
    editor_mode_accordion_section_expanded_set(&ui.configuration_section,false);
    editor_mode_accordion_section_expanded_set(&ui.connections_section,false);
    editor_mode_accordion_section_expanded_set(&ui.parameters_section,true);
    EditorViewportState v={0};editor_viewport_state_init(&v);
    assert(editor_viewport_selection_set(&p,&v,(EditorSelectionRef){EDITOR_SELECTION_JOINT,o->id,0,0,o->joint_items[0].id},false));
    assert(editor_navigation_selected_open(&p,&v));p.navigation=editor_navigation_state_get(&p,&v);
    EditorModeContext c={.project=&p,.history=&h,.viewport=&v,.width=400};
    joint_panel(&ui,&c,(Position){0},MOUSE_BUTTON_STATE_UP);
    joint_panel(&ui,&c,(Position){100,396},MOUSE_BUTTON_STATE_PRESSED);
    joint_panel(&ui,&c,(Position){100,396},MOUSE_BUTTON_STATE_RELEASED);
    assert(o->joint_items[0].editor_rest_length_locked);near(o->rigid_bodies[0].position.x,20);
    assert(editor_history_undo(&h));o=&p.objects[0];assert(!o->joint_items[0].editor_rest_length_locked);near(o->rigid_bodies[0].position.x,-100);
    assert(editor_history_redo(&h));o=&p.objects[0];near(o->rigid_bodies[0].position.x,20);
    editor_joint_editor_destroy(&ui);editor_mode_named_text_clear_all();
    o->joint_items[0].editor_rest_length_locked=false;
    o->joint_items[0].kind=EDITOR_JOINT_REVOLUTE;
    o->anchors[0].position=(Position){90,0};o->anchors[1].position=(Position){-90,0};
    EditorCommand place=move(o,0,-100,0,0);OK(editor_command_execute(&p,&place));
    editor_viewport_state_destroy(&v);editor_viewport_state_init(&v);
    assert(editor_viewport_selection_set(&p,&v,(EditorSelectionRef){EDITOR_SELECTION_RIGID_BODY,o->id,0,0,o->rigid_bodies[0].id},false));
    assert(editor_navigation_selected_open(&p,&v));
    size_t undo_count=h.undo_count;
    pointer_update(&p,&h,&v,(Position){-130,20},MOUSE_BUTTON_STATE_PRESSED);assert(v.dragged_body);
    pointer_update(&p,&h,&v,(Position){-110,30},MOUSE_BUTTON_STATE_DOWN);
    near(o->rigid_bodies[0].position.x,-80);near(o->rigid_bodies[1].position.x,100);
    o->rigid_bodies[1].entity_settings.axis_lock_enabled=true;
    o->rigid_bodies[1].entity_settings.axis_lock=(AxisLockDescription){{1,0},{0,10}};
    pointer_update(&p,&h,&v,(Position){-90,40},MOUSE_BUTTON_STATE_DOWN);near(o->rigid_bodies[0].position.x,-60);near(o->rigid_bodies[0].position.y,10);
    o->rigid_bodies[1].entity_settings.axis_lock_enabled=false;
    pointer_update(&p,&h,&v,(Position){-100,35},MOUSE_BUTTON_STATE_DOWN);near(o->rigid_bodies[0].position.x,-70);
    pointer_update(&p,&h,&v,(Position){-100,35},MOUSE_BUTTON_STATE_RELEASED);
    assert(h.undo_count==undo_count+1);assert(editor_history_undo(&h));o=&p.objects[0];near(o->rigid_bodies[0].position.x,-100);near(o->rigid_bodies[1].position.x,80);
    editor_command_executing_callback_set(NULL,NULL);editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&h);editor_viewport_state_destroy(&v);editor_project_destroy(&p);rohr_graphics_stop();
}
int main(int argc,char **argv) {
    OK(rohr_engine_start());history_check();constraints_check();static_authoring_check();axis_drag_check();batch_check();soft_check();spring_check();pointer_check();
    if(argc==2) {
        static EditorProject p;EditorWorkspace w={0};editor_project_init(&p);OK(editor_workspace_create(&w,&p,argv[1]));
        EditorObject *o=pair(&p,EDITOR_JOINT_REVOLUTE,"JointPins");
        o->anchors[0].position=(Position){90,0};o->anchors[1].position=(Position){-90,0};
        EditorCommand c=move(o,0,-100,0,0);OK(editor_command_execute(&p,&c));
        o=pair(&p,EDITOR_JOINT_WELD,"JointWeld");
        o->anchors[0].position=(Position){90,0};o->anchors[1].position=(Position){-90,0};
        c=property(o,EDITOR_PROPERTY_REST_ANGLE,30);OK(editor_command_execute(&p,&c));
        o=pair(&p,EDITOR_JOINT_SPRING,"JointSpring");c=property(o,EDITOR_PROPERTY_REST_LENGTH,120);OK(editor_command_execute(&p,&c));
        o->anchors[0].position=(Position){60,0};o->anchors[1].position=(Position){-60,0};
        o=pair(&p,EDITOR_JOINT_REVOLUTE,"JointAxisLocked");
        o->anchors[0].position=(Position){90,0};o->anchors[1].position=(Position){-90,0};
        c=move(o,0,-100,0,0);OK(editor_command_execute(&p,&c));
        o->rigid_bodies[1].entity_settings.axis_lock_enabled=true;
        o->rigid_bodies[1].entity_settings.axis_lock=(AxisLockDescription){{1,0},{0,0}};
        o=pair(&p,EDITOR_JOINT_REVOLUTE,"JointStatic");
        o->anchors[0].position=(Position){90,0};o->anchors[1].position=(Position){-90,0};
        o->rigid_bodies[1].static_body=true;c=move(o,0,-100,0,0);OK(editor_command_execute(&p,&c));
        o=pair(&p,EDITOR_JOINT_SPRING,"JointLockedSpring");
        c=property(o,EDITOR_PROPERTY_REST_LENGTH,80);OK(editor_command_execute(&p,&c));
        c=lock(o,true);OK(editor_command_execute(&p,&c));
        o->joint_items[0].stiffness=0;o->joint_items[0].damping=0;
        for(size_t i=0;i<2;++i) {o->rigid_bodies[i].collision_enabled=false;o->rigid_bodies[i].initial_velocity=(Velocity){0};}
        p.selected=o->id;assert(editor_workspace_save(&w,&p) && editor_workspace_c_generate(&w,&p));editor_project_destroy(&p);
    }
    rohr_engine_stop();puts("joint authoring passed");return 0;
}
