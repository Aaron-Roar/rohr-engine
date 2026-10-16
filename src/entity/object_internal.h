/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_OBJECT_INTERNAL_H
#define ROHR_OBJECT_INTERNAL_H
#include "object.h"

#define OBJECT_DEPTH_MAX 128
bool object_flags_check(EntityObjectFlags flags);
void object_instance_resources_clear(EntityIndex index);
EngineResult object_instance_camera_restore(Entity entity, CameraId camera);
Entity object_instance_anchor_owner_get(Entity entity);
EngineResult object_instance_anchor_restore(Entity entity, Entity owner, JointAnchorId anchor);
typedef struct ObjectAppearance {
    ObjectKind kind;
    Color color;
    Color border_color;
    bool visible;
    bool enabled;
} ObjectAppearance;
bool object_instance_appearance_get(Entity entity, ObjectAppearance *appearance);
EngineResult object_instance_appearance_set(Entity entity, ObjectAppearance appearance);
void object_description_tree_destroy(ObjectDescription *description);
ObjectDescriptionResult object_description_register(ObjectDescription *root,
    char *path, bool editable);
ObjectDescriptionResult object_description_file_read(const char *path);
struct EditorProject;
ObjectDescriptionResult object_description_project_read(const struct EditorProject *project,
    const char *path);
char *object_string_copy(const char *text);
EngineResult object_description_world_pose_get(const ObjectDescription *description,
    Position *position, Orientation *orientation);
#endif
