/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "experiment.h"
#include <float.h>
#include <math.h>

static void projection_get(const Shape *s, Vec2D n, float *low, float *high) {
    *low = FLT_MAX; *high = -FLT_MAX;
    for(unsigned i = 0; i < s->amount_of_vertices; ++i) {
        float p = s->vertices[i].x*n.x + s->vertices[i].y*n.y;
        *low = fminf(*low, p); *high = fmaxf(*high, p);
    }
}

ExperimentContact experiment_separation_get(const Shape *a, const Shape *b) {
    ExperimentContact c = {.gap = -FLT_MAX};
    const Shape *shapes[] = {a, b};
    for(unsigned s = 0; s < 2; ++s)
        for(unsigned i = 0; i < shapes[s]->amount_of_vertices; ++i) {
            Vec2D p = shapes[s]->vertices[i];
            Vec2D q = shapes[s]->vertices[(i+1)%shapes[s]->amount_of_vertices];
            Vec2D n = {p.y-q.y, q.x-p.x};
            float length = hypotf(n.x, n.y);
            if(length == 0) continue;
            n.x /= length; n.y /= length;
            float al, ah, bl, bh;
            projection_get(a, n, &al, &ah); projection_get(b, n, &bl, &bh);
            if(bl-ah > c.gap) { c.gap = bl-ah; c.normal = n; }
            if(al-bh > c.gap) { c.gap = al-bh; c.normal = (Vec2D){-n.x, -n.y}; }
        }
    Vec2D tangent = {-c.normal.y, c.normal.x};
    float al, ah, bl, bh, atl, ath, btl, bth;
    projection_get(a, c.normal, &al, &ah); projection_get(b, c.normal, &bl, &bh);
    projection_get(a, tangent, &atl, &ath); projection_get(b, tangent, &btl, &bth);
    float normal = (ah+bl)*.5f, side = (fmaxf(atl,btl)+fminf(ath,bth))*.5f;
    c.point = (Vec2D){normal*c.normal.x+side*tangent.x, normal*c.normal.y+side*tangent.y};
    return c;
}

static ExperimentPose interpolate(ExperimentPose a, ExperimentPose b, double t) {
    return (ExperimentPose){{a.p.x+(b.p.x-a.p.x)*t, a.p.y+(b.p.y-a.p.y)*t},
        a.angle+(b.angle-a.angle)*t};
}

static double radius_get(EntityIndex i) {
    double radius = 0;
    for(unsigned v = 0; v < hit_boxes[i].amount_of_vertices; ++v)
        radius = fmax(radius, hypot(hit_boxes[i].vertices[v].x, hit_boxes[i].vertices[v].y));
    return radius;
}

bool experiment_cast_check(EntityIndex a, EntityIndex b,
        ExperimentPose sa, ExperimentPose ea, ExperimentPose sb, ExperimentPose eb,
        double *fraction, ExperimentContact *contact, unsigned *iterations, bool *limited) {
    /* Convex SAT distance is a lower bound on Euclidean separation. The sum
     * of translation and angular arc lengths bounds closure over this linear
     * pose interpolation. Finite search exhaustion is reported, never success. */
    double speed = hypot((eb.p.x-sb.p.x)-(ea.p.x-sa.p.x), (eb.p.y-sb.p.y)-(ea.p.y-sa.p.y));
    speed += fabs(rohr_math_degrees_to_radians(ea.angle-sa.angle))*radius_get(a);
    speed += fabs(rohr_math_degrees_to_radians(eb.angle-sb.angle))*radius_get(b);
    double t = 0;
    *limited = false; *iterations = 0;
    for(unsigned iteration = 0; iteration < 48; ++iteration) {
        ExperimentPose pa = interpolate(sa, ea, t), pb = interpolate(sb, eb, t);
        Shape wa = physics_shape_world_translate(hit_boxes[a], pa.p, pa.angle);
        Shape wb = physics_shape_world_translate(hit_boxes[b], pb.p, pb.angle);
        *contact = experiment_separation_get(&wa, &wb); ++*iterations;
        if(contact->gap <= 0.0001f) { *fraction = t; return true; }
        if(!(speed > 0)) return false;
        double advance = (contact->gap-0.00005)/speed;
        if(t+advance > 1) return false;
        if(!(advance > 0) || t+advance == t) { *limited = true; return false; }
        t += advance;
    }
    *limited = true;
    return false;
}
