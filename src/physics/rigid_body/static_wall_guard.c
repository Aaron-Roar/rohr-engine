/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "static_wall_guard.h"
#include "physics/physics_step_internal.h"
#include "math2d.h"
#include <float.h>
#include <math.h>
#include <SDL3/SDL_timer.h>

#ifndef ROHR_PHYSICS_STATIC_WALL_GUARD
#define ROHR_PHYSICS_STATIC_WALL_GUARD 0
#endif
#ifndef ROHR_WALL_GUARD_PROFILE
#define ROHR_WALL_GUARD_PROFILE 0
#endif
#if ROHR_WALL_GUARD_PROFILE
#define GUARD_CLOCK() SDL_GetPerformanceCounter()
#else
#define GUARD_CLOCK() UINT64_C(0)
#endif

#if ROHR_PHYSICS_STATIC_WALL_GUARD
/* Fixed scratch capacity covers every possible entity without allocator work. */
static AABBTreeNode wall_nodes[2 * MAX_ENTITIES];
static uint32_t wall_stack[2 * MAX_ENTITIES];
static AABBTree wall_tree = {.nodes = wall_nodes, .query_stack = wall_stack,
    .capacity = 2 * MAX_ENTITIES, .root = AABB_TREE_NODE_INVALID};
static unsigned scope_depth;
static bool dirty;
static PhysicsWallGuardCounts counts;

static bool wall_check(EntityIndex i) {
    return physics_step_entity_components_check(i, ROHR_STATIC | ROHR_COLLISION | ROHR_HIT_BOX) &&
        !physics_step_entity_components_check(i, ROHR_PARTICLE);
}

static void wall_prepare_internal(Report *report) {
    aabb_tree_clear(&wall_tree);
    ++counts.builds;
    dirty = false;
    for(uint32_t n = 0; n < physics_step_collider_iteration_count_get(); ++n) {
        EntityIndex i;
        if(!physics_step_collider_index_at(n, &i) || !wall_check(i)) continue;
        if(!physics_step_hitbox_update(i, report)) continue;
        const Shape *s = &world_hit_boxes[i];
        if(s->amount_of_vertices < 3) continue;
        AABB bounds = {.min_x = s->vertices[0].x, .max_x = s->vertices[0].x,
            .min_y = s->vertices[0].y, .max_y = s->vertices[0].y};
        for(uint32_t v = 1; v < s->amount_of_vertices; ++v) {
            bounds.min_x = fminf(bounds.min_x, s->vertices[v].x);
            bounds.max_x = fmaxf(bounds.max_x, s->vertices[v].x);
            bounds.min_y = fminf(bounds.min_y, s->vertices[v].y);
            bounds.max_y = fmaxf(bounds.max_y, s->vertices[v].y);
        }
        Entity e = physics_operation_entity_get(i);
        EngineResult result = aabb_tree_insert(&wall_tree, e, bounds);
        if(result.kind == ERROR_RESULT_ERROR) {
            physics_report_issue_add(report, ROHR_PHYSICS_GEOMETRY, ROHR_REPORT_SEVERITY_FATAL,
                result.result.error, ROHR_REPORT_UPDATE_ABORTED, e, 0,
                "static wall guard could not prepare geometry");
            return;
        }
    }
}

static void wall_prepare(Report *report) {
    uint64_t start = GUARD_CLOCK();
    wall_prepare_internal(report);
    counts.prepare_ticks += GUARD_CLOCK() - start;
}

void physics_wall_guard_begin(Report *report) {
    if(scope_depth++ == 0) wall_prepare(report);
}
void physics_wall_guard_end(void) {
    if(scope_depth && --scope_depth == 0) aabb_tree_clear(&wall_tree);
}
void physics_wall_guard_dirty_add(EntityIndex index) {
    if(scope_depth && wall_check(index)) dirty = true;
}
PhysicsWallGuardCounts physics_wall_guard_counts_get(void) { return counts; }
void physics_wall_guard_counts_clear(void) { counts = (PhysicsWallGuardCounts){0}; }

typedef struct WallCast {
    EntityIndex particle, wall;
    double x, y, dx, dy, radius, fraction, nx, ny;
    bool hit;
} WallCast;

