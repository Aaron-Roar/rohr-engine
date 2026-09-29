/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "graphics/animation_assets.h"

#include <math.h>
#include <string.h>

#define ANIMATION_HANDLE_SLOT_MASK UINT32_C(0xffff)
#define ANIMATION_HANDLE_GENERATION_SHIFT 16

typedef struct GraphicsAnimationResource {
    AnimationAsset asset;
    AnimationInfo info;
    AnimationFrame frames[MAX_ANIMATIONS_FRAMES];
    size_t references;
    bool used;
} GraphicsAnimationResource;

static GraphicsAnimationResource animation_resources[MAX_ANIMATION_ASSETS];
static uint16_t animation_generations[MAX_ANIMATION_ASSETS];
static bool animation_assets_initialized;

static AnimationHandle graphics_animation_handle_create(size_t slot) {
    return ((uint32_t)animation_generations[slot]
        << ANIMATION_HANDLE_GENERATION_SHIFT) | (uint32_t)(slot + 1);
}

static GraphicsAnimationResource *graphics_animation_resource_get(
        AnimationHandle handle) {
    uint32_t stored_slot;
    size_t slot;

    if(handle == ANIMATION_HANDLE_INVALID) return NULL;
    stored_slot = handle & ANIMATION_HANDLE_SLOT_MASK;
    if(stored_slot == 0) return NULL;
    slot = (size_t)(stored_slot - 1);
    if(slot >= MAX_ANIMATION_ASSETS || !animation_resources[slot].used ||
            animation_resources[slot].asset.handle != handle ||
            animation_resources[slot].references == 0) return NULL;
    return &animation_resources[slot];
}

static void graphics_animation_frames_release(AnimationFrame *frames,
        size_t frame_count) {
    for(size_t i = 0; i < frame_count; i += 1)
        (void)graphics_texture_release(&frames[i].texture);
}

static void graphics_animation_resource_destroy(
        GraphicsAnimationResource *resource) {
    if(resource == NULL || !resource->used) return;
    graphics_animation_frames_release(resource->frames,
        resource->info.frame_count);
    *resource = (GraphicsAnimationResource){0};
}

static bool graphics_animation_resource_matches(
        const GraphicsAnimationResource *resource, AnimationId id,
        const AnimationFrame *frames, size_t frame_count,
        Tick ticks_per_frame, Time time_per_frame) {
    if(resource == NULL || !resource->used || resource->references == 0 ||
            resource->info.id != id ||
            resource->info.frame_count != frame_count ||
            resource->info.ticks_per_frame != ticks_per_frame ||
            resource->info.time_per_frame != time_per_frame) return false;
    for(size_t i = 0; i < frame_count; i += 1) {
        if(resource->frames[i].id != frames[i].id ||
                resource->frames[i].texture.handle != frames[i].texture.handle ||
                resource->frames[i].texture.size.x != frames[i].texture.size.x ||
                resource->frames[i].texture.size.y != frames[i].texture.size.y)
            return false;
    }
    return true;
}

EngineResult graphics_animation_assets_init(void) {
    if(animation_assets_initialized) return error_result_value(true);
    memset(animation_resources, 0, sizeof(animation_resources));
    animation_assets_initialized = true;
    return error_result_value(true);
}

void graphics_animation_assets_clear(void) {
    for(size_t i = 0; i < MAX_ANIMATION_ASSETS; i += 1)
        graphics_animation_resource_destroy(&animation_resources[i]);
}

void graphics_animation_assets_destroy(void) {
    graphics_animation_assets_clear();
    animation_assets_initialized = false;
}

GraphicsAnimationAssetStats graphics_animation_assets_stats_get(void) {
    GraphicsAnimationAssetStats stats = {0};
    for(size_t slot = 0; slot < MAX_ANIMATION_ASSETS; slot += 1) {
        if(!animation_resources[slot].used) continue;
        stats.live_resources += 1;
        stats.owner_references += animation_resources[slot].references;
        stats.frame_texture_references +=
            animation_resources[slot].info.frame_count;
    }
    return stats;
}

