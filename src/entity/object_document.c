/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "object_internal.h"
#include "editor_project.h"
#include "physics/soft_body/soft_body_motion.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct ImportEntry {
    ObjectDescription *description;
    const EditorObject *object;
    const EditorSoftBody *soft_body;
    uint32_t source_id;
} ImportEntry;
typedef struct ImportContext {
    ImportEntry *entries;
    size_t count;
    const EditorProject *project;
    const char *path;
} ImportContext;

static ObjectDescription *find(ImportContext *c, const EditorObject *object,
        const EditorSoftBody *soft, ObjectKind kind, uint32_t id) {
    if(!id) return NULL;
    for(size_t i = 0; i < c->count; ++i) {
        ImportEntry *e = &c->entries[i];
        if(e->object == object && e->soft_body == soft && e->source_id == id &&
                (e->description->kind == kind ||
                 (kind == OBJECT_BODY && e->description->kind == OBJECT_PARTICLE)))
            return e->description;
    }
    return NULL;
}
static ObjectDescription *add(ImportContext *c, ObjectDescription *parent,
        const EditorObject *object, const EditorSoftBody *soft, ObjectKind kind,
        uint32_t id, const char *name) {
    if(c->count >= MAX_ENTITIES || !name || !name[0]) return NULL;
    ObjectDescription *d = calloc(1, sizeof(*d));
    if(!d) return NULL;
    d->name = object_string_copy(name);
    if(!d->name) { free(d); return NULL; }
    d->id = c->count + 1; d->parent = parent; d->kind = kind;
    d->categories = (kind == OBJECT_SPRITE || kind == OBJECT_ANIMATION || kind == OBJECT_CAMERA) ? ROHR_ENTITY_OBJECT_VISUALS : ROHR_ENTITY_OBJECT_PHYSICS;
    if(kind <= OBJECT_AREA || kind == OBJECT_JOINT || kind == OBJECT_FORCE || kind == OBJECT_TORQUE)
        d->categories = ROHR_ENTITY_OBJECT_ALL;
    d->visible = true; d->scale = (Scale){1, 1};
    c->entries[c->count++] = (ImportEntry){d, object, soft, id};
    return d;
}
static int layer(const EditorProject *project, EditorGraphicsLayerBinding binding) {
    for(size_t i = 0; i < project->graphics_layer_count; ++i)
        if(binding.layer && project->graphics_layers[i].id == binding.layer)
            return project->graphics_layers[i].value;
    return binding.value;
}
static char *asset_path(const char *document_path, const char *asset) {
    if(!asset || !asset[0]) return NULL;
    if(asset[0] == '/' || asset[0] == '\\' || (strlen(asset) > 1 && asset[1] == ':'))
        return object_string_copy(asset);
    const char *end = strrchr(document_path, '/');
    const char *back = strrchr(document_path, '\\');
    if(back && (!end || back > end)) end = back;
    size_t prefix = end ? (size_t)(end - document_path) + 1 : 0;
    /* Editor project assets are rooted beside the objects directory. */
    if(end && !strcmp(end + 1, "project.rohr.json") && prefix >= 8 &&
            (!strncmp(document_path + prefix - 8, "objects/", 8) ||
             !strncmp(document_path + prefix - 8, "objects\\", 8))) prefix -= 8;
    char *result = malloc(prefix + strlen(asset) + 1);
    if(result) { memcpy(result, document_path, prefix); strcpy(result + prefix, asset); }
    return result;
}
static bool body_read(ImportContext *c, ObjectDescription *root, const EditorObject *o, const EditorRigidBody *s) {
    ObjectDescription *d = add(c, root, o, NULL, s->standalone_particle ? OBJECT_PARTICLE : OBJECT_BODY, s->id, s->name);
    if(!d) return false;
    d->position = s->position; d->orientation = s->rotation;
    d->mass = s->mass_value; d->velocity = s->initial_velocity;
    d->acceleration = s->initial_acceleration; d->angular_velocity = s->initial_angular_velocity;
    d->friction = s->friction; d->restitution = s->restitution;
    d->static_body = s->static_body; d->gravity_enabled = s->gravity_enabled;
    d->collision_enabled = s->collision_enabled; d->rotation_locked = s->rotation_locked;
    d->radius = s->particle ? s->particle_radius : 0; d->particle_origin = s->particle_origin;
    d->particle_vertices = s->particle_rigid_vertices; d->visible = !s->app_hidden;
    d->center_of_mass_explicit = s->center_of_mass_explicit; d->center_of_mass = s->center_of_mass_offset;
    d->collision_filter = (CollisionFilterConfig){s->collision_category, s->collision_with};
    d->color = graphics_color_hex_create(s->standalone_particle ? s->particle_fill_color : s->surface_color);
    d->border_color = graphics_color_hex_create(s->standalone_particle ? s->particle_ring_color : s->border_color);
    d->layer = layer(c->project, s->graphics_layer);
    if(s->hitbox_count) {
        ObjectHitbox *geometry = calloc(s->hitbox_count, sizeof(*geometry));
        if(!geometry) return false;
        d->geometry = geometry; d->geometry_count = s->hitbox_count; d->active_geometry = s->active_hitbox_index;
        for(size_t i = 0; i < s->hitbox_count; ++i) {
            geometry[i].id = s->hitboxes[i].id;
            geometry[i].shape.amount_of_vertices = s->hitboxes[i].vertex_count;
            for(uint32_t v = 0; v < s->hitboxes[i].vertex_count; ++v)
                geometry[i].shape.vertices[v] = s->hitboxes[i].vertices[v].position;
        }
    }
    if(s->hitbox_animation_binding_count) {
        HitboxAnimationBinding *bindings = calloc(s->hitbox_animation_binding_count, sizeof(*bindings));
        if(!bindings) return false;
        d->hitbox_bindings = bindings; d->hitbox_binding_count = s->hitbox_animation_binding_count;
        for(size_t i = 0; i < s->hitbox_animation_binding_count; ++i)
            bindings[i] = (HitboxAnimationBinding){s->hitbox_animation_bindings[i].animation,
                s->hitbox_animation_bindings[i].frame, s->hitbox_animation_bindings[i].hitbox};
    }
    return true;
}
static bool soft_read(ImportContext *c, ObjectDescription *root, const EditorObject *o, const EditorSoftBody *s) {
    ObjectDescription *d = add(c, root, o, NULL, OBJECT_SOFT_BODY, s->id, s->name);
    if(!d) return false;
    d->position = s->position; d->orientation = s->rotation; d->velocity = s->initial_velocity;
    d->acceleration = s->initial_acceleration; d->angular_velocity = s->initial_angular_velocity;
    d->visible = !s->app_hidden; d->layer = layer(c->project, s->graphics_layer);
    Position points[SOFT_BODY_MAX_NODES]; Mass masses[SOFT_BODY_MAX_NODES]; Velocity motion_values[SOFT_BODY_MAX_NODES];
    for(size_t i = 0; i < s->node_count; ++i) {
        points[i] = math_vector_rotate(s->nodes[i].position, s->rotation);
        masses[i] = s->nodes[i].node_mass; motion_values[i] = s->initial_velocity;
        d->mass += masses[i];
    }
    if(s->node_count && s->initial_angular_velocity != 0 &&
            !physics_soft_angular_velocities_get(points, masses, motion_values, (uint32_t)s->node_count,
                s->initial_angular_velocity, motion_values)) return false;
    for(size_t i = 0; i < s->node_count; ++i) {
        const EditorSoftNode *n = &s->nodes[i];
        ObjectDescription *v = add(c, d, o, s, OBJECT_NODE, n->id, n->name);
        if(!v) return false;
        v->position = n->position; v->mass = n->node_mass; v->radius = n->radius;
        v->velocity = n->initial_motion_inherited ? motion_values[i] : n->initial_velocity;
        v->acceleration = n->initial_motion_inherited ? s->initial_acceleration : n->initial_acceleration;
        v->friction = n->friction; v->restitution = n->restitution;
        v->gravity_enabled = n->gravity_enabled; v->collision_enabled = n->collision_enabled;
        v->collision_filter = (CollisionFilterConfig){n->collision_category, n->collision_with};
        v->visible = !n->app_hidden; v->color = graphics_color_hex_create(n->color_overridden ? n->color : s->node_color);
        v->color_overridden = true;
        v->layer = n->graphics_layer_inherited ? d->layer : layer(c->project, n->graphics_layer);
    }
    for(size_t i = 0; i < s->beam_count; ++i) {
        const EditorSoftBeam *b = &s->beams[i];
        ObjectDescription *v = add(c, d, o, s, OBJECT_BEAM, b->id, b->name);
        ObjectDescription *a = find(c, o, s, OBJECT_NODE, b->node_a), *z = find(c, o, s, OBJECT_NODE, b->node_b);
        if(!v || !a || !z) return false;
        v->endpoint_a = a->id; v->endpoint_b = z->id;
        v->rest_length = math_vector_magnitude(math_vector_subtract(a->position, z->position));
        v->stiffness = b->stiffness; v->damping = b->damping;
        v->friction = b->friction; v->restitution = b->restitution; v->thickness = b->collision_thickness;
        v->collision_enabled = b->collision_enabled;
        v->collision_filter = (CollisionFilterConfig){b->collision_category, b->collision_with};
        v->visible = !b->app_hidden; v->color_overridden = true;
        v->color = graphics_color_hex_create(b->color_overridden ? b->color : s->beam_color);
        v->layer = b->graphics_layer_inherited ? d->layer : layer(c->project, b->graphics_layer);
    }
    for(size_t i = 0; i < s->area_count; ++i) {
        const EditorSoftArea *a = &s->areas[i];
        ObjectDescription *v = add(c, d, o, s, OBJECT_AREA, a->id, a->name);
        if(!v) return false;
        ObjectAreaGeometry *geometry = calloc(1, sizeof(*geometry));
        if(!geometry) return false;
        v->area = geometry; geometry->hole_count = a->hole_count;
        for(uint32_t l = 0; l <= a->hole_count; ++l) {
            const EditorSoftAreaLoop *source = l ? &a->holes[l - 1].loop : &a->outer;
            ObjectAreaLoop *target = l ? &geometry->holes[l - 1] : &geometry->outer;
            target->node_count = source->node_count;
            for(uint32_t j = 0; j < source->node_count; ++j) {
                ObjectDescription *node = find(c, o, s, OBJECT_NODE, source->nodes[j]);
                if(!node) return false;
                target->nodes[j] = node->id;
            }
        }
        v->visible = !a->app_hidden; v->color_overridden = true; v->color = graphics_color_hex_create(a->color);
        v->layer = a->graphics_layer_inherited ? d->layer : layer(c->project, a->graphics_layer);
    }
    return true;
}
static ObjectDescription *node_find(ImportContext *c, const EditorObject *o, uint32_t body, uint32_t node) {
    for(size_t i = 0; i < o->soft_body_count; ++i)
        if(o->soft_body_items[i].id == body) return find(c, o, &o->soft_body_items[i], OBJECT_NODE, node);
    return NULL;
}
static bool source_reference_import(ImportContext *c, const EditorObject *o, ObjectSourceReference *r) {
    if(r->kind == OBJECT_SOURCE_NONE) return true;
    ObjectKind kinds[] = {OBJECT_CONTAINER, OBJECT_BODY, OBJECT_PARTICLE, OBJECT_SOFT_BODY, OBJECT_NODE, OBJECT_BEAM};
    if(r->kind < OBJECT_SOURCE_BODY || r->kind > OBJECT_SOURCE_BEAM) return false;
    const EditorSoftBody *soft = NULL;
    if(r->soft_body) for(size_t i = 0; i < o->soft_body_count; ++i)
        if(o->soft_body_items[i].id == r->soft_body) soft = &o->soft_body_items[i];
    ObjectDescription *d = find(c, o, soft, kinds[r->kind], (uint32_t)r->id);
    if(!d) return false;
    r->id = d->id;
    if(soft) {
        ObjectDescription *body = find(c, o, NULL, OBJECT_SOFT_BODY, soft->id);
        if(!body) return false;
        r->soft_body = body->id;
    }
    return true;
}
static bool source_location_import(ImportContext *c, const EditorObject *o, ObjectForceLocation *l) {
    return l->kind == ROHR_FORCE_WORLD_POSITION || source_reference_import(c, o, &l->reference);
}
static bool sources_import(ImportContext *c, ObjectDescription *root, const EditorObject *o) {
    if(!editor_sources_check(o)) return false;
    for(size_t i = 0; i < o->force_count; ++i) {
        const EditorForce *f = &o->force_items[i];
        ObjectDescription *d = add(c, root, o, NULL, OBJECT_FORCE, f->id, f->name);
        if(!d) return false;
        d->force = f->description;
        d->visible = !f->app_hidden; d->color = (Color){240, 180, 70, 255};
        ObjectForceOrientation *v = &d->force.orientation;
        if(!source_reference_import(c,o,&d->force.target) || !source_location_import(c,o,&d->force.location)) return false;
        if(v->kind == ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION) {
            if(!source_location_import(c,o,&v->from) || !source_location_import(c,o,&v->to)) return false;
        } else if(v->kind != ROHR_FORCE_WORLD_ORIENTATION && !source_reference_import(c,o,&v->reference)) return false;
    }
    for(size_t i = 0; i < o->torque_count; ++i) {
        const EditorTorque *t = &o->torque_items[i];
        ObjectDescription *d = add(c,root,o,NULL,OBJECT_TORQUE,t->id,t->name);
        if(!d) return false;
        d->torque = t->description;
        d->visible = !t->app_hidden; d->color = (Color){240, 180, 70, 255};
        if(!source_reference_import(c,o,&d->torque.target)) return false;
    }
    return true;
}
static bool attachments_read(ImportContext *c, ObjectDescription *root, const EditorObject *o) {
    for(size_t i = 0; i < o->anchor_count; ++i) {
        const EditorAnchor *s = &o->anchors[i];
        ObjectDescription *target = s->attachment_kind == EDITOR_ANCHOR_ATTACHMENT_SOFT_NODE ?
            node_find(c, o, s->attachment_soft_body, s->attachment_soft_node) :
            find(c, o, NULL, OBJECT_BODY, s->rigid_body);
        if(!s->position_follows_body) target = NULL;
        ObjectDescription *d = add(c, target ? target : root, o, NULL, OBJECT_ANCHOR, s->id, s->name);
        if(!d) return false;
        d->target = target ? target->id : 0; d->position = s->position; d->orientation = s->rotation;
        d->follow_position = s->position_follows_body; d->follow_orientation = s->rotation_follows_body;
    }
    for(size_t i = 0; i < o->joint_count; ++i) {
        const EditorJoint *s = &o->joint_items[i];
        ObjectDescription *d = add(c, root, o, NULL, OBJECT_JOINT, s->id, s->name);
        ObjectDescription *a = find(c, o, NULL, OBJECT_ANCHOR, s->anchor_a), *b = find(c, o, NULL, OBJECT_ANCHOR, s->anchor_b);
        if(!d || !a || !b) return false;
        d->endpoint_a = a->id; d->endpoint_b = b->id;
        d->color = (Color){225, 190, 85, 255};
        d->joint_type = s->kind == EDITOR_JOINT_REVOLUTE ? JOINT_PIN : s->kind == EDITOR_JOINT_WELD ? JOINT_WELD : JOINT_SPRING;
        d->rest_length = s->rest_length; d->stiffness = s->stiffness; d->damping = s->damping; d->rest_angle = s->rest_angle;
        d->visible = !s->app_hidden; d->layer = layer(c->project, s->graphics_layer);
    }
    for(size_t i = 0; i < o->sprite_count; ++i) {
        const EditorSprite *s = &o->sprites[i];
        ObjectDescription *target = find(c, o, NULL, OBJECT_BODY, s->rigid_body);
        ObjectDescription *d = add(c, target ? target : root, o, NULL, OBJECT_SPRITE, s->id, s->name);
        if(!d) return false;
        d->target = target ? target->id : 0; d->position = s->position; d->orientation = s->rotation;
        d->size = s->size; d->follow_orientation = s->follow_body_rotation; d->visible = !s->app_hidden;
        d->asset_path = asset_path(c->path, s->path); d->layer = layer(c->project, s->graphics_layer);
        if(!d->asset_path) return false;
    }
    for(size_t i = 0; i < o->animated_sprite_count; ++i) {
        const EditorAnimatedSprite *s = &o->animated_sprite_items[i];
        ObjectDescription *target = find(c, o, NULL, OBJECT_BODY, s->rigid_body);
        ObjectDescription *d = add(c, target ? target : root, o, NULL, OBJECT_ANIMATION, s->id, s->name);
        if(!d) return false;
        d->target = target ? target->id : 0; d->position = s->editor_position; d->orientation = s->editor_rotation;
        d->scale = s->scale; d->ticks_per_frame = s->ticks_per_frame; d->time_per_frame = s->time_per_frame;
        d->starting_frame = s->starting_frame; d->direction = s->direction; d->playing = s->playing;
        d->follow_orientation = s->follow_body_rotation; d->visible = !s->app_hidden;
        d->layer = layer(c->project, s->graphics_layer);
        ObjectFrame *frames = calloc(s->frame_count, sizeof(*frames));
        if(s->frame_count && !frames) return false;
        d->frames = frames; d->frame_count = s->frame_count;
        for(size_t j = 0; j < s->frame_count; ++j) {
            frames[j] = (ObjectFrame){.id = s->frames[j].id, .scale = s->frames[j].scale,
                .offset = s->frames[j].offset, .rotation = s->frames[j].rotation,
                .path = asset_path(c->path, s->frames[j].path)};
            if(!frames[j].path) return false;
        }
    }
    for(size_t i = 0; i < o->camera_count; ++i) {
        const EditorCamera *s = &o->cameras[i];
        ObjectDescription *d = add(c, root, o, NULL, OBJECT_CAMERA, s->id, s->name), *target = NULL;
        if(!d) return false;
        if(s->attachment_kind == EDITOR_CAMERA_ATTACHMENT_RIGID_BODY) target = find(c, o, NULL, OBJECT_BODY, s->attachment);
        if(s->attachment_kind == EDITOR_CAMERA_ATTACHMENT_SOFT_BODY) target = find(c, o, NULL, OBJECT_SOFT_BODY, s->attachment);
        if(s->attachment_kind == EDITOR_CAMERA_ATTACHMENT_SOFT_NODE) target = node_find(c, o, s->attachment_soft_body, s->attachment);
        if(s->attachment_kind == EDITOR_CAMERA_ATTACHMENT_ANCHOR) target = find(c, o, NULL, OBJECT_ANCHOR, s->attachment);
        d->target = target ? target->id : 0; d->position = s->position; d->orientation = s->rotation;
        d->size = s->dimensions; d->zoom = s->zoom; d->visible = !s->app_hidden;
        d->follow_position = target != NULL; d->follow_orientation = s->inherit_orientation;
    }
    return true;
}

