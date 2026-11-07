/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "viewport_creation.h"
#include "entity/object_internal.h"
#include <string.h>
#include <stdio.h>

/* Creation-time inventory; drawing never allocates or traverses this storage.
 * Removed recipe items retain their resources until the viewport is destroyed. */
typedef struct ViewportResources {
    ViewportId viewport;
    struct {
        char name[ENTITY_NAME_MAX];
        ViewportItemId item;
        ScreenId screen;
        GraphicsUiId ui;
        TextAsset text;
        FontAsset font;
    } items[MAX_VIEWPORT_ITEMS];
    size_t count;
    ScreenId added_screens[MAX_SCREENS];
    size_t added_count;
} ViewportResources;
static ViewportResources resources[MAX_VIEWPORTS];

static ViewportResources *resources_get(ViewportId viewport) {
    if(viewport) for(size_t i = 0; i < MAX_VIEWPORTS; ++i)
        if(resources[i].viewport == viewport) return &resources[i];
    return NULL;
}
bool graphics_viewport_recipe_check(ViewportId viewport) { return resources_get(viewport) != NULL; }
void graphics_viewport_resources_release(ViewportId viewport) {
    ViewportResources *owned = resources_get(viewport);
    if(!owned) return;
    for(size_t i = 0; i < owned->count; ++i) {
        if(owned->items[i].screen) (void)graphics_screen_destroy(owned->items[i].screen);
        if(owned->items[i].ui) graphics_ui_owned_destroy(owned->items[i].ui);
    }
    for(size_t i = 0; i < owned->added_count; ++i)
        (void)graphics_screen_destroy(owned->added_screens[i]);
    for(size_t i = 0; i < owned->count; ++i) (void)graphics_text_destroy(&owned->items[i].text);
    for(size_t i = 0; i < owned->count; ++i) (void)graphics_font_release(&owned->items[i].font);
    *owned = (ViewportResources){0};
}
ViewportItemIdResult graphics_viewport_item_by_name_get(ViewportId viewport, const char *name) {
    ViewportResources *owned = resources_get(viewport);
    if(!owned || !name) return ERROR_RESULT_MAKE_ERROR(ViewportItemIdResult, ERROR_ENGINE_STATE_INVALID);
    for(size_t i = 0; i < owned->count; ++i) {
        if(strcmp(owned->items[i].name, name)) continue;
        ViewportIdResult mounted = graphics_viewport_item_viewport_get(owned->items[i].item);
        if(!error_check(mounted) && mounted.result.value == viewport)
            return ERROR_RESULT_MAKE_VALUE(ViewportItemIdResult, owned->items[i].item);
        break;
    }
    return ERROR_RESULT_MAKE_ERROR(ViewportItemIdResult, ERROR_ENGINE_STATE_REFERENCE_NOT_FOUND);
}
ViewportItemIdResult graphics_viewport_owned_camera_add(ViewportId viewport, CameraId camera,
        ViewportItemDescription placement) {
    ViewportResources *owned = resources_get(viewport);
    if(!owned) return ERROR_RESULT_MAKE_ERROR(ViewportItemIdResult, ERROR_ENGINE_STATE_INVALID);
    /* Only reuse an automatic screen in this owner's inventory, with matching dimensions. */
    for(size_t i = 0; i < owned->count + owned->added_count; ++i) {
        ScreenId screen = i < owned->count ? owned->items[i].screen : owned->added_screens[i - owned->count];
        ScreenDescriptionResult existing = graphics_screen_description_get(screen);
        if(!error_check(existing) && existing.result.value.camera == camera &&
                existing.result.value.content_mode == SCREEN_CONTENT_AUTOMATIC &&
                existing.result.value.width == WINDOW_WIDTH && existing.result.value.height == WINDOW_HEIGHT)
            return graphics_viewport_screen_add(viewport, screen, placement);
    }
    if(owned->added_count == MAX_SCREENS)
        return ERROR_RESULT_MAKE_ERROR(ViewportItemIdResult, ERROR_MEMORY_POOL_FULL);
    ScreenIdResult screen = graphics_screen_create((ScreenDescription){.camera = camera,
        .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT, .content_mode = SCREEN_CONTENT_AUTOMATIC,
        .background_color = {0,0,0,255}});
    if(error_check(screen)) return ERROR_RESULT_MAKE_ERROR(ViewportItemIdResult, screen.result.error);
    ViewportItemIdResult item = graphics_viewport_screen_add(viewport, screen.result.value, placement);
    if(error_check(item)) (void)graphics_screen_destroy(screen.result.value);
    else owned->added_screens[owned->added_count++] = screen.result.value;
    return item;
}

