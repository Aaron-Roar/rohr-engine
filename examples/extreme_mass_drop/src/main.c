/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "drop_fixture.h"
#include "example_viewport.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define TEXT_COUNT 13

typedef struct DropDemo {
    DropFixture fixture;
    ViewportId viewports[3];
    TextAsset text[TEXT_COUNT];
    ViewportItemId snapshots[2][DROP_BODY_COUNT];
    ViewportItemId labels[TEXT_COUNT];
    ViewportItemId warning;
    bool composed;
    bool paused;
    bool quit;
    bool failed;
    double pending;
} DropDemo;

static const Color colors[] = {{105, 125, 150, 255}, {255, 210, 60, 255}, {70, 175, 240, 255}};
static const unsigned rates[] = {30, 60, 120};

/* Diagnostic rectangles describe the last finite observation, not live bodies.
 * Keeping these as viewport UI preserves a halted snapshot without adding
 * entities or changing the physics fixture. Each view keeps its true scale. */
static bool snapshot_update(DropDemo *demo) {
    for(unsigned view=0; view<2; ++view) {
        const double width=view==0 ? 2240.0 : 0.08;
        const double height=view==0 ? 1400.0 : 0.05;
        const double camera_y=view==0 ? 550.0 : 0.0125;
        for(size_t i=0; i<DROP_BODY_COUNT; ++i) {
            const DropBody *body=&demo->fixture.bodies[i];
            ViewportItemDescriptionResult current=rohr_viewport_item_description_get(demo->snapshots[view][i]);
            if(rohr_error_check(current)) return false;
            ViewportItemDescription item=current.result.value;
            item.rectangle.x=(float)(320+body->position.x*640/width);
            item.rectangle.y=(float)(200-(body->position.y-camera_y)*400/height);
            item.orientation=body->angle;
            item.content_scale=(Scale){(float)(body->half_width*2*640/width),
                (float)(body->half_height*2*400/height)};
            if(!drop_result_check(rohr_viewport_item_description_set(demo->snapshots[view][i],item))) return false;
        }
    }
    ViewportItemDescriptionResult warning=rohr_viewport_item_description_get(demo->warning);
    if(rohr_error_check(warning)) return false;
    warning.result.value.visible=demo->fixture.maximum_depth>DROP_PENETRATION_LIMIT ||
        demo->fixture.halted || demo->fixture.floor_moved;
    return drop_result_check(rohr_viewport_item_description_set(demo->warning,warning.result.value));
}

static bool text_set(DropDemo *demo, size_t index, const char *value) {
    if(rohr_graphics_text_value_set(&demo->text[index], value)) {
        ViewportItemDescriptionResult item=rohr_viewport_item_description_get(demo->labels[index]);
        if(rohr_error_check(item)) return false;
        ViewportUiTextDescription description={.text=demo->text[index],.scale={1,1},
            .position={demo->text[index].size.x*.5f,demo->text[index].size.y*.5f}};
        return drop_result_check(rohr_graphics_ui_text_description_set(item.result.value.ui,description));
    }
    fprintf(stderr, "Unable to update demo text\n");
    return false;
}

static bool display_update(DropDemo *demo) {
    char line[512];
    DropFixture *f = &demo->fixture;
    snprintf(line, sizeof(line), "%s | CASE %u | %u Hz | t=%.6f s | %u updates | peak sampled depth %.9g (limit %.9g)",
        f->halted ? "HALTED - R to restart" : demo->paused ? "PAUSED" : "RUNNING",
        f->case_index + 1, f->rate, f->time, f->steps, f->maximum_depth, DROP_PENETRATION_LIMIT);
    if(!text_set(demo, 2, line)) return false;
    for(size_t i = 0; i < DROP_BODY_COUNT; ++i) {
        DropBody *b = &f->bodies[i];
        snprintf(line, sizeof(line), "%s  position=(% .9g, % .9g)  velocity=(% .7g, % .7g)  angle=% .7g deg  omega=% .7g deg/s",
            i == 0 ? "FLOOR" : i == 1 ? "TINY " : "LARGE", b->position.x, b->position.y,
            b->velocity.x, b->velocity.y, b->angle, b->angular_velocity);
        if(!text_set(demo, 3 + i, line)) return false;
    }
    snprintf(line, sizeof(line), "Geometric depths: floor/tiny=%.9g  tiny/large=%.9g  floor/large=%.9g",
        f->depths[0], f->depths[1], f->depths[2]);
    if(!text_set(demo, 6, line)) return false;
    if(f->halted) snprintf(line, sizeof(line), "STOPPED: %s", f->failure);
    else if(f->first_violation_time >= 0)
        snprintf(line, sizeof(line), "VIOLATION retained: first penetration >%.9g at t=%.6f s. Floor moved: %s. Motion remains unrestricted.",
            DROP_PENETRATION_LIMIT, f->first_violation_time, f->floor_moved ? "YES" : "no");
    else snprintf(line, sizeof(line), "No penetration violation observed yet. Floor moved: %s. This is not physics acceptance.",
        f->floor_moved ? "YES" : "no");
    return text_set(demo, 7, line);
}

