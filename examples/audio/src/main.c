/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "rohr.h"
#include "example_input.h"
#include "example_viewport.h"

#include <stddef.h>
#include <stdio.h>

#define AUDIO_MUSIC_COUNT 3

static const char *audio_music_paths[AUDIO_MUSIC_COUNT] = {
    "assets/audio/ogg/golem_keep_golem_guarder_17.ogg",
    "assets/audio/ogg/golem_keep_farewell_2.ogg",
    "assets/audio/ogg/tug_of_war.ogg",
};

static const char *audio_music_names[AUDIO_MUSIC_COUNT] = {
    "Golem Gaurder",
    "Farewell",
    "Tug Of War",
};

typedef struct AudioRenderContext {
    TextAsset *title;
    TextAsset *description;
    TextAsset *sound_label;
    TextAsset *music_labels;
    TextAsset *quit_label;
    Music *music;
    bool sound_clicked;
    bool music_clicked[AUDIO_MUSIC_COUNT];
    bool quit_clicked;
} AudioRenderContext;

static void error_print(EngineError error) {
    fprintf(stderr, "audio example error %d: %s\n", (int)error,
        rohr_error_code_message_get(error));
}

static bool text_create(FontAsset *font, const char *value, TextAsset *text) {
    TextAssetResult result = rohr_graphics_text_create(
        font, value, (Color){238, 242, 248, 255});
    if(rohr_error_check(result)) {
        error_print(result.result.error);
        return false;
    }
    *text = result.result.value;
    return true;
}

static bool music_labels_update(TextAsset labels[AUDIO_MUSIC_COUNT],
        Music music[AUDIO_MUSIC_COUNT]) {
    char value[96];
    for(size_t index = 0; index < AUDIO_MUSIC_COUNT; index += 1) {
        const char *action = "Play";
        if(rohr_audio_music_paused_check(music[index])) action = "Resume";
        else if(rohr_audio_music_playing_check(music[index])) action = "Pause";
        (void)snprintf(value, sizeof(value), "%s %s", action,
            audio_music_names[index]);
        if(!rohr_graphics_text_value_set(&labels[index], value)) return false;
    }
    return true;
}

static void render_scene(CameraId camera, void *context_value) {
    AudioRenderContext *context = context_value;
    const float button_x = 390.0f;
    const float button_width = 500.0f;
    const float button_height = 58.0f;
    (void)camera;

    rohr_graphics_layer_active_set(-100);
    rohr_graphics_background_draw((Color){20, 25, 34, 255});
    rohr_graphics_layer_active_set(0);
    rohr_ui_frame_begin((UIInput){
        .pointer = rohr_graphics_mouse_screen_position_get(),
        .primary_button = example_input_mouse_button_state_get(
            INPUT_MOUSE_BUTTON_LEFT),
    });
    rohr_ui_label(context->title, (UIRect){340.0f, 55.0f, 600.0f, 55.0f});
    rohr_ui_label(context->description,
        (UIRect){250.0f, 115.0f, 780.0f, 40.0f});
    context->sound_clicked = rohr_ui_button("audio.sound.water_splash",
        context->sound_label,
        (UIRect){button_x, 185.0f, button_width, button_height}, NULL).clicked;
    for(size_t index = 0; index < AUDIO_MUSIC_COUNT; index += 1) {
        char id[48];
        (void)snprintf(id, sizeof(id), "audio.music.%zu", index);
        context->music_clicked[index] = rohr_ui_button(id,
            &context->music_labels[index],
            (UIRect){button_x, 275.0f + (float)index * 75.0f,
                button_width, button_height}, NULL).clicked;
    }
    context->quit_clicked = rohr_ui_button("audio.quit", context->quit_label,
        (UIRect){490.0f, 535.0f, 300.0f, 55.0f}, NULL).clicked;
    rohr_ui_frame_end();
}

