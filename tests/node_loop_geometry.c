/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "math/node_loop_geometry.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr,"loop line %d: %s\n",__LINE__,#c); return false; } } while(0)
static double cross(Vec2D a,Vec2D b,Vec2D c) {
    return ((double)b.x-a.x)*((double)c.y-a.y)-((double)b.y-a.y)*((double)c.x-a.x);
}
static double area_get(const NodeLoopFill *fill) {
    double area=0;
    for(size_t i=0;i<fill->vertex_count;i+=3)
        area+=fabs(cross(fill->vertices[i],fill->vertices[i+1],fill->vertices[i+2]))/2;
    return area;
}
static bool contains(const NodeLoopFill *fill,float x,float y) {
    Vec2D p={x,y};
    for(size_t i=0;i<fill->vertex_count;i+=3) {
        double a=cross(fill->vertices[i],fill->vertices[i+1],p);
        double b=cross(fill->vertices[i+1],fill->vertices[i+2],p);
        double c=cross(fill->vertices[i+2],fill->vertices[i],p);
        if((a>=0 && b>=0 && c>=0) || (a<=0 && b<=0 && c<=0)) return true;
    }
    return false;
}
static NodeLoopPoints square(float x,float y,float half) {
    return (NodeLoopPoints){.points={{x-half,y-half},{x+half,y-half},{x+half,y+half},{x-half,y+half}},.count=4};
}
static bool cases_check(NodeLoopWorkspace *w,NodeLoopFill *fill) {
    NodeLoopPoints loops[NODE_LOOP_MAX_LOOPS]={square(0,0,10)};
    CHECK(node_loop_fill_build(loops,1,w,fill)==NODE_LOOP_GEOMETRY_OK);
    CHECK(fabs(area_get(fill)-400)<0.001 && fill->enclosed_loops==1);
    /* Analytic hourglass: two 100-unit lobes, no left/right spill. */
    loops[0].points[0].x=10; loops[0].points[1].x=-10;
    CHECK(node_loop_fill_build(loops,1,w,fill)==NODE_LOOP_GEOMETRY_OK);
    CHECK(fabs(area_get(fill)-200)<0.001);
    CHECK(contains(fill,0,8) && contains(fill,0,-8));
    CHECK(!contains(fill,-8,0) && !contains(fill,8,0));
    /* Same square traced twice in either direction never erases itself. */
    loops[0]=square(0,0,10); loops[0].count=8;
    for(int i=0;i<4;i+=1) loops[0].points[i+4]=loops[0].points[i];
    CHECK(node_loop_fill_build(loops,1,w,fill)==NODE_LOOP_GEOMETRY_OK);
    CHECK(fabs(area_get(fill)-400)<0.001);
    for(int i=0;i<4;i+=1) loops[0].points[i+4]=loops[0].points[(4-i)%4];
    CHECK(node_loop_fill_build(loops,1,w,fill)==NODE_LOOP_GEOMETRY_OK);
    CHECK(fabs(area_get(fill)-400)<0.001);
    /* Nested connected contour: bounded pockets inside one loop stay filled. */
    loops[0]=(NodeLoopPoints){.points={{-10,-10},{10,-10},{10,10},{-10,10},{-10,-10},
        {-3,-3},{3,-3},{3,3},{-3,3},{-3,-3}},.count=10};
    CHECK(node_loop_fill_build(loops,1,w,fill)==NODE_LOOP_GEOMETRY_OK);
    CHECK(fabs(area_get(fill)-400)<0.001);
    loops[0]=square(0,0,10); loops[1]=square(0,0,4);
    CHECK(node_loop_fill_build(loops,2,w,fill)==NODE_LOOP_GEOMETRY_OK);
    CHECK(fabs(area_get(fill)-336)<0.001 && !contains(fill,0,0) && contains(fill,8,0));
    loops[2]=square(3,0,4);
    CHECK(node_loop_fill_build(loops,3,w,fill)==NODE_LOOP_GEOMETRY_OK);
    CHECK(fabs(area_get(fill)-312)<0.001);
    NodeLoopPoints tmp=loops[1]; loops[1]=loops[2]; loops[2]=tmp;
    CHECK(node_loop_fill_build(loops,3,w,fill)==NODE_LOOP_GEOMETRY_OK && fabs(area_get(fill)-312)<0.001);
    loops[1]=square(10,0,4);
    CHECK(node_loop_fill_build(loops,2,w,fill)==NODE_LOOP_GEOMETRY_OK && fabs(area_get(fill)-368)<0.001);
    loops[1]=square(20,0,4);
    CHECK(node_loop_fill_build(loops,2,w,fill)==NODE_LOOP_GEOMETRY_OK && fabs(area_get(fill)-400)<0.001);
    loops[1]=square(0,0,10);
    CHECK(node_loop_fill_build(loops,2,w,fill)==NODE_LOOP_GEOMETRY_OK && fill->vertex_count==0 && fill->enclosed_loops==3);
    /* Touching edges, overlapping collinear runs and dangling excursions. */
    loops[0]=(NodeLoopPoints){.points={{0,0},{10,0},{10,10},{0,10},{0,0},{5,0},{5,-5},{5,0}},.count=8};
    CHECK(node_loop_fill_build(loops,1,w,fill)==NODE_LOOP_GEOMETRY_OK && fabs(area_get(fill)-100)<0.001);
    for(int i=0;i<8;i+=1) loops[0].points[i].y=0;
    CHECK(node_loop_fill_build(loops,1,w,fill)==NODE_LOOP_GEOMETRY_OK && fill->vertex_count==0 && fill->enclosed_loops==0);
    /* Finite-precision tests across scale/translation and reversed winding. */
    const float scales[]={0.00001f,1,10000};
    for(unsigned k=0;k<3;k+=1) {
        float s=scales[k],offset=17*s;
        loops[0]=square(offset,offset,10*s); loops[1]=square(offset,offset,4*s);
        Vec2D swap=loops[1].points[0]; loops[1].points[0]=loops[1].points[2]; loops[1].points[2]=swap;
        CHECK(node_loop_fill_build(loops,2,w,fill)==NODE_LOOP_GEOMETRY_OK);
        CHECK(fabs(area_get(fill)/(s*s)-336)<0.01);
    }
    NodeLoopFill prior=*fill;
    loops[0].points[0].x=NAN;
    CHECK(node_loop_fill_build(loops,1,w,fill)==NODE_LOOP_GEOMETRY_NONFINITE);
    CHECK(!memcmp(&prior,fill,sizeof(prior)));
    CHECK(node_loop_fill_build(loops,18,w,fill)==NODE_LOOP_GEOMETRY_INVALID_COUNT);
    CHECK(node_loop_fill_build(NULL,1,w,fill)==NODE_LOOP_GEOMETRY_INVALID_ARGUMENT);
    return true;
}
static bool motion_check(NodeLoopWorkspace *w,NodeLoopFill *fill) {
    /* Sweep the hourglass through collapse. Its analytic slice width is known. */
    for(unsigned frame=0;frame<=200;frame+=1) {
        double t=(double)frame/200;
        NodeLoopPoints loop=square(0,0,10);
        loop.points[0].x=(float)(-10+20*t); loop.points[1].x=-loop.points[0].x;
        CHECK(node_loop_fill_build(&loop,1,w,fill)==NODE_LOOP_GEOMETRY_OK);
        for(int y=-9;y<=9;y+=3) for(int x=-9;x<=9;x+=3) {
            double fraction=(y+10)/20.0;
            double width=fabs(loop.points[1].x*(1-fraction)+10*fraction);
            if(fabs(fabs((double)x)-width)>0.001) CHECK(contains(fill,(float)x,(float)y)==(fabs((double)x)<width));
        }
    }
    return true;
}
static bool tangled_check(NodeLoopWorkspace *w,NodeLoopFill *fill) {
    uint32_t seed=271828;
    for(unsigned test=0;test<120;test+=1) {
        NodeLoopPoints loops[2]={0};
        loops[0].count=3+test%62;
        for(unsigned i=0;i<loops[0].count;i+=1) {
            seed=seed*1664525u+1013904223u; float x=(float)(seed%20001)/100-100;
            seed=seed*1664525u+1013904223u; float y=(float)(seed%20001)/100-100;
            loops[0].points[i]=(Vec2D){x,y};
        }
        CHECK(node_loop_fill_build(loops,1,w,fill)==NODE_LOOP_GEOMETRY_OK);
        double area=area_get(fill); CHECK(area>=0 && area<=40000.001);
        loops[1].count=loops[0].count;
        for(size_t i=0;i<loops[0].count;i+=1) loops[1].points[i]=loops[0].points[loops[0].count-1-i];
        CHECK(node_loop_fill_build(&loops[1],1,w,fill)==NODE_LOOP_GEOMETRY_OK);
        CHECK(fabs(area_get(fill)-area)<0.001);
        CHECK(node_loop_fill_build(loops,2,w,fill)==NODE_LOOP_GEOMETRY_OK);
        CHECK(area_get(fill)<0.001);
    }
    return true;
}
static bool benchmark(NodeLoopWorkspace *w,NodeLoopFill *fill) {
    NodeLoopPoints loops[NODE_LOOP_MAX_LOOPS]={0};
    for(unsigned l=0;l<NODE_LOOP_MAX_LOOPS;l+=1) {
        loops[l].count=64;
        for(unsigned i=0;i<64;i+=1) {
            unsigned step=2*l+1;
            double angle=(double)((i*step)%64)*6.283185307179586/64;
            loops[l].points[i]=(Vec2D){(float)(100*cos(angle)),(float)(100*sin(angle))};
        }
    }
    clock_t start=clock();
    for(unsigned i=0;i<3;i+=1) CHECK(node_loop_fill_build(loops,17,w,fill)==NODE_LOOP_GEOMETRY_OK);
    printf("17 loops x 64 shared nodes, tangled: %.3f ms/build; %zu triangles\n",
        1000.0*(clock()-start)/CLOCKS_PER_SEC/3,fill->vertex_count/3);
    loops[0]=square(0,0,10); loops[1]=square(0,0,4);
    start=clock();
    for(unsigned i=0;i<1000;i+=1) CHECK(node_loop_fill_build(loops,2,w,fill)==NODE_LOOP_GEOMETRY_OK);
    printf("square with window: %.3f ms/build\n",1000.0*(clock()-start)/CLOCKS_PER_SEC/1000);
    return true;
}
int main(void) {
    NodeLoopWorkspace *workspace=node_loop_workspace_create();
    NodeLoopFill fill={0};
    bool passed=workspace!=NULL && cases_check(workspace,&fill) && motion_check(workspace,&fill) && tangled_check(workspace,&fill) && benchmark(workspace,&fill);
    node_loop_fill_destroy(&fill); node_loop_workspace_destroy(workspace);
    return passed ? 0 : 1;
}
