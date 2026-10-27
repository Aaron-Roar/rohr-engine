/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef DROP_FIXTURE_H
#define DROP_FIXTURE_H

#include "rohr.h"

#define DROP_BODY_COUNT 3
#define DROP_PAIR_COUNT 3
#define DROP_PENETRATION_LIMIT 0.0001

typedef struct DropBody {
    Entity entity;
    Position position;
    Velocity velocity;
    float angle;
    float angular_velocity;
    double half_width;
    double half_height;
} DropBody;

typedef struct DropFixture {
    DropBody bodies[DROP_BODY_COUNT]; /* Floor, tiny body, large body; owned. */
    double depths[DROP_PAIR_COUNT]; /* Floor/tiny, tiny/large, floor/large. */
    double maximum_depth;
    double first_violation_time;
    double time;
    unsigned steps;
    unsigned rate;
    unsigned case_index;
    bool halted;
    bool floor_moved;
    char failure[160];
} DropFixture;

bool drop_result_check(EngineResult result);
void drop_fixture_destroy(DropFixture *fixture);
bool drop_fixture_reset(DropFixture *fixture, unsigned case_index, unsigned rate);
bool drop_fixture_step(DropFixture *fixture);
void drop_fixture_print(const DropFixture *fixture);
double drop_rect_depth_get(const DropBody *a, const DropBody *b);
bool drop_fixture_verify(void);

#endif
