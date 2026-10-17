/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_force_geometry.h"
#include "editor_navigation.h"
#include "editor_layout.h"
#include "editors/physics/editor_force.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

float editor_viewport_width=1024,editor_window_width=1280,editor_window_height=720,editor_viewport_bottom=720;
static void near(float a,float b) { assert(fabsf(a-b)<0.005f); }
static void point(Position a,Position b) { near(a.x,b.x); near(a.y,b.y); }
static void begin(const EditorProject *p,const EditorCommand *c,void *h) { editor_history_command_begin(h,p,c); }
static void finish(const EditorCommand *c,const EditorCommandResult *r,void *h) { editor_history_command_finish(h,c,r); }
static Position screen(Position p,float zoom) { return (Position){512+p.x*zoom,EDITOR_MENU_HEIGHT+(720-EDITOR_MENU_HEIGHT)*0.5f-p.y*zoom}; }
static bool pointer(EditorProject *p,EditorViewportState *s,EditorHistory *h,Position at,MouseButtonState button) {
    bool active=editor_viewport_transform_active_check(s);
    bool result=editor_viewport_update(s,p,at,button,MOUSE_BUTTON_STATE_UP,false,0,false);
    assert(editor_navigation_viewport_transform_history_update(p,s,h,active)); return result;
}
static void panel(EditorForceEditor *ui,EditorModeContext *context,Position at,MouseButtonState button) {
    rohr_ui_input_set((UIInput){at,button});
    (void)editor_force_editor_draw(ui,context); rohr_ui_update();
}
static bool picked(EditorProject *p,EditorViewportState *state,Position world,EditorHierarchySelection kind,uint32_t id) {
    EditorSelectionRef hit={0};
    bool found=editor_viewport_selection_at_get(p,state,screen(world,p->viewport_camera_zoom),&hit);
    bool ok=kind==EDITOR_SELECTION_NONE?!found:found && hit.kind==kind && hit.item==id;
    if(!ok) fprintf(stderr,"pick at %.2f,%.2f zoom %.2f: found %u kind %u id %u; expected %u id %u\n",
        world.x,world.y,p->viewport_camera_zoom,found,hit.kind,hit.item,kind,id);
    return ok;
}
static void marker_picking_check(void) {
    EditorProject p={0}; editor_project_init(&p); p.viewport_local_view=false; p.viewport_camera_zoom=1;
    EditorObject *o=editor_project_object_add(&p,(Position){0}); assert(o);
    EditorAnchor *a=editor_project_anchor_add(&p,o,(Position){0},0); assert(a); uint32_t aid=a->id;
    EditorAnchor *b=editor_project_anchor_add(&p,o,(Position){0},0); assert(b); uint32_t bid=b->id;
    EditorJoint *j=editor_project_joint_add(&p,o,EDITOR_JOINT_REVOLUTE); assert(j);
    j->anchor_a=aid; j->anchor_b=bid; j->visual_size=3;
    uint32_t jid=j->id;
    EditorViewportState state={0}; editor_viewport_state_init(&state); state.mode=EDITOR_VIEWPORT_OBJECT;
    for(unsigned kind=0;kind<2;++kind) for(unsigned z=0;z<3;++z) {
        j=&o->joint_items[0]; j->kind=kind?EDITOR_JOINT_WELD:EDITOR_JOINT_REVOLUTE;
        float zoom=z==0?.25f:z==1?1:4; p.viewport_camera_zoom=zoom;
        assert(picked(&p,&state,(Position){0},EDITOR_SELECTION_ANCHOR,bid));
        Position hit=kind?(Position){27+2/zoom,20}:(Position){(27+2/zoom)*.70710678f,(27+2/zoom)*.70710678f};
        assert(picked(&p,&state,hit,EDITOR_SELECTION_JOINT,jid));
        assert(editor_viewport_update(&state,&p,screen(hit,zoom),MOUSE_BUTTON_STATE_PRESSED,MOUSE_BUTTON_STATE_UP,false,0,false));
        assert(state.mode==EDITOR_VIEWPORT_JOINT && state.selected_joint==jid && !state.dragged_anchor);
        (void)editor_viewport_update(&state,&p,screen(hit,zoom),MOUSE_BUTTON_STATE_RELEASED,MOUSE_BUTTON_STATE_UP,false,0,false);
        state.mode=EDITOR_VIEWPORT_OBJECT;
    }
    p.viewport_camera_zoom=1; j->kind=EDITOR_JOINT_REVOLUTE; j->app_hidden=true;
    assert(picked(&p,&state,(Position){27,0},EDITOR_SELECTION_JOINT,jid));
    j->visual_size=1; assert(picked(&p,&state,(Position){27,0},EDITOR_SELECTION_NONE,0));
    /* The actual rotated anchor diamond remains usable, with no world-size disk. */
    a=editor_project_anchor_get(o,aid); b=editor_project_anchor_get(o,bid);
    b->visible=false; a->rotation=45;
    assert(picked(&p,&state,(Position){4,4},EDITOR_SELECTION_ANCHOR,aid));
    a->visible=false; j->visible=false;
    EditorForce *f=editor_project_force_add(&p,o); assert(f); uint32_t fid=f->id;
    f->description.magnitude=10;
    /* Force origin/shaft/head remain selectable independently of app visibility. */
    f->app_hidden=true;
    assert(picked(&p,&state,(Position){0,0},EDITOR_SELECTION_FORCE,fid));
    assert(picked(&p,&state,(Position){9,36},EDITOR_SELECTION_FORCE,fid));
    f->app_hidden=false;
    f->visuals.main=(ObjectVisualStyle){.specified=true,.fill_color=0xff8844ff,.border_color=0xffffffff,
        .border_thickness=3,.scale={-2,.5f},.offset={100,30},.rotation=90};
    Position preview={112.5f,30};
    assert(picked(&p,&state,preview,EDITOR_SELECTION_FORCE,fid));
    EditorCamera *camera=editor_project_camera_add(&p,o); assert(camera);
    camera->position=(Position){0,100}; camera->dimensions=(Scale){400,300}; camera->zoom=1;
    assert(picked(&p,&state,preview,EDITOR_SELECTION_FORCE,fid));
    assert(picked(&p,&state,(Position){0,100},EDITOR_SELECTION_CAMERA,camera->id));
    assert(picked(&p,&state,(Position){200,100},EDITOR_SELECTION_CAMERA,camera->id));
    assert(picked(&p,&state,(Position){100,100},EDITOR_SELECTION_NONE,0));
    f->app_hidden=true; assert(picked(&p,&state,preview,EDITOR_SELECTION_NONE,0)); f->app_hidden=false;
    o->app_hidden=true; assert(picked(&p,&state,preview,EDITOR_SELECTION_NONE,0)); o->app_hidden=false;
    f->visuals.main.fill_color=0;
    assert(picked(&p,&state,preview,EDITOR_SELECTION_NONE,0));
    assert(picked(&p,&state,(Position){112.5f,38},EDITOR_SELECTION_FORCE,fid));
    f->visuals.main.fill_color=0; f->visuals.main.border_color=0;
    assert(picked(&p,&state,preview,EDITOR_SELECTION_NONE,0));
    f->visuals.main.fill_color=0xff8844ff; f->visuals.main.scale.x=0;
    assert(picked(&p,&state,preview,EDITOR_SELECTION_NONE,0)); f->visuals.main.scale.x=-2;
    f->visible=false; assert(picked(&p,&state,(Position){0,20},EDITOR_SELECTION_NONE,0)); f->visible=true;
    /* Starting from the offset preview must neither snap nor change its style. */
    EditorHistory history={0}; assert(editor_history_init(&history,&p));
    editor_command_executing_callback_set(begin,&history); editor_command_finished_callback_set(finish,&history);
    assert(pointer(&p,&state,&history,screen(preview,1),MOUSE_BUTTON_STATE_PRESSED));
    assert(state.mode==EDITOR_VIEWPORT_FORCE && state.dragged_force);
    point(f->description.location.offset,(Position){0});
    assert(pointer(&p,&state,&history,screen((Position){preview.x+12,preview.y+6},1),MOUSE_BUTTON_STATE_DOWN));
    point(f->description.location.offset,(Position){12,6});
    (void)pointer(&p,&state,&history,screen(preview,1),MOUSE_BUTTON_STATE_RELEASED);
    assert(history.undo_count==1 && editor_history_undo(&history));
    o=editor_project_selected_get(&p); f=&o->force_items[0]; point(f->description.location.offset,(Position){0});
    assert(f->visuals.main.offset.x==100 && f->visuals.main.rotation==90);
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_history_destroy(&history); camera=&o->cameras[0]; camera->visible=false; f->visible=false;
    /* Spring coil and each transformed marker use the app's exact geometry. */
    j=&o->joint_items[0]; j->visible=true; j->app_hidden=false; j->kind=EDITOR_JOINT_SPRING;
    a=editor_project_anchor_get(o,aid); b=editor_project_anchor_get(o,bid); a->position=(Position){-50,0}; b->position=(Position){50,0};
    j->visuals.main=(ObjectVisualStyle){.specified=true,.fill_color=0xff8844ff,.scale={1,1},.offset={0,100}};
    j->visuals.endpoint_a=(ObjectVisualStyle){.specified=true,.fill_color=0xffffffff,.scale={3,.5f},.offset={10,20},.rotation=90};
    j->visuals.endpoint_b=(ObjectVisualStyle){.specified=true,.fill_color=0xffffffff,.scale={2,1},.offset={-5,-10}};
    assert(picked(&p,&state,(Position){-40,120},EDITOR_SELECTION_JOINT,jid));
    assert(picked(&p,&state,(Position){45,90},EDITOR_SELECTION_JOINT,jid));
    assert(picked(&p,&state,(Position){-40,128},EDITOR_SELECTION_JOINT,jid));
    float length=hypotf(85,30);
    Position coil={-40+85/18.0f-7*30/length*.5f,120-30/18.0f-7*85/length*.5f};
    assert(picked(&p,&state,coil,EDITOR_SELECTION_JOINT,jid));
    assert(picked(&p,&state,(Position){-32,120},EDITOR_SELECTION_NONE,0));
    j->app_hidden=true; assert(picked(&p,&state,(Position){-40,120},EDITOR_SELECTION_NONE,0));
    editor_viewport_state_destroy(&state); editor_project_destroy(&p);
}
int main(void) {
    assert(!rohr_error_check(rohr_engine_start()));
    assert(!rohr_error_check(rohr_graphics_start()));
    marker_picking_check();
    EditorProject p={0}; editor_project_init(&p);
    EditorObject *o=editor_project_object_add(&p,(Position){20,30}); assert(o);
    uint32_t oid=o->id;
    EditorRigidBody *b=editor_project_rigid_body_add(&p,o); assert(b); b->position=(Position){10,15};
    b->rotation=90; b->center_of_mass_explicit=true; b->center_of_mass_offset=(Position){2,3};
    ObjectSourceReference body={OBJECT_SOURCE_BODY,b->id,0};
    Position out;
    ObjectForceLocation l={.kind=ROHR_FORCE_ORIGIN_POSITION,.reference=body,.offset={5,7}};
    assert(editor_force_location_get(o,l,&out)); point(out,(Position){37,40});
    assert(editor_force_location_set(o,&l,(Position){39,42}));
    assert(l.reference.id==body.id); assert(editor_force_location_get(o,l,&out)); point(out,(Position){39,42});
    l=(ObjectForceLocation){.kind=ROHR_FORCE_COM_POSITION,.reference=body,.offset={5,7}};
    assert(editor_force_location_get(o,l,&out)); point(out,(Position){38,50});
    EditorSoftBody *soft=editor_project_soft_body_add(&p,o); assert(soft);
    soft->position=(Position){5,8}; soft->rotation=90;
    EditorSoftNode *n=editor_project_soft_node_add(&p,soft,(Position){0,0}); assert(n); uint32_t na=n->id;
    n=editor_project_soft_node_add(&p,soft,(Position){10,0}); assert(n); uint32_t nb=n->id;
    EditorSoftBeam *beam=editor_project_soft_beam_add(&p,soft,na,nb); assert(beam);
    ObjectSourceReference beamref={OBJECT_SOURCE_BEAM,beam->id,soft->id};
    l=(ObjectForceLocation){.kind=ROHR_FORCE_BEAM_POSITION,.reference=beamref,.anchor=ROHR_FORCE_BEAM_CENTER,.fraction=.2f,.offset={3,4}};
    assert(editor_force_location_get(o,l,&out)); point(out,(Position){29,28});
    assert(editor_force_location_set(o,&l,(Position){35,21})); near(l.fraction,.2f);
    assert(editor_force_location_get(o,l,&out)); point(out,(Position){35,21});
    ObjectForceOrientation angle={.kind=ROHR_FORCE_BODY_ORIENTATION,.reference=body,.offset=15}; float degrees;
    assert(editor_force_orientation_get(o,angle,&degrees)); near(degrees,105);
    angle=(ObjectForceOrientation){.kind=ROHR_FORCE_BEAM_ORIENTATION,.reference=beamref,.offset=20};
    assert(editor_force_orientation_get(o,angle,&degrees)); near(fabsf(degrees-20),180);
    assert(editor_force_orientation_set(o,&angle,45)); assert(editor_force_orientation_get(o,angle,&degrees)); near(degrees,45);
    angle=(ObjectForceOrientation){.kind=ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION,.to.offset={10,0},.offset=5};
    assert(editor_force_orientation_get(o,angle,&degrees)); near(degrees,95);
    angle.to.offset=(Vec2D){0}; assert(!editor_force_orientation_get(o,angle,&degrees));
    soft->nodes[1].position=soft->nodes[0].position; assert(!editor_force_location_get(o,l,&out)); soft->nodes[1].position=(Position){10,0};

    EditorHistory history={0}; assert(editor_history_init(&history,&p));
    editor_command_executing_callback_set(begin,&history); editor_command_finished_callback_set(finish,&history);
    EditorCommand add={.type=EDITOR_COMMAND_ITEM_ADD,.data.item_add={.kind=EDITOR_ITEM_FORCE,.object=oid}};
    EditorCommandResult added=editor_command_execute(&p,&add); assert(added.kind==ERROR_RESULT_VALUE);
    uint32_t id=added.created.item; EditorForce *f=editor_project_force_get(o,id); assert(f && f->visible);
    EditorViewportState s; editor_viewport_state_init(&s);
    assert(editor_viewport_selection_set(&p,&s,(EditorSelectionRef){EDITOR_SELECTION_FORCE,oid,0,0,id},false));
    assert(editor_navigation_selected_open(&p,&s)); assert(s.mode==EDITOR_VIEWPORT_FORCE);
    assert(editor_navigation_selection_visibility_set(&p,(EditorSelectionRef){EDITOR_SELECTION_FORCE,oid,0,0,id},false));
    assert(!editor_project_force_get(o,id)->visible); assert(editor_history_undo(&history)); assert(editor_project_force_get(o,id)->visible);
    /* Arrow drag at each zoom starts on selecting press and preserves offset. */
    for(size_t z=0;z<3;++z) {
        float zoom=z==0?.25f:z==1?1:4; p.viewport_camera_zoom=zoom; p.viewport_local_view=false;
        f=editor_project_force_get(o,id); f->description=(ObjectForceDescription){.magnitude=z==1?-10:0};
        Position tail=screen((Position){0},zoom), press={tail.x+3,tail.y+(z==1?20:-20)};
        assert(pointer(&p,&s,&history,press,MOUSE_BUTTON_STATE_PRESSED)); assert(s.dragged_force);
        Position moved={press.x+20,press.y-10}; assert(pointer(&p,&s,&history,moved,MOUSE_BUTTON_STATE_DOWN));
        point(editor_project_force_get(o,id)->description.location.offset,(Position){20/zoom,10/zoom});
        (void)pointer(&p,&s,&history,moved,MOUSE_BUTTON_STATE_RELEASED); assert(!s.dragged_force);
        assert(editor_history_undo(&history)); point(editor_project_force_get(o,id)->description.location.offset,(Position){0});
        assert(editor_history_redo(&history)); point(editor_project_force_get(o,id)->description.location.offset,(Position){20/zoom,10/zoom});
    }
    p.viewport_camera_zoom=1;
    f=editor_project_force_get(o,id); f->description=(ObjectForceDescription){.magnitude=-2};
    Position rotation=screen((Position){0,-60},1);
    assert(pointer(&p,&s,&history,rotation,MOUSE_BUTTON_STATE_PRESSED)); assert(s.rotated_force);
    assert(pointer(&p,&s,&history,screen((Position){60,0},1),MOUSE_BUTTON_STATE_DOWN));
    near(editor_project_force_get(o,id)->description.orientation.offset,-90);
    editor_history_transaction_cancel(&history); editor_viewport_transform_cancel(&s);
    near(editor_project_force_get(o,id)->description.orientation.offset,0);
    f=editor_project_force_get(o,id); f->description.target=body; f->description.location=(ObjectForceLocation){.kind=ROHR_FORCE_ORIGIN_POSITION,.reference=body,.offset={1,2}};
    f->description.orientation=(ObjectForceOrientation){.kind=ROHR_FORCE_BODY_ORIENTATION,.reference=body};
    /* Moving a force and its referenced body is independent of selection order. */
    for(unsigned order=0;order<2;++order) {
        EditorSelectionRef fr={EDITOR_SELECTION_FORCE,oid,0,0,id}, br={EDITOR_SELECTION_RIGID_BODY,oid,0,0,body.id};
        editor_viewport_selection_clear(&s);
        assert(editor_viewport_selection_set(&p,&s,order?fr:br,false));
        assert(editor_viewport_selection_set(&p,&s,order?br:fr,true));
        s.mode=EDITOR_VIEWPORT_FORCE; f=editor_project_force_get(o,id);
        Position before; assert(editor_force_location_get(o,f->description.location,&before));
        Position offset=f->description.location.offset;
        s.group_dragging=true; s.group_pointer=before;
        assert(editor_history_transaction_begin(&history)); assert(editor_history_transaction_object_track(&history,oid));
        assert(pointer(&p,&s,&history,screen((Position){before.x+12,before.y+6},1),MOUSE_BUTTON_STATE_DOWN));
        Position after; assert(editor_force_location_get(o,editor_project_force_get(o,id)->description.location,&after));
        point(after,(Position){before.x+12,before.y+6}); point(editor_project_force_get(o,id)->description.location.offset,offset);
        (void)pointer(&p,&s,&history,screen(after,1),MOUSE_BUTTON_STATE_RELEASED);
        assert(editor_history_undo(&history));
    }
    editor_viewport_selection_clear(&s);
    assert(editor_viewport_selection_set(&p,&s,(EditorSelectionRef){EDITOR_SELECTION_FORCE,oid,0,0,id},false));
    editor_project_force_get(o,id)->app_hidden=true;
    editor_project_force_get(o,id)->visuals.main=(ObjectVisualStyle){.specified=true,
        .fill_color=0x12345678,.scale={-2,3},.offset={4,5},.rotation=30};
    EditorForceClipboard clipboard={0}; assert(editor_navigation_forces_copy(&p,&s,&clipboard));
    assert(editor_navigation_forces_paste(&p,&s,&history,&clipboard)); assert(o->force_count==2);
    EditorForce *copy=editor_project_force_get(o,s.selected_force); assert(copy && copy->id!=id && copy->description.target.id==body.id);
    assert(copy->app_hidden);
    assert(copy->visuals.main.fill_color==0x12345678 && copy->visuals.main.scale.x==-2);
    copy->description.magnitude=123; near(editor_project_force_get(o,id)->description.magnitude,-2);
    editor_project_object_hierarchy_sync(o);
    assert(editor_navigation_selection_reorder(&p,&s,(EditorSelectionRef){EDITOR_SELECTION_FORCE,oid,0,0,copy->id},(EditorSelectionRef){EDITOR_SELECTION_RIGID_BODY,oid,0,0,body.id},false,&history));
    assert(o->hierarchy[0].kind==EDITOR_HIERARCHY_FORCE);
    EditorObject *other=editor_project_object_add(&p,(Position){0}); assert(other); p.selected=other->id;
    assert(editor_navigation_forces_paste(&p,&s,&history,&clipboard));
    copy=editor_project_force_get(other,s.selected_force); assert(copy && !copy->description.target.id && copy->description.location.kind==ROHR_FORCE_WORLD_POSITION && copy->description.orientation.kind==ROHR_FORCE_WORLD_ORIENTATION);
    assert(editor_history_undo(&history)); assert(other->force_count==0); assert(editor_history_redo(&history)); assert(other->force_count==1);
    p.navigation=editor_navigation_state_get(&p,&s);
    assert(editor_project_save(&p,"/tmp/rohr_force_editor_test.json"));
    EditorProject loaded={0}; editor_project_init(&loaded);
    assert(editor_project_load(&loaded,"/tmp/rohr_force_editor_test.json").kind==ERROR_RESULT_VALUE);
    assert(loaded.navigation.mode==EDITOR_VIEWPORT_FORCE && loaded.navigation.force==s.selected_force);
    assert(loaded.objects[0].force_count==2 && loaded.objects[0].force_items[0].visible);
    assert(loaded.objects[0].hierarchy[0].kind==EDITOR_HIERARCHY_FORCE);
    assert(loaded.objects[0].force_items[0].app_hidden);
    FontAsset loaded_font=rohr_graphics_font_default_get(); EditorForceEditor loaded_ui={0};
    assert(editor_force_editor_create(&loaded_ui,&loaded_font));
    EditorModeContext loaded_context={.project=&loaded,.viewport=&s,.x=0,.width=320};
    panel(&loaded_ui,&loaded_context,(Position){0,0},MOUSE_BUTTON_STATE_UP);
    editor_viewport_draw(&loaded,&s,false);
    editor_force_editor_destroy(&loaded_ui);
    editor_project_destroy(&loaded);
    EditorCommand visibility={.type=EDITOR_COMMAND_VISIBILITY,.data.visibility={EDITOR_VISIBILITY_FORCE,oid,0,id,false}};
    char cli[1024];
    assert(!editor_result_check(editor_command_cli_standard_write(&p,&visibility,NULL,"a b.json",cli,sizeof(cli))));
    assert(strstr(cli,"--property visible set false"));
    char object_id[32],force_id[32]; snprintf(object_id,sizeof(object_id),"%u",oid); snprintf(force_id,sizeof(force_id),"%u",id);
    char *args[]={"rohr-cli","--project","a b.json","--object-id",object_id,"--force-id",force_id,"--property","visible","set","false"};
    const char *path=NULL; EditorCommand parsed; bool handled=false;
    assert(!editor_result_check(editor_source_cli_parse(&p,11,args,&path,&parsed,&handled)) && handled);
    assert(parsed.type==EDITOR_COMMAND_VISIBILITY && !parsed.data.visibility.visible);

    args[8]="visible_in_app";
    assert(!editor_result_check(editor_source_cli_parse(&p,11,args,&path,&parsed,&handled)) && handled);
    assert(parsed.type==EDITOR_COMMAND_APP_VISIBILITY && !parsed.data.app_visibility.visible);

    /* Property controls own their text resources and use command history. */
    FontAsset font=rohr_graphics_font_default_get(); EditorForceEditor ui={0};
    assert(editor_force_editor_create(&ui,&font));
    EditorModeContext context={.project=&p,.viewport=&s,.history=&history,.x=0,.width=320};
    other=editor_project_selected_get(&p); assert(other);
    editor_project_force_get(other,s.selected_force)->app_hidden=false;
    panel(&ui,&context,(Position){200,90},MOUSE_BUTTON_STATE_UP);
    assert(ui.content_height>300);
    panel(&ui,&context,(Position){200,90},MOUSE_BUTTON_STATE_PRESSED);
    panel(&ui,&context,(Position){200,90},MOUSE_BUTTON_STATE_RELEASED);
    panel(&ui,&context,(Position){0,0},MOUSE_BUTTON_STATE_UP);
    assert(editor_project_force_get(other,s.selected_force)->app_hidden);
    assert(editor_project_force_get(other,s.selected_force)->visible);
    assert(editor_history_undo(&history)); assert(!editor_project_force_get(other,s.selected_force)->app_hidden);
    /* Every property branch can mount, including unresolved between-locations. */
    f=editor_project_force_get(other,s.selected_force);
    f->description.orientation.kind=ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION;
    panel(&ui,&context,(Position){0,0},MOUSE_BUTTON_STATE_UP);
    editor_force_editor_destroy(&ui);
    editor_command_executing_callback_set(NULL,NULL); editor_command_finished_callback_set(NULL,NULL);
    editor_viewport_state_destroy(&s); editor_history_destroy(&history); editor_project_destroy(&p);
    editor_viewport_assets_destroy(); rohr_graphics_stop(); rohr_engine_stop();
    puts("force geometry, viewport controls, history, clipboard, properties and persistence passed"); return 0;
}
