/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "physics/soft_body/soft_body_mass.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define OK(value) assert(!rohr_error_check(value))
static void near(double actual, double expected, double tolerance) {
    if(fabs(actual - expected) > tolerance)
        fprintf(stderr, "Expected %.12g, got %.12g\n", expected, actual);
    assert(fabs(actual - expected) <= tolerance);
}
static void distribution_check(Position *points, Mass *prior, uint32_t count,
        Position target) {
    Mass output[SOFT_BODY_MAX_NODES];
    bool success = physics_soft_mass_distribution_get(points, prior, count, target, output);
    if(!success) fprintf(stderr, "Redistribution failed for (%g, %g)\n", target.x, target.y);
    assert(success);
    double before = 0, total = 0, x = 0, y = 0;
    for(uint32_t i = 0; i < count; i++) {
        assert(output[i] > 0 && isfinite(output[i]));
        before += prior[i]; total += output[i];
        x += output[i] * (double)points[i].x;
        y += output[i] * (double)points[i].y;
    }
    near(total, before, before * 1e-6);
    near(x / total, target.x, 2e-5);
    near(y / total, target.y, 2e-5);
}
static void kernel_tests(void) {
    Position square[] = {{-1,-1},{1,-1},{1,1},{-1,1}};
    Mass equal[] = {1,1,1,1}, custom[] = {1,2,4,3};
    Mass output[4] = {31,32,33,34}, preserved[4];
    for(int i = -9; i <= 9; i++) for(int j = -9; j <= 9; j++)
        distribution_check(square, custom, 4, (Position){i / 10.0f, j / 10.0f});
    distribution_check(square, custom, 4, (Position){.9999f,-.9999f});
    assert(physics_soft_mass_distribution_get(square, equal, 4, (Position){.5f,0}, output));
    near(output[0], .5, 1e-6); near(output[1], 1.5, 1e-6);
    near(output[2], 1.5, 1e-6); near(output[3], .5, 1e-6);
    memcpy(preserved, output, sizeof(output));
    Position invalid[] = {{1,0},{2,0},{NAN,0},{0,INFINITY}};
    for(size_t i = 0; i < sizeof(invalid)/sizeof(*invalid); i++) {
        assert(!physics_soft_mass_distribution_get(square, equal, 4, invalid[i], output));
        assert(memcmp(output, preserved, sizeof(output)) == 0);
    }
    Position line[] = {{-2,0},{0,0},{2,0},{2,0}};
    distribution_check(line, custom, 4, (Position){1,0});
    assert(!physics_soft_mass_distribution_get(line, custom, 4, (Position){1,.1f}, output));
    assert(!physics_soft_mass_distribution_get(line, custom, 4, (Position){2,0}, output));
    assert(physics_soft_mass_distribution_get(line, custom, 4, (Position){1,0}, output));
    near(output[2]/output[3], 4.0/3, 1e-6);
    Position same[] = {{5,6},{5,6},{5,6},{5,6}};
    assert(physics_soft_mass_distribution_get(same, custom, 4, (Position){5,6}, output));
    assert(memcmp(output, custom, sizeof(output)) == 0);
    assert(!physics_soft_mass_distribution_get(same, custom, 4, (Position){5,7}, output));
    assert(physics_soft_mass_scale_get(custom, 4, 20, output));
    for(int i=0;i<4;i++) near(output[i], custom[i]*2, 1e-6);
    Mass zero[] = {1,0,1,1};
    assert(!physics_soft_mass_scale_get(zero, 4, 20, output));
    assert(!physics_soft_mass_distribution_get(square, zero, 4, (Position){0,0}, output));
    Mass extremes[] = {FLT_MIN, FLT_MAX/2};
    assert(!physics_soft_mass_scale_get(extremes, 2, FLT_MIN, output));
    assert(!physics_soft_mass_scale_get(custom, 4, INFINITY, output));
    assert(!physics_soft_mass_scale_get(custom, 0, 1, output));
    Position triangle[] = {{0,0},{4,0},{0,4}};
    distribution_check(triangle, custom, 3, (Position){1,2});
    assert(!physics_soft_mass_distribution_get(triangle, custom, 3, (Position){3,3}, output));
    /* The entropy minimizer on a rectangle preserves the cross-product ratio. */
    assert(physics_soft_mass_distribution_get(square, custom, 4, (Position){.3f,-.4f}, output));
    near((double)output[0]*output[2]/(output[1]*output[3]), 4.0/6, 1e-6);
    /* Translation and scale do not change the mass distribution. */
    Mass reference[4]; memcpy(reference, output, sizeof(reference));
    for(int i=0;i<4;i++) {
        square[i].x = square[i].x*100 + 2000;
        square[i].y = square[i].y*100 - 3000;
    }
    assert(physics_soft_mass_distribution_get(square, custom, 4, (Position){2030,-3040}, output));
    for(int i=0;i<4;i++) near(output[i], reference[i], 1e-6);
    Position cloud[SOFT_BODY_MAX_NODES];
    Mass weights[SOFT_BODY_MAX_NODES], solved[SOFT_BODY_MAX_NODES];
    double expected[SOFT_BODY_MAX_NODES], sum = 0, total = 0, cx = 0, cy = 0;
    for(uint32_t i=0;i<SOFT_BODY_MAX_NODES;i++) {
        double angle = i * 6.283185307179586 / SOFT_BODY_MAX_NODES;
        cloud[i] = (Position){(float)(2*cos(angle)), (float)(3*sin(angle))};
        weights[i] = 1 + i % 5;
        expected[i] = weights[i]*exp(.2*cloud[i].x-.3*cloud[i].y);
        total += weights[i]; sum += expected[i];
        cx += expected[i]*cloud[i].x; cy += expected[i]*cloud[i].y;
    }
    assert(physics_soft_mass_distribution_get(cloud, weights, SOFT_BODY_MAX_NODES,
        (Position){(float)(cx/sum),(float)(cy/sum)}, solved));
    for(uint32_t i=0;i<SOFT_BODY_MAX_NODES;i++) near(solved[i], total*expected[i]/sum, 2e-5);
    Position diagonal[] = {{-2,-2},{0,0},{2,2},{2,2}};
    distribution_check(diagonal, custom, 4, (Position){1,1});
    Position corners[] = {{-1,-1},{1,-1},{1,1},{-1,1}};
    Mass skewed[] = {1e-8f,1e8f,1e-8f,1e8f};
    distribution_check(corners, skewed, 4, (Position){.2f,.3f});
    /* Preserving these stronger preferences would require a positive mass
     * below the float range. Reject rather than silently making a node zero. */
    Mass unrepresentable[] = {1e-20f,1e20f,1e-20f,1e20f};
    memcpy(preserved, output, sizeof(output));
    assert(!physics_soft_mass_distribution_get(corners, unrepresentable, 4,
        (Position){.2f,.3f}, output));
    assert(memcmp(preserved, output, sizeof(output)) == 0);
    assert(!physics_soft_mass_distribution_get(cloud, weights, SOFT_BODY_MAX_NODES+1,
        (Position){0}, solved));
}

