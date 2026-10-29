/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"beam pair check failed: %s:%d: %s\n",__FILE__,__LINE__,#x);exit(1);}}while(0)
#define OK(x) CHECK(!rohr_error_check(x))
static Entity node(Entity owner,float x,float y,float vy,float m) {
    EntityResult n=rohr_physics_soft_body_node_add((SoftBodyNodeDescription){.owner=owner,.position={x,y},.mass=m,.radius=.1});OK(n);
    OK(rohr_physics_velocity_set(n.result.value,((Velocity){0,vy})));return n.result.value;
}
static Entity beam(Entity owner,Entity a,Entity b,float restitution,unsigned filter) {
    EntityResult r=rohr_physics_soft_body_beam_add((SoftBodyBeamDescription){.owner=owner,.node_a=a,.node_b=b,
        .restitution=restitution,.collision={.enabled=true,.thickness=2,.filter={.category=filter,.responds_to=filter}}});OK(r);return r.result.value;
}
static Velocity velocity(Entity e){EntityIndexResult i=rohr_entity_index_get(e);OK(i);return velocities[i.result.value];}
static void impact(bool reverse,float restitution,bool filtering,bool fixed) {
    OK(rohr_engine_start());OK(rohr_physics_solver_iterations_set(16));
    EntityResult o=rohr_physics_soft_body_add((SoftBodyDescription){0});OK(o);
    Entity n[4]={node(o.result.value,-10,0,0,1),node(o.result.value,10,0,0,3),
        node(o.result.value,-10,1.9,-10,2),node(o.result.value,10,1.9,-10,4)};
    if(fixed){OK(rohr_physics_static_set(n[0]));OK(rohr_physics_static_set(n[1]));}
    Entity a=beam(o.result.value,n[reverse?1:0],n[reverse?0:1],restitution,filtering?0:1);
    Entity b=beam(o.result.value,n[reverse?3:2],n[reverse?2:3],1,filtering?0:1);
    Report r=rohr_system_physics_update(0);CHECK(!r.has_fatal&&!r.diagnostic_count);
    CHECK(rohr_physics_overlap_check(a,b));CHECK(rohr_physics_overlap_check(b,a));
    CHECK(rohr_physics_contact_check(a,b)==!filtering);
    double relative=.5*(velocity(n[2]).y+velocity(n[3]).y-velocity(n[0]).y-velocity(n[1]).y);
    CHECK(fabs(relative-(filtering?-10:10*restitution))<(restitution?.05:.0001));
    if(!fixed)CHECK(fabs(velocity(n[0]).y+3*velocity(n[1]).y+2*velocity(n[2]).y+4*velocity(n[3]).y+60)<.0001);
    if(!filtering){ContactInfo ab=rohr_physics_contact_get(a,b),ba=rohr_physics_contact_get(b,a);CHECK(ab.detected&&ba.detected);CHECK(fabs(ab.normal.x+ba.normal.x)<1e-6&&fabs(ab.normal.y+ba.normal.y)<1e-6);}
    rohr_engine_stop();
}
static void connected(void) {
    OK(rohr_engine_start());EntityResult o=rohr_physics_soft_body_add((SoftBodyDescription){0});OK(o);
    Entity n[3]={node(o.result.value,0,0,0,1),node(o.result.value,10,0,0,1),node(o.result.value,10,1,0,1)};
    Entity a=beam(o.result.value,n[0],n[1],0,1),b=beam(o.result.value,n[0],n[2],0,1);
    Report r=rohr_system_physics_update(0);CHECK(!r.has_fatal);CHECK(!rohr_physics_contact_check(a,b));rohr_engine_stop();
}
int main(void){for(unsigned rev=0;rev<2;++rev)for(unsigned e=0;e<2;++e)for(unsigned fixed=0;fixed<2;++fixed)impact(rev,.5*e,false,fixed);impact(false,0,true,false);connected();puts("PASS beam pairs: rebound, momentum, reversed endpoints, static support, filtering, connected exclusion");return 0;}
