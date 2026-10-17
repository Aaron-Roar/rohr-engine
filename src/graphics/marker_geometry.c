/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "marker_geometry.h"
#include "entity/object_visuals.h"
#include <math.h>

typedef struct MarkerDraw {
    const MarkerGeometry *geometry;
    ObjectVisualStyle assembly;
    MarkerPolygonDraw draw;
    void *context;
} MarkerDraw;
static Position transform(Position p,ObjectVisualStyle s) {
    p=math_vector_rotate((Position){p.x*s.scale.x,p.y*s.scale.y},s.rotation);
    return (Position){p.x+s.offset.x,p.y+s.offset.y};
}
static bool emit(MarkerDraw *d,const Position *points,size_t count,uint32_t rgba,
        const ObjectVisualStyle *endpoint,Position center) {
    if(!(rgba&255u) || d->assembly.scale.x==0 || d->assembly.scale.y==0 ||
            (endpoint && (endpoint->scale.x==0 || endpoint->scale.y==0))) return true;
    Position world[MAX_VERTICIES];
    if(count>MAX_VERTICIES) return false;
    for(size_t i=0;i<count;++i) {
        Position p=points[i];
        if(endpoint) p=transform(p,*endpoint);
        p.x+=center.x; p.y+=center.y;
        p=transform(p,d->assembly);
        p=math_vector_rotate(p,d->geometry->orientation);
        p.x+=d->geometry->origin.x; p.y+=d->geometry->origin.y;
        if(!isfinite(p.x) || !isfinite(p.y)) return false;
        world[i]=p;
    }
    Color color=graphics_color_hex_create(rgba);
    if(d->geometry->kind!=OBJECT_JOINT && d->geometry->magnitude==0) color.alpha/=2;
    return d->draw(world,count,color,d->context);
}
/* Bounded miter offsets keep sharp corners finite. Border strips do not cover
 * the fill, so translucent fills retain their requested alpha. */