static void api_tests(void) {
    OK(rohr_engine_start());
    EntityResult ordinary = rohr_entity_add(); OK(ordinary);
    assert(rohr_error_check(rohr_physics_mass_get(ordinary.result.value)));
    OK(rohr_physics_mass_set(ordinary.result.value, 3));
    near(rohr_physics_mass_get(ordinary.result.value).result.value, 3, 0);
    EntityResult body = rohr_physics_soft_body_create(); OK(body);
    near(rohr_physics_mass_get(body.result.value).result.value, 0, 0);
    assert(rohr_error_check(rohr_physics_center_of_mass_local_position_get(body.result.value)));
    assert(rohr_error_check(rohr_physics_mass_set(body.result.value, 10)));
    assert(rohr_error_check(rohr_physics_center_of_mass_local_position_set(body.result.value, (Position){0})));
    OK(rohr_physics_center_of_mass_automatic_set(body.result.value));
    Position points[] = {{-1,-1},{1,-1},{1,1},{-1,1}};
    Entity nodes[4];
    for(int i=0;i<4;i++) {
        EntityResult node = rohr_physics_soft_body_node_create(body.result.value, points[i], 1, .1f);
        OK(node); nodes[i] = node.result.value;
        OK(rohr_physics_velocity_set(nodes[i], (Velocity){2,3}));
    }
    OK(rohr_physics_center_of_mass_local_position_set(body.result.value, (Position){.5f,0}));
    near(rohr_physics_mass_get(body.result.value).result.value, 4, 1e-6);
    OK(rohr_physics_mass_set(body.result.value, 20));
    for(int i=0;i<4;i++) {
        near(rohr_physics_mass_get(nodes[i]).result.value, (i==1 || i==2) ? 7.5 : 2.5, 1e-5);
        EntityIndex index = rohr_entity_index_get(nodes[i]).result.value;
        near(positions[index].x, points[i].x, 0); near(positions[index].y, points[i].y, 0);
        near(velocities[index].x, 2, 0); near(velocities[index].y, 3, 0);
    }
    assert(!rohr_entity_components_check(body.result.value, ROHR_MASS));
    assert(rohr_physics_center_of_mass_automatic_check(body.result.value).result.value);
    OK(rohr_physics_center_of_mass_automatic_set(body.result.value));
    near(rohr_physics_center_of_mass_local_position_get(body.result.value).result.value.x, .5, 1e-6);
    OK(rohr_physics_position_set(body.result.value, (Position){10,20}));
    OK(rohr_physics_orientation_set(body.result.value, 90));
    PositionResult world = rohr_physics_center_of_mass_world_position_get(body.result.value); OK(world);
    near(world.result.value.x, 10, 1e-5); near(world.result.value.y, 19.5, 1e-5);
    OK(rohr_physics_center_of_mass_local_position_set(body.result.value, (Position){0,.5f}));
    near(rohr_physics_center_of_mass_world_position_get(body.result.value).result.value.x, 10.5, 1e-5);
    Mass before[4]; for(int i=0;i<4;i++) before[i]=rohr_physics_mass_get(nodes[i]).result.value;
    assert(rohr_error_check(rohr_physics_center_of_mass_local_position_set(body.result.value, (Position){5,0})));
    assert(rohr_error_check(rohr_physics_mass_set(body.result.value, 0)));
    assert(rohr_error_check(rohr_physics_mass_set(body.result.value, NAN)));
    for(int i=0;i<4;i++) near(rohr_physics_mass_get(nodes[i]).result.value, before[i], 0);
    OK(rohr_physics_mass_set(nodes[0], before[0]+1));
    near(rohr_physics_mass_get(body.result.value).result.value, 21, 1e-5);
    OK(rohr_physics_soft_body_node_local_position_set(nodes[0], (Position){-2,-2}));
    double x=0,y=0,total=0,vx=0,vy=0;
    for(int i=0;i<4;i++) {
        Position p=rohr_physics_position_get(nodes[i]).result.value;
        double m=rohr_physics_mass_get(nodes[i]).result.value;
        total+=m;x+=m*p.x;y+=m*p.y;
        EntityIndex index=rohr_entity_index_get(nodes[i]).result.value;
        vx+=m*velocities[index].x;vy+=m*velocities[index].y;
    }
    world=rohr_physics_center_of_mass_world_position_get(body.result.value); OK(world);
    near(world.result.value.x,x/total,1e-5);near(world.result.value.y,y/total,1e-5);
    assert((rohr_system_physics_update(.1)).header.status != ROHR_REPORT_FAILED);
    world=rohr_physics_center_of_mass_world_position_get(body.result.value); OK(world);
    near(world.result.value.x,(x+.1*vx)/total,1e-5);
    near(world.result.value.y,(y+.1*vy)/total,1e-5);
    float first_mass=rohr_physics_mass_get(nodes[0]).result.value;
    OK(rohr_physics_mass_set(nodes[0],0));
    assert(rohr_error_check(rohr_physics_mass_set(body.result.value,10)));
    near(rohr_physics_mass_get(nodes[0]).result.value,0,0);
    OK(rohr_physics_mass_set(nodes[0],first_mass));
    assert(rohr_error_check(rohr_physics_mass_set(body.result.value,nextafterf(0.0f, 1.0f))));
    OK(rohr_entity_delete(nodes[3]));
    near(rohr_physics_mass_get(body.result.value).result.value, total-before[3], 1e-5);
    OK(rohr_entity_delete(body.result.value));
    assert(rohr_error_check(rohr_physics_mass_get(body.result.value)));
    assert(rohr_error_check(rohr_physics_center_of_mass_local_position_set(body.result.value,(Position){0})));
    rohr_engine_stop();
}
int main(void) { kernel_tests(); api_tests(); return 0; }
