/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "entity_info.h"
#include "physics/physics_operation.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define FLAG(name) ROHR_ENTITY_INFO_##name
#define USED(pool, i) ((i) < (pool).capacity && (pool).used[(i)])

/* Separate from engine tables: snapshots survive table destruction. Calls are
 * serialized by the caller, just like ECS inspection between updates. IDs never
 * repeat, even when the last snapshot is released. */
typedef struct Snapshot {
    struct Snapshot *next;
    EntityInfo value;
    size_t owners;
    EntityInfoIssue *issues;
    size_t issue_count, issue_capacity;
    void *arrays[10];
    size_t array_count;
    EngineError failure;
} Snapshot;
static Snapshot *snapshots;
static uint64_t next_handle = 1;

static bool flags_check(EntityInfoFlags flags) {
    return flags != 0 && (flags & ~ROHR_ENTITY_INFO) == 0;
}
static Snapshot *snapshot_get(const EntityInfo *info) {
    if(info == NULL || info->handle == 0) return NULL;
    for(Snapshot *s = snapshots; s != NULL; s = s->next)
        if(s->value.handle == info->handle) return s;
    return NULL;
}
static void snapshot_destroy(Snapshot *s) {
    for(size_t i = 0; i < s->array_count; ++i) free(s->arrays[i]);
    free(s->issues);
    free(s);
}
static void *array_allocate(Snapshot *s, size_t count, size_t size) {
    if(count == 0 || s->failure) return NULL;
    if(count > SIZE_MAX / size || s->array_count == 10) {
        s->failure = ERROR_MEMORY_POOL_CAPACITY_OVERFLOW;
        return NULL;
    }
    void *array = calloc(count, size);
    if(array == NULL) s->failure = ERROR_MEMORY_POOL_ALLOCATION_FAILED;
    else s->arrays[s->array_count++] = array;
    return array;
}
static EntityInfoObjectRef object(EntityInfoObjectKind kind, uint64_t id, Entity owner) {
    return (EntityInfoObjectRef){kind, id, owner};
}
static void issue_add(Snapshot *s, EntityInfoFlags section, EntityInfoField field,
        EntityInfoIssueReason reason, EntityInfoObjectRef reference,
        bool has_index, size_t index, const char *detail) {
    if(s->failure) return;
    if(s->issue_count == s->issue_capacity) {
        size_t capacity = s->issue_capacity ? s->issue_capacity * 2 : 8;
        if(capacity < s->issue_capacity || capacity > SIZE_MAX / sizeof(*s->issues)) {
            s->failure = ERROR_MEMORY_POOL_CAPACITY_OVERFLOW;
            return;
        }
        EntityInfoIssue *values = realloc(s->issues, capacity * sizeof(*values));
        if(values == NULL) { s->failure = ERROR_MEMORY_POOL_ALLOCATION_FAILED; return; }
        s->issues = values;
        s->issue_capacity = capacity;
    }
    EntityInfoIssue *issue = &s->issues[s->issue_count++];
    *issue = (EntityInfoIssue){.section = section, .field = field, .reason = reason,
        .kind = reason == ROHR_ENTITY_INFO_COMPONENT_ABSENT ?
            ROHR_ENTITY_INFO_INFORMATION : ROHR_ENTITY_INFO_WARNING,
        .reference = reference, .has_index = has_index, .index = index};
    size_t length = strlen(detail);
    issue->detail_truncated = length >= sizeof(issue->detail);
    if(length >= sizeof(issue->detail)) length = sizeof(issue->detail) - 1;
    memcpy(issue->detail, detail, length);
}
static void missing(Snapshot *s, EntityInfoFlags flag) {
    issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, ROHR_ENTITY_INFO_COMPONENT_ABSENT,
        object(ROHR_ENTITY_INFO_OBJECT_ENTITY, s->value.entity, 0), false, 0,
        "Requested component is absent");
}
static void invalid(Snapshot *s, EntityInfoFlags flag, bool indexed, size_t index) {
    issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, ROHR_ENTITY_INFO_VALUE_INVALID,
        object(ROHR_ENTITY_INFO_OBJECT_ENTITY, s->value.entity, 0), indexed, index,
        "Captured value is invalid or non-finite");
}
static void unresolved(Snapshot *s, EntityInfoFlags flag, EngineError error) {
    if(error == ERROR_ENGINE_COMPONENT_MISSING) { missing(s, flag); return; }
    issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, ROHR_ENTITY_INFO_CALCULATION_FAILED,
        object(ROHR_ENTITY_INFO_OBJECT_ENTITY, s->value.entity, 0), false, 0,
        "Requested value could not be resolved");
}
static bool vector_check(Vec2D value) { return isfinite(value.x) && isfinite(value.y); }
static bool shape_check(Shape value) {
    if(value.amount_of_vertices < 3 || value.amount_of_vertices > MAX_VERTICIES) return false;
    for(size_t i = 0; i < value.amount_of_vertices; ++i)
        if(!vector_check(value.vertices[i])) return false;
    return true;
}
static bool reference_check(Snapshot *s, EntityInfoFlags flag, EntityInfoField field,
        Entity entity, RohrComponentMask required, bool indexed, size_t index) {
    EntityInfoIssueReason reason;
    const char *detail;
    if(!entity_alive_check(entity)) {
        reason = ROHR_ENTITY_INFO_REFERENCE_MISSING;
        detail = "Referenced entity is missing";
    } else if(required && !entity_components_check(entity, required)) {
        reason = ROHR_ENTITY_INFO_REFERENCE_DATA_MISSING;
        detail = "Referenced entity lacks required components";
    } else return true;
    issue_add(s, flag, field, reason,
        object(ROHR_ENTITY_INFO_OBJECT_ENTITY, entity, 0), indexed, index, detail);
    return false;
}
static bool transform_check(Snapshot *s, EntityInfoFlags flag, Entity entity,
        EntityInfoField field, bool indexed, size_t at, bool orientation_required) {
    EntityIndex index;
    if(!entity_index_get(entity, &index) || !entity_index_alive_check(index)) return false;
    if(!USED(positions_pool, index) || (orientation_required && !USED(orientations_pool, index))) {
        issue_add(s, flag, field, ROHR_ENTITY_INFO_REFERENCE_DATA_MISSING,
            object(ROHR_ENTITY_INFO_OBJECT_ENTITY, entity, 0), indexed, at,
            "Required position or orientation is absent");
        return false;
    }
    if(!vector_check(positions[index]) || (orientation_required && !isfinite(orientations[index]))) {
        issue_add(s, flag, field, ROHR_ENTITY_INFO_VALUE_INVALID,
            object(ROHR_ENTITY_INFO_OBJECT_ENTITY, entity, 0), indexed, at,
            "Required position or orientation is non-finite");
        return false;
    }
    return true;
}
static void resource_check(Snapshot *s, EntityInfoFlags flag, TextureAsset texture,
        bool indexed, size_t index) {
    if(!graphics_texture_valid_check(texture))
        issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_ANIMATION_FRAME,
            ROHR_ENTITY_INFO_REFERENCE_MISSING,
            object(ROHR_ENTITY_INFO_OBJECT_TEXTURE, texture.handle, 0), indexed, index,
            "Referenced texture is missing");
}
static void collection_copy(Snapshot *s, EntityInfoFlags flag, EntityInfoFlags count_flag,
        const void *source, size_t count, size_t size, const void **destination,
        size_t *count_destination) {
    *count_destination = count;
    s->value.available |= count_flag;
    if(!(s->value.requested & flag)) return;
    if(count && source == NULL) { invalid(s, flag, false, 0); return; }
    void *copy = array_allocate(s, count, size);
    if(count && !copy) return;
    if(count) memcpy(copy, source, count * size);
    *destination = copy;
    s->value.available |= flag;
}

