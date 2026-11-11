/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/soft_body/soft_body_area.h"
#include "yyjson/yyjson.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OK(x) do { if(error_check(x)) { fprintf(stderr, "failed %s:%d: %s\n", __FILE__, __LINE__, #x); abort(); } } while(0)
#define BAD(x) assert(error_check(x))
#ifdef ROHR_TEST_ALLOCATION_WRAP
static int fail_after = -1;
static bool reject(void) { return fail_after >= 0 && fail_after-- == 0; }
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t n) { return reject() ? NULL : __real_malloc(n); }
void *__wrap_calloc(size_t n, size_t s) { return reject() ? NULL : __real_calloc(n,s); }
void *__wrap_realloc(void *p, size_t n) { return reject() ? NULL : __real_realloc(p,n); }
#endif
static Entity take(EntityResult r) { OK(r); return r.result.value; }
static Entity named(const char *name) { return take(entity_by_name_get(name)); }
static EntityIndex idx(Entity e) { EntityIndex i; assert(entity_index_get(e,&i)); return i; }
static void restart(void) { rohr_engine_stop(); OK(rohr_engine_start()); }
static void expect_name(Entity e, const char *name) {
    EntityNameResult r = rohr_entity_name_get(e); OK(r);
    assert(strcmp(r.result.value.value,name)==0);
}
static void write_text(const char *path, const char *text) {
    FILE *f=fopen(path,"wb"); assert(f); assert(fwrite(text,1,strlen(text),f)==strlen(text)); assert(fclose(f)==0);
}
static char *read_text(const char *path) {
    FILE *f=fopen(path,"rb"); assert(f); assert(fseek(f,0,SEEK_END)==0);
    long n=ftell(f); assert(n>=0); rewind(f); char *text=malloc((size_t)n+1); assert(text);
    assert(fread(text,1,(size_t)n,f)==(size_t)n); text[n]=0; fclose(f); return text;
}
static void naming(void) {
    Entity a=take(entity_add()), b=take(rohr_entity_add());
    expect_name(a,"entity"); expect_name(b,"entity_1");
    OK(entity_name_set(a,"entity_3")); expect_name(take(entity_add()),"entity");
    expect_name(take(entity_add()),"entity_2");
    OK(physics_force_description_set(b,(ForceDescription){0})); expect_name(b,"entity_1");
    expect_name(take(physics_force_add((ForceDescription){0})),"force");
    expect_name(take(rohr_physics_torque_add((TorqueDescription){0})),"torque");
    expect_name(take(rohr_physics_particle_add(physics_particle_description_default_get())),"particle");
    Entity body=take(rohr_physics_soft_body_add((SoftBodyDescription){0})); expect_name(body,"soft_body");
    Entity node=take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = body,
        .position = (Position){0},
        .mass = 1,
        .radius = 1
    }));
    expect_name(node,"node");
    OK(entity_name_set(body,"renamed"));
    Entity node2=take(rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = body,
        .position = (Position){2,0},
        .mass = 1,
        .radius = 1
    }));
    expect_name(node2,"node_1"); expect_name(node,"node");
    expect_name(take(physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = body,
        .node_a = node,
        .node_b = node2,
        .stiffness = 1,
        .damping = 1
    })),"beam");
    char long_name[ENTITY_NAME_MAX]; memset(long_name,'x',sizeof(long_name)-1); long_name[sizeof(long_name)-1]=0;
    OK(entity_name_set(body,long_name));
    Entity first=take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = body,
        .position = (Position){0},
        .mass = 1,
        .radius = 1
    }));
    EntityName n=entity_name_get(first).result.value;
    assert(strcmp(n.value,"node_2")==0);
    Entity other=take(physics_soft_body_add((SoftBodyDescription){0})); long_name[ENTITY_NAME_MAX-2]='y'; OK(entity_name_set(other,long_name));
    n=entity_name_get(take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = other,
        .position = (Position){0},
        .mass = 1,
        .radius = 1
    }))).result.value;
    assert(strcmp(n.value,"node_3")==0);
    restart();
}

