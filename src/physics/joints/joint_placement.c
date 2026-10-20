/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics.h"
#include "physics/physics_internal.h"
#include "physics/soft_body/soft_body_frame.h"
#include <math.h>
#include <stdlib.h>

typedef struct PlacementPose {
    Position position;
    Orientation orientation;
    bool selected;
    bool attachment;
} PlacementPose;

static bool placement_position_equal(Position a, Position b) {
    return hypot((double)a.x - b.x, (double)a.y - b.y) <= 0.0001;
}

static bool placement_angle_equal(float a, float b) {
    return fabs(remainder((double)a - b, 360.0)) <= 0.0001;
}

static bool placement_select(PlacementPose *poses, Entity entity) {
    EntityIndex index;
    if(!entity_index_get(entity, &index) || poses[index].selected) return false;
    poses[index].selected = true;
    return true;
}

static bool placement_anchor_get(const PlacementPose *poses, JointAnchorId anchor,
        EntityIndex *index, Position *position) {
    JointAnchorDescriptionResult result = physics_joint_anchor_description_get(anchor);
    if(error_check(result) || !entity_index_get(result.result.value.entity, index) ||
            !positions_pool.used[*index]) return false;
    Vec2D offset = math_vector_rotate(result.result.value.local_offset,
        poses[*index].orientation);
    *position = (Position){poses[*index].position.x + offset.x,
        poses[*index].position.y + offset.y};
    return physics_world_position_check(*position);
}

static bool placement_locks_check(const PlacementPose *poses, EntityIndex index) {
    PlacementPose pose = poses[index];
    if(pose.selected) {
        bool moved = !placement_position_equal(pose.position, positions[index]) ||
            !placement_angle_equal(pose.orientation, orientations[index]);
        if(moved && (entity_index_components_check(index, ROHR_STATIC) ||
                entity_index_components_check(index, ROHR_HOLD))) return false;
        if(entity_index_components_check(index, ROHR_PARTICLE) &&
                particle_geometries_pool.used[index] && particle_geometries_pool.objects[index].standalone &&
                !placement_angle_equal(pose.orientation, orientations[index])) return false;
        if(!physics_world_position_check(pose.position) || !isfinite(pose.orientation))
            return false;
        if(entity_index_components_check(index, ROHR_AXIS_LOCK)) {
            AxisLock lock = axis_locks[index];
            double dx = (double)pose.position.x - lock.point_on_axis.x;
            double dy = (double)pose.position.y - lock.point_on_axis.y;
            if(fabs(dx * lock.axis.y - dy * lock.axis.x) > 0.0001) return false;
        }
        if(entity_index_components_check(index, ROHR_ANGLE_LOCK)) {
            float min = fminf(angle_locks[index].min, angle_locks[index].max);
            float max = fmaxf(angle_locks[index].min, angle_locks[index].max);
            if(pose.orientation < min - 0.0001f || pose.orientation > max + 0.0001f)
                return false;
        }
    }
    if(entity_index_components_check(index, ROHR_TRANSFORM_LOCK)) {
        TransformLock lock = transform_locks[index];
        EntityIndex driver;
        if(!entity_index_get(lock.driver, &driver)) return !pose.selected;
        if(!pose.selected && !poses[driver].selected) return true;
        Vec2D offset = math_vector_rotate(lock.local_offset, poses[driver].orientation);
        Position expected = {poses[driver].position.x + offset.x,
            poses[driver].position.y + offset.y};
        if(lock.lock_position && !placement_position_equal(pose.position, expected))
            return false;
        if(lock.lock_orientation && !placement_angle_equal(pose.orientation,
                poses[driver].orientation + lock.local_angle)) return false;
    }
    return true;
}