static bool presentation_create(DropDemo *demo) {
    const char *labels[TEXT_COUNT] = {
        "OVERVIEW - true scale; tiny body is below pixel size",
        "CONTACT VIEW - width 0.08, height 0.05 world units",
        "", "", "", "", "", "",
        "Space: pause/run   N: one update while paused   R: restart   Esc: exit",
        "1: centered   2/3: +/-0.0025   4/5: +/-0.01 tipping   Floor width: 0.01 in all cases",
        "F1/F2/F3: 30/60/120 Hz (restarts paused). Yellow: tiny body. Blue: large body. Gray: floor.",
        "Gravity -9.8 | friction 0.8 | restitution 0 | adaptive substeps | 8 solver iterations | unrestricted motion",
        "Centered: settle within 2 s of impact; hold 10 s | speed <=0.0001 units/s, spin <=0.0001 deg/s"
    };
    for(size_t i = 0; i < TEXT_COUNT; ++i) {
        TextDescription description = rohr_graphics_text_description_default_get();
        description.value = labels[i];
        TextAssetResult result = rohr_graphics_text_create(description);
        if(rohr_error_check(result)) return false;
        demo->text[i] = result.result.value;
    }
    for(unsigned i=0; i<3; ++i) {
        ViewportCreateDescription description={.viewport=rohr_viewport_description_default_get()};
        description.viewport.rectangle=(ViewportRectangle){i==1 ? 640 : 0,i==2 ? 400 : 0,
            i==2 ? 1280 : 640,i==2 ? 320 : 400};
        description.viewport.background_color=(Color){18,24,34,255};
        description.viewport.enabled=true;
        ViewportIdResult created=rohr_viewport_create(description);
        if(rohr_error_check(created)) return false;
        demo->viewports[i]=created.result.value;
        if(i<2) {
            for(size_t b=0; b<DROP_BODY_COUNT; ++b) {
                /* Validate a unit square, then scale its placement. The tiny
                 * body's true overview size is below geometry tolerances. */
                demo->snapshots[i][b]=example_rectangle_add(demo->viewports[i],
                    (UIRect){0,0,1,1},colors[b],0);
                if(!demo->snapshots[i][b]) return false;
            }
            if(!example_rectangle_add(demo->viewports[i],(UIRect){0,0,640,32},(Color){18,24,34,235},10)) return false;
        } else {
            demo->warning=example_rectangle_add(demo->viewports[i],(UIRect){0,0,1280,30},(Color){125,35,40,255},0);
            if(!demo->warning) return false;
        }
    }
    for(size_t i=0; i<TEXT_COUNT; ++i) {
        ViewportItemDescription placement=rohr_viewport_item_description_default_get();
        placement.fit=SCREEN_FIT_NONE; placement.layer=20;
        placement.rectangle=(ViewportRectangle){12,i<2 ? 10 : 9+(float)(i-2)*27,0,0};
        ViewportUiTextDescription text={.text=demo->text[i],.scale={1,1},
            .position={demo->text[i].size.x*.5f,demo->text[i].size.y*.5f}};
        ViewportItemIdResult added=rohr_viewport_ui_text_add(demo->viewports[i<2 ? i : 2],text,placement);
        if(rohr_error_check(added)) return false;
        demo->labels[i]=added.result.value;
    }
    return display_update(demo);
}

static void presentation_destroy(DropDemo *demo) {
    for(size_t i = 0; i < 3; ++i) {
        if(demo->viewports[i]) (void)drop_result_check(rohr_viewport_destroy(demo->viewports[i]));
    }
    for(size_t i = 0; i < TEXT_COUNT; ++i)
        (void)drop_result_check(rohr_graphics_text_destroy(&demo->text[i]));
}

static void restart(DropDemo *demo, unsigned case_index, unsigned rate) {
    demo->paused = true;
    demo->pending = 0;
    if(!drop_fixture_reset(&demo->fixture, case_index, rate)) demo->failed = true;
    drop_fixture_print(&demo->fixture);
}

