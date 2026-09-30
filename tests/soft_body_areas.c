/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "soft_body_areas_visual.h"
#include "physics/soft_body/soft_body_area.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "areas line %d: %s\n", __LINE__, #c); return false; } } while(0)
#define OK(c) CHECK(!rohr_error_check(c))

static bool runtime_check(void) {
    EntityResult body = rohr_physics_soft_body_create(); OK(body);
    Position points[] = {{-60,-50},{60,-50},{60,50},{0,10},{-60,50}};
    SoftBodyAreaLoop loop = {.node_count = 5};
    for(unsigned i = 0; i < 5; i += 1) {
        EntityResult node = rohr_physics_soft_body_node_create(body.result.value, points[i], 1, 1);
        OK(node); loop.nodes[i] = node.result.value;
    }
    EntityResult area = rohr_physics_soft_body_area_create(body.result.value, loop); OK(area);
    SoftBodyAreaResult value = rohr_physics_soft_body_area_get(area.result.value); OK(value);
    CHECK(value.result.value.visible && !value.result.value.draw_color_overridden);
    CHECK(value.result.value.soft_body == body.result.value);
    CHECK(!memcmp(value.result.value.reference_positions, points, sizeof(points)));
    CHECK(rohr_physics_soft_body_get(body.result.value).result.value.beam_count == 0);
    CHECK(!rohr_entity_components_check(area.result.value, ROHR_HIT_BOX));
    OK(rohr_graphics_soft_body_area_color_set(area.result.value, (Color){12,34,56,255}));
    OK(rohr_graphics_soft_body_area_visibility_set(area.result.value, false));
    EntityIndex index = rohr_entity_index_get(area.result.value).result.value;
    SoftBodyAreaState before = soft_body_area_states[index];
    CHECK(before.triangulation.triangle_count == 3);
    SoftBodyAreaLoop invalid[] = {
        {.node_count = 2}, {.node_count = SOFT_BODY_MAX_NODES + 1},
        {.nodes = {loop.nodes[0], loop.nodes[1], loop.nodes[0]}, .node_count = 3},
        {.nodes = {loop.nodes[0], loop.nodes[1], ENTITY_INVALID}, .node_count = 3},
        {.nodes = {loop.nodes[0], loop.nodes[2], loop.nodes[1], loop.nodes[4]}, .node_count = 4}
    };
    for(unsigned i = 0; i < sizeof(invalid)/sizeof(*invalid); i += 1) {
        CHECK(rohr_error_check(rohr_physics_soft_body_area_nodes_set(area.result.value, invalid[i])));
        CHECK(rohr_error_check(rohr_physics_soft_body_area_create(body.result.value, invalid[i])));
        CHECK(!memcmp(&before, &soft_body_area_states[index], sizeof(before)));
    }
    EntityResult other = rohr_physics_soft_body_create(); OK(other);
    EntityResult foreign = rohr_physics_soft_body_node_create(other.result.value, (Position){0,50}, 1, 1); OK(foreign);
    SoftBodyAreaLoop bad = loop; bad.nodes[2] = foreign.result.value;
    CHECK(rohr_error_check(rohr_physics_soft_body_area_nodes_set(area.result.value, bad)));
    CHECK(rohr_error_check(rohr_physics_soft_body_area_create(foreign.result.value, loop)));
    OK(rohr_entity_delete(other.result.value));
    CHECK(rohr_error_check(rohr_physics_soft_body_area_get(foreign.result.value)));
    /* A fold, inversion and collapse never rebuild the authored mesh. */
    Position poses[] = {{-100,-70},{0,-80},{-60,-50}};
    for(unsigned i = 0; i < 3; i += 1) {
        OK(rohr_physics_position_set(loop.nodes[2], poses[i]));
        CHECK(!memcmp(&before, &soft_body_area_states[index], sizeof(before)));
    }
    CHECK(rohr_error_check(rohr_physics_soft_body_area_nodes_set(area.result.value, loop)));
    CHECK(!memcmp(&before, &soft_body_area_states[index], sizeof(before)));
    OK(rohr_physics_position_set(loop.nodes[2], points[2]));
    SoftBodyAreaLoop reversed = {.node_count = 5};
    for(unsigned i = 0; i < 5; i += 1) reversed.nodes[i] = loop.nodes[4-i];
    OK(rohr_physics_soft_body_area_nodes_set(area.result.value, reversed));
    value = rohr_physics_soft_body_area_get(area.result.value); OK(value);
    CHECK(!value.result.value.visible && value.result.value.draw_color.red == 12);
    CHECK(value.result.value.loop.nodes[0] == loop.nodes[4]);
    OK(rohr_graphics_soft_body_area_color_clear(area.result.value));
    CHECK(!rohr_physics_soft_body_area_get(area.result.value).result.value.draw_color_overridden);
    /* Returned loop/snapshot is independent; capacity and ordering are transactional. */
    value.result.value.loop.nodes[0] = ENTITY_INVALID;
    Entity areas[SOFT_BODY_MAX_AREAS]; areas[0] = area.result.value;
    for(unsigned i = 1; i < SOFT_BODY_MAX_AREAS; i += 1) {
        EntityResult next = rohr_physics_soft_body_area_create(body.result.value, loop); OK(next);
        areas[i] = next.result.value;
    }
    CHECK(rohr_error_check(rohr_physics_soft_body_area_create(body.result.value, loop)));
    OK(rohr_physics_soft_body_area_order_set(areas[0], SOFT_BODY_MAX_AREAS-1));
    SoftBody topology = rohr_physics_soft_body_get(body.result.value).result.value;
    CHECK(topology.areas[0] == areas[1] && topology.areas[SOFT_BODY_MAX_AREAS-1] == areas[0]);
    CHECK(rohr_error_check(rohr_physics_soft_body_area_order_set(areas[0], SOFT_BODY_MAX_AREAS)));
    OK(rohr_physics_soft_body_area_order_set(areas[0], 0));
    OK(rohr_entity_delete(areas[1]));
    topology = rohr_physics_soft_body_get(body.result.value).result.value;
    CHECK(topology.area_count == SOFT_BODY_MAX_AREAS-1 && topology.areas[1] == areas[2]);
    CHECK(rohr_error_check(rohr_graphics_soft_body_area_visibility_set(areas[1], true)));
    EntityResult replacement = rohr_physics_soft_body_area_create(body.result.value, loop); OK(replacement);
    CHECK(rohr_error_check(rohr_physics_soft_body_area_get(areas[1])));
    OK(rohr_entity_delete(loop.nodes[0]));
    CHECK(rohr_physics_soft_body_get(body.result.value).result.value.area_count == 0);
    for(unsigned i = 0; i < SOFT_BODY_MAX_AREAS; i += 1)
        CHECK(!rohr_entity_alive_check(areas[i]));
    CHECK(!rohr_entity_alive_check(replacement.result.value));
    SoftBodyAreaLoop remaining = {.nodes = {loop.nodes[1],loop.nodes[2],loop.nodes[3]}, .node_count = 3};
    replacement = rohr_physics_soft_body_area_create(body.result.value, remaining); OK(replacement);
    OK(rohr_entity_delete(body.result.value));
    CHECK(!rohr_entity_alive_check(replacement.result.value));
    return true;
}

