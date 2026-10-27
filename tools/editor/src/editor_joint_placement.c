/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_project.h"
#include "editor_numeric.h"
#include "editor_command.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct EditorPlacementPose {
    ObjectEntityReference reference;
    Position before, position;
    Orientation before_angle, angle;
    const ObjectEntitySettings *settings;
    size_t group, attachment;
    bool selected, fixed, rotation_locked;
    bool numeric_x, numeric_y, numeric_rotation;
    float numeric_frame_angle;
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

static EditorPlacementPose *placement_poses_create(EditorObject *object, size_t *amount) {
    size_t count = 1 + object->rigid_body_count + object->soft_body_count + object->anchor_count;
    for(size_t i = 0; i < object->soft_body_count; ++i) count += object->soft_body_items[i].node_count;
    EditorPlacementPose *poses = calloc(count, sizeof(*poses));
    if(!poses) return NULL;
    size_t cursor = 0;
#define ADD(kind_value, id_value, parent_value, point, rotation, setting, group_value, fixed_value, locked_value) \
    poses[cursor++] = (EditorPlacementPose){.reference = {kind_value, id_value, parent_value}, \
        .before = point, .position = point, .before_angle = rotation, .angle = rotation, \
        .settings = setting, .group = group_value, .attachment = SIZE_MAX, .fixed = fixed_value, .rotation_locked = locked_value}
    ADD(OBJECT_CONTAINER, object->id, 0, ((Position){0}), 0, &object->entity_settings, SIZE_MAX, true, true);
    for(size_t i = 0; i < object->rigid_body_count; ++i) {
        EditorRigidBody *body = &object->rigid_bodies[i];
        ADD(OBJECT_BODY, body->id, 0, body->position, body->rotation,
            &body->entity_settings, SIZE_MAX, false, body->rotation_locked || body->standalone_particle);
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
                &node->entity_settings, group, false, false);
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
    *amount = count;
    return poses;
}

