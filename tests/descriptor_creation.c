/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "entity/object_internal.h"
#include "graphics/texture_assets.h"
#include "test_png.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OK(call) do { if(rohr_error_check(call)) { \
    fprintf(stderr, "failed %s:%d: %s\n", __FILE__, __LINE__, #call); abort(); } } while(0)

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
void *__wrap_calloc(size_t n, size_t s) { return reject() ? NULL : __real_calloc(n, s); }
void *__wrap_realloc(void *p, size_t n) { return reject() ? NULL : __real_realloc(p, n); }
#endif

/* The same initial-member layout used by generated named catalog views.
 * All actual fixture data is const, including the nested member template. */
typedef struct WaterView {
    union { ObjectDescription description; struct { ROHR_OBJECT_DESCRIPTION_FIELDS; }; };
    EntityCreateDescription particle;
} WaterView;
typedef struct Project { WaterView water; } Project;
static const Project project;
static const ObjectDescription *const water_members[] = {&project.water.particle};
static const Project project = {.water = {
    .description = {.id = 1, .name = "water", .kind = OBJECT_CONTAINER,
        .categories = ROHR_ENTITY_OBJECT_ALL, .children = water_members, .child_count = 1,
        .position = {10, 20}, .visible = false},
    .particle = {.id = 2, .name = "particle", .kind = OBJECT_PARTICLE,
        .parent = &project.water.description, .categories = ROHR_ENTITY_OBJECT_ALL,
        .position = {2, 3}, .mass = 2, .radius = 1, .particle_vertices = 8, .visible = true}
}};

static Entity value(EntityResult result) { OK(result); return result.result.value; }
static Entity member(Entity root, const char *name) { return value(rohr_object_member_get(root, name)); }
static void clean(Entity entity) { OK(rohr_entity_delete(entity)); }

static void instances(void) {
    uint32_t initial = rohr_entity_alive_count_get();
    Entity a = value(rohr_object_create(&project.water));
    Entity b = value(object_create(&project.water));
    Entity pa = member(a, "particle"), pb = member(b, "particle");
    assert(a != b && pa != pb);
    assert(rohr_entity_alive_count_get() == initial + 4);
    OK(rohr_physics_mass_set(pa, 7));
    assert(rohr_physics_mass_get(pb).result.value == 2);
    assert(project.water.particle.mass == 2);
    ObjectAppearance appearance;
    assert(object_instance_appearance_get(pa, &appearance) && appearance.visible);
    Entity p = value(rohr_entity_create(&project.water.particle));
    assert(object_instance_appearance_get(p, &appearance) && !appearance.visible);
    assert(rohr_entity_owner_get(p).result.value == ENTITY_INVALID);
    Position position = rohr_physics_position_get(p).result.value;
    assert(position.x == 12 && position.y == 23);
    assert(rohr_error_check(rohr_entity_create(&project.water)));
    assert(rohr_error_check(rohr_object_create(&project.water.particle)));
    assert(rohr_error_check(rohr_entity_create(NULL)));
    assert(rohr_error_check(rohr_object_create(NULL)));
    assert(rohr_error_check(rohr_object_member_get(a, "missing")));
    assert(rohr_error_check(rohr_object_member_get(a, NULL)));
    EntityResult (*create)(const EntityCreateDescription *) = rohr_entity_create;
    Entity via_pointer = value(create(&project.water.particle));
    clean(via_pointer);
    EntityCreateDescription parent = project.water.particle;
    parent.parent = NULL;
    ObjectDescription child = {.id=3,.name="anchor",.kind=OBJECT_ANCHOR,
        .categories=ROHR_ENTITY_OBJECT_ALL,.parent=&parent,.target=parent.id};
    const ObjectDescription *children[] = {&child};
    parent.children = children; parent.child_count = 1;
    Entity parent_only = value(rohr_entity_create(&parent));
    assert(!rohr_entity_components_check(parent_only, ROHR_OBJECT));
    JointAnchorListResult anchors = rohr_physics_joint_anchors_get(parent_only); OK(anchors);
    assert(anchors.result.value.count == 0);
    clean(parent_only);
    ObjectDescriptionResult copy = rohr_object_copy(project.water); OK(copy);
    Entity copied = value(rohr_object_create(copy.result.value));
    OK(rohr_object_destroy(copy.result.value));
    assert(rohr_physics_mass_get(member(copied, "particle")).result.value == 2);
    clean(copied); clean(a);
    assert(rohr_entity_alive_check(pb));
    assert(rohr_error_check(rohr_object_member_get(a, "particle")));
    clean(b); clean(p);
    assert(rohr_entity_alive_count_get() == initial);
}

static void owners_and_names(void) {
    Entity owner = value(rohr_entity_add());
    EntityCreateOptions options = rohr_entity_create_options_default_get();
    options.owner = owner; options.offset = (Position){3, -4};
    Entity p = value(rohr_entity_create_with_options(&project.water.particle, options));
    assert(rohr_entity_owner_get(p).result.value == owner);
    assert(member(owner, "particle") == p);
    Position position = rohr_physics_position_get(p).result.value;
    assert(position.x == 15 && position.y == 19);
    assert(rohr_error_check(rohr_entity_create_with_options(&project.water.particle, options)));
    OK(rohr_entity_name_set(p, "renamed_particle"));
    Entity ambiguous = value(rohr_entity_description_add((EntityDescription){.name="particle",.owner=owner}));
    assert(rohr_error_check(rohr_object_member_get(owner, "particle")));
    uint32_t before = rohr_entity_alive_count_get();
    assert(rohr_error_check(rohr_entity_create_with_options(&project.water.particle, options)));
    assert(rohr_entity_alive_count_get() == before);
    clean(ambiguous);
    ObjectCreateOptions object_options = rohr_object_create_options_default_get();
    object_options.owner = owner;
    Entity nested = value(rohr_object_create_with_options(&project.water, object_options));
    assert(member(owner, "particle") == p);
    assert(member(owner, "water") == nested);
    assert(rohr_error_check(rohr_entity_object_member_by_name_get(owner, "particle")));
    clean(p);
    assert(rohr_error_check(rohr_object_member_get(owner, "particle")));
    assert(!rohr_error_check(rohr_entity_object_member_by_name_get(owner, "particle")));
    clean(owner);
    assert(!rohr_entity_alive_check(nested));
    assert(rohr_error_check(rohr_entity_create_with_options(&project.water.particle, options)));
}

typedef struct Fixture {
    ObjectDescription root, body, anchor, joint, force, camera, sprite;
    const ObjectDescription *members[6];
} Fixture;
static void fixture_init(Fixture *f) {
    *f = (Fixture){0};
    f->root = (ObjectDescription){.id=10,.name="root",.kind=OBJECT_CONTAINER,
        .categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.children=f->members,.child_count=6};
    f->body = (ObjectDescription){.id=11,.name="body",.kind=OBJECT_PARTICLE,
        .categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.parent=&f->root,
        .radius=2,.particle_vertices=8,.mass=3};
    f->anchor = (ObjectDescription){.id=12,.name="anchor",.kind=OBJECT_ANCHOR,
        .categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.parent=&f->root,.target=11,.position={1,0}};
    f->joint = (ObjectDescription){.id=13,.name="joint",.kind=OBJECT_JOINT,
        .categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.parent=&f->root,.endpoint_a=12,.joint_type=JOINT_PIN};
    f->force = (ObjectDescription){.id=14,.name="push",.kind=OBJECT_FORCE,
        .categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.parent=&f->root,
        .force={.mode=OBJECT_SOURCE_INITIAL,.target={OBJECT_SOURCE_PARTICLE,11,0},
            .magnitude=5,.location={.kind=ROHR_FORCE_WORLD_POSITION},
            .orientation={.kind=ROHR_FORCE_WORLD_ORIENTATION}}};
    f->camera = (ObjectDescription){.id=15,.name="camera",.kind=OBJECT_CAMERA,
        .categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.parent=&f->root,.target=11,
        .size={64,64},.zoom=1,.follow_position=true};
    f->sprite = (ObjectDescription){.id=16,.name="sprite",.kind=OBJECT_SPRITE,
        .categories=ROHR_ENTITY_OBJECT_ALL,.visible=true,.parent=&f->root,.target=11,
        .asset_path="creation_pixel.png",.size={2,2},.scale={1,1}};
    /* Forward references deliberately precede their body. */
    f->members[0]=&f->joint; f->members[1]=&f->anchor; f->members[2]=&f->camera;
    f->members[3]=&f->sprite; f->members[4]=&f->body; f->members[5]=&f->force;
}

static void dependencies(void) {
    Fixture f; fixture_init(&f);
    Entity root = value(rohr_object_create(&f.root));
    Entity body = member(root, "body"), anchor = member(root, "anchor");
    assert(member(root, "sprite") == body);
    JointDescription joint = rohr_physics_joint_description_get(member(root,"joint")).result.value;
    assert(joint.anchor_a == rohr_entity_object_anchor_get(anchor).result.value);
    CameraId camera = rohr_entity_object_camera_get(member(root,"camera")).result.value;
    assert(rohr_camera_attachment_description_get(camera).result.value.entity == body);
    assert(rohr_physics_force_description_get(member(root,"push")).result.value.target == body);
    clean(root);

    body = value(rohr_entity_create(&f.body));
    Entity owner = value(rohr_entity_add());
    ObjectCreateBinding binding = {.description=&f.body,.entity=body};
    EntityCreateOptions options = {.bindings=&binding,.binding_count=1};
    uint32_t before = rohr_entity_alive_count_get();
    assert(rohr_error_check(rohr_entity_create(&f.anchor)));
    assert(rohr_entity_alive_count_get() == before);
    anchor = value(rohr_entity_create_with_options(&f.anchor, options));
    Entity visual = value(rohr_entity_create_with_options(&f.sprite, options));
    assert(visual != body && !rohr_entity_components_check(body, ROHR_SPRITE));
    assert(rohr_physics_transform_lock_description_get(visual).result.value.driver == body);
    Entity camera_entity = value(rohr_entity_create_with_options(&f.camera, options));
    camera = rohr_entity_object_camera_get(camera_entity).result.value;
    assert(rohr_camera_attachment_description_get(camera).result.value.entity == body);
    ObjectCreateBinding anchor_binding = {.description=&f.anchor,.entity=anchor};
    EntityCreateOptions joint_options = {.bindings=&anchor_binding,.binding_count=1};
    Entity linked = value(rohr_entity_create_with_options(&f.joint, joint_options));
    assert(rohr_physics_joint_description_get(linked).result.value.anchor_a ==
        rohr_entity_object_anchor_get(anchor).result.value);
    options.owner = owner;
    Velocity velocity = rohr_physics_particle_description_get(body).result.value.velocity;
    Entity force = value(rohr_entity_create_with_options(&f.force, options));
    assert(rohr_physics_force_description_get(force).result.value.target == body);
    assert(rohr_entity_lifetime_description_get(force).result.value.tick_enabled);
    assert(rohr_physics_particle_description_get(body).result.value.velocity.x == velocity.x);
    clean(force);
    f.force.kind = OBJECT_TORQUE;
    f.force.torque = (ObjectTorqueDescription){.mode=OBJECT_SOURCE_INITIAL,
        .target={OBJECT_SOURCE_PARTICLE,11,0},.magnitude=4};
    Entity torque = value(rohr_entity_create_with_options(&f.force, options));
    assert(rohr_physics_torque_description_get(torque).result.value.owner == owner);
    assert(rohr_physics_torque_description_get(torque).result.value.target == body);
    assert(rohr_entity_lifetime_description_get(torque).result.value.tick_enabled);
    clean(owner); clean(linked); clean(anchor); clean(visual); clean(camera_entity);
    assert(rohr_entity_alive_check(body));

    options.owner = ENTITY_INVALID;
    ObjectCreateBinding wrong = {.description=&f.anchor,.entity=body};
    options.bindings = &wrong;
    before = rohr_entity_alive_count_get();
    assert(rohr_error_check(rohr_entity_create_with_options(&f.joint, options)));
    ObjectCreateBinding duplicates[] = {binding, binding};
    options.bindings = duplicates; options.binding_count = 2;
    assert(rohr_error_check(rohr_entity_create_with_options(&f.anchor, options)));
    Fixture foreign; fixture_init(&foreign);
    wrong = (ObjectCreateBinding){.description=&foreign.body,.entity=body};
    options.bindings=&wrong; options.binding_count=1;
    assert(rohr_error_check(rohr_entity_create_with_options(&f.anchor, options)));
    assert(rohr_entity_alive_count_get() == before);
    clean(body);
    options.bindings = &binding;
    assert(rohr_error_check(rohr_entity_create_with_options(&f.anchor, options)));
}

static void topology(void) {
    ObjectDescription soft = {.id=1,.name="soft",.kind=OBJECT_SOFT_BODY,
        .categories=ROHR_ENTITY_OBJECT_ALL,.visible=true};
    ObjectDescription a = {.id=2,.name="a",.kind=OBJECT_NODE,.parent=&soft,
        .categories=ROHR_ENTITY_OBJECT_ALL,.mass=1,.radius=1,.visible=true};
    ObjectDescription b = a; b.id=3; b.name="b"; b.position.x=5;
    ObjectDescription beam = {.id=4,.name="beam",.kind=OBJECT_BEAM,.parent=&soft,
        .categories=ROHR_ENTITY_OBJECT_ALL,.endpoint_a=2,.endpoint_b=3,
        .rest_length=5,.stiffness=10,.thickness=1,.visible=true};
    const ObjectDescription *children[] = {&beam,&a,&b};
    soft.children=children; soft.child_count=3;
    Entity instance = value(rohr_object_create(&soft));
    Entity na = member(instance,"a"), nb = member(instance,"b");
    assert(rohr_physics_soft_body_beam_get(member(instance,"beam")).result.value.node_a == na);
    clean(member(instance,"beam"));
    ObjectCreateBinding bindings[] = {{&a,na},{&b,nb}};
    EntityCreateOptions options = {.owner=instance,.bindings=bindings,.binding_count=2};
    Entity edge = value(rohr_entity_create_with_options(&beam, options));
    assert(rohr_physics_soft_body_beam_get(edge).result.value.soft_body == instance);
    assert(rohr_error_check(rohr_entity_create(&a)));
    Entity other = value(rohr_physics_soft_body_add((SoftBodyDescription){0}));
    options.owner = other;
    assert(rohr_error_check(rohr_entity_create_with_options(&beam, options)));
    assert(rohr_physics_soft_body_get(other).result.value.beam_count == 0);
    clean(other); clean(instance);
}

static void invalid_and_rollback(void) {
    Fixture f; fixture_init(&f);
    uint32_t before = rohr_entity_alive_count_get();
    f.anchor.target = 999;
    assert(rohr_error_check(rohr_object_create(&f.root)));
    f.anchor.target = f.camera.id; f.camera.target = f.anchor.id;
    assert(rohr_error_check(rohr_object_create(&f.root)));
    fixture_init(&f);
    f.body.id = f.camera.id;
    assert(rohr_error_check(rohr_object_create(&f.root)));
    fixture_init(&f); f.camera.name = f.body.name;
    assert(rohr_error_check(rohr_object_create(&f.root)));
    fixture_init(&f); f.root.parent = &f.root;
    assert(rohr_error_check(rohr_object_create(&f.root)));
    fixture_init(&f); f.body.categories = ROHR_ENTITY_OBJECT_PHYSICS;
    ObjectCreateOptions only_visuals = rohr_object_create_options_default_get();
    only_visuals.flags = ROHR_ENTITY_OBJECT_VISUALS;
    assert(rohr_error_check(rohr_object_create_with_options(&f.root, only_visuals)));
    fixture_init(&f); f.force.visuals.main.specified = true;
    f.force.visuals.main.scale.x = NAN;
    assert(rohr_error_check(rohr_object_create(&f.root)));
    assert(rohr_entity_alive_count_get() == before);

#ifdef ROHR_TEST_ALLOCATION_WRAP
    fixture_init(&f);
    Entity body = value(rohr_entity_create(&f.body));
    Entity owner = value(rohr_entity_add());
    ObjectCreateBinding binding = {&f.body,body};
    EntityCreateOptions options = {.owner=owner,.bindings=&binding,.binding_count=1};
    before = rohr_entity_alive_count_get();
    bool completed = false; unsigned failures = 0;
    for(long fail = 0; fail < 512; ++fail) {
        allocation_countdown = fail;
        EntityResult result = rohr_entity_create_with_options(&f.force, options);
        allocation_countdown = -1;
        if(!rohr_error_check(result)) { clean(result.result.value); completed=true; break; }
        ++failures;
        assert(rohr_entity_alive_count_get() == before);
        assert(!rohr_entity_components_check(owner, ROHR_OBJECT));
        assert(rohr_physics_particle_description_get(body).result.value.velocity.x == 0);
    }
    assert(completed && failures > 2);
    completed=false; failures=0;
    for(long fail = 0; fail < 512; ++fail) {
        allocation_countdown = fail;
        EntityResult result = rohr_entity_create_with_options(&f.anchor, options);
        allocation_countdown = -1;
        if(!rohr_error_check(result)) { clean(result.result.value); completed=true; break; }
        ++failures;
        assert(rohr_entity_alive_count_get() == before);
        JointAnchorListResult anchors = rohr_physics_joint_anchors_get(body); OK(anchors);
        assert(anchors.result.value.count == 0);
    }
    assert(completed && failures > 1);
    assert(rohr_physics_joint_anchors_get(body).result.value.count == 0);
    clean(owner); clean(body);
    before = rohr_entity_alive_count_get(); completed=false; failures=0;
    GraphicsTextureAssetStats texture_before = graphics_texture_assets_stats_get();
    for(long fail = 0; fail < 1024; ++fail) {
        allocation_countdown = fail;
        EntityResult result = rohr_object_create(&f.root);
        allocation_countdown = -1;
        if(!rohr_error_check(result)) { clean(result.result.value); completed=true; break; }
        ++failures;
        assert(rohr_entity_alive_count_get() == before);
        GraphicsTextureAssetStats textures = graphics_texture_assets_stats_get();
        assert(textures.live_resources == texture_before.live_resources);
        assert(textures.owner_references == texture_before.owner_references);
    }
    assert(completed && failures > 5);
#endif
}

int main(void) {
    OK(rohr_engine_start());
    assert(SDL_SaveFile("creation_pixel.png", test_png, sizeof(test_png)));
    OK(rohr_graphics_start());
    instances(); owners_and_names(); dependencies(); topology(); invalid_and_rollback();
    uint32_t before = rohr_entity_alive_count_get();
    GraphicsTextureAssetStats texture_before = graphics_texture_assets_stats_get();
    Fixture fixture; fixture_init(&fixture);
    for(unsigned i = 0; i < 1000; ++i) {
        Entity root = value(rohr_object_create(&project.water));
        Entity independent = value(rohr_entity_create(&project.water.particle));
        Entity complex = value(rohr_object_create(&fixture.root));
        CameraId camera = rohr_entity_object_camera_get(member(complex, "camera")).result.value;
        clean(complex);
        assert(rohr_error_check(rohr_camera_get(camera)));
        clean(root);
        assert(rohr_entity_alive_check(independent));
        clean(independent);
        assert(rohr_entity_alive_count_get() == before);
        GraphicsTextureAssetStats textures = graphics_texture_assets_stats_get();
        assert(textures.live_resources == texture_before.live_resources);
        assert(textures.owner_references == texture_before.owner_references);
    }
    assert(before == 0);
    rohr_graphics_stop(); rohr_engine_stop();
    assert(remove("creation_pixel.png") == 0);
    return 0;
}