EngineResult physics_joint_place(Entity entity) {
    EntityIndex joint_index;
    EngineResult result = physics_live_index_get(entity, &joint_index);
    if(error_check(result)) return result;
    if(!joints_pool.used[joint_index])
        return error_result_error(ERROR_ENGINE_COMPONENT_MISSING);
    Joint joint = joints[joint_index];
    /* Temporary scratch storage is acquired before any mutation. Every failure
     * releases it, and commit only writes existing component slots. */
    size_t capacity = entity_mask_pool.capacity;
    PlacementPose *poses = calloc(capacity, sizeof(*poses));
    if(!poses) return error_result_error(ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    result = error_result_error(ERROR_ENGINE_STATE_INVALID);
    for(EntityIndex i = 0; i < capacity; ++i) {
        if(!entity_index_alive_check(i)) continue;
        poses[i].position = positions[i];
        poses[i].orientation = orientations[i];
    }
    EntityIndex a, b;
    Position anchor_a, anchor_b;
    if(!placement_anchor_get(poses, joint.anchor_a, &a, &anchor_a) ||
            !placement_anchor_get(poses, joint.anchor_b, &b, &anchor_b)) goto done;
    float rotation = joint.type == JOINT_WELD ?
        (float)remainder((double)poses[b].orientation - joint.rest_angle -
            poses[a].orientation, 360.0) : 0;
    Position destination = anchor_b;
    if(joint.type == JOINT_SPRING) {
        double dx = (double)anchor_a.x - anchor_b.x;
        double dy = (double)anchor_a.y - anchor_b.y;
        double distance = hypot(dx, dy);
        if(distance == 0) { dx = 0; dy = 1; distance = 1; }
        destination.x += (float)(dx / distance * joint.rest_length);
        destination.y += (float)(dy / distance * joint.rest_length);
    }
    if(placement_position_equal(anchor_a, destination) &&
            placement_angle_equal(rotation, 0)) {
        result = error_result_value(true);
        goto done;
    }
    poses[a].selected = true;
    bool changed;
    do {
        changed = false;
        for(EntityIndex i = 0; i < capacity; ++i) {
            if(!entity_index_alive_check(i)) continue;
            if(poses[i].selected && soft_body_nodes_pool.used[i])
                changed |= placement_select(poses, soft_body_nodes[i].soft_body);
            if(poses[i].selected && soft_bodies_pool.used[i]) {
                for(uint32_t n = 0; n < soft_bodies[i].node_count; ++n)
                    changed |= placement_select(poses, soft_bodies[i].nodes[n]);
            }
            /* Owned attachment helpers follow their driver as part of the
             * same object. Unrelated transform-locked entities remain fixed
             * and are checked below, so external constraints cannot be broken. */
            if(!poses[i].selected && transform_locks_pool.used[i]) {
                EntityIndex driver;
                EntityResult current = entity_from_index_get(i);
                EntityResult owner = error_check(current) ? current :
                    entity_owner_get(current.result.value);
                if(!error_check(owner) && owner.result.value == transform_locks[i].driver &&
                        entity_index_get(owner.result.value, &driver) && poses[driver].selected) {
                    poses[i].selected = true;
                    poses[i].attachment = true;
                    changed = true;
                }
            }
            if(i == joint_index || !joints_pool.used[i] ||
                    joints[i].type == JOINT_SPRING) continue;
            EntityIndex first, second;
            if(!entity_index_get(joints[i].a, &first) ||
                    !entity_index_get(joints[i].b, &second)) continue;
            if(poses[first].selected)
                changed |= placement_select(poses, joints[i].b);
            if(poses[second].selected)
                changed |= placement_select(poses, joints[i].a);
        }
    } while(changed);
    /* A second path to B would move the supposedly stationary side. */
    if(poses[b].selected) goto done;
    for(EntityIndex i = 0; i < capacity; ++i) {
        if(!poses[i].selected) continue;
        if(!positions_pool.used[i] || !orientations_pool.used[i]) goto done;
        Vec2D offset = math_vector_rotate((Vec2D){
            poses[i].position.x - anchor_a.x, poses[i].position.y - anchor_a.y}, rotation);
        poses[i].position = (Position){destination.x + offset.x, destination.y + offset.y};
        poses[i].orientation += rotation;
        if(poses[i].attachment) {
            if(!transform_locks[i].lock_position) poses[i].position = positions[i];
            if(!transform_locks[i].lock_orientation) poses[i].orientation = orientations[i];
        }
    }
    for(EntityIndex i = 0; i < capacity; ++i) {
        if(!entity_index_alive_check(i)) continue;
        if(!placement_locks_check(poses, i)) goto done;
        if(!joints_pool.used[i] || joints[i].type == JOINT_SPRING) continue;
        EntityIndex first, second;
        Position first_point, second_point;
        if(!entity_index_get(joints[i].a, &first) ||
                !entity_index_get(joints[i].b, &second) ||
                (!poses[first].selected && !poses[second].selected)) continue;
        if(!placement_anchor_get(poses, joints[i].anchor_a, &first, &first_point) ||
                !placement_anchor_get(poses, joints[i].anchor_b, &second, &second_point) ||
                !placement_position_equal(first_point, second_point)) goto done;
        if(joints[i].type == JOINT_WELD && !placement_angle_equal(
                poses[second].orientation - poses[first].orientation,
                joints[i].rest_angle)) goto done;
    }
    for(EntityIndex i = 0; i < capacity; ++i) {
        if(!poses[i].selected) continue;
        positions[i] = poses[i].position;
        orientations[i] = poses[i].orientation;
    }
    for(EntityIndex i = 0; i < capacity; ++i)
        if(poses[i].selected && soft_bodies_pool.used[i]) physics_soft_body_frame_reset(i);
    result = error_result_value(true);
done:
    free(poses);
    return result;
}