static bool name_check(const char *name) { return name && name[0] && strlen(name) < ENTITY_NAME_MAX; }
static bool camera_member_check(const ObjectDescription *object, const ObjectDescription *member) {
    if(!object || !member || member->kind != OBJECT_CAMERA) return false;
    for(size_t depth = 0; depth <= OBJECT_DEPTH_MAX; ++depth) {
        if(!name_check(member->name) || !member->id) return false;
        if(member == object) return true;
        const ObjectDescription *parent = member->parent;
        if(!parent || parent->child_count > MAX_ENTITIES ||
                (parent->child_count && !parent->children)) return false;
        size_t matches = 0;
        for(size_t i = 0; i < parent->child_count; ++i) {
            const ObjectDescription *child = parent->children[i];
            if(!child || !name_check(child->name)) return false;
            if(child == member) ++matches;
            else if(child->id == member->id || !strcmp(child->name, member->name)) return false;
        }
        if(matches != 1) return false;
        member = parent;
    }
    return false;
}
static EntityResult member_resolve(Entity instance, const ObjectDescription *object,
        const ObjectDescription *member) {
    const ObjectDescription *path[OBJECT_DEPTH_MAX];
    size_t count = 0;
    const ObjectDescription *step = member;
    if(!object_instance_description_check(instance, object)) goto invalid;
    while(step && step != object && count < OBJECT_DEPTH_MAX) {
        if(!name_check(step->name)) goto invalid;
        path[count++] = step;
        step = step->parent;
    }
    if(step != object) goto invalid;
    while(count) {
        step = path[--count];
        EntityResult found = object_member_get(instance, step->name);
        if(error_check(found)) return found;
        instance = found.result.value;
        if(!object_instance_description_check(instance, step)) goto invalid;
    }
    return ERROR_RESULT_MAKE_VALUE(EntityResult, instance);
invalid:
    return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_REFERENCE_NOT_FOUND);
}

