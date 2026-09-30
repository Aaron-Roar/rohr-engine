/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "area_geometry.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AREA_EPS 0.000001
static double cross(Vec2D a, Vec2D b, Vec2D c) {
    return ((double)b.x-a.x)*(c.y-a.y)-((double)b.y-a.y)*(c.x-a.x);
}
static Vec2D lerp(Vec2D a, Vec2D b, double t) {
    return (Vec2D){(float)(a.x+(b.x-a.x)*t),(float)(a.y+(b.y-a.y)*t)};
}
static bool intersect(Vec2D a, Vec2D b, Vec2D c, Vec2D d,
        double *t, double *u) {
    double x=b.x-a.x,y=b.y-a.y,xx=d.x-c.x,yy=d.y-c.y;
    double det=x*yy-y*xx;
    if(fabs(det)<AREA_EPS) return false;
    *t=((c.x-a.x)*yy-(c.y-a.y)*xx)/det;
    *u=((c.x-a.x)*y-(c.y-a.y)*x)/det;
    return *t>=-AREA_EPS && *t<=1+AREA_EPS && *u>=-AREA_EPS && *u<=1+AREA_EPS;
}
static bool append(void **items,size_t *count,size_t size,const void *value) {
    if(*count==SIZE_MAX/size) return false;
    void *next=realloc(*items,(*count+1)*size);
    if(!next) return false;
    *items=next; memcpy((char*)next+(*count)++*size,value,size); return true;
}
void area_faces_destroy(AreaFaces *f) {
    if(!f) return;
    for(size_t i=0;i<f->count;i++) free(f->items[i].points);
    free(f->items); *f=(AreaFaces){0};
}
void area_mesh_destroy(AreaMesh *m) {
    if(!m) return;
    free(m->triangles); *m=(AreaMesh){0};
}
typedef struct Vertex { Vec2D p; AreaBoundaryPoint point; } Vertex;
typedef struct Edge { size_t a,b; uint32_t beam; bool visited; } Edge;
static size_t vertex_add(Vertex **v,size_t *n,Vec2D p,AreaBoundaryPoint point) {
    for(size_t i=0;i<*n;i++) if(hypot((*v)[i].p.x-p.x,(*v)[i].p.y-p.y)<AREA_EPS) {
        if(point.beams[0]==0 && ((*v)[i].point.beams[0]!=0 || point.nodes[0]<(*v)[i].point.nodes[0]))
            (*v)[i].point=point;
        return i;
    }
    Vertex value={p,point};
    return append((void**)v,n,sizeof(value),&value)?*n-1:SIZE_MAX;
}
typedef struct Cut { double t; size_t vertex; } Cut;
static int cut_compare(const void *a,const void *b) {
    double x=((const Cut*)a)->t,y=((const Cut*)b)->t; return (x>y)-(x<y);
}
/* Disconnected inner loops bound holes in their containing face. A doubled
 * zero-width bridge encodes multiple rings in one even-odd boundary; edge=0
 * marks the bridge as a drawing seam rather than an authored beam. */
