/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include <stdio.h>
#include <math.h>
#include "rohr.h"
#include "example_viewport.h"
#include "example_input.h"

#define PRINT_ENGINE_ERROR(engine_result) \
    fprintf(stderr, "error %d: %s\n", (int)(engine_result).result.error, \
        rohr_error_message_get(engine_result))

int main(void) {
    if(rohr_error_check(rohr_directory_working_set(
            rohr_directory_base_get()))) return 1;
    InputControllerId input_controller = INPUT_CONTROLLER_INVALID;
    InputActionId exit_action = INPUT_ACTION_INVALID;
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
    ViewportItemId play=0, settings=0, quit=0, slider=0, minus=0, plus=0;
    bool armed[5]={0};
    GraphicsUiId slider_ui=GRAPHICS_UI_INVALID;

    {
        EngineResult init_result = rohr_engine_start();
        if(rohr_error_check(init_result)) {
            PRINT_ENGINE_ERROR(init_result);
            return 1;
        }
    }
    {
        InputBinding exit_binding = example_input_key_binding(SDL_SCANCODE_ESCAPE,
            1.0f, (Vec2D){0});
        if(!example_input_controller_create("ui", &input_controller) ||
                !example_input_action_create(input_controller, "exit",
                    INPUT_ACTION_BUTTON, &exit_binding, 1, &exit_action)) goto fail;
    }
    {
        EngineResult graphics_result = rohr_graphics_start();
        if(rohr_error_check(graphics_result)) {
            PRINT_ENGINE_ERROR(graphics_result);
            (void)rohr_input_controller_destroy(input_controller);
            rohr_engine_stop();
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
        FontAssetResult font_result = rohr_graphics_font_load((FontDescription){
            .file = font_definition.file,
            .point_size = font_definition.point_size,
        });
        if(rohr_error_check(font_result)) {
            PRINT_ENGINE_ERROR(font_result);
            goto fail;
        }
        font = font_result.result.value;

        TextAssetResult title_result = rohr_graphics_text_create(
            (TextDescription){.font = font,
                .value = title_definition.text,
                .color = title_definition.color});
        if(rohr_error_check(title_result)) {
            PRINT_ENGINE_ERROR(title_result);
            goto fail;
        }
        title = title_result.result.value;

        TextAssetResult play_result = rohr_graphics_text_create(
            (TextDescription){.font = font,
                .value = play_definition.text,
                .color = play_definition.color});
        if(rohr_error_check(play_result)) {
            PRINT_ENGINE_ERROR(play_result);
            goto fail;
        }
        play_label = play_result.result.value;

        TextAssetResult settings_result = rohr_graphics_text_create(
            (TextDescription){.font = font,
                .value = settings_button.label,
                .color = settings_button.text_color});
        if(rohr_error_check(settings_result)) {
            PRINT_ENGINE_ERROR(settings_result);
            goto fail;
        }
        settings_label = settings_result.result.value;

        TextAssetResult quit_result = rohr_graphics_text_create(
            (TextDescription){.font = font,
                .value = "Quit",
                .color = (Color){240, 244, 250, 255}});
        if(rohr_error_check(quit_result)) {
            PRINT_ENGINE_ERROR(quit_result);
            goto fail;
        }
        quit_label = quit_result.result.value;

        TextAssetResult description_result = rohr_graphics_text_create(
            (TextDescription){.font = font,
                .value = description_definition.text,
                .color = description_definition.color});
        if(rohr_error_check(description_result)) {
            PRINT_ENGINE_ERROR(description_result);
            goto fail;
        }
        description = description_result.result.value;

        TextAssetResult slider_label_result = rohr_graphics_text_create(
            (TextDescription){.font = font,
                .value = value_slider.label,
                .color = value_slider.text_color});
        if(rohr_error_check(slider_label_result)) {
            PRINT_ENGINE_ERROR(slider_label_result);
            goto fail;
        }
        slider_label = slider_label_result.result.value;
        TextAssetResult minus_result = rohr_graphics_text_create(
            (TextDescription){.font = font,
                .value = "-",
                .color = value_slider.text_color});
        if(rohr_error_check(minus_result)) {
            PRINT_ENGINE_ERROR(minus_result);
            goto fail;
        }
        slider_minus = minus_result.result.value;
        TextAssetResult plus_result = rohr_graphics_text_create(
            (TextDescription){.font = font,
                .value = "+",
                .color = value_slider.text_color});
        if(rohr_error_check(plus_result)) {
            PRINT_ENGINE_ERROR(plus_result);
            goto fail;
        }
        slider_plus = plus_result.result.value;
        char value_text[UI_LABEL_MAX];
        snprintf(value_text, sizeof(value_text), value_slider.value_format, slider_value);
        TextAssetResult value_result = rohr_graphics_text_create(
            (TextDescription){.font = font,
                .value = value_text,
                .color = value_slider.text_color});
        if(rohr_error_check(value_result)) {
            PRINT_ENGINE_ERROR(value_result);
            goto fail;
        }
        slider_value_label = value_result.result.value;
    }
    if(!example_ui_viewport_create((Color){18,22,30,255},&viewport) ||
            !example_label_add(viewport,title,title_definition.bounds) ||
            !example_label_add(viewport,description,description_definition.bounds)) goto fail;
    play=example_button_add(viewport,play_label,play_definition.bounds,NULL);
    settings=example_button_add(viewport,settings_label,settings_button.bounds,&settings_button.style);
    quit=example_button_add(viewport,quit_label,(UIRect){220,280,200,55},NULL);
    if(!play || !settings || !quit) goto fail;
    const UISliderConfig *config=&value_slider.config;
    ViewportUiSliderDescription description_slider={
        .minimum=fminf(config->min_value,config->max_value),.maximum=fmaxf(config->min_value,config->max_value),
        .value=slider_value,.step=config->step,.length=config->length,
        .track_thickness=config->style.track_thickness,.thumb_width=config->style.handle_width,
        .thumb_height=config->style.handle_height,.track_color=config->style.track,
        .filled_track_color=config->style.fill,.thumb_color=config->style.handle,
        .hover_thumb_color=config->style.handle_hovered,.pressed_thumb_color=config->style.handle_pressed,
        .enabled=!config->read_only};
    ViewportItemDescription placement=rohr_viewport_item_description_default_get();
    placement.fit=SCREEN_FIT_NONE; placement.layer=10;
    placement.rectangle=(ViewportRectangle){config->center.x,config->center.y,0,0};
    placement.orientation=config->angle;
    ViewportItemIdResult added=rohr_viewport_ui_slider_add(viewport,description_slider,placement);
    if(rohr_error_check(added)) goto fail;
    slider=added.result.value;
    slider_ui=rohr_viewport_item_description_get(slider).result.value.ui;
    float height=fmaxf(config->style.handle_height,config->style.track_thickness);
    if(!example_label_add(viewport,slider_label,(UIRect){config->center.x-config->length*.5f,
            config->center.y-height*.5f-42,config->length,28}) ||
       !example_label_add(viewport,slider_value_label,(UIRect){config->center.x-60,config->center.y+height*.5f+8,120,28})) goto fail;
    float size=config->style.step_button_size;
    float offset=config->length*.5f+config->style.step_button_gap+size*.5f;
    float radians=rohr_math_degrees_to_radians(config->angle);
    Position axis={offset*cosf(radians),offset*sinf(radians)};
    UIButtonStyle step_style={config->style.handle,config->style.handle_hovered,config->style.handle_pressed,config->style.handle};
    minus=example_button_add(viewport,slider_minus,(UIRect){config->center.x-axis.x-size*.5f,config->center.y-axis.y-size*.5f,size,size},&step_style);
    plus=example_button_add(viewport,slider_plus,(UIRect){config->center.x+axis.x-size*.5f,config->center.y+axis.y-size*.5f,size,size},&step_style);
    if(!minus || !plus) goto fail;

    while(running) {
        SDL_Event event;
        bool exit_requested = false;

        rohr_input_frame_begin();
        while((event = rohr_engine_event_poll()).type != 0) {
            if(event.type == SDL_EVENT_QUIT) exit_requested = true;
        }
        if(exit_requested ||
                rohr_input_action_button_pressed_check(exit_action)) break;

        if(rohr_graphics_viewport_show(viewport).header.status == ROHR_REPORT_FAILED) goto fail;
        GraphicsUiSliderValueResult value=rohr_graphics_ui_slider_value_get(slider_ui);
        if(rohr_error_check(value)) goto fail;
        float next=value.result.value;
        if(example_button_clicked_check(minus,&armed[3]) && !config->read_only) next-=config->step;
        if(example_button_clicked_check(plus,&armed[4]) && !config->read_only) next+=config->step;
        next=fmaxf(description_slider.minimum,fminf(description_slider.maximum,next));
        if(next!=value.result.value && rohr_error_check(rohr_graphics_ui_slider_value_set(slider_ui,next))) goto fail;
        if(next!=slider_value) {
            slider_value=next;
            char text[UI_LABEL_MAX];
            snprintf(text,sizeof(text),value_slider.value_format,slider_value);
            if(!rohr_graphics_text_value_set(&slider_value_label,text)) goto fail;
        }
        if(example_button_clicked_check(play,&armed[0])) printf("Play clicked\n");
        if(example_button_clicked_check(settings,&armed[1])) printf("Settings clicked\n");
        if(example_button_clicked_check(quit,&armed[2])) running=false;
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
    (void)rohr_graphics_font_release(&font);
    rohr_graphics_stop();
    if(input_controller != INPUT_CONTROLLER_INVALID)
        (void)rohr_input_controller_destroy(input_controller);
    rohr_engine_stop();
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
    (void)rohr_graphics_font_release(&font);
    rohr_graphics_stop();
    if(input_controller != INPUT_CONTROLLER_INVALID)
        (void)rohr_input_controller_destroy(input_controller);
    rohr_engine_stop();
    return 1;
}
