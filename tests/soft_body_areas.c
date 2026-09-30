/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "soft_body_areas_visual.h"
#include "physics/soft_body/soft_body_area.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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
    EntityResult area = rohr_physics_soft_body_area_create(body.result.value, (SoftBodyAreaGeometry){.outer = loop}); OK(area);
    SoftBodyAreaResult value = rohr_physics_soft_body_area_get(area.result.value); OK(value);
    CHECK(value.result.value.visible && !value.result.value.draw_color_overridden);
    CHECK(value.result.value.soft_body == body.result.value);
    CHECK(value.result.value.geometry.outer.node_count == 5);
    CHECK(rohr_physics_soft_body_get(body.result.value).result.value.beam_count == 0);
    CHECK(!rohr_entity_components_check(area.result.value, ROHR_HIT_BOX));
    OK(rohr_graphics_soft_body_area_color_set(area.result.value, (Color){12,34,56,255}));
    OK(rohr_graphics_soft_body_area_visibility_set(area.result.value, false));
    EntityIndex index = rohr_entity_index_get(area.result.value).result.value;
    SoftBodyAreaState before = soft_body_area_states[index];
    CHECK(before.cache->fill.vertex_count > 0);
    SoftBodyAreaLoop invalid[] = {
        {.node_count = 2}, {.node_count = SOFT_BODY_MAX_NODES + 1},
        {.nodes = {loop.nodes[0], loop.nodes[1], loop.nodes[0]}, .node_count = 3},
        {.nodes = {loop.nodes[0], loop.nodes[1], ENTITY_INVALID}, .node_count = 3},
    };
    for(unsigned i = 0; i < sizeof(invalid)/sizeof(*invalid); i += 1) {
        CHECK(rohr_error_check(rohr_physics_soft_body_area_geometry_set(area.result.value, (SoftBodyAreaGeometry){.outer = invalid[i]})));
        CHECK(rohr_error_check(rohr_physics_soft_body_area_create(body.result.value, (SoftBodyAreaGeometry){.outer = invalid[i]})));
        CHECK(!memcmp(&before, &soft_body_area_states[index], sizeof(before)));
    }
    EntityResult other = rohr_physics_soft_body_create(); OK(other);
    EntityResult foreign = rohr_physics_soft_body_node_create(other.result.value, (Position){0,50}, 1, 1); OK(foreign);
    SoftBodyAreaLoop bad = loop; bad.nodes[2] = foreign.result.value;
    CHECK(rohr_error_check(rohr_physics_soft_body_area_geometry_set(area.result.value, (SoftBodyAreaGeometry){.outer = bad})));
    CHECK(rohr_error_check(rohr_physics_soft_body_area_create(foreign.result.value, (SoftBodyAreaGeometry){.outer = loop})));
    OK(rohr_entity_delete(other.result.value));
    CHECK(rohr_error_check(rohr_physics_soft_body_area_get(foreign.result.value)));
    /* Pose changes update the fill while preserving the stored description. */
    Position poses[] = {{-100,-70},{0,-80},{-60,-50}};
    for(unsigned i = 0; i < 3; i += 1) {
        OK(rohr_physics_position_set(loop.nodes[2], poses[i]));
        const NodeLoopFill *fill;
        OK(physics_soft_body_area_fill_get(area.result.value, &fill));
        CHECK(!memcmp(&before.value, &soft_body_area_states[index].value, sizeof(before.value)));
    }
    OK(rohr_physics_position_set(loop.nodes[2], points[2]));
    SoftBodyAreaLoop reversed = {.node_count = 5};
    for(unsigned i = 0; i < 5; i += 1) reversed.nodes[i] = loop.nodes[4-i];
    OK(rohr_physics_soft_body_area_geometry_set(area.result.value, (SoftBodyAreaGeometry){.outer = reversed}));
    value = rohr_physics_soft_body_area_get(area.result.value); OK(value);
    CHECK(!value.result.value.visible && value.result.value.draw_color.red == 12);
    CHECK(value.result.value.geometry.outer.nodes[0] == loop.nodes[4]);
    OK(rohr_graphics_soft_body_area_color_clear(area.result.value));
    CHECK(!rohr_physics_soft_body_area_get(area.result.value).result.value.draw_color_overridden);
    /* Returned loop/snapshot is independent; capacity and ordering are transactional. */
    value.result.value.geometry.outer.nodes[0] = ENTITY_INVALID;
    Entity areas[SOFT_BODY_MAX_AREAS]; areas[0] = area.result.value;
    for(unsigned i = 1; i < SOFT_BODY_MAX_AREAS; i += 1) {
        EntityResult next = rohr_physics_soft_body_area_create(body.result.value, (SoftBodyAreaGeometry){.outer = loop}); OK(next);
        areas[i] = next.result.value;
    }
    CHECK(rohr_error_check(rohr_physics_soft_body_area_create(body.result.value, (SoftBodyAreaGeometry){.outer = loop})));
    OK(rohr_physics_soft_body_area_order_set(areas[0], SOFT_BODY_MAX_AREAS-1));
    SoftBody topology = rohr_physics_soft_body_get(body.result.value).result.value;
    CHECK(topology.areas[0] == areas[1] && topology.areas[SOFT_BODY_MAX_AREAS-1] == areas[0]);
    CHECK(rohr_error_check(rohr_physics_soft_body_area_order_set(areas[0], SOFT_BODY_MAX_AREAS)));
    OK(rohr_physics_soft_body_area_order_set(areas[0], 0));
    OK(rohr_entity_delete(areas[1]));
    topology = rohr_physics_soft_body_get(body.result.value).result.value;
    CHECK(topology.area_count == SOFT_BODY_MAX_AREAS-1 && topology.areas[1] == areas[2]);
    CHECK(rohr_error_check(rohr_graphics_soft_body_area_visibility_set(areas[1], true)));
    EntityResult replacement = rohr_physics_soft_body_area_create(body.result.value, (SoftBodyAreaGeometry){.outer = loop}); OK(replacement);
    CHECK(rohr_error_check(rohr_physics_soft_body_area_get(areas[1])));
    OK(rohr_entity_delete(loop.nodes[0]));
    CHECK(rohr_physics_soft_body_get(body.result.value).result.value.area_count == 0);
    for(unsigned i = 0; i < SOFT_BODY_MAX_AREAS; i += 1)
        CHECK(!rohr_entity_alive_check(areas[i]));
    CHECK(!rohr_entity_alive_check(replacement.result.value));
    SoftBodyAreaLoop remaining = {.nodes = {loop.nodes[1],loop.nodes[2],loop.nodes[3]}, .node_count = 3};
    replacement = rohr_physics_soft_body_area_create(body.result.value, (SoftBodyAreaGeometry){.outer = remaining}); OK(replacement);
    OK(rohr_entity_delete(body.result.value));
    CHECK(!rohr_entity_alive_check(replacement.result.value));
    return true;
}