int main(void) {
    bool engine_started = false;
    bool graphics_started = false;
    bool audio_started = false;
    bool running = true;
    int exit_code = 1;
    Sound sound = SOUND_INVALID;
    Music music[AUDIO_MUSIC_COUNT] = {MUSIC_INVALID};
    FontAsset font = {0};
    TextAsset title = {0};
    TextAsset description = {0};
    TextAsset sound_label = {0};
    TextAsset music_labels[AUDIO_MUSIC_COUNT] = {0};
    TextAsset quit_label = {0};
    ViewportId viewport = VIEWPORT_INVALID;
    AudioRenderContext render_context = {0};
    EngineResult result;

    result = rohr_directory_working_set(rohr_directory_base_get());
    if(rohr_error_check(result)) {
        error_print(result.result.error);
        return 1;
    }
    result = rohr_engine_start();
    if(rohr_error_check(result)) {
        error_print(result.result.error);
        return 1;
    }
    engine_started = true;
    result = rohr_graphics_start();
    if(rohr_error_check(result)) {
        error_print(result.result.error);
        goto cleanup;
    }
    graphics_started = true;
    result = rohr_audio_start();
    if(rohr_error_check(result)) {
        error_print(result.result.error);
        goto cleanup;
    }
    audio_started = true;

    {
        SoundConfig config = rohr_audio_sound_config_default_get();
        SoundResult created;
        config.path = "assets/audio/wav/water-splash-3.wav";
        created = rohr_audio_sound_create(config);
        if(rohr_error_check(created)) {
            error_print(created.result.error);
            goto cleanup;
        }
        sound = created.result.value;
    }
    for(size_t index = 0; index < AUDIO_MUSIC_COUNT; index += 1) {
        MusicConfig config = rohr_audio_music_config_default_get();
        MusicResult created;
        config.path = audio_music_paths[index];
        config.volume = 0.7f;
        config.loop = true;
        created = rohr_audio_music_create(config);
        if(rohr_error_check(created)) {
            error_print(created.result.error);
            goto cleanup;
        }
        music[index] = created.result.value;
    }

    font = rohr_graphics_font_default_get();
    if(!text_create(&font, "Rohr Audio", &title) ||
            !text_create(&font,
                "Play a WAV sound or control streamed Ogg Vorbis music",
                &description) ||
            !text_create(&font, "Play Water Splash", &sound_label) ||
            !text_create(&font, "Quit", &quit_label)) goto cleanup;
    for(size_t index = 0; index < AUDIO_MUSIC_COUNT; index += 1)
        if(!text_create(&font, "", &music_labels[index])) goto cleanup;
    if(!music_labels_update(music_labels, music)) goto cleanup;

    render_context = (AudioRenderContext){
        .title = &title,
        .description = &description,
        .sound_label = &sound_label,
        .music_labels = music_labels,
        .quit_label = &quit_label,
        .music = music,
    };
    if(!example_viewport_create(render_scene, &render_context, &viewport))
        goto cleanup;

    while(running) {
        SDL_Event event;
        rohr_input_frame_begin();
        while((event = rohr_engine_event_poll()).type != 0) {
            rohr_ui_event_add(&event);
            if(event.type == SDL_EVENT_QUIT ||
                    (event.type == SDL_EVENT_KEY_DOWN &&
                        event.key.scancode == SDL_SCANCODE_ESCAPE)) {
                running = false;
            }
        }
        if(!running) break;

        rohr_graphics_show();
        if(render_context.quit_clicked) break;
        if(render_context.sound_clicked) {
            result = rohr_audio_sound_play(sound);
            if(rohr_error_check(result)) {
                error_print(result.result.error);
                goto cleanup;
            }
        }
        for(size_t index = 0; index < AUDIO_MUSIC_COUNT; index += 1) {
            if(!render_context.music_clicked[index]) continue;
            if(rohr_audio_music_paused_check(music[index]))
                result = rohr_audio_music_resume(music[index]);
            else if(rohr_audio_music_playing_check(music[index]))
                result = rohr_audio_music_pause(music[index]);
            else
                result = rohr_audio_music_play(music[index]);
            if(rohr_error_check(result)) {
                error_print(result.result.error);
                goto cleanup;
            }
            if(!music_labels_update(music_labels, music)) goto cleanup;
        }
    }
    exit_code = 0;

cleanup:
    if(graphics_started) {
        example_viewport_destroy(&viewport);
        rohr_graphics_text_destroy(&quit_label);
        for(size_t index = 0; index < AUDIO_MUSIC_COUNT; index += 1)
            rohr_graphics_text_destroy(&music_labels[index]);
        rohr_graphics_text_destroy(&sound_label);
        rohr_graphics_text_destroy(&description);
        rohr_graphics_text_destroy(&title);
    }
    if(audio_started) {
        if(sound != SOUND_INVALID) (void)rohr_audio_sound_destroy(&sound);
        for(size_t index = 0; index < AUDIO_MUSIC_COUNT; index += 1)
            if(music[index] != MUSIC_INVALID)
                (void)rohr_audio_music_destroy(&music[index]);
        rohr_audio_stop();
    }
    if(graphics_started) rohr_graphics_stop();
    if(engine_started) rohr_engine_stop();
    return exit_code;
}
