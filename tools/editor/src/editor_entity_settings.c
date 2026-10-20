/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_project.h"
#include "entity/object_internal.h"

ObjectEntitySettings *editor_project_entity_settings_get(EditorObject *o, ObjectEntityReference ref) {
    if(!o || !ref.id || ref.id > UINT32_MAX || ref.soft_body > UINT32_MAX) return NULL;
    if(ref.soft_body && ref.kind != OBJECT_NODE && ref.kind != OBJECT_BEAM && ref.kind != OBJECT_AREA) return NULL;
    EditorSoftBody *soft = NULL;
    if(ref.soft_body) for(size_t i = 0; i < o->soft_body_count; ++i)
        if(o->soft_body_items[i].id == ref.soft_body) soft = &o->soft_body_items[i];
#define FIND(array, count) do { \
    for(size_t i = 0; i < (count); ++i) \
        if((array)[i].id == ref.id) return &(array)[i].entity_settings; \
    return NULL; \
} while(0)
    switch(ref.kind) {
        case OBJECT_CONTAINER: return ref.id == o->id ? &o->entity_settings : NULL;
        case OBJECT_BODY: FIND(o->rigid_bodies, o->rigid_body_count);
        case OBJECT_PARTICLE:
            for(size_t i = 0; i < o->rigid_body_count; ++i)
                if(o->rigid_bodies[i].id == ref.id && o->rigid_bodies[i].standalone_particle)
                    return &o->rigid_bodies[i].entity_settings;
            return NULL;
        case OBJECT_SOFT_BODY: FIND(o->soft_body_items, o->soft_body_count);
        case OBJECT_NODE: if(soft) FIND(soft->nodes, soft->node_count); return NULL;
        case OBJECT_BEAM: if(soft) FIND(soft->beams, soft->beam_count); return NULL;
        case OBJECT_AREA: if(soft) FIND(soft->areas, soft->area_count); return NULL;
        case OBJECT_ANCHOR: FIND(o->anchors, o->anchor_count);
        case OBJECT_JOINT: FIND(o->joint_items, o->joint_count);
        case OBJECT_SPRITE: FIND(o->sprites, o->sprite_count);
        case OBJECT_ANIMATION: FIND(o->animated_sprite_items, o->animated_sprite_count);
        case OBJECT_CAMERA: FIND(o->cameras, o->camera_count);
        case OBJECT_FORCE: FIND(o->force_items, o->force_count);
        case OBJECT_TORQUE: FIND(o->torque_items, o->torque_count);
    }
#undef FIND
    return NULL;
}
typedef bool (*SettingsVisit)(EditorObject *, ObjectEntitySettings *, void *);
static bool settings_each(EditorObject *o, SettingsVisit visit, void *context) {
    if(!visit(o, &o->entity_settings, context)) return false;
#define VISIT(array, count) \
    for(size_t i = 0; i < (count); ++i) \
        if(!visit(o, &(array)[i].entity_settings, context)) return false;
    VISIT(o->rigid_bodies, o->rigid_body_count)
    VISIT(o->soft_body_items, o->soft_body_count)
    VISIT(o->anchors, o->anchor_count)
    VISIT(o->joint_items, o->joint_count)
    VISIT(o->sprites, o->sprite_count)
    VISIT(o->animated_sprite_items, o->animated_sprite_count)
    VISIT(o->cameras, o->camera_count)
    VISIT(o->force_items, o->force_count)
    VISIT(o->torque_items, o->torque_count)
    for(size_t j = 0; j < o->soft_body_count; ++j) {
        EditorSoftBody *b = &o->soft_body_items[j];
        VISIT(b->nodes, b->node_count)
        VISIT(b->beams, b->beam_count)
        VISIT(b->areas, b->area_count)
    }
#undef VISIT
    return true;
}
static bool settings_check(EditorObject *o, ObjectEntitySettings *s, void *context) {
    (void)context;
    if(s->angular_acceleration || s->angular_velocity_maximum_enabled) {
        bool rotational_body = false;
        for(size_t i = 0; i < o->rigid_body_count; ++i)
            if(s == &o->rigid_bodies[i].entity_settings)
                rotational_body = !o->rigid_bodies[i].standalone_particle;
        if(!rotational_body) return false;
    }
    if(s->gameplay_target.id) {
        for(size_t i = 0; i < o->force_count; ++i)
            if(s == &o->force_items[i].entity_settings) return false;
        for(size_t i = 0; i < o->torque_count; ++i)
            if(s == &o->torque_items[i].entity_settings) return false;
    }
    return object_entity_settings_check(*s) &&
        (!s->gameplay_target.id || editor_project_entity_settings_get(o, s->gameplay_target)) &&
        (!s->transform_driver.id || editor_project_entity_settings_get(o, s->transform_driver));
}
bool editor_project_entity_settings_check(EditorObject *object) {
    return object && settings_each(object, settings_check, NULL);
}
static bool settings_reference_remove(EditorObject *o, ObjectEntitySettings *s, void *context) {
    (void)o;
    ObjectEntityReference removed = *(ObjectEntityReference *)context;
    ObjectEntityReference *references[] = {&s->gameplay_target, &s->transform_driver};
    for(size_t i = 0; i < 2; ++i) {
        ObjectEntityReference r = *references[i];
        bool body_kind = (r.kind == OBJECT_BODY || r.kind == OBJECT_PARTICLE) &&
            (removed.kind == OBJECT_BODY || removed.kind == OBJECT_PARTICLE);
        if((r.id == removed.id && r.soft_body == removed.soft_body &&
                (r.kind == removed.kind || body_kind)) ||
                (removed.kind == OBJECT_SOFT_BODY && r.soft_body == removed.id))
            *references[i] = (ObjectEntityReference){0};
    }
    return true;
}
void editor_project_entity_reference_remove(EditorObject *object, ObjectEntityReference removed) {
    if(object) (void)settings_each(object, settings_reference_remove, &removed);
}
static bool settings_prune(EditorObject *o, ObjectEntitySettings *s, void *context) {
    (void)context;
    if(s->gameplay_target.id && !editor_project_entity_settings_get(o, s->gameplay_target))
        s->gameplay_target = (ObjectEntityReference){0};
    if(s->transform_driver.id && !editor_project_entity_settings_get(o, s->transform_driver))
        s->transform_driver = (ObjectEntityReference){0};
    return true;
}
void editor_project_entity_references_prune(EditorObject *object) {
    if(object) (void)settings_each(object, settings_prune, NULL);
}
static bool settings_remap(EditorObject *o, ObjectEntitySettings *s, void *context) {
    (void)o;
    ObjectEntityReference *refs = context;
    ObjectEntityReference *values[] = {&s->gameplay_target, &s->transform_driver};
    for(size_t i = 0; i < 2; ++i)
        if(values[i]->kind == refs[0].kind && values[i]->id == refs[0].id &&
                values[i]->soft_body == refs[0].soft_body) *values[i] = refs[1];
    return true;
}
void editor_project_entity_reference_remap(EditorObject *object,
        ObjectEntityReference before, ObjectEntityReference after) {
    ObjectEntityReference refs[] = {before, after};
    if(object) (void)settings_each(object, settings_remap, refs);
}
