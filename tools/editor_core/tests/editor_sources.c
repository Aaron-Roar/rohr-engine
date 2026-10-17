/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_command.h"
#include "editor_history.h"
#include "editor_workspace.h"
#include "editor_sources_json.h"
#include "entity/object_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define OK(x) assert((x).kind == ERROR_RESULT_VALUE)

#ifdef ROHR_TEST_ALLOCATION_WRAP
static long fail_after=-1;
void *__real_malloc(size_t); void *__real_calloc(size_t,size_t); void *__real_realloc(void *,size_t);
static bool reject(void) { if(fail_after<0) return false; if(fail_after==0) { fail_after=-1;return true; } --fail_after;return false; }
void *__wrap_malloc(size_t n) { return reject()?NULL:__real_malloc(n); }
void *__wrap_calloc(size_t n,size_t m) { return reject()?NULL:__real_calloc(n,m); }
void *__wrap_realloc(void *p,size_t n) { return reject()?NULL:__real_realloc(p,n); }
#endif
static void source_failure_tests(void) {
#ifdef ROHR_TEST_ALLOCATION_WRAP
    for(unsigned torque=0;torque<2;++torque) {
        EditorProject p;editor_project_init(&p);
        EditorObject *o=editor_project_object_add(&p,(Position){0});assert(o);
        uint32_t next=torque?p.next_torque_id:p.next_force_id;
        fail_after=0;
        bool failed=torque?editor_project_torque_add(&p,o)==NULL:editor_project_force_add(&p,o)==NULL;
        fail_after=-1;assert(failed && o->force_count==0 && o->torque_count==0);
        assert((torque?p.next_torque_id:p.next_force_id)==next);
        for(unsigned i=0;i<EDITOR_SOURCE_MAX;++i) assert(torque?(void *)editor_project_torque_add(&p,o):(void *)editor_project_force_add(&p,o));
        assert(torque?editor_project_torque_add(&p,o)==NULL:editor_project_force_add(&p,o)==NULL);
        editor_project_destroy(&p);
    }
    unsigned failed=0,passed=0;
    for(long n=0;n<30;++n) {
        EditorProject p;editor_project_init(&p);EditorObject *o=editor_project_object_add(&p,(Position){0});assert(o);
        assert(editor_project_force_add(&p,o));assert(editor_project_torque_add(&p,o));
        EditorObject copy={0};fail_after=n;bool ok=editor_project_object_clone(&copy,o);fail_after=-1;
        if(ok) {++passed;assert(copy.force_items!=o->force_items && copy.torque_items!=o->torque_items);editor_project_object_destroy(&copy);} else ++failed;
        assert(o->force_count==1 && o->torque_count==1);editor_project_destroy(&p);
    }
    assert(failed && passed);
#endif
}

