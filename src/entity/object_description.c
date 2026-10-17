/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "object_internal.h"
#include "graphics/asset_path.h"
#include "physics/soft_body/soft_body_mass.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

typedef struct ObjectStorage {
    ObjectDescription *root;
    char *path;
    bool editable;
    size_t references;
    struct ObjectStorage *next;
} ObjectStorage;
static ObjectStorage *stores;

bool object_flags_check(EntityObjectFlags flags) {
    return flags && !(flags & ~ROHR_ENTITY_OBJECT_ALL);
}
char *object_string_copy(const char *text) {
    if(!text) return NULL;
    size_t n = strlen(text) + 1;
    char *copy = malloc(n);
    if(copy) memcpy(copy, text, n);
    return copy;
}
const ObjectDescription *object_description_get(const ObjectDescription *description) {
    return description;
}
void object_description_tree_destroy(ObjectDescription *d) {
    if(!d) return;
    for(size_t i = 0; i < d->child_count; ++i)
        object_description_tree_destroy((ObjectDescription *)d->children[i]);
    for(size_t i = 0; i < d->frame_count; ++i) free((void *)d->frames[i].path);
    free((void *)d->children); free((void *)d->name);
    free((void *)d->geometry); free((void *)d->hitbox_bindings);
    free((void *)d->area); free((void *)d->asset_path);
    free((void *)d->frames); free(d);
}
ObjectDescriptionResult object_description_register(ObjectDescription *root,
        char *path, bool editable) {
    ObjectStorage *store = calloc(1, sizeof(*store));
    if(!store) {
        object_description_tree_destroy(root); free(path);
        return ERROR_RESULT_MAKE_ERROR(ObjectDescriptionResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    *store = (ObjectStorage){.root = root, .path = path, .editable = editable,
        .references = 1, .next = stores};
    stores = store;
    return ERROR_RESULT_MAKE_VALUE(ObjectDescriptionResult, root);
}
ObjectDescriptionResult object_load(const char *path) {
    if(!path || !path[0])
        return ERROR_RESULT_MAKE_ERROR(ObjectDescriptionResult, ERROR_ENGINE_STATE_INVALID);
    char *resolved = graphics_asset_path_resolve(path);
    if(!resolved)
        return ERROR_RESULT_MAKE_ERROR(ObjectDescriptionResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    for(ObjectStorage *s = stores; s; s = s->next) {
        if(!s->path || strcmp(s->path, resolved)) continue;
        free(resolved);
        if(s->references == SIZE_MAX)
            return ERROR_RESULT_MAKE_ERROR(ObjectDescriptionResult, ERROR_ENGINE_STATE_INVALID);
        ++s->references;
        return ERROR_RESULT_MAKE_VALUE(ObjectDescriptionResult, s->root);
    }
    ObjectDescriptionResult loaded = object_description_file_read(resolved);
    if(error_check(loaded)) { free(resolved); return loaded; }
    return object_description_register((ObjectDescription *)loaded.result.value, resolved, false);
}
EngineResult object_destroy(const ObjectDescription *description) {
    if(!description) return error_result_value(true);
    for(ObjectStorage **link = &stores; *link; link = &(*link)->next) {
        ObjectStorage *s = *link;
        if(s->root != description) continue;
        if(--s->references) return error_result_value(true);
        *link = s->next;
        object_description_tree_destroy(s->root);
        free(s->path); free(s);
        return error_result_value(true);
    }
    return error_result_error(ERROR_ENGINE_STATE_INVALID);
}

static bool find_name(const ObjectDescription *d, const char *name,
        unsigned depth, const ObjectDescription **found) {
    if(!d || depth > OBJECT_DEPTH_MAX || d->child_count > MAX_ENTITIES ||
            (d->child_count && !d->children)) return false;
    if(d->name && !strcmp(d->name, name)) {
        if(*found) return false;
        *found = d;
    }
    for(size_t i = 0; i < d->child_count; ++i)
        if(!find_name(d->children[i], name, depth + 1, found)) return false;
    return true;
}
ObjectDescriptionResult object_description_name_get(const ObjectDescription *scope, const char *name) {
    const ObjectDescription *found = NULL;
    if(!name || !name[0] || !find_name(scope, name, 0, &found))
        return ERROR_RESULT_MAKE_ERROR(ObjectDescriptionResult, ERROR_ENGINE_STATE_INVALID);
    if(!found) return ERROR_RESULT_MAKE_ERROR(ObjectDescriptionResult, ERROR_ENGINE_ENTITY_NOT_FOUND);
    return ERROR_RESULT_MAKE_VALUE(ObjectDescriptionResult, found);
}
static const ObjectDescription *id_find(const ObjectDescription *d, uint64_t id, unsigned depth) {
    if(!d || depth > OBJECT_DEPTH_MAX || d->child_count > MAX_ENTITIES ||
            (d->child_count && !d->children)) return NULL;
    if(d->id == id) return d;
    for(size_t i=0;i<d->child_count;++i) {
        const ObjectDescription *found=id_find(d->children[i],id,depth+1);
        if(found) return found;
    }
    return NULL;
}
EngineResult object_description_world_pose_get(const ObjectDescription *d,
        Position *position, Orientation *orientation) {
    if(!d || !position || !orientation) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(d->kind == OBJECT_CAMERA && d->target && d->parent) {
        const ObjectDescription *root=d; unsigned depth=0;
        while(root->parent) {
            if(++depth>OBJECT_DEPTH_MAX) return error_result_error(ERROR_ENGINE_STATE_INVALID);
            root=root->parent;
        }
        const ObjectDescription *target=id_find(root,d->target,0);
        if(target && target->kind != OBJECT_CAMERA) {
            EngineResult result=object_description_world_pose_get(target,position,orientation);
            if(error_check(result)) return result;
            Position offset=math_vector_rotate(d->position,*orientation);
            position->x+=offset.x; position->y+=offset.y;
            *orientation=d->orientation+(d->follow_orientation ? *orientation : 0);
            if(!isfinite(position->x) || !isfinite(position->y) || !isfinite(*orientation))
                return error_result_error(ERROR_ENGINE_STATE_INVALID);
            return error_result_value(true);
        }
    }
    *position = d->position; *orientation = d->orientation;
    unsigned depth = 0;
    for(const ObjectDescription *p = d->parent; p; p = p->parent) {
        if(++depth > OBJECT_DEPTH_MAX) return error_result_error(ERROR_ENGINE_STATE_INVALID);
        Position rotated = math_vector_rotate(*position, p->orientation);
        *position = (Position){p->position.x + rotated.x, p->position.y + rotated.y};
        *orientation += p->orientation;
    }
    if(!d->follow_orientation && (d->kind == OBJECT_ANCHOR ||
            ((d->kind == OBJECT_SPRITE || d->kind == OBJECT_ANIMATION) && d->target)))
        *orientation = d->orientation;
    if(!isfinite(position->x) || !isfinite(position->y) || !isfinite(*orientation))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    return error_result_value(true);
}

static void *array_copy(const void *input, size_t count, size_t size) {
    if(!count) return NULL;
    if(!input || count > MAX_ENTITIES || count > SIZE_MAX / size) return NULL;
    void *copy = malloc(count * size);
    if(copy) memcpy(copy, input, count * size);
    return copy;
}
static ObjectDescription *tree_copy(const ObjectDescription *s, ObjectDescription *parent,
        EntityObjectFlags flags, unsigned depth) {
    if(!s || depth > OBJECT_DEPTH_MAX || !s->name || !s->name[0] ||
            strlen(s->name) >= ENTITY_NAME_MAX || s->child_count > MAX_ENTITIES ||
            (s->child_count && !s->children)) return NULL;
    ObjectDescription *d = malloc(sizeof(*d));
    if(!d) return NULL;
    *d = *s;
    d->parent = parent; d->categories &= flags;
    if(!(flags & ROHR_ENTITY_OBJECT_PHYSICS)) {
        d->force = (ObjectForceDescription){0}; d->torque = (ObjectTorqueDescription){0};
    }
    d->name = NULL; d->children = NULL; d->child_count = 0;
    d->geometry = NULL; d->geometry_count = 0;
    d->hitbox_bindings = NULL; d->hitbox_binding_count = 0;
    d->area = NULL; d->asset_path = NULL; d->frames = NULL; d->frame_count = 0;
    d->name = object_string_copy(s->name);
    if(!d->name) goto fail;
    if(flags & s->categories) {
        if(s->geometry_count) {
            d->geometry = array_copy(s->geometry, s->geometry_count, sizeof(*d->geometry));
            if(!d->geometry) goto fail;
            d->geometry_count = s->geometry_count;
        }
        if(s->hitbox_binding_count) {
            d->hitbox_bindings = array_copy(s->hitbox_bindings, s->hitbox_binding_count, sizeof(*d->hitbox_bindings));
            if(!d->hitbox_bindings) goto fail;
            d->hitbox_binding_count = s->hitbox_binding_count;
        }
        if(s->area) {
            d->area = array_copy(s->area, 1, sizeof(*d->area));
            if(!d->area) goto fail;
        }
    }
    if(flags & s->categories & ROHR_ENTITY_OBJECT_VISUALS) {
        if(s->asset_path && !(d->asset_path = object_string_copy(s->asset_path))) goto fail;
        if(s->frame_count) {
            if(!s->frames || s->frame_count > MAX_ANIMATIONS_FRAMES) goto fail;
            ObjectFrame *frames = calloc(s->frame_count, sizeof(*frames));
            if(!frames) goto fail;
            d->frames = frames; d->frame_count = s->frame_count;
            for(size_t i = 0; i < s->frame_count; ++i) {
                frames[i] = s->frames[i];
                frames[i].path = object_string_copy(s->frames[i].path);
                if(!frames[i].path) goto fail;
            }
        }
    }
    if(s->child_count) {
        const ObjectDescription **children = calloc(s->child_count, sizeof(*children));
        if(!children) goto fail;
        d->children = children;
        for(size_t i = 0; i < s->child_count; ++i) {
            const ObjectDescription *child = s->children[i];
            if(!child) goto fail;
            if(!(child->categories & flags) && !child->child_count) continue;
            ObjectDescription *copy = tree_copy(child, d, flags, depth + 1);
            if(!copy) goto fail;
            children[d->child_count++] = copy;
        }
    }
    return d;
fail:
    object_description_tree_destroy(d);
    return NULL;
}
ObjectDescriptionResult object_description_copy(const ObjectDescription *description, EntityObjectFlags flags) {
    Position position; Orientation orientation;
    if(!object_flags_check(flags) || error_check(object_description_world_pose_get(description, &position, &orientation)))
        return ERROR_RESULT_MAKE_ERROR(ObjectDescriptionResult, ERROR_ENGINE_STATE_INVALID);
    ObjectDescription *copy = tree_copy(description, NULL, flags, 0);
    if(!copy) return ERROR_RESULT_MAKE_ERROR(ObjectDescriptionResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    copy->position = position; copy->orientation = orientation;
    return object_description_register(copy, NULL, true);
}
static ObjectDescription *editable_find(ObjectDescription *root, const ObjectDescription *d) {
    if(root == d) return root;
    for(size_t i = 0; i < root->child_count; ++i) {
        ObjectDescription *found = editable_find((ObjectDescription *)root->children[i], d);
        if(found) return found;
    }
    return NULL;
}
static ObjectDescription *editable_get(const ObjectDescription *description) {
    for(ObjectStorage *s = stores; s; s = s->next) {
        if(!s->editable) continue;
        ObjectDescription *found = editable_find(s->root, description);
        if(found) return found;
    }
    return NULL;
}
EngineResult object_mass_set(const ObjectDescription *description, Mass mass) {
    ObjectDescription *d = editable_get(description);
    if(!d || !isfinite(mass) || mass <= 0 ||
            !(d->categories & ROHR_ENTITY_OBJECT_PHYSICS))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(d->kind == OBJECT_SOFT_BODY) {
        Mass prior[SOFT_BODY_MAX_NODES], scaled[SOFT_BODY_MAX_NODES];
        ObjectDescription *nodes[SOFT_BODY_MAX_NODES]; uint32_t count = 0;
        for(size_t i = 0; i < d->child_count; ++i) {
            if(d->children[i]->kind != OBJECT_NODE) continue;
            if(count == SOFT_BODY_MAX_NODES) return error_result_error(ERROR_ENGINE_STATE_INVALID);
            nodes[count] = (ObjectDescription *)d->children[i];
            prior[count++] = d->children[i]->mass;
        }
        if(!physics_soft_mass_scale_get(prior, count, mass, scaled))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        for(uint32_t i = 0; i < count; ++i) nodes[i]->mass = scaled[i];
    } else if(
            (d->kind != OBJECT_BODY && d->kind != OBJECT_PARTICLE && d->kind != OBJECT_NODE))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    d->mass = mass;
    if(d->kind == OBJECT_NODE && d->parent && d->parent->kind == OBJECT_SOFT_BODY) {
        ObjectDescription *parent = (ObjectDescription *)d->parent;
        double total = 0;
        for(size_t i = 0; i < parent->child_count; ++i)
            if(parent->children[i]->kind == OBJECT_NODE) total += parent->children[i]->mass;
        parent->mass = (Mass)total;
    }
    return error_result_value(true);
}
#define VECTOR_SETTER(name, field, type) \
EngineResult name(const ObjectDescription *description, type value) { \
    ObjectDescription *d = editable_get(description); \
    if(!d || !isfinite(value.x) || !isfinite(value.y)) \
        return error_result_error(ERROR_ENGINE_STATE_INVALID); \
    d->field = value; return error_result_value(true); \
}
VECTOR_SETTER(object_position_set, position, Position)
VECTOR_SETTER(object_velocity_set, velocity, Velocity)
VECTOR_SETTER(object_acceleration_set, acceleration, Acceleration)
#undef VECTOR_SETTER
EngineResult object_orientation_set(const ObjectDescription *description, Orientation orientation) {
    ObjectDescription *d = editable_get(description);
    if(!d || !isfinite(orientation)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    d->orientation = orientation;
    return error_result_value(true);
}
EngineResult object_angular_velocity_set(const ObjectDescription *description, AngularVelocity value) {
    ObjectDescription *d = editable_get(description);
    if(!d || d->kind != OBJECT_BODY || !isfinite(value)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    d->angular_velocity = value; return error_result_value(true);
}
EngineResult object_material_set(const ObjectDescription *description, float friction, float restitution) {
    ObjectDescription *d = editable_get(description);
    if(!d || !isfinite(friction) || friction < 0 || !isfinite(restitution) || restitution < 0 || restitution > 1 ||
            !(d->categories & ROHR_ENTITY_OBJECT_PHYSICS) ||
            (d->kind != OBJECT_BODY && d->kind != OBJECT_PARTICLE && d->kind != OBJECT_NODE && d->kind != OBJECT_BEAM))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    d->friction = friction; d->restitution = restitution; return error_result_value(true);
}
EngineResult object_collision_set(const ObjectDescription *description, bool enabled, CollisionFilterConfig filter) {
    ObjectDescription *d = editable_get(description);
    if(!d || !(d->categories & ROHR_ENTITY_OBJECT_PHYSICS) ||
            (d->kind != OBJECT_BODY && d->kind != OBJECT_PARTICLE && d->kind != OBJECT_NODE && d->kind != OBJECT_BEAM))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    d->collision_enabled = enabled; d->collision_filter = filter; return error_result_value(true);
}
EngineResult object_radius_set(const ObjectDescription *description, float radius) {
    ObjectDescription *d = editable_get(description);
    if(!d || !isfinite(radius) || radius <= 0 ||
            (d->kind != OBJECT_PARTICLE && d->kind != OBJECT_NODE && !(d->kind == OBJECT_BODY && d->radius > 0)))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    d->radius = radius; return error_result_value(true);
}
EngineResult object_visibility_set(const ObjectDescription *description, bool visible) {
    ObjectDescription *d = editable_get(description);
    if(!d) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    d->visible = visible; return error_result_value(true);
}
EngineResult object_color_set(const ObjectDescription *description, Color color, Color border) {
    ObjectDescription *d = editable_get(description);
    if(!d) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    d->color = color; d->border_color = border; d->color_overridden = true; return error_result_value(true);
}
EngineResult object_scale_set(const ObjectDescription *description, Scale scale) {
    ObjectDescription *d = editable_get(description);
    if(!d || (d->kind != OBJECT_SPRITE && d->kind != OBJECT_ANIMATION) ||
            !isfinite(scale.x) || !isfinite(scale.y)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    d->scale = scale; return error_result_value(true);
}

EngineResult object_force_description_set(const ObjectDescription *description, ObjectForceDescription value) {
    ObjectDescription *d = editable_get(description);
    if(!d || d->kind != OBJECT_FORCE || !(d->categories & ROHR_ENTITY_OBJECT_PHYSICS) ||
            !object_source_force_check(d->parent, value, object_source_reference_check))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    d->force = value;
    return error_result_value(true);
}
EngineResult object_torque_description_set(const ObjectDescription *description, ObjectTorqueDescription value) {
    ObjectDescription *d = editable_get(description);
    if(!d || d->kind != OBJECT_TORQUE || !(d->categories & ROHR_ENTITY_OBJECT_PHYSICS) ||
            !object_source_torque_check(d->parent, value, object_source_reference_check))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    d->torque = value;
    return error_result_value(true);
}
