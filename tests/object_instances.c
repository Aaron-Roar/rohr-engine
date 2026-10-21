/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "entity/entity_internal.h"
#include "entity/object_internal.h"
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
    EntityResult n = physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = ENTITY_INVALID,
        .position = (Position){2, 3},
        .mass = 2,
        .radius = 1
    });
    OK(n);
    EntityResult b = physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = ENTITY_INVALID,
        .node_a = n.result.value,
        .node_b = ENTITY_INVALID,
        .stiffness = 100,
        .damping = 2
    });
    OK(b);
    EntityResult a = physics_soft_body_area_add((SoftBodyAreaDescription){.visible = true,
        .owner = ENTITY_INVALID,
        .geometry = (SoftBodyAreaGeometry){.outer = {.nodes = {n.result.value, ENTITY_INVALID}, .node_count = 2}}
    });
    OK(a);
    Entity joint = make();
    OK(physics_joint_description_set(joint, (JointDescription){.type = JOINT_PIN, .anchor_a = JOINT_ANCHOR_INVALID, .anchor_b = JOINT_ANCHOR_INVALID}));
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
    root.visible=false;
    a=entity_object_add(root); OK(a);
    assert(!animated_sprites[index_of(member(a.result.value,"box"))].visible);
    assert(!animated_sprites[index_of(member(a.result.value,"overlay"))].visible);
    assert(entity_components_check(member(a.result.value,"box"),ROHR_MASS));
    assert(animation.visible && overlay.visible && camera.visible);
    OK(entity_delete(a.result.value)); root.visible=true;
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
    ObjectDescriptionResult immutable = object_description_by_name_get(a.result.value, "automatic_body"); OK(immutable);
    ObjectMaterialDescriptionResult material = object_material_description_get(immutable.result.value); OK(material);
    ObjectCollisionDescriptionResult collision = object_collision_description_get(immutable.result.value); OK(collision);
    ObjectColorDescriptionResult color = object_color_description_get(immutable.result.value); OK(color);
    assert(error_check(object_material_description_set(immutable.result.value, material.result.value)));
    assert(error_check(object_collision_description_set(immutable.result.value, collision.result.value)));
    assert(error_check(object_color_description_set(immutable.result.value, color.result.value)));
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
        "{\"version\":9,\"entities\":[{\"name\":\"a\",\"components\":{\"object\":[{\"entity\":\"b\",\"name\":\"b\"}]}},{\"name\":\"b\",\"components\":{\"object\":[{\"entity\":\"a\",\"name\":\"a\"}]}}]}",
        "{\"version\":9,\"entities\":[{\"name\":\"a\",\"components\":{\"object\":[{\"entity\":\"sentinel\",\"name\":\"foreign\"}]}}]}",
        "{\"version\":9,\"entities\":[{\"name\":\"a\",\"components\":{\"object\":[],\"object_aliases\":[{\"entity\":\"b\",\"name\":\"foreign\"}]}},{\"name\":\"b\",\"components\":{}}]}"
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
static void soft_creation_descriptions(void) {
    Entity container = make();
    SoftBodyDescription body_description = {.owner = container,
        .position = {10, 20}, .orientation = 90};
    Entity body = take(rohr_physics_soft_body_add(body_description));
    Entity another = take(physics_soft_body_add(body_description));
    assert(body != another && entity_owner_get(body).result.value == container);
    assert(physics_position_get(body).result.value.x == 10);
    assert(orientations[rohr_entity_index_get(body).result.value] == 90);
    SoftBodyNodeDescription node_description = {
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = body, .position = {2, 0}, .mass = 3, .radius = 2};
    Entity local = take(rohr_physics_soft_body_node_local_add(node_description));
    Entity local_copy = take(physics_soft_body_node_local_add(node_description));
    assert(local != local_copy && node_description.position.x == 2 && node_description.position.y == 0);
    Position position = physics_position_get(local).result.value;
    assert(fabsf(position.x - 10) < 0.0001f && fabsf(position.y - 18) < 0.0001f);
    Entity world = take(rohr_physics_soft_body_node_add(node_description));
    assert(physics_position_get(world).result.value.x == 2 && physics_position_get(world).result.value.y == 0);
    node_description.owner = container;
    node_description.position = (Position){4, 8};
    Entity generic = take(physics_soft_body_node_add(node_description));
    assert(entity_owner_get(generic).result.value == container);
    assert(physics_soft_body_node_get(generic).result.value.soft_body == ENTITY_INVALID);
    assert(physics_soft_body_get(body).result.value.node_count == 3);
    EntityIndex index; assert(entity_index_get(world, &index));
    assert(mass[index] == 3 && velocities[index].x == 0 && velocities[index].y == 0);
    assert(accelerations[index].x == 0 && accelerations[index].y == 0);
    assert(frictions[index] == 0 && restitutions[index] == 0.25f);
    assert(physics_soft_body_node_get(world).result.value.radius == 2);
    SoftBodyBeamDescription beam_description = {
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = container, .node_a = world, .node_b = local};
    Entity beam = take(rohr_physics_soft_body_beam_add(beam_description));
    Entity beam_copy = take(physics_soft_body_beam_add(beam_description));
    SoftBodyBeam value = physics_soft_body_beam_get(beam).result.value;
    assert(beam != beam_copy && value.node_a == world && value.node_b == local);
    assert(value.stiffness == 0 && value.damping == 0 && value.friction == 0 && value.restitution == 0.25f);
    assert(fabsf(value.rest_length - sqrtf(8 * 8 + 18 * 18)) < 0.0001f);
    assert(!value.collision_enabled && value.collision_thickness == ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN && value.soft_body == ENTITY_INVALID);
    SoftBodyAreaDescription area_description = {.visible = true, .owner = container,
        .geometry = {.outer = {.nodes = {world, generic, local}, .node_count = 3}}};
    Entity area = take(rohr_physics_soft_body_area_add(area_description));
    Entity area_copy = take(physics_soft_body_area_add(area_description));
    area_description.geometry.outer.nodes[0] = ENTITY_INVALID;
    assert(area != area_copy && physics_soft_body_area_get(area).result.value.visible);
    assert(physics_soft_body_area_get(area).result.value.geometry.outer.nodes[0] == world);
    assert(entity_owner_get(area).result.value == container);
    assert(physics_soft_body_area_get(area).result.value.soft_body == ENTITY_INVALID);
    size_t count = entity_alive_count_get();
    assert(error_check(physics_soft_body_node_local_add(node_description))); /* generic owner has no frame */
    assert(error_check(rohr_physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .mass = 1})));
    assert(error_check(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .radius = 1})));
    assert(error_check(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .mass = NAN, .radius = 1})));
    assert(error_check(physics_soft_body_node_local_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = body, .position = {INFINITY, 0}, .mass = 1, .radius = 1})));
    assert(error_check(physics_soft_body_add((SoftBodyDescription){.position = {INFINITY, 0}})));
    assert(error_check(rohr_physics_soft_body_add((SoftBodyDescription){.orientation = NAN})));
    assert(error_check(physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .stiffness = -1})));
    assert(error_check(physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .damping = INFINITY})));
    assert(entity_alive_count_get() == count);
    Entity stale = make(); OK(entity_delete(stale));
    assert(error_check(physics_soft_body_add((SoftBodyDescription){.owner = stale})));
    assert(error_check(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = stale, .mass = 1, .radius = 1})));
    assert(error_check(physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = stale})));
    assert(error_check(physics_soft_body_area_add((SoftBodyAreaDescription){.visible = true, .owner = stale})));
    assert(entity_alive_count_get() == count);
    Entity detached = take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .mass = 1, .radius = 1}));
    assert(entity_owner_get(detached).result.value == ENTITY_INVALID);
    OK(entity_delete(container));
    assert(!entity_alive_check(body) && !entity_alive_check(generic) && !entity_alive_check(beam) && !entity_alive_check(area));
    assert(entity_alive_check(detached)); OK(entity_delete(detached));
}