static void fixture(void) {
    /* Source created before topology: reconnection must be a separate pass. */
    Entity source=take(physics_force_add((ForceDescription){0}));
    OK(graphics_layer_create((LayerDescription){.name="unused",.value=-17}));
    OK(graphics_layer_create((LayerDescription){.name="sources",.value=3}));
    OK(graphics_layer_entity_name_set(source,"sources"));
    OK(graphics_layer_sprite_name_set(source,"sources"));
    OK(graphics_layer_animation_name_set(source,"sources"));
    Entity body=take(physics_soft_body_add((SoftBodyDescription){0}));
    Entity nodes[8];
    const Position node_positions[8]={{0,0},{20,0},{20,20},{0,20},{5,5},{5,10},{10,10},{10,5}};
    for(int i=0;i<8;++i) nodes[i]=take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = body,
        .position = node_positions[i],
        .mass = i+1,
        .radius = 1
    }));
    Entity beam=take(physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = body,
        .node_a = nodes[0],
        .node_b = nodes[1],
        .stiffness = 17,
        .damping = 0.4f
    }));
    OK(physics_friction_set(beam,0.7f)); OK(physics_restitution_set(beam,0.6f));
    SoftBodyAreaGeometry geometry={.outer={.node_count=4},.hole_count=1,.holes={{.node_count=4}}};
    memcpy(geometry.outer.nodes,nodes,4*sizeof(Entity)); memcpy(geometry.holes[0].nodes,nodes+4,4*sizeof(Entity));
    Entity area=take(physics_soft_body_area_add((SoftBodyAreaDescription){.visible = true, .owner = body, .geometry = geometry}));
    expect_name(area,"area");
    OK(graphics_soft_body_area_color_set(area,(Color){11,22,33,44}));
    OK(graphics_soft_body_area_visibility_set(area,false));
    OK(graphics_soft_body_node_color_set(body,nodes[0],(Color){1,2,3,4}));
    OK(graphics_soft_body_beam_color_set(body,nodes[0],nodes[1],(Color){4,3,2,1}));
    OK(physics_position_set(body,(Position){100,-50})); OK(physics_orientation_set(body,35));
    Position p=physics_position_get(nodes[1]).result.value; p.x+=3; OK(physics_position_set(nodes[1],p));
    OK(physics_velocity_set(nodes[1],(Velocity){2,3})); OK(physics_acceleration_set(nodes[1],(Acceleration){4,5}));
    OK(physics_angular_velocity_maximum_set(nodes[0], 0));
    OK(physics_angular_velocity_maximum_set(nodes[1], 45));
    OK(physics_static_set(nodes[0])); OK(physics_entity_hold(nodes[2]));
    OK(physics_axis_lock_description_set(nodes[5], (AxisLockDescription){.axis = (Axis){1,0}, .point_on_axis = physics_position_get(nodes[5]).result.value}));
    OK(physics_angle_lock_set(nodes[5],-20,80));
    OK(physics_transform_lock_description_set(nodes[6], (TransformLockDescription){
        .driver = nodes[7], .local_offset = (Vec2D){5,0}, .local_angle = 0,
        .lock_position = true, .lock_orientation = false, .inherit_velocity = true}));
    JointAnchorIdResult anchor=physics_joint_anchor_add((JointAnchorDescription){.entity = nodes[0], .local_offset = (Vec2D){0.2f,0.3f}}); OK(anchor);
    OK(physics_joint_anchor_add((JointAnchorDescription){.entity = nodes[0], .local_offset = (Vec2D){4,5}})); /* unused anchor */
    SoftBodyNodeAnchorPinResult pin=physics_soft_body_node_to_anchor_pin_add((SoftBodyNodeAnchorPinDescription){.node = nodes[3], .anchor = anchor.result.value}); OK(pin);
    pin=physics_soft_body_node_to_anchor_pin_add((SoftBodyNodeAnchorPinDescription){.node = nodes[4], .anchor = anchor.result.value}); OK(pin);
    expect_name(pin.result.value.joint,"joint_1");
    for(int kind=ROHR_FORCE_WORLD_POSITION;kind<=ROHR_FORCE_BEAM_POSITION;++kind) {
        ForceLocation location={.offset={1,2},.reference={.kind=kind}};
        if(kind==ROHR_FORCE_COM_POSITION || kind==ROHR_FORCE_ORIGIN_POSITION) location.reference.entity=body;
        if(kind==ROHR_FORCE_BEAM_POSITION) { location.reference.beam.entity=beam; location.reference.beam.anchor=ROHR_FORCE_BEAM_START; location.reference.beam.fraction=0.3f; }
        for(int orientation=ROHR_FORCE_WORLD_ORIENTATION;orientation<=ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION;++orientation) {
            ForceDescription force={.target=body,.magnitude=2,.location=location,.orientation={.offset=15,.reference={.kind=orientation}}};
            if(orientation==ROHR_FORCE_BODY_ORIENTATION) force.orientation.reference.entity=body;
            if(orientation==ROHR_FORCE_BEAM_ORIENTATION) force.orientation.reference.entity=beam;
            if(orientation==ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION) {
                force.orientation.reference.between.from=location;
                force.orientation.reference.between.to=(ForceLocation){.offset={2,3}};
            }
            take(physics_force_add(force));
        }
    }
    OK(physics_force_description_set(source,(ForceDescription){.target=nodes[1],.magnitude=3}));
    take(physics_torque_add((TorqueDescription){.target=beam,.magnitude=7}));
    take(physics_torque_add((TorqueDescription){.target=body,.magnitude=-2}));
    take(physics_torque_add((TorqueDescription){0}));
    OK(physics_force_apply((ForceDescription){.target=nodes[1],.magnitude=3}));
    OK(physics_torque_apply((TorqueDescription){.target=body,.magnitude=3}));
    take(physics_soft_body_add((SoftBodyDescription){0})); /* empty owner */
}
static void same_file(const char *a,const char *b) {
    char *first=read_text(a),*second=read_text(b);
    if(strcmp(first,second)!=0) { fprintf(stderr,"snapshots differ: %s, %s\n",a,b); abort(); }
    free(first); free(second);
}
static void round_trip(void) {
    fixture();
    OK(rohr_game_state_file_save("runtime_original.json"));
    uint32_t count=entity_alive_count_get();
    for(int pass=0;pass<3;++pass) {
        restart();
        /* Force different handle generations, then load exact names. */
        Entity dummy=take(entity_add()); OK(entity_delete(dummy));
        OK(game_state_file_load("runtime_original.json")); assert(entity_alive_count_get()==count);
        OK(game_state_file_save("runtime_round_trip.json")); same_file("runtime_original.json","runtime_round_trip.json");
        assert(angular_velocity_maximums_pool.used[idx(named("node"))]);
        assert(physics_angular_velocity_maximum_get(named("node")).result.value==0);
        assert(physics_angular_velocity_maximum_get(named("node_1")).result.value==45);
        assert(!angular_velocity_maximums_pool.used[idx(named("node_2"))]);
        assert(graphics_layer_name_get("unused").result.value==-17);
        GraphicsLayerId layer=graphics_layer_name_id_get("sources").result.value;
        assert(graphics_layer_entity_id_get(named("force")).result.value==layer);
        assert(graphics_layer_sprite_id_get(named("force")).result.value==layer);
        assert(graphics_layer_animation_id_get(named("force")).result.value==layer);
        Joint first=joints[idx(named("joint"))],second=joints[idx(named("joint_1"))];
        assert(first.anchor_b==second.anchor_b);
        assert(physics_joint_anchors_get(named("node")).result.value.count==2);
        SoftBodyBeam beam=physics_soft_body_beam_get(named("beam")).result.value;
        assert(beam.rest_length==20 && beam.friction==0.7f);
        assert(entity_components_check(named("node"),ROHR_STATIC));
        assert(!entity_components_check(named("node"),ROHR_DYNAMIC));
        assert(entity_components_check(named("node_2"),ROHR_HOLD));
        EntityInfoResult info=entity_info_get(named("soft_body"),ROHR_ENTITY_INFO); OK(info);
        OK(entity_info_release(&info.result.value));
    }
    /* Compare continued simulation under exactly the same caller settings. */
    OK(physics_solver_iterations_set(7)); OK(physics_substeps_set(2));
    physics_dt_update(0.01); OK(game_state_file_save("runtime_continued.json"));
    restart(); OK(physics_solver_iterations_set(7)); OK(physics_substeps_set(2));
    OK(game_state_file_load("runtime_original.json"));
    assert(physics_solver_iterations_get()==7 && physics_substeps_get()==2);
    physics_dt_update(0.01); OK(game_state_file_save("runtime_continued_again.json"));
    same_file("runtime_continued.json","runtime_continued_again.json");
    /* Geometry can be temporarily degenerate without losing topology. */
    SoftBody body=physics_soft_body_get(named("soft_body")).result.value;
    for(uint32_t i=0;i<body.node_count;++i) OK(physics_position_set(body.nodes[i],(Position){0}));
    OK(game_state_file_save("runtime_degenerate.json")); restart();
    OK(game_state_file_load("runtime_degenerate.json"));
    OK(game_state_file_save("runtime_degenerate_again.json"));
    same_file("runtime_degenerate.json","runtime_degenerate_again.json");
    restart();
}
static void malformed(void) {
    Entity retained=take(entity_add()); OK(entity_name_set(retained,"retained"));
    OK(physics_position_set(retained,(Position){7,9}));
    char *snapshot=read_text("runtime_original.json");
    const char *bad[]={
        "{\"version\":9,\"entities\":[]}",
        "{\"version\":9,\"runtime_snapshot\":true,\"entities\":[]}",
        "{\"version\":10,\"entities\":[{\"name\":\"retained\",\"components\":{}}]}",
        "{\"version\":10,\"entities\":[{\"name\":\"a\",\"count\":2,\"components\":{}},{\"name\":\"a_1\",\"components\":{}}]}",
        "{\"version\":10,\"entities\":[{\"name\":\"new\",\"components\":{\"position\":{\"x\":1e100,\"y\":0}}}]}",
        "{\"version\":5,\"entities\":[]}",
        "{\"version\":7,\"entities\":[]}"};
    for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);++i) {
        write_text("runtime_bad.json",bad[i]); BAD(game_state_file_load("runtime_bad.json"));
        assert(entity_alive_count_get()==1 && named("retained")==retained);
    }
    /* Mutate valid saved graphs to exercise failures after creation and attachment. */
    const char *keys[]={"owner","node_a","nodes","outer","anchor_b","radius","rest_length"};
    for(size_t k=0;k<sizeof(keys)/sizeof(keys[0]);++k) {
        yyjson_doc *read=yyjson_read(snapshot,strlen(snapshot),0); assert(read);
        yyjson_mut_doc *doc=yyjson_doc_mut_copy(read,NULL); assert(doc); yyjson_doc_free(read);
        yyjson_mut_val *entities=yyjson_mut_obj_get(yyjson_mut_doc_get_root(doc),"entities");
        bool changed=false;
        for(size_t i=0;i<yyjson_mut_arr_size(entities) && !changed;++i) {
            yyjson_mut_val *c=yyjson_mut_obj_get(yyjson_mut_arr_get(entities,i),"components");
            const char *records[]={"soft_body","soft_body_node","soft_body_beam","soft_body_area","joint"};
            for(size_t r=0;r<5 && !changed;++r) {
                yyjson_mut_val *v=yyjson_mut_obj_get(c,records[r]);
                if(k == 0) {
                    if(r != 1 || !v) continue;
                    assert(yyjson_mut_obj_add_str(doc, v, "owner", "retained"));
                    changed = true;
                    continue;
                }
                if(!yyjson_mut_obj_get(v,keys[k])) continue;
                yyjson_mut_obj_remove_key(v,keys[k]);
                assert(yyjson_mut_obj_add_str(doc,v,keys[k],"missing")); changed=true;
            }
        }
        assert(changed && yyjson_mut_write_file("runtime_bad.json",doc,0,NULL,NULL)); yyjson_mut_doc_free(doc);
        BAD(game_state_file_load("runtime_bad.json"));
        assert(entity_alive_count_get()==1 && named("retained")==retained);
        assert(physics_position_get(retained).result.value.x==7);
    }
    free(snapshot); restart();
}
static void multiple_documents(void) {
    char *snapshot=read_text("runtime_original.json");
    yyjson_doc *read=yyjson_read(snapshot,strlen(snapshot),0); assert(read); free(snapshot);
    yyjson_val *root=yyjson_doc_get_root(read), *entities=yyjson_obj_get(root,"entities");
    for(int part=0;part<2;++part) {
        yyjson_mut_doc *doc=yyjson_mut_doc_new(NULL); assert(doc);
        yyjson_mut_val *out=yyjson_mut_obj(doc), *array=yyjson_mut_arr(doc);
        yyjson_mut_doc_set_root(doc,out); assert(yyjson_mut_obj_add_uint(doc,out,"version",GAME_STATE_VERSION));
        assert(yyjson_mut_obj_add_val(doc,out,"entities",array));
        assert(yyjson_mut_obj_add_val(doc,out,"graphics_layers",
            yyjson_val_mut_copy(doc,yyjson_obj_get(root,"graphics_layers"))));
        for(size_t i=(size_t)part;i<yyjson_arr_size(entities);i+=2)
            assert(yyjson_mut_arr_add_val(array,yyjson_val_mut_copy(doc,yyjson_arr_get(entities,i))));
        assert(yyjson_mut_write_file(part ? "runtime_part_b.json" : "runtime_part_a.json",doc,0,NULL,NULL));
        yyjson_mut_doc_free(doc);
    }
    yyjson_doc_free(read);
    const char *paths[]={"runtime_part_b.json","runtime_part_a.json"};
    OK(rohr_game_state_files_load(paths,2));
    assert(physics_soft_body_get(named("soft_body")).result.value.node_count==8);
    Position authored=physics_position_get(named("node_1")).result.value;
    OK(physics_position_set(named("node_1"),(Position){500,600}));
    OK(game_state_template_file_save("runtime_template.json")); restart();
    OK(game_state_file_load("runtime_template.json"));
    Position restored=physics_position_get(named("node_1")).result.value;
    assert(restored.x==authored.x && restored.y==authored.y);
    restart();
    /* A clash in a later document must prevent creation from an earlier one. */
    const char *duplicates[]={"runtime_part_a.json","runtime_part_a.json"};
    BAD(game_state_files_load(duplicates,2)); assert(entity_alive_count_get()==0);
    write_text("runtime_exact_names.json","{\"version\":10,\"entities\":["
        "{\"name\":\"entity_2\",\"components\":{}},{\"name\":\"entity_1\",\"components\":{}}]}");
    OK(game_state_file_load("runtime_exact_names.json")); named("entity_1"); named("entity_2"); restart();
}

