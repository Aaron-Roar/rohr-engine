/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 * Opt-in scaling diagnostic: standalone particles and three rigid walls only.
 */
#include "rohr.h"
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PIT_PARTICLES 800u
#define PIT_FRAMES 7200u
typedef struct PitSample {
    double cpu, wall, integration, gather, solve, work, render;
    uint64_t substeps, candidates, contacts;
} PitSample;
typedef struct Pit {
    Entity particles[PIT_PARTICLES], walls[3];
    Position poses[PIT_PARTICLES];
    PitSample samples[PIT_FRAMES], last;
    unsigned count, frames, completed_frames, limit;
    bool headless, paused, quit, numerical_valid, benchmark, uncapped;
    double active_wall, maximum_depth, maximum_side_escape, maximum_lag;
    uint64_t escape_samples, diagnostic_frames;
    const char *stop_reason;
    CameraId camera;
    ScreenId screen;
    ViewportId viewport;
    TextAsset text[3];
    FILE *csv;
} Pit;
static volatile sig_atomic_t interrupted;
static void interrupt_pit(int number) { (void)number; interrupted = 1; }
static bool result_check(EngineResult result) {
    if(!rohr_error_check(result)) return true;
    fprintf(stderr, "%s\n", rohr_error_message_get(result));
    return false;
}
static bool wall_create(Entity *out, Position position, double width, double height) {
    EntityResult entity = rohr_entity_add();
    if(rohr_error_check(entity)) return false;
    *out = entity.result.value;
    return result_check(rohr_physics_position_set(*out, position)) &&
        result_check(rohr_physics_hitbox_set(*out, rohr_math_square_create(width, height))) &&
        result_check(rohr_physics_mass_set(*out, 1)) &&
        result_check(rohr_physics_static_set(*out)) &&
        result_check(rohr_entity_components_add(*out, ROHR_COLLISION)) &&
        result_check(rohr_physics_friction_set(*out, .5f)) &&
        result_check(rohr_physics_restitution_set(*out, .5f));
}
static bool scene_create(Pit *p) {
    p->stop_reason = "setup_failed";
    p->frames = p->completed_frames = 0; p->last = (PitSample){0};
    p->active_wall = p->maximum_depth = p->maximum_side_escape = 0;
    p->maximum_lag = 0;
    p->escape_samples = p->diagnostic_frames = 0;
    p->numerical_valid = false;
    for(unsigned i = 0; i < p->count; ++i)
        if(p->particles[i] && !result_check(rohr_entity_delete(p->particles[i]))) return false;
    for(unsigned i = 0; i < 3; ++i)
        if(p->walls[i] && !result_check(rohr_entity_delete(p->walls[i]))) return false;
    memset(p->particles, 0, sizeof(p->particles));
    memset(p->walls, 0, sizeof(p->walls));
    if(!wall_create(&p->walls[0], (Position){0, -5}, 250, 10) ||
        !wall_create(&p->walls[1], (Position){-120, 85}, 10, 180) ||
        !wall_create(&p->walls[2], (Position){120, 85}, 10, 180)) return false;
    for(unsigned i = 0; i < p->count; ++i) {
        unsigned row = i/40, column = i%40;
        ParticleDescription d = rohr_physics_particle_description_default_get();
        d.position = (Position){(column-19.5)*5.5+(row%2)*2.75, 32.5+row*5.5};
        d.radius = 2.5f; d.rigid_vertices = 16; d.mass_value = 1;
        d.friction = .5f; d.restitution = .5f; d.gravity_enabled = true;
        EntityResult entity = rohr_physics_particle_add(d);
        if(rohr_error_check(entity)) return false;
        p->particles[i] = entity.result.value;
        p->poses[i] = d.position;
    }
    /* Prevent this diagnostic from accidentally acquiring soft-body or joint work. */
    const RohrComponentMask forbidden[] = {ROHR_SOFT_BODY, ROHR_SOFT_BODY_NODE,
        ROHR_SOFT_BODY_BEAM, ROHR_SOFT_BODY_AREA, ROHR_JOINT};
    for(unsigned i = 0; i < rohr_entity_alive_count_get(); ++i) {
        EntityResult entity = rohr_entity_alive_at_get(i);
        if(rohr_error_check(entity)) return false;
        for(unsigned j = 0; j < sizeof(forbidden)/sizeof(forbidden[0]); ++j)
            if(rohr_entity_components_check(entity.result.value, forbidden[j])) return false;
    }
    p->numerical_valid = true; p->stop_reason = NULL;
    p->paused = !p->headless && !p->benchmark;
    printf("particle_pit count=%u diameter=5 vertices=16 mass=1 friction=0.5 restitution=0.5 "
        "gravity=9.8 tick_hz=60 substeps=4 iterations=4 seconds=%.3f soft_bodies=0 joints=0\n",
        p->count, p->limit/60.0);
    fflush(stdout);
    return true;
}
static void report_add(PitSample *s, const Report *r) {
    for(size_t i = 0; i < r->statistic_count; ++i) {
        ReportStatistic metric = r->statistics[i];
        if(metric.report_id == ROHR_PHYSICS_REPORT_ID && metric.metric_id == ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT)
            s->substeps += metric.value;
        if(metric.metric_id == ROHR_REPORT_CANDIDATE_COUNT) s->candidates += metric.value;
        if(metric.metric_id == ROHR_REPORT_CONTACT_COUNT) s->contacts += metric.value;
    }
    for(size_t i = 0; i < r->entry_count; ++i) {
        ReportEntry entry = r->entries[i];
        if(entry.header.id == ROHR_PHYSICS_INTEGRATION_REPORT_ID) s->integration += entry.header.total_ms;
        if(entry.header.id == ROHR_PHYSICS_CONTACTS_REPORT_ID) s->gather += entry.header.total_ms;
        if(entry.header.id == ROHR_PHYSICS_CONSTRAINTS_REPORT_ID) s->solve += entry.header.total_ms;
    }
}
static bool step_check(Pit *p) {
    uint64_t started = SDL_GetTicksNS(); clock_t cpu = clock();
    Report report = rohr_physics_update(1);
    PitSample s = {.cpu = 1000.0*(clock()-cpu)/CLOCKS_PER_SEC,
        .wall = (SDL_GetTicksNS()-started)/1e6};
    report_add(&s, &report);
    p->samples[p->frames++] = p->last = s;
    if(report.has_fatal || report.header.status == ROHR_REPORT_FAILED) {
        p->stop_reason = "physics_failed";
        fprintf(stderr, "physics report failed at frame %u\n", p->frames);
        return false;
    }
    if(s.substeps == 0) {
        p->stop_reason = "no_completed_substeps";
        return false;
    }
    if(report.diagnostic_count) {
        if(p->numerical_valid)
            fprintf(stderr, "physics diagnostics at frame %u; continuing to endpoint\n", p->frames);
        ++p->diagnostic_frames;
        p->numerical_valid = false;
    }
    /* Checks are outside the physics timer. This is a diagnostic, not the
     * milestone's independent swept-contact/penetration acceptance fixture. */
    for(unsigned i = 0; i < p->count; ++i) {
        ParticleDescriptionResult result = rohr_physics_particle_description_get(p->particles[i]);
        if(rohr_error_check(result)) {
            p->stop_reason = "particle_state_unavailable";
            return false;
        }
        ParticleDescription d = result.result.value;
        p->poses[i] = d.position;
        if(!isfinite(d.position.x) || !isfinite(d.position.y) ||
            !isfinite(d.velocity.x) || !isfinite(d.velocity.y)) {
            p->stop_reason = "nonfinite_particle_state";
            fprintf(stderr, "invalid particle %u at frame %u: (%g,%g)\n",
                i, p->frames, d.position.x, d.position.y);
            return false;
        }
        p->maximum_depth = fmax(p->maximum_depth, fmax(0, 2.5-d.position.y));
        p->maximum_side_escape = fmax(p->maximum_side_escape, fmax(0, fabs(d.position.x)-117.5));
        if(d.position.y < -2.5f || fabs(d.position.x) > 117.5f) {
            if(p->numerical_valid)
                fprintf(stderr, "escaped particle %u at frame %u: (%g,%g); continuing to endpoint\n",
                    i, p->frames, d.position.x, d.position.y);
            ++p->escape_samples;
            p->numerical_valid = false;
        }
    }
    ++p->completed_frames;
    if(p->csv) {
        fprintf(p->csv, "%u,%.9f,%.6f,%.6f,%.6f,%.6f,%.6f,%llu,%llu,%llu\n",
            p->frames, p->frames/60.0, s.cpu, s.wall, s.integration, s.gather, s.solve,
            (unsigned long long)s.substeps, (unsigned long long)s.candidates, (unsigned long long)s.contacts);
        if(fflush(p->csv) || ferror(p->csv)) {
            p->stop_reason = "csv_write_failed";
            return false;
        }
    }
    if(p->frames%60 == 0) {
        printf("progress simulated=%.3f cpu_ms=%.3f integrate_ms=%.3f gather_ms=%.3f solve_ms=%.3f substeps=%llu\n",
            p->frames/60.0, s.cpu, s.integration, s.gather, s.solve, (unsigned long long)s.substeps);
        fflush(stdout);
    }
    return true;
}
static int number_compare(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y)-(x < y);
}
static double work_p95_get(const Pit *p) {
    double values[PIT_FRAMES];
    for(unsigned i = 0; i < p->frames; ++i) values[i] = p->samples[i].work;
    qsort(values, p->frames, sizeof(*values), number_compare);
    return p->frames ? values[(p->frames*95+99)/100-1] : 0;
}
static bool realtime_check(const Pit *p) {
    return p->benchmark && !p->uncapped && !p->headless && p->count == PIT_PARTICLES &&
        p->limit == 720 && p->completed_frames == p->limit && !p->stop_reason && !interrupted &&
        p->active_wall > 0 && work_p95_get(p) <= 16.67 &&
        fabs((p->completed_frames/60.0)/p->active_wall - 1) <= .01;
}
static void summary_print(const Pit *p) {
    double values[PIT_FRAMES], total = 0; PitSample sums = {0};
    bool complete = p->limit > 0 && p->completed_frames == p->limit && !p->stop_reason && !interrupted;
    const char *reason = p->stop_reason ? p->stop_reason : complete ? "endpoint_reached" :
        interrupted ? "interrupted" : p->quit ? "window_closed" : "endpoint_not_reached";
    for(unsigned i = 0; i < p->frames; ++i) {
        const PitSample *s = &p->samples[i]; values[i] = s->cpu; total += s->cpu;
        sums.integration += s->integration; sums.gather += s->gather; sums.solve += s->solve;
        sums.substeps += s->substeps; sums.candidates += s->candidates; sums.contacts += s->contacts;
    }
    qsort(values, p->frames, sizeof(*values), number_compare);
    printf("result count=%u frames=%u complete=%d simulated=%.9f active_wall=%.6f "
        "physics_cpu_ms=%.3f median_ms=%.3f p95_ms=%.3f max_ms=%.3f "
        "floor_circle_depth=%.9f side_escape=%.9f escape_samples=%llu diagnostic_frames=%llu\n",
        p->count, p->frames, complete,
        p->completed_frames/60.0, p->active_wall, total, p->frames ? values[(p->frames-1)/2] : 0,
        p->frames ? values[(p->frames*95+99)/100-1] : 0, p->frames ? values[p->frames-1] : 0,
        p->maximum_depth, p->maximum_side_escape,
        (unsigned long long)p->escape_samples, (unsigned long long)p->diagnostic_frames);
    printf("completion condition=time target=%.9f reached=%d reason=%s measurements=%s\n",
        p->limit/60.0, complete, reason, complete ? "full_run" : "partial_run");
    printf("verdict=%s numerical_valid=%d\n", !complete ? "INCOMPLETE" :
        p->numerical_valid && !p->stop_reason ? "PASS" : "FAIL", p->numerical_valid);
    printf("profile_ms integration=%.3f gather=%.3f solve=%.3f substeps=%llu candidates=%llu contacts=%llu\n",
        sums.integration, sums.gather, sums.solve, (unsigned long long)sums.substeps,
        (unsigned long long)sums.candidates, (unsigned long long)sums.contacts);
    if(p->benchmark) {
        double render_total = 0;
        for(unsigned i = 0; i < p->frames; ++i) render_total += p->samples[i].render;
        printf("benchmark rendered=%d uncapped=%d frame_work_p95_ms=%.6f render_total_ms=%.6f "
            "fps=%.6f simulated_elapsed_ratio=%.9f final_lag_ms=%.6f maximum_lag_ms=%.6f realtime_pass=%d\n",
            !p->headless, p->uncapped, work_p95_get(p), render_total,
            p->active_wall > 0 ? p->completed_frames/p->active_wall : 0,
            p->active_wall > 0 ? (p->completed_frames/60.0)/p->active_wall : 0,
            1000*(p->active_wall-p->completed_frames/60.0), 1000*p->maximum_lag, realtime_check(p));
    }
    puts("CPU excludes validation/drawing; frame work excludes pacing. Stage timings are inclusive wall time. Headless/uncapped/short runs are diagnostic; one run is not milestone acceptance.");
    fflush(stdout);
}
static void shape_draw(Shape shape, Position center, Color color) {
    for(unsigned i = 0; i < shape.amount_of_vertices; ++i) {
        shape.vertices[i].x = 640+3*(shape.vertices[i].x+center.x);
        shape.vertices[i].y = 605-3*(shape.vertices[i].y+center.y);
    }
    rohr_graphics_screen_shape_filled_draw(shape, color);
}
static void render(CameraId camera, void *context) {
    (void)camera; Pit *p = context;
    rohr_graphics_background_draw((Color){18,24,34,255});
    shape_draw(rohr_math_square_create(250,10), (Position){0,-5}, (Color){95,105,120,255});
    shape_draw(rohr_math_square_create(10,180), (Position){-120,85}, (Color){95,105,120,255});
    shape_draw(rohr_math_square_create(10,180), (Position){120,85}, (Color){95,105,120,255});
    Shape circle = rohr_math_circle_create(2.5f,16);
    for(unsigned i = 0; i < p->count; ++i)
        shape_draw(circle, p->poses[i], i/40%2 ? (Color){242,198,90,255} : (Color){87,179,235,255});
    for(unsigned i = 0; i < 3; ++i) rohr_graphics_text_draw(&p->text[i], (Position){15,630+i*28});
}
static bool graphics_create(Pit *p) {
    WindowPresentationDescription w = rohr_graphics_window_presentation_description_get();
    w.mode = GRAPHICS_WINDOW_MODE_WINDOWED; w.window_width = w.logical_width = 1280;
    w.window_height = w.logical_height = 720;
    if(!result_check(rohr_graphics_window_presentation_description_set(w)) ||
        !result_check(rohr_graphics_vsync_set(false)) || !result_check(rohr_graphics_frame_limit_set(p->uncapped ? 0 : 60))) return false;
    int count = 0; SDL_Window **windows = SDL_GetWindows(&count);
    if(windows && count) SDL_SetWindowTitle(windows[0], "Particle-only rigid pit measurement");
    SDL_free(windows);
    for(unsigned i = 0; i < 3; ++i) {
        TextDescription d = rohr_graphics_text_description_default_get(); d.value = "";
        TextAssetResult t = rohr_graphics_text_create(d);
        if(rohr_error_check(t)) return false;
        p->text[i] = t.result.value;
    }
    CameraDescription c = rohr_camera_description_default_get();
    c.dimensions = (Vec2D){1280,720}; c.render_callback = render; c.render_context = p;
    CameraIdResult camera = rohr_camera_create(c); if(rohr_error_check(camera)) return false;
    p->camera = camera.result.value;
    ScreenDescription s = rohr_screen_description_default_get(); s.camera = p->camera; s.width = 1280; s.height = 720;
    ScreenIdResult screen = rohr_screen_create(s); if(rohr_error_check(screen)) return false;
    p->screen = screen.result.value;
    ViewportDescription v = rohr_viewport_description_default_get(); v.rectangle = (ViewportRectangle){0,0,1280,720};
    ViewportIdResult viewport = rohr_viewport_create(v); if(rohr_error_check(viewport)) return false;
    p->viewport = viewport.result.value;
    ViewportItemDescription item = rohr_viewport_item_description_default_get(); item.rectangle = v.rectangle;
    return !rohr_error_check(rohr_viewport_screen_add(p->viewport,p->screen,item)) &&
        result_check(rohr_viewport_enable_set(p->viewport));
}
static bool draw_check(Pit *p, double *wait_ms) {
    uint64_t started = SDL_GetTicksNS();
    char text[384];
    snprintf(text, sizeof(text), "%u particles | diameter 5 | restitution 0.5 | %s | simulated %.2f / %.2f s | active wall %.2f s",
        p->count, p->paused ? "PAUSED" : "RUNNING", p->completed_frames/60.0, p->limit/60.0, p->active_wall);
    if(!rohr_graphics_text_value_set(&p->text[0],text)) return false;
    snprintf(text, sizeof(text), "Physics CPU %.2f ms | integrate %.2f / gather %.2f / solve %.2f ms wall | substeps %llu",
        p->last.cpu, p->last.integration, p->last.gather, p->last.solve, (unsigned long long)p->last.substeps);
    if(!rohr_graphics_text_value_set(&p->text[1],text) ||
        !rohr_graphics_text_value_set(&p->text[2], "Space play/pause | N step | R reset | Esc close | Live fixed steps: playback slows when CPU-bound.")) return false;
    Report report = rohr_graphics_show();
    for(size_t i = 0; i < report.statistic_count; ++i)
        if(report.statistics[i].metric_id == ROHR_REPORT_FRAME_LIMIT_WAIT_NS)
            *wait_ms += report.statistics[i].value/1e6;
    p->last.render = (SDL_GetTicksNS()-started)/1e6 - *wait_ms;
    return report.header.status != ROHR_REPORT_FAILED;
}
int main(int argc, char **argv) {
    Pit *p = calloc(1,sizeof(*p)); if(!p) return 1;
    p->count = PIT_PARTICLES; p->limit = 720; p->paused = true;
    p->stop_reason = "setup_failed";
    bool smoke = false, graphics = false, valid = true; int status = 1;
    const char *csv_path = NULL;
    for(int i = 1; i < argc; ++i) {
        if(!strcmp(argv[i],"--headless")) p->headless = true;
        else if(!strcmp(argv[i],"--benchmark=rendered")) p->benchmark = true;
        else if(!strcmp(argv[i],"--uncapped")) p->uncapped = true;
        else if(!strcmp(argv[i],"--smoke")) smoke = true;
        else if(!strncmp(argv[i],"--csv=",6)) csv_path = argv[i]+6;
        else if(!strncmp(argv[i],"--count=",8)) {
            char *end; unsigned long count = strtoul(argv[i]+8,&end,10);
            if(*end || !count || count > PIT_PARTICLES) valid = false;
            else p->count = count;
        } else if(!strncmp(argv[i],"--seconds=",10)) {
            char *end; double seconds = strtod(argv[i]+10,&end);
            if(*end || !isfinite(seconds) || seconds < 1.0/60 || seconds > 120) valid = false;
            else p->limit = (unsigned)ceil(seconds*60);
        } else valid = false;
    }
    if((p->benchmark && p->headless) || (p->uncapped && !p->benchmark)) valid = false;
    if(!valid) { fprintf(stderr,"usage: particle_pit [--headless|--benchmark=rendered [--uncapped]] [--count=1..800] [--seconds=0.017..120] [--csv=PATH] [--smoke]\n"); free(p); return 2; }
    if(smoke) p->limit = 6;
    if(csv_path) {
        p->csv = fopen(csv_path,"w"); if(!p->csv) { perror(csv_path); free(p); return 1; }
        fprintf(p->csv,"frame,simulated_seconds,physics_cpu_ms,physics_wall_ms,integration_wall_ms,gather_wall_ms,solve_wall_ms,substeps,candidates,contacts\n");
    }
    signal(SIGINT,interrupt_pit); signal(SIGTERM,interrupt_pit);
    if(!result_check(rohr_engine_start())) { summary_print(p); goto finish; }
    if(!result_check(rohr_engine_time_per_tick_set(1.0/60)) ||
        !result_check(rohr_physics_substeps_set(4)) || !result_check(rohr_physics_solver_iterations_set(4)) ||
        !result_check(rohr_physics_gravity_set((Acceleration){0,-9.8f}))) goto done;
    rohr_physics_engine_time_per_tick_use();
    if(!p->headless) {
        if(!result_check(rohr_graphics_start())) goto done;
        graphics = true; if(!graphics_create(p)) goto done;
    }
    if(!scene_create(p)) goto done;
    if(smoke) p->paused = false;
    while(!p->quit && !interrupted) {
        uint64_t frame_start = SDL_GetTicksNS(); bool single = false;
        if(!p->headless) {
            rohr_input_frame_begin(); SDL_Event event;
            while((event = rohr_engine_event_poll()).type) {
                if(event.type == SDL_EVENT_QUIT) p->quit = true;
                if(event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) continue;
                if(event.key.scancode == SDL_SCANCODE_ESCAPE) p->quit = true;
                if(p->benchmark) continue;
                if(event.key.scancode == SDL_SCANCODE_SPACE) p->paused = !p->paused;
                if(event.key.scancode == SDL_SCANCODE_N) { single = true; p->paused = true; }
                if(event.key.scancode == SDL_SCANCODE_R) {
                    if(p->completed_frames != p->limit) p->stop_reason = "user_reset";
                    summary_print(p);
                    if(!scene_create(p)) goto done;
                }
            }
        }
        bool stepping = !p->paused || single;
        if(stepping && p->frames < p->limit && !p->quit) {
            if(!step_check(p)) {
                p->active_wall += (SDL_GetTicksNS()-frame_start)/1e9;
                goto done;
            }
            if(p->frames == p->limit) p->paused = true;
        }
        double wait_ms = 0;
        if(!p->headless && !draw_check(p, &wait_ms)) { p->stop_reason = "graphics_failed"; goto done; }
        if(stepping) {
            double elapsed = (SDL_GetTicksNS()-frame_start)/1e9;
            p->active_wall += elapsed;
            p->maximum_lag = fmax(p->maximum_lag, p->active_wall-p->completed_frames/60.0);
            if(p->frames) {
                p->last.work = elapsed*1000-wait_ms;
                p->samples[p->frames-1].work = p->last.work;
                p->samples[p->frames-1].render = p->last.render;
            }
        }
        if((p->headless || smoke || p->benchmark) && p->frames == p->limit) break;
    }
    status = interrupted ? 130 : (!p->numerical_valid || p->stop_reason ||
        ((p->headless || smoke || p->benchmark) && p->completed_frames != p->limit) ||
        (p->benchmark && !p->uncapped && !smoke && p->limit == 720 && p->count == PIT_PARTICLES && !realtime_check(p)) ? 1 : 0);
done:
    summary_print(p);
    if(graphics) {
        if(p->viewport) rohr_viewport_destroy(p->viewport);
        if(p->screen) rohr_screen_destroy(p->screen);
        if(p->camera) rohr_camera_destroy(p->camera);
        for(unsigned i = 0; i < 3; ++i) rohr_graphics_text_destroy(&p->text[i]);
        rohr_graphics_stop();
    }
    rohr_engine_stop();
finish:
    if(p->csv && fclose(p->csv)) status = 1;
    free(p); return status;
}