static void soft_creation_rollback(void) {
#ifdef ROHR_TEST_ALLOCATION_WRAP
    for(unsigned kind = 0; kind < 5; ++kind) {
        unsigned failures = 0, successes = 0;
        for(long fail = 0; fail < 60; ++fail) {
            Entity owner = kind == 2 ? take(physics_soft_body_add((SoftBodyDescription){0})) : make();
            size_t before = entity_alive_count_get();
            allocation_countdown = fail;
            EntityResult created;
            switch(kind) {
            case 0: created = physics_soft_body_add((SoftBodyDescription){.owner = owner}); break;
            case 1: created = physics_soft_body_node_add((SoftBodyNodeDescription){
                .restitution = 0.25f, .collision_enabled = true,
                .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                    .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
                .owner = owner, .mass = 1, .radius = 1}); break;
            case 2: created = physics_soft_body_node_local_add((SoftBodyNodeDescription){
                .restitution = 0.25f, .collision_enabled = true,
                .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                    .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
                .owner = owner, .mass = 1, .radius = 1}); break;
            case 3: created = physics_soft_body_beam_add((SoftBodyBeamDescription){
                .restitution = 0.25f,
                .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
                    .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                        .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
                .owner = owner}); break;
            default: created = physics_soft_body_area_add((SoftBodyAreaDescription){.visible = true, .owner = owner}); break;
            }
            allocation_countdown = -1;
            if(error_check(created)) {
                ++failures;
                assert(entity_alive_count_get() == before);
                assert(!entity_components_check(owner, ROHR_OBJECT));
                if(kind == 2) assert(physics_soft_body_get(owner).result.value.node_count == 0);
            } else {
                ++successes;
                assert(entity_alive_count_get() == before + 1);
                assert(entity_owner_get(created.result.value).result.value == owner);
            }
            OK(entity_delete(owner));
            assert(entity_alive_count_get() == before - 1);
        }
        assert(failures && successes);
    }
#endif
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
    Entity n = take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = a,
        .position = (Position){0, 0},
        .mass = 2,
        .radius = 1
    }));
    Entity x = take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = a,
        .position = (Position){10, 0},
        .mass = 3,
        .radius = 1
    }));
    Entity y = take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = b,
        .position = (Position){0, 10},
        .mass = 5,
        .radius = 1
    }));
    Entity beam = take(physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = a,
        .node_a = n,
        .node_b = x,
        .stiffness = 100,
        .damping = 2
    }));
    Entity independent = take(physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = ENTITY_INVALID,
        .node_a = n,
        .node_b = y,
        .stiffness = 100,
        .damping = 2
    }));
    Entity bridge = take(physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = a,
        .node_a = x,
        .node_b = y,
        .stiffness = 100,
        .damping = 2
    }));
    SoftBodyAreaGeometry geometry = {.outer = {.nodes = {n, x, y}, .node_count = 3}};
    Entity area = take(physics_soft_body_area_add((SoftBodyAreaDescription){.visible = true, .owner = a, .geometry = geometry}));
    JointAnchorIdResult anchor_n = physics_joint_anchor_add((JointAnchorDescription){.entity = n}); OK(anchor_n);
    JointAnchorIdResult anchor_y = physics_joint_anchor_add((JointAnchorDescription){.entity = y}); OK(anchor_y);
    Entity joint = take(physics_joint_add((JointDescription){.type = JOINT_PIN,
        .anchor_a = anchor_n.result.value, .anchor_b = anchor_y.result.value}));
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
    ParticleDescription particle = physics_particle_description_default_get();
    Entity decoration = take(physics_particle_add(particle));
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
    Entity a = take(physics_soft_body_add((SoftBodyDescription){0})), b = take(physics_soft_body_add((SoftBodyDescription){0}));
    OK(entity_name_set(a, "owner_a")); OK(entity_name_set(b, "owner_b"));
    Entity n = take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = a,
        .position = (Position){0, 0},
        .mass = 1,
        .radius = 1
    }));
    Entity x = take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = b,
        .position = (Position){10, 0},
        .mass = 1,
        .radius = 1
    }));
    Entity y = take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = b,
        .position = (Position){0, 10},
        .mass = 1,
        .radius = 1
    }));
    OK(entity_name_set(n, "cross_node")); OK(entity_name_set(x, "foreign_node"));
    Entity beam = take(physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = a,
        .node_a = n,
        .node_b = x,
        .stiffness = 100,
        .damping = 2
    }));
    Entity area = take(physics_soft_body_area_add((SoftBodyAreaDescription){.visible = true,
        .owner = a,
        .geometry = (SoftBodyAreaGeometry){.outer = {.nodes = {n, x, y}, .node_count = 3}}
    }));
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
    Entity body = take(physics_soft_body_add((SoftBodyDescription){0}));
    for(unsigned i = 0; i < SOFT_BODY_MAX_NODES; ++i)
        (void)take(physics_soft_body_node_add((SoftBodyNodeDescription){
            .restitution = 0.25f, .collision_enabled = true,
            .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
            .owner = body,
            .position = (Position){(float)i, 0},
            .mass = 1,
            .radius = 1
        }));
    Entity prior = take(physics_soft_body_add((SoftBodyDescription){0}));
    Entity node = take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = prior,
        .position = (Position){0},
        .mass = 1,
        .radius = 1
    }));
    assert(error_check(entity_owner_set(node, body)));
    assert(entity_owner_get(node).result.value == prior);
    assert(physics_soft_body_get(prior).result.value.node_count == 1);
    SoftBody full = physics_soft_body_get(body).result.value;
    for(unsigned i = 0; i < SOFT_BODY_MAX_BEAMS; ++i)
        (void)take(physics_soft_body_beam_add((SoftBodyBeamDescription){
            .restitution = 0.25f,
            .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
                .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                    .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
            .owner = body,
            .node_a = full.nodes[0],
            .node_b = full.nodes[1],
            .stiffness = 0,
            .damping = 0
        }));
    Entity beam = take(physics_soft_body_beam_add((SoftBodyBeamDescription){
        .restitution = 0.25f,
        .collision = {.thickness = ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN,
            .filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE}},
        .owner = prior,
        .node_a = node,
        .node_b = ENTITY_INVALID,
        .stiffness = 0,
        .damping = 0
    }));
    assert(error_check(entity_owner_set(beam, body)));
    assert(entity_owner_get(beam).result.value == prior);
    for(unsigned i = 0; i < SOFT_BODY_MAX_AREAS; ++i)
        (void)take(physics_soft_body_area_add((SoftBodyAreaDescription){.visible = true,
            .owner = body
        }));
    Entity area = take(physics_soft_body_area_add((SoftBodyAreaDescription){.visible = true,
        .owner = prior
    }));
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
static void remaining_creation_descriptions(void) {
    Entity owner = make(), other = make();
    ParticleDescription particle = physics_particle_description_default_get();
    assert(particle.owner == ENTITY_INVALID && particle.radius == 1 && particle.mass_value == 1);
    particle.owner = owner; particle.position = (Position){3, 4};
    Entity first = take(rohr_physics_particle_add(particle));
    Entity second = take(physics_particle_add(particle));
    particle.position.x = 100;
    assert(first != second && physics_position_get(first).result.value.x == 3);
    OK(entity_owner_set(second, other));
    JointAnchorIdResult first_anchor = physics_joint_anchor_add((JointAnchorDescription){.entity = first, .local_offset = {1, 0}}); OK(first_anchor);
    JointAnchorIdResult second_anchor = physics_joint_anchor_add((JointAnchorDescription){.entity = second, .local_offset = {-1, 0}}); OK(second_anchor);
    JointDescription description = {.owner = owner, .anchor_a = first_anchor.result.value,
        .anchor_b = second_anchor.result.value, .type = JOINT_SPRING,
        .stiffness = 10, .damping = 2};
    Entity joint = take(rohr_physics_joint_add(description));
    Entity copy = take(physics_joint_add(description));
    EntityIndex ji = rohr_entity_index_get(joint).result.value;
    assert(joints[ji].rest_length == 2 && joints[ji].stiffness == 10);
    OK(entity_owner_set(joint, other));
    assert(joints[ji].a == first && joints[ji].b == second);
    JointAnchorIdResult anchor = rohr_physics_joint_anchor_add((JointAnchorDescription){
        .entity = second, .local_offset = {2, 0}}); OK(anchor);
    Entity node = take(physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = other, .mass = 1, .radius = 1}));
    SoftBodyNodeAnchorPinResult pin = rohr_physics_soft_body_node_to_anchor_pin_add(
        (SoftBodyNodeAnchorPinDescription){.owner = owner, .node = node, .anchor = anchor.result.value}); OK(pin);
    assert(entity_owner_get(pin.result.value.joint).result.value == owner);
    size_t before = entity_alive_count_get();
    Entity stale = make(); OK(entity_delete(stale));
    particle.owner = stale;
    assert(error_check(physics_particle_add(particle)));
    description.owner = stale;
    assert(error_check(physics_joint_add(description)));
    assert(error_check(physics_joint_anchor_add((JointAnchorDescription){.entity = stale})));
    assert(error_check(physics_soft_body_node_to_anchor_pin_add((SoftBodyNodeAnchorPinDescription){
        .owner = stale, .node = node, .anchor = anchor.result.value})));
    assert(physics_joint_anchors_get(node).result.value.count == 1);
    assert(entity_alive_count_get() == before);
    OK(entity_delete(owner));
    assert(!entity_alive_check(first) && !entity_alive_check(copy) && !entity_alive_check(pin.result.value.joint));
    assert(entity_alive_check(second) && entity_alive_check(joint) && entity_alive_check(node));
    assert(joints[ji].a == ENTITY_INVALID && joints[ji].b == second);
    OK(physics_joint_anchor_world_position_get(pin.result.value.node_anchor));
    OK(physics_joint_anchor_remove(pin.result.value.node_anchor));
    /* Removing a referenced anchor disconnects, but retains, the helper joint. */
    pin = physics_soft_body_node_to_anchor_pin_add((SoftBodyNodeAnchorPinDescription){
        .node = node, .anchor = anchor.result.value}); OK(pin);
    OK(physics_joint_anchor_remove(pin.result.value.node_anchor));
    assert(entity_alive_check(pin.result.value.joint));
    OK(entity_delete(pin.result.value.joint));
    for(unsigned i = 0; i < MAX_JOINT_ANCHORS_PER_ENTITY; ++i)
        OK(physics_joint_anchor_add((JointAnchorDescription){.entity = node}));
    before = entity_alive_count_get();
    assert(error_check(physics_joint_anchor_add((JointAnchorDescription){.entity = node})));
    assert(error_check(physics_soft_body_node_to_anchor_pin_add((SoftBodyNodeAnchorPinDescription){
        .owner = other, .node = node, .anchor = anchor.result.value})));
    assert(physics_joint_anchors_get(node).result.value.count == MAX_JOINT_ANCHORS_PER_ENTITY);
    assert(entity_alive_count_get() == before);
    OK(entity_delete(other));
    assert(error_check(physics_joint_anchor_world_position_get(anchor.result.value)));
}

