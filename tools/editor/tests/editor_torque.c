/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_force_geometry.h"
#include "editor_navigation.h"
#include "editor_layout.h"
#include "editors/physics/editor_force.h"
#include "entity/object_internal.h"
#include "entity/object_visuals.h"
#include "yyjson/yyjson.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

float editor_viewport_width=1024,editor_window_width=1280,editor_window_height=720,editor_viewport_bottom=720;
static void near(float a,float b) { assert(fabsf(a-b)<.005f); }
static void point(Position a,Position b) { near(a.x,b.x); near(a.y,b.y); }
static Position screen(Position p,float zoom) { return (Position){512+p.x*zoom,377-p.y*zoom}; }
static void begin(const EditorProject *p,const EditorCommand *c,void *h) { editor_history_command_begin(h,p,c); }
static void finish(const EditorCommand *c,const EditorCommandResult *r,void *h) { editor_history_command_finish(h,c,r); }
static bool pointer(EditorProject *p,EditorViewportState *s,EditorHistory *h,Position at,MouseButtonState button) {
    bool active=editor_viewport_transform_active_check(s);
    bool result=editor_viewport_update(s,p,at,button,MOUSE_BUTTON_STATE_UP,false,0,false);
    assert(editor_navigation_viewport_transform_history_update(p,s,h,active)); return result;
}
static void panel(EditorTorqueEditor *ui,EditorModeContext *c,Position at,MouseButtonState button) {
    rohr_ui_input_set((UIInput){at,button}); (void)editor_torque_editor_draw(ui,c); rohr_ui_update();
}
static void legacy_check(const char *path) {
    yyjson_doc *doc=yyjson_read_file(path,0,NULL,NULL); assert(doc);
    for(unsigned version=8;version<=9;++version) {
        yyjson_mut_doc *copy=yyjson_doc_mut_copy(doc,NULL); assert(copy);
        yyjson_mut_val *root=yyjson_mut_doc_get_root(copy);
        assert(yyjson_mut_obj_put(root,yyjson_mut_str(copy,"format_version"),yyjson_mut_uint(copy,version)));
        yyjson_mut_val *objects=yyjson_mut_obj_get(root,"objects");
        for(size_t i=0;i<yyjson_mut_arr_size(objects);++i) {
            yyjson_mut_val *o=yyjson_mut_arr_get(objects,i),*items=yyjson_mut_obj_get(o,"torques");
            for(size_t n=0;n<yyjson_mut_arr_size(items);++n) {
                yyjson_mut_val *t=yyjson_mut_arr_get(items,n);
                yyjson_mut_obj_remove_key(t,"visible"); yyjson_mut_obj_remove_key(t,"marker_offset_x"); yyjson_mut_obj_remove_key(t,"marker_offset_y");
            }
            yyjson_mut_val *hierarchy=yyjson_mut_obj_get(o,"hierarchy");
            for(size_t n=yyjson_mut_arr_size(hierarchy);n>0;--n)
                if(yyjson_mut_get_uint(yyjson_mut_obj_get(yyjson_mut_arr_get(hierarchy,n-1),"kind"))==EDITOR_HIERARCHY_TORQUE)
                    yyjson_mut_arr_remove(hierarchy,n-1);
        }
        assert(yyjson_mut_write_file("/tmp/rohr_torque_legacy.json",copy,0,NULL,NULL));
        EditorProject loaded={0}; editor_project_init(&loaded);
        assert(!editor_result_check(editor_project_load(&loaded,"/tmp/rohr_torque_legacy.json")));
        assert(loaded.objects[0].torque_items[0].visible);
        point(loaded.objects[0].torque_items[0].marker_offset,(Position){0});
        editor_project_destroy(&loaded); yyjson_mut_doc_free(copy);
    }
    yyjson_doc_free(doc);
}
int main(void) {
    assert(!rohr_error_check(rohr_engine_start())); assert(!rohr_error_check(rohr_graphics_start()));
    EditorProject p={0}; editor_project_init(&p); p.viewport_local_view=false; p.viewport_camera_zoom=1;
    EditorObject *o=editor_project_object_add(&p,(Position){20,30}); assert(o); uint32_t oid=o->id;
    EditorRigidBody *body=editor_project_rigid_body_add(&p,o); assert(body); body->position=(Position){10,15};
    ObjectSourceReference refs[7]={{0},{OBJECT_SOURCE_BODY,body->id,0}};
    EditorRigidBody *particle=editor_project_rigid_body_add(&p,o); assert(particle);
    particle->standalone_particle=true; particle->particle=true; particle->particle_radius=8;
    refs[2]=(ObjectSourceReference){OBJECT_SOURCE_PARTICLE,particle->id,0};
    EditorSoftBody *soft=editor_project_soft_body_add(&p,o); assert(soft);
    refs[3]=(ObjectSourceReference){OBJECT_SOURCE_SOFT_BODY,soft->id,0};
    EditorSoftNode *node=editor_project_soft_node_add(&p,soft,(Position){0,0}); assert(node);
    refs[4]=(ObjectSourceReference){OBJECT_SOURCE_NODE,node->id,soft->id};
    node=editor_project_soft_node_add(&p,soft,(Position){20,0}); assert(node);
    refs[5]=(ObjectSourceReference){OBJECT_SOURCE_NODE,node->id,soft->id};
    EditorSoftBeam *beam=editor_project_soft_beam_add(&p,soft,refs[4].id,refs[5].id); assert(beam);
    refs[6]=(ObjectSourceReference){OBJECT_SOURCE_BEAM,beam->id,soft->id};
    EditorCommand add={.type=EDITOR_COMMAND_ITEM_ADD,.data.item_add={.kind=EDITOR_ITEM_TORQUE,.object=oid}};
    EditorCommandResult added=editor_command_execute(&p,&add); assert(added.kind==ERROR_RESULT_VALUE);
    uint32_t tid=added.created.item; EditorTorque *t=editor_project_torque_get(o,tid); assert(t && t->visible && !t->app_hidden);
    EditorViewportState state={0}; editor_viewport_state_init(&state);
    assert(editor_viewport_selection_set(&p,&state,(EditorSelectionRef){EDITOR_SELECTION_TORQUE,oid,0,0,tid},false));
    assert(editor_navigation_selected_open(&p,&state) && state.mode==EDITOR_VIEWPORT_TORQUE);
    point(editor_torque_marker_position_get(o,t),o->position);
    for(size_t i=1;i<7;++i) {
        t->description.target=refs[i]; Position base;
        assert(editor_torque_target_position_get(o,t,&base));
        t->marker_offset=(Position){7,-3}; point(editor_torque_marker_position_get(o,t),(Position){base.x+7,base.y-3});
    }
    t->description=(ObjectTorqueDescription){.magnitude=10}; t->marker_offset=(Position){0};
    EditorHistory h={0}; assert(editor_history_init(&h,&p));
    editor_command_executing_callback_set(begin,&h); editor_command_finished_callback_set(finish,&h);
    for(unsigned z=0;z<3;++z) for(int sign=-1;sign<=1;++sign) {
        float zoom=z==0?.25f:z==1?1:4; p.viewport_camera_zoom=zoom;
        t=editor_project_torque_get(o,tid); t->description.magnitude=sign; t->app_hidden=true; t->marker_offset=(Position){0};
        Position center=editor_torque_marker_position_get(o,t),strokes[EDITOR_TORQUE_MARKER_STROKE_COUNT][2];
        assert(editor_torque_marker_strokes_get(o,t,zoom,strokes));
        near((strokes[0][0].y-center.y)*zoom,44);
        assert(sign<0?strokes[0][1].x<center.x:strokes[0][1].x>center.x);
        Position at=strokes[16][0]; EditorSelectionRef hit;
        assert(editor_viewport_selection_at_get(&p,&state,screen(at,zoom),&hit) && hit.kind==EDITOR_SELECTION_TORQUE);
        Position offset=t->marker_offset; size_t count=h.undo_count;
        assert(pointer(&p,&state,&h,screen(at,zoom),MOUSE_BUTTON_STATE_PRESSED) && state.dragged_torque);
        point(t->marker_offset,offset);
        assert(pointer(&p,&state,&h,screen((Position){at.x+12,at.y-6},zoom),MOUSE_BUTTON_STATE_DOWN));
        point(t->marker_offset,(Position){offset.x+12,offset.y-6}); near(t->description.magnitude,sign);
        (void)pointer(&p,&state,&h,screen(at,zoom),MOUSE_BUTTON_STATE_RELEASED);
        assert(h.undo_count==count+1 && editor_history_undo(&h));
        point(editor_project_torque_get(o,tid)->marker_offset,offset);
        assert(editor_history_redo(&h)); point(editor_project_torque_get(o,tid)->marker_offset,(Position){offset.x+12,offset.y-6});
    }
    p.viewport_camera_zoom=1; t=editor_project_torque_get(o,tid); t->description.target=refs[1]; t->marker_offset=(Position){0};
    Position center=editor_torque_marker_position_get(o,t),press={center.x+44,center.y};
    assert(pointer(&p,&state,&h,screen(press,1),MOUSE_BUTTON_STATE_PRESSED));
    assert(pointer(&p,&state,&h,screen((Position){press.x+9,press.y+4},1),MOUSE_BUTTON_STATE_DOWN));
    editor_history_transaction_cancel(&h); editor_viewport_transform_cancel(&state);
    point(editor_project_torque_get(o,tid)->marker_offset,(Position){0});
    /* App preview selection does not drag either visual or physics position. */
    t=editor_project_torque_get(o,tid); t->app_hidden=false;
    t->visuals.main=(ObjectVisualStyle){.specified=true,.fill_color=0xff6633ff,.scale={1,1},.offset={200,0}};
    Position preview={center.x+222,center.y};
    assert(pointer(&p,&state,&h,screen(preview,1),MOUSE_BUTTON_STATE_PRESSED)); assert(!state.dragged_torque);
    (void)pointer(&p,&state,&h,screen(preview,1),MOUSE_BUTTON_STATE_RELEASED);
    point(t->marker_offset,(Position){0}); point(t->visuals.main.offset,(Position){200,0});
    /* A selected torque and its target move once, regardless of selection order. */
    for(unsigned order=0;order<2;++order) {
        EditorSelectionRef tr={EDITOR_SELECTION_TORQUE,oid,0,0,tid},br={EDITOR_SELECTION_RIGID_BODY,oid,0,0,refs[1].id};
        editor_viewport_selection_clear(&state);
        assert(editor_viewport_selection_set(&p,&state,order?tr:br,false));
        assert(editor_viewport_selection_set(&p,&state,order?br:tr,true));
        center=editor_torque_marker_position_get(o,editor_project_torque_get(o,tid));
        state.group_dragging=true; state.group_pointer=center;
        assert(editor_history_transaction_begin(&h)); assert(editor_history_transaction_object_track(&h,oid));
        assert(pointer(&p,&state,&h,screen((Position){center.x+15,center.y+8},1),MOUSE_BUTTON_STATE_DOWN));
        t=editor_project_torque_get(o,tid); point(t->marker_offset,(Position){0});
        point(editor_torque_marker_position_get(o,t),(Position){center.x+15,center.y+8});
        (void)pointer(&p,&state,&h,screen(center,1),MOUSE_BUTTON_STATE_RELEASED); assert(editor_history_undo(&h));
    }
    editor_viewport_selection_clear(&state);
    assert(editor_viewport_selection_set(&p,&state,(EditorSelectionRef){EDITOR_SELECTION_TORQUE,oid,0,0,tid},false));
    state.mode=EDITOR_VIEWPORT_TORQUE;
    assert(editor_navigation_selection_visibility_set(&p,(EditorSelectionRef){EDITOR_SELECTION_TORQUE,oid,0,0,tid},false));
    assert(!editor_project_torque_get(o,tid)->visible && editor_history_undo(&h));
    assert(editor_project_torque_get(o,tid)->visible);
    EditorCommand marker={.type=EDITOR_COMMAND_SOURCE,.data.source={.object=oid,.id=tid,.torque=true,.operation=EDITOR_SOURCE_MARKER_SET,.marker_offset={19,23}}};
    assert(editor_command_execute(&p,&marker).kind==ERROR_RESULT_VALUE);
    char cli[2048]; assert(!editor_result_check(editor_command_cli_standard_write(&p,&marker,NULL,"project.json",cli,sizeof(cli))));
    assert(strstr(cli,"--torque-id") && strstr(cli,"marker_offset"));
    char object_id[24],torque_id[24]; snprintf(object_id,sizeof(object_id),"%u",oid); snprintf(torque_id,sizeof(torque_id),"%u",tid);
    char *args[]={"rohr-cli","--project","project.json","--object-id",object_id,"--torque-id",torque_id,"--property","marker_offset","set","{\"x\":19,\"y\":23}"};
    EditorCommand parsed; const char *path; bool handled;
    assert(!editor_result_check(editor_source_cli_parse(&p,11,args,&path,&parsed,&handled)) && handled);
    point(parsed.data.source.marker_offset,(Position){19,23});
    marker.data.source.marker_offset.x=NAN; assert(editor_command_execute(&p,&marker).kind==ERROR_RESULT_ERROR);
    point(editor_project_torque_get(o,tid)->marker_offset,(Position){19,23});
    assert(editor_navigation_selection_name_set(&p,(EditorSelectionRef){EDITOR_SELECTION_TORQUE,oid,0,0,tid},"drive_torque"));
    assert(!strcmp(editor_project_torque_get(o,tid)->name,"drive_torque"));
    EditorCommand duplicate={.type=EDITOR_COMMAND_SOURCE,.data.source={.object=oid,.id=tid,.torque=true,.operation=EDITOR_SOURCE_DUPLICATE}};
    EditorCommandResult duplicated=editor_command_execute(&p,&duplicate); assert(duplicated.kind==ERROR_RESULT_VALUE);
    t=editor_project_torque_get(o,duplicated.created.item); assert(t && t->id!=tid && strcmp(t->name,"drive_torque"));
    point(t->marker_offset,(Position){19,23}); point(t->visuals.main.offset,(Position){200,0});
    assert(editor_history_undo(&h) && o->torque_count==1);
    EditorTorqueClipboard clipboard={0}; assert(editor_navigation_torques_copy(&p,&state,&clipboard));
    assert(editor_navigation_torques_paste(&p,&state,&h,&clipboard)); assert(o->torque_count==2);
    t=editor_project_torque_get(o,state.selected_torque); point(t->marker_offset,(Position){19,23}); assert(t->description.target.id==refs[1].id);
    uint32_t copied=t->id;
    assert(editor_navigation_selection_reorder(&p,&state,(EditorSelectionRef){EDITOR_SELECTION_TORQUE,oid,0,0,copied},(EditorSelectionRef){EDITOR_SELECTION_RIGID_BODY,oid,0,0,refs[1].id},false,&h));
    assert(o->hierarchy[0].kind==EDITOR_HIERARCHY_TORQUE);
    p.navigation=editor_navigation_state_get(&p,&state);
    editor_project_torque_get(o,copied)->visible=false;
    assert(editor_project_save(&p,"/tmp/rohr_torque_editor.json"));
    EditorProject loaded={0}; editor_project_init(&loaded);
    assert(!editor_result_check(editor_project_load(&loaded,"/tmp/rohr_torque_editor.json")));
    assert(loaded.navigation.torque==copied && loaded.navigation.mode==EDITOR_VIEWPORT_TORQUE);
    assert(!editor_project_torque_get(&loaded.objects[0],copied)->visible);
    point(loaded.objects[0].torque_items[0].marker_offset,(Position){19,23}); editor_project_destroy(&loaded);
    editor_project_torque_get(o,copied)->visible=true;
    legacy_check("/tmp/rohr_torque_editor.json");
    /* Invalid serialized offsets reject the load without replacing the project. */
    yyjson_doc *saved=yyjson_read_file("/tmp/rohr_torque_editor.json",0,NULL,NULL); assert(saved);
    yyjson_mut_doc *bad=yyjson_doc_mut_copy(saved,NULL); assert(bad);
    yyjson_mut_val *first=yyjson_mut_arr_get(yyjson_mut_obj_get(yyjson_mut_arr_get(
        yyjson_mut_obj_get(yyjson_mut_doc_get_root(bad),"objects"),0),"torques"),0);
    assert(yyjson_mut_obj_put(first,yyjson_mut_str(bad,"marker_offset_x"),yyjson_mut_real(bad,1e100)));
    assert(yyjson_mut_write_file("/tmp/rohr_torque_invalid.json",bad,0,NULL,NULL));
    assert(editor_result_check(editor_project_load(&p,"/tmp/rohr_torque_invalid.json")));
    assert(p.objects[0].id==oid && p.objects[0].torque_count==2);
    yyjson_mut_doc_free(bad); yyjson_doc_free(saved);
    /* Target deletion retains marker metadata and undo reconnects the source. */
    EditorCommand remove={.type=EDITOR_COMMAND_ITEM_REMOVE,
        .data.item_remove={.kind=EDITOR_ITEM_RIGID_BODY,.object=oid,.item=refs[1].id}};
    assert(editor_command_execute(&p,&remove).kind==ERROR_RESULT_VALUE);
    t=editor_project_torque_get(o,copied); assert(t->description.target.kind==OBJECT_SOURCE_NONE);
    point(editor_torque_marker_position_get(o,t),(Position){o->position.x+19,o->position.y+23});
    assert(editor_history_undo(&h)); assert(editor_project_torque_get(o,copied)->description.target.id==refs[1].id);
    /* Rotation moves the marker about the group pivot without a torque angle. */
    assert(editor_viewport_selection_set(&p,&state,(EditorSelectionRef){EDITOR_SELECTION_RIGID_BODY,oid,0,0,refs[1].id},true));
    t=editor_project_torque_get(o,copied); center=editor_torque_marker_position_get(o,t);
    ObjectTorqueDescription before=t->description;
    state.group_rotating=true; state.group_pivot=(Position){0}; state.group_pointer_angle=0;
    assert(editor_history_transaction_begin(&h)); assert(editor_history_transaction_object_track(&h,oid));
    assert(pointer(&p,&state,&h,screen((Position){100,0},1),MOUSE_BUTTON_STATE_DOWN));
    t=editor_project_torque_get(o,copied);
    point(editor_torque_marker_position_get(o,t),(Position){center.y,-center.x});
    assert(t->description.magnitude==before.magnitude && t->description.target.id==before.target.id);
    (void)pointer(&p,&state,&h,screen(center,1),MOUSE_BUTTON_STATE_RELEASED); assert(editor_history_undo(&h));
    editor_viewport_selection_clear(&state);
    assert(editor_viewport_selection_set(&p,&state,(EditorSelectionRef){EDITOR_SELECTION_TORQUE,oid,0,0,copied},false));
    state.mode=EDITOR_VIEWPORT_TORQUE;
    FontAsset font=rohr_graphics_font_default_get(); EditorTorqueEditor ui={0}; assert(editor_torque_editor_create(&ui,&font));
    editor_mode_accordion_section_expanded_set(&ui.visuals.outer,false);
    EditorModeContext context={.project=&p,.viewport=&state,.history=&h,.width=320};
    panel(&ui,&context,(Position){200,232},MOUSE_BUTTON_STATE_UP);
    assert(state.preview_reference_object==oid && state.preview_reference.id==refs[1].id);
    editor_viewport_draw(&p,&state,false); (void)rohr_graphics_pipeline_commands_execute();
    for(size_t i=0;i<7;++i) {
        editor_project_torque_get(o,copied)->description.target=refs[i];
        panel(&ui,&context,(Position){0},MOUSE_BUTTON_STATE_UP);
        if(i==2 || i==4 || i==5) assert(!strcmp(ui.label_cache[ui.row-1],"No angular effect"));
    }
    editor_project_rigid_body_get(o,refs[1].id)->particle=true;
    editor_project_torque_get(o,copied)->description.target=refs[1];
    panel(&ui,&context,(Position){0},MOUSE_BUTTON_STATE_UP);
    assert(strcmp(ui.label_cache[ui.row-1],"No angular effect"));
    editor_force_editor_destroy(&ui);
    /* Cross-object paste clears unavailable targets but retains editor/style state. */
    EditorObject *other=editor_project_object_add(&p,(Position){0}); assert(other); p.selected=other->id;
    assert(editor_navigation_torques_paste(&p,&state,&h,&clipboard));
    t=editor_project_torque_get(other,state.selected_torque); assert(t && t->description.target.kind==OBJECT_SOURCE_NONE);
    point(t->marker_offset,(Position){19,23}); assert(t->visuals.main.offset.x==200);
    assert(editor_history_undo(&h)); assert(other->torque_count==0);
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&h); editor_viewport_state_destroy(&state); editor_project_destroy(&p);
    editor_viewport_assets_destroy(); rohr_graphics_stop(); rohr_engine_stop();
    puts("torque targets, marker geometry, drag/history, visibility, clipboard, panel and persistence passed"); return 0;
}