static void events_process(DropDemo *demo) {
    SDL_Event event;
    rohr_input_frame_begin();
    while((event = rohr_engine_event_poll()).type != 0) {
        if(event.type == SDL_EVENT_QUIT) demo->quit = true;
        if(event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) continue;
        SDL_Scancode key = event.key.scancode;
        if(key == SDL_SCANCODE_ESCAPE) demo->quit = true;
        else if(key == SDL_SCANCODE_SPACE && !demo->fixture.halted) {
            demo->paused = !demo->paused;
            demo->pending = 0;
        } else if(key == SDL_SCANCODE_N && demo->paused && !demo->fixture.halted)
            (void)drop_fixture_step(&demo->fixture);
        else if(key == SDL_SCANCODE_R)
            restart(demo, demo->fixture.case_index, demo->fixture.rate);
        else if(key >= SDL_SCANCODE_1 && key <= SDL_SCANCODE_5)
            restart(demo, (unsigned)(key - SDL_SCANCODE_1), demo->fixture.rate);
        else if(key >= SDL_SCANCODE_F1 && key <= SDL_SCANCODE_F3)
            restart(demo, demo->fixture.case_index, rates[key - SDL_SCANCODE_F1]);
    }
}

static bool frame_run(DropDemo *demo, double elapsed) {
    if(demo->failed) return false;
    if(!demo->paused && !demo->fixture.halted) {
        demo->pending += elapsed;
        double dt = 1.0 / demo->fixture.rate;
        while(demo->pending >= dt) {
            if(!drop_fixture_step(&demo->fixture)) break;
            demo->pending -= dt;
        }
    }
    if(!display_update(demo) || !snapshot_update(demo)) return false;
    Report frame = rohr_graphics_viewports_show(demo->viewports,3);
    if(frame.header.status == ROHR_REPORT_FAILED) {
        fprintf(stderr, "Presentation failed: %s\n", frame.has_fatal ? frame.first_fatal.detail : "see graphics diagnostics");
        return false;
    }
    for(size_t i=0; i<frame.statistic_count; ++i)
        if(frame.statistics[i].report_id==ROHR_GRAPHICS_VIEWPORTS_REPORT_ID &&
                frame.statistics[i].metric_id==ROHR_REPORT_PROCESSED_COUNT &&
                /* Thirteen labels, six snapshots and two title bars across
                 * the three viewports; the warning banner is optional. */
                frame.statistics[i].value>=TEXT_COUNT+2*DROP_BODY_COUNT+2) demo->composed=true;
    return true;
}

static bool key_send(DropDemo *demo, SDL_Scancode key) {
    SDL_Event event;
    SDL_zero(event);
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.scancode = key;
    event.key.down = true;
    event.key.repeat = false;
    if(!SDL_PushEvent(&event)) {
        fprintf(stderr, "Unable to queue test key down: %s\n", SDL_GetError());
        return false;
    }
    event.type = SDL_EVENT_KEY_UP;
    event.key.down = false;
    if(!SDL_PushEvent(&event)) {
        fprintf(stderr, "Unable to queue test key up: %s\n", SDL_GetError());
        return false;
    }
    events_process(demo);
    /* SDL may have queued a poll sentinel before our injected key events.
     * Consume the next event batch too when those keys remain queued. */
    if(SDL_HasEvents(SDL_EVENT_KEY_DOWN, SDL_EVENT_KEY_UP)) events_process(demo);
    return !demo->failed;
}