static void wall_hit(WallCast *q, double t, double nx, double ny) {
    if(t < 0 || t > q->fraction || q->dx * nx + q->dy * ny >= 0) return;
    q->fraction = t;
    q->nx = nx; q->ny = ny; q->hit = true;
}

/* A moving circle against a polygon boundary is a point against edge capsules.
 * This supports the actual outer boundary of concave polygons too, without
 * casting against decomposition seams. Double intermediates avoid cancellation
 * in high-speed/large-coordinate quadratic roots. */
static void wall_vertex_cast(WallCast *q, double x, double y, bool embedded) {
    double px = q->x - x, py = q->y - y;
    double c = px * px + py * py - q->radius * q->radius;
    double b = px * q->dx + py * q->dy;
    if(c <= 0) {
        double length = hypot(px, py);
        if(!embedded && length > 0) wall_hit(q, 0, px / length, py / length);
        return;
    }
    double a = q->dx * q->dx + q->dy * q->dy;
    double discriminant = b * b - a * c;
    if(b >= 0 || discriminant < 0 || a == 0) return;
    double t = c / (-b + sqrt(discriminant));
    double nx = px + q->dx * t, ny = py + q->dy * t;
    double length = hypot(nx, ny);
    if(length > 0) wall_hit(q, t, nx / length, ny / length);
}

static void wall_shape_cast(WallCast *q, const Shape *s) {
    ++counts.casts;
    bool inside = false;
    double nearest = DBL_MAX, outward_x = 0, outward_y = 0;
    double area = 0, nearest_edge_x = 0, nearest_edge_y = 0;
    for(uint32_t v = 0; v < s->amount_of_vertices; ++v) {
        Position a = s->vertices[v], b = s->vertices[(v + 1) % s->amount_of_vertices];
        double ex = (double)b.x - a.x, ey = (double)b.y - a.y;
        area += (double)a.x * b.y - (double)a.y * b.x;
        if((a.y > q->y) != (b.y > q->y) &&
                q->x < a.x + ex * (q->y - a.y) / ey) inside = !inside;
        double length2 = ex * ex + ey * ey;
        double t = length2 > 0 ? fmax(0, fmin(1,
            ((q->x - a.x) * ex + (q->y - a.y) * ey) / length2)) : 0;
        double nx = a.x + ex * t - q->x, ny = a.y + ey * t - q->y;
        double distance = nx * nx + ny * ny;
        if(distance < nearest) {
            nearest = distance; outward_x = nx; outward_y = ny;
            nearest_edge_x = ex; nearest_edge_y = ey;
        }
    }
    /* Ray parity is ambiguous exactly on the boundary. Use polygon winding
     * there so an embedded circle can separate but cannot enter the wall. */
    if(nearest == 0) {
        inside = true;
        outward_x = area >= 0 ? nearest_edge_y : -nearest_edge_y;
        outward_y = area >= 0 ? -nearest_edge_x : nearest_edge_x;
    }
    if(inside && q->dx * outward_x + q->dy * outward_y < 0) {
        double length = hypot(outward_x, outward_y);
        if(length == 0) return;
        wall_hit(q, 0, outward_x / length, outward_y / length);
        return;
    }
    for(uint32_t v = 0; v < s->amount_of_vertices; ++v) {
        Position a = s->vertices[v], b = s->vertices[(v + 1) % s->amount_of_vertices];
        double ex = (double)b.x - a.x, ey = (double)b.y - a.y;
        double length = hypot(ex, ey);
        wall_vertex_cast(q, a.x, a.y, inside);
        if(length == 0) continue;
        double tx = ex / length, ty = ey / length, nx = -ty, ny = tx;
        double distance = (q->x - a.x) * nx + (q->y - a.y) * ny;
        double speed = q->dx * nx + q->dy * ny;
        double along = (q->x - a.x) * tx + (q->y - a.y) * ty;
        if(fabs(distance) <= q->radius && along >= 0 && along <= length) {
            if(!inside) wall_hit(q, 0, distance >= 0 ? nx : -nx, distance >= 0 ? ny : -ny);
            continue;
        }
        if(speed == 0) continue;
        double sign = speed < 0 ? 1 : -1;
        double t = (sign * q->radius - distance) / speed;
        double projection = along + t * (q->dx * tx + q->dy * ty);
        if(projection >= 0 && projection <= length) wall_hit(q, t, nx * sign, ny * sign);
    }
}

