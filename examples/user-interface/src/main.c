/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include <stdio.h>
#include "rohr.h"
#include "example_runtime.h"
#include "example_viewport.h"

#define PRINT_ENGINE_ERROR(engine_result) \
    fprintf(stderr, "error %d: %s\n", (int)(engine_result).result.error, \
        rohr_error_message_get(engine_result))

typedef struct RenderContext {
    MouseState *mouse;
    TextAsset *title;
    TextAsset *play_label;
    TextAsset *settings_label;
    TextAsset *quit_label;
    TextAsset *description;
    TextAsset *slider_label;
    TextAsset *slider_value_label;
    TextAsset *slider_minus;
    TextAsset *slider_plus;
    UILabelDefinition *title_definition;
    UILabelDefinition *play_definition;
    UILabelDefinition *description_definition;
    UIButtonDefinition *settings_button;
    UISliderDefinition *value_slider;
    float slider_value;
    UIButtonResult play;
    UIButtonResult settings;
    UIButtonResult quit;
    UISliderResult slider;
} RenderContext;

static void render_scene(CameraId camera, void *context_value) {
    RenderContext *context = context_value;
    UIRect play_bounds = context->play_definition->bounds;
    (void)camera;
    rohr_graphics_layer_active_set(-100);
    rohr_graphics_background_draw((Color){18, 22, 30, 255});
    rohr_graphics_layer_active_set(0);
    rohr_ui_frame_begin((UIInput){
        .pointer = rohr_graphics_mouse_screen_position_get(),
        .primary_button = context->mouse->button_states[MOUSE_BUTTON_LEFT],
    });
    rohr_ui_label(context->title, context->title_definition->bounds);
    context->play = rohr_ui_button("main_menu.play", NULL, play_bounds, NULL);
    context->settings = rohr_ui_button(context->settings_button->name,
        context->settings_label, context->settings_button->bounds,
        &context->settings_button->style);
    context->quit = rohr_ui_button("main_menu.quit", context->quit_label,
        (UIRect){220.0f, 280.0f, 200.0f, 55.0f}, NULL);
    rohr_ui_label(context->play_label, play_bounds);
    rohr_ui_label(context->description, context->description_definition->bounds);
    context->slider = rohr_ui_slider_with_text(context->value_slider->name,
        context->slider_value, &context->value_slider->config,
        &(UISliderText){.label = context->slider_label,
            .value = context->slider_value_label, .minus = context->slider_minus,
            .plus = context->slider_plus});
    rohr_ui_frame_end();
}

