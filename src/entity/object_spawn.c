/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "entity/object_visuals.h"
#include "object_internal.h"
#include "entity_internal.h"
#include "physics/soft_body/soft_body_area.h"
#include "graphics/object_drawing.h"
#include "graphics/marker_geometry.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct SpawnEntry {
    const ObjectDescription *description;
    Entity entity;
    JointAnchorId anchor;
    bool alias;
} SpawnEntry;
typedef struct SpawnContext {
    SpawnEntry *entries;
    size_t count;
    ObjectSpawnDescription options;
} SpawnContext;
static bool object_description_visible_check(const ObjectDescription *description) {
    for(const ObjectDescription *item = description; item; item = item->parent)
        if(!item->visible) return false;
    return true;
}

static CameraId instance_cameras[MAX_ENTITIES];
static JointAnchorId instance_anchors[MAX_ENTITIES];
static ObjectAppearance instance_appearance[MAX_ENTITIES];
static bool instance_appearance_used[MAX_ENTITIES];

bool object_instance_appearance_get(Entity entity, ObjectAppearance *appearance) {
    EntityIndex index;
    if(!appearance || !entity_index_get(entity, &index) || !instance_appearance_used[index]) return false;
    *appearance = instance_appearance[index]; return true;
}
EngineResult object_instance_appearance_set(Entity entity, ObjectAppearance appearance) {
    EntityIndex index;
    if(!entity_index_get(entity, &index) || appearance.kind < OBJECT_CONTAINER || appearance.kind > OBJECT_TORQUE || !object_visuals_check(appearance.visuals))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    instance_appearance[index] = appearance; instance_appearance_used[index] = true;
    return error_result_value(true);
}
bool entity_object_draw(Entity entity) {
    if(!entity_alive_check(entity)) return false;
    Entity pending[MAX_ENTITIES]; size_t count = 1; pending[0] = entity;
    bool ok = true;
    while(count) {
        Entity current = pending[--count];
        ObjectAppearance appearance = {0};
        if(object_instance_appearance_get(current, &appearance)) {
            if(!appearance.visible) continue;
            if(appearance.enabled) {
                if(appearance.kind == OBJECT_BODY || appearance.kind == OBJECT_PARTICLE) {
                    graphics_hit_box_colored_draw(current, GRAPHICS_FILLED, appearance.color);
                    graphics_hit_box_colored_draw(current, GRAPHICS_OUTLINE, appearance.border_color);
                } else if(appearance.kind == OBJECT_NODE || appearance.kind == OBJECT_BEAM || appearance.kind == OBJECT_AREA)
                    ok = graphics_soft_body_element_draw(current, appearance.color) && ok;
                else if(appearance.kind == OBJECT_JOINT && entity_components_check(current, ROHR_JOINT))
                    ok = graphics_marker_entity_draw(current, appearance.kind, appearance.visuals) && ok;
                else if(appearance.kind == OBJECT_FORCE || appearance.kind == OBJECT_TORQUE)
                    ok = graphics_marker_entity_draw(current, appearance.kind, appearance.visuals) && ok;
                if(entity_components_check(current, ROHR_SPRITE)) ok = graphics_sprite_draw(current) && ok;
                if(entity_components_check(current, ROHR_ANIMATED_SPRITE)) ok = graphics_animated_sprite_draw(current) && ok;
            }
        }
        GroupIdResult members = entity_object_members_get(current);
        if(error_check(members)) continue;
        EntityGroupResult group = entity_group_get(members.result.value);
        if(error_check(group)) return false;
        /* Match normal soft-body drawing at equal layers: fill, beams, nodes. */
        unsigned passes = appearance.kind == OBJECT_SOFT_BODY ? 4 : 1;
        for(unsigned pass = passes; pass > 0; --pass) {
            for(size_t i = group.result.value.entities.capacity; i > 0; --i) {
                if(!group.result.value.entities.used[i - 1]) continue;
                Entity child = group.result.value.entities.objects[i - 1];
                if(passes > 1) {
                    unsigned order = entity_components_check(child, ROHR_SOFT_BODY_AREA) ? 0 :
                        entity_components_check(child, ROHR_SOFT_BODY_BEAM) ? 1 :
                        entity_components_check(child, ROHR_SOFT_BODY_NODE) ? 2 : 3;
                    if(order != pass - 1) continue;
                }
                if(count >= MAX_ENTITIES) return false;
                pending[count++] = child;
            }
        }
    }
    return ok;
}

