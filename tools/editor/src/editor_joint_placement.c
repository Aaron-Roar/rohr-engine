/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_project.h"
#include <math.h>
#include <stdlib.h>

typedef struct EditorPlacementPose {
    ObjectEntityReference reference;
    Position before, position;
    Orientation before_angle, angle;
    const ObjectEntitySettings *settings;
    size_t group, attachment;
    bool selected, fixed, rotation_locked;
} EditorPlacementPose;

static bool same_position(Position a, Position b) {
    return hypot((double)a.x - b.x, (double)a.y - b.y) <= .0001;
}
static bool same_angle(float a, float b) {
    return fabs(remainder((double)a - b, 360)) <= .0001;
}
static size_t pose_find(const EditorPlacementPose *poses, size_t count, ObjectEntityReference reference) {
    if(!reference.id) return SIZE_MAX;
    for(size_t i = 0; i < count; ++i)
        if(poses[i].reference.id == reference.id &&
                (poses[i].reference.kind == reference.kind ||
                    (poses[i].reference.kind == OBJECT_BODY && reference.kind == OBJECT_PARTICLE)) &&
                poses[i].reference.soft_body == reference.soft_body) return i;
    return SIZE_MAX;
}
static Position offset_position(Position base, Position offset, float rotation) {
    Position rotated = math_vector_rotate(offset, rotation);
    return (Position){base.x + rotated.x, base.y + rotated.y};
}
/* Records outside the moved physical group must not acquire a different
 * transform through a generic lock. Ordinary owned visual attachments follow
 * their owner automatically and retain their authored local configuration. */
static bool external_lock_check(const EditorPlacementPose *poses, size_t count,
        const ObjectEntitySettings *settings, ObjectEntityReference owner) {
    if(!settings->transform_driver.id) return true;
    size_t driver = pose_find(poses, count, settings->transform_driver);
    if(driver == SIZE_MAX || !poses[driver].selected) return true;
    if(owner.id && pose_find(poses, count, owner) == driver) return true;
    const EditorPlacementPose *pose = &poses[driver];
    if(settings->transform_position && !same_position(
            offset_position(pose->before, settings->transform_offset, pose->before_angle),
            offset_position(pose->position, settings->transform_offset, pose->angle))) return false;
    return !settings->transform_orientation || same_angle(pose->before_angle, pose->angle);
}
static size_t anchor_owner(const EditorObject *object, const EditorPlacementPose *poses,
        size_t count, EditorAnchorId id, Position *position) {
    const EditorAnchor *anchor = NULL;
    for(size_t i = 0; i < object->anchor_count; ++i)
        if(object->anchors[i].id == id) anchor = &object->anchors[i];
    if(!anchor) return SIZE_MAX;
    ObjectEntityReference reference = {OBJECT_ANCHOR, id, 0};
    if(anchor->position_follows_body) {
        if(anchor->attachment_kind == EDITOR_ANCHOR_ATTACHMENT_SOFT_NODE)
            reference = (ObjectEntityReference){OBJECT_NODE, anchor->attachment_soft_node, anchor->attachment_soft_body};
        else if(anchor->rigid_body)
            reference = (ObjectEntityReference){OBJECT_BODY, anchor->rigid_body, 0};
    }
    size_t owner = pose_find(poses, count, reference);
    if(owner == SIZE_MAX) return owner;
    *position = reference.kind == OBJECT_ANCHOR ? poses[owner].position :
        offset_position(poses[owner].position, anchor->position, poses[owner].angle);
    return owner;
}