static void source_description_ownership(void) {
    Entity a = make(), b = make(), target = make();
    OK(entity_name_set(a, "source_owner_a")); OK(entity_name_set(b, "source_owner_b"));
    OK(entity_name_set(target, "source_target"));
    ForceDescription force = {.owner = a, .target = target, .magnitude = 3};
    Entity source = take(rohr_physics_force_add(force));
    OK(entity_name_set(source, "owned_source"));
    OK(physics_torque_description_set(source, (TorqueDescription){.owner = a, .target = target, .magnitude = 4}));
    OK(rohr_entity_owner_set(source, b));
    assert(physics_force_description_get(source).result.value.owner == b);
    assert(rohr_physics_torque_description_get(source).result.value.owner == b);
    OK(rohr_physics_force_magnitude_set(source, 5)); OK(physics_torque_magnitude_set(source, 6));
    assert(entity_owner_get(source).result.value == b);
    force.owner = source; force.magnitude = 99;
    assert(error_check(physics_force_description_set(source, force)));
    assert(physics_force_description_get(source).result.value.magnitude == 5);
    assert(entity_owner_get(source).result.value == b);
    Entity stale = make(); OK(entity_delete(stale));
    assert(error_check(physics_torque_description_set(source, (TorqueDescription){.owner = stale})));
    assert(physics_torque_description_get(source).result.value.magnitude == 6);
    size_t before = entity_alive_count_get();
    force.owner = a;
    assert(error_check(rohr_physics_force_apply(force)));
    assert(error_check(physics_torque_apply((TorqueDescription){.owner = a, .target = target})));
    force.owner = stale;
    assert(error_check(physics_force_add(force)));
    assert(error_check(physics_torque_add((TorqueDescription){.owner = stale})));
    assert(entity_alive_count_get() == before);
    force.owner = a; force.magnitude = 7;
    OK(rohr_physics_force_description_set(source, force));
    assert(physics_torque_description_get(source).result.value.owner == a);
    OK(physics_torque_description_set(source, (TorqueDescription){.target = target, .magnitude = 8}));
    assert(physics_force_description_get(source).result.value.owner == ENTITY_INVALID);
    OK(entity_owner_set(source, b));
    OK(game_state_file_save("creation_ownership.json"));
    OK(entity_delete(a)); OK(entity_delete(b)); OK(entity_delete(target));
    OK(game_state_file_load("creation_ownership.json"));
    source = take(entity_by_name_get("owned_source"));
    b = take(entity_by_name_get("source_owner_b"));
    a = take(entity_by_name_get("source_owner_a"));
    target = take(entity_by_name_get("source_target"));
    assert(physics_force_description_get(source).result.value.owner == b);
    assert(physics_torque_description_get(source).result.value.owner == b);
    assert(physics_force_description_get(source).result.value.target == target);
    assert(physics_torque_description_get(source).result.value.target == target);
    OK(entity_delete(b)); assert(!entity_alive_check(source) && entity_alive_check(target));
    OK(entity_delete(target)); OK(entity_delete(a));
    assert(remove("creation_ownership.json") == 0);
}

static void remaining_creation_rollback(void) {
#ifdef ROHR_TEST_ALLOCATION_WRAP
    for(unsigned kind = 0; kind < 5; ++kind) {
        unsigned failures = 0, successes = 0;
        for(long fail = 0; fail < 60; ++fail) {
            Entity owner = make(), a = make();
            Entity node = take(physics_soft_body_node_add((SoftBodyNodeDescription){
                .restitution = 0.25f, .collision_enabled = true,
                .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
                    .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
                .mass = 1, .radius = 1}));
            JointAnchorIdResult anchor = physics_joint_anchor_add((JointAnchorDescription){.entity = a}); OK(anchor);
            JointAnchorIdResult node_anchor = physics_joint_anchor_add((JointAnchorDescription){.entity = node}); OK(node_anchor);
            size_t before = entity_alive_count_get();
            ParticleDescription particle = physics_particle_description_default_get(); particle.owner = owner;
            allocation_countdown = fail;
            EntityResult created;
            switch(kind) {
            case 0: created = physics_particle_add(particle); break;
            case 1: created = physics_joint_add((JointDescription){.owner = owner, .anchor_a = anchor.result.value, .anchor_b = node_anchor.result.value, .type = JOINT_PIN}); break;
            case 2: {
                SoftBodyNodeAnchorPinResult pin = physics_soft_body_node_to_anchor_pin_add(
                    (SoftBodyNodeAnchorPinDescription){.owner = owner, .node = node, .anchor = anchor.result.value});
                created = error_check(pin) ? ERROR_RESULT_MAKE_ERROR(EntityResult, pin.result.error) :
                    ERROR_RESULT_MAKE_VALUE(EntityResult, pin.result.value.joint);
                break;
            }
            case 3: created = physics_force_add((ForceDescription){.owner = owner, .target = a}); break;
            default: created = physics_torque_add((TorqueDescription){.owner = owner, .target = a}); break;
            }
            allocation_countdown = -1;
            if(error_check(created)) {
                ++failures;
                assert(entity_alive_count_get() == before);
                assert(!entity_components_check(owner, ROHR_OBJECT));
                assert(physics_joint_anchors_get(node).result.value.count == 1);
            } else {
                ++successes;
                assert(entity_owner_get(created.result.value).result.value == owner);
                assert(entity_alive_count_get() == before + 1);
            }
            OK(entity_delete(owner)); OK(entity_delete(a)); OK(entity_delete(node));
            assert(entity_alive_count_get() == before - 3);
        }
        assert(failures && successes);
    }
    unsigned joint_failures = 0, joint_successes = 0;
    for(long fail = 0; fail < 40; ++fail) {
        Entity a = make(), b = make();
        Entity joint = take(physics_joint_add((JointDescription){.owner = a, .type = JOINT_PIN}));
        JointDescriptionResult prior = physics_joint_description_get(joint); OK(prior);
        JointDescription replacement = prior.result.value;
        replacement.owner = b; replacement.type = JOINT_SPRING; replacement.stiffness = 17;
        allocation_countdown = fail;
        EngineResult result = rohr_physics_joint_description_set(joint, replacement);
        allocation_countdown = -1;
        JointDescriptionResult after = physics_joint_description_get(joint); OK(after);
        if(error_check(result)) {
            ++joint_failures;
            assert(after.result.value.owner == a && after.result.value.type == JOINT_PIN && after.result.value.stiffness == 0);
            assert(!entity_components_check(b, ROHR_OBJECT));
        } else {
            ++joint_successes;
            assert(after.result.value.owner == b && after.result.value.type == JOINT_SPRING && after.result.value.stiffness == 17);
        }
        OK(entity_delete(a)); OK(entity_delete(b));
        assert(!entity_alive_check(joint));
    }
    assert(joint_failures && joint_successes);
    for(unsigned torque = 0; torque < 2; ++torque) {
        unsigned failures = 0, successes = 0;
        for(long fail = 0; fail < 20; ++fail) {
            Entity a = make(), b = make();
            Entity source = take(physics_force_add((ForceDescription){.owner = a, .magnitude = 2}));
            OK(physics_torque_description_set(source, (TorqueDescription){.owner = a, .magnitude = 3}));
            allocation_countdown = fail;
            EngineResult result = torque ? physics_torque_description_set(source, (TorqueDescription){.owner = b, .magnitude = 9}) :
                physics_force_description_set(source, (ForceDescription){.owner = b, .magnitude = 9});
            allocation_countdown = -1;
            if(error_check(result)) {
                ++failures;
                assert(entity_owner_get(source).result.value == a);
                assert(physics_force_description_get(source).result.value.magnitude == 2);
                assert(physics_torque_description_get(source).result.value.magnitude == 3);
                assert(!entity_components_check(b, ROHR_OBJECT));
            } else {
                ++successes;
                assert(physics_force_description_get(source).result.value.owner == b);
                assert(physics_torque_description_get(source).result.value.owner == b);
                assert(physics_force_description_get(source).result.value.magnitude == (torque ? 2 : 9));
                assert(physics_torque_description_get(source).result.value.magnitude == (torque ? 9 : 3));
            }
            OK(entity_delete(a)); OK(entity_delete(b));
        }
        assert(failures && successes);
    }
#endif
}

