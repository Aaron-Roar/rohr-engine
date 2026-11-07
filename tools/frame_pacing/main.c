/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FRAME_COUNT 600
static Uint64 animation_start;
static bool draw_ok;
static volatile sig_atomic_t interrupted;
static void interrupt_run(int signal_number) { (void)signal_number; interrupted = 1; }

static bool result_check(EngineResult result) {
    if(!rohr_error_check(result)) return true;
    fprintf(stderr, "%s\n", error_result_message_get(result.kind, result.result.error));
    return false;
}
static void render(CameraId camera, void *context) {
    (void)camera; (void)context;
    double seconds = (SDL_GetTicksNS() - animation_start) / 1e9;
    rohr_graphics_background_draw((Color){18, 24, 34, 255});
    draw_ok = true;
    for(int row = 0; row < 3; ++row) {
        float x = 40 + (float)fmod(seconds * (120 + row * 120), 1160);
        draw_ok &= rohr_graphics_screen_rect_draw(x, 100 + row * 180, 40, 100,
            (Color){(Uint8)(100 + row * 65), 200, 230, 255});
    }
}
static int number_compare(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
int main(int argc, char **argv) {
    signal(SIGINT, interrupt_run);
    signal(SIGTERM, interrupt_run);
    bool vsync = false, stall = false, toggle = false;
    for(int i = 1; i < argc; ++i) {
        if(!strcmp(argv[i], "--vsync")) vsync = true;
        else if(!strcmp(argv[i], "--stall")) stall = true;
        else if(!strcmp(argv[i], "--toggle")) toggle = true;
        else { fprintf(stderr, "usage: frame_pacing [--vsync] [--stall] [--toggle]\n"); return 2; }
    }
    if(!result_check(rohr_directory_working_set(rohr_directory_base_get())) ||
       !result_check(rohr_engine_start())) return 2;
    if(!result_check(rohr_graphics_start())) { rohr_engine_stop(); return 2; }
    int status = 2;
    if(!result_check(rohr_graphics_vsync_set(vsync)) ||
       !result_check(rohr_graphics_frame_limit_set(60))) goto done;
    WindowPresentationDescription window = rohr_graphics_window_presentation_description_get();
    window.window_width = window.logical_width = 1280;
    window.window_height = window.logical_height = 720;
    window.mode = GRAPHICS_WINDOW_MODE_WINDOWED;
    if(!result_check(rohr_graphics_window_presentation_description_set(window))) goto done;
    int window_count = 0;
    SDL_Window **windows = SDL_GetWindows(&window_count);
    if(windows && window_count) SDL_SetWindowTitle(windows[0], "Frame pacing review - moving bars - Escape closes");
    SDL_free(windows);
    CameraId camera = rohr_camera_active_get();
    ViewportIdResult viewport = rohr_viewport_create(rohr_viewport_description_default_get());
    if(rohr_error_check(viewport)) { fprintf(stderr, "%s\n", rohr_error_message_get(viewport)); goto done; }
    if(!result_check(rohr_viewport_camera_set(viewport.result.value, camera)) ||
       !result_check(rohr_viewport_enable_set(viewport.result.value)) ||
       !result_check(rohr_camera_render_callback_set(camera, render, NULL))) goto done;
    animation_start = SDL_GetTicksNS();
    Report report = rohr_graphics_show(); /* Warm up outside the measured endpoint. */
    if(report.has_fatal || !draw_ok) goto done;
    double intervals[FRAME_COUNT], work[FRAME_COUNT], waits[FRAME_COUNT];
    Uint64 start = SDL_GetTicksNS(), previous = start;
    unsigned frames = 0, focus_gained = 0, focus_lost = 0;
    bool quit = false, healthy = true;
    for(; frames < FRAME_COUNT && !quit && !interrupted;) {
        rohr_input_frame_begin();
        SDL_Event event;
        while((event = rohr_engine_event_poll()).type != 0) {
            if(event.type == SDL_EVENT_QUIT) quit = true;
            if(event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) ++focus_gained;
            if(event.type == SDL_EVENT_WINDOW_FOCUS_LOST) ++focus_lost;
        }
        if(rohr_input_key_pressed_check(SDL_SCANCODE_ESCAPE)) quit = true;
        if(quit) break;
        if(stall && frames == 120) SDL_Delay(200);
        if(toggle && (frames == 200 || frames == 400)) {
            vsync = !vsync;
            if(!result_check(rohr_graphics_vsync_set(vsync))) { healthy = false; break; }
        }
        report = rohr_graphics_show();
        if(report.has_fatal || !draw_ok) { healthy = false; break; }
        Uint64 now = SDL_GetTicksNS(), wait = 0;
        for(size_t i = 0; i < report.statistic_count; ++i) {
            ReportStatistic s = report.statistics[i];
            if(s.report_id == ROHR_GRAPHICS_PRESENTATION_REPORT_ID && s.metric_id == ROHR_REPORT_FRAME_LIMIT_WAIT_NS)
                wait += s.value;
        }
        intervals[frames] = (now - previous) / 1e6;
        waits[frames] = wait / 1e6;
        work[frames] = intervals[frames] - waits[frames];
        previous = now;
        ++frames;
    }
    double elapsed = (previous - start) / 1e9;
    printf("pacing complete=%d frames=%u target=%d elapsed=%.9f fps=%.6f vsync_final=%d stall=%d toggle=%d focus_gained=%u focus_lost=%u\n",
        frames == FRAME_COUNT && healthy, frames, FRAME_COUNT, elapsed,
        elapsed > 0 ? frames / elapsed : 0, vsync, stall, toggle, focus_gained, focus_lost);
    for(unsigned i = 0; i < frames; ++i)
        printf("frame index=%u interval_ms=%.6f software_wait_ms=%.6f nonwait_ms=%.6f\n", i, intervals[i], waits[i], work[i]);
    if(frames) {
        qsort(intervals, frames, sizeof(*intervals), number_compare);
        printf("interval_ms median=%.6f p95=%.6f max=%.6f\n", intervals[(frames-1)/2],
            intervals[(frames*95+99)/100-1], intervals[frames-1]);
    }
    status = frames == FRAME_COUNT && healthy ? 0 : 1;
done:
    rohr_graphics_stop();
    rohr_engine_stop();
    return status;
}