static bool wall_visit(Entity target, void *context) {
    WallCast *q = context;
    EntityIndex i;
    ++counts.candidates;
    if(!entity_index_get(target, &i) || !wall_check(i) ||
            !physics_collision_response_to_by_index_check(q->particle, i)) return true;
    WallCast candidate = *q;
    candidate.hit = false;
    uint64_t start = GUARD_CLOCK();
    wall_shape_cast(&candidate, &world_hit_boxes[i]);
    counts.cast_ticks += GUARD_CLOCK() - start;
    if(candidate.hit) { *q = candidate; q->wall = i; }
    return true;
}

static void wall_change(PhysicsChange *c, Report *report) {
    EntityIndex i = c->index;
    if(!scope_depth || !(c->fields & PHYSICS_CHANGE_POSITION) ||
            !physics_step_entity_components_check(i, ROHR_PARTICLE | ROHR_COLLISION | ROHR_HIT_BOX) ||
            !physics_entity_simulated_get(i) || !physics_entity_nonrotating_check(i)) return;
    /* Prescribed transforms own their target pose. Protecting the driver and
     * its coupled dependents is broader blocking work, not a particle clamp. */
    if(physics_step_entity_components_check(i, ROHR_TRANSFORM_LOCK) &&
            (transform_locks[i].lock_position || transform_locks[i].lock_orientation)) return;
    if(c->position.x == positions[i].x && c->position.y == positions[i].y &&
            (!(c->fields & PHYSICS_CHANGE_ORIENTATION) || c->orientation == orientations[i])) return;
    if(dirty) wall_prepare(report);
    if(wall_tree.root == AABB_TREE_NODE_INVALID || (report && report->has_fatal)) return;
    Position local = {0};
    if(i < particle_geometries_pool.capacity && particle_geometries_pool.used[i])
        local = particle_geometries_pool.objects[i].local_origin;
    float angle = (c->fields & PHYSICS_CHANGE_ORIENTATION) ? c->orientation : orientations[i];
    Position center = positions[i], offset = {0};
    if(local.x != 0 || local.y != 0) {
        center = physics_particle_world_origin_by_index_get(i);
        offset = math_vector_rotate(local, angle);
    }
    double dx = (double)c->position.x + offset.x - center.x;
    double dy = (double)c->position.y + offset.y - center.y;
    if(dx == 0 && dy == 0) return;
    double radius = physics_particle_radius_by_index_get(i);
    if(!physics_vector_finite_check(center) || !isfinite(radius) || radius < 0) return;
    WallCast q = {.particle = i, .x = center.x, .y = center.y,
        .dx = dx, .dy = dy, .radius = radius, .fraction = 1};
    AABB bounds = {.min_x = nextafterf((float)(fmin(q.x, q.x + dx) - radius), -INFINITY),
        .max_x = nextafterf((float)(fmax(q.x, q.x + dx) + radius), INFINITY),
        .min_y = nextafterf((float)(fmin(q.y, q.y + dy) - radius), -INFINITY),
        .max_y = nextafterf((float)(fmax(q.y, q.y + dy) + radius), INFINITY)};
    ++counts.queries;
    uint64_t query_start = GUARD_CLOCK();
    (void)aabb_tree_query(&wall_tree, bounds, wall_visit, &q);
    counts.query_ticks += GUARD_CLOCK() - query_start;
    if(!q.hit) return;
    ++counts.hits;
    /* Retreat by a small fraction of the radius, capped at 1e-5 world units.
     * nextafter rounds the committed float back toward the safe starting pose. */
    double length = hypot(dx, dy);
    double t = fmax(0, q.fraction - fmin(1e-5, radius * 1e-4) / length);
    Position next = {(float)(q.x + dx * t - offset.x), (float)(q.y + dy * t - offset.y)};
    if(dx != 0) next.x = nextafterf(next.x, dx > 0 ? -INFINITY : INFINITY);
    if(dy != 0) next.y = nextafterf(next.y, dy > 0 ? -INFINITY : INFINITY);
    if(!physics_world_position_check(next)) {
        next = positions[i];
        if(c->fields & PHYSICS_CHANGE_ORIENTATION) c->orientation = orientations[i];
        physics_operation_reject(report, ROHR_PHYSICS_POSITION,
            ERROR_ENGINE_POSITION_OUT_OF_RANGE, physics_operation_entity_get(i),
            physics_operation_entity_get(q.wall));
    }
    c->position = next;
    Velocity before = (c->fields & PHYSICS_CHANGE_VELOCITY) ? c->velocity : velocities[i];
    double inward = before.x * q.nx + before.y * q.ny;
    if(inward < 0) {
        double response_x = q.nx, response_y = q.ny;
        if(physics_step_entity_components_check(i, ROHR_AXIS_LOCK)) {
            Axis axis = axis_locks[i].axis;
            double length2 = (double)axis.x * axis.x + (double)axis.y * axis.y;
            double projection = length2 > 0 ? (q.nx * axis.x + q.ny * axis.y) / length2 : 0;
            response_x = axis.x * projection; response_y = axis.y * projection;
        }
        double denominator = q.nx * response_x + q.ny * response_y;
        double impulse = denominator > 1e-12 ? inward / denominator : 0;
        c->velocity = (Velocity){(float)(before.x - impulse * response_x),
            (float)(before.y - impulse * response_y)};
        /* Extreme oblique locks can require an unrepresentable impulse.
         * Holding all linear motion is a finite, dissipative fallback. */
        if(!physics_vector_finite_check(c->velocity)) c->velocity = (Velocity){0};
        c->fields |= PHYSICS_CHANGE_VELOCITY;
    }
    ++counts.clamped;
    ContactInfo contact = {.detected = true, .normal = {(float)-q.nx, (float)-q.ny},
        .point_count = 1};
    contact.points[0].position = (Position){(float)(q.x + dx * q.fraction - q.nx * radius),
        (float)(q.y + dy * q.fraction - q.ny * radius)};
    contact.points[0].relative_velocity = (Velocity){-before.x, -before.y};
    if(inward < 0) contact.points[0].normal_impulse = (Vec2D){
        (before.x - c->velocity.x) * mass[i], (before.y - c->velocity.y) * mass[i]};
    if(!physics_vector_finite_check(contact.points[0].normal_impulse)) {
        /* Detection remains reportable when the impulse cannot be represented;
         * do not publish an infinite or fabricated point impulse. */
        contact.point_count = 0;
        contact.points[0] = (ContactPointInfo){0};
        physics_report_issue_add(report, ROHR_PHYSICS_NORMAL_IMPULSE,
            ROHR_REPORT_SEVERITY_WARNING, ERROR_ENGINE_STATE_INVALID,
            ROHR_REPORT_CONTRIBUTION_SKIPPED, physics_operation_entity_get(i),
            physics_operation_entity_get(q.wall),
            "wall guard contact point impulse exceeds reporting range");
    }
    /* A swept response is a contact, not a claim of geometric overlap at the
     * stopped position. Preserve query orientation through the shared recorder. */
    physics_step_interaction_by_index_record(i, q.wall, (OverlapInfo){0}, contact,
        PHYSICS_INTERACTION_CONTACT, report);
}

void physics_wall_guard_change(PhysicsChange *change, Report *report) {
    uint64_t start = GUARD_CLOCK();
    uint64_t prepare_before = counts.prepare_ticks;
    wall_change(change, report);
    counts.change_ticks += GUARD_CLOCK() - start - (counts.prepare_ticks - prepare_before);
}
#else
void physics_wall_guard_begin(Report *report) { (void)report; }
void physics_wall_guard_end(void) { }
void physics_wall_guard_dirty_add(EntityIndex index) { (void)index; }
void physics_wall_guard_change(PhysicsChange *change, Report *report) { (void)change; (void)report; }
PhysicsWallGuardCounts physics_wall_guard_counts_get(void) { return (PhysicsWallGuardCounts){0}; }
void physics_wall_guard_counts_clear(void) { }
#endif