static bool limits_and_reference_check(void) {
    EntityResult body = rohr_physics_soft_body_create(); OK(body);
    OK(rohr_physics_position_set(body.result.value,(Position){100,200}));
    OK(rohr_physics_orientation_set(body.result.value,90));
    SoftBodyAreaLoop loop = {.node_count=SOFT_BODY_MAX_NODES};
    for(unsigned i=0;i<SOFT_BODY_MAX_NODES;i+=1) {
        float angle = (float)i * 2.0f * PI_F / SOFT_BODY_MAX_NODES;
        Position local = {100*cosf(angle),100*sinf(angle)};
        EntityResult node = rohr_physics_soft_body_node_local_create(body.result.value,local,1,1);
        OK(node); loop.nodes[i] = node.result.value;
    }
    EntityResult area = rohr_physics_soft_body_area_create(body.result.value,loop); OK(area);
    SoftBodyArea value = rohr_physics_soft_body_area_get(area.result.value).result.value;
    CHECK(fabsf(value.reference_positions[0].x-100) < 0.001f);
    CHECK(fabsf(value.reference_positions[0].y) < 0.001f);
    EntityIndex index = rohr_entity_index_get(area.result.value).result.value;
    CHECK(soft_body_area_states[index].triangulation.triangle_count == SOFT_BODY_MAX_NODES-2);
    EntityResult beam = rohr_physics_soft_body_beam_create(body.result.value,loop.nodes[0],loop.nodes[1],1,1); OK(beam);
    OK(rohr_entity_delete(beam.result.value));
    CHECK(rohr_entity_alive_check(area.result.value));
    EntityResult independent = rohr_physics_soft_body_area_create(body.result.value,
        (SoftBodyAreaLoop){.nodes={loop.nodes[1],loop.nodes[2],loop.nodes[3]},.node_count=3}); OK(independent);
    OK(rohr_entity_delete(loop.nodes[0]));
    CHECK(!rohr_entity_alive_check(area.result.value) && rohr_entity_alive_check(independent.result.value));
    OK(rohr_entity_delete(body.result.value));
    CHECK(!rohr_entity_alive_check(independent.result.value));
    return true;
}