bool editor_project_joint_place(EditorObject *object, EditorJoint *joint) {
    if(!object || !joint) return false;
    size_t count = 1 + object->rigid_body_count + object->soft_body_count + object->anchor_count;
    for(size_t i = 0; i < object->soft_body_count; ++i) count += object->soft_body_items[i].node_count;
    EditorPlacementPose *poses = calloc(count, sizeof(*poses));
    if(!poses) return false;
    bool result = false;
    size_t cursor = 0;
#define ADD(kind_value, id_value, parent_value, point, rotation, setting, group_value, fixed_value, locked_value) \
    poses[cursor++] = (EditorPlacementPose){.reference = {kind_value, id_value, parent_value}, \
        .before = point, .position = point, .before_angle = rotation, .angle = rotation, \
        .settings = setting, .group = group_value, .attachment = SIZE_MAX, .fixed = fixed_value, .rotation_locked = locked_value}
    ADD(OBJECT_CONTAINER, object->id, 0, ((Position){0}), 0, &object->entity_settings, SIZE_MAX, true, true);
    for(size_t i = 0; i < object->rigid_body_count; ++i) {
        EditorRigidBody *body = &object->rigid_bodies[i];
        ADD(OBJECT_BODY, body->id, 0, body->position, body->rotation,
            &body->entity_settings, SIZE_MAX, body->static_body, body->rotation_locked || body->standalone_particle);
    }
    for(size_t i = 0; i < object->soft_body_count; ++i) {
        EditorSoftBody *body = &object->soft_body_items[i];
        size_t group = cursor;
        ADD(OBJECT_SOFT_BODY, body->id, 0, body->position, body->rotation,
            &body->entity_settings, group, false, false);
        for(size_t n = 0; n < body->node_count; ++n) {
            EditorSoftNode *node = &body->nodes[n];
            Position position = offset_position(body->position, node->position, body->rotation);
            ADD(OBJECT_NODE, node->id, body->id, position, body->rotation,
                &node->entity_settings, group, node->static_body, false);
        }
    }
    for(size_t i = 0; i < object->anchor_count; ++i) {
        EditorAnchor *anchor = &object->anchors[i];
        ObjectEntityReference reference = anchor->attachment_kind == EDITOR_ANCHOR_ATTACHMENT_SOFT_NODE ?
            (ObjectEntityReference){OBJECT_NODE, anchor->attachment_soft_node, anchor->attachment_soft_body} :
            (ObjectEntityReference){OBJECT_BODY, anchor->rigid_body, 0};
        size_t owner = anchor->position_follows_body ? pose_find(poses, cursor, reference) : SIZE_MAX;
        Position position = owner == SIZE_MAX ? anchor->position :
            offset_position(poses[owner].position, anchor->position, poses[owner].angle);
        float angle = anchor->rotation + (owner != SIZE_MAX && anchor->rotation_follows_body ? poses[owner].angle : 0);
        ADD(OBJECT_ANCHOR, anchor->id, 0, position, angle, &anchor->entity_settings,
            SIZE_MAX, owner == SIZE_MAX, false);
        poses[cursor - 1].attachment = owner;
    }
#undef ADD
    Position point_a, point_b;
    size_t a = anchor_owner(object, poses, count, joint->anchor_a, &point_a);
    size_t b = anchor_owner(object, poses, count, joint->anchor_b, &point_b);
    if(a == SIZE_MAX || b == SIZE_MAX || !isfinite(joint->rest_length) ||
            joint->rest_length < 0 || !isfinite(joint->rest_angle)) goto done;
    float rotation = joint->kind == EDITOR_JOINT_WELD ?
        (float)remainder((double)poses[b].angle - joint->rest_angle - poses[a].angle, 360) : 0;
    Position destination = point_b;
    if(joint->kind == EDITOR_JOINT_SPRING) {
        double dx = (double)point_a.x - point_b.x, dy = (double)point_a.y - point_b.y;
        double length = hypot(dx, dy);
        if(!length) { dx = 0; dy = 1; length = 1; }
        destination.x += (float)(dx / length * joint->rest_length);
        destination.y += (float)(dy / length * joint->rest_length);
    }
    if(same_position(point_a, destination) && same_angle(rotation, 0)) { result = true; goto done; }
    poses[a].selected = true;
    bool changed;
    do {
        changed = false;
        for(size_t i = 0; i < count; ++i) {
            if(poses[i].selected && poses[i].group != SIZE_MAX) {
                for(size_t j = 0; j < count; ++j)
                    if(poses[j].group == poses[i].group && !poses[j].selected)
                        poses[j].selected = changed = true;
            }
            if(!poses[i].selected && poses[i].attachment != SIZE_MAX && poses[poses[i].attachment].selected)
                poses[i].selected = changed = true;
        }
        for(size_t i = 0; i < object->joint_count; ++i) {
            const EditorJoint *edge = &object->joint_items[i];
            if(edge == joint || edge->kind == EDITOR_JOINT_SPRING) continue;
            Position first_point, second_point;
            size_t first = anchor_owner(object, poses, count, edge->anchor_a, &first_point);
            size_t second = anchor_owner(object, poses, count, edge->anchor_b, &second_point);
            if(first == SIZE_MAX || second == SIZE_MAX) continue;
            if(poses[first].selected && !poses[second].selected) poses[second].selected = changed = true;
            if(poses[second].selected && !poses[first].selected) poses[first].selected = changed = true;
        }
    } while(changed);
    if(poses[b].selected) goto done;
    for(size_t i = 0; i < count; ++i) if(poses[i].selected) {
        Position offset = {poses[i].before.x - point_a.x, poses[i].before.y - point_a.y};
        poses[i].position = offset_position(destination, offset, rotation);
        poses[i].angle += rotation;
        if(poses[i].attachment != SIZE_MAX) {
            EditorAnchor *anchor = editor_project_anchor_get(object, (EditorAnchorId)poses[i].reference.id);
            if(anchor && !anchor->rotation_follows_body) poses[i].angle = poses[i].before_angle;
        }
    }
    for(size_t i = 0; i < count; ++i) {
        EditorPlacementPose *pose = &poses[i];
        const ObjectEntitySettings *settings = pose->settings;
        if(pose->selected) {
            if(pose->fixed || (pose->rotation_locked && !same_angle(pose->angle, pose->before_angle))) goto done;
            Position world = {object->position.x + pose->position.x, object->position.y + pose->position.y};
            if(!rohr_physics_world_position_check(world) || !isfinite(pose->angle)) goto done;
            if(settings->axis_lock_enabled) {
                Position point = settings->axis_lock.point_on_axis;
                if(!settings->axis_lock_world_fixed) {
                    point.x += object->position.x; point.y += object->position.y;
                }
                Vec2D axis = settings->axis_lock.axis;
                double magnitude = hypot(axis.x, axis.y);
                if(!magnitude || fabs(((double)world.x - point.x) * axis.y -
                        ((double)world.y - point.y) * axis.x) / magnitude > .0001) goto done;
            }
        }
        if(settings->transform_driver.id) {
            size_t driver = pose_find(poses, count, settings->transform_driver);
            if(driver == SIZE_MAX) { if(pose->selected) goto done; continue; }
            if(!pose->selected && !poses[driver].selected) continue;
            if(settings->transform_position && !same_position(pose->position,
                    offset_position(poses[driver].position, settings->transform_offset, poses[driver].angle))) goto done;
            if(settings->transform_orientation && !same_angle(pose->angle, poses[driver].angle + settings->transform_angle)) goto done;
        }
    }
    for(size_t i = 0; i < object->joint_count; ++i) {
        const EditorJoint *edge = &object->joint_items[i];
        if(edge->kind == EDITOR_JOINT_SPRING) continue;
        Position first_point, second_point;
        size_t first = anchor_owner(object, poses, count, edge->anchor_a, &first_point);
        size_t second = anchor_owner(object, poses, count, edge->anchor_b, &second_point);
        if(first == SIZE_MAX || second == SIZE_MAX || (!poses[first].selected && !poses[second].selected)) continue;
        if(!same_position(first_point, second_point) || (edge->kind == EDITOR_JOINT_WELD &&
                !same_angle(poses[second].angle - poses[first].angle, edge->rest_angle))) goto done;
    }
    ObjectEntityReference root = {OBJECT_CONTAINER, object->id, 0};
#define CHECK_EXTERNAL(array, amount, owner) \
    for(size_t i = 0; i < (amount); ++i) \
        if(!external_lock_check(poses, count, &(array)[i].entity_settings, owner)) goto done;
    CHECK_EXTERNAL(object->joint_items, object->joint_count, root)
    CHECK_EXTERNAL(object->force_items, object->force_count, root)
    CHECK_EXTERNAL(object->torque_items, object->torque_count, root)
    CHECK_EXTERNAL(object->cameras, object->camera_count, root)
    CHECK_EXTERNAL(object->sprites, object->sprite_count,
        ((ObjectEntityReference){OBJECT_BODY, object->sprites[i].rigid_body, 0}))
    CHECK_EXTERNAL(object->animated_sprite_items, object->animated_sprite_count,
        ((ObjectEntityReference){OBJECT_BODY, object->animated_sprite_items[i].rigid_body, 0}))
    for(size_t j = 0; j < object->soft_body_count; ++j) {
        EditorSoftBody *soft = &object->soft_body_items[j];
        ObjectEntityReference owner = {OBJECT_SOFT_BODY, soft->id, 0};
        CHECK_EXTERNAL(soft->beams, soft->beam_count, owner)
        CHECK_EXTERNAL(soft->areas, soft->area_count, owner)
    }
#undef CHECK_EXTERNAL
    /* Commit only root poses; local geometry, attachments, motion, and ownership
     * remain untouched. All constraints have already been checked. */
    for(size_t i = 0; i < object->rigid_body_count; ++i) {
        size_t index = pose_find(poses, count, (ObjectEntityReference){OBJECT_BODY, object->rigid_bodies[i].id, 0});
        if(poses[index].selected) {
            object->rigid_bodies[i].position = poses[index].position;
            object->rigid_bodies[i].rotation = poses[index].angle;
        }
    }
    for(size_t i = 0; i < object->soft_body_count; ++i) {
        size_t index = pose_find(poses, count, (ObjectEntityReference){OBJECT_SOFT_BODY, object->soft_body_items[i].id, 0});
        if(poses[index].selected) {
            object->soft_body_items[i].position = poses[index].position;
            object->soft_body_items[i].rotation = poses[index].angle;
        }
    }
    result = true;
done:
    free(poses);
    return result;
}