static Vec2D boundary_position(const Vertex *vertices,size_t count,AreaBoundaryPoint p) {
    for(size_t i=0;i<count;i++) {
        AreaBoundaryPoint q=vertices[i].point;
        if(p.beams[0]==q.beams[0] && p.beams[1]==q.beams[1] &&
            memcmp(p.nodes,q.nodes,sizeof(p.nodes))==0) return vertices[i].p;
    }
    return (Vec2D){0};
}
static bool face_holes_add(AreaFaces *faces,const Vertex *vertices,size_t vertex_count) {
    if(faces->count<2) return true;
    AreaFace *original=malloc(faces->count*sizeof(*original));
    size_t *parents=malloc(faces->count*sizeof(*parents));
    double *areas=calloc(faces->count,sizeof(*areas));
    Vec2D **points=calloc(faces->count,sizeof(*points));
    bool ok=false;
    if(!original || !parents || !areas || !points) goto done;
    memcpy(original,faces->items,faces->count*sizeof(*original));
    for(size_t i=0;i<faces->count;i++) {
        parents[i]=SIZE_MAX;
        points[i]=malloc(original[i].count*sizeof(**points));
        if(!points[i]) goto done;
        for(size_t k=0;k<original[i].count;k++) points[i][k]=boundary_position(vertices,vertex_count,original[i].points[k]);
        for(size_t k=0;k<original[i].count;k++) {
            Vec2D a=points[i][k],b=points[i][(k+1)%original[i].count];
            areas[i]+=(double)a.x*b.y-(double)a.y*b.x;
        }
    }
    for(size_t child=0;child<faces->count;child++) for(size_t parent=0;parent<faces->count;parent++) {
        if(areas[parent]<=areas[child] || !area_polygon_contains_check(points[parent],original[parent].count,points[child][0])) continue;
        bool shared=false;
        for(size_t a=0;a<original[parent].count;a++) for(size_t b=0;b<original[child].count;b++)
            shared|=hypot(points[parent][a].x-points[child][b].x,points[parent][a].y-points[child][b].y)<AREA_EPS;
        if(!shared && (parents[child]==SIZE_MAX || areas[parent]<areas[parents[child]])) parents[child]=parent;
    }
    /* Build replacements without changing ownership until all allocations pass. */
    for(size_t parent=0;parent<faces->count;parent++) {
        size_t count=original[parent].count;
        for(size_t child=0;child<faces->count;child++) if(parents[child]==parent) count+=original[child].count+2;
        if(count==original[parent].count) continue;
        AreaBoundaryPoint *ring=malloc(count*sizeof(*ring));
        if(!ring) goto done;
        size_t at=0;
        for(size_t child=0;child<faces->count;child++) if(parents[child]==parent) {
            ring[at]=original[parent].points[0]; ring[at++].edge=0;
            for(size_t k=0;k<original[child].count;k++) {
                size_t index=(original[child].count-k)%original[child].count;
                ring[at]=original[child].points[index];
                ring[at++].edge=original[child].points[(index+original[child].count-1)%original[child].count].edge;
            }
            ring[at]=original[child].points[0]; ring[at++].edge=0;
        }
        memcpy(ring+at,original[parent].points,original[parent].count*sizeof(*ring));
        faces->items[parent]=(AreaFace){ring,count};
    }
    ok=true;
done:
    if(original && points && parents && areas) for(size_t i=0;i<faces->count;i++) {
        if(faces->items[i].points!=original[i].points) free(original[i].points);
    }
    if(points) for(size_t i=0;i<faces->count;i++) free(points[i]);
    free(original); free(parents); free(areas); free(points); return ok;
}
bool area_faces_create(const AreaSegment *s,size_t count,AreaFaces *out) {
    Vertex *v=NULL; size_t nv=0; Edge *e=NULL; size_t ne=0;
    Cut *cuts=NULL; size_t *path=NULL;
    if(!out || (count && !s)) return false;
    *out=(AreaFaces){0};
    for(size_t i=0;i<count;i++) {
        size_t nc=0;
        free(cuts); cuts=NULL;
        for(size_t j=0;j<count;j++) {
            /* Endpoints split collinear overlaps as well as T junctions. */
            Vec2D p[2]={s[j].a,s[j].b}; uint32_t ids[2]={s[j].node_a,s[j].node_b};
            double dx=s[i].b.x-s[i].a.x,dy=s[i].b.y-s[i].a.y,len=dx*dx+dy*dy;
            if(len<AREA_EPS*AREA_EPS) continue;
            for(size_t k=0;k<2;k++) {
                double t=((p[k].x-s[i].a.x)*dx+(p[k].y-s[i].a.y)*dy)/len;
                if(t< -AREA_EPS || t>1+AREA_EPS || fabs(cross(s[i].a,s[i].b,p[k]))>AREA_EPS*sqrt(len)) continue;
                size_t idx=vertex_add(&v,&nv,p[k],(AreaBoundaryPoint){.nodes={ids[k]}});
                Cut c={fmax(0,fmin(1,t)),idx};
                if(idx==SIZE_MAX || !append((void**)&cuts,&nc,sizeof(c),&c)) goto fail;
            }
            if(j==i) continue;
            double t,u;
            if(!intersect(s[i].a,s[i].b,s[j].a,s[j].b,&t,&u) || t<=AREA_EPS || t>=1-AREA_EPS || u<=AREA_EPS || u>=1-AREA_EPS) continue;
            size_t a=s[i].id<s[j].id?i:j,b=a==i?j:i;
            AreaBoundaryPoint point={.nodes={s[a].node_a,s[a].node_b,s[b].node_a,s[b].node_b},
                .beams={s[a].id,s[b].id},.fractions={(float)(a==i?t:u),(float)(a==i?u:t)}};
            size_t idx=vertex_add(&v,&nv,lerp(s[i].a,s[i].b,t),point);
            Cut c={t,idx};
            if(idx==SIZE_MAX || !append((void**)&cuts,&nc,sizeof(c),&c)) goto fail;
        }
        if(nc>1) qsort(cuts,nc,sizeof(*cuts),cut_compare);
        for(size_t k=1;k<nc;k++) {
            size_t a=cuts[k-1].vertex,b=cuts[k].vertex;
            if(a==b) continue;
            bool duplicate=false;
            for(size_t j=0;j<ne;j++) if(e[j].a==a && e[j].b==b) { duplicate=true; break; }
            if(duplicate) continue;
            Edge forward={a,b,s[i].id,false},back={b,a,s[i].id,false};
            if(!append((void**)&e,&ne,sizeof(forward),&forward) || !append((void**)&e,&ne,sizeof(back),&back)) goto fail;
        }
    }
    path=malloc((ne+1)*sizeof(*path)); if(!path) goto fail;
    for(size_t start=0;start<ne;start++) {
        if(e[start].visited) continue;
        size_t at=start,n=0; bool closed=false;
        for(size_t step=0;step<=ne;step++) {
            if(e[at].visited) { closed=at==start; break; }
            e[at].visited=true; path[n++]=at;
            double incoming=atan2(v[e[at].a].p.y-v[e[at].b].p.y,v[e[at].a].p.x-v[e[at].b].p.x);
            size_t next=SIZE_MAX; double best=10;
            for(size_t j=0;j<ne;j++) if(e[j].a==e[at].b) {
                double angle=atan2(v[e[j].b].p.y-v[e[j].a].p.y,v[e[j].b].p.x-v[e[j].a].p.x);
                double turn=fmod(incoming-angle+2*3.141592653589793,2*3.141592653589793);
                if(turn<AREA_EPS) turn=2*3.141592653589793;
                if(turn<best) { best=turn; next=j; }
            }
            if(next==SIZE_MAX) break;
            at=next;
        }
        if(!closed || n<3) continue;
        /* Remove excursions along bridges, which do not enclose a surface. */
        bool changed=true;
        while(changed && n>=3) {
            changed=false;
            for(size_t k=0;k<n;k++) if(e[path[k]].a==e[path[(k+1)%n]].b) {
                size_t next=(k+1)%n;
                if(next==0) { memmove(path,path+1,(n-2)*sizeof(*path)); n-=2; }
                else { memmove(path+k,path+k+2,(n-k-2)*sizeof(*path)); n-=2; }
                changed=true; break;
            }
        }
        double area=0;
        for(size_t k=0;k<n;k++) { Vec2D a=v[e[path[k]].a].p,b=v[e[path[k]].b].p; area+=(double)a.x*b.y-(double)a.y*b.x; }
        if(n<3 || area<=AREA_EPS) continue;
        AreaFace face={.points=malloc(n*sizeof(*face.points)),.count=n};
        if(!face.points) goto fail;
        for(size_t k=0;k<n;k++) { face.points[k]=v[e[path[k]].a].point; face.points[k].edge=e[path[k]].beam; }
        if(!append((void**)&out->items,&out->count,sizeof(face),&face)) { free(face.points); goto fail; }
    }
    if(!face_holes_add(out,v,nv)) goto fail;
    free(v); free(e); free(cuts); free(path); return true;
fail:
    free(v); free(e); free(cuts); free(path); area_faces_destroy(out); return false;
}
static int real_compare(const void *a,const void *b) { double x=*(const double*)a,y=*(const double*)b; return (x>y)-(x<y); }
static double edge_x(Vec2D a,Vec2D b,double y) { return a.x+(y-a.y)*(b.x-a.x)/(b.y-a.y); }
bool area_polygon_contains_check(const Vec2D *p,size_t n,Vec2D q) {
    bool inside=false; if(!p || n<3) return false;
    for(size_t i=0,j=n-1;i<n;j=i++) if((p[i].y>q.y)!=(p[j].y>q.y) && q.x<edge_x(p[i],p[j],q.y)) inside=!inside;
    return inside;
}
bool area_mesh_create(const Vec2D *p,size_t n,AreaMesh *out) {
    double *ys=NULL; size_t ny=0; Cut *hits=NULL;
    if(!out || (n && !p)) return false;
    *out=(AreaMesh){0}; if(n<3) return true;
    for(size_t i=0;i<n;i++) {
        if(!isfinite(p[i].x)||!isfinite(p[i].y)) goto fail;
        double y=p[i].y;
        if(!append((void**)&ys,&ny,sizeof(y),&y)) goto fail;
        for(size_t j=i+1;j<n;j++) { double t,u;
            if(intersect(p[i],p[(i+1)%n],p[j],p[(j+1)%n],&t,&u)) {
                y=lerp(p[i],p[(i+1)%n],t).y;
                if(!append((void**)&ys,&ny,sizeof(y),&y)) goto fail;
            }
        }
    }
    qsort(ys,ny,sizeof(*ys),real_compare);
    hits=malloc(n*sizeof(*hits)); if(!hits) goto fail;
    for(size_t strip=1;strip<ny;strip++) {
        double lo=ys[strip-1],hi=ys[strip],mid=(lo+hi)*.5; size_t nh=0;
        if(hi-lo<AREA_EPS) continue;
        for(size_t i=0;i<n;i++) { Vec2D a=p[i],b=p[(i+1)%n];
            if((a.y>mid)==(b.y>mid)) continue;
            hits[nh++]=(Cut){edge_x(a,b,mid),i};
        }
        qsort(hits,nh,sizeof(*hits),cut_compare);
        for(size_t k=1;k<nh;k+=2) {
            size_t l=hits[k-1].vertex,r=hits[k].vertex;
            Vec2D a={(float)edge_x(p[l],p[(l+1)%n],lo),(float)lo};
            Vec2D b={(float)edge_x(p[r],p[(r+1)%n],lo),(float)lo};
            Vec2D c={(float)edge_x(p[r],p[(r+1)%n],hi),(float)hi};
            Vec2D d={(float)edge_x(p[l],p[(l+1)%n],hi),(float)hi};
            Vec2D tris[2][3]={{a,b,c},{a,c,d}};
            for(size_t t=0;t<2;t++) if(fabs(cross(tris[t][0],tris[t][1],tris[t][2]))>AREA_EPS &&
                !append((void**)&out->triangles,&out->count,sizeof(tris[t]),tris[t])) goto fail;
        }
    }
    free(ys); free(hits); return true;
fail:
    free(ys); free(hits); area_mesh_destroy(out); return false;
}
double area_mesh_overlap_get(const AreaMesh *a,const AreaMesh *b) {
    double total=0;
    if(!a||!b) return 0;
    for(size_t i=0;i<a->count;i++) for(size_t j=0;j<b->count;j++) {
        Vec2D p[12],q[12]; size_t n=3; memcpy(p,a->triangles[i],3*sizeof(*p));
        double sign=cross(b->triangles[j][0],b->triangles[j][1],b->triangles[j][2])>=0?1:-1;
        for(size_t edge=0;edge<3 && n;edge++) {
            size_t m=0; Vec2D x=b->triangles[j][edge],y=b->triangles[j][(edge+1)%3];
            for(size_t k=0,prev=n-1;k<n;prev=k++) {
                double ca=sign*cross(x,y,p[prev]),cb=sign*cross(x,y,p[k]);
                if((ca>=0)!=(cb>=0)) q[m++]=lerp(p[prev],p[k],ca/(ca-cb));
                if(cb>=0) q[m++]=p[k];
            }
            n=m; memcpy(p,q,n*sizeof(*p));
        }
        double area=0; for(size_t k=0;k<n;k++) area+=(double)p[k].x*p[(k+1)%n].y-(double)p[k].y*p[(k+1)%n].x;
        total+=fabs(area)*.5;
    }
    return total;
}