/* Validate extents before copying or calling legacy getters that assume valid
 * component storage. An invalid count cannot be advertised as available. */
static bool collection_storage_check(Snapshot *s, EntityInfoFlags flag,
        const void *values, size_t count, size_t capacity, size_t size) {
    if(count > SIZE_MAX / size) {
        s->failure = ERROR_MEMORY_POOL_CAPACITY_OVERFLOW;
        return false;
    }
    if(count > capacity || (count != 0 && values == NULL)) {
        invalid(s, flag, false, 0);
        return false;
    }
    return true;
}
static bool hitbox_storage_check(Snapshot *s, EntityIndex index, EntityInfoFlags flag,
        bool active_required) {
    if(!(entity_mask[index] & ROHR_HIT_BOX) || !USED(hitbox_variants_pool, index)) {
        missing(s, flag);
        return false;
    }
    HitboxVariantList *list = &hitbox_variants_pool.objects[index];
    if(!collection_storage_check(s, flag, list->values, list->count,
            list->capacity, sizeof(*list->values))) return false;
    if(active_required && (list->count == 0 || list->active_index >= list->count)) {
        invalid(s, flag, false, 0);
        return false;
    }
    return true;
}

static void groups_capture(Snapshot *s, EntityIndex index, bool children_query, EntityInfoFlags requested_flag) {
    EntityInfo *v = &s->value;
    EntityInfoFlags flag = children_query ? FLAG(CORE_CHILDREN) : FLAG(CORE_GROUPS);
    EntityInfoFlags count_flag = children_query ? FLAG(CORE_CHILD_COUNT) : FLAG(CORE_GROUP_COUNT);
    const uint32_t *values = NULL;
    const uint8_t *used = NULL;
    size_t capacity = 0, count = 0;
    if(children_query && USED(children_pool, index)) {
        GroupId id = children_pool.objects[index];
        EntityGroupResult result = entity_group_get(id);
        if(result.kind == ERROR_RESULT_ERROR) {
            EntityInfoFlags sections = requested_flag;
            for(unsigned bit = 0; bit < 61; ++bit) if(sections & (UINT64_C(1) << bit))
                issue_add(s, UINT64_C(1) << bit, ROHR_ENTITY_INFO_FIELD_VALUE,
                    ROHR_ENTITY_INFO_REFERENCE_MISSING,
                    object(ROHR_ENTITY_INFO_OBJECT_GROUP, id, v->entity), false, 0,
                    "Children group is missing");
            return;
        }
        GroupEntityPool pool = result.result.value.entities;
        values = pool.objects; used = pool.used; capacity = pool.capacity;
    } else if(!children_query && USED(entity_group_memberships_pool, index)) {
        GroupIdPool pool = entity_group_memberships_pool.objects[index].groups;
        values = pool.objects; used = pool.used; capacity = pool.capacity;
    }
    for(size_t i = 0; i < capacity; ++i) if(used[i]) ++count;
    if(children_query) v->child_count = count; else v->group_count = count;
    v->available |= count_flag;
    if(!(v->requested & flag)) return;
    uint32_t *copy = array_allocate(s, count, sizeof(*copy));
    if(count && !copy) return;
    size_t out = 0;
    for(size_t i = 0; i < capacity; ++i) if(used[i]) {
        copy[out] = values[i];
        if(children_query) reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE,
            values[i], 0, true, out);
        else if(entity_group_get(values[i]).kind == ERROR_RESULT_ERROR)
            issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, ROHR_ENTITY_INFO_REFERENCE_MISSING,
                object(ROHR_ENTITY_INFO_OBJECT_GROUP, values[i], 0), true, out,
                "Referenced group is missing");
        ++out;
    }
    if(children_query) v->children = copy; else v->groups = copy;
    v->available |= flag;
}

static void anchors_capture(Snapshot *s, EntityIndex index, EntityInfoFlags flag) {
    EntityInfo *v = &s->value;
    JointAnchorListResult result = physics_joint_anchors_get(v->entity);
    if(result.kind == ERROR_RESULT_ERROR) { unresolved(s, flag, result.result.error); return; }
    JointAnchorList list = result.result.value;
    v->joint_anchor_count = list.count;
    v->available |= FLAG(PHYSICS_JOINT_ANCHOR_COUNT);
    if(flag == FLAG(PHYSICS_JOINT_ANCHOR_COUNT)) return;
    bool world = flag == FLAG(PHYSICS_JOINT_ANCHORS_WORLD), valid = true;
    void *array = array_allocate(s, list.count,
        world ? sizeof(EntityInfoJointAnchorWorld) : sizeof(EntityInfoJointAnchor));
    if(list.count && !array) return;
    for(size_t i = 0; i < list.count; ++i) {
        JointAnchorPositionResult position = world ? physics_joint_anchor_world_position_get(list.values[i]) :
            physics_joint_anchor_local_position_get(list.values[i]);
        bool transform_missing = world && (!USED(positions_pool, index) ||
            (!(entity_mask[index] & ROHR_PARTICLE) && !USED(orientations_pool, index)));
        if(position.kind == ERROR_RESULT_ERROR || transform_missing || !vector_check(position.result.value)) {
            valid = false;
            issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_ANCHOR,
                transform_missing ? ROHR_ENTITY_INFO_REFERENCE_DATA_MISSING : ROHR_ENTITY_INFO_CALCULATION_FAILED,
                object(ROHR_ENTITY_INFO_OBJECT_JOINT_ANCHOR, list.values[i], v->entity), true, i,
                "Anchor position could not be resolved");
        } else if(world) ((EntityInfoJointAnchorWorld *)array)[i] =
            (EntityInfoJointAnchorWorld){list.values[i], position.result.value};
        else ((EntityInfoJointAnchor *)array)[i] =
            (EntityInfoJointAnchor){list.values[i], {v->entity, position.result.value}};
    }
    if(valid) {
        if(world) v->joint_anchors_world = array; else v->joint_anchors = array;
        v->available |= flag;
    }
}

