/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics.h"
#include "entity/entity_internal.h"

#include "physics/physics_internal.h"

#include <string.h>
#include <math.h>
#include <float.h>
#include "physics/physics_operation.h"

static JointAnchor joint_anchors[MAX_JOINT_ANCHORS];
static uint32_t joint_anchor_generations[MAX_JOINT_ANCHORS];
static bool joint_anchor_used[MAX_JOINT_ANCHORS];

static JointAnchorId physics_joint_anchor_id_make(uint32_t slot) {
    return ((uint64_t)joint_anchor_generations[slot] << 32) |
        ((uint64_t)slot + 1);
}

static bool physics_joint_anchor_slot_get(
        JointAnchorId anchor, uint32_t *slot) {
    uint32_t candidate;
    uint32_t generation;

    if(anchor == JOINT_ANCHOR_INVALID || slot == NULL) return false;
    candidate = (uint32_t)(anchor & UINT32_MAX);
    generation = (uint32_t)(anchor >> 32);
    if(candidate == 0) return false;
    candidate -= 1;
    if(candidate >= MAX_JOINT_ANCHORS || !joint_anchor_used[candidate] ||
            joint_anchor_generations[candidate] != generation) return false;
    *slot = candidate;
    return true;
}

void physics_joint_state_init(void) {
    memset(joint_anchors, 0, sizeof(joint_anchors));
    memset(joint_anchor_used, 0, sizeof(joint_anchor_used));
    for(uint32_t i = 0; i < MAX_JOINT_ANCHORS; i += 1)
        joint_anchor_generations[i] = 1;
}

void physics_joint_entity_clear(Entity entity, EntityIndex index) {
    for(uint32_t slot = 0; slot < MAX_JOINT_ANCHORS; slot += 1) {
        if(joint_anchor_used[slot] && joint_anchors[slot].entity == entity)
            (void)physics_joint_anchor_remove(
                physics_joint_anchor_id_make(slot));
    }
    for(EntityIndex joint_index = 0;
            joint_index < joints_pool.capacity;
            joint_index += 1) {
        if(!joints_pool.used[joint_index] ||
                (joints[joint_index].a != entity &&
                    joints[joint_index].b != entity)) continue;
        if(joints[joint_index].a == entity) {
            joints[joint_index].a = ENTITY_INVALID;
            joints[joint_index].anchor_a = JOINT_ANCHOR_INVALID;
        }
        if(joints[joint_index].b == entity) {
            joints[joint_index].b = ENTITY_INVALID;
            joints[joint_index].anchor_b = JOINT_ANCHOR_INVALID;
        }
    }
    if(index < joints_pool.capacity && joints_pool.used[index])
        (void)JointPool_release_at(&joints_pool, index);
}

