/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "soft_body_motion.h"
#include "physics/physics_internal.h"
#include <float.h>
#include <math.h>
#include <string.h>

typedef struct SoftMotion {
    double x, y, omega;
} SoftMotion;

static bool motion_get(const Position *points, const Mass *masses,
        const Velocity *values, uint32_t count, SoftMotion *output) {
    double total = 0, x = 0, y = 0, vx = 0, vy = 0;
    double inertia = 0, momentum = 0;
    if(points == NULL || masses == NULL || values == NULL || output == NULL ||
            count == 0 || count > SOFT_BODY_MAX_NODES) return false;
    for(uint32_t i = 0; i < count; i += 1) {
        if(!isfinite(masses[i]) || masses[i] <= 0 ||
                !isfinite(points[i].x) || !isfinite(points[i].y) ||
                !isfinite(values[i].x) || !isfinite(values[i].y)) return false;
        total += masses[i];
        x += masses[i] * ((double)points[i].x - points[0].x);
        y += masses[i] * ((double)points[i].y - points[0].y);
        vx += masses[i] * ((double)values[i].x - values[0].x);
        vy += masses[i] * ((double)values[i].y - values[0].y);
    }
    x /= total; y /= total; vx /= total; vy /= total;
    for(uint32_t i = 0; i < count; i += 1) {
        double rx = ((double)points[i].x - points[0].x) - x;
        double ry = ((double)points[i].y - points[0].y) - y;
        double ux = ((double)values[i].x - values[0].x) - vx;
        double uy = ((double)values[i].y - values[0].y) - vy;
        inertia += masses[i] * (rx * rx + ry * ry);
        momentum += masses[i] * (ry * ux - rx * uy);
    }
    if(!isfinite(inertia) || inertia <= 0 || !isfinite(momentum)) return false;
    output->x = x; output->y = y;
    output->omega = momentum / inertia;
    return isfinite(output->omega);
}

bool physics_soft_angular_velocities_get(const Position *points, const Mass *masses,
        const Velocity *prior, uint32_t count, AngularVelocity degrees, Velocity *output) {
    SoftMotion motion;
    Velocity next[SOFT_BODY_MAX_NODES];
    if(output == NULL || !isfinite(degrees) ||
            !motion_get(points, masses, prior, count, &motion)) return false;
    double delta = (double)degrees * ((double)PI_F / 180.0) - motion.omega;
    for(uint32_t i = 0; i < count; i += 1) {
        double rx = ((double)points[i].x - points[0].x) - motion.x;
        double ry = ((double)points[i].y - points[0].y) - motion.y;
        double x = prior[i].x + delta * ry;
        double y = prior[i].y - delta * rx;
        if(!isfinite(x) || !isfinite(y) || fabs(x) > FLT_MAX || fabs(y) > FLT_MAX)
            return false;
        next[i] = (Velocity){(float)x, (float)y};
    }
    memcpy(output, next, count * sizeof(*output));
    return true;
}

typedef struct SoftNodeMotion {
    EntityIndex indices[SOFT_BODY_MAX_NODES];
    Position points[SOFT_BODY_MAX_NODES];
    Mass masses[SOFT_BODY_MAX_NODES];
    Velocity values[SOFT_BODY_MAX_NODES];
    uint32_t count;
} SoftNodeMotion;

static bool nodes_get(EntityIndex body, SoftNodeMotion *motion) {
    if(body >= soft_bodies_pool.capacity || !soft_bodies_pool.used[body]) return false;
    motion->count = soft_bodies[body].node_count;
    if(motion->count == 0 || motion->count > SOFT_BODY_MAX_NODES) return false;
    for(uint32_t i = 0; i < motion->count; i += 1) {
        EntityIndex node;
        if(!entity_index_get(soft_bodies[body].nodes[i], &node) ||
                !entity_index_alive_check(node) ||
                !entity_index_components_check(node, ROHR_SOFT_BODY_NODE | ROHR_MASS) ||
                node >= positions_pool.capacity || !positions_pool.used[node] ||
                node >= mass_pool.capacity || !mass_pool.used[node] ||
                node >= velocities_pool.capacity || !velocities_pool.used[node]) return false;
        motion->indices[i] = node;
        motion->points[i] = positions[node];
        motion->masses[i] = mass[node];
        motion->values[i] = velocities[node];
    }
    return true;
}

AngularVelocityResult physics_soft_body_angular_velocity_get(EntityIndex body) {
    SoftNodeMotion nodes;
    SoftMotion motion;
    if(!nodes_get(body, &nodes) || !motion_get(nodes.points, nodes.masses,
            nodes.values, nodes.count, &motion))
        return ERROR_RESULT_MAKE_ERROR(AngularVelocityResult, ERROR_ENGINE_STATE_INVALID);
    double degrees = motion.omega * (180.0 / (double)PI_F);
    if(!isfinite(degrees) || fabs(degrees) > FLT_MAX)
        return ERROR_RESULT_MAKE_ERROR(AngularVelocityResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(AngularVelocityResult, (AngularVelocity)degrees);
}

EngineResult physics_soft_body_angular_velocity_set(EntityIndex body, AngularVelocity degrees) {
    SoftNodeMotion nodes;
    Velocity next[SOFT_BODY_MAX_NODES];
    if(!nodes_get(body, &nodes) || !physics_soft_angular_velocities_get(nodes.points,
            nodes.masses, nodes.values, nodes.count, degrees, next))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    /* All velocity slots already exist. Commit cannot allocate or fail partway.
     * Keep each node's static/dynamic/held state and the authored body frame. */
    for(uint32_t i = 0; i < nodes.count; i += 1)
        velocities[nodes.indices[i]] = next[i];
    return error_result_value(true);
}
