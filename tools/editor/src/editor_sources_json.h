/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_EDITOR_SOURCES_JSON_H
#define ROHR_EDITOR_SOURCES_JSON_H
#include "editor_project.h"
#include "yyjson/yyjson.h"
yyjson_mut_val *editor_force_json_write(yyjson_mut_doc *doc, ObjectForceDescription value);
yyjson_mut_val *editor_torque_json_write(yyjson_mut_doc *doc, ObjectTorqueDescription value);
bool editor_force_json_read(yyjson_val *json, ObjectForceDescription *value);
bool editor_torque_json_read(yyjson_val *json, ObjectTorqueDescription *value);
bool editor_sources_json_write(yyjson_mut_doc *doc, yyjson_mut_val *json, const EditorObject *object);
bool editor_sources_json_read(yyjson_val *json, EditorObject *object, EditorProject *project);
void editor_entity_settings_json_write(yyjson_mut_doc *doc, yyjson_mut_val *json,
    ObjectEntitySettings value);
bool editor_entity_settings_json_read(yyjson_val *json, ObjectEntitySettings *value);
#endif