bool editor_project_joint_place(EditorObject *object, EditorJoint *joint) {
    if(!object || !joint) return false;
    size_t count;
    EditorPlacementPose *poses = placement_poses_create(object, &count);
    if(!poses) return false;
    bool result = false;
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

/* Automatic authoring placement. Scratch poses are committed only after the
 * entire affected component has been resolved and checked. */
static size_t placement_root(const EditorPlacementPose *poses, size_t index) {
    while(poses[index].attachment != SIZE_MAX) index = poses[index].attachment;
    return poses[index].group == SIZE_MAX ? index : poses[index].group;
}
static void placement_group_resolve(EditorPlacementPose *poses, size_t count, size_t root) {
    for(size_t i = 0; i < count; ++i)
        if(placement_root(poses, i) == root) poses[i].selected = true;
}
static void placement_group_move(EditorObject *object, EditorPlacementPose *poses,
        size_t count, size_t root, Position pivot, Position destination, float rotation) {
    for(size_t i = 0; i < count; ++i) if(placement_root(poses, i) == root) {
        Position offset = {poses[i].position.x - pivot.x, poses[i].position.y - pivot.y};
        poses[i].position = offset_position(destination, offset, rotation);
        poses[i].angle += rotation;
        if(poses[i].attachment != SIZE_MAX) {
            EditorAnchor *anchor = editor_project_anchor_get(object, poses[i].reference.id);
            if(anchor && !anchor->rotation_follows_body) poses[i].angle = poses[i].before_angle;
        }
        poses[i].selected = true;
    }
}
static bool placement_edge_satisfied_check(const EditorJoint *joint,
        Position a, Position b, float angle_a, float angle_b) {
    if(joint->kind == EDITOR_JOINT_SPRING)
        return fabs(hypot((double)a.x-b.x, (double)a.y-b.y)-joint->rest_length) <= .0001;
    return same_position(a,b) && (joint->kind != EDITOR_JOINT_WELD ||
        same_angle(angle_b-angle_a, joint->rest_angle));
}

void editor_project_placement_poses_restore(EditorObject *object, const EditorObject *before) {
    object->position = before->position;
    for(size_t i=0;i<object->rigid_body_count;++i) {
        object->rigid_bodies[i].position=before->rigid_bodies[i].position;
        object->rigid_bodies[i].rotation=before->rigid_bodies[i].rotation;
    }
    for(size_t i=0;i<object->soft_body_count;++i) {
        EditorSoftBody *body=&object->soft_body_items[i];
        const EditorSoftBody *old=&before->soft_body_items[i];
        body->position=old->position; body->rotation=old->rotation;
        for(size_t n=0;n<body->node_count;++n) body->nodes[n].position=old->nodes[n].position;
    }
    for(size_t i=0;i<object->anchor_count;++i) {
        object->anchors[i].position=before->anchors[i].position;
        object->anchors[i].rotation=before->anchors[i].rotation;
    }
}

/* Find the closest shared translation satisfying the axis lines in each
 * connected component. This preserves pin/weld/rest-length relationships while
 * retaining the allowed component of a diagonal pointer movement. */
static bool placement_drag_project_check(EditorObject *object,
        EditorPlacementPose *poses, size_t count) {
    size_t *components = malloc(count * sizeof(*components));
    if(!components) return false;
    for(size_t i = 0; i < count; ++i) components[i] = placement_root(poses, i);
    for(size_t j = 0; j < object->joint_count; ++j) {
        const EditorJoint *edge = &object->joint_items[j];
        if(edge->kind == EDITOR_JOINT_SPRING && !edge->editor_rest_length_locked) continue;
        Position a, b;
        size_t ia = anchor_owner(object, poses, count, edge->anchor_a, &a);
        size_t ib = anchor_owner(object, poses, count, edge->anchor_b, &b);
        if(ia == SIZE_MAX || ib == SIZE_MAX || !poses[ia].selected || !poses[ib].selected) continue;
        size_t from = components[ib], to = components[ia];
        for(size_t i = 0; i < count; ++i) if(components[i] == from) components[i] = to;
    }
    bool valid = true;
    for(size_t component = 0; component < count && valid; ++component) {
        double nx = 0, ny = 0, offset = 0, dx = 0, dy = 0;
        unsigned rank = 0;
        for(size_t i = 0; i < count && valid; ++i) {
            const EditorPlacementPose *pose = &poses[i];
            const ObjectEntitySettings *settings = pose->settings;
            if(!pose->selected || components[i] != component) continue;
            for(unsigned constraint=0;constraint<3 && valid;++constraint) {
            if(constraint==0 && !settings->axis_lock_enabled) continue;
            if(constraint==1 && !pose->numeric_x) continue;
            if(constraint==2 && !pose->numeric_y) continue;
            Axis axis = constraint==1?(Axis){0,1}:constraint==2?(Axis){1,0}:settings->axis_lock.axis;
            if(constraint) axis=math_vector_rotate(axis,pose->numeric_frame_angle);
            double length = hypot((double)axis.x, (double)axis.y);
            if(!isfinite(length) || length == 0) { valid = false; break; }
            double x = -axis.y / length, y = axis.x / length;
            Position point = constraint?pose->before:settings->axis_lock.point_on_axis;
            if(!constraint && settings->axis_lock_world_fixed) {
                point.x -= object->position.x; point.y -= object->position.y;
            }
            double distance = x * ((double)point.x - pose->position.x) +
                y * ((double)point.y - pose->position.y);
            if(!rank) {
                nx = x; ny = y; offset = distance;
                dx = x * distance; dy = y * distance; rank = 1;
            } else {
                double determinant = nx * y - ny * x;
                if(rank == 1 && fabs(determinant) > 1e-8) {
                    dx = (offset * y - ny * distance) / determinant;
                    dy = (nx * distance - offset * x) / determinant;
                    rank = 2;
                } else if(fabs(x * dx + y * dy - distance) > .0001) valid = false;
            }
            }
        }
        if(!valid) break;
        for(size_t i = 0; i < count; ++i) if(poses[i].selected && components[i] == component) {
            poses[i].position.x += (float)dx;
            poses[i].position.y += (float)dy;
        }
    }
    free(components);
    return valid;
}

EditorJointPlacementResult editor_project_joints_resolve(EditorObject *object,
        const EditorObject *before, const ObjectEntityReference *drivers, size_t driver_count,
        EditorJointId configured_joint, bool project_axes) {
    return editor_project_joints_resolve_locked(object,before,drivers,driver_count,configured_joint,project_axes,NULL);
}

/* Same endpoint ownership as anchor_owner, without allocating solver scratch
 * storage or changing the document for a presentation query. */
static bool status_anchor_pose_get(const EditorObject *object, EditorAnchorId id,
        Position *position, Orientation *angle) {
    const EditorAnchor *anchor = NULL;
    for(size_t i = 0; i < object->anchor_count; ++i)
        if(object->anchors[i].id == id) anchor = &object->anchors[i];
    if(!anchor) return false;
    *position = anchor->position; *angle = anchor->rotation;
    if(!anchor->position_follows_body) return true;
    if(anchor->attachment_kind == EDITOR_ANCHOR_ATTACHMENT_SOFT_NODE) {
        for(size_t i = 0; i < object->soft_body_count; ++i) {
            const EditorSoftBody *soft = &object->soft_body_items[i];
            if(soft->id != anchor->attachment_soft_body) continue;
            for(size_t n = 0; n < soft->node_count; ++n) if(soft->nodes[n].id == anchor->attachment_soft_node) {
                *angle = soft->rotation;
                *position = offset_position(offset_position(soft->position, soft->nodes[n].position, *angle), anchor->position, *angle);
                return true;
            }
        }
        return false;
    }
    if(!anchor->rigid_body) return true;
    for(size_t i = 0; i < object->rigid_body_count; ++i) {
        const EditorRigidBody *body = &object->rigid_bodies[i];
        if(body->id == anchor->rigid_body) {
            *angle = body->rotation;
            *position = offset_position(body->position, anchor->position, *angle);
            return true;
        }
    }
    return false;
}
EditorJointPlacementResult editor_project_joint_status_get(EditorObject *object,
        const EditorJoint *joint) {
    EditorJointPlacementResult result = {0};
    if(!object || !joint) return result;
    result.joint = joint->id;
    Position a, b; Orientation angle_a, angle_b;
    if(!status_anchor_pose_get(object, joint->anchor_a, &a, &angle_a) ||
            !status_anchor_pose_get(object, joint->anchor_b, &b, &angle_b)) {
        result.blocked = true;
        snprintf(result.reason, sizeof(result.reason), "Incomplete endpoints; joint does not respond");
    } else if((joint->kind != EDITOR_JOINT_SPRING || joint->editor_rest_length_locked) &&
            !placement_edge_satisfied_check(joint, a, b, angle_a, angle_b)) {
        result.blocked = true;
        snprintf(result.reason, sizeof(result.reason), "Authored placement does not satisfy this joint; check endpoints, rest value and connected constraints");
    }
    return result;
}

EditorJointPlacementResult editor_project_joints_resolve_locked(EditorObject *object,
        const EditorObject *before, const ObjectEntityReference *drivers, size_t driver_count,
        EditorJointId configured_joint, bool project_axes, const EditorProject *lock_project) {
    EditorJointPlacementResult result={0};
    if(!object || !before) return result;
    size_t count=0, old_count=0;
    EditorPlacementPose *poses=placement_poses_create(object,&count);
    EditorPlacementPose *old=placement_poses_create((EditorObject *)before,&old_count);
    if(!poses || !old || count!=old_count) {
        result.blocked=true; snprintf(result.reason,sizeof(result.reason),"unable to allocate placement poses");
        goto done;
    }
    for(size_t i=0;i<count;++i) {
        poses[i].before=old[i].position; poses[i].before_angle=old[i].angle;
        EditorNumericTarget target={.object=object->id,.item=(uint32_t)poses[i].reference.id,
            .parent=(uint32_t)poses[i].reference.soft_body};
        switch(poses[i].reference.kind) {
            case OBJECT_BODY:target.kind=EDITOR_ITEM_RIGID_BODY;break;
            case OBJECT_SOFT_BODY:target.kind=EDITOR_ITEM_SOFT_BODY;break;
            case OBJECT_NODE:target.kind=EDITOR_ITEM_SOFT_NODE;break;
            case OBJECT_ANCHOR:target.kind=EDITOR_ITEM_ANCHOR;break;
            default:continue;
        }
        poses[i].numeric_x=editor_numeric_lock_check(lock_project,target,"position.x");
        poses[i].numeric_y=editor_numeric_lock_check(lock_project,target,"position.y");
        poses[i].numeric_rotation=editor_numeric_lock_check(lock_project,target,"rotation");
        size_t numeric_owner=poses[i].reference.kind==OBJECT_NODE?poses[i].group:poses[i].attachment;
        if(numeric_owner!=SIZE_MAX && numeric_owner!=i) poses[i].numeric_frame_angle=old[numeric_owner].angle;
    }
    if(configured_joint) {
        for(size_t j=0;j<object->joint_count;++j) if(object->joint_items[j].id==configured_joint) {
            EditorJoint *edge=&object->joint_items[j]; Position a,b;
            if(edge->kind==EDITOR_JOINT_SPRING && !edge->editor_rest_length_locked) goto done;
            size_t ia=anchor_owner(object,poses,count,edge->anchor_a,&a);
            size_t ib=anchor_owner(object,poses,count,edge->anchor_b,&b);
            if(ia==SIZE_MAX || ib==SIZE_MAX) goto done;
            placement_group_resolve(poses,count,placement_root(poses,ib));
        }
    } else for(size_t i=0;i<driver_count;++i) {
        size_t index=pose_find(poses,count,drivers[i]);
        if(index!=SIZE_MAX) placement_group_resolve(poses,count,placement_root(poses,index));
    }
    bool progress, affected=false;
    do {
        progress=false;
        for(size_t j=0;j<object->joint_count;++j) {
            EditorJoint *edge=&object->joint_items[j];
            if(edge->kind==EDITOR_JOINT_SPRING && !edge->editor_rest_length_locked) continue;
            Position a,b;
            size_t ia=anchor_owner(object,poses,count,edge->anchor_a,&a);
            size_t ib=anchor_owner(object,poses,count,edge->anchor_b,&b);
            if(ia==SIZE_MAX || ib==SIZE_MAX || (!poses[ia].selected && !poses[ib].selected)) continue;
            affected=true;
            result.joint=edge->id;
            if(poses[ia].selected && poses[ib].selected) {
                if(!placement_edge_satisfied_check(edge,a,b,poses[ia].angle,poses[ib].angle)) {
                    snprintf(result.reason,sizeof(result.reason),"conflicting connected joint poses"); goto blocked;
                }
                continue;
            }
            bool a_drives=poses[ia].selected;
            size_t driven=a_drives?ib:ia, driver=a_drives?ia:ib;
            Position pivot=a_drives?b:a, destination=a_drives?a:b;
            float rotation=0;
            if(edge->kind==EDITOR_JOINT_WELD)
                rotation=(float)remainder((double)poses[driver].angle+
                    (a_drives?edge->rest_angle:-edge->rest_angle)-poses[driven].angle,360);
            if(edge->kind==EDITOR_JOINT_SPRING) {
                double dx=(double)pivot.x-destination.x,dy=(double)pivot.y-destination.y;
                double length=hypot(dx,dy);
                if(length<=.000001) { dx=0;dy=a_drives?-1:1;length=1; }
                destination.x+=(float)(dx/length*edge->rest_length);
                destination.y+=(float)(dy/length*edge->rest_length);
            }
            placement_group_move(object,poses,count,placement_root(poses,driven),pivot,destination,rotation);
            progress=true;
        }
    } while(progress);
    if(!affected && !project_axes) goto done;
    /* Local node/attachment values stay unchanged when their owner carries them. */
    for(size_t i=0;i<count;++i) {
        size_t owner=poses[i].reference.kind==OBJECT_NODE?poses[i].group:poses[i].attachment;
        if(owner!=SIZE_MAX && owner!=i && poses[owner].selected)
            poses[i].numeric_x=poses[i].numeric_y=poses[i].numeric_rotation=false;
    }
    if(project_axes && !placement_drag_project_check(object, poses, count)) {
        snprintf(result.reason, sizeof(result.reason), "incompatible axis constraints");
        goto blocked;
    }
    for(size_t i=0;i<count;++i) {
        EditorPlacementPose *pose=&poses[i];
        bool moved=!same_position(pose->before,pose->position) || !same_angle(pose->before_angle,pose->angle);
        if(!pose->selected || !moved) continue;
        result.blocker=pose->reference;
        bool directly_edited=false;
        for(size_t d=0;d<driver_count;++d) {
            size_t index=pose_find(poses,count,drivers[d]);
            if(index!=SIZE_MAX && placement_root(poses,index)==placement_root(poses,i)) directly_edited=true;
        }
        if(pose->fixed && !directly_edited) {
            snprintf(result.reason,sizeof(result.reason),"fixed world anchor cannot move"); goto blocked;
        }
        if(pose->rotation_locked && !same_angle(pose->angle,pose->before_angle)) {
            snprintf(result.reason,sizeof(result.reason),"rotation lock prevents placement"); goto blocked;
        }
        Position numeric_delta=math_vector_rotate((Position){pose->position.x-pose->before.x,pose->position.y-pose->before.y},-pose->numeric_frame_angle);
        if((pose->numeric_x && fabs(numeric_delta.x)>.0001) ||
           (pose->numeric_y && fabs(numeric_delta.y)>.0001) ||
           (pose->numeric_rotation && !same_angle(pose->angle,pose->before_angle))) {
            snprintf(result.reason,sizeof(result.reason),"numeric property lock prevents placement");goto blocked;
        }
        Position world={object->position.x+pose->position.x,object->position.y+pose->position.y};
        if(!rohr_physics_world_position_check(world) || !isfinite(pose->angle)) {
            snprintf(result.reason,sizeof(result.reason),"transform exceeds the supported world range"); goto blocked;
        }
        const ObjectEntitySettings *settings=pose->settings;
        if(settings->axis_lock_enabled) {
            Position point=settings->axis_lock.point_on_axis;
            if(!settings->axis_lock_world_fixed) { point.x+=object->position.x;point.y+=object->position.y; }
            Vec2D axis=settings->axis_lock.axis;double length=hypot(axis.x,axis.y);
            if(!length || fabs(((double)world.x-point.x)*axis.y-((double)world.y-point.y)*axis.x)/length>.0001) {
                snprintf(result.reason,sizeof(result.reason),"axis lock prevents placement");goto blocked;
            }
        }
    }
    for(size_t i=0;i<count;++i) {
        const ObjectEntitySettings *settings=poses[i].settings;
        if(!settings->transform_driver.id) continue;
        size_t driver=pose_find(poses,count,settings->transform_driver);
        if(driver==SIZE_MAX) {
            if(poses[i].selected) { snprintf(result.reason,sizeof(result.reason),"transform driver is unavailable");goto blocked; }
            continue;
        }
        if(!poses[i].selected && !poses[driver].selected) continue;
        if((settings->transform_position && !same_position(poses[i].position,
                offset_position(poses[driver].position,settings->transform_offset,poses[driver].angle))) ||
                (settings->transform_orientation && !same_angle(poses[i].angle,poses[driver].angle+settings->transform_angle))) {
            result.blocker=poses[i].reference;
            snprintf(result.reason,sizeof(result.reason),"transform-driver lock prevents placement");goto blocked;
        }
    }
#define EXTERNAL(array, amount, owner) \
    for(size_t i=0;i<(amount);++i) if(!external_lock_check(poses,count,&(array)[i].entity_settings,owner)) { \
        snprintf(result.reason,sizeof(result.reason),"placement would move an external locked attachment");goto blocked; }
    ObjectEntityReference root={OBJECT_CONTAINER,object->id,0};
    EXTERNAL(object->joint_items,object->joint_count,root)
    EXTERNAL(object->force_items,object->force_count,root)
    EXTERNAL(object->torque_items,object->torque_count,root)
    EXTERNAL(object->cameras,object->camera_count,root)
    EXTERNAL(object->sprites,object->sprite_count,((ObjectEntityReference){OBJECT_BODY,object->sprites[i].rigid_body,0}))
    EXTERNAL(object->animated_sprite_items,object->animated_sprite_count,((ObjectEntityReference){OBJECT_BODY,object->animated_sprite_items[i].rigid_body,0}))
    for(size_t b=0;b<object->soft_body_count;++b) {
        EditorSoftBody *body=&object->soft_body_items[b];ObjectEntityReference owner={OBJECT_SOFT_BODY,body->id,0};
        EXTERNAL(body->beams,body->beam_count,owner)
        EXTERNAL(body->areas,body->area_count,owner)
    }
#undef EXTERNAL
    for(size_t i=0;i<object->rigid_body_count;++i) {
        EditorRigidBody *body=&object->rigid_bodies[i];size_t index=pose_find(poses,count,(ObjectEntityReference){OBJECT_BODY,body->id,0});
        if(poses[index].selected) {body->position=poses[index].position;body->rotation=poses[index].angle;}
    }
    for(size_t i=0;i<object->soft_body_count;++i) {
        EditorSoftBody *body=&object->soft_body_items[i];size_t index=pose_find(poses,count,(ObjectEntityReference){OBJECT_SOFT_BODY,body->id,0});
        if(poses[index].selected) {body->position=poses[index].position;body->rotation=poses[index].angle;}
    }
    /* Unattached anchors have no parent transform to carry the projection. */
    for(size_t i=0;i<object->anchor_count;++i) {
        EditorAnchor *anchor=&object->anchors[i];
        size_t index=pose_find(poses,count,(ObjectEntityReference){OBJECT_ANCHOR,anchor->id,0});
        if(poses[index].selected && poses[index].attachment==SIZE_MAX)
            anchor->position=poses[index].position;
    }
    goto done;
blocked:
    result.blocked=true;
done:
    free(poses);free(old);return result;
}

EditorSpringGuide editor_project_spring_guide_get(Position a, Position b, float rest_length) {
    EditorSpringGuide guide={.start=a,.end=b};
    if(!isfinite(rest_length) || rest_length<0) return guide;
    double dx=(double)b.x-a.x,dy=(double)b.y-a.y,length=hypot(dx,dy);
    guide.length=(float)length;
    if(length<=.000001) {dx=0;dy=1;length=1;}
    double cx=((double)a.x+b.x)*.5,cy=((double)a.y+b.y)*.5;
    guide.rest_start=(Position){(float)(cx-dx/length*rest_length*.5),(float)(cy-dy/length*rest_length*.5)};
    guide.rest_end=(Position){(float)(cx+dx/length*rest_length*.5),(float)(cy+dy/length*rest_length*.5)};
    guide.stretched=guide.length>rest_length+.0001f;
    guide.compressed=guide.length<rest_length-.0001f;
    return guide;
}

/* Connectivity for choosing one driver per component when rotating selected
 * bodies about their own handles. Ordinary group transforms retain every driver. */
bool editor_project_joint_connected_check(EditorObject *object,
        ObjectEntityReference first, ObjectEntityReference second) {
    if(!object) return false;
    size_t count;EditorPlacementPose *poses=placement_poses_create(object,&count);
    if(!poses) return false;
    size_t a=pose_find(poses,count,first),b=pose_find(poses,count,second);
    bool connected=false;
    if(a==SIZE_MAX || b==SIZE_MAX) goto done;
    placement_group_resolve(poses,count,placement_root(poses,a));
    bool changed;
    do {
        changed=false;
        for(size_t j=0;j<object->joint_count;++j) {
            const EditorJoint *edge=&object->joint_items[j];Position pa,pb;
            if(edge->kind==EDITOR_JOINT_SPRING && !edge->editor_rest_length_locked) continue;
            size_t ia=anchor_owner(object,poses,count,edge->anchor_a,&pa);
            size_t ib=anchor_owner(object,poses,count,edge->anchor_b,&pb);
            if(ia==SIZE_MAX || ib==SIZE_MAX || poses[ia].selected==poses[ib].selected) continue;
            placement_group_resolve(poses,count,placement_root(poses,poses[ia].selected?ib:ia));changed=true;
        }
    } while(changed);
    connected=poses[b].selected;
done:
    free(poses);return connected;
}
