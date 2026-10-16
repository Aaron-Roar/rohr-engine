/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "runtime_topology.h"
#include "entity/entity_internal.h"
#include "entity/object_internal.h"
#include "physics/soft_body/soft_body_area.h"
#include "physics/physics_internal.h"
#include <float.h>
#include <math.h>
#include <string.h>

static bool number(yyjson_val *v, const char *key, float *out) {
    yyjson_val *n = yyjson_obj_get(v, key);
    if(!yyjson_is_num(n) || !isfinite(yyjson_get_num(n)) ||
            fabs(yyjson_get_num(n)) > FLT_MAX) return false;
    *out = (float)yyjson_get_num(n);
    return true;
}
static bool boolean(yyjson_val *v, const char *key, bool *out) {
    yyjson_val *n = yyjson_obj_get(v, key);
    if(!yyjson_is_bool(n)) return false;
    *out = yyjson_get_bool(n);
    return true;
}
static bool uint64(yyjson_val *v, const char *key, uint64_t *out) {
    yyjson_val *n = yyjson_obj_get(v, key);
    if(!yyjson_is_uint(n)) return false;
    *out = yyjson_get_uint(n);
    return true;
}
static bool reference(yyjson_val *v, Entity *out) {
    if(yyjson_is_null(v)) { *out = ENTITY_INVALID; return true; }
    if(!yyjson_is_str(v) || strlen(yyjson_get_str(v)) != yyjson_get_len(v)) return false;
    EntityResult result = entity_by_name_get(yyjson_get_str(v));
    if(error_check(result)) return false;
    *out = result.result.value;
    return true;
}
static bool incoming(Entity e, const Entity *created, size_t count) {
    for(size_t i = 0; i < count; ++i) if(created[i] == e) return true;
    return false;
}
static bool color_read(yyjson_val *v, Color *color, bool *overridden) {
    yyjson_val *c = yyjson_obj_get(v, "draw_color");
    uint64_t r, g, b, a;
    if(!uint64(c, "red", &r) || !uint64(c, "green", &g) ||
            !uint64(c, "blue", &b) || !uint64(c, "alpha", &a) ||
            r > 255 || g > 255 || b > 255 || a > 255 ||
            !boolean(v, "draw_color_overridden", overridden)) return false;
    *color = (Color){r, g, b, a};
    return true;
}
static bool list_read(yyjson_val *v, Entity *entities, uint32_t *count, uint32_t capacity) {
    if(!yyjson_is_arr(v) || yyjson_arr_size(v) > capacity) return false;
    *count = (uint32_t)yyjson_arr_size(v);
    for(uint32_t i = 0; i < *count; ++i) {
        if(!reference(yyjson_arr_get(v, i), &entities[i])) return false;
        for(uint32_t j = 0; j < i; ++j)
            if(entities[i] != ENTITY_INVALID && entities[j] == entities[i]) return false;
    }
    return true;
}
static bool order_read(yyjson_val *v, Entity *members, uint32_t count, uint32_t capacity) {
    Entity ordered[SOFT_BODY_MAX_BEAMS];
    uint32_t length;
    if(!list_read(v, ordered, &length, capacity) || length != count) return false;
    for(uint32_t i = 0; i < count; ++i) {
        bool found = false;
        for(uint32_t j = 0; j < count; ++j) if(ordered[i] == members[j]) found = true;
        if(!found) return false;
    }
    memcpy(members, ordered, count * sizeof(Entity));
    return true;
}

