/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#define main generated_simulation_main
#include "project_scene.c"
#undef main
#include <assert.h>
#include <stdlib.h>
#define OK(value) assert(!rohr_error_check(value))
int main(int argc, char **argv) {
    assert(argc == 5);
    size_t expected_objects = (size_t)atoi(argv[1]);
    OK(rohr_directory_working_set(rohr_directory_base_get()));
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    Scene scene = {0};
    EngineResult initialized = scene_initialize(&scene);
    if(rohr_error_check(initialized)) fprintf(stderr, "Scene initialization: %s\n", rohr_error_message_get(initialized));
    OK(initialized);
    assert(objects.child_count == expected_objects && rohr_entity_alive_check(scene.objects));
    assert(rohr_entity_alive_count_get() >= 1); /* The created root also owns an empty scene. */
    assert(project.viewport_count == (size_t)atoi(argv[2]));
    size_t screens = 0, ui_items = 0;
    for(size_t i = 0; i < project.viewport_count; ++i) {
        OK(rohr_viewport_description_get(scene.viewports[i]));
        const ViewportCreateDescription *recipe = project.viewports[i];
        for(size_t j = 0; j < recipe->item_count; ++j) {
            ViewportItemIdResult item = rohr_viewport_item_by_name_get(scene.viewports[i], recipe->items[j].name);
            OK(item);
            OK(rohr_viewport_item_description_get(item.result.value));
            if(recipe->items[j].kind == VIEWPORT_CREATE_SCREEN) ++screens;
            else ++ui_items;
        }
    }
    assert(screens == (size_t)atoi(argv[3]));
    assert(ui_items == (size_t)atoi(argv[4]));
    for(int tick = 0; tick < 4; ++tick) {
        assert(rohr_physics_update(1).header.status != ROHR_REPORT_FAILED);
        assert(rohr_graphics_show().header.status != ROHR_REPORT_FAILED);
    }
    scene_shutdown(&scene);
    assert(rohr_entity_alive_count_get() == 0);
    rohr_graphics_stop(); rohr_engine_stop(); return 0;
}