static bool controls_verify(DropDemo *demo) {
    /* Exercise actual SDL event dispatch and the same frame/step path as the UI. */
    for(unsigned c = 0; c < DROP_CASE_COUNT; ++c) {
        for(unsigned r = 0; r < 3; ++r) {
            if(!key_send(demo, (SDL_Scancode)(SDL_SCANCODE_1 + c)) ||
                    !key_send(demo, (SDL_Scancode)(SDL_SCANCODE_F1 + r)) ||
                    !demo->paused || demo->fixture.steps != 0 ||
                    demo->fixture.case_index != c || demo->fixture.rate != rates[r] ||
                    !key_send(demo, SDL_SCANCODE_N) || demo->fixture.steps != 1 ||
                    !key_send(demo, SDL_SCANCODE_SPACE) || demo->paused ||
                    !frame_run(demo, 1.0 / rates[r]) || demo->fixture.steps != 2 ||
                    !key_send(demo, SDL_SCANCODE_SPACE) || !demo->paused ||
                    !frame_run(demo, 1.0) || demo->fixture.steps != 2 ||
                    !key_send(demo, SDL_SCANCODE_R) || demo->fixture.steps != 0 ||
                    !frame_run(demo, 0)) return false;
        }
    }
    if(!demo->composed) return false;
    /* Background/return events must not discard a delayed running interval. */
    if(!key_send(demo, SDL_SCANCODE_F2) || !key_send(demo, SDL_SCANCODE_SPACE)) return false;
    SDL_Event focus;
    SDL_zero(focus);
    focus.type = SDL_EVENT_WINDOW_FOCUS_LOST;
    if(!SDL_PushEvent(&focus)) return false;
    events_process(demo);
    events_process(demo);
    if(!frame_run(demo, 0.1)) return false;
    focus.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
    if(!SDL_PushEvent(&focus)) return false;
    events_process(demo);
    events_process(demo);
    if(!frame_run(demo, 1.0) || demo->paused || demo->fixture.halted ||
            fabs(demo->fixture.time + demo->pending - 1.1) > 1e-9 ||
            demo->fixture.maximum_depth > DROP_PENETRATION_LIMIT ||
            demo->fixture.floor_moved || !key_send(demo, SDL_SCANCODE_R)) return false;
    /* Force an invalid duration through the real step path. The public pipeline
     * rejects it; the demo must retain the last snapshot and require restart. */
    demo->fixture.rate = 0;
    if(drop_fixture_step(&demo->fixture) || !demo->fixture.halted ||
            demo->fixture.steps != 0 || demo->fixture.failure[0] == '\0') return false;
    demo->fixture.rate = 120;
    if(!key_send(demo, SDL_SCANCODE_SPACE) || !key_send(demo, SDL_SCANCODE_N) ||
            demo->fixture.steps != 0 || !frame_run(demo, 0) ||
            !key_send(demo, SDL_SCANCODE_R) || demo->fixture.halted ||
            !key_send(demo, SDL_SCANCODE_ESCAPE) || !demo->quit) return false;
    puts("PASS: SDL controls, focus events and delayed catch-up, restart, pause/step, all cases/rates, three composed views, halted-state recovery, exit.");
    return true;
}

int main(int argc, char **argv) {
    bool verify = argc == 2 && strcmp(argv[1], "--verify") == 0;
    bool smoke = argc == 2 && strcmp(argv[1], "--smoke") == 0;
    if(argc > 1 && !verify && !smoke) {
        fprintf(stderr, "Usage: extreme_mass_drop [--verify|--smoke]\n");
        return 2;
    }
    DropDemo demo = {.paused = true};
    int status = 1;
    bool graphics_started = false;
    if(!drop_result_check(rohr_directory_working_set(rohr_directory_base_get())) ||
            !drop_result_check(rohr_engine_start())) return 1;
    if(verify) {
        status = drop_fixture_verify() ? 0 : 1;
        goto done;
    }
    if(!drop_result_check(rohr_graphics_start())) goto done;
    graphics_started = true;
    if(!drop_result_check(rohr_graphics_frame_limit_set(smoke ? 0 : 60)) ||
            !drop_fixture_reset(&demo.fixture, 0, 60) || !presentation_create(&demo)) goto done;
    puts("Extreme mass/size drop: user-reviewed motion; centered settling limits accepted.");
    puts("Space pause/run | N single step | R restart | 1 centered | 2/3 +/-0.0025 | 4/5 +/-0.01 tipping | F1/F2/F3 30/60/120 Hz | Esc exit");
    puts("All five cases use a 0.01-wide static floor, matching the yellow body.");
    puts("All cases start paused. Penetration violations persist until restart; no automatic settling/freezing.");
    drop_fixture_print(&demo.fixture);
    if(smoke) { status = controls_verify(&demo) ? 0 : 1; goto done; }
    uint64_t previous = SDL_GetPerformanceCounter();
    double frequency = (double)SDL_GetPerformanceFrequency();
    while(!demo.quit) {
        uint64_t now = SDL_GetPerformanceCounter();
        double elapsed = (double)(now - previous) / frequency;
        previous = now;
        bool previously_paused = demo.paused;
        events_process(&demo);
        if(demo.quit) break;
        /* Paused wall time is not simulation time. Running catch-up retains
         * every elapsed update; this observer is not a timing benchmark. */
        if(!frame_run(&demo, previously_paused ? 0 : elapsed)) goto done;
    }
    status = 0;
done:
    if(graphics_started) presentation_destroy(&demo);
    drop_fixture_destroy(&demo.fixture);
    if(graphics_started) rohr_graphics_stop();
    rohr_engine_stop();
    return status;
}
