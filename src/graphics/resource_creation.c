/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "graphics.h"
#include "entity/object_internal.h"

CameraIdResult rohr_internal_camera_create_recipe(const ObjectDescription *d) {
    if(!d || d->kind != OBJECT_CAMERA)
        return ERROR_RESULT_MAKE_ERROR(CameraIdResult, ERROR_ENGINE_STATE_INVALID);
    Position position; Orientation orientation;
    EngineResult pose = object_description_world_pose_get(d, &position, &orientation);
    if(error_check(pose)) return ERROR_RESULT_MAKE_ERROR(CameraIdResult, pose.result.error);
    bool visible = true;
    for(const ObjectDescription *p = d; p; p = p->parent) visible = visible && p->visible;
    return graphics_camera_create((CameraDescription){.position=position,
        .orientation=orientation, .dimensions={d->size.x,d->size.y}, .zoom=d->zoom,
        .enabled=visible, .pause_with_engine=d->pause_with_engine});
}
TextureAssetResult rohr_internal_texture_load_recipe(const ObjectDescription *d) {
    if(!d || d->kind != OBJECT_SPRITE)
        return ERROR_RESULT_MAKE_ERROR(TextureAssetResult, ERROR_ENGINE_STATE_INVALID);
    return graphics_texture_load((TextureDescription){.file=d->asset_path, .size=d->size});
}
AnimationAssetResult rohr_internal_animation_load_recipe(const ObjectDescription *d) {
    if(!d || d->kind != OBJECT_ANIMATION || (d->frame_count && !d->frames) ||
            d->frame_count > MAX_ANIMATIONS_FRAMES)
        return ERROR_RESULT_MAKE_ERROR(AnimationAssetResult, ERROR_ENGINE_STATE_INVALID);
    AnimationDescription native = {.id=(AnimationId)d->id, .frame_count=d->frame_count,
        .ticks_per_frame=d->playing ? d->ticks_per_frame : 0,
        .time_per_frame=d->playing ? d->time_per_frame : 0};
    for(size_t i=0; i<d->frame_count; ++i) {
        native.frame_ids[i]=d->frames[i].id;
        native.frame_files[i]=d->frames[i].path;
    }
    return graphics_animation_load(native);
}
