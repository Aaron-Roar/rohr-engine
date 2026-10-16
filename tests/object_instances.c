/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "entity/entity_internal.h"
#include "graphics/texture_assets.h"
#include "graphics/animation_assets.h"
#include "test_png.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef ROHR_TEST_ALLOCATION_WRAP
static long allocation_countdown = -1;
static bool reject(void) {
    if(allocation_countdown < 0) return false;
    if(allocation_countdown-- == 0) { allocation_countdown = -1; return true; }
    return false;
}
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t n) { return reject() ? NULL : __real_malloc(n); }
void *__wrap_calloc(size_t n, size_t s) { return reject() ? NULL : __real_calloc(n,s); }
void *__wrap_realloc(void *p, size_t n) { return reject() ? NULL : __real_realloc(p,n); }
#endif

#define OK(call) do { if(error_check(call)) { fprintf(stderr,"failed %s:%d: %s\n",__FILE__,__LINE__,#call); abort(); } } while(0)
static Entity make(void) {
    EntityResult result = entity_add();
    OK(result);
    return result.result.value;
}
static void ownership(void) {
    Entity car = make(), wheel = make(), node = make(), other = make();
    OK(entity_object_initialize(car));
    OK(entity_object_initialize(wheel));
    OK(entity_object_member_restore(car, wheel, "wheel"));
    OK(entity_object_member_restore(wheel, node, "node 45"));
    assert(error_check(entity_object_member_restore(wheel, car, "cycle")));
    assert(error_check(entity_object_member_restore(car, node, "duplicate owner")));
    OK(entity_parent_set(node, other));
    GroupIdResult generic = entity_group_create(); OK(generic);
    OK(entity_group_add(generic.result.value, node));
    GroupIdResult owned = rohr_entity_object_members_get(wheel); OK(owned);
    assert(error_check(entity_group_add(owned.result.value, other)));
    assert(error_check(entity_group_remove(owned.result.value, node)));
    assert(error_check(entity_group_destroy(owned.result.value)));
    OK(entity_name_set(node, "renamed"));
    EntityResult lookup = rohr_entity_object_member_by_name_get(car, "node 45");
    OK(lookup); assert(lookup.result.value == node);
    assert(!strcmp(entity_object_member_name_get(node).result.value.value, "node 45"));
    OK(entity_delete(car));
    assert(!entity_alive_check(wheel) && !entity_alive_check(node));
    assert(entity_alive_check(other));
    assert(!entity_group_entity_check(generic.result.value, node));
    OK(entity_delete(other));
    OK(entity_group_destroy(generic.result.value));
}
static void release_and_ordinary_parent(void) {
    Entity object = make(), member = make(), unrelated = make();
    OK(entity_object_initialize(object));
    OK(entity_object_member_restore(object, member, "member"));
    OK(entity_parent_set(unrelated, object));
    OK(rohr_entity_owner_set(member, ENTITY_INVALID));
    assert(rohr_entity_owner_get(member).result.value == ENTITY_INVALID);
    OK(entity_delete(object));
    assert(entity_alive_check(member) && entity_alive_check(unrelated));
    OK(entity_delete(member)); OK(entity_delete(unrelated));
}
static void ambiguity_and_component_removal(void) {
    Entity root = make(), a = make(), b = make();
    OK(entity_object_initialize(root));
    OK(entity_object_member_restore(root, a, "same"));
    OK(entity_object_member_restore(root, b, "same"));
    assert(error_check(entity_object_member_by_name_get(root, "same")));
    OK(entity_delete(a));
    assert(entity_object_member_by_name_get(root, "same").result.value == b);
    OK(entity_components_delete(root, ROHR_OBJECT));
    assert(entity_alive_check(root) && !entity_alive_check(b));
    assert(error_check(entity_object_members_get(root)));
    OK(entity_delete(root));
}
static void deep_cleanup(void) {
    Entity root = make(), parent = root;
    for(unsigned i = 0; i < 1000; ++i) {
        Entity child = make();
        OK(entity_object_initialize(parent));
        OK(entity_object_member_restore(parent, child, "child"));
        parent = child;
    }
    OK(entity_delete(root));
    assert(!entity_alive_check(parent));
}
static void detached_round_trip(void) {
    Entity root = make();
    OK(entity_name_set(root, "detached"));
    OK(entity_object_initialize(root));
    EntityResult n = physics_soft_body_node_create(ENTITY_INVALID, (Position){2, 3}, 2, 1);
    OK(n);
    EntityResult b = physics_soft_body_beam_create(ENTITY_INVALID, n.result.value,
        ENTITY_INVALID, 100, 2);
    OK(b);
    EntityResult a = physics_soft_body_area_create(ENTITY_INVALID,
        (SoftBodyAreaGeometry){.outer = {.nodes = {n.result.value, ENTITY_INVALID}, .node_count = 2}});
    OK(a);
    Entity joint = make();
    OK(physics_joint_pin_set(joint, JOINT_ANCHOR_INVALID, JOINT_ANCHOR_INVALID));
    OK(entity_object_member_restore(root, n.result.value, "node"));
    OK(entity_object_member_restore(root, b.result.value, "beam"));
    OK(entity_object_member_restore(root, a.result.value, "area"));
    OK(entity_object_member_restore(root, joint, "joint"));
    OK(game_state_file_save("object_instances_round_trip.json"));
    OK(entity_delete(root));
    OK(game_state_file_load("object_instances_round_trip.json"));
    EntityResult restored = entity_by_name_get("detached"); OK(restored);
    root = restored.result.value;
    n = entity_object_member_by_name_get(root, "node"); OK(n);
    b = entity_object_member_by_name_get(root, "beam"); OK(b);
    a = entity_object_member_by_name_get(root, "area"); OK(a);
    assert(physics_soft_body_node_get(n.result.value).result.value.soft_body == ENTITY_INVALID);
    assert(physics_soft_body_beam_get(b.result.value).result.value.node_a == n.result.value);
    OK(entity_delete(n.result.value));
    assert(entity_alive_check(b.result.value) && entity_alive_check(a.result.value));
    assert(physics_soft_body_beam_get(b.result.value).result.value.node_a == ENTITY_INVALID);
    assert(physics_soft_body_area_get(a.result.value).result.value.geometry.outer.nodes[0] == ENTITY_INVALID);
    OK(entity_delete(root));
    assert(remove("object_instances_round_trip.json") == 0);
}
static void description_spawning(void) {
    ObjectHitbox box_shape = {.id = 1, .shape = (Shape){.amount_of_vertices = 4, .vertices = {{-2,-3},{2,-3},{2,3},{-2,3}}}};
    ObjectDescription car = {.id = 1, .name = "car", .kind = OBJECT_CONTAINER,
        .categories = ROHR_ENTITY_OBJECT_ALL, .position = {100, 0}};
    ObjectDescription box = {.id = 2, .name = "box", .kind = OBJECT_BODY,
        .categories = ROHR_ENTITY_OBJECT_ALL, .parent = &car, .position = {20, 5},
        .mass = 2, .geometry = &box_shape, .geometry_count = 1,
        .velocity = {3, 4}, .restitution = 0.25f, .visible = true};
    const ObjectDescription *members[] = {&box};
    car.children = members; car.child_count = 1;
    ObjectDescriptionResult copy = object_copy(car); OK(copy);
    ObjectDescriptionResult copied_box = object_description_by_name_get(copy.result.value, "box"); OK(copied_box);
    OK(object_mass_set(copied_box.result.value, 7));
    assert(box.mass == 2);
    EntityResult direct = entity_object_add(box); OK(direct);
    EntityResult instance = entity_object_add(copy.result.value); OK(instance);
    EntityResult child = entity_object_member_by_name_get(instance.result.value, "box"); OK(child);
    assert(child.result.value != direct.result.value);
    assert(physics_mass_get(child.result.value).result.value == 7);
    assert(physics_position_get(direct.result.value).result.value.x == 120);
    assert(physics_position_get(child.result.value).result.value.x == 120);
    EntityResult another = entity_object_by_name_add(car, "box"); OK(another);
    assert(!strcmp(entity_name_get(direct.result.value).result.value.value, "box"));
    assert(!strcmp(entity_name_get(another.result.value).result.value.value, "box_1"));
    assert(error_check(object_mass_set(&box, 9)));
    assert(error_check(object_copy_with_options(car, 0)));
    OK(object_destroy(copy.result.value));
    assert(physics_mass_get(child.result.value).result.value == 7);
    OK(entity_delete(instance.result.value));
    assert(entity_alive_check(direct.result.value) && entity_alive_check(another.result.value));
    OK(entity_delete(direct.result.value)); OK(entity_delete(another.result.value));
}
static EntityIndex index_of(Entity entity) {
    EntityIndex index; assert(entity_index_get(entity, &index)); return index;
}
static Entity member(Entity root, const char *name) {
    EntityResult r = entity_object_member_by_name_get(root, name); OK(r); return r.result.value;
}
static void visual_resources(void) {
    assert(SDL_SaveFile("object_frame.png", test_png, sizeof(test_png)));
    OK(graphics_start());
    ObjectDescription root = {.id=1,.name="scene",.kind=OBJECT_CONTAINER,.categories=ROHR_ENTITY_OBJECT_ALL,.visible=true};
    ObjectHitbox shape = {.id=1,.shape={.amount_of_vertices=4,.vertices={{-1,-1},{1,-1},{1,1},{-1,1}}}};
    ObjectDescription box = {.id=2,.name="box",.kind=OBJECT_BODY,.parent=&root,.categories=ROHR_ENTITY_OBJECT_ALL,
        .mass=2,.position={10,20},.geometry=&shape,.geometry_count=1,.visible=true};
    ObjectFrame frames[] = {{.id=1,.path="object_frame.png",.scale={1,1}},
        {.id=2,.path="object_frame.png",.scale={-2,3},.offset={4,5},.rotation=30}};
    ObjectDescription animation = {.id=3,.name="animation",.kind=OBJECT_ANIMATION,.parent=&box,
        .target=2,.categories=ROHR_ENTITY_OBJECT_VISUALS,.frames=frames,.frame_count=2,.scale={1,1},
        .ticks_per_frame=2,.playing=true,.visible=true,.layer=7};
    ObjectDescription anchor = {.id=4,.name="anchor",.kind=OBJECT_ANCHOR,.parent=&box,
        .target=2,.categories=ROHR_ENTITY_OBJECT_PHYSICS,.position={1,2}};
    ObjectDescription camera = {.id=5,.name="camera",.kind=OBJECT_CAMERA,.parent=&root,
        .target=4,.categories=ROHR_ENTITY_OBJECT_VISUALS,.size={160,90},.zoom=2,.visible=true,.follow_position=true};
    ObjectDescription joint = {.id=6,.name="joint",.kind=OBJECT_JOINT,.parent=&root,
        .endpoint_a=4,.categories=ROHR_ENTITY_OBJECT_PHYSICS,.joint_type=JOINT_PIN};
    ObjectDescription overlay=animation; overlay.id=7; overlay.name="overlay";
    const ObjectDescription *box_children[]={&animation,&anchor,&overlay}, *children[]={&box,&camera,&joint};
    box.children=box_children; box.child_count=3; root.children=children; root.child_count=3;
    ObjectDescriptionResult copy=object_copy(root); OK(copy);
    EntityResult a=entity_object_add(copy.result.value), b=entity_object_add(copy.result.value); OK(a); OK(b);
    OK(object_destroy(copy.result.value));
    Entity box_a=member(a.result.value,"box"), box_b=member(b.result.value,"box");
    assert(member(a.result.value,"animation")==box_a);
    assert(member(a.result.value,"overlay")!=box_a);
    OK(physics_velocity_set(box_a,(Velocity){12,0}));
    assert(rohr_system_physics_update(1.0/60.0).header.status!=ROHR_REPORT_FAILED);
    Entity proxy=member(a.result.value,"anchor");
    Position proxy_position=physics_position_get(proxy).result.value;
    Position body_position=physics_position_get(box_a).result.value;
    assert(fabsf(proxy_position.x-body_position.x-1)<0.001f);
    assert(fabsf(proxy_position.y-body_position.y-2)<0.001f);
    OK(graphics_animated_sprite_frame_index_set(box_a,1));
    assert(animated_sprites[index_of(box_b)].player.frame_index==0);
    OK(entity_name_set(box_a,"renamed_box"));
    for(unsigned i=0;i<3;++i) {
        OK(game_state_file_save("object_resources.json"));
        OK(entity_delete(a.result.value)); OK(entity_delete(b.result.value));
        assert(graphics_animation_assets_stats_get().live_resources==0);
        assert(graphics_texture_assets_stats_get().live_resources==0);
        OK(game_state_file_load("object_resources.json"));
        a=entity_by_name_get("scene"); b=entity_by_name_get("scene_1"); OK(a); OK(b);
        box_a=member(a.result.value,"box"); box_b=member(b.result.value,"box");
        assert(member(a.result.value,"animation")==box_a);
        assert(animated_sprites[index_of(box_a)].player.frame_index==1);
        assert(animated_sprites[index_of(box_b)].player.frame_index==0);
        assert(animated_sprites[index_of(member(a.result.value,"overlay"))].player.frame_index==0);
        assert(graphics_layer_animation_get(box_a).result.value==7);
        OK(entity_object_camera_get(member(a.result.value,"camera")));
        JointAnchorIdResult owned=entity_object_anchor_get(member(a.result.value,"anchor")); OK(owned);
        Joint j=joints[index_of(member(a.result.value,"joint"))];
        assert(j.anchor_a==owned.result.value && j.b==ENTITY_INVALID);
    }
    OK(entity_delete(a.result.value)); OK(entity_delete(b.result.value));
    ObjectDescriptionResult physics=object_copy_with_options(root,ROHR_ENTITY_OBJECT_PHYSICS); OK(physics);
    assert(error_check(object_description_by_name_get(physics.result.value,"animation")));
    a=entity_object_add(physics.result.value); OK(a);
    assert(!entity_components_check(member(a.result.value,"box"),ROHR_ANIMATED_SPRITE));
    OK(object_destroy(physics.result.value)); OK(entity_delete(a.result.value));
    ObjectDescriptionResult visual=object_copy_with_options(root,ROHR_ENTITY_OBJECT_VISUALS); OK(visual);
    a=entity_object_add(visual.result.value); OK(a);
    assert(!entity_components_check(member(a.result.value,"box"),ROHR_COLLISION | ROHR_MASS));
    OK(object_destroy(visual.result.value)); OK(entity_delete(a.result.value));
    frames[0].path="missing_object_frame.png";
    assert(error_check(entity_object_add(root)));
    assert(error_check(entity_by_name_get("scene")));
    assert(graphics_animation_assets_stats_get().live_resources==0);
    assert(graphics_texture_assets_stats_get().live_resources==0);
    ObjectDescriptionResult player=object_load(ROHR_TEST_SOURCE_DIR "/examples/player_controller/objects/project.rohr.json");
    OK(player);
    EntityResult player_instance=entity_object_add(player.result.value); OK(player_instance);
    OK(object_destroy(player.result.value)); OK(entity_delete(player_instance.result.value));
    assert(graphics_animation_assets_stats_get().live_resources==0);
    assert(graphics_texture_assets_stats_get().live_resources==0);
    graphics_stop();
    assert(remove("object_frame.png")==0); assert(remove("object_resources.json")==0);
}
static void runtime_documents(void) {
    const char *path=ROHR_TEST_SOURCE_DIR "/examples/center_of_mass/objects/project.rohr.json";
    ObjectDescriptionResult a=object_load(path), b=object_load(path); OK(a); OK(b);
    assert(a.result.value==b.result.value);
    ObjectDescriptionResult copy=object_copy(a.result.value); OK(copy);
    OK(object_destroy(a.result.value)); OK(object_destroy(b.result.value));
    EntityResult instance=entity_object_add(copy.result.value); OK(instance);
    OK(object_destroy(copy.result.value));
    OK(game_state_file_save("object_document.json"));
    OK(entity_delete(instance.result.value));
    OK(game_state_file_load("object_document.json"));
    instance=entity_by_name_get("objects"); OK(instance);
    OK(entity_delete(instance.result.value));
    assert(remove("object_document.json")==0);
}
static void allocation_rollback(void) {
#ifdef ROHR_TEST_ALLOCATION_WRAP
    ObjectDescription root={.id=1,.name="failure_root",.kind=OBJECT_CONTAINER,.categories=ROHR_ENTITY_OBJECT_ALL};
    ObjectDescription child={.id=2,.name="failure_child",.kind=OBJECT_NODE,.parent=&root,
        .categories=ROHR_ENTITY_OBJECT_PHYSICS,.mass=1,.radius=1};
    const ObjectDescription *children[]={&child}; root.children=children; root.child_count=1;
    Entity sentinel=make(); OK(entity_name_set(sentinel,"sentinel"));
    unsigned failures=0;
    for(long n=0;n<32;++n) {
        allocation_countdown=n;
        ObjectDescriptionResult copy=object_copy(root);
        allocation_countdown=-1;
        if(error_check(copy)) ++failures; else OK(object_destroy(copy.result.value));
        allocation_countdown=n;
        EntityResult spawned=entity_object_add(root);
        allocation_countdown=-1;
        if(error_check(spawned)) ++failures; else OK(entity_delete(spawned.result.value));
        assert(error_check(entity_by_name_get("failure_root")));
        assert(entity_alive_check(sentinel));
        assert(entity_alive_count_get()==1);
    }
    assert(failures>5); OK(entity_delete(sentinel));
#endif
}
static void names_and_release(void) {
    char long_name[ENTITY_NAME_MAX]; memset(long_name,'x',sizeof(long_name)-1); long_name[sizeof(long_name)-1]=0;
    ObjectDescription root={.id=1,.name=long_name,.kind=OBJECT_SOFT_BODY,.categories=ROHR_ENTITY_OBJECT_ALL};
    ObjectDescription node={.id=2,.name="node_45",.kind=OBJECT_NODE,.parent=&root,
        .categories=ROHR_ENTITY_OBJECT_ALL,.mass=2,.radius=1};
    const ObjectDescription *children[]={&node}; root.children=children; root.child_count=1;
    Entity occupied=make(); OK(entity_name_set(occupied,"node_45_1"));
    EntityResult first=entity_object_add(node), second=entity_object_add(node); OK(first); OK(second);
    assert(!strcmp(entity_name_get(first.result.value).result.value.value,"node_45"));
    assert(!strcmp(entity_name_get(second.result.value).result.value.value,"node_45_2"));
    EntityResult whole=entity_object_add(root); OK(whole);
    Entity nested=member(whole.result.value,"node_45");
    EntityNameResult saved_name=entity_name_get(nested);
    const char *name=saved_name.result.value.value;
    assert(strlen(name)<ENTITY_NAME_MAX && strstr(name,"_node_45"));
    OK(entity_owner_set(nested, ENTITY_INVALID)); OK(entity_delete(whole.result.value));
    assert(entity_alive_check(nested));
    assert(physics_soft_body_node_get(nested).result.value.soft_body==ENTITY_INVALID);
    EntityInfoResult inspection=entity_info_get(nested,ROHR_ENTITY_INFO_PHYSICS_SOFT_BODY_NODE); OK(inspection);
    OK(entity_info_release(&inspection.result.value));
    OK(entity_delete(first.result.value)); OK(entity_delete(second.result.value));
    OK(entity_delete(nested)); OK(entity_delete(occupied));
}
static void malformed_ownership(void) {
    Entity sentinel=make(); OK(entity_name_set(sentinel,"sentinel"));
    const char *documents[]={
        "{\"version\":8,\"entities\":[{\"name\":\"a\",\"components\":{\"object\":[{\"entity\":\"b\",\"name\":\"b\"}]}},{\"name\":\"b\",\"components\":{\"object\":[{\"entity\":\"a\",\"name\":\"a\"}]}}]}",
        "{\"version\":8,\"entities\":[{\"name\":\"a\",\"components\":{\"object\":[{\"entity\":\"sentinel\",\"name\":\"foreign\"}]}}]}",
        "{\"version\":8,\"entities\":[{\"name\":\"a\",\"components\":{\"object\":[],\"object_aliases\":[{\"entity\":\"b\",\"name\":\"foreign\"}]}},{\"name\":\"b\",\"components\":{}}]}"
    };
    for(size_t i=0;i<sizeof(documents)/sizeof(documents[0]);++i) {
        assert(SDL_SaveFile("object_bad_ownership.json",documents[i],strlen(documents[i])));
        assert(error_check(game_state_file_load("object_bad_ownership.json")));
        assert(entity_alive_count_get()==1 && entity_alive_check(sentinel));
        assert(entity_owner_get(sentinel).result.value==ENTITY_INVALID);
    }
    assert(remove("object_bad_ownership.json")==0); OK(entity_delete(sentinel));
}
static Entity take(EntityResult result) {
    OK(result); return result.result.value;
}
static Entity named(const char *name) {
    return take(entity_by_name_get(name));
}
static void ownership_transfers(void) {
    Entity a = make(), b = make(), nested = make(), child = make(), parent = make();
    OK(entity_owner_set(nested, a));
    OK(entity_object_member_restore(nested, child, "stable_child"));
    OK(entity_object_alias_restore(a, child, "old_alias"));
    OK(entity_object_alias_restore(nested, child, "inner_alias"));
    OK(entity_parent_set(child, parent));
    GroupIdResult group = entity_group_create(); OK(group);
    OK(entity_group_add(group.result.value, child));
    OK(entity_name_set(child, "renamed_child"));
    OK(rohr_entity_owner_set(nested, b));
    assert(rohr_entity_owner_get(nested).result.value == b);
    assert(error_check(entity_object_member_by_name_get(a, "old_alias")));
    assert(take(entity_object_member_by_name_get(b, "inner_alias")) == child);
    assert(!strcmp(entity_object_member_name_get(child).result.value.value, "stable_child"));
    assert(entity_group_entity_check(group.result.value, child));
    assert(parents[rohr_entity_index_get(child).result.value] == parent);
    assert(error_check(entity_owner_set(b, child)));
    assert(error_check(entity_owner_set(child, child)));
    OK(entity_owner_set(nested, b));
    OK(entity_delete(a));
    assert(entity_alive_check(child));
    Entity stale = a;
    assert(error_check(entity_owner_set(child, stale)));
    assert(error_check(entity_owner_set(stale, b)));
    assert(error_check(entity_owner_get(stale)));
    Entity reused = make();
    assert(entity_owner_get(reused).result.value == ENTITY_INVALID);
    OK(entity_owner_set(child, ENTITY_INVALID));
    OK(entity_owner_set(child, b));
    assert(!strcmp(entity_object_member_name_get(child).result.value.value, "stable_child"));
    OK(entity_delete(b));
    assert(!entity_alive_check(nested) && !entity_alive_check(child));
    assert(entity_alive_check(parent) && entity_alive_check(reused));
    OK(entity_delete(parent)); OK(entity_delete(reused)); OK(entity_group_destroy(group.result.value));
}
static void physics_ownership(void) {
    ObjectDescription description = {.id = 1, .name = "expandable", .kind = OBJECT_SOFT_BODY,
        .categories = ROHR_ENTITY_OBJECT_ALL};
    Entity a = take(entity_object_add(description));
    Entity b = take(entity_object_add(description));
    Entity n = take(physics_soft_body_node_create(a, (Position){0, 0}, 2, 1));
    Entity x = take(physics_soft_body_node_create(a, (Position){10, 0}, 3, 1));
    Entity y = take(physics_soft_body_node_create(b, (Position){0, 10}, 5, 1));
    Entity beam = take(physics_soft_body_beam_create(a, n, x, 100, 2));
    Entity independent = take(physics_soft_body_beam_create(ENTITY_INVALID, n, y, 100, 2));
    Entity bridge = take(physics_soft_body_beam_create(a, x, y, 100, 2));
    SoftBodyAreaGeometry geometry = {.outer = {.nodes = {n, x, y}, .node_count = 3}};
    Entity area = take(physics_soft_body_area_create(a, geometry));
    Entity joint = take(physics_joint_create(n, y, JOINT_PIN, (Vec2D){0}, (Vec2D){0}, 0, 0));
    OK(physics_velocity_set(n, (Velocity){7, 8}));
    OK(rohr_entity_owner_set(n, b));
    assert(entity_owner_get(n).result.value == b);
    assert(physics_soft_body_node_get(n).result.value.soft_body == b);
    assert(physics_soft_body_get(a).result.value.node_count == 1);
    assert(physics_soft_body_get(b).result.value.node_count == 2);
    assert(physics_mass_get(a).result.value == 3 && physics_mass_get(b).result.value == 7);
    assert(physics_position_get(n).result.value.x == 0);
    EntityIndex ni; assert(entity_index_get(n, &ni));
    assert(velocities[ni].x == 7 && velocities[ni].y == 8);
    assert(physics_soft_body_beam_get(beam).result.value.node_a == n);
    assert(physics_soft_body_area_get(area).result.value.geometry.outer.nodes[0] == n);
    EntityIndex ji; assert(entity_index_get(joint, &ji)); assert(joints[ji].a == n);
    OK(physics_position_set(a, (Position){20, 0}));
    assert(physics_position_get(x).result.value.x == 30);
    assert(physics_position_get(n).result.value.x == 0 && physics_position_get(y).result.value.y == 10);
    OK(physics_soft_body_beam_endpoints_set(bridge, n, y));
    OK(entity_owner_set(beam, b)); OK(entity_owner_set(area, b));
    assert(physics_soft_body_beam_get(beam).result.value.soft_body == b);
    assert(physics_soft_body_area_get(area).result.value.soft_body == b);
    OK(entity_owner_set(area, ENTITY_INVALID));
    assert(physics_soft_body_area_get(area).result.value.soft_body == ENTITY_INVALID);
    OK(entity_owner_set(area, b));
    /* A plain particle shares lifetime only; it does not become a node. */
    ParticleConfig particle = physics_particle_config_default_get();
    Entity decoration = take(physics_particle_create(particle));
    OK(entity_owner_set(decoration, a));
    assert(!entity_components_check(decoration, ROHR_SOFT_BODY_NODE));
    assert(physics_soft_body_get(a).result.value.node_count == 1);
    OK(entity_delete(a));
    assert(!entity_alive_check(x) && !entity_alive_check(bridge) && !entity_alive_check(decoration));
    assert(entity_alive_check(n) && entity_alive_check(y) && entity_alive_check(beam));
    assert(physics_soft_body_beam_get(beam).result.value.node_b == ENTITY_INVALID);
    assert(physics_soft_body_area_get(area).result.value.geometry.outer.nodes[1] == ENTITY_INVALID);
    assert(physics_soft_body_beam_get(independent).result.value.node_a == n);
    OK(entity_delete(b));
    assert(!entity_alive_check(n) && !entity_alive_check(y) && !entity_alive_check(beam) && !entity_alive_check(area));
    assert(entity_alive_check(independent) && entity_alive_check(joint));
    assert(physics_soft_body_beam_get(independent).result.value.node_a == ENTITY_INVALID);
    assert(physics_soft_body_beam_get(independent).result.value.node_b == ENTITY_INVALID);
    assert(joints[ji].a == ENTITY_INVALID && joints[ji].b == ENTITY_INVALID);
    assert(description.child_count == 0);
    OK(entity_delete(independent)); OK(entity_delete(joint));
}
static void ownership_round_trip(void) {
    Entity a = take(physics_soft_body_create()), b = take(physics_soft_body_create());
    OK(entity_name_set(a, "owner_a")); OK(entity_name_set(b, "owner_b"));
    Entity n = take(physics_soft_body_node_create(a, (Position){0, 0}, 1, 1));
    Entity x = take(physics_soft_body_node_create(b, (Position){10, 0}, 1, 1));
    Entity y = take(physics_soft_body_node_create(b, (Position){0, 10}, 1, 1));
    OK(entity_name_set(n, "cross_node")); OK(entity_name_set(x, "foreign_node"));
    Entity beam = take(physics_soft_body_beam_create(a, n, x, 100, 2));
    Entity area = take(physics_soft_body_area_create(a,
        (SoftBodyAreaGeometry){.outer = {.nodes = {n, x, y}, .node_count = 3}}));
    OK(entity_name_set(beam, "cross_beam")); OK(entity_name_set(area, "cross_area"));
    OK(entity_owner_set(n, b));
    Entity container = make(); OK(entity_name_set(container, "assembly"));
    OK(entity_owner_set(a, container)); OK(entity_owner_set(b, container));
    OK(game_state_file_save("ownership_round_trip.json"));
    OK(entity_delete(container));
    OK(game_state_file_load("ownership_round_trip.json"));
    a = named("owner_a"); b = named("owner_b"); n = named("cross_node");
    beam = named("cross_beam"); area = named("cross_area");
    assert(entity_owner_get(n).result.value == b && entity_owner_get(beam).result.value == a);
    assert(physics_soft_body_get(a).result.value.node_count == 0);
    assert(physics_soft_body_get(b).result.value.node_count == 3);
    assert(physics_soft_body_beam_get(beam).result.value.node_a == n);
    assert(physics_soft_body_area_get(area).result.value.geometry.outer.nodes[0] == n);
    OK(entity_delete(named("foreign_node")));
    OK(game_state_file_save("ownership_round_trip.json"));
    OK(entity_delete(named("assembly")));
    OK(game_state_file_load("ownership_round_trip.json"));
    assert(physics_soft_body_beam_get(named("cross_beam")).result.value.node_b == ENTITY_INVALID);
    assert(physics_soft_body_area_get(named("cross_area")).result.value.geometry.outer.nodes[1] == ENTITY_INVALID);
    OK(entity_delete(named("owner_a")));
    assert(entity_alive_check(named("cross_node")));
    OK(entity_delete(named("assembly")));
    assert(remove("ownership_round_trip.json") == 0);
}
static void ownership_capacity(void) {
    Entity body = take(physics_soft_body_create());
    for(unsigned i = 0; i < SOFT_BODY_MAX_NODES; ++i)
        (void)take(physics_soft_body_node_create(body, (Position){(float)i, 0}, 1, 1));
    Entity prior = take(physics_soft_body_create());
    Entity node = take(physics_soft_body_node_create(prior, (Position){0}, 1, 1));
    assert(error_check(entity_owner_set(node, body)));
    assert(entity_owner_get(node).result.value == prior);
    assert(physics_soft_body_get(prior).result.value.node_count == 1);
    SoftBody full = physics_soft_body_get(body).result.value;
    for(unsigned i = 0; i < SOFT_BODY_MAX_BEAMS; ++i)
        (void)take(physics_soft_body_beam_create(body, full.nodes[0], full.nodes[1], 0, 0));
    Entity beam = take(physics_soft_body_beam_create(prior, node, ENTITY_INVALID, 0, 0));
    assert(error_check(entity_owner_set(beam, body)));
    assert(entity_owner_get(beam).result.value == prior);
    for(unsigned i = 0; i < SOFT_BODY_MAX_AREAS; ++i)
        (void)take(physics_soft_body_area_create(body, (SoftBodyAreaGeometry){0}));
    Entity area = take(physics_soft_body_area_create(prior, (SoftBodyAreaGeometry){0}));
    assert(error_check(entity_owner_set(area, body)));
    assert(entity_owner_get(area).result.value == prior);
    OK(entity_owner_set(full.nodes[0], body));
    OK(entity_delete(body)); OK(entity_delete(prior));
}
static void ownership_allocation_failure(void) {
#ifdef ROHR_TEST_ALLOCATION_WRAP
    unsigned failures = 0;
    for(unsigned transferring = 0; transferring < 2; ++transferring)
    for(long fail = 0; fail < 20; ++fail) {
        Entity old = make(), next = make(), child = make();
        if(transferring) OK(entity_owner_set(child, old));
        allocation_countdown = fail;
        EngineResult result = entity_owner_set(child, next);
        allocation_countdown = -1;
        if(error_check(result)) {
            ++failures;
            assert(entity_owner_get(child).result.value == (transferring ? old : ENTITY_INVALID));
            assert(!entity_components_check(next, ROHR_OBJECT));
            assert(error_check(entity_object_members_get(next)));
            if(transferring) assert(entity_group_entity_check(entity_object_members_get(old).result.value, child));
            else assert(error_check(entity_groups_get(child)));
        } else assert(entity_owner_get(child).result.value == next);
        if(error_check(result) && !transferring) OK(entity_delete(child));
        OK(entity_delete(old)); OK(entity_delete(next));
        assert(!entity_alive_check(child));
    }
    assert(failures > 0);
#endif
}
int main(void) {
    OK(rohr_engine_start());
    ownership_transfers(); physics_ownership(); ownership_round_trip();
    ownership_capacity(); ownership_allocation_failure();
    ownership(); release_and_ordinary_parent();
    ambiguity_and_component_removal(); deep_cleanup();
    detached_round_trip();
    description_spawning();
    names_and_release();
    malformed_ownership();
    allocation_rollback(); runtime_documents(); visual_resources();
    rohr_engine_stop();
    OK(rohr_engine_start());
    ownership();
    rohr_engine_stop();
    return 0;
}