ViewportIdResult graphics_viewport_create_description(const ViewportCreateDescription *input) {
    if(!input) return ERROR_RESULT_MAKE_ERROR(ViewportIdResult, ERROR_ENGINE_STATE_INVALID);
    const ViewportCreateDescription *d = input->template ? input->template : input;
    if(d->template || (input->template && (d->object_count || d->camera_count)) ||
            d->item_count > MAX_VIEWPORT_ITEMS || (d->item_count && !d->items) ||
            d->layer_count > MAX_GRAPHICS_LAYERS || (d->layer_count && !d->layers) ||
            input->object_count > MAX_VIEWPORT_ITEMS || (input->object_count && !input->objects) ||
            input->camera_count > MAX_VIEWPORT_ITEMS || (input->camera_count && !input->cameras))
        return ERROR_RESULT_MAKE_ERROR(ViewportIdResult, ERROR_ENGINE_STATE_INVALID);
    ScreenDescription screens[MAX_VIEWPORT_ITEMS] = {0};
    bool objects_used[MAX_VIEWPORT_ITEMS] = {0}, cameras_used[MAX_VIEWPORT_ITEMS] = {0};
    EngineError failure = ERROR_ENGINE_STATE_INVALID;
    const char *failure_item = NULL;
    for(size_t b = 0; b < input->object_count; ++b) {
        const ViewportObjectBinding *binding = &input->objects[b];
        if(!binding->description || !object_instance_description_check(binding->instance, binding->description)) goto invalid;
        for(size_t prior = 0; prior < b; ++prior)
            if(input->objects[prior].description == binding->description) goto invalid;
    }
    for(size_t b = 0; b < input->camera_count; ++b) {
        const ViewportCameraBinding *binding = &input->cameras[b];
        if(!name_check(binding->item_name) || (!!binding->entity == !!binding->camera)) goto invalid;
        for(size_t prior = 0; prior < b; ++prior)
            if(!strcmp(input->cameras[prior].item_name, binding->item_name)) goto invalid;
    }
    for(size_t i = 0; i < d->item_count; ++i) {
        const ViewportCreateItemDescription *item = &d->items[i];
        failure_item = item->name;
        if(!name_check(item->name) || item->kind < VIEWPORT_CREATE_SCREEN || item->kind > VIEWPORT_CREATE_SLIDER ||
                item->placement.screen || item->placement.ui || item->placement.layer_id ||
                item->shape.text.text.handle || item->text.text.handle) goto invalid;
        for(size_t prior = 0; prior < i; ++prior)
            if(!strcmp(d->items[prior].name, item->name)) goto invalid;
        if(item->kind != VIEWPORT_CREATE_SCREEN) {
            if(item->camera_object || item->camera_member) goto invalid;
            continue;
        }
        screens[i] = (ScreenDescription){.width = item->render_width, .height = item->render_height,
            .content_mode = SCREEN_CONTENT_AUTOMATIC, .background_color = item->background_color};
        if(item->render_width < 0 || item->render_height < 0 ||
                (!!item->camera_object != !!item->camera_member) ||
                (item->camera_member && !camera_member_check(item->camera_object, item->camera_member))) goto invalid;
        const ViewportObjectBinding *object = NULL;
        for(size_t b = 0; b < input->object_count; ++b)
            if(input->objects[b].description == item->camera_object) { object = &input->objects[b]; objects_used[b] = true; }
        const ViewportCameraBinding *override = NULL;
        for(size_t b = 0; b < input->camera_count; ++b)
            if(!strcmp(input->cameras[b].item_name, item->name)) { override = &input->cameras[b]; cameras_used[b] = true; }
        Entity entity = ENTITY_INVALID;
        if(override) {
            entity = override->entity;
            screens[i].camera = override->camera;
        } else if(item->camera_member && object) {
            EntityResult member = member_resolve(object->instance, item->camera_object, item->camera_member);
            if(error_check(member)) { failure = member.result.error; goto invalid; }
            entity = member.result.value;
        }
        if(entity) {
            CameraIdResult camera = entity_object_camera_get(entity);
            if(error_check(camera)) { failure = camera.result.error; goto invalid; }
            screens[i].camera = camera.result.value;
            screens[i].camera_entity = entity;
        }
        if((override || object) && error_check(graphics_camera_get(screens[i].camera))) goto invalid;
    }
    for(size_t b = 0; b < input->object_count; ++b) if(!objects_used[b]) goto invalid;
    for(size_t b = 0; b < input->camera_count; ++b) if(!cameras_used[b]) goto invalid;
    for(size_t i = 0; i < d->layer_count; ++i) {
        if(!d->layers[i].name || !d->layers[i].name[0] || strlen(d->layers[i].name) >= GRAPHICS_LAYER_NAME_MAX) goto invalid;
        for(size_t j = 0; j < i; ++j) if(!strcmp(d->layers[i].name, d->layers[j].name)) goto invalid;
        GraphicsLayerIdResult found = graphics_layer_name_id_get(d->layers[i].name);
        if(!error_check(found) && graphics_layer_get(found.result.value).result.value != d->layers[i].value) {
            failure = ERROR_ENGINE_DUPLICATE_GRAPHICS_LAYER_NAME; goto invalid;
        }
    }
    ViewportResources *owned = NULL;
    for(size_t i = 0; i < MAX_VIEWPORTS; ++i) if(!resources[i].viewport) { owned = &resources[i]; break; }
    if(!owned) { failure = ERROR_MEMORY_POOL_FULL; goto invalid; }
    ViewportIdResult created = (graphics_viewport_create)(d->viewport);
    if(error_check(created)) return created;
    owned->viewport = created.result.value;
    GraphicsLayerId introduced[MAX_GRAPHICS_LAYERS]; size_t introduced_count = 0;
    for(size_t i = 0; i < d->layer_count; ++i) {
        GraphicsLayerIdResult layer = graphics_layer_name_id_get(d->layers[i].name);
        if(error_check(layer)) {
            layer = graphics_layer_create(d->layers[i]);
            if(error_check(layer)) { failure = layer.result.error; goto rollback; }
            introduced[introduced_count++] = layer.result.value;
        }
    }
    for(size_t i = 0; i < d->item_count; ++i) {
        const ViewportCreateItemDescription *item = &d->items[i];
        failure_item = item->name;
        owned->count = i + 1;
        memcpy(owned->items[i].name, item->name, strlen(item->name) + 1);
        ViewportItemDescription placement = item->placement;
        if(item->layer_name && item->layer_name[0]) {
            GraphicsLayerIdResult layer = graphics_layer_name_id_get(item->layer_name);
            if(error_check(layer)) { failure = layer.result.error; goto rollback; }
            placement.layer_id = layer.result.value;
        }
        if(item->kind == VIEWPORT_CREATE_SCREEN) {
            ScreenIdResult screen = graphics_screen_create(screens[i]);
            if(error_check(screen)) { failure = screen.result.error; goto rollback; }
            owned->items[i].screen = screen.result.value;
            placement.screen = screen.result.value;
        } else {
            if(item->text_value) {
                FontAssetResult font = graphics_font_load(item->font);
                if(error_check(font)) { failure = font.result.error; goto rollback; }
                owned->items[i].font = font.result.value;
                TextAssetResult text = graphics_text_create((TextDescription){.font = font.result.value,
                    .value = item->text_value, .color = item->text_color, .wrap_width = item->text_wrap_width});
                if(error_check(text)) { failure = text.result.error; goto rollback; }
                owned->items[i].text = text.result.value;
            }
            GraphicsUiIdResult ui;
            if(item->kind == VIEWPORT_CREATE_SHAPE) {
                ViewportUiShapeDescription shape = item->shape; shape.text.text = owned->items[i].text;
                ui = graphics_ui_shape_create(shape);
            } else if(item->kind == VIEWPORT_CREATE_TEXT) {
                ViewportUiTextDescription text = item->text; text.text = owned->items[i].text;
                ui = graphics_ui_text_create(text);
            } else ui = graphics_ui_slider_create(item->slider);
            if(error_check(ui)) { failure = ui.result.error; goto rollback; }
            owned->items[i].ui = ui.result.value;
            placement.ui = ui.result.value;
        }
        ViewportItemIdResult mounted = graphics_viewport_item_add(created.result.value, placement);
        if(error_check(mounted)) { failure = mounted.result.error; goto rollback; }
        owned->items[i].item = mounted.result.value;
    }
    return created;
rollback:
    (void)graphics_viewport_destroy(created.result.value);
    for(size_t i = 0; i < introduced_count; ++i) (void)graphics_layer_destroy(introduced[i]);
invalid:
    if(failure_item) {
        char detail[512];
        snprintf(detail, sizeof(detail), "Viewport item '%s': %s", failure_item,
            error_result_message_get(ERROR_RESULT_ERROR, failure));
        error_detail_set(failure, detail);
    }
    return ERROR_RESULT_MAKE_ERROR(ViewportIdResult, failure);
}
