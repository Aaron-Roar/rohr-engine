/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "project_objects.h"
#include "project_viewports.h"
#include <stdio.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "generated areas line %d: %s\n", __LINE__, #c); return 1; } } while(0)
#define OK(c) CHECK(!rohr_error_check(c))
int main(void) {
    ProjectObjects objects = {0};
    ProjectViewports resources = {0};
    AreaFixture other = {0};
    OK(rohr_engine_start());
    OK(rohr_graphics_start());
    OK(project_objects_create_all(&objects));
    OK(project_viewports_create(&resources, &objects));
    OK(area_fixture_create(&other, (Position){300,0}));
    AreaFixture *first = &objects.area_fixture;
    SoftBodyAreaResult area = rohr_physics_soft_body_area_get(first->cloth_areas[0]);
    OK(area);
    CHECK(area.result.value.geometry.outer.node_count == 4 &&
        area.result.value.geometry.hole_count == 1 &&
        area.result.value.geometry.holes[0].node_count == 4);
    CHECK(area.result.value.geometry.outer.nodes[0] == first->node_1 &&
        area.result.value.geometry.holes[0].nodes[0] == first->node_5);
    CHECK(area.result.value.draw_color_overridden && area.result.value.visible &&
        area.result.value.draw_color.alpha == 128 && area.result.value.draw_color.red == 34);
    CHECK(rohr_graphics_layer_entity_get(first->cloth_areas[0]).result.value == 9);
    CHECK(other.cloth_areas[0] != first->cloth_areas[0]);
    OK(rohr_physics_position_set(other.node_5, (Position){500,0}));
    CHECK(rohr_physics_position_get(first->node_5).result.value.x < 0);
    /* Draw both generated instances through a real viewport/screen. */
    CameraIdResult camera = rohr_camera_create(rohr_camera_config_default_get());
    OK(camera);
    ViewportIdResult viewport = rohr_viewport_create(rohr_viewport_config_default_get());
    OK(viewport);
    OK(rohr_viewport_camera_set(viewport.result.value, camera.result.value));
    project_objects_draw_all(&objects);
    area_fixture_draw(&other);
    rohr_graphics_show();
    OK(rohr_physics_position_set(first->node_1, (Position){70,-70}));
    OK(rohr_physics_position_set(first->node_2, (Position){-70,-70}));
    project_objects_draw_all(&objects);
    rohr_graphics_show();
    Entity removed_area = first->cloth_areas[0];
    area_fixture_destroy(&other);
    project_viewports_destroy(&resources);
    project_objects_destroy_all(&objects);
    CHECK(rohr_error_check(rohr_physics_soft_body_area_get(removed_area)));
    rohr_graphics_stop();
    rohr_engine_stop();
    return 0;
}
