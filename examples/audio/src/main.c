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

static void error_print(EngineError error) {
    fprintf(stderr, "audio example error %d: %s\n", (int)error,
        rohr_error_code_message_get(error));
}

static bool text_create(FontAsset *font, const char *value, TextAsset *text) {
    TextAssetResult result = rohr_graphics_text_create(
        (TextDescription){.font = font != NULL ? *font : (FontAsset){0},
            .value = value,
            .color = (Color){238, 242, 248, 255}});
    if(rohr_error_check(result)) {
        error_print(result.result.error);
        return false;
    }
    *text = result.result.value;
    return true;
}

static bool music_labels_update(TextAsset labels[AUDIO_MUSIC_COUNT],
        Music music[AUDIO_MUSIC_COUNT], unsigned char states[AUDIO_MUSIC_COUNT]) {
    char value[96];
    for(size_t index = 0; index < AUDIO_MUSIC_COUNT; index += 1) {
        unsigned char state = rohr_audio_music_paused_check(music[index]) ? 3 :
            (rohr_audio_music_playing_check(music[index]) ? 2 : 1);
        if(states[index] == state) continue;
        const char *action = state == 3 ? "Resume" : (state == 2 ? "Pause" : "Play");
        (void)snprintf(value, sizeof(value), "%s %s", action,
            audio_music_names[index]);
        if(!rohr_graphics_text_value_set(&labels[index], value)) return false;
        states[index] = state;
    }
    return true;
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
    unsigned char music_label_states[AUDIO_MUSIC_COUNT] = {0};
    TextAsset quit_label = {0};
    ViewportId viewport = VIEWPORT_INVALID;
    ViewportItemId sound_button=0, quit_button=0, music_buttons[AUDIO_MUSIC_COUNT]={0};
    bool sound_armed=false, quit_armed=false, music_armed[AUDIO_MUSIC_COUNT]={0};
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
        SoundDescription config = rohr_audio_sound_description_default_get();
        SoundResult created;
        config.path = "assets/audio/wav/water-splash-3.wav";
        config.volume = 1.0f;
        created = rohr_audio_sound_create(config);
        if(rohr_error_check(created)) {
            error_print(created.result.error);
            goto cleanup;
        }
        sound = created.result.value;
    }
    for(size_t index = 0; index < AUDIO_MUSIC_COUNT; index += 1) {
        MusicDescription config = rohr_audio_music_description_default_get();
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
    if(!music_labels_update(music_labels, music, music_label_states)) goto cleanup;

    if(!example_ui_viewport_create((Color){20,25,34,255},&viewport) ||
            !example_label_add(viewport,title,(UIRect){340,55,600,55}) ||
            !example_label_add(viewport,description,(UIRect){250,115,780,40})) goto cleanup;
    sound_button=example_button_add(viewport,sound_label,(UIRect){390,185,500,58},NULL);
    quit_button=example_button_add(viewport,quit_label,(UIRect){490,535,300,55},NULL);
    if(!sound_button || !quit_button) goto cleanup;
    for(size_t i=0;i<AUDIO_MUSIC_COUNT;++i) {
        music_buttons[i]=example_button_add(viewport,music_labels[i],(UIRect){390,275+i*75,500,58},NULL);
        if(!music_buttons[i]) goto cleanup;
    }

    while(running) {
        SDL_Event event;
        rohr_input_frame_begin();
        while((event = rohr_engine_event_poll()).type != 0) {
            if(event.type == SDL_EVENT_QUIT ||
                    (event.type == SDL_EVENT_KEY_DOWN &&
                        event.key.scancode == SDL_SCANCODE_ESCAPE)) {
                running = false;
            }
        }
        if(!running) break;

        Report audio_report = rohr_audio_update();
        if(audio_report.diagnostic_count || audio_report.has_fatal) {
            EngineResult printed = rohr_report_console_write(&audio_report);
            if(rohr_error_check(printed)) error_print(printed.result.error);
        }
        if(!music_labels_update(music_labels, music, music_label_states)) goto cleanup;
        if(rohr_graphics_viewport_show(viewport).header.status == ROHR_REPORT_FAILED) goto cleanup;
        if(example_button_clicked_check(quit_button,&quit_armed)) break;
        if(example_button_clicked_check(sound_button,&sound_armed)) {
            result = rohr_audio_sound_play(sound);
            if(rohr_error_check(result)) {
                error_print(result.result.error);
                goto cleanup;
            }
        }
        for(size_t index = 0; index < AUDIO_MUSIC_COUNT; index += 1) {
            if(!example_button_clicked_check(music_buttons[index],&music_armed[index])) continue;
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
        Report stopped = rohr_audio_stop();
        if(stopped.diagnostic_count || stopped.has_fatal) {
            EngineResult printed = rohr_report_console_write(&stopped);
            if(rohr_error_check(printed)) error_print(printed.result.error);
        }
    }
    if(graphics_started) rohr_graphics_stop();
    if(engine_started) rohr_engine_stop();
    return exit_code;
}
