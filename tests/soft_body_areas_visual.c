/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "soft_body_areas_visual.h"
#include "rohr.h"
#include "physics/soft_body/soft_body_area.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "area visual line %d: %s\n", __LINE__, #c); return false; } } while(0)
#define OK(c) CHECK(!rohr_error_check(c))

typedef struct AreaScene {
    Entity body;
    Entity area;
    Entity overlay;
    Entity nodes[9];
    Position reference[9];
    unsigned count;
} AreaScene;

typedef struct AreaVisual {
    AreaScene scenes[2];
    TextAsset labels[3];
    unsigned selected;
    double seconds;
    bool paused;
    bool high_layer;
    bool draw_failed;
} AreaVisual;

/* A scene owns its body and all nodes, beams and areas through that body. */
static bool area_scene_create(AreaScene *scene, bool layered) {
    static const Position square[] = {{-110,-110},{110,-110},{110,110},{-110,110}};
    static const Position layers[] = {
        {-180,-100},{40,-100},{40,-20},{-60,-20},{-60,110},{-180,110},
        {-100,-60},{150,-60},{60,140}};
    EntityResult body = rohr_physics_soft_body_create(); OK(body);
    scene->body = body.result.value;
    scene->count = layered ? 9 : 4;
    memcpy(scene->reference, layered ? layers : square, scene->count * sizeof(Position));
    for(unsigned i = 0; i < scene->count; i += 1) {
        EntityResult node = rohr_physics_soft_body_node_create(
            scene->body, scene->reference[i], 1, 4); OK(node);
        scene->nodes[i] = node.result.value;
        OK(rohr_graphics_layer_entity_set(node.result.value, 10));
    }
    SoftBodyAreaLoop loop = {.node_count = layered ? 6 : 4};
    memcpy(loop.nodes, scene->nodes, loop.node_count * sizeof(Entity));
    EntityResult area = rohr_physics_soft_body_area_create(scene->body, loop); OK(area);
    scene->area = area.result.value;
    if(layered) {
        EntityResult overlay = rohr_physics_soft_body_area_create(scene->body,
            (SoftBodyAreaLoop){.nodes={scene->nodes[6],scene->nodes[7],scene->nodes[8]},.node_count=3});
        OK(overlay); scene->overlay = overlay.result.value;
        OK(rohr_graphics_soft_body_area_color_set(scene->overlay,(Color){235,95,65,255}));
        OK(rohr_graphics_layer_entity_set(scene->overlay,3));
    } else {
        for(unsigned i = 0; i < 4; i += 1) {
            EntityResult edge = rohr_physics_soft_body_beam_create(
                scene->body,scene->nodes[i],scene->nodes[(i+1)%4],1,0); OK(edge);
            OK(rohr_graphics_layer_entity_set(edge.result.value,10));
        }
    }
    return true;
}

/* Kinematic movement only: no physics step and no area edits/retriangulation. */
static bool area_scene_pose_set(AreaScene *scene, bool layered, double seconds) {
    double phase = fmod(seconds,8.0);
    float swap = phase < 1 ? 0 : phase < 4 ? (float)((phase-1)/3) :
        phase < 5 ? 1 : (float)((8-phase)/3);
    for(unsigned i = 0; i < scene->count; i += 1) {
        Position moved = scene->reference[i];
        if(layered) {
            moved.x += 35 * sinf((float)seconds + (float)i * 0.65f);
            moved.y += 25 * sinf((float)seconds * 0.8f + (float)i);
            if(i == 3) moved.x += 140 * sinf((float)seconds * 0.5f);
        } else if(i < 2) {
            moved.x *= 1 - 2 * swap;
        }
        OK(rohr_physics_position_set(scene->nodes[i],moved));
    }
    return true;
}

static bool area_motion_check(AreaScene *scene) {
    EntityIndex index = rohr_entity_index_get(scene->area).result.value;
    SoftBodyAreaState authored = soft_body_area_states[index];
    const double times[] = {0,0.5,1,2.5,4,4.5,5,6.5,8,9};
    const float left_x[] = {-110,-110,-110,0,110,110,110,0,-110,-110};
    for(unsigned t = 0; t < sizeof(times)/sizeof(*times); t += 1) {
        CHECK(area_scene_pose_set(scene,false,times[t]));
        for(unsigned i = 0; i < 4; i += 1) {
            PositionResult result = rohr_physics_position_get(scene->nodes[i]); OK(result);
            Position actual = result.result.value;
            CHECK(actual.y == scene->reference[i].y);
            CHECK(actual.x == (i == 0 ? left_x[t] : i == 1 ? -left_x[t] : scene->reference[i].x));
        }
        CHECK(!memcmp(&authored,&soft_body_area_states[index],sizeof(authored)));
        CHECK(rohr_physics_soft_body_get(scene->body).result.value.areas[0] == scene->area);
    }
    return true;
}

bool soft_body_areas_visual_motion_check(void) {
    AreaScene scene = {0};
    bool passed = area_scene_create(&scene,false) && area_motion_check(&scene);
    if(scene.body != ENTITY_INVALID) (void)rohr_entity_delete(scene.body);
    return passed;
}