static bool limits_and_transform_check(void) {
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
    EntityResult area = rohr_physics_soft_body_area_create(body.result.value, (SoftBodyAreaGeometry){.outer = loop}); OK(area);
    SoftBodyArea value = rohr_physics_soft_body_area_get(area.result.value).result.value;
    CHECK(value.geometry.outer.node_count == SOFT_BODY_MAX_NODES);
    const NodeLoopFill *fill;
    OK(physics_soft_body_area_fill_get(area.result.value, &fill));
    CHECK(fill->vertex_count > 0);
    EntityResult beam = rohr_physics_soft_body_beam_create(body.result.value,loop.nodes[0],loop.nodes[1],1,1); OK(beam);
    OK(rohr_entity_delete(beam.result.value));
    CHECK(rohr_entity_alive_check(area.result.value));
    EntityResult independent = rohr_physics_soft_body_area_create(body.result.value, (SoftBodyAreaGeometry){.outer = (SoftBodyAreaLoop){.nodes={loop.nodes[1],loop.nodes[2],loop.nodes[3]},.node_count=3}}); OK(independent);
    OK(rohr_entity_delete(loop.nodes[0]));
    CHECK(!rohr_entity_alive_check(area.result.value) && rohr_entity_alive_check(independent.result.value));
    OK(rohr_entity_delete(body.result.value));
    CHECK(!rohr_entity_alive_check(independent.result.value));
    return true;
}

