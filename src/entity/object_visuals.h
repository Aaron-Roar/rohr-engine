/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_OBJECT_VISUALS_H
#define ROHR_OBJECT_VISUALS_H
#include "object.h"
#include "yyjson/yyjson.h"
ObjectVisualStyle object_visual_style_get(ObjectVisualStyle value, ObjectKind kind);
bool object_visuals_check(ObjectVisuals value);
yyjson_mut_val *object_visuals_json_write(yyjson_mut_doc *doc, ObjectVisuals value);
bool object_visuals_json_read(yyjson_val *value, ObjectVisuals *output);
#endif