static void lifetime(void) {
    write_text("runtime_lifetime.json","{\"version\":10,\"entities\":["
        "{\"name\":\"due\",\"components\":{\"lifetime\":{\"time\":0,\"tick\":0,\"time_enabled\":false,\"tick_enabled\":true}}},"
        "{\"name\":\"disabled\",\"components\":{\"lifetime\":{\"time\":0,\"tick\":0,\"time_enabled\":false,\"tick_enabled\":false}}},"
        "{\"name\":\"later\",\"components\":{\"lifetime\":{\"time\":12.5,\"tick\":42,\"time_enabled\":true,\"tick_enabled\":true}}}]}");
    OK(game_state_file_load("runtime_lifetime.json"));
    Entity due_time=take(entity_description_add((EntityDescription){.name="due_time"}));
    OK(entity_lifetime_description_set(due_time,(EntityLifetimeDescription){.time_enabled=true}));
    assert(entity_lifetime_description_get(named("due")).result.value.tick_enabled);
    assert(!entity_components_check(named("disabled"),ROHR_LIFETIME));
    assert(life_times[idx(named("later"))].expirey_tick==engine_tick_get()+42);
    assert(fabs(life_times[idx(named("later"))].expirey_time-engine_time_get()-12.5)<0.001);
    OK(game_state_file_save("runtime_lifetime_saved.json")); restart();
    OK(game_state_file_load("runtime_lifetime_saved.json"));
    engine_lifecycle_update(); BAD(entity_by_name_get("due_time")); BAD(entity_by_name_get("due")); named("disabled"); named("later");
    restart();
    OK(engine_time_per_tick_set(0.001)); SDL_Delay(3); engine_tick_update();
    assert(engine_tick_get()>0);
    take(physics_force_add((ForceDescription){0}));
    OK(physics_force_apply((ForceDescription){0}));
    OK(physics_torque_apply((TorqueDescription){0}));
    Entity timed=take(entity_add()); OK(entity_name_set(timed,"timed"));
    OK(entity_lifetime_description_set(timed,(EntityLifetimeDescription){
        .expiry_time=engine_time_get()+10,.expiry_tick=engine_tick_get()+50,.time_enabled=true,.tick_enabled=true}));
    OK(game_state_file_save("runtime_source_lifetime.json")); restart();
    OK(game_state_file_load("runtime_source_lifetime.json"));
    assert(life_times[idx(named("force_1"))].expirey_tick==1);
    assert(life_times[idx(named("timed"))].expirey_tick==50);
    assert(fabs(life_times[idx(named("timed"))].expirey_time-10)<0.0001);
    engine_lifecycle_update(); named("force_1"); named("torque");
    OK(engine_time_per_tick_set(0.001)); SDL_Delay(3); engine_tick_update(); engine_lifecycle_update();
    BAD(entity_by_name_get("force_1")); BAD(entity_by_name_get("torque")); named("force");
    restart();
}
static void capacity(void) {
    Entity body=take(physics_soft_body_add((SoftBodyDescription){0})); Entity nodes[SOFT_BODY_MAX_NODES];
    for(unsigned i=0;i<SOFT_BODY_MAX_NODES;++i)
        nodes[i]=take(physics_soft_body_node_add((SoftBodyNodeDescription){
            .restitution = 0.25f, .collision_enabled = true,
            .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
            .owner = body,
            .position = (Position){(float)(i%8)*10,(float)(i/8)*10},
            .mass = 1,
            .radius = 1
        }));
    for(unsigned i=0;i<SOFT_BODY_MAX_BEAMS;++i)
        take(physics_soft_body_beam_add((SoftBodyBeamDescription){
            .restitution = 0.25f,
            .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
                .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                    .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
            .owner = body,
            .node_a = nodes[i%64],
            .node_b = nodes[(i+1)%64],
            .stiffness = 1,
            .damping = 0
        }));
    SoftBodyAreaGeometry geometry={.outer={.nodes={nodes[0],nodes[1],nodes[8]},.node_count=3}};
    for(unsigned i=0;i<SOFT_BODY_MAX_AREAS;++i) take(physics_soft_body_area_add((SoftBodyAreaDescription){.visible = true,
        .owner = body,
        .geometry = geometry
    }));
    OK(game_state_file_save("runtime_capacity.json")); restart();
    OK(game_state_file_load("runtime_capacity.json"));
    SoftBody value=physics_soft_body_get(named("soft_body")).result.value;
    assert(value.node_count==SOFT_BODY_MAX_NODES && value.beam_count==SOFT_BODY_MAX_BEAMS && value.area_count==SOFT_BODY_MAX_AREAS);
    OK(game_state_file_save("runtime_capacity_again.json")); same_file("runtime_capacity.json","runtime_capacity_again.json");
    Entity owner=named("soft_body");
    OK(entity_delete(value.nodes[0]));
    value=physics_soft_body_get(owner).result.value;
    assert(value.node_count==SOFT_BODY_MAX_NODES-1 && value.area_count==SOFT_BODY_MAX_AREAS);
    assert(physics_soft_body_area_get(value.areas[0]).result.value.geometry.outer.nodes[0]==ENTITY_INVALID);
    OK(entity_delete(value.beams[0]));
    OK(entity_delete(value.areas[0]));
    Entity replacement=take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = owner,
        .position = (Position){0,0},
        .mass = 2,
        .radius = 1
    }));
    take(physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .responds_to = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = owner,
        .node_a = replacement,
        .node_b = value.nodes[0],
        .stiffness = 3,
        .damping = 0.1f
    }));
    take(physics_soft_body_area_add((SoftBodyAreaDescription){.visible = true,
        .owner = owner,
        .geometry = (SoftBodyAreaGeometry){
        .outer={.nodes={replacement,named("node_1"),named("node_8")},.node_count=3}}
    }));
    OK(game_state_file_save("runtime_changed.json")); restart();
    OK(game_state_file_load("runtime_changed.json"));
    OK(game_state_file_save("runtime_changed_again.json")); same_file("runtime_changed.json","runtime_changed_again.json");
    restart();
}
static void allocation_failures(void) {
#ifdef ROHR_TEST_ALLOCATION_WRAP
    yyjson_doc *read=yyjson_read_file("runtime_original.json",0,NULL,NULL); assert(read);
    yyjson_mut_doc *doc=yyjson_doc_mut_copy(read,NULL); yyjson_doc_free(read); assert(doc);
    yyjson_mut_val *entities=yyjson_mut_obj_get(yyjson_mut_doc_get_root(doc),"entities");
    yyjson_mut_val *components=yyjson_mut_obj_get(yyjson_mut_arr_get(entities,0),"components");
    assert(yyjson_mut_obj_add_str(doc,components,"parent","retained"));
    assert(yyjson_mut_write_file("runtime_parent.json",doc,0,NULL,NULL)); yyjson_mut_doc_free(doc);
    unsigned failures=0;
    for(int ordinal=0;ordinal<1000;++ordinal) {
        Entity retained=take(entity_add()); OK(entity_name_set(retained,"retained"));
        fail_after=ordinal; EngineResult result=game_state_file_load("runtime_parent.json"); fail_after=-1;
        if(!error_check(result)) { restart(); break; }
        ++failures; assert(entity_alive_count_get()==1 && named("retained")==retained);
        BAD(graphics_layer_name_id_get("sources")); BAD(graphics_layer_name_id_get("unused"));
        assert(!children_pool.used[idx(retained)] && !entity_components_check(retained,ROHR_CHILD));
        OK(game_state_file_load("runtime_original.json")); restart();
    }
    assert(failures>10 && failures<1000);
    fixture(); failures=0;
    for(int ordinal=0;ordinal<1000;++ordinal) {
        write_text("runtime_atomic.json","preserve destination");
        fail_after=ordinal; EngineResult result=game_state_file_save("runtime_atomic.json"); fail_after=-1;
        if(!error_check(result)) break;
        ++failures; char *text=read_text("runtime_atomic.json"); assert(strcmp(text,"preserve destination")==0); free(text);
    }
    assert(failures>5 && failures<1000); restart();
#endif
}

