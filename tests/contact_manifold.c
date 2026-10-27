/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "physics/collision/contact_manifold.h"

#include <math.h>
#include <assert.h>
#include <stdio.h>

static Shape transformed(Shape shape, unsigned start, bool reverse,
        float angle, Vec2D offset) {
    Shape result = shape;
    for(unsigned i = 0; i < shape.amount_of_vertices; ++i) {
        unsigned index = (start + (reverse ? shape.amount_of_vertices - i : i)) %
            shape.amount_of_vertices;
        Vec2D point = math_vector_rotate(shape.vertices[index], angle);
        result.vertices[i] = (Vec2D){point.x + offset.x, point.y + offset.y};
    }
    return result;
}

static void equivalent_contacts(Shape first, Shape second, Shape control_first,
        Shape control_second) {
    ContactManifoldSet actual = contact_manifold_set_polygon_get(first, second);
    ContactManifoldSet expected = contact_manifold_set_polygon_get(
        control_first, control_second);
    assert(expected.count == 1 && expected.values[0].count > 0);
    if(actual.count != 1 || actual.values[0].count != expected.values[0].count) {
        fprintf(stderr, "Subdivided face lost contacts: manifolds=%u points=%u\n",
            actual.count, actual.values[0].count);
        assert(false);
    }
    assert(fabsf(actual.values[0].depth - expected.values[0].depth) < .003f);
    assert(math_dot_product(actual.values[0].normal, expected.values[0].normal) > .9999f);
    for(unsigned i = 0; i < actual.values[0].count; ++i) {
        bool matched = false;
        Vec2D point = actual.values[0].points[i];
        assert(isfinite(point.x) && isfinite(point.y));
        for(unsigned j = 0; j < expected.values[0].count; ++j) {
            Vec2D delta = math_vector_subtract(point, expected.values[0].points[j]);
            if(math_dot_product(delta, delta) < .003f * .003f) matched = true;
        }
        assert(matched);
    }
}

static void subdivided_faces_test(void) {
    const Shape triangle = {.amount_of_vertices = 5, .vertices = {
        {0, 67.6931839f}, {78.1653595f, 67.6931839f}, {39.0826797f, 0},
        {0, -67.6931839f}, {-78.1653595f, 67.6931839f}}};
    const Shape control = {.amount_of_vertices = 3, .vertices = {
        {78.1653595f, 67.6931839f}, {0, -67.6931839f}, {-78.1653595f, 67.6931839f}}};
    for(unsigned beam = 0; beam < 2; ++beam)
    for(unsigned side = 0; side < 2; ++side)
    for(unsigned rotation = 0; rotation < 4; ++rotation)
    for(unsigned start = 0; start < 5; ++start)
    for(unsigned reverse = 0; reverse < 2; ++reverse) {
        float angle = 30.0f * rotation;
        Vec2D offset = rotation & 1 ? (Vec2D){200, -170} : (Vec2D){0, 0};
        Shape other = beam ? math_square_create(40, 2) : math_circle_create(10, 8);
        other = transformed(other, 0, false, 0,
            (Vec2D){side ? 40 : -40, beam ? 68 : 75});
        Shape a = transformed(triangle, start, reverse, angle, offset);
        Shape c = transformed(control, 0, false, angle, offset);
        for(unsigned other_start = 0; other_start < other.amount_of_vertices; ++other_start) {
            Shape b = transformed(other, other_start, reverse, angle, offset);
            equivalent_contacts(a, b, c, b);
            equivalent_contacts(b, a, b, c);
        }
    }
    /* Both participants have subdivided support faces. Either can become the
     * reference on a tied normal, so incident extraction must expand too. */
    Shape floor = {.amount_of_vertices = 6, .vertices = {
        {0, 0}, {50, 0}, {50, -20}, {0, -20}, {-50, -20}, {-50, 0}}};
    Shape floor_control = {.amount_of_vertices = 4, .vertices = {
        {50, 0}, {50, -20}, {-50, -20}, {-50, 0}}};
    Shape box = {.amount_of_vertices = 6, .vertices = {
        {0, -1}, {30, -1}, {30, 10}, {0, 10}, {-30, 10}, {-30, -1}}};
    Shape box_control = {.amount_of_vertices = 4, .vertices = {
        {30, -1}, {30, 10}, {-30, 10}, {-30, -1}}};
    for(unsigned a = 0; a < 6; ++a)
    for(unsigned b = 0; b < 6; ++b)
    for(unsigned reverse = 0; reverse < 2; ++reverse) {
        Shape first = transformed(floor, a, reverse, 0, (Vec2D){0});
        Shape second = transformed(box, b, !reverse, 0, (Vec2D){0});
        equivalent_contacts(first, second, floor_control, box_control);
        equivalent_contacts(second, first, box_control, floor_control);
        equivalent_contacts(first, box_control, floor_control, box_control);
        equivalent_contacts(floor_control, second, floor_control, box_control);
    }
}