static void layer_capture(Snapshot *s, EntityInfoFlags flag,
        GraphicsLayerIdResult id, GraphicsLayerValueResult value, EntityInfoLayer *destination) {
    if(value.kind == ERROR_RESULT_VALUE) {
        *destination = (EntityInfoLayer){id.kind == ERROR_RESULT_VALUE ? id.result.value : 0,
            value.result.value};
        s->value.available |= flag;
    } else if(id.kind == ERROR_RESULT_ERROR) missing(s, flag);
    else issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_LAYER_REFERENCE,
        ROHR_ENTITY_INFO_REFERENCE_MISSING,
        object(ROHR_ENTITY_INFO_OBJECT_GRAPHICS_LAYER, id.result.value, 0), false, 0,
        "Referenced graphics layer is missing");
}

#define POOL_CAPTURE(pool, field, component, valid) do { \
    if(!USED(pool, index) || ((component) && !(mask & (component)))) missing(s, flag); \
    else if(!(valid)) invalid(s, flag, false, 0); \
    else { v->field = (pool).objects[index]; v->available |= flag; } \
} while(0)
#define GET_CAPTURE(type, call, field, valid) do { \
    type result = (call); \
    if(result.kind == ERROR_RESULT_ERROR) unresolved(s, flag, result.result.error); \
    else if(!(valid)) invalid(s, flag, false, 0); \
    else { v->field = result.result.value; v->available |= flag; } \
} while(0)
#define COPY_COLLECTION(name, count_name, type, source, count, count_flag) do { \
    const void *copy = NULL; \
    collection_copy(s, flag, count_flag, source, count, sizeof(type), &copy, &v->count_name); \
    if(v->requested & flag) v->name = copy; \
} while(0)

/* Inspect all direct prerequisites, rather than returning only the first failed
 * node from a derived soft-body getter. No caches or node state are modified. */
static bool soft_mass_dependencies_check(Snapshot *s, EntityIndex index, EntityInfoFlags flag) {
    if(!(entity_mask[index] & ROHR_SOFT_BODY) || !USED(soft_bodies_pool, index)) return true;
    SoftBody *body = &soft_bodies[index];
    if(body->node_count > SOFT_BODY_MAX_NODES) { invalid(s, flag, false, 0); return false; }
    bool valid = true;
    for(size_t i = 0; i < body->node_count; ++i) {
        Entity node = body->nodes[i];
        if(!reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, node, ROHR_MASS, true, i)) {
            valid = false; continue;
        }
        EntityIndex ni;
        if(!entity_index_get(node, &ni)) { valid = false; continue; }
        if(!USED(mass_pool, ni) || !USED(positions_pool, ni)) {
            issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, ROHR_ENTITY_INFO_REFERENCE_DATA_MISSING,
                object(ROHR_ENTITY_INFO_OBJECT_ENTITY, node, 0), true, i,
                "Node mass or position is absent");
            valid = false;
        } else if(!isfinite(mass[ni]) || mass[ni] < 0 || !vector_check(positions[ni])) {
            issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, ROHR_ENTITY_INFO_VALUE_INVALID,
                object(ROHR_ENTITY_INFO_OBJECT_ENTITY, node, 0), true, i,
                "Node mass or position is invalid");
            valid = false;
        }
    }
    return valid;
}

