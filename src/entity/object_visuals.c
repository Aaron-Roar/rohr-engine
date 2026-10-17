/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "object_visuals.h"
#include <math.h>
#include <float.h>
ObjectVisualStyle object_visual_style_get(ObjectVisualStyle v, ObjectKind kind) {
    if(v.specified) return v;
    return (ObjectVisualStyle){.specified=true,
        .fill_color=(kind==OBJECT_FORCE || kind==OBJECT_TORQUE)?0xff6633ffu:kind==OBJECT_JOINT?0xe1be55ffu:0xf0b446ffu,
        .border_color=0x20242cffu,.border_thickness=1,.scale={1,1}};
}
bool object_visuals_check(ObjectVisuals value) {
    ObjectVisualStyle parts[]={value.main,value.endpoint_a,value.endpoint_b};
    for(size_t i=0;i<3;++i) {
        ObjectVisualStyle v=parts[i];
        if(v.specified && (!isfinite(v.border_thickness) || v.border_thickness<0 ||
                !isfinite(v.scale.x) || !isfinite(v.scale.y) || !isfinite(v.offset.x) ||
                !isfinite(v.offset.y) || !isfinite(v.rotation))) return false;
    }
    return true;
}
static yyjson_mut_val *style_write(yyjson_mut_doc *d,ObjectVisualStyle v) {
    yyjson_mut_val *o=yyjson_mut_obj(d);
    if(!o) return NULL;
    if(!v.specified) return o;
    if(!yyjson_mut_obj_add_uint(d,o,"fill_color",v.fill_color) ||
       !yyjson_mut_obj_add_uint(d,o,"border_color",v.border_color) ||
       !yyjson_mut_obj_add_real(d,o,"border_thickness",v.border_thickness) ||
       !yyjson_mut_obj_add_real(d,o,"scale_x",v.scale.x) ||
       !yyjson_mut_obj_add_real(d,o,"scale_y",v.scale.y) ||
       !yyjson_mut_obj_add_real(d,o,"offset_x",v.offset.x) ||
       !yyjson_mut_obj_add_real(d,o,"offset_y",v.offset.y) ||
       !yyjson_mut_obj_add_real(d,o,"rotation",v.rotation)) return NULL;
    return o;
}
yyjson_mut_val *object_visuals_json_write(yyjson_mut_doc *d,ObjectVisuals v) {
    if(!d || !object_visuals_check(v)) return NULL;
    yyjson_mut_val *o=yyjson_mut_obj(d);
    if(!o || !yyjson_mut_obj_add_val(d,o,"main",style_write(d,v.main)) ||
       !yyjson_mut_obj_add_val(d,o,"endpoint_a",style_write(d,v.endpoint_a)) ||
       !yyjson_mut_obj_add_val(d,o,"endpoint_b",style_write(d,v.endpoint_b))) return NULL;
    return o;
}
static bool style_read(yyjson_val *o,ObjectVisualStyle *v) {
    *v=(ObjectVisualStyle){0};
    if(!o) return true;
    if(!yyjson_is_obj(o)) return false;
    if(!yyjson_obj_size(o)) return true;
    if(yyjson_obj_size(o)!=8) return false;
    yyjson_val *fill=yyjson_obj_get(o,"fill_color"),*border=yyjson_obj_get(o,"border_color");
    if(!yyjson_is_uint(fill) || yyjson_get_uint(fill)>UINT32_MAX ||
       !yyjson_is_uint(border) || yyjson_get_uint(border)>UINT32_MAX) return false;
    v->fill_color=(uint32_t)yyjson_get_uint(fill); v->border_color=(uint32_t)yyjson_get_uint(border);
    const char *names[]={"border_thickness","scale_x","scale_y","offset_x","offset_y","rotation"};
    float *values[]={&v->border_thickness,&v->scale.x,&v->scale.y,&v->offset.x,&v->offset.y,&v->rotation};
    for(size_t i=0;i<6;++i) {
        yyjson_val *n=yyjson_obj_get(o,names[i]); double x=yyjson_get_num(n);
        if(!yyjson_is_num(n) || !isfinite(x) || fabs(x)>FLT_MAX) return false;
        *values[i]=(float)x;
    }
    v->specified=true; return true;
}
bool object_visuals_json_read(yyjson_val *o,ObjectVisuals *output) {
    if(!output) return false;
    ObjectVisuals v={0};
    if(o && (!yyjson_is_obj(o) ||
       !style_read(yyjson_obj_get(o,"main"),&v.main) ||
       !style_read(yyjson_obj_get(o,"endpoint_a"),&v.endpoint_a) ||
       !style_read(yyjson_obj_get(o,"endpoint_b"),&v.endpoint_b))) return false;
    if(!object_visuals_check(v)) return false;
    *output=v; return true;
}