static void metadata_descriptions(void) {
    uint32_t before = entity_alive_count_get();
    Entity owner = make(), next = make(), parent = make();
    EntityDescription description = {.name = "metadata member", .owner = owner};
    EntityResult created = rohr_entity_description_add(description); OK(created);
    Entity entity = created.result.value;
    description.name[0] = 'X';
    EntityDescriptionResult saved = rohr_entity_description_get(entity); OK(saved);
    assert(!strcmp(saved.result.value.name, "metadata member") && saved.result.value.owner == owner);
    assert(error_check(entity_description_add(saved.result.value)));
    OK(entity_parent_set(entity, parent));
    OK(physics_position_set(entity, (Position){12, 34}));
    EntityDescription replacement = {.name = "metadata renamed", .owner = next};
    OK(rohr_entity_description_set(entity, replacement));
    assert(!strcmp(saved.result.value.name, "metadata member"));
    assert(entity_owner_get(entity).result.value == next);
    assert(entity_parent_get(entity).result.value == parent);
    assert(physics_position_get(entity).result.value.x == 12);
    assert(!strcmp(entity_object_member_name_get(entity).result.value.value, "metadata member"));
    assert(error_check(entity_object_member_by_name_get(owner, "metadata member")));
    assert(entity_object_member_by_name_get(next, "metadata member").result.value == entity);
    EntityDescription invalid = replacement;
    memcpy(invalid.name, entity_name_get(owner).result.value.value, sizeof(invalid.name));
    invalid.owner = owner;
    assert(error_check(entity_description_set(entity, invalid)));
    invalid = replacement; invalid.name[0] = '\0';
    assert(error_check(entity_description_set(entity, invalid)));
    memset(invalid.name, 'x', sizeof(invalid.name));
    assert(error_check(entity_description_set(entity, invalid)));
    assert(error_check(entity_description_add(invalid)));
    invalid = replacement; invalid.owner = entity;
    assert(error_check(entity_description_set(entity, invalid)));
    invalid = (EntityDescription){.name = "cycle", .owner = entity};
    assert(error_check(entity_description_set(next, invalid)));
    Entity stale = make(); OK(entity_delete(stale));
    invalid = replacement; invalid.owner = stale;
    assert(error_check(entity_description_set(entity, invalid)));
    assert(error_check(entity_description_add(invalid)));
    assert(error_check(entity_description_get(stale)));
    assert(error_check(entity_description_set(stale, replacement)));
    EntityDescriptionResult current = entity_description_get(entity); OK(current);
    assert(!strcmp(current.result.value.name, replacement.name) && current.result.value.owner == next);
    replacement.owner = ENTITY_INVALID;
    OK(entity_description_set(entity, replacement));
    EntityResult generated = entity_description_add((EntityDescription){.owner = owner}); OK(generated);
    EntityResult generated2 = entity_description_add((EntityDescription){.owner = owner}); OK(generated2);
    assert(strcmp(entity_name_get(generated.result.value).result.value.value,
        entity_name_get(generated2.result.value).result.value.value));
    EntityDescription longest = {0}; memset(longest.name, 'a', sizeof(longest.name) - 1);
    EntityResult bounded = entity_description_add(longest); OK(bounded);
    OK(entity_delete(bounded.result.value));
    OK(entity_delete(owner)); OK(entity_delete(next)); OK(entity_delete(parent));
    assert(entity_alive_check(entity));
    OK(entity_delete(entity));
    assert(entity_alive_count_get() == before);
}

static void metadata_allocation_rollback(void) {
#ifdef ROHR_TEST_ALLOCATION_WRAP
    for(int creating = 0; creating < 2; ++creating) {
        unsigned failures = 0, successes = 0;
        for(long fail = 0; fail < 24; ++fail) {
            Entity owner = make(), next = make();
            EntityResult member = entity_description_add((EntityDescription){.name = "before", .owner = owner}); OK(member);
            uint32_t before = entity_alive_count_get();
            EntityDescription description = {.name = "after", .owner = next};
            allocation_countdown = fail;
            EntityResult added = {0};
            EngineResult changed = {0};
            if(creating) added = entity_description_add(description);
            else changed = entity_description_set(member.result.value, description);
            allocation_countdown = -1;
            if(creating ? error_check(added) : error_check(changed)) {
                ++failures;
                assert(entity_alive_count_get() == before);
                assert(error_check(entity_by_name_get("after")));
                EntityDescriptionResult old = entity_description_get(member.result.value); OK(old);
                assert(!strcmp(old.result.value.name, "before") && old.result.value.owner == owner);
                assert(!entity_components_check(next, ROHR_OBJECT));
            } else {
                ++successes;
                Entity entity = creating ? added.result.value : member.result.value;
                assert(entity_owner_get(entity).result.value == next);
                assert(!strcmp(entity_name_get(entity).result.value.value, "after"));
            }
            OK(entity_delete(owner)); OK(entity_delete(next));
        }
        assert(failures && successes);
    }
#endif
}

static void lifetime_descriptions(void) {
    Entity entity = make();
    EntityLifetimeDescriptionResult empty = rohr_entity_lifetime_description_get(entity); OK(empty);
    assert(!empty.result.value.time_enabled && !empty.result.value.tick_enabled);
    assert(empty.result.value.expiry_time == 0 && empty.result.value.expiry_tick == 0);
    for(unsigned flags = 0; flags < 4; ++flags) {
        EntityLifetimeDescription description = {.expiry_time = engine_time_get() + 100,
            .expiry_tick = engine_tick_get() + 100, .time_enabled = (flags & 1) != 0, .tick_enabled = (flags & 2) != 0};
        OK(rohr_entity_lifetime_description_set(entity, description));
        EntityLifetimeDescriptionResult got = entity_lifetime_description_get(entity); OK(got);
        assert(got.result.value.time_enabled == description.time_enabled && got.result.value.tick_enabled == description.tick_enabled);
        assert(got.result.value.expiry_time == (description.time_enabled ? description.expiry_time : 0));
        assert(got.result.value.expiry_tick == (description.tick_enabled ? description.expiry_tick : 0));
        assert(entity_components_check(entity, ROHR_LIFETIME) == (flags != 0));
        OK(entity_lifetime_description_set(entity, got.result.value));
    }
    Time invalid[] = {-1, INFINITY, NAN};
    for(size_t i = 0; i < sizeof(invalid)/sizeof(invalid[0]); ++i) {
        EntityLifetimeDescription prior = entity_lifetime_description_get(entity).result.value;
        assert(error_check(entity_lifetime_description_set(entity, (EntityLifetimeDescription){.expiry_time = invalid[i]})));
        EntityLifetimeDescription got = entity_lifetime_description_get(entity).result.value;
        assert(got.expiry_time == prior.expiry_time && got.expiry_tick == prior.expiry_tick);
        assert(got.time_enabled && got.tick_enabled);
    }
    OK(entity_lifetime_description_set(entity, (EntityLifetimeDescription){.expiry_time = 12, .expiry_tick = 23}));
    assert(!entity_components_check(entity, ROHR_LIFETIME));
    assert(engine_lifecycle_update().header.status == ROHR_REPORT_COMPLETE);
    assert(entity_alive_check(entity)); OK(entity_delete(entity));
    for(unsigned flags = 1; flags < 4; ++flags) {
        entity = make();
        OK(entity_lifetime_description_set(entity, (EntityLifetimeDescription){
            .time_enabled = (flags & 1) != 0, .tick_enabled = (flags & 2) != 0}));
        assert(entity_alive_check(entity));
        assert(engine_lifecycle_update().header.status == ROHR_REPORT_COMPLETE);
        assert(!entity_alive_check(entity));
    }
    assert(error_check(entity_lifetime_description_get(entity)));
    assert(error_check(entity_lifetime_description_set(entity, (EntityLifetimeDescription){0})));
}

