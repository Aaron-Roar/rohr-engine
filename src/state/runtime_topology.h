/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_RUNTIME_TOPOLOGY_H
#define ROHR_RUNTIME_TOPOLOGY_H
#include "physics.h"
#include "yyjson/yyjson.h"

/* Passes: owners, nodes, beams/areas/anchors, ordered membership. */
EngineResult state_topology_load(Entity entity, yyjson_val *components,
    unsigned pass, const Entity *created, size_t count);
bool state_topology_write(yyjson_mut_doc *doc, yyjson_mut_val *components, Entity entity);
/* Resolve animation drivers only after every animation component is loaded. */
EngineResult state_hitbox_animation_load(Entity entity, yyjson_val *components);
bool state_joint_anchor_read(yyjson_val *value, Entity owner, JointAnchorId *anchor);
bool state_joint_anchor_write(yyjson_mut_doc *doc, yyjson_mut_val *object,
    const char *key, Entity owner, JointAnchorId anchor);
#endif
