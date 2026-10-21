/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_RUNTIME_VISUALS_H
#define ROHR_RUNTIME_VISUALS_H
#include "entity_components.h"
#include "graphics.h"
#include "yyjson/yyjson.h"
bool state_visuals_write(yyjson_mut_doc *doc, yyjson_mut_val *components, Entity entity);
EngineResult state_visuals_load(Entity entity, yyjson_val *components);
EngineResult state_layers_load(yyjson_val *root, GraphicsLayerId *created, size_t *created_count);
bool state_layers_write(yyjson_mut_doc *doc, yyjson_mut_val *root);
bool state_layers_template_copy(yyjson_mut_doc *doc, yyjson_mut_val *layers, yyjson_val *root);
#endif