static void layer_transactions(void) {
    Entity sentinel=take(entity_add()); OK(entity_name_set(sentinel,"sentinel"));
    GraphicsLayerIdResult existing=graphics_layer_create((LayerDescription){.name="shared",.value=7}); OK(existing);
    OK(graphics_layer_entity_id_set(sentinel,existing.result.value));
    write_text("runtime_layers_a.json","{\"version\":10,\"graphics_layers\":["
        "{\"name\":\"new\",\"value\":-3},{\"name\":\"shared\",\"value\":7}],\"entities\":["
        "{\"name\":\"bound\",\"components\":{\"graphics_layer\":\"shared\",\"sprite_layer\":0}},"
        "{\"name\":\"unbound\",\"components\":{}}]}");
    const char *bad[]={
        "{\"version\":9,\"entities\":[]}",
        "{\"version\":9,\"runtime_snapshot\":true,\"entities\":[]}",
        "{\"version\":8,\"entities\":[]}", "{\"version\":11,\"entities\":[]}",
        "{\"version\":10,\"graphics_layers\":{},\"entities\":[]}",
        "{\"version\":10,\"graphics_layers\":[{\"name\":\"shared\",\"value\":8}],\"entities\":[]}",
        "{\"version\":10,\"graphics_layers\":[{\"name\":\"new\",\"value\":2}],\"entities\":[]}",
        "{\"version\":10,\"graphics_layers\":[{\"name\":\"overflow\",\"value\":18446744073709551615}],\"entities\":[]}",
        "{\"version\":10,\"entities\":[{\"name\":\"bad\",\"components\":{\"animation_layer\":\"missing\"}}]}",
        "{\"version\":10,\"entities\":[{\"name\":\"bad\",\"components\":{\"graphics_layer\":18446744073709551615}}]}",
        "{\"version\":10,\"entities\":[{\"name\":\"bad\",\"components\":{\"angular_velocity_maximum\":-1}}]}",
        "{\"version\":10,\"entities\":[{\"name\":\"bad\",\"components\":{\"angular_velocity_maximum\":1e100}}]}"
    };
    const char *paths[]={"runtime_layers_a.json","runtime_layers_b.json"};
    for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);++i) {
        write_text(paths[1],bad[i]);
        EngineResult loaded = game_state_files_load(paths,2); BAD(loaded);
        if(i < 2) assert(strstr(error_result_message_get(loaded.kind, loaded.result.error),
            "migration required") != NULL);
        assert(entity_alive_count_get()==1 && named("sentinel")==sentinel);
        assert(graphics_layer_entity_id_get(sentinel).result.value==existing.result.value);
        assert(graphics_layer_name_get("shared").result.value==7);
        BAD(graphics_layer_name_id_get("new")); BAD(graphics_layer_name_id_get("overflow"));
    }
    write_text(paths[1],"{\"version\":10,\"graphics_layers\":[{\"name\":\"shared\",\"value\":7}],\"entities\":[]}");
    OK(game_state_files_load(paths,2));
    assert(graphics_layer_entity_id_get(named("bound")).result.value==existing.result.value);
    BAD(graphics_layer_entity_get(named("unbound")));
    BAD(graphics_layer_sprite_id_get(named("bound")));
    assert(graphics_layer_sprite_get(named("bound")).result.value==0);
    OK(graphics_layer_set(existing.result.value,19));
    assert(graphics_layer_entity_get(named("bound")).result.value==19);
    assert(graphics_layer_sprite_get(named("bound")).result.value==0);
    OK(game_state_file_save("runtime_layers_saved.json"));
    OK(game_state_template_file_save("runtime_layers_template.json"));
    restart(); OK(game_state_file_load("runtime_layers_saved.json"));
    assert(graphics_layer_name_get("shared").result.value==19);
    OK(game_state_file_save("runtime_layers_saved_again.json"));
    same_file("runtime_layers_saved.json","runtime_layers_saved_again.json");
    restart(); OK(game_state_file_load("runtime_layers_template.json"));
    assert(graphics_layer_name_get("shared").result.value==7);
    assert(graphics_layer_name_get("new").result.value==-3);
    BAD(entity_by_name_get("sentinel"));
    restart();
    /* Registry exhaustion must roll back the last available slot as well. */
    for(size_t i=0;i<MAX_GRAPHICS_LAYERS-1;++i) {
        char name[32]; snprintf(name,sizeof(name),"occupied_%zu",i);
        OK(graphics_layer_create((LayerDescription){.name=name,.value=(int)i}));
    }
    BAD(game_state_file_load(paths[0]));
    BAD(graphics_layer_name_id_get("new")); BAD(graphics_layer_name_id_get("shared"));
    assert(graphics_layer_name_get("occupied_0").result.value==0 && entity_alive_count_get()==0);
    restart();
}

