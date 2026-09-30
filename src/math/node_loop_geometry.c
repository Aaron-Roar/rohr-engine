/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "node_loop_geometry.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* One loop has n edges, each split by at most 2n endpoints/intersections. */
#define GRAPH_LIMIT (NODE_LOOP_MAX_POINTS * (2 * NODE_LOOP_MAX_POINTS + 1))
typedef struct Point { double x, y; } Point;
#define VERTEX_BUCKETS 32768
typedef struct Vertex { Point p; int head, hash_next; } Vertex;
typedef struct HalfEdge {
    int from, to, next_at_vertex, next, face;
    double angle;
} HalfEdge;
typedef struct Segment { Point a, b; uint32_t loops; } Segment;
typedef struct Crossing { double x; size_t segment; } Crossing;
struct NodeLoopWorkspace {
    Vertex vertices[GRAPH_LIMIT];
    int vertex_buckets[VERTEX_BUCKETS];
    HalfEdge edges[GRAPH_LIMIT * 2];
    bool exterior[GRAPH_LIMIT * 2];
    size_t vertex_count, edge_count;
    Segment *segments;
    size_t segment_count, segment_capacity;
    double *levels;
    size_t level_count, level_capacity;
    Crossing *crossings;
    size_t crossing_capacity;
    NodeLoopFill staging;
    Point tolerance;
    Point origin;
};

static bool reserve(void **data, size_t *capacity, size_t needed, size_t size) {
    if(needed <= *capacity) return true;
    size_t next = *capacity ? *capacity : 64;
    while(next < needed) {
        if(next > SIZE_MAX / 2) return false;
        next *= 2;
    }
    if(next > SIZE_MAX / size) return false;
    void *grown = realloc(*data,next * size);
    if(grown == NULL) return false;
    *data = grown; *capacity = next;
    return true;
}
NodeLoopWorkspace *node_loop_workspace_create(void) { return calloc(1,sizeof(NodeLoopWorkspace)); }
void node_loop_fill_destroy(NodeLoopFill *fill) {
    if(fill == NULL) return;
    free(fill->vertices); *fill = (NodeLoopFill){0};
}
void node_loop_workspace_destroy(NodeLoopWorkspace *w) {
    if(w == NULL) return;
    free(w->segments); free(w->levels); free(w->crossings);
    node_loop_fill_destroy(&w->staging); free(w);
}
static Point subtract(Point a, Point b) { return (Point){a.x-b.x,a.y-b.y}; }
static double cross(Point a, Point b) { return a.x*b.y-a.y*b.x; }
static Point interpolate(Point a, Point b, double t) {
    return (Point){a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t};
}
static bool near_point(Point a, Point b, Point tolerance) {
    return fabs(a.x-b.x) <= tolerance.x && fabs(a.y-b.y) <= tolerance.y;
}
static bool parallel(Point a, Point b) {
    return fabs(cross(a,b)) <= 16*DBL_EPSILON*(fabs(a.x*b.y)+fabs(a.y*b.x));
}
static int double_compare(const void *a, const void *b) {
    double x=*(const double *)a,y=*(const double *)b;
    return (x>y)-(x<y);
}
static int crossing_compare(const void *a, const void *b) {
    const Crossing *x=a,*y=b;
    return x->x < y->x ? -1 : x->x > y->x ? 1 :
        (x->segment > y->segment)-(x->segment < y->segment);
}
static bool intersection(Point a, Point b, Point c, Point d, double *t, double *u) {
    Point r=subtract(b,a),s=subtract(d,c),q=subtract(c,a);
    if(parallel(r,s)) return false;
    double determinant=cross(r,s);
    *t=cross(q,s)/determinant; *u=cross(q,r)/determinant;
    const double e=32*DBL_EPSILON;
    return *t >= -e && *t <= 1+e && *u >= -e && *u <= 1+e;
}
static size_t vertex_bucket(int64_t x, int64_t y) {
    uint64_t hash=(uint64_t)x*UINT64_C(0x9e3779b185ebca87) ^
        (uint64_t)y*UINT64_C(0xc2b2ae3d27d4eb4f);
    hash ^= hash>>33;
    return (size_t)(hash & (VERTEX_BUCKETS-1));
}
static int vertex_get(NodeLoopWorkspace *w, Point p) {
    /* Centered coordinates divided by a range-relative cell size stay well
     * inside int64_t, including at tiny scales. */
    int64_t x=w->tolerance.x>0 ? (int64_t)floor(p.x/w->tolerance.x) : 0;
    int64_t y=w->tolerance.y>0 ? (int64_t)floor(p.y/w->tolerance.y) : 0;
    for(int dx=-1;dx<=1;dx+=1) for(int dy=-1;dy<=1;dy+=1)
        for(int i=w->vertex_buckets[vertex_bucket(x+dx,y+dy)];i>=0;i=w->vertices[i].hash_next)
            if(near_point(p,w->vertices[i].p,w->tolerance)) return i;
    if(w->vertex_count == GRAPH_LIMIT) return -1;
    int index=(int)w->vertex_count++;
    size_t bucket=vertex_bucket(x,y);
    w->vertices[index]=(Vertex){.p=p,.head=-1,.hash_next=w->vertex_buckets[bucket]};
    w->vertex_buckets[bucket]=index;
    return index;
}
static bool edge_add(NodeLoopWorkspace *w, Point a, Point b) {
    int from=vertex_get(w,a),to=vertex_get(w,b);
    if(from < 0 || to < 0) return false;
    if(from == to) return true;
    for(int h=w->vertices[from].head;h>=0;h=w->edges[h].next_at_vertex)
        if(w->edges[h].to == to) return true;
    if(w->edge_count+2 > GRAPH_LIMIT*2) return false;
    for(int direction=0;direction<2;direction+=1) {
        int u=direction ? to : from,v=direction ? from : to;
        Point delta=subtract(w->vertices[v].p,w->vertices[u].p);
        int h=(int)w->edge_count++;
        w->edges[h]=(HalfEdge){.from=u,.to=v,.next_at_vertex=w->vertices[u].head,
            .face=-1,.angle=atan2(delta.y,delta.x)};
        w->vertices[u].head=h;
    }
    return true;
}
static bool segment_add(NodeLoopWorkspace *w, Point a, Point b, uint32_t mask) {
    if(near_point(a,b,w->tolerance)) return true;
    for(size_t i=0;i<w->segment_count;i+=1) {
        Segment *s=&w->segments[i];
        if((near_point(a,s->a,w->tolerance) && near_point(b,s->b,w->tolerance)) ||
                (near_point(a,s->b,w->tolerance) && near_point(b,s->a,w->tolerance))) {
            s->loops |= mask; return true;
        }
    }
    if(!reserve((void **)&w->segments,&w->segment_capacity,w->segment_count+1,sizeof(Segment))) return false;
    w->segments[w->segment_count++]=(Segment){a,b,mask}; return true;
}

