/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

#include <string.h>

static void count_camera_render(CameraId camera, void *context) {
    int *count = context;
    (void)camera;
    *count += 1;
    rohr_graphics_background_draw((Color){0, 0, 0, 255});
}

int main(void) {
    CameraConfig config = rohr_camera_config_default_get();
    CameraId original;
    CameraIdResult first_result;
    CameraIdResult second_result;
    CameraResult camera_result;
    ViewportIdResult viewport_result;
    ViewportIdResult second_viewport_result;
    ViewportItemIdResult viewport_item_result;
    ViewportItemIdResult ui_item_result;
    ViewportItemIdResult second_ui_item_result;
    GraphicsUiIdResult ui_result;
    GraphicsLayerIdResult world_layer_result;
    ViewportUiShapeConfig ui_shape = {
        .shape = {.amount_of_vertices = 4, .vertices = {
            {10.0f, 10.0f}, {110.0f, 10.0f},
            {110.0f, 70.0f}, {10.0f, 70.0f}}},
        .position = {5.0f, 8.0f},
        .orientation = 0.2f,
        .border_enabled = true,
        .border_type = VIEWPORT_UI_BORDER_HASHED,
        .border_thickness = 2.0f,
        .border_hash_spacing = 7.0f,
        .border_corner_radius = 9.0f,
        .border_color = {255, 255, 255, 255},
        .fill_color = {20, 40, 80, 255},
        .button_enabled = true,
        .hover_border_color = {255, 220, 80, 255},
        .hover_fill_color = {30, 60, 110, 255},
        .click_border_color = {255, 255, 255, 255},
        .click_fill_color = {10, 20, 50, 255},
    };
    FontAsset default_font;
    TextAssetResult default_text;
    Position screen;
    int render_count = 0;
    EntityResult target_entity_result;

    if(rohr_error_check(rohr_engine_init())) return 1;
    if(rohr_error_check(rohr_graphics_start())) {
        rohr_engine_shutdown();
        return 1;
    }
    {
        const char *missing_path = "/missing/custom-font-test.ttf";
        FontAssetResult missing = rohr_graphics_font_load((FontDescriptor){
            .file = missing_path, .point_size = 12.0f});
        if(!rohr_error_check(missing) ||
                strstr(rohr_error_message_get(missing), missing_path) == NULL) {
            rohr_graphics_end();
            rohr_engine_shutdown();
            return 1;
        }
    }
    rohr_graphics_layer_active_set(27);
    if(rohr_graphics_layer_active_get() != 27) {
        rohr_graphics_end();
        rohr_engine_shutdown();
        return 1;
    }
    world_layer_result = rohr_graphics_layer_create("world", 100);
    if(rohr_error_check(world_layer_result) ||
            !rohr_error_check(rohr_graphics_layer_create("world", 200)) ||
            rohr_error_check(rohr_graphics_layer_create("also-world", 100)) ||
            rohr_error_check(rohr_graphics_layer_name_set("world", 125)) ||
            rohr_error_check(rohr_graphics_layer_name_get("world")) ||
            rohr_graphics_layer_name_get("world").result.value != 125 ||
            rohr_error_check(rohr_graphics_layer_active_id_set(
                world_layer_result.result.value)) ||
            rohr_graphics_layer_active_get() != 125) {
        rohr_graphics_end();
        rohr_engine_shutdown();
        return 1;
    }
    rohr_graphics_layer_active_set(0);
    {
        GraphicsWindowPresentationConfig presentation =
            rohr_graphics_window_presentation_default_get();
        GraphicsWindowPresentationConfig active =
            rohr_graphics_window_presentation_get();
        if(active.mode != GRAPHICS_WINDOW_MODE_WINDOWED ||
                active.logical_width != WINDOW_WIDTH ||
                active.logical_height != WINDOW_HEIGHT) {
            rohr_graphics_end();
            rohr_engine_shutdown();
            return 1;
        }
        presentation.window_width = 640;
        presentation.window_height = 480;
        presentation.logical_width = 800;
        presentation.logical_height = 600;
        if(rohr_error_check(rohr_graphics_window_presentation_set(presentation))) {
            rohr_graphics_end();
            rohr_engine_shutdown();
            return 1;
        }
        active = rohr_graphics_window_presentation_get();
        if(active.mode != GRAPHICS_WINDOW_MODE_WINDOWED ||
                active.window_width <= 0 || active.window_height <= 0 ||
                active.logical_width != 800 || active.logical_height != 600 ||
                active.aspect_ratio_auto ||
                rohr_error_check(rohr_graphics_window_presentation_set(
                    rohr_graphics_window_presentation_default_get()))) {
            rohr_graphics_end();
            rohr_engine_shutdown();
            return 1;
        }
    }
    {
        EngineResult frame_limit_result = rohr_graphics_frame_limit_set(-1);
        if(!rohr_error_check(frame_limit_result) ||
                frame_limit_result.result.error != ERROR_ENGINE_INVALID_FRAME_LIMIT ||
                rohr_error_check(rohr_graphics_frame_limit_set(0))) {
            rohr_graphics_end();
            rohr_engine_shutdown();
            return 1;
        }
    }
    original = rohr_camera_active_get();
    config.position = (Position){10.0f, 20.0f};
    config.zoom = 2.0f;
    first_result = rohr_camera_create(config);
    second_result = rohr_camera_create(rohr_camera_config_default_get());
    if(rohr_error_check(first_result) || rohr_error_check(second_result) ||
            rohr_error_check(rohr_camera_active_set(first_result.result.value))) {
        rohr_engine_shutdown();
        return 1;
    }
    camera_result = rohr_camera_get(first_result.result.value);
    screen = rohr_graphics_world_to_screen_get((Position){11.0f, 20.0f});
    if(rohr_error_check(camera_result) || camera_result.result.value.zoom != 2.0f ||
            screen.x != WINDOW_WIDTH * 0.5f + 2.0f ||
            screen.y != WINDOW_HEIGHT * 0.5f ||
            !rohr_error_check(rohr_camera_destroy(first_result.result.value)) ||
            rohr_error_check(rohr_camera_active_set(second_result.result.value)) ||
            rohr_error_check(rohr_camera_destroy(first_result.result.value)) ||
            rohr_error_check(rohr_camera_active_set(original))) {
        rohr_graphics_end();
        rohr_engine_shutdown();
        return 1;
    }
    target_entity_result = rohr_entity_add();
    if(rohr_error_check(target_entity_result)
            || rohr_error_check(rohr_graphics_layer_entity_name_set(
                target_entity_result.result.value, "world"))
            || rohr_error_check(rohr_graphics_layer_entity_get(
                target_entity_result.result.value))
            || rohr_graphics_layer_entity_get(
                target_entity_result.result.value).result.value != 125
            || rohr_error_check(rohr_graphics_layer_entity_id_get(
                target_entity_result.result.value))
            || rohr_error_check(rohr_graphics_layer_sprite_set(
                target_entity_result.result.value, 225))
            || rohr_graphics_layer_sprite_get(
                target_entity_result.result.value).result.value != 225
            || rohr_error_check(rohr_graphics_layer_animation_name_set(
                target_entity_result.result.value, "world"))
            || rohr_graphics_layer_animation_get(
                target_entity_result.result.value).result.value != 125
            || rohr_graphics_layer_entity_get(
                target_entity_result.result.value).result.value != 125
            || rohr_error_check(rohr_graphics_layer_sprite_clear(
                target_entity_result.result.value))
            || rohr_error_check(rohr_graphics_layer_animation_clear(
                target_entity_result.result.value))
            || rohr_error_check(rohr_graphics_layer_entity_clear(
                target_entity_result.result.value))
            || !rohr_error_check(rohr_graphics_layer_entity_get(
                target_entity_result.result.value))
            || rohr_error_check(rohr_physics_position_set(
                target_entity_result.result.value,
                (Position){20.0f, 30.0f}
            ))
            || rohr_error_check(rohr_camera_position_from_entity_set(
                original,
                target_entity_result.result.value,
                0.0
            ))
            || rohr_graphics_camera_attachment_get().kind != ERROR_RESULT_ERROR
            || rohr_error_check(rohr_camera_entity_attachment_set(
                original,
                target_entity_result.result.value
            ))
            || rohr_graphics_camera_attachment_get().kind == ERROR_RESULT_ERROR
            || rohr_error_check(rohr_camera_position_move(
                original,
                (Vec2D){5.0f, -5.0f},
                -1.0
            ))
            || rohr_graphics_camera_attachment_get().kind != ERROR_RESULT_ERROR) {
        rohr_graphics_end();
        rohr_engine_shutdown();
        return 1;
    }
    camera_result = rohr_camera_get(original);
    if(rohr_error_check(camera_result)
            || camera_result.result.value.position.x != 25.0f
            || camera_result.result.value.position.y != 25.0f
            || rohr_error_check(rohr_camera_position_set(
                original,
                (Position){0.0f, 0.0f},
                0.0
            ))) {
        rohr_graphics_end();
        rohr_engine_shutdown();
        return 1;
    }
    {
        EngineResult moving_result;
        CameraZoomResult zoom_result;
        if(rohr_error_check(rohr_camera_position_set(
                original,
                (Position){10.0f, 10.0f},
                0.5
            ))) {
            rohr_graphics_end();
            rohr_engine_shutdown();
            return 1;
        }
        moving_result = rohr_camera_moving_get(original);
        if(rohr_error_check(moving_result) || !moving_result.result.value
                || rohr_error_check(rohr_camera_position_move(
                    original,
                    (Vec2D){0.0f, 0.0f},
                    0.0
                ))) {
            rohr_graphics_end();
            rohr_engine_shutdown();
            return 1;
        }
        moving_result = rohr_camera_moving_get(original);
        if(rohr_error_check(moving_result) || moving_result.result.value
            || rohr_error_check(rohr_camera_zoom_set(original, 1.5f, -1.0))) {
            rohr_graphics_end();
            rohr_engine_shutdown();
            return 1;
        }
        zoom_result = rohr_camera_zoom_get(original);
        if(rohr_error_check(zoom_result) || zoom_result.result.value != 1.5f
            || !rohr_error_check(rohr_camera_zoom_set(original, 0.0f, 0.0))
            || rohr_error_check(rohr_entity_delete(target_entity_result.result.value))) {
            rohr_graphics_end();
            rohr_engine_shutdown();
            return 1;
        }
    }
    if(rohr_error_check(rohr_camera_render_callback_set(original,
                count_camera_render, &render_count)) ||
            (rohr_graphics_show(), render_count != 0)) {
        rohr_graphics_end();
        rohr_engine_shutdown();
        return 1;
    }
    {
        ViewportConfig viewport_config = rohr_viewport_config_default_get();
        viewport_result = rohr_viewport_create(viewport_config);
    }
    if(rohr_error_check(viewport_result)
            || rohr_viewport_config_default_get().fit != SCREEN_FIT_CONTAIN
            || rohr_error_check(rohr_viewport_camera_set(
                viewport_result.result.value,
                original
            ))
            || rohr_error_check(rohr_camera_render_callback_set(
                original,
                count_camera_render,
                &render_count
            ))
            || rohr_error_check(rohr_viewport_enable_set(viewport_result.result.value))
            || (rohr_graphics_show(), render_count != 1)
            || rohr_graphics_layer_active_get() != 0
            || rohr_error_check(rohr_camera_disable_set(original))
            || (rohr_graphics_show(), render_count != 1)
            || rohr_error_check(rohr_camera_enable_set(original))
            || rohr_error_check(rohr_camera_pause_with_engine_set(original))
            || (rohr_engine_pause(), rohr_graphics_show(), render_count != 1)
            || rohr_error_check(rohr_camera_render_when_paused_set(original))
            || (rohr_graphics_show(), render_count != 2)
            || (rohr_engine_resume(), false)
            || rohr_error_check(rohr_camera_render_callback_set(
                second_result.result.value,
                count_camera_render,
                &render_count
            ))
            || (viewport_item_result = rohr_viewport_camera_add(
                    viewport_result.result.value,
                    second_result.result.value,
                    (ViewportItemConfig){
                        .rectangle = {20.0f, 30.0f, 320.0f, 180.0f},
                        .fit = SCREEN_FIT_COVER,
                        .orientation = 0.25f,
                        .layer = 2,
                        .visible = true,
                    }), rohr_error_check(viewport_item_result))
            || rohr_error_check(rohr_viewport_item_drag_mode_set(
                viewport_item_result.result.value, VIEWPORT_ITEM_DRAG_Y))
            || rohr_error_check(rohr_viewport_item_drag_mode_get(
                viewport_item_result.result.value))
            || rohr_viewport_item_drag_mode_get(viewport_item_result.result.value).
                result.value != VIEWPORT_ITEM_DRAG_Y
            || (rohr_graphics_show(), render_count != 4)
            || rohr_error_check(rohr_viewport_item_set(
                viewport_item_result.result.value,
                (ViewportItemConfig){
                    .rectangle = {40.0f, 50.0f, 160.0f, 90.0f},
                    .fit = SCREEN_FIT_CONTAIN,
                    .layer = -1,
                    .visible = false,
                }
            ))
            || (rohr_graphics_show(), render_count != 5)
            || rohr_error_check(rohr_viewport_item_remove(
                viewport_item_result.result.value
            ))
            || !rohr_error_check(rohr_viewport_item_remove(
                viewport_item_result.result.value
            ))
            || rohr_error_check(rohr_viewport_disable_set(viewport_result.result.value))
            || (rohr_graphics_show(), render_count != 5)
            || rohr_error_check(rohr_viewport_camera_clear(viewport_result.result.value))
            || rohr_error_check(rohr_viewport_destroy(viewport_result.result.value))) {
        rohr_graphics_end();
        rohr_engine_shutdown();
        return 1;
    }
    viewport_result = rohr_viewport_create(rohr_viewport_config_default_get());
    default_font = rohr_graphics_font_default_get();
    default_text = rohr_graphics_text_create(&default_font, "Built-In 123!",
        (Color){255, 255, 255, 255});
    ui_shape.text = (ViewportUiTextConfig){.text =
        rohr_error_check(default_text) ? NULL : &default_text.result.value,
        .scale = {2.0f, 2.0f}};
    second_viewport_result = rohr_viewport_create(rohr_viewport_config_default_get());
    if(rohr_error_check(viewport_result) || rohr_error_check(second_viewport_result) ||
            rohr_error_check(default_text) ||
            !rohr_graphics_text_value_set(&default_text.result.value,
                "Built-In Font") ||
            (ui_result = rohr_graphics_ui_shape_create(ui_shape),
                rohr_error_check(ui_result)) ||
            (ui_item_result = rohr_viewport_ui_add(
                viewport_result.result.value, ui_result.result.value,
                (ViewportItemConfig){.layer = 1, .visible = true}),
                rohr_error_check(ui_item_result)) ||
            rohr_error_check(rohr_viewport_item_drag_mode_set(
                ui_item_result.result.value, VIEWPORT_ITEM_DRAG_X)) ||
            rohr_error_check(rohr_viewport_item_drag_mode_get(
                ui_item_result.result.value)) ||
            rohr_viewport_item_drag_mode_get(ui_item_result.result.value).
                result.value != VIEWPORT_ITEM_DRAG_X ||
            rohr_viewport_item_dragging_check(ui_item_result.result.value) ||
            (second_ui_item_result = rohr_viewport_ui_add(
                second_viewport_result.result.value, ui_result.result.value,
                (ViewportItemConfig){.rectangle = {10.0f, 12.0f, 80.0f, 40.0f},
                    .content_scale = {1.5f, 0.75f}, .content_orientation = 0.2f,
                    .layer = 2, .visible = true, .clip_enabled = true,
                    .clip_rectangle = {10.0f, 12.0f, 80.0f, 40.0f}}),
                rohr_error_check(second_ui_item_result)) ||
            rohr_error_check(rohr_viewport_item_set(ui_item_result.result.value,
                (ViewportItemConfig){.rectangle = {20.0f, 30.0f, 0.0f, 0.0f},
                    .content_scale = {1.0f, 1.0f}, .visible = true})) ||
            rohr_error_check(rohr_viewport_item_get(ui_item_result.result.value)) ||
            rohr_viewport_item_get(ui_item_result.result.value)
                .result.value.rectangle.x != 20.0f ||
            rohr_error_check(rohr_viewport_item_viewport_get(
                second_ui_item_result.result.value)) ||
            rohr_viewport_item_viewport_get(
                second_ui_item_result.result.value).result.value !=
                    second_viewport_result.result.value ||
            rohr_error_check(rohr_graphics_layer_ui_name_set(
                ui_item_result.result.value, "world")) ||
            rohr_error_check(rohr_graphics_layer_ui_get(
                ui_item_result.result.value)) ||
            rohr_graphics_layer_ui_get(ui_item_result.result.value).result.value != 125 ||
            rohr_error_check(rohr_graphics_layer_name_set("world", 150)) ||
            rohr_graphics_layer_ui_get(ui_item_result.result.value).result.value != 150 ||
            !rohr_error_check(rohr_graphics_ui_destroy(ui_result.result.value)) ||
            (viewport_item_result = rohr_viewport_ui_shape_add(
                viewport_result.result.value, ui_shape,
                (ViewportItemConfig){.layer = 1, .visible = true}),
                rohr_error_check(viewport_item_result)) ||
            !rohr_error_check(rohr_viewport_ui_shape_add(
                viewport_result.result.value, (ViewportUiShapeConfig){0},
                (ViewportItemConfig){.visible = true})) ||
            rohr_error_check(rohr_viewport_enable_set(viewport_result.result.value)) ||
            (rohr_graphics_show(), false) ||
            rohr_viewport_ui_pressed_check(ui_item_result.result.value) ||
            rohr_error_check(rohr_viewport_item_remove(
                ui_item_result.result.value)) ||
            rohr_error_check(rohr_viewport_item_remove(
                second_ui_item_result.result.value)) ||
            rohr_error_check(rohr_graphics_ui_destroy(ui_result.result.value)) ||
            rohr_error_check(rohr_graphics_layer_sprite_name_set(
                target_entity_result.result.value, "world")) ||
            rohr_error_check(rohr_graphics_layer_animation_name_set(
                target_entity_result.result.value, "world")) ||
            rohr_error_check(rohr_graphics_layer_destroy(
                world_layer_result.result.value)) ||
            rohr_graphics_layer_sprite_get(
                target_entity_result.result.value).result.value != 150 ||
            rohr_graphics_layer_animation_get(
                target_entity_result.result.value).result.value != 150 ||
            rohr_error_check(rohr_viewport_destroy(second_viewport_result.result.value)) ||
            rohr_error_check(rohr_viewport_destroy(viewport_result.result.value))) {
        rohr_graphics_end();
        rohr_engine_shutdown();
        return 1;
    }
    rohr_graphics_text_destroy(&default_text.result.value);
    rohr_graphics_font_destroy(&default_font);
    rohr_graphics_end();
    rohr_engine_shutdown();
    return 0;
}