static bool holes_check(void) {
    EntityResult body=rohr_physics_soft_body_create(); OK(body);
    Position points[]={{-100,-100},{100,-100},{100,100},{-100,100},
        {-20,-20},{20,-20},{20,20},{-20,20}};
    Entity nodes[8];
    for(unsigned i=0;i<8;i+=1) {
        EntityResult node=rohr_physics_soft_body_node_create(body.result.value,points[i],1,1); OK(node);
        nodes[i]=node.result.value;
    }
    SoftBodyAreaGeometry geometry={.outer={.nodes={nodes[0],nodes[1],nodes[2],nodes[3]},.node_count=4},.hole_count=16};
    for(unsigned i=0;i<16;i+=1)
        geometry.holes[i]=(SoftBodyAreaLoop){.nodes={nodes[4],nodes[5],nodes[6],nodes[7]},.node_count=4};
    EntityResult area=rohr_physics_soft_body_area_create(body.result.value,geometry); OK(area);
    EntityResult unrelated=rohr_physics_soft_body_area_create(body.result.value,(SoftBodyAreaGeometry){.outer=geometry.outer}); OK(unrelated);
    SoftBodyArea snapshot=rohr_physics_soft_body_area_get(area.result.value).result.value;
    CHECK(snapshot.geometry.hole_count==16);
    geometry.hole_count=17;
    EntityResult rejected=rohr_physics_soft_body_area_create(body.result.value,geometry);
    CHECK(rohr_error_check(rejected) && rejected.result.error==ERROR_ENGINE_MAX_AREA_HOLES_EXCEEDED);
    EngineResult edit=rohr_physics_soft_body_area_geometry_set(area.result.value,geometry);
    CHECK(rohr_error_check(edit) && edit.result.error==ERROR_ENGINE_MAX_AREA_HOLES_EXCEEDED);
    CHECK(strstr(rohr_error_message_get(edit),"maximum 16")!=NULL);
    SoftBodyArea after=rohr_physics_soft_body_area_get(area.result.value).result.value;
    CHECK(!memcmp(&snapshot,&after,sizeof(snapshot)));
    geometry=snapshot.geometry;
    const NodeLoopFill *fill;
    OK(physics_soft_body_area_fill_get(area.result.value,&fill));
    Vec2D *cached_vertices=fill->vertices;
    OK(physics_soft_body_area_fill_get(area.result.value,&fill));
    CHECK(fill->vertices==cached_vertices);
    OK(rohr_graphics_soft_body_area_color_set(area.result.value,(Color){12,34,56,128}));
    OK(physics_soft_body_area_fill_get(area.result.value,&fill));
    CHECK(fill->vertices==cached_vertices);
    /* Collapsed runtime holes are harmless, but rejected as new descriptions. */
    for(unsigned i=4;i<8;i+=1) OK(rohr_physics_position_set(nodes[i],(Position){0,0}));
    OK(physics_soft_body_area_fill_get(area.result.value,&fill)); CHECK(fill->vertex_count>0);
    CHECK(rohr_error_check(rohr_physics_soft_body_area_geometry_set(area.result.value,geometry)));
    for(unsigned i=4;i<8;i+=1) OK(rohr_physics_position_set(nodes[i],points[i]));
    /* Initial hourglasses are valid; zero signed area is not zero enclosed area. */
    geometry.hole_count=0;
    geometry.outer=(SoftBodyAreaLoop){.nodes={nodes[1],nodes[0],nodes[2],nodes[3]},.node_count=4};
    EntityResult crossed=rohr_physics_soft_body_area_create(body.result.value,geometry); OK(crossed);
    OK(rohr_entity_delete(crossed.result.value));
    OK(rohr_entity_delete(nodes[4]));
    CHECK(!rohr_entity_alive_check(area.result.value));
    CHECK(rohr_entity_alive_check(unrelated.result.value));
    OK(rohr_entity_delete(body.result.value));
    return true;
}