EngineResult state_topology_load(Entity entity, yyjson_val *components,
        unsigned pass, const Entity *created, size_t count) {
    EntityIndex index;
    if(!entity_index_get(entity, &index)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    yyjson_val *body = yyjson_obj_get(components, "soft_body");
    yyjson_val *node = yyjson_obj_get(components, "soft_body_node");
    yyjson_val *beam = yyjson_obj_get(components, "soft_body_beam");
    yyjson_val *area = yyjson_obj_get(components, "soft_body_area");
    yyjson_val *object = yyjson_obj_get(components, "object");
#define REQUIRE(condition) do { if(!(condition)) return error_result_error(ERROR_ENGINE_STATE_INVALID); } while(0)
#define STORE(call) do { if(error_check(call)) return error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED); } while(0)
    if(pass == 0) {
        REQUIRE((body != NULL) + (node != NULL) + (beam != NULL) + (area != NULL) <= 1);
        yyjson_val *mask = yyjson_obj_get(components, "mask");
        if(mask != NULL) {
            const char *keys[] = {"soft_body", "soft_body_node", "soft_body_beam", "soft_body_area", "object"};
            const RohrComponentMask bits[] = {ROHR_SOFT_BODY, ROHR_SOFT_BODY_NODE,
                ROHR_SOFT_BODY_BEAM, ROHR_SOFT_BODY_AREA, ROHR_OBJECT};
            for(size_t k = 0; k < 5; ++k)
                REQUIRE(((yyjson_get_uint(mask) & bits[k]) != 0) ==
                    (yyjson_obj_get(components, keys[k]) != NULL));
        }
        if(body != NULL) {
            REQUIRE(yyjson_is_obj(body) && positions_pool.used[index] && orientations_pool.used[index]);
            STORE(SoftBodyPool_store_at(&soft_bodies_pool, index, (SoftBody){.origin = entity}));
            entity_mask[index] |= ROHR_SOFT_BODY;
        }
        if(object != NULL) {
            REQUIRE(yyjson_is_arr(object) && yyjson_arr_size(object) <= MAX_ENTITIES);
            STORE(entity_object_initialize(entity));
        }
    }
    if(pass == 2 && node != NULL) {
        SoftBodyNode value = {0};
        REQUIRE(yyjson_is_obj(node) && !yyjson_obj_get(node, "owner"));
        REQUIRE(number(node, "radius", &value.radius) &&
            value.radius > 0 && positions_pool.used[index] && mass_pool.used[index] && mass[index] > 0 &&
            velocities_pool.used[index] && accelerations_pool.used[index] &&
            color_read(node, &value.draw_color, &value.draw_color_overridden));
        CollisionFilterConfigResult filter = physics_collision_filter_get(entity);
        REQUIRE(!error_check(filter));
        value.category = filter.result.value.category;
        value.collides_with = filter.result.value.collides_with;
        STORE(SoftBodyNodePool_store_at(&soft_body_nodes_pool, index, value));
        entity_mask[index] |= ROHR_SOFT_BODY_NODE | ROHR_PARTICLE;
        Entity owner = entity_owner_get(entity).result.value;
        EngineResult membership = physics_soft_body_membership_validate(index, owner);
        if(error_check(membership)) return membership;
        physics_soft_body_membership_update(entity, index, owner);
    }
    if(pass == 3 && beam != NULL) {
        SoftBodyBeam value = {0};
        EntityIndex a = 0, b = 0;
        REQUIRE(yyjson_is_obj(beam) && !yyjson_obj_get(beam, "owner") &&
            reference(yyjson_obj_get(beam, "node_a"), &value.node_a) &&
            reference(yyjson_obj_get(beam, "node_b"), &value.node_b) &&
            (!value.node_a || value.node_a != value.node_b) &&
            (!value.node_a || (entity_index_get(value.node_a, &a) && soft_body_nodes_pool.used[a])) &&
            (!value.node_b || (entity_index_get(value.node_b, &b) && soft_body_nodes_pool.used[b])));
#define BEAM_NUMBER(field) REQUIRE(number(beam, #field, &value.field))
        BEAM_NUMBER(rest_length); BEAM_NUMBER(stiffness); BEAM_NUMBER(damping);
        BEAM_NUMBER(collision_thickness); BEAM_NUMBER(friction); BEAM_NUMBER(restitution);
#undef BEAM_NUMBER
        REQUIRE(value.rest_length >= 0 && value.stiffness >= 0 && value.damping >= 0 &&
            value.collision_thickness >= ROHR_SOFT_BODY_BEAM_COLLISION_THICKNESS_MIN &&
            value.friction >= 0 && value.restitution >= 0 && value.restitution <= 1 &&
            boolean(beam, "collision_enabled", &value.collision_enabled) &&
            uint64(beam, "category", &value.category) && uint64(beam, "collides_with", &value.collides_with) &&
            color_read(beam, &value.draw_color, &value.draw_color_overridden));
        REQUIRE(!value.collision_enabled || !value.node_a || !value.node_b || value.collision_thickness <=
            2.0f * fminf(soft_body_nodes[a].radius, soft_body_nodes[b].radius));
        STORE(SoftBodyBeamPool_store_at(&soft_body_beams_pool, index, value));
        entity_mask[index] |= ROHR_SOFT_BODY_BEAM;
        Entity owner = entity_owner_get(entity).result.value;
        EngineResult membership = physics_soft_body_membership_validate(index, owner);
        if(error_check(membership)) return membership;
        physics_soft_body_membership_update(entity, index, owner);
    }
    if(pass == 3 && area != NULL) {
        SoftBodyArea value = {0};
        REQUIRE(yyjson_is_obj(area) && !yyjson_obj_get(area, "owner") &&
            list_read(yyjson_obj_get(area, "outer"), value.geometry.outer.nodes,
                &value.geometry.outer.node_count, SOFT_BODY_MAX_NODES) &&
            boolean(area, "visible", &value.visible) &&
            color_read(area, &value.draw_color, &value.draw_color_overridden));
        yyjson_val *holes = yyjson_obj_get(area, "holes");
        REQUIRE(yyjson_is_arr(holes) && yyjson_arr_size(holes) <= SOFT_BODY_MAX_AREA_HOLES);
        value.geometry.hole_count = (uint32_t)yyjson_arr_size(holes);
        for(uint32_t h = 0; h < value.geometry.hole_count; ++h)
            REQUIRE(list_read(yyjson_arr_get(holes, h), value.geometry.holes[h].nodes,
                &value.geometry.holes[h].node_count, SOFT_BODY_MAX_NODES));
        EngineResult restored = physics_soft_body_area_restore(entity, value);
        if(error_check(restored)) return restored;
    }
    if(pass == 3) {
        yyjson_val *anchors = yyjson_obj_get(components, "joint_anchors");
        if(anchors != NULL) {
            REQUIRE(yyjson_is_arr(anchors) && yyjson_arr_size(anchors) <= MAX_JOINT_ANCHORS_PER_ENTITY);
            for(size_t a = 0; a < yyjson_arr_size(anchors); ++a) {
                Vec2D local;
                REQUIRE(number(yyjson_arr_get(anchors, a), "x", &local.x) &&
                    number(yyjson_arr_get(anchors, a), "y", &local.y));
                JointAnchorIdResult result = physics_joint_anchor_create(entity, local);
                if(error_check(result)) return error_result_error(result.result.error);
            }
        }
    }
    if(pass == 4 && body != NULL) {
        SoftBody *value = &soft_bodies[index];
        REQUIRE(order_read(yyjson_obj_get(body, "nodes"), value->nodes, value->node_count, SOFT_BODY_MAX_NODES) &&
            order_read(yyjson_obj_get(body, "beams"), value->beams, value->beam_count, SOFT_BODY_MAX_BEAMS) &&
            order_read(yyjson_obj_get(body, "areas"), value->areas, value->area_count, SOFT_BODY_MAX_AREAS));
    }
    if(pass == 1 && object != NULL) {
        for(size_t n = 0; n < yyjson_arr_size(object); ++n) {
            yyjson_val *entry = yyjson_arr_get(object, n);
            yyjson_val *name = yyjson_obj_get(entry, "name");
            Entity member;
            REQUIRE(yyjson_is_obj(entry) && yyjson_is_str(name) &&
                yyjson_get_len(name) == strlen(yyjson_get_str(name)) &&
                reference(yyjson_obj_get(entry, "entity"), &member) && incoming(member, created, count));
            REQUIRE(entity_owner_get(member).result.value == ENTITY_INVALID);
            EngineResult result = entity_object_member_restore(entity, member, yyjson_get_str(name));
            if(error_check(result)) return result;
        }
    }
    if(pass == 5) {
        yyjson_val *owned_anchor = yyjson_obj_get(components, "object_anchor");
        if(owned_anchor) {
            Entity owner; JointAnchorId anchor;
            REQUIRE(yyjson_is_obj(owned_anchor) && reference(yyjson_obj_get(owned_anchor, "owner"), &owner) &&
                incoming(owner, created, count) && state_joint_anchor_read(yyjson_obj_get(owned_anchor, "index"), owner, &anchor));
            EngineResult restored = object_instance_anchor_restore(entity, owner, anchor);
            if(error_check(restored)) return restored;
        }
        yyjson_val *aliases = yyjson_obj_get(components, "object_aliases");
        if(aliases) {
            REQUIRE(object && yyjson_is_arr(aliases) && yyjson_arr_size(aliases) <= MAX_ENTITIES);
            for(size_t n = 0; n < yyjson_arr_size(aliases); ++n) {
                yyjson_val *entry = yyjson_arr_get(aliases, n), *name = yyjson_obj_get(entry, "name");
                Entity member;
                REQUIRE(yyjson_is_obj(entry) && yyjson_is_str(name) &&
                    yyjson_get_len(name) == strlen(yyjson_get_str(name)) &&
                    reference(yyjson_obj_get(entry, "entity"), &member) && incoming(member, created, count));
                EngineResult result = entity_object_alias_restore(entity, member, yyjson_get_str(name));
                if(error_check(result)) return result;
            }
        }
    }
#undef REQUIRE
#undef STORE
    return error_result_value(true);
}

static bool reference_write(yyjson_mut_doc *doc, yyjson_mut_val *v, const char *key, Entity entity) {
    if(entity == ENTITY_INVALID) return yyjson_mut_obj_add_null(doc, v, key);
    EntityNameResult name = entity_name_get(entity);
    return !error_check(name) && yyjson_mut_obj_add_strcpy(doc, v, key, name.result.value.value);
}
static yyjson_mut_val *list_write(yyjson_mut_doc *doc, const Entity *entities, uint32_t count) {
    yyjson_mut_val *array = yyjson_mut_arr(doc);
    if(!array) return NULL;
    for(uint32_t i = 0; i < count; ++i) {
        if(entities[i] == ENTITY_INVALID) {
            if(!yyjson_mut_arr_add_null(doc, array)) return NULL;
            continue;
        }
        EntityNameResult name = entity_name_get(entities[i]);
        if(error_check(name) || !yyjson_mut_arr_add_strcpy(doc, array, name.result.value.value)) return NULL;
    }
    return array;
}
static bool color_write(yyjson_mut_doc *doc, yyjson_mut_val *v, Color color, bool overridden) {
    yyjson_mut_val *c = yyjson_mut_obj(doc);
    return c && yyjson_mut_obj_add_uint(doc, c, "red", color.red) &&
        yyjson_mut_obj_add_uint(doc, c, "green", color.green) &&
        yyjson_mut_obj_add_uint(doc, c, "blue", color.blue) &&
        yyjson_mut_obj_add_uint(doc, c, "alpha", color.alpha) &&
        yyjson_mut_obj_add_val(doc, v, "draw_color", c) &&
        yyjson_mut_obj_add_bool(doc, v, "draw_color_overridden", overridden);
}

bool state_topology_write(yyjson_mut_doc *doc, yyjson_mut_val *components, Entity entity) {
    EntityIndex index;
    if(!entity_index_get(entity, &index)) return false;
#define WRITE(condition) do { if(!(condition)) return false; } while(0)
    JointAnchorIdResult anchor = entity_object_anchor_get(entity);
    if(!error_check(anchor)) {
        Entity owner = object_instance_anchor_owner_get(entity);
        yyjson_mut_val *v = yyjson_mut_obj(doc);
        WRITE(v && reference_write(doc, v, "owner", owner) &&
            state_joint_anchor_write(doc, v, "index", owner, anchor.result.value) &&
            yyjson_mut_obj_add_val(doc, components, "object_anchor", v));
    }
    if(entity_mask[index] & ROHR_OBJECT) {
        GroupIdResult members = entity_object_members_get(entity);
        WRITE(!error_check(members));
        EntityGroupResult group = entity_group_get(members.result.value);
        WRITE(!error_check(group));
        yyjson_mut_val *array = yyjson_mut_arr(doc);
        WRITE(array);
        const GroupEntityPool *pool = &group.result.value.entities;
        for(size_t n = 0; n < pool->capacity; ++n) {
            if(!pool->used[n]) continue;
            EntityNameResult name = entity_object_member_name_get(pool->objects[n]);
            yyjson_mut_val *entry = yyjson_mut_obj(doc);
            WRITE(!error_check(name) && entry &&
                reference_write(doc, entry, "entity", pool->objects[n]) &&
                yyjson_mut_obj_add_strcpy(doc, entry, "name", name.result.value.value) &&
                yyjson_mut_arr_add_val(array, entry));
        }
        WRITE(yyjson_mut_obj_add_val(doc, components, "object", array));
        size_t count;
        const ObjectMemberAlias *aliases = entity_object_aliases_get(entity, &count);
        if(count) {
            array = yyjson_mut_arr(doc); WRITE(array);
            for(size_t i = 0; i < count; ++i) {
                yyjson_mut_val *entry = yyjson_mut_obj(doc);
                WRITE(entry && reference_write(doc, entry, "entity", aliases[i].entity) &&
                    yyjson_mut_obj_add_strcpy(doc, entry, "name", aliases[i].name.value) &&
                    yyjson_mut_arr_add_val(array, entry));
            }
            WRITE(yyjson_mut_obj_add_val(doc, components, "object_aliases", array));
        }
    }
    if(soft_bodies_pool.used[index]) {
        const SoftBody *value = &soft_bodies[index];
        WRITE(value->origin == entity && value->node_count <= SOFT_BODY_MAX_NODES &&
            value->beam_count <= SOFT_BODY_MAX_BEAMS && value->area_count <= SOFT_BODY_MAX_AREAS);
        yyjson_mut_val *v = yyjson_mut_obj(doc);
        WRITE(v && yyjson_mut_obj_add_val(doc, v, "nodes", list_write(doc, value->nodes, value->node_count)) &&
            yyjson_mut_obj_add_val(doc, v, "beams", list_write(doc, value->beams, value->beam_count)) &&
            yyjson_mut_obj_add_val(doc, v, "areas", list_write(doc, value->areas, value->area_count)) &&
            yyjson_mut_obj_add_val(doc, components, "soft_body", v));
    }
    if(soft_body_nodes_pool.used[index]) {
        const SoftBodyNode *value = &soft_body_nodes[index];
        yyjson_mut_val *v = yyjson_mut_obj(doc);
        WRITE(v &&
            yyjson_mut_obj_add_real(doc, v, "radius", value->radius) &&
            color_write(doc, v, value->draw_color, value->draw_color_overridden) &&
            yyjson_mut_obj_add_val(doc, components, "soft_body_node", v));
    }
    if(soft_body_beams_pool.used[index]) {
        const SoftBodyBeam *value = &soft_body_beams[index];
        yyjson_mut_val *v = yyjson_mut_obj(doc);
        WRITE(v &&
            reference_write(doc, v, "node_a", value->node_a) && reference_write(doc, v, "node_b", value->node_b));
#define BEAM_WRITE(field) WRITE(yyjson_mut_obj_add_real(doc, v, #field, value->field))
        BEAM_WRITE(rest_length); BEAM_WRITE(stiffness); BEAM_WRITE(damping);
        BEAM_WRITE(collision_thickness); BEAM_WRITE(friction); BEAM_WRITE(restitution);
#undef BEAM_WRITE
        WRITE(yyjson_mut_obj_add_bool(doc, v, "collision_enabled", value->collision_enabled) &&
            yyjson_mut_obj_add_uint(doc, v, "category", value->category) &&
            yyjson_mut_obj_add_uint(doc, v, "collides_with", value->collides_with) &&
            color_write(doc, v, value->draw_color, value->draw_color_overridden) &&
            yyjson_mut_obj_add_val(doc, components, "soft_body_beam", v));
    }
    if(soft_body_area_states_pool.used[index]) {
        const SoftBodyArea *value = &soft_body_area_states[index].value;
        yyjson_mut_val *v = yyjson_mut_obj(doc), *holes = yyjson_mut_arr(doc);
        WRITE(value->geometry.hole_count <= SOFT_BODY_MAX_AREA_HOLES && v && holes &&
            yyjson_mut_obj_add_val(doc, v, "outer", list_write(doc, value->geometry.outer.nodes, value->geometry.outer.node_count)));
        for(uint32_t h = 0; h < value->geometry.hole_count; ++h)
            WRITE(yyjson_mut_arr_add_val(holes, list_write(doc, value->geometry.holes[h].nodes, value->geometry.holes[h].node_count)));
        WRITE(yyjson_mut_obj_add_val(doc, v, "holes", holes) &&
            yyjson_mut_obj_add_bool(doc, v, "visible", value->visible) &&
            color_write(doc, v, value->draw_color, value->draw_color_overridden) &&
            yyjson_mut_obj_add_val(doc, components, "soft_body_area", v));
    }
    JointAnchorListResult anchors = physics_joint_anchors_get(entity);
    WRITE(!error_check(anchors));
    if(anchors.result.value.count) {
        yyjson_mut_val *array = yyjson_mut_arr(doc);
        WRITE(array);
        for(uint32_t a = 0; a < anchors.result.value.count; ++a) {
            JointAnchorPositionResult p = physics_joint_anchor_local_position_get(anchors.result.value.values[a]);
            yyjson_mut_val *v = yyjson_mut_obj(doc);
            WRITE(!error_check(p) && v && yyjson_mut_obj_add_real(doc, v, "x", p.result.value.x) &&
                yyjson_mut_obj_add_real(doc, v, "y", p.result.value.y) && yyjson_mut_arr_add_val(array, v));
        }
        WRITE(yyjson_mut_obj_add_val(doc, components, "joint_anchors", array));
    }
#undef WRITE
    return true;
}

bool state_joint_anchor_read(yyjson_val *value, Entity owner, JointAnchorId *anchor) {
    *anchor = JOINT_ANCHOR_INVALID;
    if(value == NULL || yyjson_is_null(value)) return true;
    JointAnchorListResult list = physics_joint_anchors_get(owner);
    if(!yyjson_is_uint(value) || error_check(list) || yyjson_get_uint(value) >= list.result.value.count) return false;
    *anchor = list.result.value.values[yyjson_get_uint(value)];
    return true;
}
bool state_joint_anchor_write(yyjson_mut_doc *doc, yyjson_mut_val *object,
        const char *key, Entity owner, JointAnchorId anchor) {
    if(anchor == JOINT_ANCHOR_INVALID) return yyjson_mut_obj_add_null(doc, object, key);
    JointAnchorListResult list = physics_joint_anchors_get(owner);
    if(error_check(list)) return false;
    for(uint32_t a = 0; a < list.result.value.count; ++a)
        if(list.result.value.values[a] == anchor) return yyjson_mut_obj_add_uint(doc, object, key, a);
    return false;
}