static void body_state(void) {
    const char *names[]={"explicit","inferred","held","static","inactive"};
    for(size_t i=0;i<5;++i) {
        Entity body=take(entity_add()); OK(entity_name_set(body,names[i]));
        OK(physics_position_set(body,(Position){(float)i*10,0}));
        OK(physics_velocity_set(body,(Velocity){2,0}));
        OK(physics_mass_set(body,i==1 ? 0 : 3));
        OK(physics_dynamic_set(body));
        if(i==0 || i>=3) OK(physics_kinematic_driven_set(body));
        if(i==2) OK(physics_entity_hold(body));
        if(i==3) OK(physics_static_set(body));
        if(i==4) OK(entity_components_delete(body,ROHR_DYNAMIC));
    }
    OK(game_state_file_save("runtime_body_state.json")); restart();
    OK(game_state_file_load("runtime_body_state.json"));
    OK(game_state_file_save("runtime_body_state_again.json"));
    same_file("runtime_body_state.json","runtime_body_state_again.json");
    assert(physics_kinematic_driven_check(named("explicit")));
    assert(physics_kinematic_driven_check(named("inferred")));
    OK(physics_mass_set(named("explicit"),4)); OK(physics_mass_set(named("inferred"),4));
    assert(physics_kinematic_driven_check(named("explicit")));
    assert(!physics_kinematic_driven_check(named("inferred")));
    assert(!physics_kinematic_driven_check(named("static")));
    assert(!physics_kinematic_driven_check(named("inactive")));
    physics_dt_update(.1f);
    assert(physics_position_get(named("held")).result.value.x==20);
    assert(physics_position_get(named("static")).result.value.x==30);
    assert(physics_position_get(named("inactive")).result.value.x==40);
    OK(physics_dynamic_set(named("static"))); OK(physics_dynamic_set(named("inactive")));
    assert(physics_kinematic_driven_check(named("static")));
    assert(physics_kinematic_driven_check(named("inactive")));
    OK(physics_kinematic_driven_remove(named("explicit")));
    assert(!physics_kinematic_driven_check(named("explicit")));
    restart();
}
int main(void) {
    OK(rohr_engine_start()); naming(); round_trip(); malformed(); multiple_documents(); lifetime(); capacity(); allocation_failures(); layer_transactions(); body_state();
    rohr_engine_stop(); puts("runtime persistence passed"); return 0;
}