static void area_visual_draw(CameraId camera, void *context) {
    (void)camera;
    AreaVisual *visual = context;
    rohr_graphics_layer_active_set(-100);
    rohr_graphics_background_draw((Color){24,28,38,255});
    rohr_graphics_layer_active_set(0);
    if(!rohr_graphics_soft_body_draw(visual->scenes[visual->selected].body,
            (Color){55,150,220,255},(Color){255,255,255,255},(Color){255,255,255,255}))
        visual->draw_failed = true;
    rohr_graphics_layer_active_set(100);
    MouseButtonState button = MOUSE_BUTTON_STATE_UP;
    if(rohr_input_mouse_button_pressed_check(INPUT_MOUSE_BUTTON_LEFT)) button = MOUSE_BUTTON_STATE_PRESSED;
    else if(rohr_input_mouse_button_released_check(INPUT_MOUSE_BUTTON_LEFT)) button = MOUSE_BUTTON_STATE_RELEASED;
    else if(rohr_input_mouse_button_down_check(INPUT_MOUSE_BUTTON_LEFT)) button = MOUSE_BUTTON_STATE_DOWN;
    rohr_ui_frame_begin((UIInput){.pointer=rohr_graphics_mouse_screen_position_get(),.primary_button=button});
    for(unsigned i = 0; i < 2; i += 1) {
        UIButtonStyle style = rohr_ui_button_style_default_get();
        if(visual->selected == i) style.idle = (Color){45,100,160,255};
        UIButtonResult result = rohr_ui_button(i == 0 ? "hourglass" : "layered",
            &visual->labels[i],(UIRect){20 + i*220,20,210,40},&style);
        if(result.clicked && visual->selected != i) {
            visual->selected = i;
            visual->seconds = 0;
        }
    }
    rohr_ui_label(&visual->labels[2],(UIRect){20,70,740,30});
    rohr_ui_frame_end();
    rohr_graphics_layer_active_set(0);
}

static bool area_visual_loop(AreaVisual *visual) {
    Uint64 previous = SDL_GetTicks();
    bool running = true;
    while(running) {
        rohr_input_frame_begin();
        SDL_Event event;
        while((event = rohr_engine_event_poll()).type != 0) {
            if(event.type == SDL_EVENT_QUIT) running = false;
            if(event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) continue;
            if(event.key.scancode == SDL_SCANCODE_ESCAPE) running = false;
            if(event.key.scancode == SDL_SCANCODE_SPACE) visual->paused = !visual->paused;
            if(event.key.scancode == SDL_SCANCODE_L && visual->selected == 1) {
                visual->high_layer = !visual->high_layer;
                OK(rohr_graphics_layer_entity_set(visual->scenes[1].overlay,visual->high_layer ? 3 : -1));
            }
        }
        Uint64 now = SDL_GetTicks();
        if(!visual->paused) visual->seconds += (double)(now-previous)/1000;
        previous = now;
        CHECK(area_scene_pose_set(&visual->scenes[visual->selected],visual->selected == 1,visual->seconds));
        rohr_graphics_show();
        CHECK(!visual->draw_failed);
    }
    return true;
}

bool soft_body_areas_visual_run(void) {
    AreaVisual visual = {.high_layer=true};
    ViewportId viewport = VIEWPORT_INVALID;
    CameraId camera = CAMERA_INVALID;
    bool passed = false;
    if(rohr_error_check(rohr_graphics_start())) return false;
    if(!area_scene_create(&visual.scenes[0],false) || !area_scene_create(&visual.scenes[1],true)) goto done;
    FontAsset font = rohr_graphics_font_default_get();
    const char *labels[] = {"Hourglass", "Layered areas", "Space: pause/resume    L: switch layers    Esc: exit"};
    for(unsigned i = 0; i < 3; i += 1) {
        TextAssetResult text = rohr_graphics_text_create(&font,labels[i],(Color){255,255,255,255});
        if(rohr_error_check(text)) goto done;
        visual.labels[i] = text.result.value;
    }
    camera = rohr_camera_active_get();
    Camera view = rohr_camera_get(camera).result.value;
    view.position = (Position){0}; view.orientation = 0; view.zoom = 1;
    if(rohr_error_check(rohr_camera_set(camera,view)) ||
            rohr_error_check(rohr_camera_render_callback_set(camera,area_visual_draw,&visual))) goto done;
    ViewportConfig config = rohr_viewport_config_default_get();
    config.rectangle = (ViewportRectangle){0,0,WINDOW_WIDTH,WINDOW_HEIGHT};
    ViewportIdResult made = rohr_viewport_create(config);
    if(rohr_error_check(made)) goto done;
    viewport = made.result.value;
    if(rohr_error_check(rohr_viewport_camera_set(viewport,camera)) ||
            rohr_error_check(rohr_viewport_enable_set(viewport))) goto done;
    passed = area_visual_loop(&visual);
done:
    if(viewport != VIEWPORT_INVALID) (void)rohr_viewport_destroy(viewport);
    if(camera != CAMERA_INVALID) (void)rohr_camera_render_callback_set(camera,NULL,NULL);
    for(unsigned i = 0; i < 3; i += 1) (void)rohr_graphics_text_destroy(&visual.labels[i]);
    for(unsigned i = 0; i < 2; i += 1)
        if(visual.scenes[i].body != ENTITY_INVALID) (void)rohr_entity_delete(visual.scenes[i].body);
    rohr_graphics_stop();
    return passed;
}
