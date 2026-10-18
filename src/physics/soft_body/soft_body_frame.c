/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "soft_body_frame.h"
#include "entity/entity_internal.h"
#include "physics/physics_internal.h"
#include "physics/physics_operation.h"
#include "physics/physics_step_internal.h"
#include <float.h>
#include <math.h>

/* Nodes store their last sampled world position. A used body slot marks a
 * complete baseline. Direct ownership gives each node exactly one frame. */
static PositionPool prior_points;

EngineResult physics_soft_body_frame_reserve(size_t capacity) {
    if(capacity <= prior_points.capacity) return error_result_value(true);
    if(error_check(PositionPool_expand(&prior_points, capacity - prior_points.capacity)))
        return error_result_error(ERROR_ENGINE_TABLE_EXPANSION_FAILED);
    return error_result_value(true);
}
void physics_soft_body_frame_destroy(void) {
    (void)PositionPool_destroy(&prior_points);
}
void physics_soft_body_frame_clear(EntityIndex index) {
    if(index < prior_points.capacity && prior_points.used[index])
        (void)PositionPool_release_at(&prior_points, index);
}

typedef struct FramePoints {
    EntityIndex indices[SOFT_BODY_MAX_NODES];
    Position points[SOFT_BODY_MAX_NODES];
    uint32_t count;
} FramePoints;

static bool frame_points_get(EntityIndex body, FramePoints *frame) {
    if(body >= soft_bodies_pool.capacity || !soft_bodies_pool.used[body] ||
            body >= prior_points.capacity) return false;
    const SoftBody *soft = &soft_bodies[body];
    if(soft->node_count > SOFT_BODY_MAX_NODES) return false;
    frame->count = soft->node_count;
    for(uint32_t i = 0; i < frame->count; ++i) {
        EntityIndex node;
        if(!entity_index_get(soft->nodes[i], &node) || node == body ||
                node >= prior_points.capacity || !soft_body_nodes_pool.used[node] ||
                soft_body_nodes[node].soft_body != soft->origin ||
                !positions_pool.used[node] || !physics_world_position_check(positions[node]))
            return false;
        frame->indices[i] = node;
        frame->points[i] = positions[node];
    }
    return true;
}
static void frame_baseline_set(EntityIndex body, const FramePoints *frame) {
    for(uint32_t i = 0; i < frame->count; ++i)
        (void)PositionPool_store_at(&prior_points, frame->indices[i], frame->points[i]);
    (void)PositionPool_store_at(&prior_points, body, (Position){0});
}
void physics_soft_body_frame_reset(EntityIndex body) {
    FramePoints frame;
    physics_soft_body_frame_clear(body);
    if(frame_points_get(body, &frame)) frame_baseline_set(body, &frame);
}

static EngineResult frame_update(EntityIndex body) {
    FramePoints frame;
    if(!frame_points_get(body, &frame) || !positions_pool.used[body] ||
            !orientations_pool.used[body] || !physics_world_position_check(positions[body]) ||
            !isfinite(orientations[body])) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(!prior_points.used[body] || frame.count == 0) {
        frame_baseline_set(body, &frame);
        return error_result_value(true);
    }
    double old_x = 0, old_y = 0, new_x = 0, new_y = 0;
    bool moved = false;
    for(uint32_t i = 0; i < frame.count; ++i) {
        EntityIndex node = frame.indices[i];
        if(!prior_points.used[node]) {
            frame_baseline_set(body, &frame);
            return error_result_value(true);
        }
        Position old = prior_points.objects[node], next = frame.points[i];
        old_x += old.x; old_y += old.y;
        new_x += next.x; new_y += next.y;
        moved |= old.x != next.x || old.y != next.y;
    }
    if(!moved) return error_result_value(true);
    old_x /= frame.count; old_y /= frame.count;
    new_x /= frame.count; new_y /= frame.count;
    double dot = 0, cross = 0, old_spread = 0, new_spread = 0;
    for(uint32_t i = 0; i < frame.count; ++i) {
        Position old = prior_points.objects[frame.indices[i]], next = frame.points[i];
        double ax = old.x - old_x, ay = old.y - old_y;
        double bx = next.x - new_x, by = next.y - new_y;
        dot += ax * bx + ay * by;
        cross += ay * bx - ax * by; /* Clockwise in world coordinates. */
        old_spread += ax * ax + ay * ay;
        new_spread += bx * bx + by * by;
    }
    /* Relative correlation avoids inventing an angle for collapsed or
     * cancelling configurations, without imposing a world-size threshold. */
    double radians = 0;
    if(hypot(dot, cross) > 64 * DBL_EPSILON * sqrt(old_spread * new_spread))
        radians = atan2(cross, dot);
    double x = positions[body].x - old_x, y = positions[body].y - old_y;
    double cosine = cos(radians), sine = sin(radians);
    double px = new_x + x * cosine + y * sine;
    double py = new_y + y * cosine - x * sine;
    double angle = orientations[body] + radians * (180.0 / 3.14159265358979323846);
    if(!isfinite(px) || !isfinite(py) || fabs(px) > FLT_MAX || fabs(py) > FLT_MAX ||
            !isfinite(angle) || fabs(angle) > FLT_MAX)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    Position position = {(float)px, (float)py};
    if(!physics_world_position_check(position))
        return error_result_error(ERROR_ENGINE_POSITION_OUT_OF_RANGE);
    /* These slots already exist. Do not call origin setters: those transform
     * the nodes, whereas this frame only observes their completed movement. */
    positions[body] = position;
    orientations[body] = (float)angle;
    frame_baseline_set(body, &frame);
    return error_result_value(true);
}

void physics_soft_body_frames_update(Report *report) {
    for(uint32_t i = 0; i < entity_alive_count_get(); ++i) {
        EntityIndex body;
        if(!physics_step_alive_index_at(i, &body) || !soft_bodies_pool.used[body]) continue;
        EngineResult result = frame_update(body);
        if(error_check(result))
            physics_report_issue_add(report, ROHR_PHYSICS_POSITION,
                ROHR_REPORT_SEVERITY_WARNING, result.result.error,
                ROHR_REPORT_CONTRIBUTION_SKIPPED, physics_operation_entity_get(body), 0,
                "Soft-body origin tracking preserved its last valid pose");
    }
}