static void authored_configuration_descriptions(void) {
    ObjectDescription body = {.name = "configured", .id = 1, .kind = OBJECT_BODY,
        .categories = ROHR_ENTITY_OBJECT_ALL, .mass = 1, .radius = 2,
        .friction = 0.2f, .restitution = 0.3f, .collision_enabled = true,
        .color = {1, 2, 3, 4}, .border_color = {5, 6, 7, 8}};
    ObjectDescription wrong = {.name = "container", .kind = OBJECT_CONTAINER, .categories = ROHR_ENTITY_OBJECT_ALL};
    ObjectMaterialDescription material = object_material_description_get(&body).result.value;
    assert(material.friction == 0.2f && material.restitution == 0.3f);
    ObjectCollisionDescription collision = object_collision_description_get(&body).result.value;
    assert(collision.enabled);
    ObjectColorDescription color = object_color_description_get(&body).result.value;
    assert(!color.overridden);
    assert(error_check(object_material_description_set(&body, material)));
    assert(error_check(object_collision_description_set(&body, collision)));
    assert(error_check(object_color_description_set(&body, color)));
    assert(error_check(object_material_description_get(&wrong)));
    assert(error_check(object_collision_description_get(&wrong)));
    assert(error_check(object_material_description_get(NULL)));
    assert(error_check(object_collision_description_get(NULL)));
    assert(error_check(object_color_description_get(NULL)));
    ObjectDescriptionResult copied = object_copy(body); OK(copied);
    ObjectDescriptionResult bad_kind = object_copy(wrong); OK(bad_kind);
    assert(error_check(object_material_description_set(bad_kind.result.value, material)));
    assert(error_check(object_collision_description_set(bad_kind.result.value, collision)));
    OK(object_destroy(bad_kind.result.value));
    ObjectSpawnDescription spawn = rohr_object_spawn_description_default_get();
    assert(spawn.flags == ROHR_ENTITY_OBJECT_ALL && spawn.offset.x == 0 && spawn.offset.y == 0);
    EntityResult before = entity_object_description_add(copied.result.value, spawn); OK(before);
    assert(error_check(entity_object_description_add(copied.result.value, (ObjectSpawnDescription){0})));
    material = (ObjectMaterialDescription){.friction = 0.7f, .restitution = 0.8f};
    OK(object_material_description_set(copied.result.value, material));
    ObjectMaterialDescription bad[] = {{-1,0}, {0,-1}, {0,2}, {NAN,0}, {0,INFINITY}};
    for(size_t i = 0; i < sizeof(bad)/sizeof(bad[0]); ++i) {
        assert(error_check(object_material_description_set(copied.result.value, bad[i])));
        assert(object_material_description_get(copied.result.value).result.value.friction == 0.7f);
        assert(object_material_description_get(copied.result.value).result.value.restitution == 0.8f);
    }
    collision = (ObjectCollisionDescription){.enabled = false, .filter = {.category = 4, .collides_with = 8}};
    OK(object_collision_description_set(copied.result.value, collision));
    ObjectCollisionDescription got = object_collision_description_get(copied.result.value).result.value;
    assert(!got.enabled && got.filter.category == 4 && got.filter.collides_with == 8);
    color.overridden = true;
    OK(object_color_description_set(copied.result.value, color));
    assert(object_color_description_get(copied.result.value).result.value.overridden);
    color.overridden = false;
    OK(object_color_description_set(copied.result.value, color));
    ObjectColorDescription style = object_color_description_get(copied.result.value).result.value;
    assert(!style.overridden && !memcmp(&style.color, &color.color, sizeof(Color)) &&
        !memcmp(&style.border_color, &color.border_color, sizeof(Color)));
    spawn.offset = (Position){40, 50}; spawn.flags = ROHR_ENTITY_OBJECT_PHYSICS;
    EntityResult after = entity_object_description_add(copied.result.value, spawn); OK(after);
    assert(frictions[index_of(before.result.value)] == 0.2f);
    assert(frictions[index_of(after.result.value)] == 0.7f);
    assert(physics_position_get(after.result.value).result.value.x == 40);
    assert(entity_components_check(before.result.value, ROHR_COLLISION));
    assert(!entity_components_check(after.result.value, ROHR_COLLISION));
    OK(object_material_description_set(copied.result.value, (ObjectMaterialDescription){0}));
    assert(object_material_description_get(copied.result.value).result.value.restitution == 0);
    OK(object_destroy(copied.result.value));
    OK(entity_delete(before.result.value)); OK(entity_delete(after.result.value));
}

