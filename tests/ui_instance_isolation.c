/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"

int main(void) {
    ViewportUiSliderConfig definition = {
        .minimum = 0.0f, .maximum = 10.0f, .value = 4.0f, .step = 1.0f,
        .length = 100.0f, .track_thickness = 4.0f,
        .thumb_width = 10.0f, .thumb_height = 20.0f, .enabled = true};
    GraphicsUiIdResult instance_1;
    GraphicsUiIdResult instance_2;
    GraphicsUiIdResult shape_1;
    GraphicsUiIdResult shape_2;
    GraphicsUiIdResult text_1;
    GraphicsUiIdResult text_2;
    int failed = 0;

    if(rohr_error_check(rohr_engine_init())) return 1;
    if(rohr_error_check(rohr_graphics_start())) {
        rohr_engine_shutdown();
        return 1;
    }
    instance_1 = rohr_graphics_ui_slider_create(definition);
    instance_2 = rohr_graphics_ui_slider_create(definition);
    shape_1 = rohr_graphics_ui_shape_create((ViewportUiShapeConfig){
        .shape = {.amount_of_vertices = 3,
            .vertices = {{0.0f, 0.0f}, {10.0f, 0.0f}, {0.0f, 10.0f}}},
        .position = {1.0f, 2.0f}, .fill_color = {10, 20, 30, 255}});
    shape_2 = rohr_graphics_ui_shape_create((ViewportUiShapeConfig){
        .shape = {.amount_of_vertices = 3,
            .vertices = {{0.0f, 0.0f}, {10.0f, 0.0f}, {0.0f, 10.0f}}},
        .position = {1.0f, 2.0f}, .fill_color = {10, 20, 30, 255}});
    text_1 = rohr_graphics_ui_text_create((ViewportUiTextConfig){
        .position = {3.0f, 4.0f}, .scale = {1.0f, 1.0f}});
    text_2 = rohr_graphics_ui_text_create((ViewportUiTextConfig){
        .position = {3.0f, 4.0f}, .scale = {1.0f, 1.0f}});
    if(rohr_error_check(instance_1) || rohr_error_check(instance_2) ||
            rohr_error_check(shape_1) || rohr_error_check(shape_2) ||
            rohr_error_check(text_1) || rohr_error_check(text_2) ||
            instance_1.result.value == instance_2.result.value ||
            shape_1.result.value == shape_2.result.value ||
            text_1.result.value == text_2.result.value ||
            rohr_error_check(rohr_graphics_ui_slider_value_set(
                instance_1.result.value, 9.0f)) ||
            rohr_graphics_ui_slider_value_get(instance_1.result.value).
                result.value != 9.0f ||
            rohr_graphics_ui_slider_value_get(instance_2.result.value).
                result.value != 4.0f ||
            rohr_error_check(rohr_graphics_ui_shape_set(shape_1.result.value,
                (ViewportUiShapeConfig){
                    .shape = {.amount_of_vertices = 3,
                        .vertices = {{0.0f, 0.0f}, {10.0f, 0.0f},
                            {0.0f, 10.0f}}},
                    .position = {8.0f, 9.0f}})) ||
            rohr_graphics_ui_shape_get(shape_1.result.value).
                result.value.position.x != 8.0f ||
            rohr_graphics_ui_shape_get(shape_2.result.value).
                result.value.position.x != 1.0f ||
            rohr_error_check(rohr_graphics_ui_text_set(text_1.result.value,
                (ViewportUiTextConfig){.position = {7.0f, 6.0f},
                    .scale = {2.0f, 2.0f}})) ||
            rohr_graphics_ui_text_get(text_1.result.value).
                result.value.position.x != 7.0f ||
            rohr_graphics_ui_text_get(text_2.result.value).
                result.value.position.x != 3.0f ||
            rohr_error_check(rohr_graphics_ui_destroy(instance_1.result.value)) ||
            rohr_graphics_ui_slider_value_get(instance_2.result.value).
                result.value != 4.0f ||
            rohr_error_check(rohr_graphics_ui_destroy(instance_2.result.value)) ||
            rohr_error_check(rohr_graphics_ui_destroy(shape_1.result.value)) ||
            rohr_error_check(rohr_graphics_ui_destroy(shape_2.result.value)) ||
            rohr_error_check(rohr_graphics_ui_destroy(text_1.result.value)) ||
            rohr_error_check(rohr_graphics_ui_destroy(text_2.result.value)))
        failed = 1;
    rohr_graphics_end();
    rohr_engine_shutdown();
    return failed;
}