static void section_capture(Snapshot *s, EntityIndex index, EntityInfoFlags flag) {
    EntityInfo *v = &s->value;
    Entity entity = v->entity;
    RohrComponentMask mask = entity_mask[index];
    switch(flag) {
    case FLAG(CORE_NAME):
        POOL_CAPTURE(entity_names_pool, name, ROHR_ENTITY_NAME, true);
        break;
    case FLAG(CORE_COMPONENTS):
        POOL_CAPTURE(entity_mask_pool, components, 0, true);
        break;
    case FLAG(CORE_LIFETIME):
        POOL_CAPTURE(life_times_pool, lifetime, ROHR_LIFETIME, true);
        break;
    case FLAG(CORE_PARENT):
        POOL_CAPTURE(parents_pool, parent, ROHR_PARENT, true);
        break;
    case FLAG(CORE_TARGET):
        POOL_CAPTURE(targets_pool, target, ROHR_TARGETABLE, true);
        break;
    case FLAG(PHYSICS_POSITION):
        POOL_CAPTURE(positions_pool, position, 0, vector_check(positions[index]));
        break;
    case FLAG(PHYSICS_ORIENTATION):
        POOL_CAPTURE(orientations_pool, orientation, 0, isfinite(orientations[index]));
        break;
    case FLAG(PHYSICS_VELOCITY):
        POOL_CAPTURE(velocities_pool, velocity, 0, vector_check(velocities[index]));
        break;
    case FLAG(PHYSICS_ACCELERATION):
        POOL_CAPTURE(accelerations_pool, acceleration, 0, vector_check(accelerations[index]));
        break;
    case FLAG(PHYSICS_ANGULAR_VELOCITY):
        POOL_CAPTURE(angular_velocities_pool, angular_velocity, 0, isfinite(angular_velocities[index]));
        break;
    case FLAG(PHYSICS_ANGULAR_ACCELERATION):
        POOL_CAPTURE(angular_accelerations_pool, angular_acceleration, 0, isfinite(angular_accelerations[index]));
        break;
    case FLAG(PHYSICS_ANGULAR_VELOCITY_MAXIMUM):
        POOL_CAPTURE(angular_velocity_maximums_pool, angular_velocity_maximum, 0, isfinite(angular_velocity_maximums[index]));
        break;
    case FLAG(PHYSICS_FRICTION):
        POOL_CAPTURE(frictions_pool, friction, 0, isfinite(frictions[index]) && frictions[index] >= 0);
        break;
    case FLAG(PHYSICS_RESTITUTION):
        POOL_CAPTURE(restitutions_pool, restitution, 0, isfinite(restitutions[index]) && restitutions[index] >= 0);
        break;
    case FLAG(PHYSICS_PARTICLE):
        POOL_CAPTURE(particle_geometries_pool, particle, ROHR_PARTICLE, true);
        break;
    case FLAG(PHYSICS_AXIS_LOCK):
        POOL_CAPTURE(axis_locks_pool, axis_lock, ROHR_AXIS_LOCK, true);
        break;
    case FLAG(PHYSICS_ANGLE_LOCK):
        POOL_CAPTURE(angle_locks_pool, angle_lock, ROHR_ANGLE_LOCK, true);
        break;
    case FLAG(PHYSICS_TRANSFORM_LOCK):
        POOL_CAPTURE(transform_locks_pool, transform_lock, ROHR_TRANSFORM_LOCK, true);
        break;
    case FLAG(PHYSICS_JOINT):
        POOL_CAPTURE(joints_pool, joint, ROHR_JOINT, true);
        break;
    case FLAG(PHYSICS_SOFT_BODY):
        POOL_CAPTURE(soft_bodies_pool, soft_body, ROHR_SOFT_BODY, true);
        break;
    case FLAG(PHYSICS_SOFT_BODY_NODE):
        POOL_CAPTURE(soft_body_nodes_pool, soft_body_node, ROHR_SOFT_BODY_NODE, true);
        break;
    case FLAG(PHYSICS_SOFT_BODY_BEAM):
        POOL_CAPTURE(soft_body_beams_pool, soft_body_beam, ROHR_SOFT_BODY_BEAM, true);
        break;
    case FLAG(PHYSICS_FORCE):
        POOL_CAPTURE(forces_pool, force, ROHR_FORCE, true);
        if(USED(forces_pool, index)) {
            EntityResult owner = entity_owner_get(entity);
            if(!error_check(owner)) v->force.owner = owner.result.value;
        }
        break;
    case FLAG(PHYSICS_TORQUE):
        POOL_CAPTURE(torques_pool, torque, ROHR_TORQUE, true);
        if(USED(torques_pool, index)) {
            EntityResult owner = entity_owner_get(entity);
            if(!error_check(owner)) v->torque.owner = owner.result.value;
        }
        break;
    case FLAG(GRAPHICS_SPRITE):
        POOL_CAPTURE(sprites_pool, sprite, ROHR_SPRITE, true);
        break;
    case FLAG(PHYSICS_MASS):
        if(!soft_mass_dependencies_check(s, index, flag)) break;
        GET_CAPTURE(MassResult, physics_mass_get(entity), mass_value, isfinite(result.result.value) && result.result.value >= 0);
        break;
    case FLAG(PHYSICS_COM_LOCAL):
        if(!soft_mass_dependencies_check(s, index, flag)) break;
        GET_CAPTURE(PositionResult, physics_center_of_mass_local_position_get(entity), center_of_mass_local, vector_check(result.result.value));
        break;
    case FLAG(PHYSICS_COM_WORLD):
        if(!soft_mass_dependencies_check(s, index, flag)) break;
        GET_CAPTURE(PositionResult, physics_center_of_mass_world_position_get(entity), center_of_mass_world, vector_check(result.result.value));
        break;
    case FLAG(PHYSICS_INERTIA):
        GET_CAPTURE(MomentOfInertiaResult, physics_moment_of_inertia_get(entity), moment_of_inertia, isfinite(result.result.value));
        break;
    case FLAG(PHYSICS_COLLISION_FILTER):
        GET_CAPTURE(CollisionFilterConfigResult, physics_collision_filter_get(entity), collision_filter, true);
        break;
    case FLAG(PHYSICS_HITBOX_LOCAL):
        if(!hitbox_storage_check(s, index, flag, true)) break;
        GET_CAPTURE(ShapeResult, physics_hitbox_get(entity), hitbox_local, shape_check(result.result.value));
        break;
    case FLAG(PHYSICS_HITBOX_WORLD):
        if(!(mask & ROHR_HIT_BOX) || !USED(hit_boxes_pool, index)) { missing(s, flag); break; }
        if(!shape_check(hit_boxes[index])) { invalid(s, flag, false, 0); break; }
        if((mask & ROHR_HIT_BOX) && !transform_check(s, flag, entity,
                ROHR_ENTITY_INFO_FIELD_SHAPE, false, 0, !(mask & ROHR_PARTICLE))) break;
        GET_CAPTURE(ShapeResult, physics_global_hit_box_get(entity), hitbox_world, shape_check(result.result.value));
        break;
    case FLAG(PHYSICS_SOFT_BODY_AREA):
        GET_CAPTURE(SoftBodyAreaResult, physics_soft_body_area_get(entity), soft_body_area, true);
        break;
    case FLAG(CORE_CHILDREN): case FLAG(CORE_CHILD_COUNT):
        groups_capture(s, index, true, flag); break;
    case FLAG(CORE_GROUPS): case FLAG(CORE_GROUP_COUNT):
        groups_capture(s, index, false, flag); break;
    case FLAG(PHYSICS_BODY_MODE):
        v->body_mode = (EntityInfoBodyMode){
            (mask & ROHR_STATIC) != 0, (mask & ROHR_DYNAMIC) != 0,
            (mask & ROHR_HOLD) != 0, (mask & ROHR_KINEMATIC_DRIVEN) != 0,
            (mask & ROHR_GRAVITY) != 0, (mask & ROHR_COLLISION) != 0};
        v->available |= flag;
        break;
    case FLAG(PHYSICS_HITBOX_ACTIVE): {
        if(!hitbox_storage_check(s, index, flag, true)) break;
        HitboxIdResult id = physics_hitbox_active_id_get(entity);
        HitboxIndexResult active = physics_hitbox_active_index_get(entity);
        if(id.kind == ERROR_RESULT_ERROR) unresolved(s, flag, id.result.error);
        else if(active.kind == ERROR_RESULT_ERROR) unresolved(s, flag, active.result.error);
        else { v->hitbox_active = (EntityInfoActiveHitbox){id.result.value, active.result.value};
            v->available |= flag; }
        break;
    }
    case FLAG(PHYSICS_HITBOXES): case FLAG(PHYSICS_HITBOX_COUNT): {
        HitboxVariantList list = USED(hitbox_variants_pool, index) && (mask & ROHR_HIT_BOX) ?
            hitbox_variants_pool.objects[index] : (HitboxVariantList){0};
        if(!collection_storage_check(s, flag, list.values, list.count, list.capacity, sizeof(*list.values))) break;
        flag = FLAG(PHYSICS_HITBOXES);
        COPY_COLLECTION(hitboxes, hitbox_count, HitboxVariant, list.values, list.count,
            FLAG(PHYSICS_HITBOX_COUNT));
        if(v->available & flag) for(size_t i = 0; i < list.count; ++i)
            if(!shape_check(list.values[i].shape)) invalid(s, flag, true, i);
        break;
    }
    case FLAG(PHYSICS_HITBOX_BINDINGS): case FLAG(PHYSICS_HITBOX_BINDING_COUNT): {
        HitboxAnimationBindingList list = USED(hitbox_animation_bindings_pool, index) &&
            (mask & ROHR_HITBOX_ANIMATION_BINDING) ? hitbox_animation_bindings_pool.objects[index] :
            (HitboxAnimationBindingList){0};
        if(!collection_storage_check(s, flag, list.values, list.count, list.capacity, sizeof(*list.values))) break;
        flag = FLAG(PHYSICS_HITBOX_BINDINGS);
        COPY_COLLECTION(hitbox_bindings, hitbox_binding_count, HitboxAnimationBinding,
            list.values, list.count, FLAG(PHYSICS_HITBOX_BINDING_COUNT));
        HitboxVariantList hitboxes = (mask & ROHR_HIT_BOX) && USED(hitbox_variants_pool, index) ?
            hitbox_variants_pool.objects[index] : (HitboxVariantList){0};
        bool hitboxes_valid = true;
        if((v->available & flag) && list.count)
            hitboxes_valid = collection_storage_check(s, flag, hitboxes.values,
                hitboxes.count, hitboxes.capacity, sizeof(*hitboxes.values));
        if(v->available & flag) for(size_t i = 0; i < list.count; ++i) {
            bool found = false;
            if(hitboxes_valid) for(size_t h = 0; h < hitboxes.count; ++h)
                if(hitboxes.values[h].id == list.values[i].hitbox_id) found = true;
            /* Invalid storage prevents lookup; do not invent missing-ID issues. */
            if(hitboxes_valid && !found)
                issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_SHAPE, ROHR_ENTITY_INFO_REFERENCE_MISSING,
                    object(ROHR_ENTITY_INFO_OBJECT_HITBOX, list.values[i].hitbox_id, entity), true, i,
                    "Bound hitbox is missing");
            bool frame_found = false;
            if(USED(animated_sprites_pool, index)) {
                const AnimatedSprite *a = &animated_sprites[index];
                AnimationInfoResult metadata = graphics_animation_info_get(a->player.animation);
                if(metadata.kind == ERROR_RESULT_VALUE && metadata.result.value.id == list.values[i].animation_id)
                    for(size_t f = 0; f < a->frame_count && f < MAX_ANIMATIONS_FRAMES; ++f)
                        if(a->frames[f].id == list.values[i].frame_id) frame_found = true;
            }
            if(!frame_found)
                issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_ANIMATION_FRAME,
                    ROHR_ENTITY_INFO_REFERENCE_DATA_MISSING,
                    object(ROHR_ENTITY_INFO_OBJECT_ENTITY, entity, 0), true, i,
                    "Bound animation or frame is missing from the entity");
        }
        break;
    }
    case FLAG(PHYSICS_JOINT_ANCHORS): case FLAG(PHYSICS_JOINT_ANCHORS_WORLD):
    case FLAG(PHYSICS_JOINT_ANCHOR_COUNT):
        anchors_capture(s, index, flag); break;
    case FLAG(PHYSICS_SOFT_BODY_NODE_COUNT): case FLAG(PHYSICS_SOFT_BODY_BEAM_COUNT):
    case FLAG(PHYSICS_SOFT_BODY_AREA_COUNT):
        if(!(mask & ROHR_SOFT_BODY) || !USED(soft_bodies_pool, index)) missing(s, flag);
        else {
            SoftBody body = soft_bodies[index];
            if(flag == FLAG(PHYSICS_SOFT_BODY_NODE_COUNT)) v->soft_body_node_count = body.node_count;
            if(flag == FLAG(PHYSICS_SOFT_BODY_BEAM_COUNT)) v->soft_body_beam_count = body.beam_count;
            if(flag == FLAG(PHYSICS_SOFT_BODY_AREA_COUNT)) v->soft_body_area_count = body.area_count;
            v->available |= flag;
        }
        break;
    case FLAG(PHYSICS_OVERLAPS): case FLAG(PHYSICS_OVERLAP_COUNT):
        v->overlap_count = physics_overlap_count_get(entity);
        v->available |= FLAG(PHYSICS_OVERLAP_COUNT);
        if(v->requested & FLAG(PHYSICS_OVERLAPS)) {
            EntityInteraction *copy = array_allocate(s, v->overlap_count, sizeof(*copy));
            if(v->overlap_count && !copy) break;
            physics_overlaps_get(entity, copy, v->overlap_count);
            v->overlaps = copy; v->available |= FLAG(PHYSICS_OVERLAPS);
        }
        break;
    case FLAG(PHYSICS_CONTACTS): case FLAG(PHYSICS_CONTACT_COUNT):
        v->contact_count = physics_contact_count_get(entity);
        v->available |= FLAG(PHYSICS_CONTACT_COUNT);
        if(v->requested & FLAG(PHYSICS_CONTACTS)) {
            EntityContact *copy = array_allocate(s, v->contact_count, sizeof(*copy));
            if(v->contact_count && !copy) break;
            physics_contacts_get(entity, copy, v->contact_count);
            v->contacts = copy; v->available |= FLAG(PHYSICS_CONTACTS);
        }
        break;
    case FLAG(GRAPHICS_ANIMATIONS): case FLAG(GRAPHICS_ANIMATION_COUNT): {
        size_t count = (mask & ROHR_ANIMATED_SPRITE) && USED(animated_sprites_pool, index) ? 1 : 0;
        flag = FLAG(GRAPHICS_ANIMATIONS);
        COPY_COLLECTION(animations, animation_count, AnimatedSprite,
            count ? &animated_sprites[index] : NULL, count, FLAG(GRAPHICS_ANIMATION_COUNT));
        if(count && (v->available & flag)) {
            const AnimatedSprite *a = v->animations;
            if(a->frame_count > MAX_ANIMATIONS_FRAMES ||
                    a->player.frame_count != a->frame_count || !isfinite(a->scale.x) ||
                    !isfinite(a->scale.y) || !vector_check(a->body_offset) ||
                    !isfinite(a->orientation_offset)) invalid(s, flag, true, 0);
            if(a->player.animation.handle && !graphics_animation_valid_check(a->player.animation))
                issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, ROHR_ENTITY_INFO_REFERENCE_MISSING,
                    object(ROHR_ENTITY_INFO_OBJECT_ANIMATION, a->player.animation.handle, 0), true, 0,
                    "Referenced animation is missing");
            for(size_t i = 0; i < a->frame_count && i < MAX_ANIMATIONS_FRAMES; ++i)
                resource_check(s, flag, a->frames[i].texture, true, i);
        }
        break;
    }
    case FLAG(GRAPHICS_PRESENCE):
        v->has_sprite = (mask & ROHR_SPRITE) != 0;
        v->is_animated = (mask & ROHR_ANIMATED_SPRITE) != 0;
        v->has_animations = v->is_animated && USED(animated_sprites_pool, index);
        v->available |= flag;
        break;
    case FLAG(GRAPHICS_SPRITE_VISIBILITY): case FLAG(GRAPHICS_SPRITE_SCALE):
        if(!(mask & ROHR_SPRITE) || !USED(sprites_pool, index)) missing(s, flag);
        else {
            if(flag == FLAG(GRAPHICS_SPRITE_VISIBILITY)) v->sprite_visible = sprite_components[index].visible;
            else {
                v->sprite_scale = sprite_components[index].scale;
                if(!isfinite(v->sprite_scale.x) || !isfinite(v->sprite_scale.y)) invalid(s, flag, false, 0);
            }
            v->available |= flag;
        }
        break;
    case FLAG(GRAPHICS_ENTITY_LAYER):
        layer_capture(s, flag, graphics_layer_entity_id_get(entity),
            graphics_layer_entity_get(entity), &v->entity_layer);
        break;
    case FLAG(GRAPHICS_SPRITE_LAYER):
        layer_capture(s, flag, graphics_layer_sprite_id_get(entity),
            graphics_layer_sprite_get(entity), &v->sprite_layer);
        break;
    case FLAG(GRAPHICS_ANIMATION_LAYER):
        layer_capture(s, flag, graphics_layer_animation_id_get(entity),
            graphics_layer_animation_get(entity), &v->animation_layer);
        break;
    default: break;
    }
}