int main(void) {
    if(!example_use_executable_directory()) return 1;
    KeyboardState keyboard = {0};
    MouseState mouse = {0};
    bool running = true;
    FontAsset font = {0};
    TextAsset title = {0};
    TextAsset play_label = {0};
    TextAsset settings_label = {0};
    TextAsset quit_label = {0};
    TextAsset description = {0};
    TextAsset slider_label = {0};
    TextAsset slider_value_label = {0};
    TextAsset slider_minus = {0};
    TextAsset slider_plus = {0};
    UIFontDefinition font_definition;
    UILabelDefinition title_definition;
    UILabelDefinition play_definition;
    UILabelDefinition description_definition;
    UIButtonDefinition settings_button;
    UISliderDefinition value_slider;
    float slider_value;
    ViewportId viewport = VIEWPORT_INVALID;
    RenderContext render_context = {0};

    {
        EngineResult init_result = rohr_engine_init();
        if(rohr_error_check(init_result)) {
            PRINT_ENGINE_ERROR(init_result);
            return 1;
        }
    }
    {
        EngineResult graphics_result = rohr_graphics_start();
        if(rohr_error_check(graphics_result)) {
            PRINT_ENGINE_ERROR(graphics_result);
            rohr_engine_shutdown();
            return 1;
        }
    }
    {
        EngineResult state_result = rohr_game_state_file_load(
            "assets/user-interface/game.json"
        );
        if(rohr_error_check(state_result)) {
            PRINT_ENGINE_ERROR(state_result);
            goto fail;
        }
        UIButtonDefinitionResult settings_result =
            rohr_ui_button_by_name_get("settings_button");
        if(rohr_error_check(settings_result)) {
            PRINT_ENGINE_ERROR(settings_result);
            goto fail;
        }
        settings_button = settings_result.result.value;

        UIFontDefinitionResult font_result =
            rohr_ui_font_by_name_get("menu_font");
        if(rohr_error_check(font_result)) {
            PRINT_ENGINE_ERROR(font_result);
            goto fail;
        }
        font_definition = font_result.result.value;

        UILabelDefinitionResult title_result =
            rohr_ui_label_by_name_get("example_title");
        if(rohr_error_check(title_result)) {
            PRINT_ENGINE_ERROR(title_result);
            goto fail;
        }
        title_definition = title_result.result.value;

        UILabelDefinitionResult play_result =
            rohr_ui_label_by_name_get("play_label");
        if(rohr_error_check(play_result)) {
            PRINT_ENGINE_ERROR(play_result);
            goto fail;
        }
        play_definition = play_result.result.value;

        UILabelDefinitionResult description_result =
            rohr_ui_label_by_name_get("example_description");
        if(rohr_error_check(description_result)) {
            PRINT_ENGINE_ERROR(description_result);
            goto fail;
        }
        description_definition = description_result.result.value;

        UISliderDefinitionResult slider_result =
            rohr_ui_slider_by_name_get("angled_value_slider");
        if(rohr_error_check(slider_result)) {
            PRINT_ENGINE_ERROR(slider_result);
            goto fail;
        }
        value_slider = slider_result.result.value;
        slider_value = value_slider.initial_value;
    }
    {
        FontAssetResult font_result = rohr_graphics_font_load((FontDescriptor){
            .file = font_definition.file,
            .point_size = font_definition.point_size,
        });
        if(rohr_error_check(font_result)) {
            PRINT_ENGINE_ERROR(font_result);
            goto fail;
        }
        font = font_result.result.value;

        TextAssetResult title_result = rohr_graphics_text_create(
            &font,
            title_definition.text,
            title_definition.color
        );
        if(rohr_error_check(title_result)) {
            PRINT_ENGINE_ERROR(title_result);
            goto fail;
        }
        title = title_result.result.value;

        TextAssetResult play_result = rohr_graphics_text_create(
            &font,
            play_definition.text,
            play_definition.color
        );
        if(rohr_error_check(play_result)) {
            PRINT_ENGINE_ERROR(play_result);
            goto fail;
        }
        play_label = play_result.result.value;

        TextAssetResult settings_result = rohr_graphics_text_create(
            &font,
            settings_button.label,
            settings_button.text_color
        );
        if(rohr_error_check(settings_result)) {
            PRINT_ENGINE_ERROR(settings_result);
            goto fail;
        }
        settings_label = settings_result.result.value;

        TextAssetResult quit_result = rohr_graphics_text_create(
            &font,
            "Quit",
            (Color){240, 244, 250, 255}
        );
        if(rohr_error_check(quit_result)) {
            PRINT_ENGINE_ERROR(quit_result);
            goto fail;
        }
        quit_label = quit_result.result.value;

        TextAssetResult description_result = rohr_graphics_text_create(
            &font,
            description_definition.text,
            description_definition.color
        );
        if(rohr_error_check(description_result)) {
            PRINT_ENGINE_ERROR(description_result);
            goto fail;
        }
        description = description_result.result.value;

        TextAssetResult slider_label_result = rohr_graphics_text_create(
            &font, value_slider.label, value_slider.text_color
        );
        if(rohr_error_check(slider_label_result)) {
            PRINT_ENGINE_ERROR(slider_label_result);
            goto fail;
        }
        slider_label = slider_label_result.result.value;
        TextAssetResult minus_result = rohr_graphics_text_create(
            &font, "-", value_slider.text_color
        );
        if(rohr_error_check(minus_result)) {
            PRINT_ENGINE_ERROR(minus_result);
            goto fail;
        }
        slider_minus = minus_result.result.value;
        TextAssetResult plus_result = rohr_graphics_text_create(
            &font, "+", value_slider.text_color
        );
        if(rohr_error_check(plus_result)) {
            PRINT_ENGINE_ERROR(plus_result);
            goto fail;
        }
        slider_plus = plus_result.result.value;
        char value_text[UI_LABEL_MAX];
        snprintf(value_text, sizeof(value_text), value_slider.value_format, slider_value);
        TextAssetResult value_result = rohr_graphics_text_create(
            &font, value_text, value_slider.text_color
        );
        if(rohr_error_check(value_result)) {
            PRINT_ENGINE_ERROR(value_result);
            goto fail;
        }
        slider_value_label = value_result.result.value;
    }
    render_context = (RenderContext){
        .mouse = &mouse, .title = &title, .play_label = &play_label,
        .settings_label = &settings_label, .quit_label = &quit_label,
        .description = &description, .slider_label = &slider_label,
        .slider_value_label = &slider_value_label, .slider_minus = &slider_minus,
        .slider_plus = &slider_plus, .title_definition = &title_definition,
        .play_definition = &play_definition,
        .description_definition = &description_definition,
        .settings_button = &settings_button, .value_slider = &value_slider,
        .slider_value = slider_value};
    if(!example_viewport_create(render_scene, &render_context, &viewport)) goto fail;

    while(running) {
        SDL_Event event;
        bool exit_requested = false;

        rohr_controller_key_states_update(&keyboard);
        rohr_controller_mouse_states_update(&mouse);
        while((event = rohr_engine_event_poll()).type != 0) {
            rohr_controller_key_event_add(&keyboard,
                rohr_controller_keyboard_event_capture(&event));
            rohr_controller_mouse_event_add(&mouse,
                rohr_controller_mouse_event_capture(&event));
            rohr_ui_event_add(&event);
            if(event.type == SDL_EVENT_QUIT) exit_requested = true;
        }
        if(exit_requested ||
                rohr_controller_key_pressed_get(&keyboard, SDLK_ESCAPE)) break;

        render_context.slider_value = slider_value;
        rohr_graphics_show();
        slider_value = render_context.slider.value;
        if(render_context.slider.changed) {
            char value_text[UI_LABEL_MAX];
            snprintf(value_text, sizeof(value_text), value_slider.value_format, slider_value);
            rohr_graphics_text_destroy(&slider_value_label);
            TextAssetResult value_result = rohr_graphics_text_create(
                &font, value_text, value_slider.text_color
            );
            if(rohr_error_check(value_result)) {
                PRINT_ENGINE_ERROR(value_result);
                goto fail;
            } else {
                slider_value_label = value_result.result.value;
            }
        }

        if(render_context.play.clicked) printf("Play clicked\n");
        if(render_context.settings.clicked) printf("Settings clicked\n");
        if(render_context.quit.clicked) running = false;
    }

    example_viewport_destroy(&viewport);
    rohr_graphics_text_destroy(&slider_plus);
    rohr_graphics_text_destroy(&slider_minus);
    rohr_graphics_text_destroy(&slider_value_label);
    rohr_graphics_text_destroy(&slider_label);
    rohr_graphics_text_destroy(&description);
    rohr_graphics_text_destroy(&quit_label);
    rohr_graphics_text_destroy(&settings_label);
    rohr_graphics_text_destroy(&play_label);
    rohr_graphics_text_destroy(&title);
    rohr_graphics_font_destroy(&font);
    rohr_graphics_end();
    rohr_engine_shutdown();
    return 0;

fail:
    example_viewport_destroy(&viewport);
    rohr_graphics_text_destroy(&slider_plus);
    rohr_graphics_text_destroy(&slider_minus);
    rohr_graphics_text_destroy(&slider_value_label);
    rohr_graphics_text_destroy(&slider_label);
    rohr_graphics_text_destroy(&description);
    rohr_graphics_text_destroy(&quit_label);
    rohr_graphics_text_destroy(&settings_label);
    rohr_graphics_text_destroy(&play_label);
    rohr_graphics_text_destroy(&title);
    rohr_graphics_font_destroy(&font);
    rohr_graphics_end();
    rohr_engine_shutdown();
    return 1;
}
