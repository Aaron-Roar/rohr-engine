/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editors/soft_body/editor_soft_body.h"
#include "editors/multi/editor_bulk_panel.h"
#include "editor_navigation.h"
#include "editor_workspace.h"
#include "entity/object_internal.h"
#include "physics/physics_internal.h"
#include "yyjson/yyjson.h"
#include "soft_physics_checks.h"
#include <string.h>
float editor_viewport_width=1024, editor_window_width=1280;
float editor_window_height=720, editor_viewport_bottom=720;
static void begin(const EditorProject *p,const EditorCommand *c,void *h) { editor_history_command_begin(h,p,c); }
static void finish(const EditorCommand *c,const EditorCommandResult *r,void *h) { editor_history_command_finish(h,c,r); }
static EditorSoftBody *soft_get(EditorProject *p) { return &p->objects[p->object_count-1].soft_body_items[0]; }
static EditorCommand property(EditorProject *p,EditorPropertyKind kind, bool boolean,float number) {
    EditorCommand c={.type=EDITOR_COMMAND_PROPERTY_SET,.data.property_set={
        EDITOR_ITEM_SOFT_BODY,p->objects[p->object_count-1].id,0,soft_get(p)->id,0,kind,
        EDITOR_PROPERTY_VALUE_FLOAT,{.number=number}}};
    if(kind==EDITOR_PROPERTY_GRAVITY || kind==EDITOR_PROPERTY_COLLISION) {
        c.data.property_set.value_kind=EDITOR_PROPERTY_VALUE_BOOL; c.data.property_set.value.boolean=boolean;
    }
    return c;
}
static void author(EditorProject *p) {
    EditorObject *o=editor_project_object_add(p,(Position){0}); assert(o);
    snprintf(o->name,sizeof(o->name),"SoftPhysics");
    size_t index; assert(editor_project_collision_mask_add(p,"middle",&index));
    assert(editor_project_collision_mask_add(p,"outer",&index));
    EditorSoftBody *s=editor_project_soft_body_add(p,o); assert(s);
    snprintf(s->name,sizeof(s->name),"cloth");
    s->initial_velocity=(Velocity){2,3}; s->initial_acceleration=(Acceleration){4,5};
    for(unsigned i=0;i<3;++i) {
        EditorSoftNode *n=editor_project_soft_node_add(p,s,(Position){(float)i*60-60,0}); assert(n);
        snprintf(n->name,sizeof(n->name),"node_%u",i); n->node_mass=3;
        n->friction=.1f*(float)i; n->restitution=.2f*(float)i;
        n->gravity_enabled=i==1; n->collision_enabled=i!=1;
        n->collision_category=n->responds_to=UINT64_C(1)<<i;
    }
    EditorSoftBeam *b=editor_project_soft_beam_add(p,s,s->nodes[0].id,s->nodes[1].id); assert(b);
    snprintf(b->name,sizeof(b->name),"seam"); b->friction=.35f;
}
static void delivery_values(EditorProject *p) {
    const EditorPropertyKind kinds[]={EDITOR_PROPERTY_GRAVITY,EDITOR_PROPERTY_COLLISION,EDITOR_PROPERTY_FRICTION,EDITOR_PROPERTY_RESTITUTION};
    for(unsigned i=0;i<4;++i) { EditorCommand c=property(p,kinds[i],true,i==2?.6f:.8f); OK(editor_command_execute(p,&c)); }
    EditorSoftBody *s=soft_get(p);
    EditorCommand c={.type=EDITOR_COMMAND_COLLISION_FILTER_SET,.data.collision_filter_set={
        .kind=EDITOR_ITEM_SOFT_BODY,.object=p->objects[p->object_count-1].id,.item=s->id,
        .filter=EDITOR_COLLISION_FILTER_CATEGORY,.enabled=true,.mask="outer"}};
    OK(editor_command_execute(p,&c));
    c.data.collision_filter_set.filter=EDITOR_COLLISION_FILTER_RESPONDS_TO;
    c.data.collision_filter_set.enabled=false; snprintf(c.data.collision_filter_set.mask,EDITOR_OBJECT_NAME_MAX,"default");
    OK(editor_command_execute(p,&c));
    s->nodes[0].gravity_enabled=false; s->nodes[0].friction=.2f; s->nodes[1].collision_enabled=false;
    EditorSoftNode *n=editor_project_soft_node_add(p,s,(Position){120,0}); assert(n);
    snprintf(n->name,sizeof(n->name),"node_3");
}
static void commands_check(EditorProject *p) {
    EditorHistory h; assert(editor_history_init(&h,p));
    editor_command_executing_callback_set(begin,&h); editor_command_finished_callback_set(finish,&h);
    EditorSoftBody *s=soft_get(p); EditorSoftNode before[3]; memcpy(before,s->nodes,sizeof(before));
    EditorSoftBeam beam=s->beams[0];
    EditorSoftBodyPhysicsValues v=editor_soft_body_physics_get(s);
    assert(!v.gravity_all && v.gravity_any && v.friction_mixed && v.restitution_mixed);
    const EditorPropertyKind kinds[]={EDITOR_PROPERTY_GRAVITY,EDITOR_PROPERTY_COLLISION,EDITOR_PROPERTY_FRICTION,EDITOR_PROPERTY_RESTITUTION};
    for(unsigned i=0;i<4;++i) {
        EditorCommand c=property(p,kinds[i],true,.7f); OK(editor_command_execute(p,&c));
        s=soft_get(p);
        for(unsigned n=0;n<3;++n) {
            EditorSoftNode expected=before[n];
            if(i==0) expected.gravity_enabled=true;
            if(i==1) expected.collision_enabled=true;
            if(i==2) expected.friction=.7f;
            if(i==3) expected.restitution=.7f;
            assert(memcmp(&expected,&s->nodes[n],sizeof(expected))==0);
        }
        assert(memcmp(&beam,&s->beams[0],sizeof(beam))==0);
        assert(editor_history_undo(&h)); assert(editor_history_redo(&h)); assert(editor_history_undo(&h));
    }
    EditorCommand invalid=property(p,EDITOR_PROPERTY_FRICTION,false,INFINITY);
    assert(editor_command_execute(p,&invalid).kind==ERROR_RESULT_ERROR);
    invalid=property(p,EDITOR_PROPERTY_RESTITUTION,false,1.1f);
    assert(editor_command_execute(p,&invalid).kind==ERROR_RESULT_ERROR);
    EditorSelectionRef ref={EDITOR_SELECTION_SOFT_BODY,p->objects[0].id,0,0,soft_get(p)->id};
    OK(editor_collision_edit(p,&h,&ref,1,0,"new_category",true,true));
    assert(p->collision_mask_count==4); s=soft_get(p);
    for(unsigned n=0;n<3;++n) assert(s->nodes[n].collision_category==((UINT64_C(1)<<n)|8));
    assert(s->node_category_default==9 && !s->beams[0].collision_enabled);
    assert(editor_history_undo(&h) && p->collision_mask_count==3); assert(editor_history_redo(&h));
    EditorObject *copy=editor_project_object_duplicate(p,p->objects[0].id); assert(copy);
    assert(copy->soft_body_items[0].node_category_default==9);
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&h);
    /* Empty parents edit defaults and new nodes inherit them. */
    s=editor_project_soft_body_add(p,&p->objects[0]); assert(s);
    EditorCommand c={.type=EDITOR_COMMAND_PROPERTY_SET,.data.property_set={EDITOR_ITEM_SOFT_BODY,
        p->objects[0].id,0,s->id,0,EDITOR_PROPERTY_FRICTION,EDITOR_PROPERTY_VALUE_FLOAT,{.number=.9f}}};
    OK(editor_command_execute(p,&c)); assert(editor_soft_body_physics_get(s).friction==.9f);
    EditorSoftNode *n=editor_project_soft_node_add(p,s,(Position){0}); assert(n && n->friction==.9f && n->collision_enabled);
}
static void cli_check(EditorProject *p) {
    char object[32],body[32]; snprintf(object,sizeof(object),"%u",p->objects[0].id);
    snprintf(body,sizeof(body),"%u",soft_get(p)->id);
    const char *properties[]={"gravity","collision","friction","restitution","category","responds-to"};
    for(unsigned i=0;i<6;++i) {
        char *args[]={"rohr-cli","soft-body",i<4?"set":"filter","soft_physics.json",object,body,
            (char*)properties[i],i<2?"true":i<4?"0.65":"outer","true"};
        EditorCommand c; const char *path=NULL; OK(editor_command_cli_parse(i<4?8:9,args,&path,&c));
        OK(editor_command_execute(p,&c)); char text[512]; OK(editor_command_cli_write(&c,path,text,sizeof(text)));
        assert(strstr(text,properties[i]));
    }
}
static void formats_check(EditorProject *p) {
    assert(editor_project_save(p,"soft_physics.json"));
    static EditorProject loaded; OK(editor_project_load(&loaded,"soft_physics.json"));
    EditorSoftBody *s=soft_get(&loaded); EditorSoftNode *n=editor_project_soft_node_add(&loaded,s,(Position){0});
    assert(n && n->gravity_enabled && n->collision_enabled && n->friction==.6f && n->restitution==.8f);
    assert(n->collision_category==5 && n->responds_to==0); editor_project_destroy(&loaded);
    yyjson_doc *doc=yyjson_read_file("soft_physics.json",0,NULL,NULL); assert(doc);
    const char *fields[]={"node_gravity_default","node_collision_default","node_category_default",
        "node_responds_to_default","node_friction_default","node_restitution_default"};
    for(unsigned version=12;version<=13;++version) for(unsigned variant=0;variant<9;++variant) {
        yyjson_mut_doc *m=yyjson_doc_mut_copy(doc,NULL); assert(m); yyjson_mut_val *root=yyjson_mut_doc_get_root(m);
        yyjson_mut_set_uint(yyjson_mut_obj_get(root,"format_version"),version);
        yyjson_mut_val *o=yyjson_mut_arr_get(yyjson_mut_obj_get(root,"objects"),0);
        yyjson_mut_val *b=yyjson_mut_arr_get(yyjson_mut_obj_get(o,"soft_bodies"),0);
        if(variant==0) for(unsigned i=0;i<6;++i) yyjson_mut_obj_remove_key(b,fields[i]);
        else if(variant<=6) yyjson_mut_obj_put(b,yyjson_mut_str(m,fields[variant-1]),yyjson_mut_str(m,"invalid"));
        else yyjson_mut_obj_put(b,yyjson_mut_str(m,fields[variant==7?4:5]),yyjson_mut_real(m,variant==7?-1:1.1));
        assert(yyjson_mut_write_file("soft_physics_variant.json",m,0,NULL,NULL)); yyjson_mut_doc_free(m);
        EditorResult r=editor_project_load(&loaded,"soft_physics_variant.json");
        if(variant==0) {
            OK(r); s=soft_get(&loaded); assert(!s->node_gravity_default && s->node_collision_default);
            assert(s->node_category_default==1 && s->node_responds_to_default==1 && s->node_restitution_default==.25f);
            assert(s->nodes[0].friction==.2f && !s->nodes[1].collision_enabled);
        } else assert(r.kind==ERROR_RESULT_ERROR);
        editor_project_destroy(&loaded);
    }
    yyjson_doc_free(doc); remove("soft_physics_variant.json");
    ObjectDescriptionResult catalog=rohr_object_load("soft_physics.json"); OK(catalog);
    EntityName names[2];
    for(unsigned i=0;i<2;++i) { EntityResult e=rohr_entity_object_by_name_add(catalog.result.value,"SoftPhysics"); OK(e);
        soft_instance_check(e.result.value); names[i]=rohr_entity_name_get(e.result.value).result.value; }
    OK(rohr_game_state_file_save("soft_physics_runtime.json"));
    for(unsigned i=0;i<2;++i) { EntityResult e=rohr_entity_by_name_get(names[i].value); OK(e); OK(rohr_entity_delete(e.result.value)); }
    OK(rohr_game_state_file_load("soft_physics_runtime.json"));
    for(unsigned i=0;i<2;++i) { EntityResult e=rohr_entity_by_name_get(names[i].value); OK(e); soft_instance_check(e.result.value); }
    OK(rohr_object_destroy(catalog.result.value)); remove("soft_physics_runtime.json");
}
static void panel(EditorSoftBodyEditor *ui,EditorModeContext *c,Position pointer,MouseButtonState button) {
    EditorAutoShapeEditor shape={0}; rohr_ui_input_set((UIInput){pointer,button});
    editor_mode_accordion_layout_measure_reset(); editor_soft_body_editor_draw(ui,&shape,c); rohr_ui_update();
}
static void click(EditorSoftBodyEditor *ui,EditorModeContext *c,Position p) {
    panel(ui,c,p,MOUSE_BUTTON_STATE_PRESSED); panel(ui,c,p,MOUSE_BUTTON_STATE_RELEASED); panel(ui,c,p,MOUSE_BUTTON_STATE_UP);
}
static void type(EditorSoftBodyEditor *ui,EditorModeContext *c,float y,const char *text,bool submit) {
    click(ui,c,(Position){200,y});
    SDL_Event event={.type=SDL_EVENT_KEY_DOWN}; event.key.key=SDLK_A; event.key.mod=SDL_KMOD_CTRL; rohr_ui_field_event_add(&event);
    event=(SDL_Event){.type=SDL_EVENT_TEXT_INPUT}; event.text.text=text; rohr_ui_field_event_add(&event);
    panel(ui,c,(Position){200,y},MOUSE_BUTTON_STATE_UP);
    if(submit) { event=(SDL_Event){.type=SDL_EVENT_KEY_DOWN}; event.key.key=SDLK_RETURN; rohr_ui_field_event_add(&event);
        panel(ui,c,(Position){200,y},MOUSE_BUTTON_STATE_UP); }
}
static void ui_check(EditorProject *p) {
    OK(rohr_graphics_start()); FontAsset font=rohr_graphics_font_default_get();
    EditorSoftBodyEditor ui; assert(editor_soft_body_editor_create(&ui,&font));
    editor_mode_accordion_section_expanded_set(&ui.transform_section,false);
    editor_mode_accordion_section_expanded_set(&ui.physics_section,true);
    EditorHistory h; assert(editor_history_init(&h,p));
    editor_command_executing_callback_set(begin,&h); editor_command_finished_callback_set(finish,&h);
    EditorViewportState state; editor_viewport_state_init(&state);
    EditorSelectionRef ref={EDITOR_SELECTION_SOFT_BODY,p->objects[0].id,0,0,soft_get(p)->id};
    assert(editor_viewport_selection_set(p,&state,ref,false)); assert(editor_navigation_selected_open(p,&state));
    p->navigation=editor_navigation_state_get(p,&state);
    EditorModeContext c={.project=p,.history=&h,.viewport=&state,.width=400};
    panel(&ui,&c,(Position){0},MOUSE_BUTTON_STATE_UP); assert(h.undo_count==0);
    assert(strcmp(ui.materials[0].text,"Mixed")==0);
    click(&ui,&c,(Position){100,362}); assert(editor_soft_body_physics_get(soft_get(p)).gravity_all);
    assert(editor_history_undo(&h)); assert(editor_history_redo(&h));
    size_t before=h.undo_count;
    type(&ui,&c,396,"0.75",false); assert(h.undo_count==before && soft_get(p)->nodes[0].friction==0);
    SDL_Event enter={.type=SDL_EVENT_KEY_DOWN}; enter.key.key=SDLK_RETURN; rohr_ui_field_event_add(&enter);
    panel(&ui,&c,(Position){200,396},MOUSE_BUTTON_STATE_UP);
    assert(soft_get(p)->node_friction_default==.75f && h.undo_count==before+1);
    assert(editor_history_undo(&h)); assert(editor_history_redo(&h));
    type(&ui,&c,428,"2",true); assert(soft_get(p)->node_restitution_default==.25f);
    type(&ui,&c,428,"0.9",true); assert(soft_get(p)->node_restitution_default==.9f);
    click(&ui,&c,(Position){100,460}); assert(soft_get(p)->nodes[1].collision_enabled);
    assert(!soft_get(p)->beams[0].collision_enabled);
    click(&ui,&c,(Position){100,492}); panel(&ui,&c,(Position){0},MOUSE_BUTTON_STATE_UP);
    assert(ui.collision.open[0]);
    click(&ui,&c,(Position){100,ui.collision.list_bounds[0].y+42});
    for(unsigned i=0;i<3;++i) assert(soft_get(p)->nodes[i].collision_category&1);
    assert(editor_history_undo(&h)); assert(editor_history_redo(&h));
    EditorBulkPanel bulk; assert(editor_bulk_panel_create(&bulk,&font));
    EditorObject *o=&p->objects[0]; EditorRigidBody *rigid=editor_project_rigid_body_add(p,o); assert(rigid);
    EditorSelectionRef other={EDITOR_SELECTION_RIGID_BODY,o->id,0,0,rigid->id};
    assert(editor_viewport_selection_set(p,&state,other,true));
    EditorPropertySetCommand set={.property=EDITOR_PROPERTY_FRICTION,.value_kind=EDITOR_PROPERTY_VALUE_FLOAT,.value.number=.4f};
    assert(editor_bulk_property_set(p,&state,&h,&set));
    assert(soft_get(p)->node_friction_default==.4f && p->objects[0].rigid_bodies[0].friction==.4f);
    assert(editor_history_undo(&h));
    rohr_ui_input_set((UIInput){{0},MOUSE_BUTTON_STATE_UP}); editor_bulk_panel_draw(&bulk,p,&state,&h,NULL,0,400,680,NULL,NULL); rohr_ui_update();
    assert(!bulk.assigned[2] && strcmp(bulk.numbers[3].text,"Mixed")==0);
    assert(editor_history_redo(&h)); editor_bulk_panel_destroy(&bulk);
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&h); editor_viewport_state_destroy(&state); editor_soft_body_editor_destroy(&ui); rohr_graphics_stop();
}
static void physics_check(void) {
    for(unsigned scenario=0;scenario<4;++scenario) {
        OK(rohr_engine_start()); OK(rohr_physics_gravity_set((Acceleration){0,10}));
        static EditorProject p; editor_project_init(&p);
        EditorObject *o=editor_project_object_add(&p,(Position){0}); assert(o);
        EditorSoftBody *s=editor_project_soft_body_add(&p,o); assert(s);
        EditorSoftNode *n=editor_project_soft_node_add(&p,s,(Position){-.75f,0}); assert(n);
        snprintf(s->name,sizeof(s->name),"test_soft"); snprintf(n->name,sizeof(n->name),"moving");
        n->radius=1; n->node_mass=2; n->initial_motion_inherited=false; n->initial_velocity=(Velocity){10,3};
        EditorCommand c=property(&p,EDITOR_PROPERTY_GRAVITY,scenario==3,0); OK(editor_command_execute(&p,&c));
        c=property(&p,EDITOR_PROPERTY_COLLISION,scenario!=0,0); OK(editor_command_execute(&p,&c));
        c=property(&p,EDITOR_PROPERTY_FRICTION,false,scenario==2?1:0); OK(editor_command_execute(&p,&c));
        c=property(&p,EDITOR_PROPERTY_RESTITUTION,false,scenario==2?1:.25f); OK(editor_command_execute(&p,&c));
        ObjectDescriptionResult catalog=object_description_project_read(&p,"physics.json"); OK(catalog);
        EntityResult root=rohr_entity_object_by_name_add(catalog.result.value,o->name); OK(root);
        Entity entity=soft_member(soft_member(root.result.value,"test_soft"),"moving");
        EntityIndex index; assert(entity_index_get(entity,&index));
        if(scenario==3) {
            physics_dt_update(.1f); assert(fabsf(velocities[index].y-4)<.001f);
        } else {
            EntityResult peer=rohr_physics_soft_body_node_add((SoftBodyNodeDescription){.position={.75f,0},
                .mass=2,.radius=1,.static_body=true,.friction=1,.restitution=1,
                .collision_enabled=true,.collision_filter={1,1}}); OK(peer);
            Report r=rohr_system_physics_update(0); assert(r.header.status!=ROHR_REPORT_FAILED);
            assert(fabsf(velocities[index].x-(scenario==0?10:scenario==1?-2.5f:-10))<.001f);
            if(scenario==2) assert(velocities[index].y<3); else assert(velocities[index].y==3);
        }
        object_description_tree_destroy((ObjectDescription*)catalog.result.value);
        editor_project_destroy(&p); rohr_engine_stop();
    }
}
int main(int argc,char **argv) {
    physics_check();
    OK(rohr_engine_start()); static EditorProject p;
    editor_project_init(&p); author(&p); commands_check(&p); editor_project_destroy(&p);
    editor_project_init(&p); author(&p); cli_check(&p); editor_project_destroy(&p);
    editor_project_init(&p); author(&p); ui_check(&p); editor_project_destroy(&p);
    editor_project_init(&p); author(&p); delivery_values(&p); formats_check(&p); editor_project_destroy(&p);
    if(argc==2) { EditorWorkspace w={0}; editor_project_init(&p);
        OK(editor_workspace_create(&w,&p,argv[1])); author(&p); delivery_values(&p);
        p.selected=p.objects[p.object_count-1].id;
        assert(editor_workspace_save(&w,&p) && editor_workspace_c_generate(&w,&p)); editor_project_destroy(&p); }
    remove("soft_physics.json"); rohr_engine_stop(); puts("soft physics passed"); return 0;
}
