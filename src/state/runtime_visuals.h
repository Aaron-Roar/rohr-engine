/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_RUNTIME_VISUALS_H
#define ROHR_RUNTIME_VISUALS_H
#include "entity_components.h"
#include "yyjson/yyjson.h"
bool state_visuals_write(yyjson_mut_doc *doc, yyjson_mut_val *components, Entity entity);
EngineResult state_visuals_load(Entity entity, yyjson_val *components);
#endif
