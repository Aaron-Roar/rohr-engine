/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "entity/object_visuals.h"
#include "entity/object_internal.h"
#include "editor_sources_json.h"
#include "editor_array.h"
#include <math.h>
#include <string.h>
#include <float.h>

static const char *reference_names[] = {"none", "body", "particle", "soft_body", "node", "beam"};
static const char *location_names[] = {"world", "com", "origin", "beam"};
static const char *orientation_names[] = {"world", "body", "beam", "between"};
static bool number(yyjson_val *o, const char *key, float *value) {
    yyjson_val *v = yyjson_obj_get(o, key);
    if(!yyjson_is_num(v) || !isfinite(yyjson_get_num(v)) || fabs(yyjson_get_num(v)) > FLT_MAX) return false;
    *value = (float)yyjson_get_num(v); return true;
}
static bool uint_value(yyjson_val *o, const char *key, uint64_t *value) {
    yyjson_val *v = yyjson_obj_get(o, key);
    if(!yyjson_is_uint(v)) return false;
    *value = yyjson_get_uint(v); return true;
}
static int enum_read(yyjson_val *o, const char *key, const char **names, size_t count) {
    const char *value = yyjson_get_str(yyjson_obj_get(o, key));
    if(value) for(size_t i = 0; i < count; ++i) if(!strcmp(value, names[i])) return (int)i;
    return -1;
}
static yyjson_mut_val *reference_write(yyjson_mut_doc *d, ObjectSourceReference r) {
    if(r.kind < OBJECT_SOURCE_NONE || r.kind > OBJECT_SOURCE_BEAM) return NULL;
    yyjson_mut_val *v = yyjson_mut_obj(d);
    if(!v || !yyjson_mut_obj_add_str(d, v, "kind", reference_names[r.kind]) ||
        !yyjson_mut_obj_add_uint(d, v, "id", r.id) || !yyjson_mut_obj_add_uint(d, v, "soft_body", r.soft_body)) return NULL;
    return v;
}
static bool reference_read(yyjson_val *v, ObjectSourceReference *r) {
    if(!yyjson_is_obj(v) || yyjson_obj_size(v) != 3) return false;
    int kind = enum_read(v, "kind", reference_names, 6);
    if(kind < 0 || !uint_value(v, "id", &r->id) || !uint_value(v, "soft_body", &r->soft_body)) return false;
    r->kind = kind; return true;
}
static yyjson_mut_val *location_write(yyjson_mut_doc *d, ObjectForceLocation l) {
    if(l.kind < ROHR_FORCE_WORLD_POSITION || l.kind > ROHR_FORCE_BEAM_POSITION) return NULL;
    yyjson_mut_val *v = yyjson_mut_obj(d);
    if(!v || !yyjson_mut_obj_add_str(d,v,"kind",location_names[l.kind]) ||
        !yyjson_mut_obj_add_real(d,v,"x",l.offset.x) || !yyjson_mut_obj_add_real(d,v,"y",l.offset.y)) return NULL;
    if(l.kind != ROHR_FORCE_WORLD_POSITION && !yyjson_mut_obj_add_val(d,v,"reference",reference_write(d,l.reference))) return NULL;
    if(l.kind == ROHR_FORCE_BEAM_POSITION && (!yyjson_mut_obj_add_uint(d,v,"anchor",l.anchor) || !yyjson_mut_obj_add_real(d,v,"fraction",l.fraction))) return NULL;
    return v;
}
static bool location_read(yyjson_val *v, ObjectForceLocation *l) {
    int kind = enum_read(v,"kind",location_names,4); uint64_t anchor;
    *l = (ObjectForceLocation){0};
    if(kind < 0 || !number(v,"x",&l->offset.x) || !number(v,"y",&l->offset.y)) return false;
    l->kind = kind;
    if(kind != ROHR_FORCE_WORLD_POSITION && !reference_read(yyjson_obj_get(v,"reference"),&l->reference)) return false;
    if(kind == ROHR_FORCE_BEAM_POSITION) {
        if(!uint_value(v,"anchor",&anchor) || anchor > ROHR_FORCE_BEAM_END || !number(v,"fraction",&l->fraction)) return false;
        l->anchor = anchor;
    }
    return true;
}
static yyjson_mut_val *orientation_write(yyjson_mut_doc *d, ObjectForceOrientation o) {
    if(o.kind < ROHR_FORCE_WORLD_ORIENTATION || o.kind > ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION) return NULL;
    yyjson_mut_val *v=yyjson_mut_obj(d);
    if(!v || !yyjson_mut_obj_add_str(d,v,"kind",orientation_names[o.kind]) || !yyjson_mut_obj_add_real(d,v,"offset",o.offset)) return NULL;
    if(o.kind == ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION) {
        if(!yyjson_mut_obj_add_val(d,v,"from",location_write(d,o.from)) || !yyjson_mut_obj_add_val(d,v,"to",location_write(d,o.to))) return NULL;
    } else if(o.kind != ROHR_FORCE_WORLD_ORIENTATION && !yyjson_mut_obj_add_val(d,v,"reference",reference_write(d,o.reference))) return NULL;
    return v;
}
static bool orientation_read(yyjson_val *v, ObjectForceOrientation *o) {
    int kind=enum_read(v,"kind",orientation_names,4); *o=(ObjectForceOrientation){0};
    if(kind<0 || !number(v,"offset",&o->offset)) return false;
    o->kind=kind;
    if(kind==ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return location_read(yyjson_obj_get(v,"from"),&o->from) && location_read(yyjson_obj_get(v,"to"),&o->to);
    return kind==ROHR_FORCE_WORLD_ORIENTATION || reference_read(yyjson_obj_get(v,"reference"),&o->reference);
}
static yyjson_mut_val *common_write(yyjson_mut_doc *d, ObjectSourceMode mode, ObjectSourceReference target, float magnitude) {
    if(mode != OBJECT_SOURCE_PERSISTENT && mode != OBJECT_SOURCE_INITIAL) return NULL;
    yyjson_mut_val *v=yyjson_mut_obj(d);
    if(!v || !yyjson_mut_obj_add_str(d,v,"mode",mode==OBJECT_SOURCE_INITIAL?"initial":"persistent") ||
        !yyjson_mut_obj_add_val(d,v,"target",reference_write(d,target)) || !yyjson_mut_obj_add_real(d,v,"magnitude",magnitude)) return NULL;
    return v;
}
yyjson_mut_val *editor_force_json_write(yyjson_mut_doc *d, ObjectForceDescription f) {
    yyjson_mut_val *v=common_write(d,f.mode,f.target,f.magnitude);
    if(!v || !yyjson_mut_obj_add_val(d,v,"location",location_write(d,f.location)) || !yyjson_mut_obj_add_val(d,v,"orientation",orientation_write(d,f.orientation))) return NULL;
    return v;
}
yyjson_mut_val *editor_torque_json_write(yyjson_mut_doc *d, ObjectTorqueDescription t) {
    return common_write(d,t.mode,t.target,t.magnitude);
}
static bool common_read(yyjson_val *v, ObjectSourceMode *mode, ObjectSourceReference *target, float *magnitude) {
    const char *names[]={"persistent","initial"}; int m=enum_read(v,"mode",names,2);
    if(m<0 || !reference_read(yyjson_obj_get(v,"target"),target) || !number(v,"magnitude",magnitude)) return false;
    *mode=m; return true;
}
bool editor_force_json_read(yyjson_val *v, ObjectForceDescription *f) {
    *f=(ObjectForceDescription){0};
    return common_read(v,&f->mode,&f->target,&f->magnitude) && location_read(yyjson_obj_get(v,"location"),&f->location) && orientation_read(yyjson_obj_get(v,"orientation"),&f->orientation);
}
bool editor_torque_json_read(yyjson_val *v, ObjectTorqueDescription *t) {
    *t=(ObjectTorqueDescription){0}; return common_read(v,&t->mode,&t->target,&t->magnitude);
}
bool editor_sources_json_write(yyjson_mut_doc *d, yyjson_mut_val *v, const EditorObject *o) {
    if(!editor_sources_check(o)) return false;
    for(unsigned torque=0;torque<2;++torque) {
        yyjson_mut_val *arr=yyjson_mut_arr(d); if(!arr) return false;
        size_t count=torque?o->torque_count:o->force_count;
        for(size_t i=0;i<count;++i) {
            yyjson_mut_val *source=torque?editor_torque_json_write(d,o->torque_items[i].description):editor_force_json_write(d,o->force_items[i].description);
            editor_entity_settings_json_write(d, source, torque ? o->torque_items[i].entity_settings : o->force_items[i].entity_settings);
            if(source && !yyjson_mut_obj_add_bool(d,source,"visible",torque?o->torque_items[i].visible:o->force_items[i].visible)) return false;
            if(torque && source && (!yyjson_mut_obj_add_real(d,source,"marker_offset_x",o->torque_items[i].marker_offset.x) ||
                    !yyjson_mut_obj_add_real(d,source,"marker_offset_y",o->torque_items[i].marker_offset.y))) return false;
            if(source && !yyjson_mut_obj_add_bool(d,source,"visible_in_app",
                    !(torque?o->torque_items[i].app_hidden:o->force_items[i].app_hidden))) return false;
            if(source && !yyjson_mut_obj_add_val(d,source,"visuals",object_visuals_json_write(d,
                    torque?o->torque_items[i].visuals:o->force_items[i].visuals))) return false;
            if(!source || !yyjson_mut_obj_add_uint(d,source,"id",torque?o->torque_items[i].id:o->force_items[i].id) ||
                !yyjson_mut_obj_add_strcpy(d,source,"name",torque?o->torque_items[i].name:o->force_items[i].name) || !yyjson_mut_arr_add_val(arr,source)) return false;
        }
        if(!yyjson_mut_obj_add_val(d,v,torque?"torques":"forces",arr)) return false;
    }
    return true;
}
bool editor_sources_json_read(yyjson_val *v, EditorObject *o, EditorProject *p) {
    for(unsigned torque=0;torque<2;++torque) {
        yyjson_val *arr=yyjson_obj_get(v,torque?"torques":"forces");
        if(!yyjson_is_arr(arr) || yyjson_arr_size(arr)>EDITOR_SOURCE_MAX) return false;
        size_t count=yyjson_arr_size(arr);
        if(torque) { if(!EDITOR_ARRAY_RESERVE(o->torque_items,o->torque_capacity,count)) return false; o->torque_count=count; if(count) memset(o->torque_items,0,count*sizeof(*o->torque_items)); }
        else { if(!EDITOR_ARRAY_RESERVE(o->force_items,o->force_capacity,count)) return false; o->force_count=count; if(count) memset(o->force_items,0,count*sizeof(*o->force_items)); }
        for(size_t i=0;i<count;++i) {
            yyjson_val *source=yyjson_arr_get(arr,i);
            if(!editor_entity_settings_json_read(source, torque ? &o->torque_items[i].entity_settings : &o->force_items[i].entity_settings)) return false; uint64_t id;
            yyjson_val *name=yyjson_obj_get(source,"name");
            if(!uint_value(source,"id",&id) || !id || id>=UINT32_MAX || !yyjson_is_str(name) || !yyjson_get_len(name) ||
                    yyjson_get_len(name)>=EDITOR_OBJECT_NAME_MAX || strlen(yyjson_get_str(name))!=yyjson_get_len(name)) return false;
            if(!object_visuals_json_read(yyjson_obj_get(source,"visuals"),
                    torque?&o->torque_items[i].visuals:&o->force_items[i].visuals)) return false;
            yyjson_val *app=yyjson_obj_get(source,"visible_in_app");
            if(app && !yyjson_is_bool(app)) return false;
            if(torque) o->torque_items[i].app_hidden=app && !yyjson_get_bool(app);
            else o->force_items[i].app_hidden=app && !yyjson_get_bool(app);
            if(torque) {
                yyjson_val *visible=yyjson_obj_get(source,"visible");
                if(visible && !yyjson_is_bool(visible)) return false;
                o->torque_items[i].visible=!visible || yyjson_get_bool(visible);
                if((yyjson_obj_get(source,"marker_offset_x") && !number(source,"marker_offset_x",&o->torque_items[i].marker_offset.x)) ||
                   (yyjson_obj_get(source,"marker_offset_y") && !number(source,"marker_offset_y",&o->torque_items[i].marker_offset.y))) return false;
                o->torque_items[i].id=id; strcpy(o->torque_items[i].name,yyjson_get_str(name)); if(!editor_torque_json_read(source,&o->torque_items[i].description) || p->next_torque_id<=id) return false; }
            else { yyjson_val *visible=yyjson_obj_get(source,"visible"); if(visible && !yyjson_is_bool(visible)) return false; o->force_items[i].visible=!visible || yyjson_get_bool(visible); o->force_items[i].id=id; strcpy(o->force_items[i].name,yyjson_get_str(name)); if(!editor_force_json_read(source,&o->force_items[i].description) || p->next_force_id<=id) return false; }
        }
    }
    return editor_sources_check(o);
}

void editor_entity_settings_json_write(yyjson_mut_doc *d, yyjson_mut_val *parent,
        ObjectEntitySettings s) {
    yyjson_mut_val *v = yyjson_mut_obj(d);
#define B(field) yyjson_mut_obj_add_bool(d, v, #field, s.field)
#define F(field) yyjson_mut_obj_add_real(d, v, #field, s.field)
#define U(field) yyjson_mut_obj_add_uint(d, v, #field, s.field)
    F(lifetime.deadline.expiry_time); U(lifetime.deadline.expiry_tick);
    B(lifetime.deadline.time_enabled); B(lifetime.deadline.tick_enabled);
    B(lifetime.time_absolute); B(lifetime.tick_absolute);
    F(angular_acceleration); F(angular_velocity_maximum); B(angular_velocity_maximum_enabled);
    U(gameplay_target.kind); U(gameplay_target.id); U(gameplay_target.soft_body);
    B(axis_lock_enabled); B(axis_lock_world_fixed);
    F(axis_lock.axis.x); F(axis_lock.axis.y);
    F(axis_lock.point_on_axis.x); F(axis_lock.point_on_axis.y);
    U(transform_driver.kind); U(transform_driver.id); U(transform_driver.soft_body);
    F(transform_offset.x); F(transform_offset.y); F(transform_angle);
    B(transform_position); B(transform_orientation); B(transform_inherit_velocity);
#undef B
#undef F
#undef U
    yyjson_mut_obj_add_val(d, parent, "entity_settings", v);
}

bool editor_entity_settings_json_read(yyjson_val *parent, ObjectEntitySettings *value) {
    yyjson_val *v = yyjson_obj_get(parent, "entity_settings");
    ObjectEntitySettings s = {0};
    if(!v) { *value = s; return true; }
    if(!yyjson_is_obj(v)) return false;
#define B(field) do { yyjson_val *p = yyjson_obj_get(v, #field); \
    if(!yyjson_is_bool(p)) return false; s.field = yyjson_get_bool(p); } while(0)
#define F(field) do { if(!number(v, #field, &s.field)) return false; } while(0)
#define U(field) do { if(!uint_value(v, #field, &s.field)) return false; } while(0)
    yyjson_val *time = yyjson_obj_get(v, "lifetime.deadline.expiry_time");
    if(!yyjson_is_num(time)) return false;
    s.lifetime.deadline.expiry_time = yyjson_get_num(time);
    U(lifetime.deadline.expiry_tick);
    B(lifetime.deadline.time_enabled); B(lifetime.deadline.tick_enabled);
    B(lifetime.time_absolute); B(lifetime.tick_absolute);
    F(angular_acceleration); F(angular_velocity_maximum); B(angular_velocity_maximum_enabled);
    uint64_t target_kind, driver_kind;
    if(!uint_value(v, "gameplay_target.kind", &target_kind) || target_kind > OBJECT_TORQUE ||
            !uint_value(v, "transform_driver.kind", &driver_kind) || driver_kind > OBJECT_TORQUE)
        return false;
    s.gameplay_target.kind = (ObjectKind)target_kind;
    s.transform_driver.kind = (ObjectKind)driver_kind;
    U(gameplay_target.id); U(gameplay_target.soft_body);
    B(axis_lock_enabled); B(axis_lock_world_fixed);
    F(axis_lock.axis.x); F(axis_lock.axis.y);
    F(axis_lock.point_on_axis.x); F(axis_lock.point_on_axis.y);
    U(transform_driver.id); U(transform_driver.soft_body);
    F(transform_offset.x); F(transform_offset.y); F(transform_angle);
    B(transform_position); B(transform_orientation); B(transform_inherit_velocity);
#undef B
#undef F
#undef U
    if(!object_entity_settings_check(s)) return false;
    *value = s;
    return true;
}
