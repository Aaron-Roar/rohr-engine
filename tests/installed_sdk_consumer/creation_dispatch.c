/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "../../tools/editor_core/tests/runtime_fixture.h"
#include "project.h"
#include <assert.h>

#define OK(call) assert(!rohr_error_check(call))

int main(void) {
    OK(rohr_engine_start());
    OK(rohr_graphics_start());
    unsigned calls = 0, options_calls = 0;
    /* Address expressions execute once and retain the exact generated catalog.
     * The camera and member relationships would fail if dispatch copied it. */
    EntityResult object = rohr_object_create_with_options(
        (&project.mass_demo)[calls++],
        (options_calls++, rohr_object_create_options_default_get()));
    OK(object); assert(calls == 1 && options_calls == 1);
    EntityResult camera = fixture_entity_get(object.result.value, "main_camera"); OK(camera);
    OK(rohr_entity_object_camera_get(camera.result.value));
    OK(rohr_entity_delete(object.result.value));
    calls = 0;
    object = object_create((&objects.mass_demo)[calls++]); OK(object);
    assert(calls == 1); OK(rohr_entity_delete(object.result.value));
    calls = 0;
    EntityResult entity = rohr_entity_create((&project.mass_demo.automatic_body)[calls++]); OK(entity);
    assert(calls == 1); OK(rohr_entity_delete(entity.result.value));
    calls = options_calls = 0;
    entity = entity_create_with_options((&objects.mass_demo.automatic_body)[calls++],
        (options_calls++, entity_create_options_default_get())); OK(entity);
    assert(calls == 1 && options_calls == 1); OK(rohr_entity_delete(entity.result.value));
    calls = 0;
    ProjectIdResult instance = rohr_project_create((&project)[calls++]); OK(instance);
    assert(calls == 1); OK(rohr_project_destroy(instance.result.value));
    /* Root-only creation accepts both the typed view and its engine recipe. */
    object = object_create(project); OK(object); OK(rohr_entity_delete(object.result.value));
    object = rohr_object_create(project._project); OK(object); OK(rohr_entity_delete(object.result.value));
    calls = 0;
    const ProjectDescription *recipes[] = {&project._project};
    instance = project_create(*recipes[calls++]); OK(instance);
    assert(calls == 1); OK(rohr_project_destroy(instance.result.value));
    instance = rohr_project_create((ProjectDescription){.root={.id=1, .name="empty",
        .kind=OBJECT_CONTAINER, .categories=ROHR_ENTITY_OBJECT_ALL}}); OK(instance);
    OK(rohr_project_destroy(instance.result.value));
    assert(rohr_entity_alive_count_get() == 0);
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
