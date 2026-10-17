/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "object_internal.h"
#include <math.h>

static bool mode_check(ObjectSourceMode mode) {
    return mode == OBJECT_SOURCE_PERSISTENT || mode == OBJECT_SOURCE_INITIAL;
}
static bool location_check(const void *scope, ObjectForceLocation v, ObjectSourceReferenceCheck check) {
    if(!isfinite(v.offset.x) || !isfinite(v.offset.y)) return false;
    switch(v.kind) {
    case ROHR_FORCE_WORLD_POSITION: return true;
    case ROHR_FORCE_COM_POSITION: case ROHR_FORCE_ORIGIN_POSITION:
        return v.reference.kind != OBJECT_SOURCE_NONE && check(scope, v.reference);
    case ROHR_FORCE_BEAM_POSITION:
        return v.reference.kind == OBJECT_SOURCE_BEAM && check(scope, v.reference) &&
            v.anchor >= ROHR_FORCE_BEAM_START && v.anchor <= ROHR_FORCE_BEAM_END && isfinite(v.fraction);
    default: return false;
    }
}
bool object_source_force_check(const void *scope, ObjectForceDescription v, ObjectSourceReferenceCheck check) {
    if(!mode_check(v.mode) || !isfinite(v.magnitude) || !check(scope, v.target) ||
            !location_check(scope, v.location, check) || !isfinite(v.orientation.offset)) return false;
    ObjectForceOrientation o = v.orientation;
    switch(o.kind) {
    case ROHR_FORCE_WORLD_ORIENTATION: return true;
    case ROHR_FORCE_BODY_ORIENTATION:
        return (o.reference.kind != OBJECT_SOURCE_NONE) && check(scope, o.reference);
    case ROHR_FORCE_BEAM_ORIENTATION:
        return o.reference.kind == OBJECT_SOURCE_BEAM && check(scope, o.reference);
    case ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION:
        return location_check(scope, o.from, check) && location_check(scope, o.to, check);
    default: return false;
    }
}
bool object_source_torque_check(const void *scope, ObjectTorqueDescription v, ObjectSourceReferenceCheck check) {
    return mode_check(v.mode) && isfinite(v.magnitude) && check(scope, v.target);
}
static const ObjectDescription *find(const ObjectDescription *d, uint64_t id, unsigned depth) {
    if(!d || depth > OBJECT_DEPTH_MAX) return NULL;
    if(d->id == id) return d;
    for(size_t i = 0; i < d->child_count; ++i) {
        const ObjectDescription *found = find(d->children[i], id, depth + 1);
        if(found) return found;
    }
    return NULL;
}
bool object_source_reference_check(const void *scope, ObjectSourceReference ref) {
    if(ref.kind == OBJECT_SOURCE_NONE) return ref.id == 0 && ref.soft_body == 0;
    const ObjectDescription *d = find(scope, ref.id, 0);
    if(!d || !ref.id) return false;
    switch(ref.kind) {
    case OBJECT_SOURCE_BODY: return !ref.soft_body && d->kind == OBJECT_BODY;
    case OBJECT_SOURCE_PARTICLE: return !ref.soft_body && d->kind == OBJECT_PARTICLE;
    case OBJECT_SOURCE_SOFT_BODY: return !ref.soft_body && d->kind == OBJECT_SOFT_BODY;
    case OBJECT_SOURCE_NODE: case OBJECT_SOURCE_BEAM:
        return d->kind == (ref.kind == OBJECT_SOURCE_NODE ? OBJECT_NODE : OBJECT_BEAM) &&
            d->parent && d->parent->kind == OBJECT_SOFT_BODY && d->parent->id == ref.soft_body;
    default: return false;
    }
}
