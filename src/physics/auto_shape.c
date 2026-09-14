/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics.h"

#include <math.h>

#define AUTO_SHAPE_PI 3.14159265358979323846f

typedef enum AutoShapeKind {
    AUTO_SHAPE_TRIANGLE,
    AUTO_SHAPE_RECTANGLE,
    AUTO_SHAPE_CIRCLE
} AutoShapeKind;

typedef struct AutoShapeConfig {
    AutoShapeKind kind;
    float width;
    float height;
    float radius;
} AutoShapeConfig;

static bool auto_shape_positive_check(float value) {
    return isfinite(value) && value > 0.0f &&
        value <= ROHR_WORLD_COORDINATE_MAX * 2.0f;
}

static void auto_shape_order_get(const Position *points, size_t count,
        size_t *order) {
    Position centroid = {0};
    for(size_t i = 0; i < count; i += 1) {
        centroid.x += points[i].x;
        centroid.y += points[i].y;
        order[i] = i;
    }
    centroid.x /= (float)count;
    centroid.y /= (float)count;
    for(size_t i = 1; i < count; i += 1) {
        size_t value = order[i];
        float angle = atan2f(points[value].y - centroid.y,
            points[value].x - centroid.x);
        size_t at = i;
        while(at > 0) {
            size_t previous = order[at - 1];
            float previous_angle = atan2f(points[previous].y - centroid.y,
                points[previous].x - centroid.x);
            if(previous_angle <= angle) break;
            order[at] = previous;
            at -= 1;
        }
        order[at] = value;
    }
}

static void auto_shape_polygon_get(const Position *corners, size_t corner_count,
        Position *points, size_t count) {
    size_t segments[4] = {1, 1, 1, 1};
    size_t output = 0;
    for(size_t i = corner_count; i < count; i += 1)
        segments[(i - corner_count) % corner_count] += 1;
    for(size_t edge = 0; edge < corner_count; edge += 1)
        for(size_t segment = 0; segment < segments[edge]; segment += 1) {
            float amount = (float)segment / (float)segments[edge];
            points[output++] = (Position){
                corners[edge].x +
                    (corners[(edge + 1) % corner_count].x - corners[edge].x) * amount,
                corners[edge].y +
                    (corners[(edge + 1) % corner_count].y - corners[edge].y) * amount};
        }
}

static bool auto_shape_positions_get(AutoShapeConfig config,
        Position *points, size_t count) {
    if(points == NULL || count < 3 || count > SOFT_BODY_MAX_NODES) return false;
    if(config.kind == AUTO_SHAPE_CIRCLE) {
        if(!auto_shape_positive_check(config.radius)) return false;
        for(size_t i = 0; i < count; i += 1) {
            float angle = -AUTO_SHAPE_PI * 0.5f +
                AUTO_SHAPE_PI * 2.0f * (float)i / (float)count;
            points[i] = (Position){cosf(angle) * config.radius,
                sinf(angle) * config.radius};
        }
        return true;
    }
    if(config.kind == AUTO_SHAPE_RECTANGLE && count >= 4) {
        if(!auto_shape_positive_check(config.width) ||
                !auto_shape_positive_check(config.height)) return false;
        Position corners[4] = {{-config.width * 0.5f, -config.height * 0.5f},
            {config.width * 0.5f, -config.height * 0.5f},
            {config.width * 0.5f, config.height * 0.5f},
            {-config.width * 0.5f, config.height * 0.5f}};
        auto_shape_polygon_get(corners, 4, points, count);
        return true;
    }
    if(config.kind == AUTO_SHAPE_TRIANGLE) {
        Position corners[3];
        float height = config.height;
        float apex = 0.0f;
        if(!auto_shape_positive_check(config.width) ||
                !auto_shape_positive_check(height) || !isfinite(apex)) return false;
        corners[0] = (Position){apex, -height * 0.5f};
        corners[1] = (Position){config.width * 0.5f, height * 0.5f};
        corners[2] = (Position){-config.width * 0.5f, height * 0.5f};
        auto_shape_polygon_get(corners, 3, points, count);
        return true;
    }
    return false;
}