static void scene_draw(CameraId camera, void *context) {
    (void)camera;
    rohr_graphics_layer_active_set(-100);
    rohr_graphics_background_draw((Color){0,0,0,255});
    rohr_graphics_layer_active_set(0);
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
    if(read && (abs((int)r-expected.red)>1 || abs((int)g-expected.green)>1 || abs((int)b-expected.blue)>1))
        fprintf(stderr,"pixel (%d,%d): got %u,%u,%u; expected %u,%u,%u\n",x,y,r,g,b,expected.red,expected.green,expected.blue);
    CHECK(read && abs((int)r-expected.red) <= 1 &&
        abs((int)g-expected.green) <= 1 && abs((int)b-expected.blue) <= 1);
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
    EntityResult lower = rohr_physics_soft_body_area_create(body.result.value, (SoftBodyAreaGeometry){.outer = square}); OK(lower);
    EntityResult upper = rohr_physics_soft_body_area_create(body.result.value, (SoftBodyAreaGeometry){.outer = (SoftBodyAreaLoop){.nodes={nodes[4],nodes[5],nodes[6]},.node_count=3}}); OK(upper);
    Color red={255,0,0,255}, green={0,255,0,255};
    OK(rohr_graphics_soft_body_area_color_set(upper.result.value,red));
    CameraId camera = rohr_camera_active_get();
    Camera c = rohr_camera_get(camera).result.value; c.position=(Position){0}; c.orientation=0; c.zoom=1; c.dimensions=(Vec2D){640,480};
    OK(rohr_camera_set(camera,c));
    OK(rohr_camera_render_callback_set(camera,scene_draw,&body.result.value));
    ViewportIdResult viewport=rohr_viewport_create((ViewportConfig){.rectangle={0,0,640,480}}); OK(viewport);
    ScreenIdResult screen=rohr_screen_create((ScreenConfig){.camera=camera,.width=640,.height=480}); OK(screen);
    ViewportItemIdResult item=rohr_viewport_screen_add(viewport.result.value,screen.result.value,
        (ViewportItemConfig){.rectangle={0,0,640,480},.visible=true}); OK(item);
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
    /* A hole reveals another area's fill, and cannot erase independent areas. */
    SoftBodyAreaGeometry window={.outer=square,.hole_count=1,.holes={{.node_count=4}}};
    Position hole_points[]={{-15,-15},{15,-15},{15,15},{-15,15}};
    for(unsigned i=0;i<4;i+=1) {
        EntityResult n=rohr_physics_soft_body_node_create(body.result.value,hole_points[i],1,1); OK(n);
        window.holes[0].nodes[i]=n.result.value;
    }
    OK(rohr_physics_soft_body_area_geometry_set(lower.result.value,window));
    EntityResult behind=rohr_physics_soft_body_area_create(body.result.value,(SoftBodyAreaGeometry){.outer=square}); OK(behind);
    Color blue={0,0,255,255};
    OK(rohr_graphics_soft_body_area_color_set(behind.result.value,blue));
    OK(rohr_graphics_layer_entity_set(behind.result.value,-1));
    OK(rohr_graphics_soft_body_area_color_set(upper.result.value,red));
    rohr_graphics_show(); CHECK(sample_check(320,240,blue) && sample_check(360,240,red));
    for(unsigned i=0;i<4;i+=1) {
        Position p=hole_points[i]; p.x+=40;
        OK(rohr_physics_position_set(window.holes[0].nodes[i],p));
    }
    rohr_graphics_show(); CHECK(sample_check(360,240,red) && sample_check(320,240,green));
    OK(rohr_graphics_soft_body_area_visibility_set(upper.result.value,false));
    OK(rohr_graphics_soft_body_area_color_set(lower.result.value,(Color){255,0,0,128}));
    rohr_graphics_show(); CHECK(sample_check(280,240,(Color){128,0,127,255}));
    CHECK(sample_check(360,240,blue));
    /* The rendered crossed square must have no side spill. */
    OK(rohr_graphics_soft_body_area_visibility_set(behind.result.value,false));
    OK(rohr_physics_soft_body_area_geometry_set(lower.result.value,(SoftBodyAreaGeometry){.outer=square}));
    OK(rohr_graphics_soft_body_area_color_set(lower.result.value,green));
    OK(rohr_physics_position_set(nodes[0],(Position){80,-60}));
    OK(rohr_physics_position_set(nodes[1],(Position){-80,-60}));
    rohr_graphics_show();
    CHECK(sample_check(270,240,(Color){0,0,0,255}) && sample_check(370,240,(Color){0,0,0,255}));
    CHECK(sample_check(320,200,green) && sample_check(320,280,green));
    OK(rohr_viewport_destroy(viewport.result.value));
    OK(rohr_screen_destroy(screen.result.value));
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
        runtime_check() && limits_and_transform_check() && holes_check() &&
        soft_body_areas_visual_motion_check() && rendering_check();
    rohr_engine_stop();
    return passed ? 0 : 1;
}