static void topology_check(Snapshot *s, EntityInfoFlags flag, const Entity *entities,
        size_t count, size_t maximum, RohrComponentMask component) {
    if(count > maximum) { invalid(s, flag, false, 0); count = maximum; }
    for(size_t i = 0; i < count; ++i)
        reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, entities[i], component, true, i);
}
static void source_beam_references_check(Snapshot *s, EntityInfoFlags flag,
        EntityInfoField field, Entity beam) {
    EntityIndex index;
    if(!reference_check(s, flag, field, beam, ROHR_SOFT_BODY_BEAM, false, 0) ||
            !entity_index_get(beam, &index)) return;
    if(!USED(soft_body_beams_pool, index)) {
        issue_add(s, flag, field, ROHR_ENTITY_INFO_REFERENCE_DATA_MISSING,
            object(ROHR_ENTITY_INFO_OBJECT_ENTITY, beam, 0), false, 0,
            "Beam configuration is absent");
        return;
    }
    SoftBodyBeam b = soft_body_beams[index];
    if(reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_BEAM_START, b.node_a,
            ROHR_SOFT_BODY_NODE, false, 0))
        transform_check(s, flag, b.node_a, ROHR_ENTITY_INFO_FIELD_BEAM_START, false, 0, true);
    if(reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_BEAM_END, b.node_b,
            ROHR_SOFT_BODY_NODE, false, 0))
        transform_check(s, flag, b.node_b, ROHR_ENTITY_INFO_FIELD_BEAM_END, false, 0, true);
}
static void source_location_references_check(Snapshot *s, EntityInfoFlags flag,
        EntityInfoField field, ForceLocation location) {
    if(location.reference.kind == ROHR_FORCE_BEAM_POSITION)
        source_beam_references_check(s, flag, field, location.reference.beam.entity);
    else if(location.reference.kind == ROHR_FORCE_COM_POSITION ||
            location.reference.kind == ROHR_FORCE_ORIGIN_POSITION) {
        if(reference_check(s, flag, field, location.reference.entity, 0, false, 0))
            transform_check(s, flag, location.reference.entity, field, false, 0, true);
    }
}
static void source_references_check(Snapshot *s, EntityInfoFlags flag) {
    EntityInfo *v = &s->value;
    bool torque = flag == FLAG(PHYSICS_TORQUE);
    Entity target = torque ? v->torque.target : v->force.target;
    size_t before = s->issue_count;
    if(target) reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_TARGET, target, 0, false, 0);
    if(!torque) {
        source_location_references_check(s, flag, ROHR_ENTITY_INFO_FIELD_APPLICATION_REFERENCE, v->force.location);
        ForceOrientationReference ref = v->force.orientation.reference;
        if(ref.kind == ROHR_FORCE_BEAM_ORIENTATION)
            source_beam_references_check(s, flag, ROHR_ENTITY_INFO_FIELD_ORIENTATION_REFERENCE, ref.entity);
        else if(ref.kind == ROHR_FORCE_BODY_ORIENTATION)
            reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_ORIENTATION_REFERENCE, ref.entity, 0, false, 0);
        else if(ref.kind == ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION) {
            source_location_references_check(s, flag, ROHR_ENTITY_INFO_FIELD_ORIENTATION_REFERENCE, ref.between.from);
            source_location_references_check(s, flag, ROHR_ENTITY_INFO_FIELD_ORIENTATION_REFERENCE, ref.between.to);
        }
    }
    bool active = false;
    PhysicsSourceChanges change;
    EngineResult result = physics_source_change_get(v->entity, torque, &change, &active);
    if(torque) v->torque_eligible = active;
    else v->force_eligible = active;
    if(error_check(result) && before == s->issue_count)
        issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, ROHR_ENTITY_INFO_CALCULATION_FAILED,
            object(ROHR_ENTITY_INFO_OBJECT_ENTITY, v->entity, 0), false, 0,
            "Source contribution cannot currently be resolved; configuration retained");
}

