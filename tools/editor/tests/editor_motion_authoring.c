/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editors/physics/editor_rigid_body.h"
#include "editors/physics/editor_particle.h"
#include "editors/soft_body/editor_soft_node.h"
#include "editors/soft_body/editor_soft_body.h"
#include "editors/multi/editor_bulk_panel.h"
#include "editor_workspace.h"
#include "editor_navigation.h"
#include "entity/object_internal.h"
#include "physics/physics_internal.h"
#include "motion_checks.h"
#include "yyjson/yyjson.h"
#include <string.h>
float editor_viewport_width = 1024, editor_window_width = 1280;
float editor_window_height = 720, editor_viewport_bottom = 720;
static void begin(const EditorProject *p, const EditorCommand *c, void *h) { editor_history_command_begin(h,p,c); }
static void finish(const EditorCommand *c, const EditorCommandResult *r, void *h) { editor_history_command_finish(h,c,r); }
static void author_append(EditorProject *p) {
    EditorObject *o = editor_project_object_add(p,(Position){0}); assert(o);
    snprintf(o->name,sizeof(o->name),"Motion");
    for(unsigned kind=0;kind<2;++kind) for(unsigned mode=0;mode<4;++mode) {
        EditorRigidBody *b=editor_project_rigid_body_add(p,o); assert(b);
        snprintf(b->name,sizeof(b->name),"%s_%u",kind?"particle":"body",mode);
        b->position=(Position){(float)mode*120-180,kind? -100:100};
        b->mass_value=mode==3?0:7; b->kinematic_driven=mode==1 || mode==2; b->static_body=mode==2;
        b->initial_velocity=(Velocity){3,4}; b->initial_acceleration=(Acceleration){2,5};
        b->gravity_enabled=true; b->collision_category=b->responds_to=1;
        if(kind) { b->particle=b->standalone_particle=true; b->particle_auto_fit=false; b->particle_radius=20;
            assert(editor_project_particle_hitbox_sync(p,b)); }
    }
    EditorSoftBody *s=editor_project_soft_body_add(p,o); assert(s); snprintf(s->name,sizeof(s->name),"soft");
    for(unsigned mode=0;mode<4;++mode) {
        EditorSoftNode *n=editor_project_soft_node_add(p,s,(Position){(float)mode*120-180,-240}); assert(n);
        snprintf(n->name,sizeof(n->name),"node_%u",mode); n->node_mass=7; n->radius=20;
        n->kinematic_driven=mode==1 || mode==2; n->static_body=mode==2; n->initial_motion_inherited=false;
        n->initial_velocity=(Velocity){3,4}; n->initial_acceleration=(Acceleration){2,5};
        n->gravity_enabled=true; n->collision_category=n->responds_to=1;
    }
    s=editor_project_soft_body_add(p,o); assert(s);
    snprintf(s->name,sizeof(s->name),"driven_soft");
    for(unsigned i=0;i<2;++i) {
        EditorSoftNode *n=editor_project_soft_node_add(p,s,(Position){(float)i*120,-360}); assert(n);
        snprintf(n->name,sizeof(n->name),"driven_%u",i); n->node_mass=7;
        n->initial_motion_inherited=false; n->initial_velocity=(Velocity){3,4};
        n->initial_acceleration=(Acceleration){2,5}; n->gravity_enabled=true;
    }
    EditorCommand command={.type=EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set={EDITOR_ITEM_SOFT_BODY,o->id,0,s->id,0,EDITOR_PROPERTY_MOTION,
            EDITOR_PROPERTY_VALUE_UINT,{.integer=EDITOR_MOTION_KINEMATIC}}};
    MOTION_OK(editor_command_execute(p,&command));
}
static void author(EditorProject *p) { editor_project_init(p); author_append(p); }
static void history_check(EditorProject *p) {
    EditorHistory h; assert(editor_history_init(&h,p));
    editor_command_executing_callback_set(begin,&h); editor_command_finished_callback_set(finish,&h);
    EditorViewportState state; editor_viewport_state_init(&state);
    EditorObject *o=&p->objects[0]; EditorSoftBody *s=&o->soft_body_items[0];
    EditorSelectionRef refs[]={ {EDITOR_SELECTION_RIGID_BODY,o->id,0,0,o->rigid_bodies[0].id},
        {EDITOR_SELECTION_PARTICLE,o->id,0,0,o->rigid_bodies[4].id},
        {EDITOR_SELECTION_SOFT_NODE,o->id,s->id,0,s->nodes[0].id}};
    for(unsigned i=0;i<3;++i) assert(editor_viewport_selection_set(p,&state,refs[i],i!=0));
    EditorPropertySetCommand set={.property=EDITOR_PROPERTY_MOTION,.value_kind=EDITOR_PROPERTY_VALUE_UINT};
    const unsigned modes[]={EDITOR_MOTION_KINEMATIC,EDITOR_MOTION_STATIC,EDITOR_MOTION_DYNAMIC};
    for(unsigned step=0;step<3;++step) {
        set.value.integer=modes[step]; assert(editor_bulk_property_set(p,&state,&h,&set));
        o=&p->objects[0]; s=&o->soft_body_items[0];
        assert(o->rigid_bodies[0].kinematic_driven==(step<2));
        assert(o->rigid_bodies[4].static_body==(step==1));
        assert(s->nodes[0].kinematic_driven==(step<2));
        assert(o->rigid_bodies[0].mass_value==7 && o->rigid_bodies[4].initial_velocity.x==3);
        assert(s->nodes[0].initial_acceleration.y==5 && s->nodes[0].gravity_enabled);
        assert(editor_history_undo(&h)); assert(editor_history_redo(&h));
    }
    assert(h.undo_count==3);
    /* Undo Dynamic restores dormant explicit intent; duplication copies it. */
    assert(editor_history_undo(&h));
    EditorObject *copy=editor_project_object_duplicate(p,p->objects[0].id); assert(copy);
    assert(copy->rigid_bodies[0].static_body && copy->rigid_bodies[0].kinematic_driven);
    assert(copy->rigid_bodies[4].kinematic_driven && copy->soft_body_items[0].nodes[0].kinematic_driven);
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&h); editor_viewport_state_destroy(&state);
}
static void soft_body_check(EditorProject *p) {
    EditorHistory h; assert(editor_history_init(&h,p));
    editor_command_executing_callback_set(begin,&h); editor_command_finished_callback_set(finish,&h);
    EditorObject *o=&p->objects[0]; EditorSoftBody *s=&o->soft_body_items[0];
    assert(editor_soft_body_motion_get(s)==EDITOR_MOTION_MIXED);
    EditorSoftNode original[4]; memcpy(original,s->nodes,sizeof(original));
    EditorCommand c={.type=EDITOR_COMMAND_PROPERTY_SET,.data.property_set={
        EDITOR_ITEM_SOFT_BODY,o->id,0,s->id,0,EDITOR_PROPERTY_MOTION,EDITOR_PROPERTY_VALUE_UINT}};
    const unsigned modes[]={EDITOR_MOTION_STATIC,EDITOR_MOTION_KINEMATIC,EDITOR_MOTION_STATIC,EDITOR_MOTION_DYNAMIC};
    for(unsigned step=0;step<4;++step) {
        c.data.property_set.value.integer=modes[step]; MOTION_OK(editor_command_execute(p,&c));
        s=&p->objects[0].soft_body_items[0];
        assert(editor_soft_body_motion_get(s)==modes[step]);
        for(unsigned i=0;i<4;++i) {
            EditorSoftNode expected=original[i];
            expected.static_body=modes[step]==EDITOR_MOTION_STATIC;
            expected.kinematic_driven=step==0?original[i].kinematic_driven:step!=3;
            assert(memcmp(&expected,&s->nodes[i],sizeof(expected))==0);
        }
        assert(editor_history_undo(&h)); assert(editor_history_redo(&h));
    }
    assert(h.undo_count==4); assert(editor_history_undo(&h));
    s=&p->objects[0].soft_body_items[0];
    assert(s->node_static_default && s->node_kinematic_driven_default);
    EditorObject *copy=editor_project_object_duplicate(p,p->objects[0].id); assert(copy);
    assert(copy->soft_body_items[0].node_static_default && copy->soft_body_items[0].node_kinematic_driven_default);
    assert(editor_project_save(p,"motion_defaults.json"));
    static EditorProject loaded; MOTION_OK(editor_project_load(&loaded,"motion_defaults.json"));
    EditorSoftNode *n=editor_project_soft_node_add(&loaded,&loaded.objects[0].soft_body_items[0],(Position){0});
    assert(n && n->static_body && n->kinematic_driven);
    editor_project_destroy(&loaded); remove("motion_defaults.json");
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&h);
    o=&p->objects[0]; s=editor_project_soft_body_add(p,o); assert(s);
    assert(editor_soft_body_motion_get(s)==EDITOR_MOTION_DYNAMIC);
    c.data.property_set.item=s->id; c.data.property_set.value.integer=EDITOR_MOTION_KINEMATIC;
    MOTION_OK(editor_command_execute(p,&c)); assert(editor_soft_body_motion_get(s)==EDITOR_MOTION_KINEMATIC);
    n=editor_project_soft_node_add(p,s,(Position){0}); assert(n && n->kinematic_driven && !n->static_body);
    n=editor_project_soft_node_add(p,s,(Position){0}); assert(n && n->kinematic_driven);
    n->kinematic_driven=false; assert(editor_soft_body_motion_get(s)==EDITOR_MOTION_MIXED);
    MOTION_OK(editor_command_execute(p,&c)); assert(editor_soft_body_motion_get(s)==EDITOR_MOTION_KINEMATIC);
    c.data.property_set.value.integer=EDITOR_MOTION_MIXED;
    assert(editor_command_execute(p,&c).kind==ERROR_RESULT_ERROR);
}
static void cli_check(EditorProject *p) {
    for(unsigned kind=0;kind<4;++kind) {
        EditorObject *o=&p->objects[0]; EditorSoftBody *s=&o->soft_body_items[0];
        char object[20],parent[20],item[20]; snprintf(object,sizeof(object),"%u",o->id);
        snprintf(parent,sizeof(parent),"%u",s->id);
        snprintf(item,sizeof(item),"%u",kind==3?s->id:kind==2?s->nodes[0].id:o->rigid_bodies[kind?4:0].id);
        for(unsigned mode=0;mode<5;++mode) {
            char *values[]={"kinematic","static","dynamic","true","false"};
            char *args[]={"rohr-cli",kind==3?"soft-body":kind==2?"soft-node":"rigid-body","set","motion.json",object,
                kind==2?parent:item,kind==2?item:(mode<3?"motion":"static"),
                kind==2?(mode<3?"motion":"static"):values[mode],values[mode]};
            EditorCommand c; const char *path=NULL; MOTION_OK(editor_command_cli_parse(kind==2?9:8,args,&path,&c));
            MOTION_OK(editor_command_execute(p,&c));
            char text[512]; MOTION_OK(editor_command_cli_write(&c,path,text,sizeof(text)));
            assert(strstr(text,values[mode]));
            bool expected=mode<2;
            assert((kind==3?s->node_kinematic_driven_default:kind==2?s->nodes[0].kinematic_driven:o->rigid_bodies[kind?4:0].kinematic_driven)==expected);
        }
    }
}
static void formats(EditorProject *p) {
    assert(editor_project_save(p,"motion.json"));
    yyjson_doc *doc=yyjson_read_file("motion.json",0,NULL,NULL); assert(doc);
    assert(yyjson_get_uint(yyjson_obj_get(yyjson_doc_get_root(doc),"format_version"))==13);
    for(unsigned variant=0;variant<13;++variant) {
        yyjson_mut_doc *m=yyjson_doc_mut_copy(doc,NULL); assert(m);
        yyjson_mut_val *root=yyjson_mut_doc_get_root(m);
        yyjson_mut_set_uint(yyjson_mut_obj_get(root,"format_version"),variant==0 || (variant>=5 && variant<11)?12:13);
        yyjson_mut_val *o=yyjson_mut_arr_get(yyjson_mut_obj_get(root,"objects"),0);
        yyjson_mut_val *b=yyjson_mut_arr_get(yyjson_mut_obj_get(o,"rigid_bodies"),1);
        yyjson_mut_val *s=yyjson_mut_arr_get(yyjson_mut_obj_get(o,"soft_bodies"),0);
        assert(s);
        yyjson_mut_val *n=yyjson_mut_arr_get(yyjson_mut_obj_get(s,"nodes"),1);
        if(variant==0) { yyjson_mut_obj_remove_key(b,"kinematic_driven"); yyjson_mut_obj_remove_key(n,"kinematic_driven");
            yyjson_mut_obj_remove_key(s,"node_static_default"); yyjson_mut_obj_remove_key(s,"node_kinematic_driven_default"); }
        else if(variant>=9) yyjson_mut_obj_put(s,yyjson_mut_str(m,variant%2?"node_static_default":"node_kinematic_driven_default"),
            variant<11?yyjson_mut_int(m,1):yyjson_mut_str(m,"true"));
        else yyjson_mut_obj_put(variant<3?b:n,yyjson_mut_str(m,"kinematic_driven"),
            variant%2?yyjson_mut_int(m,1):yyjson_mut_str(m,"true"));
        assert(yyjson_mut_write_file("motion_variant.json",m,0,NULL,NULL)); yyjson_mut_doc_free(m);
        static EditorProject loaded; editor_project_init(&loaded);
        EditorResult result=editor_project_load(&loaded,"motion_variant.json");
        if(variant==0) { MOTION_OK(result); assert(!loaded.objects[0].rigid_bodies[1].kinematic_driven);
            assert(!loaded.objects[0].soft_body_items[0].nodes[1].kinematic_driven);
            assert(!loaded.objects[0].soft_body_items[0].node_static_default &&
                !loaded.objects[0].soft_body_items[0].node_kinematic_driven_default); }
        else assert(result.kind==ERROR_RESULT_ERROR);
        editor_project_destroy(&loaded);
    }
    yyjson_doc_free(doc); remove("motion_variant.json");
}
static void persistence(EditorProject *p) {
    static EditorProject loaded; MOTION_OK(editor_project_load(&loaded,"motion.json"));
    for(unsigned path=0;path<2;++path) {
        ObjectDescriptionResult c=path?rohr_object_load("motion.json"):object_description_project_read(&loaded,"motion.json"); MOTION_OK(c);
        EntityResult a=rohr_entity_object_by_name_add(c.result.value,"Motion"), b=rohr_entity_object_by_name_add(c.result.value,"Motion");
        MOTION_OK(a); MOTION_OK(b);
        EntityName name=rohr_entity_name_get(a.result.value).result.value;
        MOTION_OK(rohr_game_state_file_save("motion_runtime.json"));
        motion_instance_check(a.result.value); motion_instance_check(b.result.value);
        MOTION_OK(rohr_entity_delete(a.result.value)); MOTION_OK(rohr_entity_delete(b.result.value));
        MOTION_OK(rohr_game_state_file_load("motion_runtime.json"));
        a=rohr_entity_by_name_get(name.value); MOTION_OK(a); motion_instance_check(a.result.value);
        MOTION_OK(rohr_entity_delete(a.result.value));
        if(path) MOTION_OK(rohr_object_destroy(c.result.value)); else object_description_tree_destroy((ObjectDescription*)c.result.value);
        /* Clear the second restored instance too. */
        rohr_engine_stop(); MOTION_OK(rohr_engine_start());
    }
    (void)p; editor_project_destroy(&loaded); remove("motion_runtime.json");
}
static Entity physics_fixture(unsigned kind, float x, bool explicit_mode) {
    CollisionFilterDescription filter={1,1};
    EntityResult e;
    if(kind==1) e=rohr_physics_particle_add((ParticleDescription){.position={x,0},.radius=1,
        .mass_value=2,.velocity={-2,0},.acceleration={0,0},.kinematic_driven=explicit_mode,
        .gravity_enabled=true,.collision_enabled=true,.collision_filter=filter});
    else if(kind==2) e=rohr_physics_soft_body_node_add((SoftBodyNodeDescription){.position={x,0},.radius=1,
        .mass=2,.velocity={-2,0},.kinematic_driven=explicit_mode,.gravity_enabled=true,
        .collision_enabled=true,.collision_filter=filter});
    else {
        e=rohr_entity_add(); MOTION_OK(e);
        MOTION_OK(rohr_physics_hitbox_set(e.result.value,rohr_math_square_create(2,2)));
        MOTION_OK(rohr_physics_position_set(e.result.value,(Position){x,0}));
        MOTION_OK(rohr_physics_mass_set(e.result.value,2));
        MOTION_OK(rohr_physics_velocity_set(e.result.value,(Velocity){-2,0}));
        MOTION_OK(rohr_physics_dynamic_set(e.result.value)); MOTION_OK(rohr_physics_gravity_enable(e.result.value));
        MOTION_OK(rohr_physics_collision_filter_description_set(e.result.value,filter));
        MOTION_OK(rohr_entity_components_add(e.result.value,ROHR_COLLISION));
        if(explicit_mode) MOTION_OK(rohr_physics_kinematic_driven_set(e.result.value));
    }
    MOTION_OK(e); return e.result.value;
}
static void physics_check(void) {
    for(unsigned kind=0;kind<3;++kind) {
        MOTION_OK(rohr_engine_start());
        Entity b=physics_fixture(kind,.75f,true);
        EntityIndex index; assert(entity_index_get(b,&index));
        physics_dt_update(.1f);
        assert(fabsf(positions[index].x-.55f)<.001f && fabsf(positions[index].y)<.001f);
        assert(velocities[index].x==-2 && velocities[index].y==0);
        MOTION_OK(rohr_physics_static_set(b));
        Position fixed=positions[index]; physics_dt_update(.1f);
        assert(positions[index].x==fixed.x && positions[index].y==fixed.y);
        MOTION_OK(rohr_physics_dynamic_set(b)); assert(rohr_physics_kinematic_driven_check(b));
        MOTION_OK(rohr_physics_position_set(b,(Position){.75f,0}));
        Entity a=physics_fixture(kind,-.75f,false); EntityIndex ai; assert(entity_index_get(a,&ai));
        MOTION_OK(rohr_physics_velocity_set(a,(Velocity){10,0}));
        Report r=rohr_system_physics_update(0); assert(r.header.status!=ROHR_REPORT_FAILED);
        assert(velocities[index].x==-2 && velocities[ai].x<10);
        MOTION_OK(rohr_physics_kinematic_driven_remove(b));
        MOTION_OK(rohr_physics_mass_set(b,2));
        MOTION_OK(rohr_physics_position_set(a,(Position){-.75f,0}));
        MOTION_OK(rohr_physics_position_set(b,(Position){.75f,0}));
        MOTION_OK(rohr_physics_velocity_set(a,(Velocity){10,0}));
        MOTION_OK(rohr_physics_velocity_set(b,(Velocity){-2,0}));
        r=rohr_system_physics_update(0); assert(r.header.status!=ROHR_REPORT_FAILED);
        assert(velocities[index].x>-2 && velocities[ai].x<10);
        rohr_engine_stop();
    }
}
static void ui_frame(void *ui, unsigned kind, EditorModeContext *c, Position pointer, MouseButtonState button) {
    rohr_ui_input_set((UIInput){pointer,button});
    editor_mode_accordion_layout_measure_reset();
    if(kind==0) editor_rigid_body_editor_draw(ui,c);
    else if(kind==1) editor_particle_editor_draw(ui,c);
    else if(kind==2) editor_soft_node_editor_draw(ui,c);
    else { EditorAutoShapeEditor shape={0}; editor_soft_body_editor_draw(ui,&shape,c); }
    rohr_ui_update();
}
static void ui_click(void *ui,unsigned kind,EditorModeContext *c,Position p) {
    ui_frame(ui,kind,c,p,MOUSE_BUTTON_STATE_PRESSED);
    ui_frame(ui,kind,c,p,MOUSE_BUTTON_STATE_RELEASED);
    ui_frame(ui,kind,c,p,MOUSE_BUTTON_STATE_UP);
}
static void ui_check(EditorProject *p) {
    MOTION_OK(rohr_graphics_start()); FontAsset font=rohr_graphics_font_default_get();
    EditorRigidBodyEditor rigid; EditorParticleEditor particle; EditorSoftNodeEditor node;
    EditorSoftBodyEditor soft; assert(editor_soft_body_editor_create(&soft,&font));
    editor_mode_accordion_section_expanded_set(&soft.transform_section,false);
    editor_mode_accordion_section_expanded_set(&soft.physics_section,true);
    assert(editor_rigid_body_editor_create(&rigid,&font)); assert(editor_particle_editor_create(&particle,&font));
    assert(editor_soft_node_editor_create(&node,&font));
    editor_mode_accordion_section_expanded_set(&rigid.transform_section,false);
    editor_mode_accordion_section_expanded_set(&particle.transform_section,false);
    editor_mode_accordion_section_expanded_set(&node.transform_section,false);
    editor_mode_accordion_section_expanded_set(&rigid.physics_section,true);
    editor_mode_accordion_section_expanded_set(&particle.physics_section,true);
    editor_mode_accordion_section_expanded_set(&node.physics_section,true);
    EditorViewportState state; editor_viewport_state_init(&state);
    EditorHistory h; assert(editor_history_init(&h,p));
    editor_command_executing_callback_set(begin,&h); editor_command_finished_callback_set(finish,&h);
    EditorModeContext c={.project=p,.history=&h,.viewport=&state,.width=400};
    void *uis[]={&rigid,&particle,&node};
    for(unsigned kind=0;kind<3;++kind) {
        EditorObject *o=&p->objects[0]; EditorSoftBody *s=&o->soft_body_items[0];
        EditorSelectionRef ref=kind==2?(EditorSelectionRef){EDITOR_SELECTION_SOFT_NODE,o->id,s->id,0,s->nodes[0].id}:
            (EditorSelectionRef){kind?EDITOR_SELECTION_PARTICLE:EDITOR_SELECTION_RIGID_BODY,o->id,0,0,o->rigid_bodies[kind?4:0].id};
        assert(editor_viewport_selection_set(p,&state,ref,false)); assert(editor_navigation_selected_open(p,&state));
        p->navigation=editor_navigation_state_get(p,&state);
        float y=kind==2?336:238;
        ui_frame(uis[kind],kind,&c,(Position){0},MOUSE_BUTTON_STATE_UP);
        ui_click(uis[kind],kind,&c,(Position){200,y});
        ui_click(uis[kind],kind,&c,(Position){200,y+80});
        o=&p->objects[0]; s=&o->soft_body_items[0];
        assert(kind==2?s->nodes[0].kinematic_driven:o->rigid_bodies[kind?4:0].kinematic_driven);
        assert(editor_history_undo(&h)); assert(editor_history_redo(&h));
        /* Dynamic clears explicit intent through the same selector. */
        ui_click(uis[kind],kind,&c,(Position){200,y});
        ui_click(uis[kind],kind,&c,(Position){200,y+24});
        o=&p->objects[0]; s=&o->soft_body_items[0];
        assert(!(kind==2?s->nodes[0].kinematic_driven:o->rigid_bodies[kind?4:0].kinematic_driven));
        if(kind<2) {
            o->rigid_bodies[kind?4:0].mass_value=0;
            ui_frame(uis[kind],kind,&c,(Position){0},MOUSE_BUTTON_STATE_UP);
            float zero_height=editor_mode_accordion_layout_measure_get();
            o->rigid_bodies[kind?4:0].mass_value=7;
            ui_frame(uis[kind],kind,&c,(Position){0},MOUSE_BUTTON_STATE_UP);
            assert(zero_height==editor_mode_accordion_layout_measure_get()+30);
        }
    }
    {
        EditorObject *o=&p->objects[0]; EditorSoftBody *s=&o->soft_body_items[0];
        EditorSelectionRef ref={EDITOR_SELECTION_SOFT_BODY,o->id,0,0,s->id};
        assert(editor_viewport_selection_set(p,&state,ref,false)); assert(editor_navigation_selected_open(p,&state));
        p->navigation=editor_navigation_state_get(p,&state);
        assert(editor_soft_body_motion_get(s)==EDITOR_MOTION_MIXED);
        ui_frame(&soft,3,&c,(Position){0},MOUSE_BUTTON_STATE_UP);
        ui_click(&soft,3,&c,(Position){200,202});
        ui_click(&soft,3,&c,(Position){200,310});
        assert(editor_soft_body_motion_get(&p->objects[0].soft_body_items[0])==EDITOR_MOTION_KINEMATIC);
        assert(editor_history_undo(&h));
        assert(editor_soft_body_motion_get(&p->objects[0].soft_body_items[0])==EDITOR_MOTION_MIXED);
        assert(editor_history_redo(&h));
        for(unsigned mode=0;mode<2;++mode) {
            ui_click(&soft,3,&c,(Position){200,202});
            ui_click(&soft,3,&c,(Position){200,mode==0?282:254});
            s=&p->objects[0].soft_body_items[0];
            assert(editor_soft_body_motion_get(s)==(mode==0?EDITOR_MOTION_STATIC:EDITOR_MOTION_DYNAMIC));
            for(size_t i=0;i<s->node_count;++i) assert(s->nodes[i].kinematic_driven==(mode==0));
            assert(editor_history_undo(&h)); assert(editor_history_redo(&h));
        }
    }
    EditorBulkPanel bulk; assert(editor_bulk_panel_create(&bulk,&font));
    EditorObject *o=&p->objects[0]; EditorSoftBody *s=&o->soft_body_items[0];
    EditorSelectionRef refs[]={ {EDITOR_SELECTION_RIGID_BODY,o->id,0,0,o->rigid_bodies[0].id},
        {EDITOR_SELECTION_PARTICLE,o->id,0,0,o->rigid_bodies[5].id},
        {EDITOR_SELECTION_SOFT_BODY,o->id,0,0,s->id}};
    for(unsigned i=0;i<3;++i) assert(editor_viewport_selection_set(p,&state,refs[i],i!=0));
    rohr_ui_input_set((UIInput){{0},MOUSE_BUTTON_STATE_UP});
    editor_bulk_panel_draw(&bulk,p,&state,&h,NULL,0,400,680,NULL,NULL); rohr_ui_update();
    assert(bulk.dropdown_indices[0]==0);
    for(unsigned click=0;click<2;++click) for(unsigned phase=0;phase<3;++phase) {
        MouseButtonState buttons[]={MOUSE_BUTTON_STATE_PRESSED,MOUSE_BUTTON_STATE_RELEASED,MOUSE_BUTTON_STATE_UP};
        rohr_ui_input_set((UIInput){{200,click?202:96},buttons[phase]});
        editor_bulk_panel_draw(&bulk,p,&state,&h,NULL,0,400,680,NULL,NULL); rohr_ui_update();
    }
    assert(bulk.dropdown_indices[0]==3);
    assert(p->objects[0].rigid_bodies[0].kinematic_driven && p->objects[0].soft_body_items[0].nodes[0].kinematic_driven);
    assert(editor_history_undo(&h));
    rohr_ui_input_set((UIInput){{0},MOUSE_BUTTON_STATE_UP});
    editor_bulk_panel_draw(&bulk,p,&state,&h,NULL,0,400,680,NULL,NULL); rohr_ui_update();
    assert(bulk.dropdown_indices[0]==0);
    assert(editor_history_redo(&h)); editor_bulk_panel_destroy(&bulk);
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&h); editor_viewport_state_destroy(&state);
    editor_rigid_body_editor_destroy(&rigid); editor_particle_editor_destroy(&particle); editor_soft_node_editor_destroy(&node);
    editor_soft_body_editor_destroy(&soft);
    rohr_graphics_stop();
}

int main(int argc,char **argv) {
    physics_check();
    MOTION_OK(rohr_engine_start());
    static EditorProject p; author(&p); history_check(&p); editor_project_destroy(&p);
    author(&p); soft_body_check(&p); editor_project_destroy(&p);
    author(&p); cli_check(&p); editor_project_destroy(&p);
    author(&p); ui_check(&p); editor_project_destroy(&p);
    author(&p); formats(&p); persistence(&p);
    if(argc==2) { EditorWorkspace w={0}; static EditorProject starter;
        MOTION_OK(editor_workspace_create(&w,&starter,argv[1])); author_append(&starter);
        starter.selected=starter.objects[starter.object_count-1].id;
        assert(editor_workspace_save(&w,&starter)); assert(editor_workspace_c_generate(&w,&starter));
        editor_project_destroy(&starter); }
    editor_project_destroy(&p); remove("motion.json"); rohr_engine_stop();
    puts("motion authoring passed"); return 0;
}