EngineResult physics_joint_component_restore(Entity entity, Joint joint) {
    EntityIndex index;
    EntityIndex a_index;
    EntityIndex b_index;
    EngineResult result = physics_live_index_get(entity, &index);

    if(result.kind == ERROR_RESULT_ERROR) {
        return result;
    }
    if(!physics_vector_finite_check(joint.local_anchor_a) || !physics_vector_finite_check(joint.local_anchor_b) ||
            !isfinite(joint.rest_length) || !isfinite(joint.stiffness) || !isfinite(joint.damping) ||
            !isfinite(joint.rest_angle) || !isfinite(joint.angular_stiffness) || !isfinite(joint.angular_damping))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(joint.rest_length < 0 || joint.stiffness < 0 || joint.damping < 0)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if((joint.a != ENTITY_INVALID) != (joint.anchor_a != JOINT_ANCHOR_INVALID) ||
            (joint.b != ENTITY_INVALID) != (joint.anchor_b != JOINT_ANCHOR_INVALID))
        return error_result_error_detail(ERROR_ENGINE_STATE_INVALID,
            "Joint body-local attachments are unsupported; connect each body through an anchor");
    if(joint.type < JOINT_SPRING || joint.type > JOINT_PIN) {
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    }
    if(joint.a != ENTITY_INVALID && error_check(result = physics_live_index_get(joint.a, &a_index))) return result;
    if(joint.b != ENTITY_INVALID && error_check(result = physics_live_index_get(joint.b, &b_index))) return result;
    uint32_t slot;
    if((joint.anchor_a != JOINT_ANCHOR_INVALID &&
            (!physics_joint_anchor_slot_get(joint.anchor_a, &slot) || joint_anchors[slot].entity != joint.a)) ||
       (joint.anchor_b != JOINT_ANCHOR_INVALID &&
            (!physics_joint_anchor_slot_get(joint.anchor_b, &slot) || joint_anchors[slot].entity != joint.b)))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(JointPool_store_at(&joints_pool, index, joint).kind
            == ERROR_RESULT_ERROR) {
        return error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    entity_mask[index] |= ROHR_JOINT;
    return error_result_value(true);
}

JointAnchorIdResult physics_joint_anchor_add(JointAnchorDescription description) {
    Entity entity = description.entity;
    Vec2D local_offset = description.local_offset;

    EntityIndex entity_index;
    uint32_t owned_count = 0;
    EngineResult result = physics_live_index_get(entity, &entity_index);

    if(result.kind == ERROR_RESULT_ERROR) {
        return ERROR_RESULT_MAKE_ERROR(JointAnchorIdResult, result.result.error);
    }
    if(!physics_world_position_check(local_offset))
        return ERROR_RESULT_MAKE_ERROR(JointAnchorIdResult, ERROR_ENGINE_POSITION_OUT_OF_RANGE);
    for(uint32_t slot = 0; slot < MAX_JOINT_ANCHORS; slot += 1) {
        if(joint_anchor_used[slot] && joint_anchors[slot].entity == entity) owned_count += 1;
    }
    if(owned_count >= MAX_JOINT_ANCHORS_PER_ENTITY) {
        return ERROR_RESULT_MAKE_ERROR(JointAnchorIdResult, ERROR_ENGINE_MAX_ENTITIES_EXCEEDED);
    }
    for(uint32_t slot = 0; slot < MAX_JOINT_ANCHORS; slot += 1) {
        if(joint_anchor_used[slot]) continue;
        joint_anchor_used[slot] = true;
        joint_anchors[slot] = (JointAnchor){
            .entity = entity,
            .local_offset = local_offset
        };
        return ERROR_RESULT_MAKE_VALUE(JointAnchorIdResult, physics_joint_anchor_id_make(slot));
    }
    return ERROR_RESULT_MAKE_ERROR(JointAnchorIdResult, ERROR_ENGINE_MAX_ENTITIES_EXCEEDED);
}

JointAnchorDescriptionResult physics_joint_anchor_description_get(JointAnchorId anchor) {
    uint32_t slot;
    if(!physics_joint_anchor_slot_get(anchor, &slot))
        return ERROR_RESULT_MAKE_ERROR(JointAnchorDescriptionResult, ERROR_ENGINE_ENTITY_NOT_FOUND);
    return ERROR_RESULT_MAKE_VALUE(JointAnchorDescriptionResult, ((JointAnchorDescription){
        .entity = joint_anchors[slot].entity, .local_offset = joint_anchors[slot].local_offset}));
}

EngineResult physics_joint_anchor_description_set(JointAnchorId anchor, JointAnchorDescription description) {
    uint32_t slot;
    if(!physics_joint_anchor_slot_get(anchor, &slot)) return error_result_error(ERROR_ENGINE_ENTITY_NOT_FOUND);
    EntityIndex index;
    EngineResult result = physics_live_index_get(description.entity, &index);
    if(error_check(result)) return result;
    if(!physics_world_position_check(description.local_offset)) return error_result_error(ERROR_ENGINE_POSITION_OUT_OF_RANGE);
    if(description.entity != joint_anchors[slot].entity) {
        uint32_t count = 0;
        for(uint32_t i = 0; i < MAX_JOINT_ANCHORS; ++i)
            if(joint_anchor_used[i] && joint_anchors[i].entity == description.entity) ++count;
        if(count >= MAX_JOINT_ANCHORS_PER_ENTITY) return error_result_error(ERROR_ENGINE_MAX_ENTITIES_EXCEEDED);
    }
    joint_anchors[slot] = (JointAnchor){.entity = description.entity, .local_offset = description.local_offset};
    for(EntityIndex i = 0; i < joints_pool.capacity; ++i) {
        if(!joints_pool.used[i]) continue;
        if(joints[i].anchor_a == anchor) joints[i].a = description.entity;
        if(joints[i].anchor_b == anchor) joints[i].b = description.entity;
    }
    return error_result_value(true);
}

JointAnchorListResult physics_joint_anchors_get(Entity entity) {
    EntityIndex entity_index;
    JointAnchorList list = {0};
    EngineResult result = physics_live_index_get(entity, &entity_index);

    if(result.kind == ERROR_RESULT_ERROR) {
        return ERROR_RESULT_MAKE_ERROR(JointAnchorListResult, result.result.error);
    }
    for(uint32_t slot = 0; slot < MAX_JOINT_ANCHORS; slot += 1) {
        if(!joint_anchor_used[slot] || joint_anchors[slot].entity != entity) continue;
        if(list.count >= MAX_JOINT_ANCHORS_PER_ENTITY) {
            return ERROR_RESULT_MAKE_ERROR(JointAnchorListResult, ERROR_ENGINE_MAX_ENTITIES_EXCEEDED);
        }
        list.values[list.count++] = physics_joint_anchor_id_make(slot);
    }
    return ERROR_RESULT_MAKE_VALUE(JointAnchorListResult, list);
}

JointAnchorPositionResult physics_joint_anchor_local_position_get(JointAnchorId anchor) {
    uint32_t slot;

    if(!physics_joint_anchor_slot_get(anchor, &slot)) {
        return ERROR_RESULT_MAKE_ERROR(JointAnchorPositionResult, ERROR_ENGINE_ENTITY_NOT_FOUND);
    }
    return ERROR_RESULT_MAKE_VALUE(JointAnchorPositionResult, joint_anchors[slot].local_offset);
}

JointAnchorPositionResult physics_joint_anchor_world_position_get(JointAnchorId anchor) {
    uint32_t slot;
    EntityIndex entity_index;
    Vec2D local_position;
    Vec2D rotated;

    if(!physics_joint_anchor_slot_get(anchor, &slot) ||
            !entity_index_get(joint_anchors[slot].entity, &entity_index) ||
            !entity_index_alive_check(entity_index)) {
        return ERROR_RESULT_MAKE_ERROR(JointAnchorPositionResult, ERROR_ENGINE_ENTITY_NOT_FOUND);
    }
    local_position = joint_anchors[slot].local_offset;
    rotated = math_vector_rotate(local_position, orientations[entity_index]);
    return ERROR_RESULT_MAKE_VALUE(JointAnchorPositionResult, ((Position){
        .x = positions[entity_index].x + rotated.x,
        .y = positions[entity_index].y + rotated.y
    }));
}

EngineResult physics_joint_anchor_local_position_x_set(JointAnchorId anchor, float x) {
    JointAnchorPositionResult current = physics_joint_anchor_local_position_get(anchor);
    if(current.kind == ERROR_RESULT_ERROR)
        return error_result_error(current.result.error);
    Position value = current.result.value;
    value.x = x;
    return physics_joint_anchor_local_position_set(anchor, value);
}

EngineResult physics_joint_anchor_local_position_y_set(JointAnchorId anchor, float y) {
    JointAnchorPositionResult current = physics_joint_anchor_local_position_get(anchor);
    if(current.kind == ERROR_RESULT_ERROR)
        return error_result_error(current.result.error);
    Position value = current.result.value;
    value.y = y;
    return physics_joint_anchor_local_position_set(anchor, value);
}

EngineResult physics_joint_anchor_local_position_set(JointAnchorId anchor, Vec2D local_offset) {
    uint32_t slot;

    if(!physics_world_position_check(local_offset))
        return error_result_error(ERROR_ENGINE_POSITION_OUT_OF_RANGE);
    if(!physics_joint_anchor_slot_get(anchor, &slot)) {
        return error_result_error(ERROR_ENGINE_ENTITY_NOT_FOUND);
    }
    joint_anchors[slot].local_offset = local_offset;
    return error_result_value(true);
}

EngineResult physics_joint_anchor_remove(JointAnchorId anchor) {
    uint32_t slot;

    if(!physics_joint_anchor_slot_get(anchor, &slot)) {
        return error_result_error(ERROR_ENGINE_ENTITY_NOT_FOUND);
    }
    for(EntityIndex index = 0; index < joints_pool.capacity; index += 1) {
        if(!joints_pool.used[index] ||
                (joints[index].anchor_a != anchor && joints[index].anchor_b != anchor)) continue;
        if(joints[index].anchor_a == anchor) {
            joints[index].anchor_a = JOINT_ANCHOR_INVALID;
            joints[index].a = ENTITY_INVALID;
        }
        if(joints[index].anchor_b == anchor) {
            joints[index].anchor_b = JOINT_ANCHOR_INVALID;
            joints[index].b = ENTITY_INVALID;
        }
    }
    joint_anchor_used[slot] = false;
    joint_anchors[slot] = (JointAnchor){0};
    joint_anchor_generations[slot] += 1;
    if(joint_anchor_generations[slot] == 0) joint_anchor_generations[slot] = 1;
    return error_result_value(true);
}

static EngineResult joint_description_resolve(JointDescription description, Joint *joint) {
    if(description.type < JOINT_SPRING || description.type > JOINT_PIN ||
            !isfinite(description.rest_length) || description.rest_length < 0 ||
            !isfinite(description.stiffness) || description.stiffness < 0 ||
            !isfinite(description.damping) || description.damping < 0 ||
            !isfinite(description.rest_angle))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    *joint = (Joint){.type = description.type,
        .anchor_a = description.anchor_a, .anchor_b = description.anchor_b,
        .stiffness = description.stiffness, .damping = description.damping,
        .rest_length = description.rest_length_explicit ? description.rest_length : 0,
        .rest_angle = description.rest_angle_explicit ? description.rest_angle : 0};
    JointAnchorId anchors[] = {description.anchor_a, description.anchor_b};
    Entity entities[] = {ENTITY_INVALID, ENTITY_INVALID};
    EntityIndex indices[2] = {0};
    for(size_t i = 0; i < 2; ++i) {
        if(anchors[i] == JOINT_ANCHOR_INVALID) continue;
        uint32_t slot;
        if(!physics_joint_anchor_slot_get(anchors[i], &slot))
            return error_result_error(ERROR_ENGINE_ENTITY_NOT_FOUND);
        entities[i] = joint_anchors[slot].entity;
        EngineResult result = physics_live_index_get(entities[i], &indices[i]);
        if(error_check(result)) return result;
    }
    joint->a = entities[0]; joint->b = entities[1];
    if(entities[0] != ENTITY_INVALID && entities[1] != ENTITY_INVALID) {
        if(!description.rest_length_explicit) {
            JointAnchorPositionResult a = physics_joint_anchor_world_position_get(anchors[0]);
            JointAnchorPositionResult b = physics_joint_anchor_world_position_get(anchors[1]);
            if(error_check(a)) return error_result_error(a.result.error);
            if(error_check(b)) return error_result_error(b.result.error);
            double length = hypot((double)b.result.value.x - a.result.value.x,
                (double)b.result.value.y - a.result.value.y);
            if(!isfinite(length) || length > FLT_MAX) return error_result_error(ERROR_ENGINE_STATE_INVALID);
            joint->rest_length = (float)length;
        }
        if(!description.rest_angle_explicit) {
            double angle = (double)orientations[indices[1]] - orientations[indices[0]];
            if(!isfinite(angle) || fabs(angle) > FLT_MAX) return error_result_error(ERROR_ENGINE_STATE_INVALID);
            joint->rest_angle = (Orientation)angle;
        }
    }
    return error_result_value(true);
}

EngineResult physics_joint_description_set(Entity entity, JointDescription description) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    if(error_check(result)) return result;
    Joint joint;
    result = joint_description_resolve(description, &joint);
    if(error_check(result)) return result;
    /* Entity-indexed pools are reserved ahead of time. Check storage before
     * transferring ownership, the only fallible mutation, just like forces. */
    if(!joints_pool.objects || !joints_pool.used || index >= joints_pool.capacity)
        return error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    result = entity_owner_set(entity, description.owner);
    if(error_check(result)) return result;
    (void)JointPool_store_at(&joints_pool, index, joint);
    entity_mask[index] |= ROHR_JOINT;
    return error_result_value(true);
}

