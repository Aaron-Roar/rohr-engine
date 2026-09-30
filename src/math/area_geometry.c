/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "area_geometry.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define AREA_EPS 0.000001
/* Promote before subtraction, including both factors of each product.
 * Mixing rounded float differences with double differences can make even
 * cross(a, b, b) nonzero and disconnect a beam from its own endpoint. */
static double cross(Vec2D a, Vec2D b, Vec2D c) {
    return ((double)b.x-a.x)*((double)c.y-a.y)-
        ((double)b.y-a.y)*((double)c.x-a.x);
}
static Vec2D lerp(Vec2D a, Vec2D b, double t) {
    return (Vec2D){(float)(a.x+((double)b.x-a.x)*t),
        (float)(a.y+((double)b.y-a.y)*t)};
}
static bool intersect(Vec2D a, Vec2D b, Vec2D c, Vec2D d,
        double *t, double *u) {
    double x=(double)b.x-a.x,y=(double)b.y-a.y;
    double xx=(double)d.x-c.x,yy=(double)d.y-c.y;
    double det=x*yy-y*xx;
    if(fabs(det)<AREA_EPS) return false;
    double dx=(double)c.x-a.x,dy=(double)c.y-a.y;
    *t=(dx*yy-dy*xx)/det;
    *u=(dx*y-dy*x)/det;
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
    free(m->triangles); free(m->edges); *m=(AreaMesh){0};
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
            double dx=(double)s[i].b.x-s[i].a.x,dy=(double)s[i].b.y-s[i].a.y,len=dx*dx+dy*dy;
            if(len<AREA_EPS*AREA_EPS) continue;
            for(size_t k=0;k<2;k++) {
                double t=(((double)p[k].x-s[i].a.x)*dx+((double)p[k].y-s[i].a.y)*dy)/len;
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
            /* Both visits to the same pair must produce the identical float
             * position; evaluating once along each beam can leave two nearby
             * graph vertices and break the face walk. */
            size_t idx=vertex_add(&v,&nv,lerp(s[a].a,s[a].b,a==i?t:u),point);
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
static double edge_x(Vec2D a,Vec2D b,double y) {
    return a.x+(y-a.y)*((double)b.x-a.x)/((double)b.y-a.y);
}
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

static const AreaSegment *segment_get(const AreaSegment *segments, size_t count,
        uint32_t id) {
    for(size_t i = 0; i < count; i++) if(segments[i].id == id) return &segments[i];
    return NULL;
}

bool area_boundary_position_get(const AreaSegment *segments, size_t count,
        AreaBoundaryPoint point, Vec2D *out) {
    if(out == NULL || (count && segments == NULL)) return false;
    if(point.beams[0] == 0) {
        for(size_t i = 0; i < count; i++) {
            if(segments[i].node_a == point.nodes[0]) { *out = segments[i].a; return true; }
            if(segments[i].node_b == point.nodes[0]) { *out = segments[i].b; return true; }
        }
        return false;
    }
    const AreaSegment *a = segment_get(segments, count, point.beams[0]);
    const AreaSegment *b = segment_get(segments, count, point.beams[1]);
    double t, u;
    if(a == NULL || b == NULL || !intersect(a->a, a->b, b->a, b->b, &t, &u)) return false;
    *out = lerp(a->a, a->b, fmax(0, fmin(1, t)));
    return true;
}

static double corner_fraction_get(AreaBoundaryPoint point, const AreaSegment *beam) {
    if(point.beams[0] == 0) return point.nodes[0] == beam->node_a ? 0 : 1;
    return point.fractions[point.beams[0] == beam->id ? 0 : 1];
}

/* A surviving turn identifies its adjacent region even when another crossing
 * of the authored boundary has disappeared. More than one definition can
 * claim the same region: drawing order resolves that overlap without erasing
 * either definition. Compare beam rays as well as IDs at crossing corners. */
static bool boundary_turn_matches_check(const AreaSegment *segments, size_t count,
        const AreaBoundaryPoint *boundary, size_t n, const AreaFace *face) {
    for(size_t i = 0; i < n; i++) {
        AreaBoundaryPoint a = boundary[i], before = boundary[(i + n - 1) % n];
        if(a.edge == 0 || before.edge == 0) continue;
        for(size_t j = 0; j < face->count; j++) {
            AreaBoundaryPoint b = face->points[j];
            AreaBoundaryPoint prev = face->points[(j + face->count - 1) % face->count];
            if(a.beams[0] == 0 ? b.beams[0] != 0 || a.nodes[0] != b.nodes[0] :
                    !((a.beams[0] == b.beams[0] && a.beams[1] == b.beams[1]) ||
                    (a.beams[0] == b.beams[1] && a.beams[1] == b.beams[0]))) continue;
            bool forward = a.edge == b.edge && before.edge == prev.edge;
            bool reverse = a.edge == prev.edge && before.edge == b.edge;
            if(!forward && !reverse) continue;
            if(a.beams[0] == 0) return true;
            const AreaSegment *incoming = segment_get(segments, count, before.edge);
            const AreaSegment *outgoing = segment_get(segments, count, a.edge);
            if(incoming == NULL || outgoing == NULL) continue;
            AreaBoundaryPoint next = boundary[(i + 1) % n];
            AreaBoundaryPoint after = face->points[(j + 1) % face->count];
            double in = corner_fraction_get(before, incoming) - corner_fraction_get(a, incoming);
            double out = corner_fraction_get(next, outgoing) - corner_fraction_get(a, outgoing);
            double other_in = corner_fraction_get(forward ? prev : after, incoming) - corner_fraction_get(b, incoming);
            double other_out = corner_fraction_get(forward ? after : prev, outgoing) - corner_fraction_get(b, outgoing);
            if(in * other_in > 0 && out * other_out > 0) return true;
        }
    }
    return false;
}

static bool boundary_edge_check(const AreaBoundaryPoint *boundary, size_t count,
        uint32_t edge) {
    bool hole = false;
    for(size_t i = 0; i < count; i++) {
        if(boundary[i].edge == 0) { hole = !hole; continue; }
        if(!hole && boundary[i].edge == edge) return true;
    }
    return false;
}

static bool mesh_face_append(AreaMesh *out, const AreaFace *face,
        const AreaSegment *segments, size_t count) {
    Vec2D *points = malloc(face->count * sizeof(*points));
    AreaMesh mesh = {0};
    bool ok = false;
    if(points == NULL) return false;
    for(size_t i = 0; i < face->count; i++)
        if(!area_boundary_position_get(segments, count, face->points[i], &points[i])) goto done;
    if(!area_mesh_create(points, face->count, &mesh)) goto done;
    for(size_t i = 0; i < mesh.count; i++)
        if(!append((void **)&out->triangles, &out->count, sizeof(*out->triangles), mesh.triangles[i])) goto done;
    for(size_t i = 0; i < face->count; i++) {
        if(face->points[i].edge == 0) continue;
        Vec2D edge[2] = {points[i], points[(i + 1) % face->count]};
        size_t shared = SIZE_MAX;
        for(size_t j = 0; j < out->edge_count; j++) {
            const Vec2D *other = out->edges[j];
            if(hypot(edge[0].x-other[1].x, edge[0].y-other[1].y) < AREA_EPS &&
                    hypot(edge[1].x-other[0].x, edge[1].y-other[0].y) < AREA_EPS) { shared = j; break; }
        }
        if(shared != SIZE_MAX) {
            --out->edge_count;
            if(shared != out->edge_count)
                memcpy(out->edges[shared], out->edges[out->edge_count], sizeof(*out->edges));
        }
        else if(!append((void **)&out->edges, &out->edge_count, sizeof(*out->edges), edge)) goto done;
    }
    ok = true;
done:
    free(points);
    area_mesh_destroy(&mesh);
    return ok;
}

bool area_boundary_mesh_create(const AreaSegment *segments, size_t count,
        const AreaBoundaryPoint *boundary, size_t n, AreaMesh *out) {
    AreaFaces faces = {0};
    AreaSegment *local = NULL;
    Vec2D *points = NULL;
    bool *holes = NULL;
    bool complete = true, has_edges = false, ok = false;
    size_t local_count = 0;
    if(out == NULL) return false;
    *out = (AreaMesh){0};
    if((count && segments == NULL) || (n && boundary == NULL)) return false;
    if(n < 3) return true;
    points = malloc(n * sizeof(*points));
    local = calloc(n, sizeof(*local));
    holes = calloc(n, sizeof(*holes));
    if(points == NULL || local == NULL || holes == NULL) goto done;
    for(size_t i = 0; i < n; i++) {
        complete &= area_boundary_position_get(segments, count, boundary[i], &points[i]);
        has_edges |= boundary[i].edge != 0;
    }
    if(complete) {
        /* Resolve the authored beam portions first, then find all bounded
         * regions of that graph. This fills self-crossing lobes and enclosed
         * overlap regions without even-odd cancellation. Explicit hole rings
         * remain holes; edge=0 seams are never drawn. */
        bool hole = false;
        for(size_t i = 0; i < n; i++) {
            if(has_edges && boundary[i].edge == 0) { hole = !hole; continue; }
            local[local_count] = (AreaSegment){(uint32_t)local_count + 1,
                (uint32_t)i + 1, (uint32_t)((i + 1) % n) + 1,
                points[i], points[(i + 1) % n]};
            holes[local_count++] = hole;
        }
        if(!area_faces_create(local, local_count, &faces)) goto done;
        for(size_t i = 0; i < faces.count; i++) {
            bool only_hole = true;
            for(size_t k = 0; k < faces.items[i].count; k++) {
                uint32_t edge = faces.items[i].points[k].edge;
                if(edge && !holes[edge - 1]) only_hole = false;
            }
            if(!only_hole && !mesh_face_append(out, &faces.items[i], local, local_count)) goto done;
        }
    } else {
        if(!area_faces_create(segments, count, &faces)) goto done;
        for(size_t i = 0; i < faces.count; i++)
            if(boundary_turn_matches_check(segments, count, boundary, n, &faces.items[i]) &&
                    !mesh_face_append(out, &faces.items[i], segments, count)) goto done;
    }
    ok = true;
done:
    free(points); free(local); free(holes);
    area_faces_destroy(&faces);
    if(!ok) area_mesh_destroy(out);
    return ok;
}

bool area_mesh_contains_check(const AreaMesh *mesh, Vec2D point) {
    if(mesh == NULL) return false;
    for(size_t i = 0; i < mesh->count; i++) {
        const Vec2D *p = mesh->triangles[i];
        double a = cross(p[0], p[1], point), b = cross(p[1], p[2], point), c = cross(p[2], p[0], point);
        if((a >= -AREA_EPS && b >= -AREA_EPS && c >= -AREA_EPS) ||
                (a <= AREA_EPS && b <= AREA_EPS && c <= AREA_EPS)) return true;
    }
    return false;
}

static bool boundary_face_matches_check(const AreaFace *definition, const AreaFace *face) {
    if(definition->count != face->count) return false;
    size_t count = face->count;
    for(size_t start = 0; start < count; start++) for(size_t reverse = 0; reverse < 2; reverse++) {
        bool matches = true;
        for(size_t i = 0; i < count; i++) {
            size_t at = (start + (reverse ? count - i : i)) % count;
            const AreaBoundaryPoint *a = &definition->points[at], *b = &face->points[i];
            bool same_corner = a->beams[0] == 0 ?
                b->beams[0] == 0 && a->nodes[0] == b->nodes[0] :
                (a->beams[0] == b->beams[0] && a->beams[1] == b->beams[1]) ||
                (a->beams[0] == b->beams[1] && a->beams[1] == b->beams[0]);
            size_t edge = reverse ? (at + count - 1) % count : at;
            if(!same_corner || definition->points[edge].edge != b->edge) { matches = false; break; }
        }
        if(matches) return true;
    }
    return false;
}

bool area_boundary_meshes_create(const AreaSegment *segments, size_t count,
        const AreaFace *definitions, size_t definition_count, AreaMesh *out) {
    AreaFaces faces = {0};
    if(definition_count && (definitions == NULL || out == NULL)) return false;
    for(size_t i = 0; i < definition_count; i++) out[i] = (AreaMesh){0};
    if(definition_count == 0) return true;
    for(size_t i = 0; i < definition_count; i++)
        if(definitions[i].count && definitions[i].points == NULL) return false;
    if(!area_faces_create(segments, count, &faces)) goto fail;
    for(size_t i = 0; i < definition_count; i++) {
        bool matched = false;
        /* Unchanged boundaries have an exact owner. Build directly from the
         * current graph instead of independently rediscovering the same cell
         * and treating a numerical disagreement as a newly merged region. */
        for(size_t j = 0; j < faces.count; j++)
            if(boundary_face_matches_check(&definitions[i], &faces.items[j])) {
                if(!mesh_face_append(&out[i], &faces.items[j], segments, count)) goto fail;
                matched = true;
                break;
            }
        if(!matched && !area_boundary_mesh_create(segments, count,
                definitions[i].points, definitions[i].count, &out[i])) goto fail;
    }
    for(size_t i = 0; i < faces.count; i++) {
        AreaMesh region = {0};
        if(!mesh_face_append(&region, &faces.items[i], segments, count)) { area_mesh_destroy(&region); goto fail; }
        if(region.count == 0) { area_mesh_destroy(&region); continue; }
        size_t largest = 0;
        for(size_t k = 1; k < region.count; k++) {
            const Vec2D *a = region.triangles[k], *b = region.triangles[largest];
            if(fabs(cross(a[0], a[1], a[2])) > fabs(cross(b[0], b[1], b[2]))) largest = k;
        }
        const Vec2D *p = region.triangles[largest];
        Vec2D sample = {(p[0].x+p[1].x+p[2].x)/3, (p[0].y+p[1].y+p[2].y)/3};
        bool covered = false;
        for(size_t j = 0; j < definition_count; j++) covered |= area_mesh_contains_check(&out[j], sample);
        area_mesh_destroy(&region);
        if(covered) continue;
        /* Motion can enclose a region using portions of several definitions,
         * without retaining any old corner. Every contributing definition
         * covers that region; layers/order choose its visible owner. Disabled
         * and hidden definitions still participate in the coverage test above,
         * so deliberately unfilled regions are not recolored by neighbors. */
        for(size_t j = 0; j < definition_count; j++) {
            bool contributes = false;
            for(size_t k = 0; k < faces.items[i].count; k++) {
                uint32_t edge = faces.items[i].points[k].edge;
                if(edge) contributes |= boundary_edge_check(definitions[j].points, definitions[j].count, edge);
            }
            if(contributes && !mesh_face_append(&out[j], &faces.items[i], segments, count)) goto fail;
        }
    }
    area_faces_destroy(&faces);
    return true;
fail:
    area_faces_destroy(&faces);
    for(size_t i = 0; i < definition_count; i++) area_mesh_destroy(&out[i]);
    return false;
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