static EditorCommandResult execute(EditorProject *p,EditorHistory *h,EditorCommand c) {
    editor_history_command_begin(h,p,&c);
    EditorCommandResult r=editor_command_execute(p,&c);
    editor_history_command_finish(h,&c,&r);return r;
}
static EditorCommand parse(EditorProject *p,int count,char **args) {
    EditorCommand c; const char *path=NULL;
    EditorResult r=editor_command_cli_standard_parse(p,count,args,&path,&c);
    if(editor_result_check(r)) fprintf(stderr,"parse: %s\n",r.result.error.message);
    OK(r);assert(path);return c;
}
static void malformed_document(const char *path,EditorProject *live) {
    yyjson_doc *d=yyjson_read_file(path,0,NULL,NULL);assert(d);
    yyjson_mut_doc *m=yyjson_doc_mut_copy(d,NULL);assert(m); yyjson_doc_free(d);
    yyjson_mut_val *objects=yyjson_mut_obj_get(yyjson_mut_doc_get_root(m),"objects");
    yyjson_mut_val *force_values=yyjson_mut_obj_get(yyjson_mut_arr_get(objects,0),"forces");
    yyjson_mut_val *target=yyjson_mut_obj_get(yyjson_mut_arr_get(force_values,0),"target");
    assert(yyjson_mut_obj_put(target,yyjson_mut_str(m,"id"),yyjson_mut_uint(m,UINT32_MAX)));
    assert(yyjson_mut_write_file("sources_invalid.json",m,0,NULL,NULL)); yyjson_mut_doc_free(m);
    size_t before=live->objects[0].force_count;
    assert(editor_result_check(editor_project_load(live,"sources_invalid.json")));
    assert(live->objects[0].force_count==before);
    assert(remove("sources_invalid.json")==0);
}
int main(void) {
    OK(engine_start()); assert(ENTITY_NONE==ENTITY_INVALID); source_failure_tests();
    EditorProject p;editor_project_init(&p);
    EditorObject *o=editor_project_object_add(&p,(Position){0});assert(o);
    uint32_t object=o->id;
    EditorRigidBody *rigid=editor_project_rigid_body_add(&p,o);assert(rigid);
    uint32_t rigid_id=rigid->id;
    EditorSoftBody *body=editor_project_soft_body_add(&p,o);assert(body);uint32_t body_id=body->id;
    EditorSoftNode *node=editor_project_soft_node_add(&p,body,(Position){0});assert(node);uint32_t node_id=node->id;
    EditorSoftNode *other=editor_project_soft_node_add(&p,body,(Position){10,0});assert(other);
    EditorSoftBeam *beam=editor_project_soft_beam_add(&p,body,node_id,other->id);assert(beam);uint32_t beam_id=beam->id;
    ObjectSourceReference target={OBJECT_SOURCE_NODE,node_id,body_id};
    ObjectSourceReference beam_ref={OBJECT_SOURCE_BEAM,beam_id,body_id};
    ObjectSourceReference rigid_ref={OBJECT_SOURCE_BODY,rigid_id,0};
    EditorHistory h;assert(editor_history_init(&h,&p));
    EditorCommand add={.type=EDITOR_COMMAND_SOURCE,.data.source={.object=object,.operation=EDITOR_SOURCE_ADD}};
    strcpy(add.data.source.name,"push");EditorCommandResult r=execute(&p,&h,add);OK(r);uint32_t force_id=r.created.item;
    assert(o->force_count==1 && o->force_items[0].description.mode==OBJECT_SOURCE_PERSISTENT && !o->force_items[0].description.target.id);
    ObjectForceDescription force={.mode=OBJECT_SOURCE_INITIAL,.target=target,.magnitude=-3,
        .location={.kind=ROHR_FORCE_BEAM_POSITION,.reference=beam_ref,.anchor=ROHR_FORCE_BEAM_CENTER,.fraction=0.25f,.offset={2,4}},
        .orientation={.kind=ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION,.offset=12,
            .from={.kind=ROHR_FORCE_ORIGIN_POSITION,.reference=rigid_ref,.offset={3,4}},
            .to={.kind=ROHR_FORCE_COM_POSITION,.reference=target}}};
    EditorCommand set={.type=EDITOR_COMMAND_SOURCE,.data.source={.object=object,.id=force_id,.operation=EDITOR_SOURCE_SET,.force=force}};
    OK(execute(&p,&h,set));assert(editor_history_undo(&h));assert(!o->force_items[0].description.target.id);
    assert(editor_history_redo(&h));assert(o->force_items[0].description.target.id==node_id);
    ObjectForceDescription before=o->force_items[0].description;
    set.data.source.force.target.soft_body=999;
    assert(execute(&p,&h,set).kind==ERROR_RESULT_ERROR);
    assert(!memcmp(&before,&o->force_items[0].description,sizeof(before)));
    set.data.source.force=force;set.data.source.force.magnitude=NAN;
    assert(execute(&p,&h,set).kind==ERROR_RESULT_ERROR);
    EditorTorque *torque=editor_project_torque_add(&p,o);assert(torque);
    uint32_t torque_id=torque->id;
    assert(editor_project_torque_set(o,torque_id,(ObjectTorqueDescription){.target=target,.magnitude=0}));
    /* All location/orientation variants validate with their supported references. */
    for(int kind=ROHR_FORCE_WORLD_POSITION;kind<=ROHR_FORCE_BEAM_POSITION;++kind) {
        ObjectForceDescription d=force;d.location.kind=kind;d.location.reference=kind==ROHR_FORCE_BEAM_POSITION?beam_ref:rigid_ref;
        assert(editor_force_description_check(o,d));
    }
    for(int kind=ROHR_FORCE_WORLD_ORIENTATION;kind<=ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION;++kind) {
        ObjectForceDescription d=force;d.orientation.kind=kind;d.orientation.reference=kind==ROHR_FORCE_BEAM_ORIENTATION?beam_ref:rigid_ref;
        assert(editor_force_description_check(o,d));
    }
    char object_text[32],source_text[32];snprintf(object_text,sizeof(object_text),"%u",object);snprintf(source_text,sizeof(source_text),"%u",force_id);
    char *args[]={"rohr-cli","--project","sources.json","--object-id",object_text,"--force-id",source_text,"--property","magnitude","set","-8"};
    EditorCommand cli=parse(&p,11,args);OK(execute(&p,&h,cli));assert(o->force_items[0].description.magnitude==-8);
    char line[4096];OK(editor_command_cli_standard_write(&p,&cli,NULL,"sources.json",line,sizeof(line)));assert(strstr(line,"--force-id") && strstr(line,"-8"));
    cli.data.source.operation=EDITOR_SOURCE_GET;char *query=editor_source_query_json(&p,&cli.data.source);assert(query && strstr(query,"push"));free(query);
    cli.data.source.operation=EDITOR_SOURCE_DUPLICATE;strcpy(cli.data.source.name,"push_copy");OK(execute(&p,&h,cli));
    assert(o->force_count==2 && o->force_items[1].id!=force_id && o->force_items[1].description.target.id==node_id);
    assert(editor_project_save(&p,"sources.json"));
    malformed_document("sources.json",&p);
    EditorProject loaded;editor_project_init(&loaded);OK(editor_project_load(&loaded,"sources.json"));
    assert(loaded.objects[0].force_items[0].description.mode==OBJECT_SOURCE_INITIAL);
    assert(loaded.objects[0].force_items[0].description.orientation.to.reference.id==node_id);
    assert(loaded.objects[0].torque_items[0].description.target.id==node_id);
    editor_project_destroy(&loaded);
    ObjectDescriptionResult asset=object_load("sources.json");OK(asset);
    const ObjectDescription *root=asset.result.value->children[0];
    ObjectDescriptionResult f=object_description_name_get(root,"push");OK(f);
    assert(f.result.value->kind==OBJECT_FORCE && f.result.value->force.target.id!=0);
    ObjectDescriptionResult copy=object_copy(root);OK(copy);
    ObjectDescriptionResult copied_force=object_description_name_get(copy.result.value,"push");OK(copied_force);
    ObjectForceDescription changed=copied_force.result.value->force;changed.magnitude=22;
    OK(rohr_object_force_description_set(copied_force.result.value,changed));
    assert(f.result.value->force.magnitude==-8);
    assert(error_check(object_force_description_set(f.result.value,changed)));
    size_t entities=entity_alive_count_get();EntityResult unsupported=entity_object_add(copy.result.value);
    assert(error_check(unsupported) && unsupported.result.error==ERROR_ENGINE_AUTHORED_SOURCE_PENDING && entity_alive_count_get()==entities);
    ObjectDescriptionResult visuals=object_copy_with_options(root,ROHR_ENTITY_OBJECT_VISUALS);OK(visuals);
    EntityResult visual_instance=entity_object_add_with_options(visuals.result.value,ROHR_ENTITY_OBJECT_VISUALS);OK(visual_instance);OK(entity_delete(visual_instance.result.value));
    OK(object_destroy(visuals.result.value));OK(object_destroy(copy.result.value));OK(object_destroy(asset.result.value));
    /* Duplication keeps local IDs but owns independent storage and source identities. */
    EditorCommand duplicate_object={.type=EDITOR_COMMAND_OBJECT_DUPLICATE,.data.object_remove.object=object};
    EditorCommandResult duplicated=execute(&p,&h,duplicate_object);OK(duplicated);
    uint32_t duplicate_id=duplicated.result.object;
    assert(editor_history_undo(&h));assert(!editor_object_query_get(&p,duplicate_id));
    assert(editor_history_redo(&h));
    EditorObject *dup=editor_object_query_get(&p,duplicate_id);assert(dup);
    o=editor_object_query_get(&p,object);
    assert(dup->force_items!=o->force_items && dup->force_items[0].id!=o->force_items[0].id);
    assert(editor_sources_check(dup));dup->force_items[0].description.magnitude=77;
    assert(o->force_items[0].description.magnitude==-8);
    /* Deleting a node preserves sources, resets target and nested direction,
     * and leaves the beam-relative application point unchanged. */
    EditorCommand remove_node={.type=EDITOR_COMMAND_ITEM_REMOVE,.data.item_remove={.kind=EDITOR_ITEM_SOFT_NODE,.object=object,.parent=body_id,.item=node_id}};
    OK(execute(&p,&h,remove_node));o=editor_object_query_get(&p,object);
    assert(o->force_items[0].description.target.kind==OBJECT_SOURCE_NONE);
    assert(o->force_items[0].description.orientation.kind==ROHR_FORCE_WORLD_ORIENTATION);
    assert(o->force_items[0].description.orientation.offset==0);
    assert(o->force_items[0].description.location.reference.id==beam_id);
    assert(o->torque_items[0].description.target.kind==OBJECT_SOURCE_NONE);
    assert(editor_history_undo(&h));o=editor_object_query_get(&p,object);
    assert(o->force_items[0].description.target.id==node_id && o->force_items[0].description.orientation.to.reference.id==node_id);
    assert(editor_history_redo(&h));assert(editor_history_undo(&h));
    EditorCommand remove_beam={.type=EDITOR_COMMAND_ITEM_REMOVE,.data.item_remove={.kind=EDITOR_ITEM_SOFT_BEAM,.object=object,.parent=body_id,.item=beam_id}};
    OK(execute(&p,&h,remove_beam));o=editor_object_query_get(&p,object);
    assert(o->force_items[0].description.location.kind==ROHR_FORCE_WORLD_POSITION && o->force_items[0].description.location.offset.x==0);
    assert(o->force_items[0].description.target.id==node_id);assert(editor_history_undo(&h));
    EditorCommand remove_body={.type=EDITOR_COMMAND_ITEM_REMOVE,.data.item_remove={.kind=EDITOR_ITEM_SOFT_BODY,.object=object,.item=body_id}};
    OK(execute(&p,&h,remove_body));o=editor_object_query_get(&p,object);
    assert(!o->force_items[0].description.target.id && !o->force_items[0].description.location.reference.id);
    assert(editor_history_undo(&h));o=editor_object_query_get(&p,object);assert(editor_sources_check(o));
    EditorWorkspace workspace={.open=true};snprintf(workspace.directory,sizeof(workspace.directory),".");
    assert(!editor_workspace_c_generate(&workspace,&p));
    /* Source deletion and undo retain the full description. */
    cli.data.source.operation=EDITOR_SOURCE_REMOVE;OK(execute(&p,&h,cli));assert(o->force_count==1);
    assert(editor_history_undo(&h));assert(o->force_count==2 && o->force_items[0].description.magnitude==-8);
    editor_history_destroy(&h);editor_project_destroy(&p);assert(remove("sources.json")==0);
    engine_stop();return 0;
}