static ShapeResult auto_shape_get(Shape shape, AutoShapeConfig config) {
    Position generated[MAX_VERTICIES];
    size_t order[MAX_VERTICIES];
    if(shape.amount_of_vertices < 3 || shape.amount_of_vertices > MAX_VERTICIES ||
            !auto_shape_positions_get(config, generated, shape.amount_of_vertices))
        return ERROR_RESULT_MAKE_ERROR(ShapeResult, ERROR_ENGINE_INVALID_SHAPE);
    auto_shape_order_get(shape.vertices, shape.amount_of_vertices, order);
    for(size_t i = 0; i < shape.amount_of_vertices; i += 1)
        shape.vertices[order[i]] = generated[i];
    shape.collision_geometry_prepared = false;
    shape.concave_piece_count = 0;
    return ERROR_RESULT_MAKE_VALUE(ShapeResult, shape);
}

static EngineResult hitbox_autoshape_set(Entity entity, AutoShapeConfig config) {
    ShapeResult shape = physics_hitbox_get(entity);
    if(shape.kind == ERROR_RESULT_ERROR) return error_result_error(shape.result.error);
    shape = auto_shape_get(shape.result.value, config);
    if(shape.kind == ERROR_RESULT_ERROR) return error_result_error(shape.result.error);
    return physics_hitbox_set(entity, shape.result.value);
}

static EngineResult soft_body_autoshape_set(Entity soft_body,
        AutoShapeConfig config) {
    SoftBodyResult body = physics_soft_body_get(soft_body);
    Position current[SOFT_BODY_MAX_NODES];
    Position generated[SOFT_BODY_MAX_NODES];
    size_t order[SOFT_BODY_MAX_NODES];
    if(body.kind == ERROR_RESULT_ERROR) return error_result_error(body.result.error);
    if(!auto_shape_positions_get(config, generated, body.result.value.node_count))
        return error_result_error(ERROR_ENGINE_INVALID_SHAPE);
    for(uint32_t i = 0; i < body.result.value.node_count; i += 1) {
        PositionResult position = physics_position_get(body.result.value.nodes[i]);
        if(position.kind == ERROR_RESULT_ERROR)
            return error_result_error(position.result.error);
        current[i] = position.result.value;
    }
    auto_shape_order_get(current, body.result.value.node_count, order);
    for(uint32_t i = 0; i < body.result.value.node_count; i += 1) {
        EngineResult result = physics_position_set(
            body.result.value.nodes[order[i]], generated[i]);
        if(result.kind == ERROR_RESULT_ERROR) return result;
    }
    return error_result_value(true);
}

EngineResult physics_hitbox_autoshape_circle_set(Entity entity, float radius) {
    return hitbox_autoshape_set(entity,
        (AutoShapeConfig){.kind = AUTO_SHAPE_CIRCLE, .radius = radius});
}

EngineResult physics_hitbox_autoshape_rectangle_set(Entity entity,
        float width, float height) {
    return hitbox_autoshape_set(entity, (AutoShapeConfig){
        .kind = AUTO_SHAPE_RECTANGLE, .width = width, .height = height});
}

EngineResult physics_hitbox_autoshape_triangle_set(Entity entity,
        float width, float height) {
    return hitbox_autoshape_set(entity, (AutoShapeConfig){
        .kind = AUTO_SHAPE_TRIANGLE, .width = width, .height = height});
}

EngineResult physics_soft_body_autoshape_circle_set(Entity soft_body,
        float radius) {
    return soft_body_autoshape_set(soft_body,
        (AutoShapeConfig){.kind = AUTO_SHAPE_CIRCLE, .radius = radius});
}

EngineResult physics_soft_body_autoshape_rectangle_set(Entity soft_body,
        float width, float height) {
    return soft_body_autoshape_set(soft_body, (AutoShapeConfig){
        .kind = AUTO_SHAPE_RECTANGLE, .width = width, .height = height});
}

EngineResult physics_soft_body_autoshape_triangle_set(Entity soft_body,
        float width, float height) {
    return soft_body_autoshape_set(soft_body, (AutoShapeConfig){
        .kind = AUTO_SHAPE_TRIANGLE, .width = width, .height = height});
}