AnimationAssetResult graphics_animation_load(AnimationDescriptor descriptor) {
    AnimationFrame frames[MAX_ANIMATIONS_FRAMES] = {0};
    AnimationId id;
    size_t frame_count = descriptor.amount_of_descriptors;
    size_t slot;

    if(!animation_assets_initialized || frame_count == 0 ||
            frame_count > MAX_ANIMATIONS_FRAMES ||
            !isfinite(descriptor.time_per_frame) ||
            descriptor.time_per_frame < 0.0)
        return ERROR_RESULT_MAKE_ERROR(
            AnimationAssetResult, ERROR_ENGINE_ANIMATION_LOAD_FAILED);
    id = descriptor.id == 0 ? 1 : descriptor.id;
    for(size_t i = 0; i < frame_count; i += 1) {
        TextureAssetResult texture;
        frames[i].scale = (Scale){1, 1};
        frames[i].id = descriptor.frame_ids[i] == 0 ?
            (AnimationFrameId)i + 1 : descriptor.frame_ids[i];
        for(size_t previous = 0; previous < i; previous += 1) {
            if(frames[previous].id == frames[i].id) {
                graphics_animation_frames_release(frames, i);
                return ERROR_RESULT_MAKE_ERROR(
                    AnimationAssetResult, ERROR_ENGINE_ANIMATION_LOAD_FAILED);
            }
        }
        texture = graphics_texture_load((TextureDescriptor){.file = descriptor.frame_files[i]});
        if(texture.kind == ERROR_RESULT_ERROR) {
            graphics_animation_frames_release(frames, i);
            return ERROR_RESULT_MAKE_ERROR(
                AnimationAssetResult, texture.result.error);
        }
        frames[i].texture = texture.result.value;
        frames[i].texture.size = graphics_texture_size_get(texture.result.value).result.value;
    }
    for(slot = 0; slot < MAX_ANIMATION_ASSETS; slot += 1) {
        GraphicsAnimationResource *resource = &animation_resources[slot];
        if(!graphics_animation_resource_matches(resource, id, frames,
                frame_count, descriptor.ticks_per_frame,
                descriptor.time_per_frame)) continue;
        if(resource->references == SIZE_MAX) {
            graphics_animation_frames_release(frames, frame_count);
            return ERROR_RESULT_MAKE_ERROR(AnimationAssetResult,
                ERROR_ENGINE_ANIMATION_CAPACITY_EXCEEDED);
        }
        resource->references += 1;
        graphics_animation_frames_release(frames, frame_count);
        return ERROR_RESULT_MAKE_VALUE(AnimationAssetResult, resource->asset);
    }
    for(slot = 0; slot < MAX_ANIMATION_ASSETS; slot += 1)
        if(!animation_resources[slot].used) break;
    if(slot == MAX_ANIMATION_ASSETS) {
        graphics_animation_frames_release(frames, frame_count);
        return ERROR_RESULT_MAKE_ERROR(AnimationAssetResult,
            ERROR_ENGINE_ANIMATION_CAPACITY_EXCEEDED);
    }
    animation_generations[slot] += 1;
    if(animation_generations[slot] == 0) animation_generations[slot] = 1;
    animation_resources[slot] = (GraphicsAnimationResource){
        .asset = {.handle = graphics_animation_handle_create(slot)},
        .info = {
            .id = id,
            .frame_count = frame_count,
            .ticks_per_frame = descriptor.ticks_per_frame,
            .time_per_frame = descriptor.time_per_frame,
        },
        .references = 1,
        .used = true,
    };
    memcpy(animation_resources[slot].frames, frames,
        frame_count * sizeof(*frames));
    return ERROR_RESULT_MAKE_VALUE(AnimationAssetResult,
        animation_resources[slot].asset);
}

EngineResult graphics_animation_retain(AnimationAsset asset) {
    GraphicsAnimationResource *resource =
        graphics_animation_resource_get(asset.handle);
    if(resource == NULL)
        return error_result_error(ERROR_ENGINE_ANIMATION_NOT_FOUND);
    if(resource->references == SIZE_MAX)
        return error_result_error(ERROR_ENGINE_ANIMATION_CAPACITY_EXCEEDED);
    resource->references += 1;
    return error_result_value(true);
}

EngineResult graphics_animation_release(AnimationAsset *asset) {
    GraphicsAnimationResource *resource;

    if(asset == NULL)
        return error_result_error(ERROR_ENGINE_ANIMATION_NOT_FOUND);
    if(asset->handle == ANIMATION_HANDLE_INVALID) {
        *asset = (AnimationAsset){0};
        return error_result_value(true);
    }
    resource = graphics_animation_resource_get(asset->handle);
    if(resource == NULL)
        return error_result_error(ERROR_ENGINE_ANIMATION_NOT_FOUND);
    resource->references -= 1;
    *asset = (AnimationAsset){0};
    if(resource->references == 0) graphics_animation_resource_destroy(resource);
    return error_result_value(true);
}

bool graphics_animation_valid_check(AnimationAsset asset) {
    return graphics_animation_resource_get(asset.handle) != NULL;
}

AnimationInfoResult graphics_animation_info_get(AnimationAsset asset) {
    GraphicsAnimationResource *resource =
        graphics_animation_resource_get(asset.handle);
    if(resource == NULL)
        return ERROR_RESULT_MAKE_ERROR(
            AnimationInfoResult, ERROR_ENGINE_ANIMATION_NOT_FOUND);
    return ERROR_RESULT_MAKE_VALUE(AnimationInfoResult, resource->info);
}

AnimationFrameResult graphics_animation_frame_get(AnimationAsset asset,
        size_t frame_index) {
    GraphicsAnimationResource *resource =
        graphics_animation_resource_get(asset.handle);
    if(resource == NULL)
        return ERROR_RESULT_MAKE_ERROR(
            AnimationFrameResult, ERROR_ENGINE_ANIMATION_NOT_FOUND);
    if(frame_index >= resource->info.frame_count)
        return ERROR_RESULT_MAKE_ERROR(
            AnimationFrameResult, ERROR_ENGINE_INDEX_OUT_OF_RANGE);
    return ERROR_RESULT_MAKE_VALUE(
        AnimationFrameResult, resource->frames[frame_index]);
}