/* The editor and runtime share the checked document reader; conversion owns all
 * copied storage and never applies document-wide physics or viewport settings. */
ObjectDescriptionResult object_description_project_read(const EditorProject *project, const char *path) {
    ImportContext c = {.project = project, .path = path};
    c.entries = calloc(MAX_ENTITIES, sizeof(*c.entries));
    ObjectDescription *root = c.entries ? add(&c, NULL, NULL, NULL, OBJECT_CONTAINER, 0, "objects") : NULL;
    if(!root) goto fail;
    for(size_t i = 0; i < project->object_count; ++i) {
        const EditorObject *o = &project->objects[i];
        ObjectDescription *d = add(&c, root, o, NULL, OBJECT_CONTAINER, o->id, o->name);
        if(!d) goto fail;
        d->position = o->position; d->visible = !o->app_hidden;
        for(size_t j = 0; j < o->rigid_body_count; ++j) if(!body_read(&c, d, o, &o->rigid_bodies[j])) goto fail;
        for(size_t j = 0; j < o->soft_body_count; ++j) if(!soft_read(&c, d, o, &o->soft_body_items[j])) goto fail;
        if(!attachments_read(&c, d, o) || !sources_import(&c, d, o)) goto fail;
        for(size_t j = 0; j < o->rigid_body_count; ++j) {
            ObjectDescription *body = find(&c, o, NULL, OBJECT_BODY, o->rigid_bodies[j].id);
            HitboxAnimationBinding *bindings = (HitboxAnimationBinding *)body->hitbox_bindings;
            for(size_t b = 0; b < body->hitbox_binding_count; ++b) {
                ObjectDescription *animation = find(&c, o, NULL, OBJECT_ANIMATION, bindings[b].animation_id);
                if(!animation) goto fail;
                bindings[b].animation_id = (AnimationId)animation->id;
            }
        }
        /* Existing body coordinates are object-local, even with ECS parenting.
         * Convert to parent-local coordinates without moving existing projects. */
        for(size_t j = 0; j < o->rigid_body_count; ++j) {
            const EditorRigidBody *body = &o->rigid_bodies[j];
            if(!body->parent) continue;
            ObjectDescription *child = find(&c, o, NULL, OBJECT_BODY, body->id);
            ObjectDescription *parent = find(&c, o, NULL, OBJECT_BODY, body->parent);
            const EditorRigidBody *source_parent = NULL;
            for(size_t k = 0; k < o->rigid_body_count; ++k)
                if(o->rigid_bodies[k].id == body->parent) source_parent = &o->rigid_bodies[k];
            if(!child || !parent || !source_parent) goto fail;
            child->parent = parent;
            child->position = math_vector_rotate(math_vector_subtract(body->position, source_parent->position), -source_parent->rotation);
            child->orientation = body->rotation - source_parent->rotation;
        }
    }
    for(size_t i = 0; i < c.count; ++i) {
        ObjectDescription *d = c.entries[i].description;
        Position p; Orientation a;
        if(error_check(object_description_world_pose_get(d, &p, &a))) goto fail;
        if(!d->parent) continue;
        ObjectDescription *parent = (ObjectDescription *)d->parent;
        const ObjectDescription **children = realloc((void *)parent->children, (parent->child_count + 1) * sizeof(*children));
        if(!children) goto fail;
        parent->children = children; children[parent->child_count++] = d;
    }
    free(c.entries);
    return ERROR_RESULT_MAKE_VALUE(ObjectDescriptionResult, root);
fail:
    for(size_t i = 0; i < c.count; ++i) c.entries[i].description->child_count = 0;
    for(size_t i = 0; i < c.count; ++i) object_description_tree_destroy(c.entries[i].description);
    free(c.entries);
    return ERROR_RESULT_MAKE_ERROR(ObjectDescriptionResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
}

ObjectDescriptionResult object_description_file_read(const char *path) {
    EditorProject project = {0};
    EditorResult loaded = editor_project_load(&project, path);
    if(editor_result_check(loaded))
        return ERROR_RESULT_MAKE_ERROR(ObjectDescriptionResult, ERROR_ENGINE_STATE_INVALID);
    ObjectDescriptionResult result = object_description_project_read(&project, path);
    editor_project_destroy(&project);
    return result;
}