static void dependencies_check(Snapshot *s, EntityInfoFlags flag) {
    EntityInfo *v = &s->value;
    if(!(v->available & flag)) return;
    if(flag == FLAG(CORE_PARENT))
        reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, v->parent, 0, false, 0);
    if(flag == FLAG(CORE_TARGET) && v->target != ENTITY_INVALID)
        reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_TARGET, v->target, 0, false, 0);
    if(flag == FLAG(PHYSICS_FORCE) || flag == FLAG(PHYSICS_TORQUE))
        source_references_check(s, flag);
    if(flag == FLAG(PHYSICS_TRANSFORM_LOCK)) {
        if(reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_TARGET, v->transform_lock.driver, 0, false, 0))
            transform_check(s, flag, v->transform_lock.driver, ROHR_ENTITY_INFO_FIELD_TARGET, false, 0, true);
        if(!vector_check(v->transform_lock.local_offset) || !isfinite(v->transform_lock.local_angle))
            invalid(s, flag, false, 0);
    }
    if(flag == FLAG(PHYSICS_AXIS_LOCK) && (!vector_check(v->axis_lock.point_on_axis) || !vector_check(v->axis_lock.axis))) invalid(s, flag, false, 0);
    if(flag == FLAG(PHYSICS_ANGLE_LOCK) && (!isfinite(v->angle_lock.min) ||
            !isfinite(v->angle_lock.max) || v->angle_lock.min > v->angle_lock.max)) invalid(s, flag, false, 0);
    if(flag == FLAG(PHYSICS_PARTICLE) && (!vector_check(v->particle.local_origin) ||
            !isfinite(v->particle.radius) || v->particle.radius <= 0)) invalid(s, flag, false, 0);
    if(flag == FLAG(PHYSICS_JOINT)) {
        Joint j = v->joint;
        reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_BEAM_START, j.a, 0, false, 0);
        reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_BEAM_END, j.b, 0, false, 0);
        JointAnchorId ids[] = {j.anchor_a, j.anchor_b};
        for(size_t i = 0; i < 2; ++i) if(ids[i]) {
            JointAnchorPositionResult position = physics_joint_anchor_world_position_get(ids[i]);
            if(position.kind == ERROR_RESULT_ERROR)
                issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_ANCHOR, ROHR_ENTITY_INFO_REFERENCE_MISSING,
                    object(ROHR_ENTITY_INFO_OBJECT_JOINT_ANCHOR, ids[i], i ? j.b : j.a), true, i,
                    "Joint anchor is missing");
        }
        if(!vector_check(j.local_anchor_a) || !vector_check(j.local_anchor_b) ||
                !isfinite(j.rest_length) || !isfinite(j.stiffness) || !isfinite(j.damping) ||
                !isfinite(j.rest_angle) || !isfinite(j.angular_stiffness) || !isfinite(j.angular_damping))
            invalid(s, flag, false, 0);
    }
    if(flag == FLAG(PHYSICS_SOFT_BODY)) {
        SoftBody *b = &v->soft_body;
        reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, b->origin, 0, false, 0);
        topology_check(s, flag, b->nodes, b->node_count, SOFT_BODY_MAX_NODES, ROHR_SOFT_BODY_NODE);
        topology_check(s, flag, b->beams, b->beam_count, SOFT_BODY_MAX_BEAMS, ROHR_SOFT_BODY_BEAM);
        topology_check(s, flag, b->areas, b->area_count, SOFT_BODY_MAX_AREAS, ROHR_SOFT_BODY_AREA);
        v->soft_body_node_count = b->node_count;
        v->soft_body_beam_count = b->beam_count;
        v->soft_body_area_count = b->area_count;
        v->available |= FLAG(PHYSICS_SOFT_BODY_NODE_COUNT) | FLAG(PHYSICS_SOFT_BODY_BEAM_COUNT) |
            FLAG(PHYSICS_SOFT_BODY_AREA_COUNT);
    }
    if(flag == FLAG(PHYSICS_SOFT_BODY_NODE)) {
        if(v->soft_body_node.soft_body) reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, v->soft_body_node.soft_body,
            ROHR_SOFT_BODY, false, 0);
        if(!isfinite(v->soft_body_node.radius) || v->soft_body_node.radius <= 0) invalid(s, flag, false, 0);
    }
    if(flag == FLAG(PHYSICS_SOFT_BODY_BEAM)) {
        SoftBodyBeam *b = &v->soft_body_beam;
        if(b->soft_body) reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, b->soft_body, ROHR_SOFT_BODY, false, 0);
        bool a = reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_BEAM_START, b->node_a,
            ROHR_SOFT_BODY_NODE, false, 0);
        bool z = reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_BEAM_END, b->node_b,
            ROHR_SOFT_BODY_NODE, false, 0);
        bool valid_a = a && transform_check(s, flag, b->node_a, ROHR_ENTITY_INFO_FIELD_BEAM_START, false, 0, false);
        bool valid_b = z && transform_check(s, flag, b->node_b, ROHR_ENTITY_INFO_FIELD_BEAM_END, false, 0, false);
        if(valid_a && valid_b) {
            PositionResult pa = physics_position_get(b->node_a), pb = physics_position_get(b->node_b);
            if(pa.kind == ERROR_RESULT_VALUE && pb.kind == ERROR_RESULT_VALUE &&
                    hypotf(pa.result.value.x - pb.result.value.x, pa.result.value.y - pb.result.value.y) <= 0.0001f)
                issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_SHAPE, ROHR_ENTITY_INFO_GEOMETRY_UNRESOLVABLE,
                    object(ROHR_ENTITY_INFO_OBJECT_ENTITY, b->node_a, 0), false, 0,
                    "Beam endpoints are too close to resolve a direction");
        }
        if(!isfinite(b->rest_length) || !isfinite(b->stiffness) || !isfinite(b->damping) ||
                !isfinite(b->collision_thickness) || !isfinite(b->friction) || !isfinite(b->restitution))
            invalid(s, flag, false, 0);
    }
    if(flag == FLAG(PHYSICS_SOFT_BODY_AREA)) {
        SoftBodyArea *a = &v->soft_body_area;
        if(a->soft_body) reference_check(s, flag, ROHR_ENTITY_INFO_FIELD_VALUE, a->soft_body, ROHR_SOFT_BODY, false, 0);
        if(a->geometry.outer.node_count < 3)
            issue_add(s, flag, ROHR_ENTITY_INFO_FIELD_SHAPE, ROHR_ENTITY_INFO_GEOMETRY_UNRESOLVABLE,
                object(ROHR_ENTITY_INFO_OBJECT_ENTITY, v->entity, 0), false, 0,
                "Area needs at least three connected outer nodes");
        topology_check(s, flag, a->geometry.outer.nodes, a->geometry.outer.node_count,
            SOFT_BODY_MAX_NODES, ROHR_SOFT_BODY_NODE);
        if(a->geometry.hole_count > SOFT_BODY_MAX_AREA_HOLES) invalid(s, flag, false, 0);
        for(size_t i = 0; i < a->geometry.hole_count && i < SOFT_BODY_MAX_AREA_HOLES; ++i)
            topology_check(s, flag, a->geometry.holes[i].nodes, a->geometry.holes[i].node_count,
                SOFT_BODY_MAX_NODES, ROHR_SOFT_BODY_NODE);
    }
    if(flag == FLAG(GRAPHICS_SPRITE)) {
        v->sprite_visible = v->sprite.visible;
        v->sprite_scale = v->sprite.scale;
        v->available |= FLAG(GRAPHICS_SPRITE_VISIBILITY) | FLAG(GRAPHICS_SPRITE_SCALE);
        resource_check(s, flag, v->sprite.texture, false, 0);
        if(!isfinite(v->sprite.scale.x) || !isfinite(v->sprite.scale.y) ||
                !vector_check(v->sprite.body_offset) || !isfinite(v->sprite.orientation_offset))
            invalid(s, flag, false, 0);
    }
}
static int issue_compare(const void *left, const void *right) {
    const EntityInfoIssue *a = left, *b = right;
#define COMPARE(field) if(a->field != b->field) return a->field < b->field ? -1 : 1
    COMPARE(section); COMPARE(index); COMPARE(field); COMPARE(reason);
    COMPARE(reference.kind); COMPARE(reference.id); COMPARE(reference.owner);
#undef COMPARE
    return 0;
}
EntityInfoResult entity_info_get(Entity entity, EntityInfoFlags flags) {
    EntityIndex index;
    if(!flags_check(flags))
        return ERROR_RESULT_MAKE_ERROR(EntityInfoResult, ERROR_ENGINE_ENTITY_INFO_INVALID);
    if(!entity_index_get(entity, &index) || !entity_index_alive_check(index))
        return ERROR_RESULT_MAKE_ERROR(EntityInfoResult, ERROR_ENGINE_INVALID_ENTITY);
    if(next_handle == 0)
        return ERROR_RESULT_MAKE_ERROR(EntityInfoResult, ERROR_MEMORY_POOL_CAPACITY_OVERFLOW);
    Snapshot *s = calloc(1, sizeof(*s));
    if(s == NULL) return ERROR_RESULT_MAKE_ERROR(EntityInfoResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    s->value.entity = entity;
    s->value.requested = flags;
    s->owners = 1;
    for(unsigned bit = 0; bit < 61 && !s->failure; ++bit) {
        EntityInfoFlags flag = UINT64_C(1) << bit;
        if(!(flags & flag)) continue;
        /* Full-sprite capture supplies the scale value, but an explicitly
         * requested scale section still needs its own filtered diagnostics. */
        if((s->value.available & flag) && flag != FLAG(GRAPHICS_SPRITE_SCALE)) continue;
        section_capture(s, index, flag);
        dependencies_check(s, flag);
    }
    if(s->failure) {
        EngineError failure = s->failure;
        snapshot_destroy(s);
        return ERROR_RESULT_MAKE_ERROR(EntityInfoResult, failure);
    }
    if(s->issue_count > 1) qsort(s->issues, s->issue_count, sizeof(*s->issues), issue_compare);
    s->value.handle = next_handle++;
    s->next = snapshots;
    snapshots = s;
    return ERROR_RESULT_MAKE_VALUE(EntityInfoResult, s->value);
}
EntityInfoResult entity_info_retain(const EntityInfo *info) {
    Snapshot *s = snapshot_get(info);
    if(s == NULL) return ERROR_RESULT_MAKE_ERROR(EntityInfoResult, ERROR_ENGINE_ENTITY_INFO_INVALID);
    if(s->owners == SIZE_MAX) return ERROR_RESULT_MAKE_ERROR(EntityInfoResult, ERROR_MEMORY_POOL_CAPACITY_OVERFLOW);
    ++s->owners;
    return ERROR_RESULT_MAKE_VALUE(EntityInfoResult, s->value);
}
EngineResult entity_info_release(EntityInfo *info) {
    if(info == NULL) return error_result_error(ERROR_ENGINE_ENTITY_INFO_INVALID);
    if(info->handle == 0) { *info = (EntityInfo){0}; return error_result_value(true); }
    Snapshot **link = &snapshots;
    while(*link && (*link)->value.handle != info->handle) link = &(*link)->next;
    if(*link == NULL) return error_result_error(ERROR_ENGINE_ENTITY_INFO_INVALID);
    Snapshot *s = *link;
    if(--s->owners == 0) { *link = s->next; snapshot_destroy(s); }
    *info = (EntityInfo){0};
    return error_result_value(true);
}
bool entity_info_check(const EntityInfo *info, EntityInfoFlags flags) {
    Snapshot *s = snapshot_get(info);
    return s != NULL && flags_check(flags) && (s->value.available & flags) == flags;
}
EntityInfoIssueCountResult entity_info_issues_count_get(const EntityInfo *info, EntityInfoFlags flags) {
    Snapshot *s = snapshot_get(info);
    if(s == NULL || !flags_check(flags))
        return ERROR_RESULT_MAKE_ERROR(EntityInfoIssueCountResult, ERROR_ENGINE_ENTITY_INFO_INVALID);
    size_t count = 0;
    for(size_t i = 0; i < s->issue_count; ++i) if(s->issues[i].section & flags) ++count;
    return ERROR_RESULT_MAKE_VALUE(EntityInfoIssueCountResult, count);
}
EntityInfoIssueReadResult entity_info_issues_get(const EntityInfo *info, EntityInfoFlags flags,
        EntityInfoIssue *destination, size_t capacity) {
    Snapshot *s = snapshot_get(info);
    if(s == NULL || !flags_check(flags) || (destination == NULL && capacity != 0))
        return ERROR_RESULT_MAKE_ERROR(EntityInfoIssueReadResult, ERROR_ENGINE_ENTITY_INFO_INVALID);
    EntityInfoIssueRead read = {0};
    for(size_t i = 0; i < s->issue_count; ++i) if(s->issues[i].section & flags) {
        ++read.total;
        if(read.written < capacity) destination[read.written++] = s->issues[i];
    }
    return ERROR_RESULT_MAKE_VALUE(EntityInfoIssueReadResult, read);
}
