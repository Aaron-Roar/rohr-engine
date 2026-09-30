/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_SOFT_BODY_AREA_H
#define ROHR_SOFT_BODY_AREA_H

#include "physics.h"
#include "math/node_loop_geometry.h"

typedef struct SoftBodyAreaState {
    SoftBodyArea value;
    NodeLoopTriangulation triangulation;
} SoftBodyAreaState;

MEMORY_DECLARE_OBJECT_POOL(SoftBodyAreaStatePool, SoftBodyAreaState);
extern SoftBodyAreaStatePool soft_body_area_states_pool;
#define soft_body_area_states (soft_body_area_states_pool.objects)

EngineResult physics_soft_body_area_index_get(Entity area, EntityIndex *index);
void physics_soft_body_area_entity_clear(Entity area, EntityIndex index);
void physics_soft_body_areas_node_clear(Entity node, Entity owner);

#endif