static void particle_descriptions(void) {
    Entity owner = make(), next = make(), parent = make();
    ParticleDescription value = physics_particle_description_default_get();
    value.owner = owner; value.position = (Position){11, 12}; value.local_origin = (Position){2, 3};
    value.velocity = (Velocity){4, 5}; value.acceleration = (Acceleration){6, 7};
    value.mass_value = 8; value.radius = 3; value.rigid_vertices = 7;
    value.static_body = true; value.gravity_enabled = true;
    EntityResult created = rohr_physics_particle_add(value); OK(created);
    Entity particle = created.result.value;
    EntityIndex index = index_of(particle);
    OK(entity_parent_set(particle, parent));
    OK(physics_entity_hold(particle));
    OK(entity_lifetime_description_set(particle, (EntityLifetimeDescription){.expiry_tick = 100, .tick_enabled = true}));
    EntityName name = entity_name_get(particle).result.value;
    ParticleDescriptionResult got = rohr_physics_particle_description_get(particle); OK(got);
    assert(got.result.value.static_body && got.result.value.mass_value == 8 && got.result.value.gravity_enabled);
    assert(got.result.value.velocity.x == 4 && got.result.value.acceleration.y == 7);
    assert(got.result.value.owner == owner && got.result.value.rigid_vertices == 7);
    HitboxId id = physics_hitbox_active_id_get(particle).result.value;
    OK(physics_particle_description_set(particle, got.result.value));
    assert(physics_hitbox_active_id_get(particle).result.value == id);
    OK(physics_entity_unhold(particle));
    (void)physics_pipeline_integrate(0.1);
    assert(positions[index].x == 11 && velocities[index].x == 4);
    OK(physics_entity_hold(particle));
    value = got.result.value; value.owner = next; value.static_body = false; value.gravity_enabled = false;
    value.radius = 5; value.rigid_vertices = 9; value.local_origin = (Position){-2, 1};
    value.collision_enabled = false; value.collision_filter = (CollisionFilterDescription){4, 8};
    OK(rohr_physics_particle_description_set(particle, value));
    assert(entity_owner_get(particle).result.value == next);
    assert(entity_parent_get(particle).result.value == parent);
    assert(!strcmp(entity_name_get(particle).result.value.value, name.value));
    assert(entity_components_check(particle, ROHR_HOLD | ROHR_LIFETIME));
    assert(!entity_components_check(particle, ROHR_COLLISION));
    assert(velocities[index].x == 4 && accelerations[index].y == 7);
    assert(physics_hitbox_active_id_get(particle).result.value == id);
    Shape shape = physics_hitbox_get(particle).result.value;
    assert(shape.amount_of_vertices == 9 && fabsf(shape.vertices[0].x - 3) < 0.0001f);
    assert(fabsf(physics_center_of_mass_local_position_get(particle).result.value.x + 2) < 0.0001f);
    OK(physics_hitbox_add(particle, math_square_create(3, 4)));
    HitboxId extra_id = physics_hitbox_id_at_get(particle, 1).result.value;
    Shape extra_shape = physics_hitbox_at_get(particle, 1).result.value;
    OK(rohr_physics_particle_radius_set(particle, 6));
    OK(rohr_physics_particle_origin_set(particle, (Position){3, -4}));
    shape = physics_hitbox_get(particle).result.value;
    assert(shape.amount_of_vertices == 9 && fabsf(shape.vertices[0].x - 9) < 0.0001f);
    assert(fabsf(shape.vertices[0].y + 4) < 0.0001f);
    assert(physics_hitbox_active_id_get(particle).result.value == id);
    assert(physics_hitbox_id_at_get(particle, 1).result.value == extra_id);
    Shape untouched = physics_hitbox_at_get(particle, 1).result.value;
    assert(!memcmp(&extra_shape, &untouched, sizeof(Shape)));
    value = physics_particle_description_get(particle).result.value;
    assert(value.radius == 6 && value.local_origin.x == 3 && value.mass_value == 8);
    ParticleDescription invalid[8];
    for(size_t i = 0; i < 8; ++i) invalid[i] = value;
    invalid[0].radius = 0; invalid[1].mass_value = -1; invalid[2].velocity.x = NAN;
    invalid[3].rigid_vertices = MAX_VERTICIES + 1; invalid[4].restitution = 2;
    invalid[5].friction = INFINITY; invalid[6].owner = particle;
    invalid[7].local_origin = (Position){ROHR_WORLD_COORDINATE_MAX, 0};
    for(size_t i = 0; i < 8; ++i) {
        assert(error_check(physics_particle_description_set(particle, invalid[i])));
        ParticleDescription after = physics_particle_description_get(particle).result.value;
        assert(after.owner == next && after.radius == 6 && after.mass_value == 8 && after.local_origin.x == 3);
        Shape after_shape = physics_hitbox_get(particle).result.value;
        assert(!memcmp(&shape, &after_shape, sizeof(Shape)));
    }
    assert(error_check(physics_particle_radius_set(particle, INFINITY)));
    assert(error_check(physics_particle_origin_set(particle, (Position){ROHR_WORLD_COORDINATE_MAX, 0})));
    assert(physics_particle_origin_get(particle).result.value.x == 3);
    Entity hybrid = make(); OK(physics_hitbox_set(hybrid, math_square_create(2, 3)));
    OK(physics_particle_radius_set(hybrid, 2));
    assert(error_check(physics_particle_description_get(hybrid)));
    assert(error_check(physics_particle_description_set(hybrid, value)));
    Shape hybrid_before = physics_hitbox_get(hybrid).result.value;
    OK(physics_particle_origin_set(hybrid, (Position){1, 1}));
    Shape hybrid_after = physics_hitbox_get(hybrid).result.value;
    assert(!memcmp(&hybrid_before, &hybrid_after, sizeof(Shape)));
    EntityResult soft = physics_soft_body_add((SoftBodyDescription){0}); OK(soft);
    EntityResult node = physics_soft_body_node_add((SoftBodyNodeDescription){
        .restitution = 0.25f, .collision_enabled = true,
        .collision_filter = {.category = ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE,
            .collides_with = ROHR_COLLISION_CATEGORY_ALL & ~ROHR_COLLISION_CATEGORY_SOFT_BODY_NODE},
        .owner = soft.result.value, .radius = 1, .mass = 1}); OK(node);
    assert(error_check(physics_particle_description_get(node.result.value)));
    assert(error_check(physics_particle_description_set(node.result.value, value)));
    OK(entity_delete(soft.result.value)); OK(entity_delete(hybrid));
    /* Full replacement clears omitted values; focused changes retain the others. */
    OK(physics_particle_description_set(particle, (ParticleDescription){.radius = 2}));
    got = physics_particle_description_get(particle); OK(got);
    assert(got.result.value.owner == ENTITY_INVALID && got.result.value.mass_value == 0);
    assert(got.result.value.velocity.x == 0 && !got.result.value.collision_enabled);
    assert(got.result.value.rigid_vertices == ROHR_PARTICLE_RIGID_VERTICES_DEFAULT);
    assert(entity_components_check(particle, ROHR_KINEMATIC_DRIVEN | ROHR_HOLD));
    /* Retained static settings survive saves, including gravity and hitbox IDs. */
    value.static_body = true; value.gravity_enabled = true;
    OK(physics_particle_description_set(particle, value));
    OK(game_state_file_save("particle_descriptions.json"));
    OK(entity_delete(owner)); OK(entity_delete(next)); OK(entity_delete(parent));
    assert(!entity_alive_check(particle));
    assert(error_check(physics_particle_description_get(particle)));
    assert(error_check(physics_particle_description_set(particle, value)));
    OK(game_state_file_load("particle_descriptions.json"));
    EntityResult restored = entity_by_name_get(name.value); OK(restored);
    got = physics_particle_description_get(restored.result.value); OK(got);
    assert(got.result.value.static_body && got.result.value.gravity_enabled && got.result.value.mass_value == 8);
    assert(got.result.value.velocity.x == 4 && got.result.value.acceleration.y == 7);
    assert(got.result.value.radius == 6 && got.result.value.rigid_vertices == 9);
    assert(physics_hitbox_active_id_get(restored.result.value).result.value == id);
    Entity restored_owner = got.result.value.owner, restored_parent = entity_parent_get(restored.result.value).result.value;
    OK(physics_entity_unhold(restored.result.value));
    got.result.value.static_body = false;
    OK(physics_particle_description_set(restored.result.value, got.result.value));
    (void)physics_pipeline_integrate(0.1);
    assert(physics_position_get(restored.result.value).result.value.x > got.result.value.position.x);
    assert(orientations[index_of(restored.result.value)] == 0);
    OK(entity_delete(restored_owner)); OK(entity_delete(restored_parent));
    /* The empty first owner was saved too. */
    while(entity_alive_count_get()) OK(entity_delete(entity_alive_at_get(0).result.value));
    assert(remove("particle_descriptions.json") == 0);
}

static void particle_description_rollback(void) {
#ifdef ROHR_TEST_ALLOCATION_WRAP
    for(unsigned missing = 0; missing < 2; ++missing) {
        unsigned failures = 0, successes = 0;
        for(long fail = 0; fail < 24; ++fail) {
            Entity owner = make(), next = make();
            ParticleDescription value = physics_particle_description_default_get(); value.owner = owner;
            EntityResult created = physics_particle_add(value); OK(created);
            Entity entity = created.result.value;
            if(missing) OK(physics_hitbox_remove(entity));
            else {
                /* With existing geometry, a focused edit needs no allocation. */
                allocation_countdown = 0;
                EngineResult result = physics_particle_radius_set(entity, 1);
                allocation_countdown = -1; OK(result);
            }
            value.owner = next; value.radius = 3; value.mass_value = 4; value.position.x = 10;
            allocation_countdown = fail;
            EngineResult result = physics_particle_description_set(entity, value);
            allocation_countdown = -1;
            ParticleDescriptionResult current = physics_particle_description_get(entity); OK(current);
            if(error_check(result)) {
                ++failures;
                assert(current.result.value.owner == owner && current.result.value.radius == 1);
                assert(current.result.value.mass_value == 1 && current.result.value.position.x == 0);
                assert(!entity_components_check(next, ROHR_OBJECT));
                assert(entity_components_check(entity, ROHR_HIT_BOX) == !missing);
            } else {
                ++successes;
                assert(current.result.value.owner == next && current.result.value.radius == 3);
                assert(current.result.value.mass_value == 4 && current.result.value.position.x == 10);
                assert(entity_components_check(entity, ROHR_HIT_BOX));
            }
            OK(entity_delete(owner)); OK(entity_delete(next));
        }
        assert(failures && successes);
    }
#endif
}