static void corner_tolerance_test(float width, float deviation, bool expand) {
    Shape support = {.amount_of_vertices = 5, .vertices = {
        {0, 0}, {width, 0}, {width, -20}, {-width, -20}, {-width, -deviation}}};
    Shape box = {.amount_of_vertices = 4, .vertices = {
        {-10, -1}, {10, -1}, {10, 10}, {-10, 10}}};
    ContactManifold manifold = contact_manifold_polygon_get(support, box, (Axis){0, 1});
    assert(manifold.count == 2);
    float min_x = fminf(manifold.points[0].x, manifold.points[1].x);
    float max_x = fmaxf(manifold.points[0].x, manifold.points[1].x);
    assert(fabsf(min_x - (expand ? -10 : 0)) < .001f);
    assert(fabsf(max_x - 10) < .001f);
}

static void maximum_face_test(void) {
    Shape support = {.amount_of_vertices = MAX_VERTICIES};
    for(unsigned i = 0; i < MAX_VERTICIES - 2; ++i)
        support.vertices[i] = (Vec2D){(float)i, 0};
    float width = MAX_VERTICIES - 3;
    support.vertices[MAX_VERTICIES - 2] = (Vec2D){width, -20};
    support.vertices[MAX_VERTICIES - 1] = (Vec2D){0, -20};
    Shape box = {.amount_of_vertices = 4, .vertices = {
        {10, -1}, {20, -1}, {20, 10}, {10, 10}}};
    for(unsigned start = 0; start < MAX_VERTICIES; ++start)
    for(unsigned reverse = 0; reverse < 2; ++reverse) {
        Shape shifted = transformed(support, start, reverse, 0, (Vec2D){0});
        ContactManifoldSet set = contact_manifold_set_polygon_get(shifted, box);
        assert(set.count == 1 && set.values[0].count == 2);
        ContactManifold contact = set.values[0];
        assert(fabsf(contact.normal.x) < .0001f && contact.normal.y > .9999f);
        assert(fabsf(contact.depth - 1) < .0001f);
        assert(fabsf(fminf(contact.points[0].x, contact.points[1].x) - 10) < .0001f);
        assert(fabsf(fmaxf(contact.points[0].x, contact.points[1].x) - 20) < .0001f);
        for(unsigned i = 0; i < 2; ++i) assert(fabsf(contact.points[i].y + .5f) < .0001f);
    }
}

int main(void) {
    /* A tilted face needs each endpoint's signed penetration, rather than
     * applying the deepest endpoint's correction to the whole face. */
    Shape tilted = transformed(math_square_create(2, 2), 0, false, .02f,
        (Vec2D){0, .9999f});
    Shape plane = transformed(math_square_create(10, 2), 0, false, 0,
        (Vec2D){0, -1});
    ContactManifoldSet tilted_set = contact_manifold_set_polygon_get(plane, tilted);
    assert(tilted_set.count == 1 && tilted_set.values[0].count == 2);
    float low = fminf(tilted_set.values[0].point_depths[0], tilted_set.values[0].point_depths[1]);
    float high = fmaxf(tilted_set.values[0].point_depths[0], tilted_set.values[0].point_depths[1]);
    assert(fabsf(low - (.0001f - sinf(.02f * 3.14159265359f / 180))) < .000001f);
    assert(fabsf(high - (.0001f + sinf(.02f * 3.14159265359f / 180))) < .000001f);
    subdivided_faces_test();
    maximum_face_test();
    corner_tolerance_test(40, .00001f, true);
    corner_tolerance_test(40, .00008f, false);
    corner_tolerance_test(10000, .002f, false);
    corner_tolerance_test(40, .02f, false);
    Shape first = {
        .amount_of_vertices = 4,
        .vertices = {{-1.0f, -1.0f}, {1.0f, -1.0f},
            {1.0f, 1.0f}, {-1.0f, 1.0f}}
    };
    Shape second = {
        .amount_of_vertices = 4,
        .vertices = {{0.5f, -1.0f}, {2.5f, -1.0f},
            {2.5f, 1.0f}, {0.5f, 1.0f}}
    };
    ContactManifold manifold = contact_manifold_polygon_get(
        first, second, (Axis){1.0f, 0.0f});
    Shape concave = {
        .amount_of_vertices = 8,
        .vertices = {
            {-20.0f, -20.0f}, {-5.0f, -20.0f}, {-5.0f, -5.0f},
            {5.0f, -5.0f}, {5.0f, -20.0f}, {20.0f, -20.0f},
            {20.0f, 20.0f}, {-20.0f, 20.0f}
        }
    };
    Shape corner = {
        .amount_of_vertices = 4,
        .vertices = {
            {-7.0f, -7.0f}, {-3.0f, -7.0f},
            {-3.0f, -3.0f}, {-7.0f, -3.0f}
        }
    };
    ContactManifoldSet set;

    if(manifold.count != 2) return 1;
    for(uint8_t i = 0; i < manifold.count; i += 1) {
        if(fabsf(manifold.points[i].x - 0.75f) > 0.0001f ||
                fabsf(fabsf(manifold.points[i].y) - 1.0f) > 0.0001f) return 1;
    }
    set = contact_manifold_set_polygon_get(concave, corner);
    if(set.count < 2) return 2;
    if(fabsf(math_dot_product(set.values[0].normal,
            set.values[1].normal)) > 0.1f) return 3;
    return 0;
}