JointAnchorIdResult entity_object_anchor_get(Entity entity) {
    EntityIndex index;
    if(!entity_index_get(entity, &index))
        return ERROR_RESULT_MAKE_ERROR(JointAnchorIdResult, ERROR_ENGINE_INVALID_ENTITY);
    if(!instance_anchors[index] || error_check(physics_joint_anchor_local_position_get(instance_anchors[index])))
        return ERROR_RESULT_MAKE_ERROR(JointAnchorIdResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(JointAnchorIdResult, instance_anchors[index]);
}
Entity object_instance_anchor_owner_get(Entity entity) {
    EntityIndex index;
    if(!entity_index_get(entity, &index)) return ENTITY_INVALID;
    JointAnchorDescriptionResult description = physics_joint_anchor_description_get(instance_anchors[index]);
    return error_check(description) ? ENTITY_INVALID : description.result.value.entity;
}
EngineResult object_instance_anchor_restore(Entity entity, Entity owner, JointAnchorId anchor) {
    EntityIndex index;
    if(!entity_index_get(entity, &index) || instance_anchors[index] ||
            error_check(physics_joint_anchor_local_position_get(anchor)))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    JointAnchorListResult list = physics_joint_anchors_get(owner);
    if(error_check(list)) return error_result_error(list.result.error);
    bool found = false;
    for(uint32_t n = 0; n < list.result.value.count; ++n) found |= list.result.value.values[n] == anchor;
    if(!found) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    for(size_t n = 0; n < MAX_ENTITIES; ++n)
        if(instance_anchors[n] == anchor) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    instance_anchors[index] = anchor;
    return error_result_value(true);
}

EngineResult object_instance_camera_restore(Entity entity, CameraId camera) {
    EntityIndex index;
    if(!entity_index_get(entity, &index) || instance_cameras[index])
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    instance_cameras[index] = camera;
    return error_result_value(true);
}

void object_instance_resources_clear(EntityIndex index) {
    if(index >= MAX_ENTITIES) return;
    if(instance_cameras[index]) (void)graphics_camera_destroy(instance_cameras[index]);
    instance_cameras[index] = 0;
    if(instance_anchors[index]) (void)physics_joint_anchor_remove(instance_anchors[index]);
    instance_anchors[index] = 0;
    instance_appearance_used[index] = false; instance_appearance[index] = (ObjectAppearance){0};
}
CameraIdResult entity_object_camera_get(Entity entity) {
    EntityIndex index;
    if(!entity_index_get(entity, &index))
        return ERROR_RESULT_MAKE_ERROR(CameraIdResult, ERROR_ENGINE_INVALID_ENTITY);
    if(!instance_cameras[index])
        return ERROR_RESULT_MAKE_ERROR(CameraIdResult, ERROR_ENGINE_COMPONENT_MISSING);
    return ERROR_RESULT_MAKE_VALUE(CameraIdResult, instance_cameras[index]);
}
static SpawnEntry *entry_get(SpawnContext *c, uint64_t id) {
    if(!id) return NULL;
    for(size_t i = 0; i < c->count; ++i)
        if(c->entries[i].description->id == id) return &c->entries[i];
    return NULL;
}
static Entity target_get(SpawnContext *c, uint64_t id) {
    SpawnEntry *entry = entry_get(c, id);
    return entry ? entry->entity : ENTITY_INVALID;
}
static bool collect(SpawnContext *c, const ObjectDescription *d, unsigned depth) {
    if(!d || !d->name || !d->name[0] || strlen(d->name) >= ENTITY_NAME_MAX ||
            !d->id || entry_get(c, d->id) || depth > OBJECT_DEPTH_MAX ||
            c->count >= MAX_ENTITIES || d->child_count > MAX_ENTITIES ||
            (d->child_count && !d->children) || d->kind < OBJECT_CONTAINER || d->kind > OBJECT_TORQUE ||
            (d->categories & ~ROHR_ENTITY_OBJECT_ALL) || d->geometry_count > MAX_ENTITIES ||
            (d->hitbox_binding_count && !d->hitbox_bindings)) return false;
    if((d->kind == OBJECT_FORCE || d->kind == OBJECT_TORQUE) &&
            d->child_count) return false;
    c->entries[c->count++].description = d;
    for(size_t i = 0; i < d->child_count; ++i) {
        const ObjectDescription *child = d->children[i];
        if(!child || child->parent != d) return false;
        if(!(child->categories & c->options.flags) && !child->child_count) continue;
        if(!collect(c, child, depth + 1)) return false;
    }
    return true;
}
static EngineResult name_set(Entity entity, const ObjectDescription *d, Entity parent) {
    char name[ENTITY_NAME_MAX];
    EntityNameResult owner = entity_name_get(parent);
    for(uint32_t suffix = 0; suffix <= MAX_ENTITIES; ++suffix) {
        char tail[24] = {0};
        int n = suffix ? snprintf(tail, sizeof(tail), "_%u", suffix) : 0;
        int available = ENTITY_NAME_MAX - 1 - n;
        int prefix = available - (int)strlen(d->name) - 1;
        if(!error_check(owner) && prefix > 0)
            snprintf(name, sizeof(name), "%.*s_%s%s", prefix, owner.result.value.value, d->name, tail);
        else snprintf(name, sizeof(name), "%.*s%s", available, d->name, tail);
        EntityResult existing = entity_by_name_get(name);
        if(error_check(existing) || existing.result.value == entity) return entity_name_set(entity, name);
    }
    return error_result_error(ERROR_ENGINE_DUPLICATE_ENTITY_NAME);
}
static EngineResult body_apply(Entity entity, const ObjectDescription *d, bool node) {
#define APPLY(call) do { EngineResult r = (call); if(error_check(r)) return r; } while(0)
    APPLY(physics_mass_set(entity, d->mass));
    APPLY(physics_velocity_set(entity, d->velocity));
    APPLY(physics_acceleration_set(entity, d->acceleration));
    if(!node) APPLY(physics_angular_velocity_set(entity, d->angular_velocity));
    APPLY(physics_friction_set(entity, d->friction));
    APPLY(physics_restitution_set(entity, d->restitution));
    APPLY(physics_collision_filter_description_set(entity, d->collision_filter));
    if(d->static_body) APPLY(physics_static_set(entity));
    else APPLY(physics_dynamic_set(entity));
    if(d->gravity_enabled) APPLY(physics_gravity_enable(entity));
    else APPLY(physics_gravity_disable(entity));
    if(d->collision_enabled) APPLY(entity_components_add(entity, ROHR_COLLISION));
    else APPLY(entity_components_delete(entity, ROHR_COLLISION));
    if(d->rotation_locked && !node) {
        EntityIndex index; entity_index_get(entity, &index);
        Orientation angle = orientations[index];
        APPLY(physics_angle_lock_set(entity, angle, angle));
    }
    if(!node && d->center_of_mass_explicit) APPLY(physics_center_of_mass_local_position_set(entity, d->center_of_mass));
    return error_result_value(true);
#undef APPLY
}
static EngineResult visuals_apply(Entity entity, const ObjectDescription *d, bool attached) {
    if(d->kind == OBJECT_SPRITE) {
        TextureAssetResult loaded = graphics_texture_load((TextureDescription){d->asset_path, d->size});
        if(error_check(loaded)) return error_result_error(loaded.result.error);
        SpriteDescription sprite = (SpriteDescription){.texture = loaded.result.value, .scale = d->scale, .direction = DIRECTION_RIGHT, .follow_entity_rotation = true, .visible = true};
        sprite.body_offset = attached ? d->position : (Position){0};
        sprite.orientation_offset = attached ? d->orientation : 0;
        sprite.follow_entity_rotation = attached ? d->follow_orientation : true; sprite.visible = object_description_visible_check(d);
        EngineResult result = graphics_sprite_add(entity, sprite);
        (void)graphics_texture_release(&loaded.result.value);
        if(!error_check(result)) result = graphics_layer_sprite_set(entity, d->layer);
        return result;
    }
    if(d->kind == OBJECT_ANIMATION) {
        if((d->frame_count && !d->frames) || d->frame_count > MAX_ANIMATIONS_FRAMES)
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        AnimationDescription descriptor = {.id = (AnimationId)d->id,
            .frame_count = d->frame_count, .ticks_per_frame = d->playing ? d->ticks_per_frame : 0,
            .time_per_frame = d->playing ? d->time_per_frame : 0};
        for(size_t i = 0; i < d->frame_count; ++i) {
            descriptor.frame_ids[i] = d->frames[i].id;
            descriptor.frame_files[i] = d->frames[i].path;
        }
        AnimationAssetResult loaded = d->frame_count ? graphics_animation_load(descriptor) :
            ERROR_RESULT_MAKE_VALUE(AnimationAssetResult, ((AnimationAsset){0}));
        if(error_check(loaded)) return error_result_error(loaded.result.error);
        AnimatedSpriteDescriptionResult prepared = graphics_animated_sprite_description_from_animation_get(loaded.result.value);
        if(error_check(prepared)) {
            (void)graphics_animation_release(&loaded.result.value);
            return error_result_error(prepared.result.error);
        }
        AnimatedSpriteDescription sprite = prepared.result.value;
        sprite.scale = d->scale;
        sprite.player.ticks_per_frame = descriptor.ticks_per_frame;
        sprite.player.time_per_frame = descriptor.time_per_frame;
        sprite.body_offset = attached ? d->position : (Position){0};
        sprite.orientation_offset = attached ? d->orientation : 0;
        sprite.follow_entity_rotation = attached ? d->follow_orientation : true; sprite.visible = object_description_visible_check(d);
        sprite.direction = d->direction;
        for(size_t i = 0; i < d->frame_count; ++i) {
            sprite.frames[i].scale = d->frames[i].scale;
            sprite.frames[i].offset = d->frames[i].offset;
            sprite.frames[i].rotation = d->frames[i].rotation;
        }
        EngineResult result = graphics_animated_sprite_add(entity, sprite);
        if(!error_check(result) && sprite.frame_count > 0)
            result = graphics_animated_sprite_frame_index_set(entity, d->starting_frame);
        (void)graphics_animation_release(&loaded.result.value);
        if(!error_check(result)) result = graphics_layer_animation_set(entity, d->layer);
        return result;
    }
    return error_result_value(true);
}
static EngineResult spawn_entry(SpawnContext *c, size_t i, unsigned pass) {
    SpawnEntry *entry = &c->entries[i];
    const ObjectDescription *d = entry->description;
    if(d->kind == OBJECT_FORCE || d->kind == OBJECT_TORQUE) return error_result_value(true);
    bool physics = (d->categories & c->options.flags & ROHR_ENTITY_OBJECT_PHYSICS) != 0;
    bool visual = (d->categories & c->options.flags & ROHR_ENTITY_OBJECT_VISUALS) != 0;
    bool topology = d->kind == OBJECT_BEAM || d->kind == OBJECT_AREA;
    bool geometry_enabled = physics || visual;
    if((pass == 0 && topology && geometry_enabled) || (pass == 1 && (!topology || !geometry_enabled))) return error_result_value(true);
    Entity parent = d->parent ? target_get(c, d->parent->id) : ENTITY_INVALID;
    Position position; Orientation orientation;
    EngineResult result = object_description_world_pose_get(d, &position, &orientation);
    if(error_check(result)) return result;
    position.x += c->options.offset.x; position.y += c->options.offset.y;
    Entity target = target_get(c, d->target);
    bool attached_visual = visual && (d->kind == OBJECT_SPRITE || d->kind == OBJECT_ANIMATION) && target;
    RohrComponentMask visual_component = d->kind == OBJECT_SPRITE ? ROHR_SPRITE : ROHR_ANIMATED_SPRITE;
    if(attached_visual && !entity_components_check(target, visual_component)) {
        entry->entity = target; entry->alias = true;
        result = visuals_apply(target, d, true);
        if(error_check(result)) return result;
        return entity_object_alias_restore(parent, target, d->name);
    }
    EntityResult created;
    if(geometry_enabled && d->kind == OBJECT_PARTICLE) created = physics_particle_add((ParticleDescription){
        .owner=parent,
        .position=position,.local_origin=d->particle_origin,.radius=d->radius,.rigid_vertices=d->particle_vertices,
        .velocity=d->velocity,.acceleration=d->acceleration,.mass_value=d->mass,.friction=d->friction,
        .restitution=d->restitution,.collision_filter=d->collision_filter,.static_body=!physics || d->static_body,
        .gravity_enabled=physics && d->gravity_enabled,.collision_enabled=physics && d->collision_enabled});
    else if(geometry_enabled && d->kind == OBJECT_SOFT_BODY) created = physics_soft_body_add(
        (SoftBodyDescription){.owner = parent, .position = position, .orientation = orientation});
    else if(geometry_enabled && d->kind == OBJECT_NODE) created = physics_soft_body_node_add((SoftBodyNodeDescription){
        .owner = parent, .position = position, .mass = physics ? d->mass : 1, .radius = d->radius,
        .velocity = d->velocity, .acceleration = d->acceleration, .friction = d->friction,
        .restitution = d->restitution, .collision_filter = d->collision_filter,
        .static_body = !physics || d->static_body, .gravity_enabled = physics && d->gravity_enabled,
        .collision_enabled = physics && d->collision_enabled,
        .draw_color = d->color, .draw_color_overridden = d->color_overridden});
    else if(geometry_enabled && d->kind == OBJECT_BEAM) created = physics_soft_body_beam_add((SoftBodyBeamDescription){
        .owner = parent, .node_a = target_get(c, d->endpoint_a), .node_b = target_get(c, d->endpoint_b),
        .stiffness = physics ? d->stiffness : 0, .damping = physics ? d->damping : 0,
        .rest_length = d->rest_length, .rest_length_explicit = true, .friction = d->friction, .restitution = d->restitution,
        .collision = {.enabled = physics && d->collision_enabled, .thickness = d->thickness, .filter = d->collision_filter},
        .draw_color = d->color, .draw_color_overridden = d->color_overridden});
    else if(geometry_enabled && d->kind == OBJECT_AREA) {
        SoftBodyAreaGeometry geometry = {0};
        if(d->area) {
            if(d->area->hole_count > SOFT_BODY_MAX_AREA_HOLES) return error_result_error(ERROR_ENGINE_STATE_INVALID);
            geometry.hole_count = d->area->hole_count;
            for(uint32_t l = 0; l <= geometry.hole_count; ++l) {
                const ObjectAreaLoop *source = l ? &d->area->holes[l - 1] : &d->area->outer;
                SoftBodyAreaLoop *loop = l ? &geometry.holes[l - 1] : &geometry.outer;
                if(source->node_count > SOFT_BODY_MAX_NODES) return error_result_error(ERROR_ENGINE_STATE_INVALID);
                loop->node_count = source->node_count;
                for(uint32_t j = 0; j < loop->node_count; ++j) loop->nodes[j] = target_get(c, source->nodes[j]);
            }
        }
        created = physics_soft_body_area_add((SoftBodyAreaDescription){.owner = parent, .geometry = geometry,
            .visible = d->visible, .draw_color = d->color, .draw_color_overridden = d->color_overridden});
    } else created = entity_add();
    if(error_check(created)) return error_result_error(created.result.error);
    entry->entity = created.result.value;
    Entity e = entry->entity;
#define APPLY(call) do { result = (call); if(error_check(result)) return result; } while(0)
    APPLY(name_set(e, d, parent));
    APPLY(object_instance_appearance_set(e, (ObjectAppearance){.kind = d->kind, .visuals = d->visuals, .color = d->color,
        .border_color = d->border_color, .visible = d->visible, .enabled = visual}));
    if(!geometry_enabled || d->kind != OBJECT_SOFT_BODY) {
        APPLY(physics_position_set(e, position));
        APPLY(physics_orientation_set(e, d->kind == OBJECT_PARTICLE ? 0 : orientation));
    }
    if(d->child_count || d->kind == OBJECT_CONTAINER) APPLY(entity_object_initialize(e));
    if(parent) APPLY(entity_object_member_restore(parent, e, d->name));
    if(geometry_enabled && d->kind == OBJECT_BODY) {
        if(d->geometry_count && (!d->geometry || d->active_geometry >= d->geometry_count))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        for(size_t n = 0; n < d->geometry_count; ++n) {
            APPLY(physics_hitbox_add(e, d->geometry[n].shape));
            APPLY(physics_hitbox_id_at_set(e, n, d->geometry[n].id));
        }
        if(d->geometry_count) APPLY(physics_hitbox_active_index_set(e, d->active_geometry));
        if(d->radius > 0) {
            APPLY(physics_particle_origin_set(e, d->particle_origin));
            APPLY(physics_particle_radius_set(e, d->radius));
        }
        if(physics) APPLY(body_apply(e, d, false));
        else { APPLY(physics_static_set(e)); APPLY(entity_components_delete(e, ROHR_COLLISION | ROHR_GRAVITY)); }
    }
    if(physics && d->kind == OBJECT_NODE) APPLY(body_apply(e, d, true));
    if(physics && d->kind == OBJECT_ANCHOR) {
        JointAnchorIdResult anchor = physics_joint_anchor_add((JointAnchorDescription){.entity = target ? target : e, .local_offset = target ? d->position : (Position){0}});
        if(error_check(anchor)) return error_result_error(anchor.result.error);
        entry->anchor = anchor.result.value;
        APPLY(object_instance_anchor_restore(e, target ? target : e, anchor.result.value));
        if(target) {
            APPLY(physics_dynamic_set(e));
            APPLY(physics_velocity_set(e, (Velocity){0}));
            APPLY(physics_transform_lock_description_set(e, (TransformLockDescription){
                .driver = target, .local_offset = d->position, .local_angle = d->orientation,
                .lock_position = true, .lock_orientation = d->follow_orientation, .inherit_velocity = true}));
        }
    }
    if(visual) {
        APPLY(graphics_layer_entity_set(e, d->layer));
        APPLY(visuals_apply(e, d, false));
        /* A component stores one sprite of each kind. Keep additional attached
         * descriptions as owned entities instead of replacing the first. */
        if(attached_visual) {
            APPLY(physics_dynamic_set(e));
            APPLY(physics_velocity_set(e, (Velocity){0}));
            APPLY(physics_transform_lock_description_set(e, (TransformLockDescription){
                .driver = target, .local_offset = d->position, .local_angle = d->orientation,
                .lock_position = true, .lock_orientation = d->follow_orientation, .inherit_velocity = true}));
        }
        if(d->kind == OBJECT_CAMERA) {
            CameraIdResult camera = graphics_camera_create((CameraDescription){
                .position = position, .orientation = orientation,
                .dimensions = {d->size.x, d->size.y}, .zoom = d->zoom,
                .enabled = object_description_visible_check(d)});
            if(error_check(camera)) return error_result_error(camera.result.error);
            EntityIndex index; entity_index_get(e, &index); instance_cameras[index] = camera.result.value;
            if(target) APPLY(graphics_camera_attachment_description_set(camera.result.value, (CameraAttachmentDescription){.entity = target, .position_offset = d->position, .orientation_offset = d->orientation, .follow_position = d->follow_position, .follow_orientation = d->follow_orientation}));
        }
    }
    return error_result_value(true);
#undef APPLY
}

typedef struct SourceScope {
    SpawnContext *spawn;
    const ObjectDescription *object;
} SourceScope;
static bool source_reference_check(const void *scope, ObjectSourceReference ref) {
    const SourceScope *s = scope;
    return object_source_reference_check(s->object, ref) &&
        (ref.kind == OBJECT_SOURCE_NONE || target_get(s->spawn, ref.id) != ENTITY_NONE);
}
static ForceLocation source_location_get(SpawnContext *c, ObjectForceLocation v) {
    ForceLocation result = {.offset = v.offset, .reference.kind = v.kind};
    if(v.kind == ROHR_FORCE_BEAM_POSITION) {
        result.reference.beam.entity = target_get(c, v.reference.id);
        result.reference.beam.anchor = v.anchor;
        result.reference.beam.fraction = v.fraction;
    } else if(v.kind != ROHR_FORCE_WORLD_POSITION)
        result.reference.entity = target_get(c, v.reference.id);
    return result;
}
static EngineResult source_spawn(SpawnContext *c, SpawnEntry *entry) {
    const ObjectDescription *d = entry->description;
    if((d->kind != OBJECT_FORCE && d->kind != OBJECT_TORQUE) ||
            !(d->categories & c->options.flags & ROHR_ENTITY_OBJECT_PHYSICS))
        return error_result_value(true);
    Entity parent = d->parent ? target_get(c, d->parent->id) : ENTITY_NONE;
    if(!parent || d->parent->kind != OBJECT_CONTAINER || d->child_count)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    SourceScope scope = {c, d->parent};
    EntityResult created;
    ObjectSourceMode mode;
    if(d->kind == OBJECT_FORCE) {
        ObjectForceDescription v = d->force;
        if(!object_source_force_check(&scope, v, source_reference_check))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        ForceDescription value = {.owner = parent, .target = target_get(c, v.target.id),
            .magnitude = v.magnitude, .location = source_location_get(c, v.location),
            .orientation = {.offset = v.orientation.offset, .reference.kind = v.orientation.kind}};
        if(v.orientation.kind == ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION) {
            value.orientation.reference.between.from = source_location_get(c, v.orientation.from);
            value.orientation.reference.between.to = source_location_get(c, v.orientation.to);
        } else if(v.orientation.kind != ROHR_FORCE_WORLD_ORIENTATION)
            value.orientation.reference.entity = target_get(c, v.orientation.reference.id);
        created = physics_force_add(value);
        mode = v.mode;
    } else {
        ObjectTorqueDescription v = d->torque;
        if(!object_source_torque_check(&scope, v, source_reference_check))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        created = physics_torque_add((TorqueDescription){.owner = parent,
            .target = target_get(c, v.target.id), .magnitude = v.magnitude});
        mode = v.mode;
    }
    if(error_check(created)) return error_result_error(created.result.error);
    entry->entity = created.result.value;
    EngineResult result = object_instance_appearance_set(entry->entity, (ObjectAppearance){
        .kind=d->kind, .visuals=d->visuals, .color=d->color, .visible=d->visible,
        .enabled=(d->categories & c->options.flags & ROHR_ENTITY_OBJECT_VISUALS)!=0});
    if(!error_check(result)) result = graphics_layer_entity_set(entry->entity, d->layer);
    if(!error_check(result)) result = name_set(entry->entity, d, parent);
    if(!error_check(result)) result = entity_object_member_restore(parent, entry->entity, d->name);
    if(!error_check(result) && mode == OBJECT_SOURCE_INITIAL)
        result = entity_lifetime_description_set(entry->entity, (EntityLifetimeDescription){
            .expiry_tick = engine_tick_get() + 1, .tick_enabled = true});
    return result;
}
EntityResult entity_object_description_add(const ObjectDescription *description, ObjectSpawnDescription options) {
    if(!description || (!(description->categories & options.flags) && !description->child_count) ||
            !object_flags_check(options.flags) || !isfinite(options.offset.x) || !isfinite(options.offset.y))
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    SpawnContext c = {.options = options};
    c.entries = calloc(MAX_ENTITIES, sizeof(*c.entries));
    if(!c.entries) return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    EngineResult result = error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(!collect(&c, description, 0)) goto fail;
    for(unsigned pass = 0; pass < 2; ++pass)
        for(size_t i = 0; i < c.count; ++i) {
            result = spawn_entry(&c, i, pass);
            if(error_check(result)) goto fail;
        }
    for(size_t i = 0; i < c.count; ++i) {
        const ObjectDescription *d = c.entries[i].description;
        if((d->kind == OBJECT_BODY || d->kind == OBJECT_PARTICLE) &&
                (d->categories & options.flags & ROHR_ENTITY_OBJECT_PHYSICS)) {
            for(size_t binding = 0; binding < d->hitbox_binding_count; ++binding) {
                const HitboxAnimationBinding *b = &d->hitbox_bindings[binding];
                result = physics_hitbox_animation_binding_set(c.entries[i].entity,
                    b->animation_id, b->frame_id, b->hitbox_id);
                if(error_check(result)) goto fail;
            }
        }
        if(d->kind != OBJECT_JOINT || !(d->categories & options.flags & ROHR_ENTITY_OBJECT_PHYSICS)) continue;
        SpawnEntry *a = entry_get(&c, d->endpoint_a), *b = entry_get(&c, d->endpoint_b);
        JointAnchorId anchor_a = a ? a->anchor : JOINT_ANCHOR_INVALID;
        JointAnchorId anchor_b = b ? b->anchor : JOINT_ANCHOR_INVALID;
        EntityResult owner = entity_owner_get(c.entries[i].entity);
        if(error_check(owner)) { result = error_result_error(owner.result.error); goto fail; }
        result = physics_joint_description_set(c.entries[i].entity, (JointDescription){
            .owner = owner.result.value, .type = d->joint_type,
            .anchor_a = anchor_a, .anchor_b = anchor_b,
            .rest_length = d->rest_length, .rest_length_explicit = true,
            .stiffness = d->stiffness, .damping = d->damping,
            .rest_angle = d->rest_angle, .rest_angle_explicit = true});
        if(error_check(result)) goto fail;
    }
    /* All bodies, topology, and joint bindings exist before source validation. */
    for(size_t i = 0; i < c.count; ++i) {
        result = source_spawn(&c, &c.entries[i]);
        if(error_check(result)) goto fail;
    }
    Entity root = c.entries[0].entity;
    free(c.entries);
    return ERROR_RESULT_MAKE_VALUE(EntityResult, root);
fail:
    for(size_t i = c.count; i > 0; --i)
        if(!c.entries[i - 1].alias && entity_alive_check(c.entries[i - 1].entity))
            (void)entity_delete(c.entries[i - 1].entity);
    free(c.entries);
    return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
}
EntityResult entity_object_description_by_name_add(const ObjectDescription *scope, const char *name, ObjectSpawnDescription options) {
    ObjectDescriptionResult found = object_description_name_get(scope, name);
    if(error_check(found)) return ERROR_RESULT_MAKE_ERROR(EntityResult, found.result.error);
    return entity_object_description_add(found.result.value, options);
}
