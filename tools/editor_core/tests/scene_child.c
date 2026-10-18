/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "scene/scene_transport.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define OK(call) assert(!error_check(call))
static const char *mode;
static ViewportItemId button;
static unsigned updates, renders[2];
static bool hosted;
static Entity particle;
static void camera_draw(CameraId camera, void *context) {
    (void)camera;
    unsigned index = *(unsigned *)context;
    ++renders[index];
    graphics_background_draw(index ? (Color){20, 180, 40, 255} : (Color){180, 40, 20, 255});
}
static EngineResult initialize(void *context) {
    (void)context;
    if(!strcmp(mode, "init_fail")) return error_result_error_detail(ERROR_ENGINE_STATE_INVALID, "fixture initialization failure");
    GraphicsWindowPresentationConfig config = graphics_window_presentation_default_get();
    config.window_width = 640; config.window_height = 480;
    config.logical_width = 320; config.logical_height = 180;
    if(hosted) config.mode = GRAPHICS_WINDOW_MODE_FULLSCREEN;
    if(!strcmp(mode, "large")) { config.logical_width = 1920; config.logical_height = 1080; config.window_width = 1920; config.window_height = 1080; }
    OK(graphics_window_presentation_set(config));
    EntityResult moving = physics_particle_add((ParticleDescription){.radius = 1,
        .mass_value = 1, .velocity = {10, 0}});
    OK(moving); particle = moving.result.value;
    if(hosted) {
        int count; SDL_Window **windows = SDL_GetWindows(&count);
        assert(windows && count == 1);
        assert((SDL_GetWindowFlags(windows[0]) & SDL_WINDOW_HIDDEN) != 0);
        assert((SDL_GetWindowFlags(windows[0]) & SDL_WINDOW_FULLSCREEN) == 0);
        SDL_free(windows);
    }
    (void)graphics_vsync_set(false);
    OK(input_text_start()); OK(input_text_area_set((SDL_Rect){10, 20, 60, 30}, 4));
    OK(input_mouse_relative_mode_set(true));
    static unsigned indices[] = {0, 1};
    ViewportIdResult viewport = graphics_viewport_create((ViewportConfig){.rectangle = {0, 0, 320, 180}, .background_color = {1,2,3,255}});
    OK(viewport); OK(graphics_viewport_enable_set(viewport.result.value));
    for(unsigned i = 0; i < 2; ++i) {
        CameraIdResult camera = graphics_camera_create(graphics_camera_config_default_get()); OK(camera);
        OK(graphics_camera_render_callback_set(camera.result.value, camera_draw, &indices[i]));
        ScreenIdResult screen = graphics_screen_create((ScreenConfig){camera.result.value, 160, 180}); OK(screen);
        ViewportItemIdResult item = graphics_viewport_screen_add(viewport.result.value, screen.result.value,
            (ViewportItemConfig){.rectangle = {i ? 160 : -10, 0, i ? 145 : 170, 180}, .fit = SCREEN_FIT_STRETCH,
                .orientation = i ? 15 : 0, .layer = i ? 2 : 0, .visible = true}); OK(item);
    }
    ViewportUiShapeConfig shape = {.shape = {.amount_of_vertices = 4,
        .vertices = {{0,0}, {40,0}, {40,30}, {0,30}}}, .position = {0,0},
        .fill_color = {20, 40, 180, 255}, .button_enabled = true,
        .hover_fill_color = {20,40,180,255}, .click_fill_color = {20,40,180,255}};
    ViewportItemIdResult ui = graphics_viewport_ui_shape_add(viewport.result.value, shape,
        (ViewportItemConfig){.rectangle = {10,10,40,30}, .layer = 3, .visible = true}); OK(ui); button = ui.result.value;
    ui = graphics_viewport_ui_shape_add(viewport.result.value, shape,
        (ViewportItemConfig){.rectangle = {130,90,40,30}, .layer = 1, .visible = true}); OK(ui);
    puts("INITIALIZED");
    if(!strcmp(mode, "logs")) for(unsigned i = 0; i < 8192; ++i) puts("bounded log overflow fixture");
    return error_result_value(true);
}
static Report update(void *context, Tick ticks, Time time) {
    (void)context; (void)time;
    PositionResult position = physics_position_get(particle); OK(position);
    /* Physics must advance exactly once for elapsed ticks, including across a
     * host pause. Extra paused steps or resumed catch-up violate this position. */
    double expected = (engine_tick_get() - ticks) * engine_time_per_tick_get() * 10;
    assert(fabs(position.result.value.x - expected) < .01);
    Report report = report_create(); report.header = (ReportHeader){ROHR_REPORT_CUSTOM_ID_BEGIN, ROHR_REPORT_COMPLETE, 0};
    ++updates;
    if(!strcmp(mode, "update_fail")) {
        ReportDiagnostic diagnostic = {.report_id = ROHR_REPORT_CUSTOM_ID_BEGIN, .code = ERROR_ENGINE_STATE_INVALID,
            .severity = ROHR_REPORT_SEVERITY_FATAL, .recovery = ROHR_REPORT_UPDATE_ABORTED, .occurrence_count = 1};
        OK(report_diagnostic_detail_set(&diagnostic, "fixture gameplay failure")); OK(report_diagnostic_add(&report, diagnostic));
    }
    if(input_key_pressed_check(SDL_SCANCODE_A)) puts("KEY_DOWN");
    if(input_key_released_check(SDL_SCANCODE_A)) puts("KEY_RELEASED");
    if(graphics_viewport_ui_hovered_check(button)) puts("UI_HOVERED");
    if(graphics_viewport_ui_pressed_check(button)) puts("UI_PRESSED");
    InputTextState text = input_text_state_get();
    if(text.committed[0]) printf("TEXT:%s\n", text.committed);
    if(text.composition[0]) printf("EDIT:%s\n", text.composition);
    if(input_mouse_wheel_get().y == 2) puts("WHEEL");
    if(input_mouse_delta_get().x == 3) puts("RELATIVE");
    if(updates == 3) puts("ADVANCED");
    return report;
}
static void shutdown_scene(void *context) {
    (void)context;
    if(updates > 1) assert(renders[0] > 0 && renders[1] > 0);
    printf("SHUTDOWN updates=%u cameras=%u,%u\n", updates, renders[0], renders[1]);
}
int main(int argc, char **argv) {
    hosted = argc == 4;
    setvbuf(stdout, NULL, _IONBF, 0);
    mode = SDL_getenv("ROHR_SCENE_TEST_MODE"); if(!mode) mode = "normal";
    if(!strcmp(mode, "hang")) { SDL_Delay(30000); return 0; }
    if(!strcmp(mode, "exit")) return 7;
    if(!strcmp(mode, "bad_token") || !strcmp(mode, "bad_version") || !strcmp(mode, "oversize")) {
        assert(argc == 4 && scene_transport_start());
        SceneWire wire; scene_wire_init(&wire, scene_socket_connect((uint16_t)atoi(argv[2])));
        uint64_t deadline = SDL_GetTicks() + 5000;
        while(scene_socket_connected_get(wire.socket) == 0 && SDL_GetTicks() < deadline) SDL_Delay(1);
        char token[SCENE_TOKEN_TEXT_SIZE]; SDL_strlcpy(token, argv[3], sizeof(token));
        if(!strcmp(mode, "bad_token")) token[0] = token[0] == 'a' ? 'b' : 'a';
        assert(scene_wire_send(&wire, SCENE_HELLO, token, strlen(token)));
        if(!strcmp(mode, "bad_version")) scene_u32_write(wire.tx + 4, 123);
        if(!strcmp(mode, "oversize")) scene_u32_write(wire.tx + 12, UINT32_MAX);
        while(scene_wire_busy_check(&wire) && SDL_GetTicks() < deadline) {
            size_t budget = SCENE_PUMP_BYTES; if(!scene_wire_flush(&wire, &budget)) break; SDL_Delay(1);
        }
        SDL_Delay(100); scene_wire_destroy(&wire); scene_transport_stop(); return 0;
    }
    Report report = rohr_scene_run(argc, argv, (SceneCallbacks){NULL, initialize, update, shutdown_scene});
    if(report.header.status == ROHR_REPORT_FAILED) {
        rohr_report_console_write(&report); return 1;
    }
    return 0;
}
