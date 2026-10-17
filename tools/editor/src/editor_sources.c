/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_project.h"
#include "editor_array.h"
#include "entity/object_internal.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>

bool editor_source_reference_check(const EditorObject *o, ObjectSourceReference r) {
    if(!o) return false;
    if(r.kind == OBJECT_SOURCE_NONE) return !r.id && !r.soft_body;
    if(!r.id || r.id > UINT32_MAX || r.soft_body > UINT32_MAX) return false;
    if(r.kind == OBJECT_SOURCE_BODY || r.kind == OBJECT_SOURCE_PARTICLE) {
        if(r.soft_body) return false;
        for(size_t i = 0; i < o->rigid_body_count; ++i)
            if(o->rigid_bodies[i].id == r.id)
                return o->rigid_bodies[i].standalone_particle == (r.kind == OBJECT_SOURCE_PARTICLE);
    }
    for(size_t i = 0; i < o->soft_body_count; ++i) {
        const EditorSoftBody *b = &o->soft_body_items[i];
        if(r.kind == OBJECT_SOURCE_SOFT_BODY && !r.soft_body && b->id == r.id) return true;
        if(r.soft_body != b->id) continue;
        if(r.kind == OBJECT_SOURCE_NODE)
            for(size_t n = 0; n < b->node_count; ++n) if(b->nodes[n].id == r.id) return true;
        if(r.kind == OBJECT_SOURCE_BEAM)
            for(size_t n = 0; n < b->beam_count; ++n) if(b->beams[n].id == r.id) return true;
    }
    return false;
}
static bool reference_check(const void *scope, ObjectSourceReference ref) {
    return editor_source_reference_check(scope, ref);
}
bool editor_force_description_check(const EditorObject *o, ObjectForceDescription v) {
    return object_source_force_check(o, v, reference_check);
}
bool editor_torque_description_check(const EditorObject *o, ObjectTorqueDescription v) {
    return object_source_torque_check(o, v, reference_check);
}
bool editor_sources_check(const EditorObject *o) {
    if(!o || o->force_count > EDITOR_SOURCE_MAX || o->torque_count > EDITOR_SOURCE_MAX ||
            (o->force_count && !o->force_items) || (o->torque_count && !o->torque_items)) return false;
    for(size_t i = 0; i < o->force_count; ++i) {
        if(!o->force_items[i].id || !o->force_items[i].name[0] || !memchr(o->force_items[i].name, 0, sizeof(o->force_items[i].name)) || !editor_force_description_check(o, o->force_items[i].description)) return false;
        for(size_t j = 0; j < i; ++j) if(o->force_items[i].id == o->force_items[j].id || !strcmp(o->force_items[i].name, o->force_items[j].name)) return false;
    }
    for(size_t i = 0; i < o->torque_count; ++i) {
        if(!o->torque_items[i].id || !o->torque_items[i].name[0] || !memchr(o->torque_items[i].name, 0, sizeof(o->torque_items[i].name)) || !editor_torque_description_check(o, o->torque_items[i].description)) return false;
        for(size_t j = 0; j < i; ++j) if(o->torque_items[i].id == o->torque_items[j].id || !strcmp(o->torque_items[i].name, o->torque_items[j].name)) return false;
    }
    return true;
}
#define SOURCE_FUNCTIONS(kind, Type) \
Editor##Type *editor_project_##kind##_add(EditorProject *p, EditorObject *o) { \
    if(!p || !o || o->kind##_count >= EDITOR_SOURCE_MAX || !p->next_##kind##_id || p->next_##kind##_id == UINT32_MAX || \
            !EDITOR_ARRAY_RESERVE(o->kind##_items, o->kind##_capacity, o->kind##_count + 1)) return NULL; \
    Editor##Type *v = &o->kind##_items[o->kind##_count++]; \
    *v = (Editor##Type){.id = p->next_##kind##_id++}; \
    snprintf(v->name, sizeof(v->name), #kind "_%u", v->id); \
    return v; \
} \
bool editor_project_##kind##_set(EditorObject *o, uint32_t id, Object##Type##Description value) { \
    if(!editor_##kind##_description_check(o, value)) return false; \
    for(size_t i = 0; i < o->kind##_count; ++i) if(o->kind##_items[i].id == id) { o->kind##_items[i].description = value; return true; } \
    return false; \
}
SOURCE_FUNCTIONS(force, Force)
SOURCE_FUNCTIONS(torque, Torque)
#undef SOURCE_FUNCTIONS
bool editor_project_source_remove(EditorObject *o, bool torque, uint32_t id) {
    if(!o || !id) return false;
    if(torque) {
        for(size_t i = 0; i < o->torque_count; ++i) if(o->torque_items[i].id == id) {
            memmove(&o->torque_items[i], &o->torque_items[i+1], (--o->torque_count-i)*sizeof(*o->torque_items)); return true;
        }
    } else for(size_t i = 0; i < o->force_count; ++i) if(o->force_items[i].id == id) {
        memmove(&o->force_items[i], &o->force_items[i+1], (--o->force_count-i)*sizeof(*o->force_items)); return true;
    }
    return false;
}
static bool affected(const EditorObject *o, ObjectSourceReference r, ObjectSourceReference removed) {
    if(r.kind == OBJECT_SOURCE_NONE) return false;
    (void)o;
    if(removed.kind == OBJECT_SOURCE_NONE) return false;
    return (r.kind == removed.kind && r.id == removed.id && r.soft_body == removed.soft_body) ||
        (removed.kind == OBJECT_SOURCE_SOFT_BODY && r.soft_body == removed.id);
}
static bool location_affected(const EditorObject *o, ObjectForceLocation l, ObjectSourceReference removed) {
    return l.kind != ROHR_FORCE_WORLD_POSITION && affected(o, l.reference, removed);
}
void editor_sources_reference_remove(EditorObject *o, ObjectSourceReference r) {
    if(!o) return;
    for(size_t i = 0; i < o->force_count; ++i) {
        ObjectForceDescription *d = &o->force_items[i].description;
        if(affected(o, d->target, r)) d->target = (ObjectSourceReference){0};
        if(location_affected(o, d->location, r)) d->location = (ObjectForceLocation){0};
        ObjectForceOrientation v = d->orientation;
        if((v.kind == ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION &&
                (location_affected(o, v.from, r) || location_affected(o, v.to, r))) ||
            ((v.kind == ROHR_FORCE_BODY_ORIENTATION || v.kind == ROHR_FORCE_BEAM_ORIENTATION) && affected(o, v.reference, r)))
            d->orientation = (ObjectForceOrientation){0};
    }
    for(size_t i = 0; i < o->torque_count; ++i)
        if(affected(o, o->torque_items[i].description.target, r)) o->torque_items[i].description.target = (ObjectSourceReference){0};
}

EditorObject *editor_project_object_duplicate(EditorProject *p, uint32_t id) {
    if(!p) return NULL;
    EditorObject copy={0}; bool found=false;
    for(size_t i=0;i<p->object_count;++i) if(p->objects[i].id==id) {
        if(!editor_project_object_clone(&copy,&p->objects[i])) return NULL;
        found=true;break;
    }
    if(!found || copy.force_count > UINT32_MAX-p->next_force_id || copy.torque_count > UINT32_MAX-p->next_torque_id) {
        editor_project_object_destroy(&copy);return NULL;
    }
    EditorObject *added=editor_project_object_add(p,copy.position);
    if(!added) { editor_project_object_destroy(&copy);return NULL; }
    uint32_t new_id=added->id; char name[EDITOR_OBJECT_NAME_MAX]; strcpy(name,added->name);
    editor_project_object_destroy(added); *added=copy; added->id=new_id; strcpy(added->name,name);
    /* Element references are object-local. Retaining element IDs remaps every
     * connection into this independent copy; source identities remain unique. */
    for(size_t i=0;i<added->force_count;++i) added->force_items[i].id=p->next_force_id++;
    for(size_t i=0;i<added->torque_count;++i) added->torque_items[i].id=p->next_torque_id++;
    return added;
}
