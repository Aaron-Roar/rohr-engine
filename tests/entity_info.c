/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "test_png.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define OK(call) assert(!error_check(call))
#define F(name) ROHR_ENTITY_INFO_##name
static const Shape box = {.amount_of_vertices = 4,
    .vertices = {{-1,-1}, {1,-1}, {1,1}, {-1,1}}};
#ifdef ROHR_TEST_ALLOCATION_WRAP
static long fail_after = -1;
static size_t allocated_bytes, allocations;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool fail_check(size_t size) {
    if(fail_after == 0) return true;
    if(fail_after > 0) --fail_after;
    allocated_bytes += size; ++allocations;
    return false;
}
void *__wrap_malloc(size_t n) { return fail_check(n) ? NULL : __real_malloc(n); }
void *__wrap_calloc(size_t n, size_t size) { return fail_check(n * size) ? NULL : __real_calloc(n, size); }
void *__wrap_realloc(void *p, size_t n) { return fail_check(n) ? NULL : __real_realloc(p, n); }
#endif
static Entity make_entity(void) {
    EntityResult result = entity_add(); OK(result); return result.result.value;
}
static EntityInfo capture(Entity e, EntityInfoFlags flags) {
    EntityInfoResult result = entity_info_get(e, flags); OK(result); return result.result.value;
}
static EntityIndex index_of(Entity e) {
    EntityIndex index; assert(entity_index_get(e, &index)); return index;
}
static void basics(void) {
    Entity e = make_entity();
    assert(error_check(entity_info_get(ENTITY_INVALID, ROHR_ENTITY_INFO)));
    assert(error_check(entity_info_get(e, 0)));
    assert(error_check(entity_info_get(e, UINT64_C(1) << 63)));
    assert(!entity_info_check(NULL, F(CORE_NAME)));
    assert(error_check(entity_info_retain(NULL)));
    assert(error_check(entity_info_release(NULL)));
    EntityInfo info = capture(e, F(GRAPHICS_PRESENCE) | F(GRAPHICS_ANIMATIONS) | F(CORE_CHILDREN));
    assert(entity_info_check(&info, F(GRAPHICS_PRESENCE) | F(GRAPHICS_ANIMATION_COUNT)));
    assert(!info.has_sprite && !info.is_animated && !info.has_animations);
    assert(info.animations == NULL && info.animation_count == 0);
    assert(info.children == NULL && info.child_count == 0);
    assert(!entity_info_check(&info, 0));
    assert(!entity_info_check(&info, UINT64_C(1) << 61));
    assert(info.requested == (F(GRAPHICS_PRESENCE) | F(GRAPHICS_ANIMATIONS) | F(CORE_CHILDREN)));
    EntityInfoResult retained = rohr_entity_info_retain(&info); OK(retained);
    EntityInfo copy = retained.result.value;
    EntityInfo stale = info; /* Non-owning alias, only used to verify stale rejection. */
    OK(entity_info_release(&info)); OK(entity_info_release(&info));
    assert(!entity_info_check(&info, ROHR_ENTITY_INFO));
    assert(entity_info_check(&copy, F(GRAPHICS_PRESENCE)));
    OK(rohr_entity_info_release(&copy));
    assert(error_check(entity_info_retain(&stale)));
    assert(error_check(entity_info_release(&stale)));
    assert(error_check(entity_info_issues_count_get(&stale, ROHR_ENTITY_INFO)));
    EntityInfo newer = capture(e, F(CORE_COMPONENTS));
    assert(newer.handle != stale.handle); OK(entity_info_release(&newer));
    OK(entity_delete(e)); assert(error_check(entity_info_get(e, ROHR_ENTITY_INFO)));
}
static void issues(void) {
    Entity e = make_entity();
    EntityInfo info = capture(e, F(PHYSICS_VELOCITY) | F(CORE_PARENT));
    assert(info.available == 0);
    assert(entity_info_issues_count_get(&info, ROHR_ENTITY_INFO).result.value == 2);
    assert(entity_info_issues_count_get(&info, F(PHYSICS_ACCELERATION)).result.value == 0);
    EntityInfoIssue record = {0};
    EntityInfoIssueReadResult read = rohr_entity_info_issues_get(&info, ROHR_ENTITY_INFO, &record, 1); OK(read);
    assert(read.result.value.total == 2 && read.result.value.written == 1);
    assert(record.section == F(CORE_PARENT) && record.reason == ROHR_ENTITY_INFO_COMPONENT_ABSENT);
    EntityInfoIssue saved = record;
    assert(error_check(entity_info_issues_get(&info, 0, &record, 1)));
    assert(memcmp(&record, &saved, sizeof(record)) == 0);
    assert(error_check(entity_info_issues_get(&info, ROHR_ENTITY_INFO, NULL, 1)));
    read = entity_info_issues_get(&info, ROHR_ENTITY_INFO, NULL, 0); OK(read);
    assert(read.result.value.written == 0 && read.result.value.total == 2);
    OK(entity_info_release(&info)); assert(strlen(record.detail) > 0);

    /* Independently missing dependencies in one still-available configuration. */
    EntityIndex index = index_of(e);
    OK(JointPool_store_at(&joints_pool, index, (Joint){.a = 123456, .b = 234567}));
    entity_mask[index] |= ROHR_JOINT;
    info = capture(e, F(PHYSICS_JOINT));
    assert(entity_info_check(&info, F(PHYSICS_JOINT)));
    EntityInfoIssue records[4];
    read = entity_info_issues_get(&info, ROHR_ENTITY_INFO, records, 4); OK(read);
    assert(read.result.value.total == 2 && read.result.value.written == 2);
    assert(records[0].reference.id == 123456 && records[1].reference.id == 234567);
    assert(records[0].kind == ROHR_ENTITY_INFO_WARNING);
    OK(entity_info_release(&info));
    entity_mask[index] &= ~ROHR_JOINT;
    OK(entity_delete(e));
}
static Entity fixture(void) {
    Entity e = make_entity();
    OK(entity_name_set(e, "inspection fixture"));
    OK(physics_position_set(e, (Position){3,4}));
    OK(physics_orientation_set(e, 90));
    OK(physics_velocity_set(e, (Velocity){5,6}));
    OK(physics_mass_set(e, 2));
    for(size_t i = 0; i < 64; ++i) OK(physics_hitbox_add(e, box));
    OK(physics_joint_anchor_create(e, (Position){1,0}));
    Entity child = make_entity(); OK(entity_child_set(e, child));
    GroupIdResult group = entity_group_create(); OK(group); OK(entity_group_add(group.result.value, e));
    EntityIndex index = index_of(e);
    AnimatedSprite animation = {.frame_count = 2, .player = {.frame_count = 2}, .visible = true, .scale = {2,3}};
    animation.frames[0].texture.handle = 1234; animation.frames[1].texture.handle = 5678;
    OK(AnimatedSpritePool_store_at(&animated_sprites_pool, index, animation));
    entity_mask[index] |= ROHR_ANIMATED_SPRITE;
    OK(graphics_layer_entity_set(e, 9));
    return e;
}
static void snapshot_checks(Entity e) {
    static ReportResult before, after;
    before = report_get(ROHR_REPORT); OK(before);
    EntityIndex index = index_of(e);
    Position position = positions[index];
    Shape world = world_hit_boxes[index];
    EntityInfo info = capture(e, ROHR_ENTITY_INFO);
    assert(info.velocity.x == 5 && info.velocity.y == 6);
    assert(info.hitbox_count == 64 && info.hitboxes[63].shape.amount_of_vertices == 4);
    assert(info.child_count == 1 && info.group_count >= 1);
    assert(info.animation_count == 1 && info.animations[0].frame_count == 2);
    assert(info.has_animations && info.is_animated && !info.has_sprite);
    assert(info.entity_layer.value == 9 && info.entity_layer.id == 0);
    assert(info.joint_anchor_count == 1 && info.joint_anchors[0].anchor.local_offset.x == 1);
    assert(fabsf(info.joint_anchors_world[0].position.x - 3) < 0.001f);
    assert(fabsf(info.joint_anchors_world[0].position.y - 3) < 0.001f);
    assert(fabsf(info.center_of_mass_world.x - 3) < 0.001f);
    assert(fabsf(info.moment_of_inertia - 4.0f/3.0f) < 0.001f);
    after = report_get(ROHR_REPORT); OK(after);
    /* Getter returns the same stored reports; no publishing during inspection. */
    assert(before.result.value.version == after.result.value.version);
    assert(memcmp(&world, &world_hit_boxes[index], sizeof(world)) == 0);
    assert(memcmp(&position, &positions[index], sizeof(position)) == 0);
    EntityInfo counts = capture(e, F(PHYSICS_HITBOX_COUNT) | F(GRAPHICS_ANIMATION_COUNT));
    assert(counts.hitboxes == NULL && counts.animations == NULL && counts.hitbox_count == 64);
    assert(counts.available == counts.requested); OK(entity_info_release(&counts));
    EntityInfoResult public_result = rohr_entity_info_get(e, F(PHYSICS_VELOCITY)); OK(public_result);
    EntityInfo direct = capture(e, F(PHYSICS_VELOCITY));
    assert(public_result.result.value.available == direct.available);
    assert(public_result.result.value.velocity.x == direct.velocity.x);
    OK(rohr_entity_info_release(&public_result.result.value)); OK(entity_info_release(&direct));
    EntityInfoIssue records[4];
    EntityInfoIssueReadResult read = entity_info_issues_get(&info, F(GRAPHICS_ANIMATIONS), records, 4); OK(read);
    assert(read.result.value.total == 2 && records[0].index == 0 && records[1].index == 1);
    OK(physics_velocity_set(e, (Velocity){20,30}));
    OK(physics_hitbox_set(e, (Shape){.amount_of_vertices=3, .vertices={{0,0},{2,0},{0,2}}}));
    assert(info.velocity.x == 5 && info.hitboxes[0].shape.amount_of_vertices == 4);
    EntityInfoResult retained = entity_info_retain(&info); OK(retained);
    assert(retained.result.value.animations == info.animations);
    OK(entity_info_release(&info));
    info = retained.result.value;
    /* Clear borrowed fake resources before engine teardown. */
    animated_sprites[index] = (AnimatedSprite){0};
    entity_mask[index] &= ~ROHR_ANIMATED_SPRITE;
    OK(entity_delete(e));
    assert(info.hitboxes[0].shape.amount_of_vertices == 4 && info.animations[0].visible);
    engine_stop();
    assert(entity_info_check(&info, F(PHYSICS_HITBOXES)));
    assert(entity_info_issues_count_get(&info, F(GRAPHICS_ANIMATIONS)).result.value == 2);
    OK(entity_info_release(&info));
}
static void geometry_and_sources(void) {
    Entity e = make_entity();
    OK(physics_force_description_set(e, (ForceDescription){0})); OK(physics_torque_description_set(e, (TorqueDescription){.magnitude = 0}));
    EntityInfo info = capture(e, F(PHYSICS_FORCE) | F(PHYSICS_TORQUE));
    assert(entity_info_check(&info, info.requested));
    assert(entity_info_issues_count_get(&info, info.requested).result.value == 0);
    OK(entity_info_release(&info));
    OK(physics_hitbox_set(e, box));
    for(size_t i = 0; i < MAX_JOINT_ANCHORS_PER_ENTITY; ++i)
        OK(physics_joint_anchor_create(e, (Position){(float)i, 0}));
    info = capture(e, F(PHYSICS_JOINT_ANCHORS) | F(PHYSICS_JOINT_ANCHORS_WORLD));
    assert(info.joint_anchor_count == MAX_JOINT_ANCHORS_PER_ENTITY);
    assert(info.joint_anchors[31].anchor.local_offset.x == 31);
    OK(entity_info_release(&info));
    EntityIndex index = index_of(e);
    uint8_t previous = positions_pool.used[index];
    positions_pool.used[index] = 0; /* Missing prerequisite must not resolve to zero. */
    info = capture(e, F(PHYSICS_JOINT_ANCHORS) | F(PHYSICS_JOINT_ANCHORS_WORLD) | F(PHYSICS_HITBOX_WORLD));
    assert(info.joint_anchors && !info.joint_anchors_world);
    assert(!entity_info_check(&info, F(PHYSICS_HITBOX_WORLD)));
    assert(entity_info_issues_count_get(&info, F(PHYSICS_JOINT_ANCHORS_WORLD)).result.value == 32);
    positions_pool.used[index] = previous; OK(entity_info_release(&info));
    OK(entity_delete(e));

    EntityResult body = physics_soft_body_create(); OK(body);
    Entity nodes[3];
    Position points[] = {{0,0}, {4,0}, {0,4}};
    for(size_t i = 0; i < 3; ++i) {
        EntityResult node = physics_soft_body_node_create(body.result.value, points[i], 1, 0.5f);
        OK(node); nodes[i] = node.result.value;
    }
    EntityResult beam = physics_soft_body_beam_create(body.result.value, nodes[0], nodes[1], 10, 1); OK(beam);
    SoftBodyAreaGeometry geometry = {.outer = {.node_count = 3}};
    memcpy(geometry.outer.nodes, nodes, sizeof(nodes));
    EntityResult area = physics_soft_body_area_create(body.result.value, geometry); OK(area);
    info = capture(body.result.value, F(PHYSICS_SOFT_BODY) | F(PHYSICS_COM_WORLD) | F(PHYSICS_MASS));
    assert(info.soft_body_node_count == 3 && info.soft_body_beam_count == 1 && info.soft_body_area_count == 1);
    assert(info.mass_value == 3 && fabsf(info.center_of_mass_world.x - 4.0f/3) < 0.001f);
    OK(entity_info_release(&info));
    info = capture(area.result.value, F(PHYSICS_SOFT_BODY_AREA));
    assert(info.soft_body_area.geometry.outer.nodes[1] == nodes[1]);
    assert(entity_info_issues_count_get(&info, ROHR_ENTITY_INFO).result.value == 0);
    OK(entity_info_release(&info));
    EntityIndex ni0 = index_of(nodes[0]), ni1 = index_of(nodes[1]);
    mass_pool.used[ni0] = mass_pool.used[ni1] = 0;
    info = capture(body.result.value, F(PHYSICS_COM_WORLD));
    assert(!entity_info_check(&info, F(PHYSICS_COM_WORLD)));
    assert(entity_info_issues_count_get(&info, ROHR_ENTITY_INFO).result.value == 2);
    OK(entity_info_release(&info));
    mass_pool.used[ni0] = mass_pool.used[ni1] = 1;
    info = capture(nodes[0], F(PHYSICS_HITBOX_WORLD));
    assert(entity_info_check(&info, F(PHYSICS_HITBOX_WORLD)));
    OK(entity_info_release(&info));
    OK(physics_position_set(nodes[1], points[0]));
    info = capture(beam.result.value, F(PHYSICS_SOFT_BODY_BEAM));
    assert(entity_info_check(&info, F(PHYSICS_SOFT_BODY_BEAM)));
    EntityInfoIssue issue;
    EntityInfoIssueReadResult read = entity_info_issues_get(&info, ROHR_ENTITY_INFO, &issue, 1); OK(read);
    assert(read.result.value.total == 1 && issue.reason == ROHR_ENTITY_INFO_GEOMETRY_UNRESOLVABLE);
    OK(entity_info_release(&info));
    OK(entity_delete(body.result.value));
}
static void resources(void) {
    OK(graphics_start());
    const char *path = "entity_info_test.png";
    FILE *file = fopen(path, "wb"); assert(file);
    assert(fwrite(test_png, 1, sizeof(test_png), file) == sizeof(test_png));
    assert(fclose(file) == 0);
    TextureAssetResult loaded = graphics_texture_load((TextureDescriptor){path, {1,1}}); OK(loaded);
    TextureAsset texture = loaded.result.value;
    Entity e = make_entity();
    Sprite sprite = graphics_sprite_create(texture, (Scale){2,3});
    sprite.visible = false;
    OK(graphics_sprite_add(e, sprite));
    AnimatedSprite animation = {.scale = {4,5}, .visible = true};
    OK(graphics_animated_sprite_value_frame_add(&animation, graphics_animation_frame_create(texture)));
    OK(graphics_animated_sprite_add(e, animation));
    EntityInfo info = capture(e, F(GRAPHICS_SPRITE) | F(GRAPHICS_ANIMATIONS));
    assert(info.sprite_visible == false && info.sprite_scale.x == 2);
    assert(info.animations[0].scale.y == 5);
    assert(entity_info_issues_count_get(&info, ROHR_ENTITY_INFO).result.value == 0);
    TextureAsset borrowed = info.sprite.texture;
    OK(graphics_texture_release(&texture)); OK(entity_delete(e));
    assert(!graphics_texture_valid_check(borrowed)); /* Snapshot did not retain. */
    assert(info.sprite_scale.x == 2 && info.animations[0].visible);
    graphics_stop();
    assert(info.animation_count == 1 && info.animations[0].frame_count == 1);
    OK(entity_info_release(&info));
    assert(remove(path) == 0);
}
static void interactions(void) {
    Entity a = make_entity(), b = make_entity();
    OK(physics_hitbox_set(a, box)); OK(physics_hitbox_set(b, box));
    OK(physics_position_set(a, (Position){0,0})); OK(physics_position_set(b, (Position){1,0}));
    OK(physics_mass_set(a, 1)); OK(physics_dynamic_set(a)); OK(physics_static_set(b));
    OK(entity_components_add(a, ROHR_COLLISION)); OK(entity_components_add(b, ROHR_COLLISION));
    (void)system_physics_update(0.01);
    EntityInfo info = capture(a, F(PHYSICS_OVERLAPS) | F(PHYSICS_CONTACTS));
    assert(info.overlap_count == 1 && info.contact_count == 1);
    assert(info.overlaps[0].target == b && info.contacts[0].target == b);
    assert(info.overlaps[0].overlap.detected && info.contacts[0].contact.detected);
    OK(physics_position_set(b, (Position){100,0}));
    (void)system_physics_update(0.01);
    assert(info.contact_count == 1 && info.contacts[0].contact.detected);
    assert(physics_contact_count_get(a) == 0);
    OK(entity_info_release(&info)); OK(entity_delete(a)); OK(entity_delete(b));
}
static void selection_parity(Entity e) {
    for(unsigned bit = 0; bit < 61; ++bit) {
        EntityInfoFlags flag = UINT64_C(1) << bit;
        EntityInfo a = capture(e, flag);
        EntityInfoResult result = rohr_entity_info_get(e, flag); OK(result);
        EntityInfo b = result.result.value;
        assert(a.requested == b.requested && a.available == b.available);
        EntityInfoIssueCountResult count = entity_info_issues_count_get(&a, flag); OK(count);
        assert(count.result.value == rohr_entity_info_issues_count_get(&b, flag).result.value);
        EntityInfoIssue left[64], right[64];
        EntityInfoIssueReadResult ar = entity_info_issues_get(&a, flag, left, 64);
        EntityInfoIssueReadResult br = rohr_entity_info_issues_get(&b, flag, right, 64);
        OK(ar); OK(br); assert(ar.result.value.total == br.result.value.total);
        for(size_t i = 0; i < ar.result.value.written; ++i) {
            assert(left[i].section == right[i].section && left[i].reason == right[i].reason);
            assert(strcmp(left[i].detail, right[i].detail) == 0);
        }
        OK(entity_info_release(&a)); OK(rohr_entity_info_release(&b));
    }
}
static void dynamic_collection(void) {
    Entity e = make_entity();
    for(size_t i = 0; i < 1024; ++i) OK(physics_hitbox_add(e, box));
    EntityInfo info = capture(e, F(PHYSICS_HITBOXES));
    assert(info.hitbox_count == 1024 && info.hitboxes[1023].shape.amount_of_vertices == 4);
    printf("1024-hitbox frozen array: %zu bytes\n", info.hitbox_count * sizeof(*info.hitboxes));
    OK(entity_delete(e));
    assert(info.hitboxes[1023].id != 0);
    OK(entity_info_release(&info));
}
static void allocation_and_performance(Entity e) {
#ifdef ROHR_TEST_ALLOCATION_WRAP
    bool succeeded = false;
    for(long n = 0; n < 64; ++n) {
        fail_after = n;
        EntityInfoResult result = entity_info_get(e, ROHR_ENTITY_INFO);
        fail_after = -1;
        if(result.kind == ERROR_RESULT_VALUE) {
            OK(entity_info_release(&result.result.value)); succeeded = true; break;
        }
        assert(result.result.error == ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    assert(succeeded);
    EntityIndex index = index_of(e);
    size_t stored_count = hitbox_variants_pool.objects[index].count;
    hitbox_variants_pool.objects[index].count = SIZE_MAX;
    EntityInfoResult overflow = entity_info_get(e, F(PHYSICS_HITBOXES));
    assert(error_check(overflow) && overflow.result.error == ERROR_MEMORY_POOL_CAPACITY_OVERFLOW);
    hitbox_variants_pool.objects[index].count = stored_count;
    allocated_bytes = allocations = 0;
    EntityInfo counts = capture(e, F(PHYSICS_HITBOX_COUNT));
    size_t count_bytes = allocated_bytes;
    assert(allocations == 1); OK(entity_info_release(&counts));
    allocated_bytes = allocations = 0;
    EntityInfo full = capture(e, ROHR_ENTITY_INFO);
    printf("EntityInfo=%zu bytes, count capture=%zu bytes, all capture=%zu bytes (%zu allocations)\n",
        sizeof(EntityInfo), count_bytes, allocated_bytes, allocations);
    OK(entity_info_release(&full));
#endif
    clock_t start = clock();
    for(size_t i = 0; i < 100; ++i) {
        EntityInfo info = capture(e, ROHR_ENTITY_INFO); OK(entity_info_release(&info));
    }
    printf("100 full captures: %.3f ms\n", 1000.0 * (clock() - start) / CLOCKS_PER_SEC);
}
int main(void) {
    OK(engine_start());
    basics(); issues(); geometry_and_sources(); resources(); interactions(); dynamic_collection();
    Entity e = fixture();
    selection_parity(e);
    allocation_and_performance(e);
    snapshot_checks(e);
    return 0;
}
