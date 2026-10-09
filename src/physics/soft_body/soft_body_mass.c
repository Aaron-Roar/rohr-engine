/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "soft_body_mass.h"
#include "physics/physics_internal.h"
#include <float.h>
#include <math.h>
#include <string.h>

static bool masses_total_get(const Mass *values, uint32_t count,
        bool positive, double *total) {
    if(values == NULL || count > SOFT_BODY_MAX_NODES) return false;
    *total = 0;
    for(uint32_t i = 0; i < count; i++) {
        if(!isfinite(values[i]) || values[i] < 0 || (positive && values[i] == 0))
            return false;
        *total += values[i];
    }
    return isfinite(*total) && *total <= FLT_MAX;
}

bool physics_soft_mass_scale_get(const Mass *prior, uint32_t count,
        Mass total, Mass *output) {
    Mass next[SOFT_BODY_MAX_NODES];
    double previous_total, actual = 0;
    if(output == NULL || count == 0 || !isfinite(total) || total <= 0 ||
            !masses_total_get(prior, count, true, &previous_total)) return false;
    for(uint32_t i = 0; i < count; i++) {
        next[i] = (float)((double)prior[i] / previous_total * total);
        if(!isfinite(next[i]) || next[i] <= 0) return false;
        actual += next[i];
    }
    if(fabs(actual - total) > (double)total * 1e-6 || actual > FLT_MAX) return false;
    memcpy(output, next, count * sizeof(*output));
    return true;
}

/* Log-sum-exp dual objective, gradient, and covariance. Coordinates are
 * centered at the target and normalized to the node span. */
static double distribution_evaluate(const double *x, const double *y,
        const double *log_prior, uint32_t count, double a, double b,
        double *probabilities, double *gx, double *gy, double *xx,
        double *xy, double *yy) {
    double maximum = -DBL_MAX, sum = 0;
    for(uint32_t i = 0; i < count; i++)
        maximum = fmax(maximum, log_prior[i] + a * x[i] + b * y[i]);
    for(uint32_t i = 0; i < count; i++) {
        probabilities[i] = exp(log_prior[i] + a * x[i] + b * y[i] - maximum);
        sum += probabilities[i];
    }
    *gx = *gy = *xx = *xy = *yy = 0;
    for(uint32_t i = 0; i < count; i++) {
        probabilities[i] /= sum;
        *gx += probabilities[i] * x[i];
        *gy += probabilities[i] * y[i];
    }
    for(uint32_t i = 0; i < count; i++) {
        double dx = x[i] - *gx, dy = y[i] - *gy;
        *xx += probabilities[i] * dx * dx;
        *xy += probabilities[i] * dx * dy;
        *yy += probabilities[i] * dy * dy;
    }
    return maximum + log(sum);
}

bool physics_soft_mass_distribution_get(const Position *points, const Mass *prior,
        uint32_t count, Position target, Mass *output) {
    double x[SOFT_BODY_MAX_NODES], y[SOFT_BODY_MAX_NODES];
    double logs[SOFT_BODY_MAX_NODES], probabilities[SOFT_BODY_MAX_NODES];
    Mass next[SOFT_BODY_MAX_NODES];
    double total, span = 0, ux = 1, uy = 0, a = 0, b = 0;
    bool line = true, converged = false;
    if(points == NULL || output == NULL || count == 0 ||
            !isfinite(target.x) || !isfinite(target.y) ||
            !masses_total_get(prior, count, true, &total)) return false;
    for(uint32_t i = 0; i < count; i++) {
        if(!isfinite(points[i].x) || !isfinite(points[i].y)) return false;
        double dx = (double)points[i].x - points[0].x;
        double dy = (double)points[i].y - points[0].y;
        double length = hypot(dx, dy);
        if(length > span) { span = length; ux = dx / length; uy = dy / length; }
    }
    if(span == 0) {
        if(target.x != points[0].x || target.y != points[0].y) return false;
        memmove(output, prior, count * sizeof(*output));
        return true;
    }
    for(uint32_t i = 0; i < count; i++) {
        double dx = ((double)points[i].x - points[0].x) / span;
        double dy = ((double)points[i].y - points[0].y) / span;
        if(fabs(-uy * dx + ux * dy) > 1e-7) line = false;
        dx = ((double)points[i].x - target.x) / span;
        dy = ((double)points[i].y - target.y) / span;
        x[i] = ux * dx + uy * dy;
        y[i] = -uy * dx + ux * dy;
        logs[i] = log((double)prior[i] / total);
    }
    if(line) {
        double minimum = DBL_MAX, maximum = -DBL_MAX;
        if(fabs(y[0]) > 2e-7) return false;
        for(uint32_t i = 0; i < count; i++) {
            minimum = fmin(minimum, x[i]); maximum = fmax(maximum, x[i]);
            y[i] = 0;
        }
        if(minimum >= 0 || maximum <= 0) return false;
    } else {
        /* Every supporting line must have the target strictly on its interior
         * side. Positive mass at every node excludes the hull boundary. */
        for(uint32_t i = 0; i < count; i++) for(uint32_t j = i + 1; j < count; j++) {
            double dx = x[j] - x[i], dy = y[j] - y[i];
            double length = hypot(dx, dy);
            if(length == 0) continue;
            dx /= length; dy /= length;
            double low = 0, high = 0;
            for(uint32_t k = 0; k < count; k++) {
                double side = dx * (y[k] - y[i]) - dy * (x[k] - x[i]);
                low = fmin(low, side); high = fmax(high, side);
            }
            double target_side = dy * x[i] - dx * y[i];
            if((low >= -1e-12 && target_side <= 1e-12) ||
                    (high <= 1e-12 && target_side >= -1e-12)) return false;
        }
    }
    for(unsigned iteration = 0; iteration < 256; iteration++) {
        double gx, gy, xx, xy, yy;
        double objective = distribution_evaluate(x, y, logs, count, a, b,
            probabilities, &gx, &gy, &xx, &xy, &yy);
        if(hypot(gx, gy) <= 1e-10) { converged = true; break; }
        double da, db = 0;
        if(line) {
            if(xx <= 0) return false;
            da = gx / xx;
        } else {
            double determinant = xx * yy - xy * xy;
            /* Strong prior mass ratios can make the initial covariance
             * numerically singular even for a full-rank node layout. */
            double trace = xx + yy;
            if(determinant <= trace * trace * 1e-12) {
                double regularization = fmax(trace, 1e-30) * 1e-12;
                xx += regularization; yy += regularization;
                determinant = xx * yy - xy * xy;
            }
            if(determinant <= 0) return false;
            da = (yy * gx - xy * gy) / determinant;
            db = (xx * gy - xy * gx) / determinant;
        }
        double length = hypot(da, db);
        if(!isfinite(length)) return false;
        if(length > 64) { da *= 64 / length; db *= 64 / length; }
        bool accepted = false;
        double previous_error = hypot(gx, gy);
        for(double step = 1; step >= 1e-12; step *= .5) {
            double trial = distribution_evaluate(x, y, logs, count,
                a - step * da, b - step * db, probabilities, &gx, &gy, &xx, &xy, &yy);
            if(trial <= objective || (trial <= objective + 1e-14 &&
                    hypot(gx, gy) < previous_error)) {
                a -= step * da; b -= step * db; accepted = true; break;
            }
        }
        if(!accepted) return false;
    }
    if(!converged) return false;
    double actual = 0, cx = 0, cy = 0;
    for(uint32_t i = 0; i < count; i++) {
        next[i] = (float)(probabilities[i] * total);
        if(!isfinite(next[i]) || next[i] <= 0) return false;
        actual += next[i]; cx += next[i] * x[i]; cy += next[i] * y[i];
    }
    if(actual > FLT_MAX || fabs(actual - total) > total * 1e-6 ||
            hypot(cx, cy) > total * 2e-6) return false;
    memcpy(output, next, count * sizeof(*output));
    return true;
}