static void scene_draw(CameraId camera, void *context) {
    (void)camera;
    rohr_graphics_background_draw((Color){0,0,0,255});
    (void)rohr_graphics_soft_body_draw(*(Entity *)context,
        (Color){0,255,0,255}, (Color){0}, (Color){0});
}

/* Small interior samples verify overlap footprint and actual queue ordering. */
static bool sample_check(int x, int y, Color expected) {
    int count = 0; SDL_Window **windows = SDL_GetWindows(&count);
    CHECK(windows != NULL && count > 0);
    SDL_Renderer *renderer = SDL_GetRenderer(windows[0]); SDL_free(windows);
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL); CHECK(surface != NULL);
    Uint8 r,g,b,a;
    bool read = SDL_ReadSurfacePixel(surface,x,y,&r,&g,&b,&a);
    SDL_DestroySurface(surface);
    CHECK(read && r == expected.red && g == expected.green && b == expected.blue);
    return true;
}

static bool rendering_check(void) {
    OK(rohr_graphics_start());
    EntityResult body = rohr_physics_soft_body_create(); OK(body);
    SoftBodyAreaLoop square = {.node_count = 4};
    Position p[] = {{-80,-60},{80,-60},{80,60},{-80,60},{0,-40},{120,-40},{60,80}};
    Entity nodes[7];
    for(unsigned i=0;i<7;i+=1) {
        EntityResult n = rohr_physics_soft_body_node_create(body.result.value,p[i],1,1); OK(n);
        nodes[i]=n.result.value;
        if(i<4) square.nodes[i]=nodes[i];
    }
    EntityResult lower = rohr_physics_soft_body_area_create(body.result.value,square); OK(lower);
    EntityResult upper = rohr_physics_soft_body_area_create(body.result.value,
        (SoftBodyAreaLoop){.nodes={nodes[4],nodes[5],nodes[6]},.node_count=3}); OK(upper);
    Color red={255,0,0,255}, green={0,255,0,255};
    OK(rohr_graphics_soft_body_area_color_set(upper.result.value,red));
    CameraId camera = rohr_camera_active_get();
    Camera c = rohr_camera_get(camera).result.value; c.position=(Position){0}; c.orientation=0; c.zoom=1; c.dimensions=(Vec2D){640,480};
    OK(rohr_camera_set(camera,c));
    OK(rohr_camera_render_callback_set(camera,scene_draw,&body.result.value));
    ViewportIdResult viewport=rohr_viewport_create((ViewportConfig){.rectangle={0,0,640,480}}); OK(viewport);
    OK(rohr_viewport_camera_set(viewport.result.value,camera));
    OK(rohr_viewport_enable_set(viewport.result.value));
    rohr_graphics_show();
    CHECK(sample_check(360,240,red) && sample_check(280,240,green));
    OK(rohr_physics_soft_body_area_order_set(lower.result.value,1));
    rohr_graphics_show(); CHECK(sample_check(360,240,green));
    OK(rohr_graphics_layer_entity_set(upper.result.value,3));
    rohr_graphics_show(); CHECK(sample_check(360,240,red) && sample_check(280,240,green));
    OK(rohr_graphics_soft_body_area_visibility_set(upper.result.value,false));
    rohr_graphics_show(); CHECK(sample_check(360,240,green));
    OK(rohr_graphics_soft_body_area_visibility_set(upper.result.value,true));
    OK(rohr_graphics_soft_body_area_color_clear(upper.result.value));
    rohr_graphics_show(); CHECK(sample_check(360,240,green));
    OK(rohr_viewport_destroy(viewport.result.value));
    OK(rohr_camera_render_callback_set(camera,NULL,NULL));
    OK(rohr_entity_delete(body.result.value));
    rohr_graphics_stop();
    return true;
}

int main(int argc, char **argv) {
    bool visual = argc == 2 && strcmp(argv[1], "--visual") == 0;
    if(argc != 1 && !visual) {
        fprintf(stderr, "Usage: %s [--visual]\n", argv[0]);
        return 1;
    }
    if(rohr_error_check(rohr_engine_start())) return 1;
    bool passed = visual ? soft_body_areas_visual_run() :
        runtime_check() && limits_and_reference_check() &&
        soft_body_areas_visual_motion_check() && rendering_check();
    rohr_engine_stop();
    return passed ? 0 : 1;
}
