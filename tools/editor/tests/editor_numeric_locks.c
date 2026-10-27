/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only */
#include "editor_numeric.h"
#include "editor_history.h"
#include "editor_workspace.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#define OK(x) assert(!error_check(x))
static void begin(const EditorProject *p,const EditorCommand *c,void *h) {editor_history_command_begin(h,p,c);}
static void finish(const EditorCommand *c,const EditorCommandResult *r,void *h) {editor_history_command_finish(h,c,r);}
static EditorCommand lock(EditorNumericTarget target,const char *key,bool enabled) {
    EditorCommand c={.type=EDITOR_COMMAND_NUMERIC_LOCK};c.data.numeric_lock.target=target;
    c.data.numeric_lock.locked=enabled;snprintf(c.data.numeric_lock.property,sizeof(c.data.numeric_lock.property),"%s",key);return c;
}
static void connected_and_independent(void) {
    static EditorProject p;editor_project_init(&p);
    EditorObject *o=editor_project_object_add(&p,(Position){0});assert(o);
    for(size_t i=0;i<3;++i) assert(editor_project_rigid_body_add(&p,o));
    for(size_t i=0;i<3;++i) o->rigid_bodies[i].position=(Position){0};
    EditorAnchor *a=editor_project_anchor_add(&p,o,(Position){0},o->rigid_bodies[0].id);assert(a);uint32_t aid=a->id;
    EditorAnchor *b=editor_project_anchor_add(&p,o,(Position){0},o->rigid_bodies[1].id);assert(b);
    EditorJoint *joint=editor_project_joint_add(&p,o,EDITOR_JOINT_WELD);assert(joint);joint->anchor_a=aid;joint->anchor_b=b->id;joint->rest_angle=0;
    EditorNumericTarget target={.kind=EDITOR_ITEM_RIGID_BODY,.object=o->id,.item=o->rigid_bodies[1].id};
    assert(editor_numeric_lock_set(&p,target,"position.x",true));
    EditorCommand move={.type=EDITOR_COMMAND_RIGID_BODY_TRANSFORM,.pointer_transform=true,
        .data.rigid_body_transform={o->id,o->rigid_bodies[0].id,{20,30},0}};
    EditorCommandResult result=editor_command_execute(&p,&move);OK(result);
    assert(o->rigid_bodies[0].position.x==0 && o->rigid_bodies[1].position.x==0);
    assert(o->rigid_bodies[0].position.y==30 && o->rigid_bodies[1].position.y==30);
    assert(result.numeric_blocked);
    assert(editor_numeric_lock_set(&p,target,"rotation",true));
    EditorCommand moves[]={move,move};moves[0].data.rigid_body_transform.rotation=45;
    moves[1].data.rigid_body_transform.body=o->rigid_bodies[2].id;moves[1].data.rigid_body_transform.position=(Position){90,40};
    EditorCommand batch={.type=EDITOR_COMMAND_TRANSFORM_BATCH,.pointer_transform=true,.data.transform_batch={o->id,moves,2}};
    result=editor_command_execute(&p,&batch);OK(result);assert(result.numeric_blocked);
    assert(o->rigid_bodies[0].rotation==0 && o->rigid_bodies[1].rotation==0);
    assert(o->rigid_bodies[2].position.x==90 && o->rigid_bodies[2].position.y==40);
    editor_project_destroy(&p);
}
static void compound_configuration(void) {
    static EditorProject p;editor_project_init(&p);
    EditorNumericTarget target={.kind=EDITOR_ITEM_PROJECT};
    assert(editor_numeric_lock_set(&p,target,"physics_gravity.x",true));
    float before=p.physics_gravity.x;
    EditorCommand command={.type=EDITOR_COMMAND_PROJECT_PHYSICS_SETTINGS_SET,
        .data.project_physics_settings_set={p.engine_time_per_tick,p.physics_timestep_override,
            p.physics_dt_per_tick,p.physics_substeps,{before+10,23},p.physics_solver_iterations}};
    EditorCommandResult result=editor_command_execute(&p,&command);OK(result);
    assert(result.numeric_blocked==1 && p.physics_gravity.x==before && p.physics_gravity.y==23);
    /* A local node coordinate stays fixed while its owning body moves. */
    EditorObject *object=editor_project_object_add(&p,(Position){0});assert(object);
    EditorSoftBody *body=editor_project_soft_body_add(&p,object);assert(body);
    EditorSoftNode *node=editor_project_soft_node_add(&p,body,(Position){10,20});assert(node);
    target=(EditorNumericTarget){.kind=EDITOR_ITEM_SOFT_NODE,.object=object->id,.parent=body->id,.item=node->id};
    assert(editor_numeric_lock_set(&p,target,"position.x",true));
    command=(EditorCommand){.type=EDITOR_COMMAND_SOFT_BODY_TRANSFORM,.pointer_transform=true,
        .data.soft_body_transform={object->id,body->id,{30,40},45}};
    OK(editor_command_execute(&p,&command));assert(node->position.x==10 && node->position.y==20);
    EditorNumericTarget malformed=target;malformed.container=3;
    assert(!editor_numeric_lock_set(&p,malformed,"position.x",true));
    uint32_t node_id=node->id;float friction=node->friction;
    assert(editor_numeric_lock_set(&p,target,"friction",true));
    assert(editor_project_soft_node_add(&p,body,(Position){30,40}));
    command=(EditorCommand){.type=EDITOR_COMMAND_PROPERTY_SET,.data.property_set={.kind=EDITOR_ITEM_SOFT_BODY,
        .object=object->id,.item=body->id,.property=EDITOR_PROPERTY_FRICTION,.value_kind=EDITOR_PROPERTY_VALUE_FLOAT,.value.number=friction+1}};
    result=editor_command_execute(&p,&command);OK(result);assert(result.numeric_blocked==1);
    assert(body->nodes[0].id==node_id && body->nodes[0].friction==friction && body->nodes[1].friction==friction+1);
    EditorViewportUiDefinition *definition=editor_project_ui_definition_add(&p,EDITOR_VIEWPORT_UI_SLIDER);assert(definition);
    definition->value.slider=(EditorViewportUiSlider){.minimum=0,.maximum=10,.value=5};
    target=(EditorNumericTarget){.kind=EDITOR_ITEM_UI_DEFINITION,.item=definition->id};
    assert(editor_numeric_lock_set(&p,target,"value.slider.maximum",true));
    EditorNumericGuard guard={0};assert(editor_numeric_guard_begin(&guard,&p));
    definition->value.slider.minimum=20;definition->value.slider.maximum=21;definition->value.slider.value=20;
    char reason[512];assert(editor_numeric_guard_finish(&guard,&p,reason,sizeof(reason))==1);
    assert(definition->value.slider.minimum==0 && definition->value.slider.maximum==10 && definition->value.slider.value==5);
    editor_numeric_guard_destroy(&guard);
    definition=editor_project_ui_definition_add(&p,EDITOR_VIEWPORT_UI_SHAPE);assert(definition);
    definition->value.shape.vertex_count=4;
    definition->value.shape.vertices[0]=(Position){0,0};definition->value.shape.vertices[1]=(Position){10,0};
    definition->value.shape.vertices[2]=(Position){10,10};definition->value.shape.vertices[3]=(Position){0,10};
    target=(EditorNumericTarget){.kind=EDITOR_ITEM_UI_VERTEX,.parent=definition->id,.item=3};
    assert(editor_numeric_lock_set(&p,target,"x",true));
    EditorHistory history;assert(editor_history_init(&history,&p));assert(editor_history_ui_change_begin(&history));
    bool removed[]={false,true,false,false};editor_numeric_ui_vertices_remap(&p,definition->id,4,removed,0);
    memmove(&definition->value.shape.vertices[1],&definition->value.shape.vertices[2],2*sizeof(Position));--definition->value.shape.vertex_count;
    assert(editor_history_ui_change_finish(&history));
    EditorNumericTarget shifted=target;shifted.item=2;assert(editor_numeric_lock_check(&p,shifted,"x"));
    assert(editor_history_undo(&history));assert(editor_numeric_lock_check(&p,target,"x") && !editor_numeric_lock_check(&p,shifted,"x"));
    assert(editor_history_redo(&history));assert(editor_numeric_lock_check(&p,shifted,"x"));editor_history_destroy(&history);
    editor_project_destroy(&p);
}
int main(int argc,char **argv) {
    /* Accordion locks include their displayed numeric children, not adjacent sections. */
    const struct {EditorItemKind kind;const char *key,*section;bool included;} sections[]={
        {EDITOR_ITEM_CAMERA,"dimensions.x","transform",true},
        {EDITOR_ITEM_SPRITE,"size.y","transform",true},
        {EDITOR_ITEM_ANIMATED_SPRITE,"scale.x","transform",true},
        {EDITOR_ITEM_RIGID_BODY,"friction","physics",false},
        {EDITOR_ITEM_RIGID_BODY,"center_of_mass_offset.x","physics",true},
        {EDITOR_ITEM_SOFT_NODE,"radius","physics",true},
        {EDITOR_ITEM_SOFT_NODE,"restitution","physics",false},
        {EDITOR_ITEM_FORCE,"description.magnitude","source",true},
        {EDITOR_ITEM_FORCE,"description.location.offset.x","application",true},
        {EDITOR_ITEM_FORCE,"description.orientation.from.fraction","direction",true},
        {EDITOR_ITEM_FORCE,"description.location.offset.x","direction",false},
        {EDITOR_ITEM_TORQUE,"marker_offset.x","marker",true},
        {EDITOR_ITEM_VIEWPORT_CAMERA,"render_width","source",true},
        {EDITOR_ITEM_UI_FONT,"point_size","text-content",true},
        {EDITOR_ITEM_JOINT,"visual_size","configuration",true}
    };
    for(size_t i=0;i<sizeof(sections)/sizeof(*sections);++i)
        assert(editor_numeric_section_check(editor_numeric_property_get((EditorNumericTarget){.kind=sections[i].kind},sections[i].key),sections[i].section)==sections[i].included);
    connected_and_independent();
    compound_configuration();
    static EditorProject p,loaded;editor_project_init(&p);
    EditorObject *o=editor_project_object_add(&p,(Position){0});assert(o);
    EditorRigidBody *b=editor_project_rigid_body_add(&p,o);assert(b);
    EditorNumericTarget target={.kind=EDITOR_ITEM_RIGID_BODY,.object=o->id,.item=b->id};
    EditorHistory history;assert(editor_history_init(&history,&p));
    editor_command_executing_callback_set(begin,&history);editor_command_finished_callback_set(finish,&history);
    EditorCommand c=lock(target,"position.x",true);OK(editor_command_execute(&p,&c));
    assert(editor_numeric_lock_check(&p,target,"position.x"));
    assert(editor_history_undo(&history));assert(!editor_numeric_lock_check(&p,target,"position.x"));
    assert(editor_history_redo(&history));assert(editor_numeric_lock_check(&p,target,"position.x"));
    float x=b->position.x,y=b->position.y;
    c=(EditorCommand){.type=EDITOR_COMMAND_RIGID_BODY_TRANSFORM,.pointer_transform=true,
        .data.rigid_body_transform={o->id,b->id,{x+25,y+40},0}};
    EditorCommandResult r=editor_command_execute(&p,&c);OK(r);assert(r.numeric_blocked==1);
    assert(b->position.x==x && b->position.y==y+40);
    assert(editor_history_undo(&history));b=&p.objects[0].rigid_bodies[0];o=&p.objects[0];assert(b->position.y==y);
    assert(editor_history_redo(&history));b=&p.objects[0].rigid_bodies[0];o=&p.objects[0];assert(b->position.y==y+40);
    c=lock(target,"mass_value",true);OK(editor_command_execute(&p,&c));float mass=b->mass_value;
    c=(EditorCommand){.type=EDITOR_COMMAND_PROPERTY_SET,.data.property_set={.kind=EDITOR_ITEM_RIGID_BODY,
        .object=o->id,.item=b->id,.property=EDITOR_PROPERTY_MASS,.value_kind=EDITOR_PROPERTY_VALUE_FLOAT,.value.number=mass+3}};
    r=editor_command_execute(&p,&c);assert(r.kind==ERROR_RESULT_ERROR && r.numeric_blocked==1);assert(b->mass_value==mass);
    c=lock(target,"@transform",true);size_t before=history.undo_count;OK(editor_command_execute(&p,&c));
    assert(history.undo_count==before+1);assert(editor_numeric_lock_check(&p,target,"rotation"));
    assert(editor_history_undo(&history));assert(!editor_numeric_lock_check(&p,target,"rotation"));
    const char *path=argc>1?argv[1]:"numeric_locks_test.json";
    assert(editor_project_save(&p,path));OK(editor_project_load(&loaded,path));
    assert(loaded.numeric_lock_count==p.numeric_lock_count);assert(editor_numeric_lock_check(&loaded,target,"mass_value"));
    char output[2048];c=lock(target,"mass_value",false);OK(editor_command_cli_write(&c,path,output,sizeof(output)));
    assert(strstr(output,"numeric-lock set"));
    char *cli[]={"rohr-cli","--project",(char *)path,"--object-id","1","--body-id","1","lock-get","mass_value"};
    const char *parsed_path=NULL;EditorCommand parsed;
    OK(editor_command_cli_standard_parse(&p,9,cli,&parsed_path,&parsed));
    assert(parsed.type==EDITOR_COMMAND_NUMERIC_LOCK && parsed.data.numeric_lock.query && editor_numeric_target_check(parsed.data.numeric_lock.target,target));
    c=(EditorCommand){.type=EDITOR_COMMAND_OBJECT_DUPLICATE,.data.object_remove.object=o->id};
    r=editor_command_execute(&p,&c);OK(r);
    EditorNumericTarget copied=target;copied.object=r.result.object;
    assert(editor_numeric_lock_check(&p,copied,"mass_value"));
    assert(editor_history_undo(&history));assert(!editor_numeric_lock_check(&p,copied,"mass_value"));
    assert(editor_history_redo(&history));assert(editor_numeric_lock_check(&p,copied,"mass_value"));
    c=(EditorCommand){.type=EDITOR_COMMAND_OBJECT_REMOVE,.data.object_remove.object=copied.object};
    OK(editor_command_execute(&p,&c));assert(!editor_numeric_lock_check(&p,copied,"mass_value"));
    assert(editor_history_undo(&history));assert(editor_numeric_lock_check(&p,copied,"mass_value"));
    editor_command_executing_callback_set(NULL,NULL);editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&history);editor_project_destroy(&loaded);editor_project_destroy(&p);
    puts("numeric locks: history, projected transform, indirect guard and persistence passed");return 0;
}
