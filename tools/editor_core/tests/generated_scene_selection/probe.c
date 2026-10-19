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
    OK(scene_initialize(&scene));
    assert(objects.child_count == expected_objects && scene.objects.created);
    assert((rohr_entity_alive_count_get() > 0) == (expected_objects > 0));
    assert(scene.viewports.viewport_count == (size_t)atoi(argv[2]));
    assert(scene.viewports.screen_count == (size_t)atoi(argv[3]));
    assert(scene.viewports.ui_item_count == (size_t)atoi(argv[4]));
    for(int tick = 0; tick < 4; ++tick) {
        assert(rohr_physics_update(1).header.status != ROHR_REPORT_FAILED);
        assert(rohr_graphics_show().header.status != ROHR_REPORT_FAILED);
    }
    scene_shutdown(&scene);
    assert(rohr_entity_alive_count_get() == 0);
    rohr_graphics_stop(); rohr_engine_stop(); return 0;
}