static void soft_description_replacement(void) {
    EntityResult body = rohr_physics_soft_body_add(rohr_physics_soft_body_description_default_get()); OK(body);
    EntityResult other = physics_soft_body_add((SoftBodyDescription){.position = {100, 0}}); OK(other);
    SoftBodyNodeDescription nd = rohr_physics_soft_body_node_description_default_get();
    nd.owner = body.result.value; nd.position = (Position){1, 0}; nd.velocity = (Velocity){2, 0};
    EntityResult a = rohr_physics_soft_body_node_add(nd); OK(a);
    nd.position = (Position){5, 0}; EntityResult b = physics_soft_body_node_add(nd); OK(b);
    nd.position = (Position){1, 4}; EntityResult c = physics_soft_body_node_add(nd); OK(c);
    SoftBodyBeamDescription bd = rohr_physics_soft_body_beam_description_default_get();
    bd.owner = body.result.value; bd.node_a = a.result.value; bd.node_b = b.result.value;
    bd.stiffness = 10; bd.damping = 2;
    EntityResult beam = rohr_physics_soft_body_beam_add(bd); OK(beam);
    SoftBodyBeamDescriptionResult read_beam = rohr_physics_soft_body_beam_description_get(beam.result.value); OK(read_beam);
    assert(!read_beam.result.value.collision.enabled && read_beam.result.value.rest_length == 4 && read_beam.result.value.rest_length_explicit);
    OK(physics_position_set(b.result.value, (Position){7, 0}));
    bd = read_beam.result.value; bd.stiffness = 20;
    OK(rohr_physics_soft_body_beam_description_set(beam.result.value, bd));
    assert(physics_soft_body_beam_get(beam.result.value).result.value.rest_length == 4);
    bd.rest_length_explicit = false;
    OK(physics_soft_body_beam_description_set(beam.result.value, bd));
    assert(physics_soft_body_beam_description_get(beam.result.value).result.value.rest_length == 6);
    bd.rest_length = 0; bd.rest_length_explicit = true;
    bd.collision = (SoftBodyBeamCollisionDescription){.enabled = true, .thickness = 8, .filter = {.category = 8, .collides_with = 16}};
    bd.draw_color = (Color){1, 2, 3, 4}; bd.draw_color_overridden = true;
    OK(physics_soft_body_beam_description_set(beam.result.value, bd));
    OK(physics_particle_radius_set(a.result.value, .01f));
    assert(physics_soft_body_beam_collision_description_get(beam.result.value).result.value.thickness == 8);
    OK(physics_soft_body_node_collision_filter_description_set(a.result.value, (CollisionFilterDescription){0}));
    assert(physics_soft_body_beam_collision_filter_description_get(beam.result.value).result.value.category == 8);
    SoftBodyBeamDescription bad_beam = bd; bad_beam.owner = other.result.value; bad_beam.collision.thickness = NAN;
    assert(error_check(physics_soft_body_beam_description_set(beam.result.value, bad_beam)));
    assert(entity_owner_get(beam.result.value).result.value == body.result.value);
    assert(physics_soft_body_beam_get(beam.result.value).result.value.draw_color.alpha == 4);
    bad_beam = bd; bad_beam.node_b = bad_beam.node_a;
    assert(error_check(physics_soft_body_beam_description_set(beam.result.value, bad_beam)));
    bad_beam = bd; bad_beam.node_b = ENTITY_INVALID; bad_beam.rest_length_explicit = false;
    OK(physics_soft_body_beam_description_set(beam.result.value, bad_beam));
    assert(physics_soft_body_beam_get(beam.result.value).result.value.rest_length == 0);
    OK(physics_soft_body_beam_description_set(beam.result.value, bd));

    SoftBodyDescription sd = rohr_physics_soft_body_description_get(body.result.value).result.value;
    sd.position = (Position){10, 20}; sd.orientation = 90;
    OK(rohr_physics_soft_body_description_set(body.result.value, sd));
    Position p = physics_position_get(b.result.value).result.value;
    assert(fabsf(p.x - 10) < .001f && fabsf(p.y - 13) < .001f);
    Velocity v = physics_soft_body_node_description_get(b.result.value).result.value.velocity;
    assert(fabsf(v.x) < .001f && fabsf(v.y + 2) < .001f);
    SoftBody topology = physics_soft_body_get(body.result.value).result.value;
    SoftBodyDescription invalid = sd; invalid.owner = a.result.value;
    assert(error_check(physics_soft_body_description_set(body.result.value, invalid)));
    assert(physics_soft_body_get(body.result.value).result.value.node_count == topology.node_count);
    assert(physics_position_get(b.result.value).result.value.y == p.y);
    invalid = sd; invalid.position.x = INFINITY;
    assert(error_check(physics_soft_body_description_set(body.result.value, invalid)));
    invalid = sd; invalid.position.x = ROHR_WORLD_COORDINATE_MAX;
    assert(error_check(physics_soft_body_description_set(body.result.value, invalid)));
    assert(physics_position_get(b.result.value).result.value.y == p.y);
    OK(physics_soft_body_description_set(body.result.value, sd));
    assert(physics_position_get(b.result.value).result.value.y == p.y);

    SoftBodyNodeDescriptionResult read_node = rohr_physics_soft_body_node_description_get(a.result.value); OK(read_node);
    nd = read_node.result.value; nd.owner = other.result.value; nd.static_body = true;
    nd.radius = 2; nd.mass = 7; nd.velocity = (Velocity){3, 4}; nd.gravity_enabled = true;
    nd.friction = .75f; nd.restitution = .5f; nd.draw_color = (Color){5, 6, 7, 8}; nd.draw_color_overridden = true;
    OK(rohr_physics_soft_body_node_description_set(a.result.value, nd));
    assert(physics_soft_body_get(body.result.value).result.value.node_count == 2);
    assert(physics_soft_body_get(other.result.value).result.value.node_count == 1);
    assert(physics_soft_body_beam_get(beam.result.value).result.value.node_a == a.result.value);
    assert(physics_particle_radius_get(a.result.value).result.value == 2);
    assert(physics_hitbox_get(a.result.value).result.value.vertices[0].x == 1);
    assert(physics_soft_body_node_description_get(a.result.value).result.value.velocity.y == 4);
    SoftBodyNodeDescription bad_node = nd; bad_node.owner = body.result.value; bad_node.radius = -1;
    assert(error_check(physics_soft_body_node_description_set(a.result.value, bad_node)));
    assert(entity_owner_get(a.result.value).result.value == other.result.value);
    assert(physics_soft_body_node_get(a.result.value).result.value.draw_color.alpha == 8);
    nd.static_body = false; OK(physics_soft_body_node_description_set(a.result.value, nd));
    assert(physics_mass_get(a.result.value).result.value == 7);
    OK(physics_mass_set(a.result.value, 0));
    OK(entity_components_delete(a.result.value, ROHR_DYNAMIC | ROHR_STATIC | ROHR_COLLISION_FILTER));
    uint64_t focused_flags = entity_mask[index_of(a.result.value)];
    OK(physics_particle_radius_set(a.result.value, 3));
    assert(entity_mask[index_of(a.result.value)] == focused_flags);
    SoftBodyNodeDescription zero_mass = physics_soft_body_node_description_get(a.result.value).result.value;
    OK(physics_soft_body_node_description_set(a.result.value, zero_mass));
    assert(physics_mass_get(a.result.value).result.value == 0);
    OK(physics_soft_body_node_description_set(a.result.value, nd));

    SoftBodyAreaDescription ad = rohr_physics_soft_body_area_description_default_get();
    ad.owner = body.result.value;
    ad.geometry.outer = (SoftBodyAreaLoop){.nodes = {a.result.value, b.result.value, c.result.value}, .node_count = 3};
    EntityResult area = rohr_physics_soft_body_area_add(ad); OK(area);
    ad = rohr_physics_soft_body_area_description_get(area.result.value).result.value;
    ad.owner = other.result.value; ad.visible = false; ad.draw_color = (Color){9, 10, 11, 12}; ad.draw_color_overridden = true;
    OK(rohr_physics_soft_body_area_description_set(area.result.value, ad));
    assert(physics_soft_body_get(body.result.value).result.value.area_count == 0);
    assert(physics_soft_body_area_get(area.result.value).result.value.geometry.outer.nodes[0] == a.result.value);
    SoftBodyAreaDescription bad_area = ad; bad_area.geometry.outer.nodes[1] = a.result.value; bad_area.visible = true;
    assert(error_check(physics_soft_body_area_description_set(area.result.value, bad_area)));
    assert(!physics_soft_body_area_get(area.result.value).result.value.visible);

    Entity old = make(), next = make();
    OK(physics_position_set(old, (Position){0})); OK(physics_orientation_set(old, 0));
    OK(physics_position_set(next, (Position){20, 30})); OK(physics_orientation_set(next, 90));
    JointAnchorIdResult anchor = rohr_physics_joint_anchor_add((JointAnchorDescription){.entity = old}); OK(anchor);
    Entity proxy = make(); OK(object_instance_anchor_restore(proxy, old, anchor.result.value));
    JointAnchorIdResult second = physics_joint_anchor_add((JointAnchorDescription){.entity = a.result.value}); OK(second);
    EntityResult joint = physics_joint_add((JointDescription){.owner = body.result.value, .type = JOINT_SPRING,
        .anchor_a = anchor.result.value, .anchor_b = second.result.value, .rest_length = 123, .rest_length_explicit = true}); OK(joint);
    JointAnchorId occupied[MAX_JOINT_ANCHORS_PER_ENTITY];
    for(size_t i = 0; i < MAX_JOINT_ANCHORS_PER_ENTITY; ++i) {
        JointAnchorIdResult temp = physics_joint_anchor_add((JointAnchorDescription){.entity = next}); OK(temp); occupied[i] = temp.result.value;
    }
    JointAnchorDescription an = {.entity = next, .local_offset = {2, 0}};
    assert(error_check(rohr_physics_joint_anchor_description_set(anchor.result.value, an)));
    assert(physics_joint_anchor_description_get(anchor.result.value).result.value.entity == old);
    for(size_t i = 0; i < MAX_JOINT_ANCHORS_PER_ENTITY; ++i) OK(physics_joint_anchor_remove(occupied[i]));
    OK(rohr_physics_joint_anchor_description_set(anchor.result.value, an));
    assert(rohr_physics_joint_anchor_description_get(anchor.result.value).result.value.entity == next);
    assert(physics_joint_anchors_get(old).result.value.count == 0);
    p = physics_joint_anchor_world_position_get(anchor.result.value).result.value;
    assert(fabsf(p.x - 20) < .001f && fabsf(p.y - 28) < .001f);
    assert(object_instance_anchor_owner_get(proxy) == next);
    OK(entity_name_set(proxy, "g5_proxy")); OK(entity_name_set(next, "g5_anchor_target"));
    OK(entity_name_set(a.result.value, "g5_node")); OK(entity_name_set(beam.result.value, "g5_beam"));
    OK(entity_name_set(area.result.value, "g5_area")); OK(entity_name_set(joint.result.value, "g5_joint"));
    nd.static_body = true; OK(physics_soft_body_node_description_set(a.result.value, nd));
    EntityResult empty_beam = physics_soft_body_beam_add((SoftBodyBeamDescription){.owner = body.result.value}); OK(empty_beam);
    OK(entity_name_set(empty_beam.result.value, "g5_empty_beam"));
    OK(game_state_file_save("soft_descriptions.json"));
    OK(entity_delete(old));
    assert(physics_joint_description_get(joint.result.value).result.value.anchor_a == anchor.result.value);
    assert(physics_joint_description_get(joint.result.value).result.value.rest_length == 123);
    assert(entity_owner_get(joint.result.value).result.value == body.result.value);
    OK(entity_delete(next));
    assert(physics_joint_description_get(joint.result.value).result.value.anchor_a == JOINT_ANCHOR_INVALID);
    assert(error_check(physics_joint_anchor_description_get(anchor.result.value)));

    Entity plain = make();
    assert(error_check(physics_soft_body_description_get(plain)));
    assert(error_check(physics_soft_body_node_description_set(plain, nd)));
    assert(error_check(physics_soft_body_beam_description_set(plain, bd)));
    assert(error_check(physics_soft_body_area_description_set(plain, ad)));
    OK(entity_delete(plain)); OK(entity_delete(body.result.value)); OK(entity_delete(other.result.value)); OK(entity_delete(proxy));
    OK(game_state_file_load("soft_descriptions.json"));
    Entity restored_node = entity_by_name_get("g5_node").result.value;
    SoftBodyNodeDescription restored = physics_soft_body_node_description_get(restored_node).result.value;
    assert(restored.static_body && restored.mass == 7 && restored.velocity.y == 4 && restored.draw_color.alpha == 8);
    SoftBodyBeamDescription restored_beam = physics_soft_body_beam_description_get(entity_by_name_get("g5_beam").result.value).result.value;
    assert(restored_beam.collision.enabled && restored_beam.collision.thickness == 8 && restored_beam.rest_length == 0);
    SoftBodyBeamDescription empty = physics_soft_body_beam_description_get(entity_by_name_get("g5_empty_beam").result.value).result.value;
    assert(!empty.collision.enabled && empty.collision.thickness == 0 && empty.node_a == ENTITY_INVALID);
    assert(!physics_soft_body_area_description_get(entity_by_name_get("g5_area").result.value).result.value.visible);
    JointAnchorId restored_anchor = entity_object_anchor_get(entity_by_name_get("g5_proxy").result.value).result.value;
    assert(physics_joint_anchor_description_get(restored_anchor).result.value.entity == entity_by_name_get("g5_anchor_target").result.value);
    assert(physics_joint_description_get(entity_by_name_get("g5_joint").result.value).result.value.rest_length == 123);
    while(entity_alive_count_get()) OK(entity_delete(entity_alive_at_get(0).result.value));
    assert(remove("soft_descriptions.json") == 0);
}

