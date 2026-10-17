/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_force_geometry.h"
#include "editor_mass_properties.h"
#include <math.h>

static Position add(Position a, Position b) { return (Position){a.x+b.x,a.y+b.y}; }
static Position sub(Position a, Position b) { return (Position){a.x-b.x,a.y-b.y}; }
static Position rotate(Position v, float degrees) {
    float a=degrees*(3.14159265358979323846f/180.0f), c=cosf(a),s=sinf(a);
    return (Position){v.x*c+v.y*s,v.y*c-v.x*s};
}
static const EditorSoftBody *soft_get(const EditorObject *o, uint64_t id) {
    for(size_t i=0;i<o->soft_body_count;++i) if(o->soft_body_items[i].id==id) return &o->soft_body_items[i];
    return NULL;
}
static const EditorSoftNode *node_get(const EditorSoftBody *b, uint64_t id) {
    if(b) for(size_t i=0;i<b->node_count;++i) if(b->nodes[i].id==id) return &b->nodes[i];
    return NULL;
}
static bool beam_get(const EditorObject *o, ObjectSourceReference r, Position *a, Position *b) {
    const EditorSoftBody *soft=soft_get(o,r.soft_body);
    if(!soft || r.kind!=OBJECT_SOURCE_BEAM) return false;
    for(size_t i=0;i<soft->beam_count;++i) if(soft->beams[i].id==r.id) {
        const EditorSoftNode *na=node_get(soft,soft->beams[i].node_a), *nb=node_get(soft,soft->beams[i].node_b);
        if(!na || !nb) return false;
        Position base=add(o->position,soft->position);
        *a=add(base,rotate(na->position,soft->rotation));
        *b=add(base,rotate(nb->position,soft->rotation));
        float length=hypotf(b->x-a->x,b->y-a->y);
        return isfinite(length) && length>ROHR_PHYSICS_BEAM_MIN_LENGTH;
    }
    return false;
}
static bool frame_get(const EditorObject *o, ObjectSourceReference r, bool com, Position *p, float *rotation) {
    if(!o || !editor_source_reference_check(o,r)) return false;
    *rotation=0;
    if(r.kind==OBJECT_SOURCE_BODY || r.kind==OBJECT_SOURCE_PARTICLE) {
        for(size_t i=0;i<o->rigid_body_count;++i) {
            const EditorRigidBody *b=&o->rigid_bodies[i];
            if(b->id!=r.id) continue;
            *p=add(o->position,b->position);
            if(b->standalone_particle) {
                if(com) *p=add(*p,b->particle_origin);
                return true;
            }
            *rotation=b->rotation;
            if(com) {
                EditorMassProperties m=editor_mass_properties_get(b);
                if(!m.center_available) return false;
                *p=add(*p,rotate(m.center,b->rotation));
            }
            return true;
        }
    }
    const EditorSoftBody *soft=soft_get(o,r.kind==OBJECT_SOURCE_SOFT_BODY?r.id:r.soft_body);
    if(!soft) return false;
    *p=add(o->position,soft->position);
    if(r.kind==OBJECT_SOURCE_SOFT_BODY) {
        *rotation=soft->rotation;
        if(com) {
            EditorSoftMassProperties m=editor_soft_mass_properties_get(soft);
            if(!m.center_available) return false;
            *p=add(*p,rotate(m.center,soft->rotation));
        }
        return true;
    }
    if(r.kind==OBJECT_SOURCE_NODE) {
        const EditorSoftNode *n=node_get(soft,r.id);
        if(!n) return false;
        *rotation=soft->rotation;
        *p=add(*p,rotate(n->position,soft->rotation)); return true;
    }
    if(r.kind==OBJECT_SOURCE_BEAM) {
        /* Authored beams have the containing soft body's origin transform.
         * Beam-position references explicitly use endpoint geometry instead. */
        *rotation=soft->rotation; return !com;
    }
    return false;
}
bool editor_force_location_get(const EditorObject *o, ObjectForceLocation l, Position *p) {
    if(!o || !p) return false;
    if(l.kind==ROHR_FORCE_WORLD_POSITION) { *p=l.offset; return isfinite(p->x) && isfinite(p->y); }
    if(l.kind==ROHR_FORCE_BEAM_POSITION) {
        Position a,b;
        if(!beam_get(o,l.reference,&a,&b)) return false;
        Position d=sub(b,a); float length=hypotf(d.x,d.y);
        float fraction=l.fraction+(l.anchor==ROHR_FORCE_BEAM_END?1.0f:l.anchor==ROHR_FORCE_BEAM_CENTER?0.5f:0.0f);
        *p=(Position){a.x+d.x*fraction+(d.x*l.offset.x-d.y*l.offset.y)/length,
            a.y+d.y*fraction+(d.y*l.offset.x+d.x*l.offset.y)/length}; return isfinite(p->x) && isfinite(p->y);
    }
    Position base; float rotation;
    if(!frame_get(o,l.reference,l.kind==ROHR_FORCE_COM_POSITION,&base,&rotation)) return false;
    *p=add(base,l.kind==ROHR_FORCE_ORIGIN_POSITION?rotate(l.offset,rotation):l.offset); return isfinite(p->x) && isfinite(p->y);
}
bool editor_force_orientation_get(const EditorObject *o, ObjectForceOrientation v, Orientation *angle) {
    if(!o || !angle) return false;
    float base=0;
    if(v.kind==ROHR_FORCE_BODY_ORIENTATION) {
        Position p;
        if(!frame_get(o,v.reference,false,&p,&base)) return false;
    } else if(v.kind==ROHR_FORCE_BEAM_ORIENTATION || v.kind==ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION) {
        Position a,b;
        if(v.kind==ROHR_FORCE_BEAM_ORIENTATION) {
            if(!beam_get(o,v.reference,&a,&b)) return false;
        } else if(!editor_force_location_get(o,v.from,&a) || !editor_force_location_get(o,v.to,&b)) return false;
        Position d=sub(b,a);
        if(d.x==0 && d.y==0) return false;
        base=atan2f(d.x,d.y)*(180.0f/3.14159265358979323846f);
    }
    *angle=(float)remainder((double)base+v.offset,360.0); return isfinite(*angle);
}
bool editor_force_location_set(const EditorObject *o, ObjectForceLocation *l, Position p) {
    if(!l || !isfinite(p.x) || !isfinite(p.y)) return false;
    ObjectForceLocation zero=*l; zero.offset=(Vec2D){0};
    Position base;
    if(!editor_force_location_get(o,zero,&base)) return false;
    Position d=sub(p,base);
    if(l->kind==ROHR_FORCE_BEAM_POSITION) {
        Position a,b;
        if(!beam_get(o,l->reference,&a,&b)) return false;
        Position axis=sub(b,a); float length=hypotf(axis.x,axis.y);
        d=(Position){(d.x*axis.x+d.y*axis.y)/length,(-d.x*axis.y+d.y*axis.x)/length};
    } else if(l->kind==ROHR_FORCE_ORIGIN_POSITION) {
        float rotation;
        if(!frame_get(o,l->reference,false,&base,&rotation)) return false;
        d=rotate(d,-rotation);
    }
    l->offset=d; return true;
}
bool editor_force_orientation_set(const EditorObject *o, ObjectForceOrientation *v, Orientation angle) {
    if(!v || !isfinite(angle)) return false;
    ObjectForceOrientation zero=*v; zero.offset=0; float base;
    if(!editor_force_orientation_get(o,zero,&base)) return false;
    v->offset=angle-base; return true;
}