/* Merge the loop into an undirected planar graph. The clockwise successor of
 * each reversed half edge walks the face on its left. Only edges bordering the
 * unbounded face survive: interior pockets are deliberately filled, regardless
 * of winding or repeated traversals. Bridges border outside on both sides. */
static NodeLoopGeometryStatus silhouette_build(NodeLoopWorkspace *w,
        const NodeLoopPoints *loop, unsigned loop_index) {
    Point points[NODE_LOOP_MAX_POINTS];
    for(size_t i=0;i<loop->count;i+=1)
        points[i]=subtract((Point){loop->points[i].x,loop->points[i].y},w->origin);
    w->vertex_count=0; w->edge_count=0;
    memset(w->vertex_buckets,0xff,sizeof(w->vertex_buckets));
    for(size_t i=0;i<loop->count;i+=1) {
        Point a=points[i],b=points[(i+1)%loop->count],r=subtract(b,a);
        if(near_point(a,b,w->tolerance)) continue;
        double cuts[2*NODE_LOOP_MAX_POINTS+2]={0,1}; size_t count=2;
        for(size_t j=0;j<loop->count;j+=1) {
            if(j == i) continue;
            Point c=points[j],d=points[(j+1)%loop->count]; double t,u;
            if(intersection(a,b,c,d,&t,&u)) cuts[count++]=fmax(0,fmin(1,t));
            else if(parallel(r,subtract(c,a)) && parallel(r,subtract(d,a))) {
                /* Collinear overlap: split at every contained endpoint. */
                for(unsigned end=0;end<2;end+=1) {
                    Point p=end ? d : c;
                    t=fabs(r.x)>=fabs(r.y) ? (p.x-a.x)/r.x : (p.y-a.y)/r.y;
                    if(t>0 && t<1) cuts[count++]=t;
                }
            }
        }
        qsort(cuts,count,sizeof(double),double_compare);
        for(size_t j=1;j<count;j+=1)
            if(!edge_add(w,interpolate(a,b,cuts[j-1]),interpolate(a,b,cuts[j])))
                return NODE_LOOP_GEOMETRY_TOPOLOGY_FAILED;
    }
    for(size_t h=0;h<w->edge_count;h+=1) {
        HalfEdge *edge=&w->edges[h]; double reverse=w->edges[h^1].angle;
        int below=-1,largest=-1;
        for(int next=w->vertices[edge->to].head;next>=0;next=w->edges[next].next_at_vertex) {
            double angle=w->edges[next].angle;
            if(largest<0 || angle>w->edges[largest].angle) largest=next;
            if(angle<reverse && (below<0 || angle>w->edges[below].angle)) below=next;
        }
        edge->next=below<0 ? largest : below;
    }
    int faces=0;
    for(size_t h=0;h<w->edge_count;h+=1) {
        if(w->edges[h].face>=0) continue;
        size_t next=h,steps=0; double area=0,magnitude=0;
        Point anchor=w->vertices[w->edges[h].from].p;
        do {
            if(next>=w->edge_count || w->edges[next].face>=0 || steps++>=w->edge_count)
                return NODE_LOOP_GEOMETRY_TOPOLOGY_FAILED;
            HalfEdge *edge=&w->edges[next]; edge->face=faces;
            Point a=subtract(w->vertices[edge->from].p,anchor),b=subtract(w->vertices[edge->to].p,anchor);
            area+=cross(a,b); magnitude+=fabs(a.x*b.y)+fabs(a.y*b.x);
            next=(size_t)edge->next;
        } while(next!=h);
        double error=32*DBL_EPSILON*magnitude;
        w->exterior[faces++]=area<=error;
        if(area>error) w->staging.enclosed_loops |= UINT32_C(1)<<loop_index;
    }
    for(size_t h=0;h<w->edge_count;h+=2) {
        if(w->exterior[w->edges[h].face] == w->exterior[w->edges[h+1].face]) continue;
        if(!segment_add(w,w->vertices[w->edges[h].from].p,w->vertices[w->edges[h].to].p,
                UINT32_C(1)<<loop_index)) return NODE_LOOP_GEOMETRY_ALLOCATION_FAILED;
    }
    return NODE_LOOP_GEOMETRY_OK;
}
static bool level_add(NodeLoopWorkspace *w,double y) {
    if(!reserve((void **)&w->levels,&w->level_capacity,w->level_count+1,sizeof(double))) return false;
    w->levels[w->level_count++]=y; return true;
}
static double segment_x(const Segment *s,double y) {
    return s->a.x+(s->b.x-s->a.x)*((y-s->a.y)/(s->b.y-s->a.y));
}
static bool triangle_add(NodeLoopWorkspace *w,Point a,Point b,Point c) {
    if(parallel(subtract(b,a),subtract(c,a))) return true;
    NodeLoopFill *fill=&w->staging;
    if(!reserve((void **)&fill->vertices,&fill->capacity,fill->vertex_count+3,sizeof(Vec2D))) return false;
    Point points[3]={a,b,c};
    for(unsigned i=0;i<3;i+=1) fill->vertices[fill->vertex_count++]=
        (Vec2D){(float)(points[i].x+w->origin.x),(float)(points[i].y+w->origin.y)};
    return true;
}
NodeLoopGeometryStatus node_loop_fill_build(const NodeLoopPoints *loops,
        size_t count,NodeLoopWorkspace *w,NodeLoopFill *output) {
    if(loops==NULL || w==NULL || output==NULL) return NODE_LOOP_GEOMETRY_INVALID_ARGUMENT;
    if(count<1 || count>NODE_LOOP_MAX_LOOPS) return NODE_LOOP_GEOMETRY_INVALID_COUNT;
    w->origin=(Point){loops[0].points[0].x,loops[0].points[0].y};
    Point scale={0};
    for(size_t l=0;l<count;l+=1) {
        if(loops[l].count<3 || loops[l].count>NODE_LOOP_MAX_POINTS) return NODE_LOOP_GEOMETRY_INVALID_COUNT;
        for(size_t i=0;i<loops[l].count;i+=1) {
            Vec2D p=loops[l].points[i];
            if(!isfinite(p.x) || !isfinite(p.y)) return NODE_LOOP_GEOMETRY_NONFINITE;
            scale.x=fmax(scale.x,fabs((double)p.x-w->origin.x));
            scale.y=fmax(scale.y,fabs((double)p.y-w->origin.y));
        }
    }
    w->tolerance=(Point){64*DBL_EPSILON*scale.x,64*DBL_EPSILON*scale.y};
    w->segment_count=0; w->level_count=0;
    w->staging.vertex_count=0; w->staging.enclosed_loops=0;
    for(size_t l=0;l<count;l+=1) {
        NodeLoopGeometryStatus status=silhouette_build(w,&loops[l],(unsigned)l);
        if(status!=NODE_LOOP_GEOMETRY_OK) return status;
    }
    /* Horizontal slabs end at every vertex and crossing. Within a slab the
     * left-to-right ordering is constant, so parity of each loop silhouette
     * yields nonoverlapping trapezoids for outer minus the union of holes. */
    for(size_t i=0;i<w->segment_count;i+=1) {
        Segment *s=&w->segments[i];
        if(!level_add(w,s->a.y) || !level_add(w,s->b.y)) return NODE_LOOP_GEOMETRY_ALLOCATION_FAILED;
        for(size_t j=0;j<i;j+=1) {
            double t,u; Segment *other=&w->segments[j];
            if(intersection(s->a,s->b,other->a,other->b,&t,&u) && t>0 && t<1 && u>0 && u<1)
                if(!level_add(w,interpolate(s->a,s->b,t).y)) return NODE_LOOP_GEOMETRY_ALLOCATION_FAILED;
        }
    }
    if(w->level_count>1) qsort(w->levels,w->level_count,sizeof(double),double_compare);
    size_t unique=0;
    for(size_t i=0;i<w->level_count;i+=1)
        if(unique==0 || w->levels[i]-w->levels[unique-1]>w->tolerance.y) w->levels[unique++]=w->levels[i];
    if(!reserve((void **)&w->crossings,&w->crossing_capacity,w->segment_count,sizeof(Crossing)))
        return NODE_LOOP_GEOMETRY_ALLOCATION_FAILED;
    for(size_t level=1;level<unique;level+=1) {
        double bottom=w->levels[level-1],top=w->levels[level],mid=bottom+(top-bottom)*0.5;
        size_t crossings=0;
        for(size_t i=0;i<w->segment_count;i+=1) {
            Segment *s=&w->segments[i];
            if(mid>fmin(s->a.y,s->b.y) && mid<fmax(s->a.y,s->b.y))
                w->crossings[crossings++]=(Crossing){segment_x(s,mid),i};
        }
        if(crossings>1) qsort(w->crossings,crossings,sizeof(Crossing),crossing_compare);
        uint32_t inside=0; const Segment *left=NULL;
        for(size_t i=0;i<crossings;) {
            size_t end=i+1; uint32_t mask=w->segments[w->crossings[i].segment].loops;
            while(end<crossings && w->crossings[end].x-w->crossings[i].x<=w->tolerance.x) {
                mask ^= w->segments[w->crossings[end].segment].loops; end+=1;
            }
            bool before=inside==1; inside ^= mask; bool after=inside==1;
            const Segment *boundary=&w->segments[w->crossings[i].segment];
            if(!before && after) left=boundary;
            if(before && !after && left!=NULL) {
                Point a={segment_x(left,bottom),bottom},b={segment_x(boundary,bottom),bottom};
                Point c={segment_x(boundary,top),top},d={segment_x(left,top),top};
                if(!triangle_add(w,a,b,c) || !triangle_add(w,a,c,d)) return NODE_LOOP_GEOMETRY_ALLOCATION_FAILED;
            }
            i=end;
        }
        if(inside!=0) return NODE_LOOP_GEOMETRY_TOPOLOGY_FAILED;
    }
    NodeLoopFill old=*output; *output=w->staging; w->staging=old;
    return NODE_LOOP_GEOMETRY_OK;
}