JointDescriptionResult physics_joint_description_get(Entity entity) {
    EntityIndex index;
    EngineResult result = physics_live_index_get(entity, &index);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(JointDescriptionResult, result.result.error);
    if(!entity_index_components_check(index, ROHR_JOINT) || !joints_pool.used[index])
        return ERROR_RESULT_MAKE_ERROR(JointDescriptionResult, ERROR_ENGINE_COMPONENT_MISSING);
    EntityResult owner = entity_owner_get(entity);
    if(error_check(owner)) return ERROR_RESULT_MAKE_ERROR(JointDescriptionResult, owner.result.error);
    Joint joint = joints[index];
    JointDescription description = {.owner = owner.result.value, .type = joint.type,
        .anchor_a = joint.anchor_a, .anchor_b = joint.anchor_b,
        .rest_length = joint.rest_length, .stiffness = joint.stiffness, .damping = joint.damping,
        .rest_angle = joint.rest_angle, .rest_length_explicit = true, .rest_angle_explicit = true};
    return ERROR_RESULT_MAKE_VALUE(JointDescriptionResult, description);
}

EntityResult physics_joint_add(JointDescription description) {
    Joint resolved;
    EngineResult result = joint_description_resolve(description, &resolved);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    if(description.owner != ENTITY_INVALID && !entity_alive_check(description.owner))
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_INVALID_ENTITY);
    EntityResult joint = entity_default_create("joint", ENTITY_INVALID);
    if(error_check(joint)) return joint;
    result = physics_joint_description_set(joint.result.value, description);
    if(error_check(result)) {
        (void)entity_delete(joint.result.value);
        return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    }
    return joint;
}