static void soft_description_rollback(void) {
#ifdef ROHR_TEST_ALLOCATION_WRAP
    for(int kind = 0; kind < 4; ++kind) {
        unsigned failures = 0, successes = 0;
        for(long fail = 0; fail < 24; ++fail) {
            Entity owner = make();
            SoftBodyNodeDescription nd = physics_soft_body_node_description_default_get();
            EntityResult a = physics_soft_body_node_add(nd); OK(a);
            nd.position.x = 5; EntityResult b = physics_soft_body_node_add(nd); OK(b);
            nd.position.y = 5; EntityResult c = physics_soft_body_node_add(nd); OK(c);
            EntityResult body = physics_soft_body_add((SoftBodyDescription){0}); OK(body);
            SoftBodyBeamDescription bd = physics_soft_body_beam_description_default_get(); bd.node_a = a.result.value; bd.node_b = b.result.value;
            EntityResult beam = physics_soft_body_beam_add(bd); OK(beam);
            SoftBodyAreaDescription ad = physics_soft_body_area_description_default_get();
            ad.geometry.outer = (SoftBodyAreaLoop){.nodes = {a.result.value, b.result.value, c.result.value}, .node_count = 3};
            EntityResult area = physics_soft_body_area_add(ad); OK(area);
            Entity selected = kind == 0 ? a.result.value : kind == 1 ? beam.result.value : kind == 2 ? area.result.value : body.result.value;
            nd = physics_soft_body_node_description_get(a.result.value).result.value; nd.owner = owner; nd.radius = 2;
            if(kind == 0) OK(physics_hitbox_remove(a.result.value));
            bd.owner = owner; bd.stiffness = 50;
            ad.owner = owner; ad.geometry.outer.nodes[1] = c.result.value; ad.geometry.outer.nodes[2] = b.result.value; ad.visible = false;
            allocation_countdown = fail;
            EngineResult result = kind == 0 ? physics_soft_body_node_description_set(selected, nd) :
                kind == 1 ? physics_soft_body_beam_description_set(selected, bd) :
                kind == 2 ? physics_soft_body_area_description_set(selected, ad) :
                physics_soft_body_description_set(selected, (SoftBodyDescription){.owner = owner, .position = {3, 4}});
            allocation_countdown = -1;
            if(error_check(result)) {
                ++failures;
                assert(entity_owner_get(selected).result.value == ENTITY_INVALID);
                assert(!entity_components_check(owner, ROHR_OBJECT));
                if(kind == 0) { assert(physics_soft_body_node_get(selected).result.value.radius == 1); assert(!entity_components_check(selected, ROHR_HIT_BOX)); }
                if(kind == 1) assert(physics_soft_body_beam_get(selected).result.value.stiffness == 0);
                if(kind == 2) { assert(physics_soft_body_area_get(selected).result.value.visible); assert(physics_soft_body_area_get(selected).result.value.geometry.outer.nodes[1] == b.result.value); }
                if(kind == 3) assert(physics_position_get(selected).result.value.x == 0);
            } else { ++successes; assert(entity_owner_get(selected).result.value == owner); }
            OK(entity_delete(area.result.value)); OK(entity_delete(beam.result.value));
            OK(entity_delete(a.result.value)); OK(entity_delete(b.result.value)); OK(entity_delete(c.result.value));
            OK(entity_delete(body.result.value)); OK(entity_delete(owner));
        }
        assert(failures && successes);
    }
#endif
}

int main(void) {
    OK(rohr_engine_start());
    soft_description_replacement(); soft_description_rollback();
    particle_descriptions(); particle_description_rollback();
    metadata_descriptions(); metadata_allocation_rollback(); lifetime_descriptions();
    authored_configuration_descriptions();
    remaining_creation_descriptions(); source_description_ownership(); remaining_creation_rollback();
    soft_creation_descriptions(); soft_creation_rollback();
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