static bool polygon(MarkerDraw *d,const Position *p,size_t n,ObjectVisualStyle style,
        const ObjectVisualStyle *endpoint,Position center) {
    if(n<3 || n>MAX_VERTICIES) return false;
    if(style.border_thickness>0 && (style.border_color&255u)) {
        double area=0;
        for(size_t i=0;i<n;++i) area+=(double)p[i].x*p[(i+1)%n].y-(double)p[i].y*p[(i+1)%n].x;
        if(fabs(area)<1e-10) return true;
        float sign=area>0?1:-1;
        Position outer[MAX_VERTICIES];
        for(size_t i=0;i<n;++i) {
            Position a=p[(i+n-1)%n],b=p[i],c=p[(i+1)%n];
            float l=hypotf(b.x-a.x,b.y-a.y),r=hypotf(c.x-b.x,c.y-b.y);
            if(l<1e-6f || r<1e-6f) return true;
            Vec2D u={sign*(b.y-a.y)/l,-sign*(b.x-a.x)/l};
            Vec2D v={sign*(c.y-b.y)/r,-sign*(c.x-b.x)/r};
            float denominator=1+u.x*v.x+u.y*v.y;
            float k=style.border_thickness/fmaxf(0.25f,denominator);
            outer[i]=(Position){b.x+(u.x+v.x)*k,b.y+(u.y+v.y)*k};
        }
        for(size_t i=0;i<n;++i) {
            size_t j=(i+1)%n;
            Position strip[]={p[i],outer[i],outer[j],p[j]};
            if(!emit(d,strip,4,style.border_color,endpoint,center)) return false;
        }
    }
    return emit(d,p,n,style.fill_color,endpoint,center);
}
static bool circle(MarkerDraw *d,float radius,ObjectVisualStyle style,
        const ObjectVisualStyle *endpoint,Position center) {
    Position p[32];
    for(size_t i=0;i<32;++i) {
        float angle=(float)i*2*PI_F/32;
        p[i]=(Position){radius*cosf(angle),radius*sinf(angle)};
    }
    return polygon(d,p,32,style,endpoint,center);
}
static bool stroke(MarkerDraw *d,const Position *path,size_t n,ObjectVisualStyle style) {
    Position p[MAX_VERTICIES];
    if(n<2 || n*2>MAX_VERTICIES) return false;
    for(size_t i=0;i<n;++i) {
        Position a=path[i?i-1:0],b=path[i],c=path[i+1<n?i+1:i];
        Vec2D u={b.x-a.x,b.y-a.y},v={c.x-b.x,c.y-b.y};
        float l=hypotf(u.x,u.y),r=hypotf(v.x,v.y);
        if(l<1e-6f) { u=v; l=r; }
        if(r<1e-6f) { v=u; r=l; }
        if(l<1e-6f || r<1e-6f) return true;
        u=(Vec2D){-u.y/l,u.x/l}; v=(Vec2D){-v.y/r,v.x/r};
        float k=2/fmaxf(0.25f,1+u.x*v.x+u.y*v.y);
        Vec2D offset={(u.x+v.x)*k,(u.y+v.y)*k};
        p[i]=(Position){b.x+offset.x,b.y+offset.y};
        p[2*n-1-i]=(Position){b.x-offset.x,b.y-offset.y};
    }
    return polygon(d,p,n*2,style,NULL,(Position){0});
}
bool graphics_marker_geometry_draw(const MarkerGeometry *g,MarkerPolygonDraw draw,void *context) {
    if(!g || !draw || !object_visuals_check(g->visuals)) return false;
    MarkerDraw d={g,object_visual_style_get(g->visuals.main,g->kind),draw,context};
    if(g->kind==OBJECT_FORCE) {
        Position p[]={{-2,0},{2,0},{2,50},{6,50},{0,60},{-6,50},{-2,50}};
        return polygon(&d,p,7,d.assembly,NULL,(Position){0});
    }
    if(g->kind==OBJECT_TORQUE) {
        Position p[65]; float sign=g->magnitude<0?-1:1;
        /* Leave the final arc segment for a head integrated into the silhouette. */
        for(size_t i=0;i<31;++i) {
            float a=sign*(float)i/32*1.5f*PI_F;
            p[i]=(Position){24*sinf(a),24*cosf(a)};
            p[64-i]=(Position){20*sinf(a),20*cosf(a)};
        }
        float a=sign*30.0f/32*1.5f*PI_F;
        p[31]=(Position){28*sinf(a),28*cosf(a)};
        p[32]=(Position){-sign*22,0};
        p[33]=(Position){16*sinf(a),16*cosf(a)};
        /* p[34] is the inner arc's final sample. */
        return polygon(&d,p,65,d.assembly,NULL,(Position){0});
    }
    if(g->kind!=OBJECT_JOINT) return false;
    if(g->joint==JOINT_PIN) return circle(&d,9,d.assembly,NULL,(Position){0});
    if(g->joint==JOINT_WELD) {
        Position p[]={{-9,-9},{9,-9},{9,9},{-9,9}};
        return polygon(&d,p,4,d.assembly,NULL,(Position){0});
    }
    if(g->joint!=JOINT_SPRING) return false;
    ObjectVisualStyle a=object_visual_style_get(g->visuals.endpoint_a,OBJECT_JOINT);
    ObjectVisualStyle b=object_visual_style_get(g->visuals.endpoint_b,OBJECT_JOINT);
    Position ca={g->endpoint_a.x-g->origin.x,g->endpoint_a.y-g->origin.y};
    Position cb={g->endpoint_b.x-g->origin.x,g->endpoint_b.y-g->origin.y};
    Position start={ca.x+a.offset.x,ca.y+a.offset.y},end={cb.x+b.offset.x,cb.y+b.offset.y};
    Vec2D delta={end.x-start.x,end.y-start.y}; float length=hypotf(delta.x,delta.y);
    if(length>0.001f) {
        Position path[10];
        for(size_t i=0;i<10;++i) {
            float t=(float)i/9,offset=i==0 || i==9?0:(i%2? -7:7);
            path[i]=(Position){start.x+t*delta.x-offset*delta.y/length,
                start.y+t*delta.y+offset*delta.x/length};
        }
        if(!stroke(&d,path,10,d.assembly)) return false;
    }
    return circle(&d,4,a,&a,ca) && circle(&d,4,b,&b,cb);
}
