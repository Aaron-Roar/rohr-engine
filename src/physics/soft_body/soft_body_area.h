/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_SOFT_BODY_AREA_H
#define ROHR_SOFT_BODY_AREA_H

#include "physics.h"
#include "math/node_loop_geometry.h"

typedef struct SoftBodyAreaCache {
    NodeLoopPoints poses[NODE_LOOP_MAX_LOOPS];
    NodeLoopFill fill;
} SoftBodyAreaCache;

typedef struct SoftBodyAreaState {
    SoftBodyArea value;
    SoftBodyAreaCache *cache;
} SoftBodyAreaState;

MEMORY_DECLARE_OBJECT_POOL(SoftBodyAreaStatePool, SoftBodyAreaState);
extern SoftBodyAreaStatePool soft_body_area_states_pool;
#define soft_body_area_states (soft_body_area_states_pool.objects)

EngineResult physics_soft_body_area_index_get(Entity area, EntityIndex *index);
EngineResult physics_soft_body_area_fill_get(Entity area, const NodeLoopFill **fill);
void physics_soft_body_area_tables_clear(void);
void physics_soft_body_area_entity_clear(Entity area, EntityIndex index);
void physics_soft_body_areas_node_clear(Entity node, Entity owner);

#endif