static bool nodes_get(EntityIndex body, EntityIndex *indices, Position *points,
        Mass *values, uint32_t *count) {
    if(body >= soft_bodies_pool.capacity || !soft_bodies_pool.used[body]) return false;
    *count = soft_bodies[body].node_count;
    if(*count > SOFT_BODY_MAX_NODES) return false;
    for(uint32_t i = 0; i < *count; i++) {
        EntityIndex node;
        if(!entity_index_get(soft_bodies[body].nodes[i], &node) ||
                !entity_index_alive_check(node) ||
                !entity_index_components_check(node, ROHR_MASS) ||
                node >= mass_pool.capacity || node >= positions_pool.capacity ||
                !mass_pool.used[node] || !positions_pool.used[node]) return false;
        indices[i] = node; points[i] = positions[node]; values[i] = mass[node];
        if(!isfinite(points[i].x) || !isfinite(points[i].y)) return false;
    }
    return true;
}

#define NODE_ARRAYS EntityIndex indices[SOFT_BODY_MAX_NODES]; \
    Position points[SOFT_BODY_MAX_NODES]; Mass values[SOFT_BODY_MAX_NODES]; uint32_t count

MassResult physics_soft_body_mass_get(EntityIndex body) {
    NODE_ARRAYS;
    double total;
    if(!nodes_get(body, indices, points, values, &count) ||
            !masses_total_get(values, count, false, &total))
        return ERROR_RESULT_MAKE_ERROR(MassResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(MassResult, (Mass)total);
}

PositionResult physics_soft_body_com_get(EntityIndex body) {
    NODE_ARRAYS;
    double total, x = 0, y = 0;
    if(!nodes_get(body, indices, points, values, &count) ||
            !masses_total_get(values, count, false, &total) || total <= 0)
        return ERROR_RESULT_MAKE_ERROR(PositionResult, ERROR_ENGINE_STATE_INVALID);
    for(uint32_t i = 0; i < count; i++) {
        x += ((double)points[i].x - points[0].x) * values[i];
        y += ((double)points[i].y - points[0].y) * values[i];
    }
    return ERROR_RESULT_MAKE_VALUE(PositionResult,
        ((Position){(float)(points[0].x + x / total), (float)(points[0].y + y / total)}));
}

static EngineResult distribution_set(EntityIndex body, bool redistribute,
        Mass total, Position target) {
    NODE_ARRAYS;
    Mass next[SOFT_BODY_MAX_NODES];
    if(!nodes_get(body, indices, points, values, &count) ||
            !(redistribute ? physics_soft_mass_distribution_get(points, values, count, target, next)
                : physics_soft_mass_scale_get(values, count, total, next)))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    /* Slots already exist and every result is validated: commit cannot allocate
     * or fail halfway through. Each node keeps its motion/ownership state. */
    for(uint32_t i = 0; i < count; i++) {
        mass[indices[i]] = next[i];
        physics_body_state_sync(indices[i]);
    }
    return error_result_value(true);
}

EngineResult physics_soft_body_mass_set(EntityIndex body, Mass total) {
    return distribution_set(body, false, total, (Position){0});
}
EngineResult physics_soft_body_com_set(EntityIndex body, Position world) {
    return distribution_set(body, true, 0, world);
}
