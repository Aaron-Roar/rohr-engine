/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "project.h"
#include "../runtime_fixture.h"
#include <stdio.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "generated areas line %d: %s\n", __LINE__, #c); return 1; } } while(0)
#define OK(c) CHECK(!rohr_error_check(c))
int main(void) {
    OK(rohr_engine_start());
    OK(rohr_graphics_start());
    EntityResult first = rohr_object_create(project.area_fixture); OK(first);
    ObjectCreateOptions options = rohr_object_create_options_default_get();
    options.offset = (Position){300,0};
    EntityResult other = rohr_object_create_with_options(project.area_fixture, options); OK(other);
    EntityResult first_area = fixture_entity_get(first.result.value, "area_1"); OK(first_area);
    EntityResult other_area = fixture_entity_get(other.result.value, "area_1"); OK(other_area);
    EntityResult node1 = fixture_entity_get(first.result.value, "node_1"); OK(node1);
    EntityResult node2 = fixture_entity_get(first.result.value, "node_2"); OK(node2);
    EntityResult node5 = fixture_entity_get(first.result.value, "node_5"); OK(node5);
    EntityResult other_node5 = fixture_entity_get(other.result.value, "node_5"); OK(other_node5);
    SoftBodyAreaResult area = rohr_physics_soft_body_area_get(first_area.result.value);
    OK(area);
    CHECK(area.result.value.geometry.outer.node_count == 4 &&
        area.result.value.geometry.hole_count == 1 &&
        area.result.value.geometry.holes[0].node_count == 4);
    CHECK(area.result.value.geometry.outer.nodes[0] == node1.result.value &&
        area.result.value.geometry.holes[0].nodes[0] == node5.result.value);
    CHECK(area.result.value.draw_color_overridden && area.result.value.visible &&
        area.result.value.draw_color.alpha == 128 && area.result.value.draw_color.red == 34);
    CHECK(rohr_graphics_layer_entity_get(first_area.result.value).result.value == 9);
    CHECK(other_area.result.value != first_area.result.value);
    OK(rohr_physics_position_set(other_node5.result.value, (Position){500,0}));
    CHECK(rohr_physics_position_get(node5.result.value).result.value.x < 0);
    /* Draw both generated instances through a real viewport/screen. */
    CameraIdResult camera = rohr_camera_create(rohr_camera_description_default_get());
    OK(camera);
    ViewportCreateItemDescription screen = {.name="world",.kind=VIEWPORT_CREATE_SCREEN,
        .placement=rohr_viewport_item_description_default_get(),.render_width=640,.render_height=480};
    ViewportCameraBinding binding = {.item_name="world",.camera=camera.result.value};
    ViewportCreateDescription description = {.viewport = rohr_viewport_description_default_get()};
    description.items=&screen; description.item_count=1;
    description.cameras=&binding; description.camera_count=1;
    ViewportIdResult viewport = rohr_viewport_create(description);
    OK(viewport);
    CHECK(rohr_graphics_viewport_show(viewport.result.value).header.status != ROHR_REPORT_FAILED);
    OK(rohr_physics_position_set(node1.result.value, (Position){70,-70}));
    OK(rohr_physics_position_set(node2.result.value, (Position){-70,-70}));
    CHECK(rohr_graphics_viewport_show(viewport.result.value).header.status != ROHR_REPORT_FAILED);
    Entity removed_area = first_area.result.value;
    OK(rohr_entity_delete(other.result.value));
    OK(rohr_viewport_destroy(viewport.result.value));
    OK(rohr_camera_destroy(camera.result.value));
    OK(rohr_entity_delete(first.result.value));
    CHECK(rohr_error_check(rohr_physics_soft_body_area_get(removed_area)));
    rohr_graphics_stop();
    rohr_engine_stop();
    return 0;
}
