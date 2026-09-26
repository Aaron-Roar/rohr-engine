/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_mass_properties.h"
#include "editor_workspace.h"
#include "project_objects.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "COM example line %d: %s\n", __LINE__, #c); return false; } } while(0)
#define OK(c) CHECK(!rohr_error_check(c))
static bool near(Position a, Position b) {
    return fabsf(a.x - b.x) < 0.003f && fabsf(a.y - b.y) < 0.003f;
}

static bool authoring_check(void) {
    EditorProject project;
    EditorWorkspace workspace = {.config = editor_workspace_config_default_get(), .open = true};
    editor_project_init(&project);
    CHECK(!editor_result_check(editor_project_load(&project,
        COM_EXAMPLE_SOURCE "/objects/project.rohr.json")));
    CHECK(project.object_count == 1 && project.objects[0].rigid_body_count == 2);
    EditorMassProperties automatic = editor_mass_properties_get(&project.objects[0].rigid_bodies[0]);
    EditorMassProperties explicit_value = editor_mass_properties_get(&project.objects[0].rigid_bodies[1]);
    CHECK(automatic.center_available && automatic.inertia_available &&
        near(automatic.center, (Position){60,30}) && fabsf(automatic.inertia - 8000) < 0.01f);
    CHECK(explicit_value.center_available && explicit_value.inertia_available &&
        near(explicit_value.center, (Position){30,15}) && fabsf(explicit_value.inertia - 21500) < 0.01f);
    CHECK(editor_project_save(&project, "com_example_round_trip.json"));
    CHECK(!editor_result_check(editor_project_load(&project, "com_example_round_trip.json")));
    snprintf(workspace.directory, sizeof(workspace.directory), "com_example_generated");
    CHECK(SDL_CreateDirectory(workspace.directory));
    CHECK(SDL_CreateDirectory("com_example_generated/src"));
    CHECK(SDL_CreateDirectory("com_example_generated/src/generated"));
    FILE *main_file = fopen("com_example_generated/src/main.c", "wb");
    CHECK(main_file != NULL);
    CHECK(fputs("/* developer-owned entry point */\n", main_file) >= 0);
    CHECK(fclose(main_file) == 0);
    CHECK(editor_workspace_c_generate(&workspace, &project));
    const char *files[] = {"project_objects.c", "project_objects.h",
        "project_viewports.c", "project_viewports.h"};
    for(size_t i = 0; i < 4; i += 1) {
        char source[1024], output[1024];
        snprintf(source, sizeof(source), "%s/src/generated/%s", COM_EXAMPLE_SOURCE, files[i]);
        snprintf(output, sizeof(output), "com_example_generated/src/generated/%s", files[i]);
        char *expected = SDL_LoadFile(source, NULL), *actual = SDL_LoadFile(output, NULL);
        bool same = expected != NULL && actual != NULL && strcmp(expected, actual) == 0;
        SDL_free(expected); SDL_free(actual);
        CHECK(same);
    }
    editor_project_destroy(&project);
    remove("com_example_round_trip.json");
    return true;
}

typedef struct Snapshot { Position origin, com, anchor; float angle; } Snapshot;

static bool simulation_check(bool generated, Snapshot *snapshot) {
    MassDemo demo = {0};
    Entity body;
    JointAnchorId anchor;
    Shape shape = {.amount_of_vertices = 4, .vertices = {{100,50},{100,10},{20,10},{20,50}}};
    OK(rohr_engine_start());
    if(generated) {
        OK(mass_demo_create(&demo, (Position){0}));
        body = demo.explicit_body;
        anchor = demo.anchor_explicit_tip;
        CHECK(rohr_physics_center_of_mass_automatic_check(demo.automatic_body).result.value);
        CHECK(near(rohr_physics_center_of_mass_local_position_get(demo.automatic_body).result.value,
            (Position){60,30}));
    } else {
        EntityResult created = rohr_entity_add(); OK(created); body = created.result.value;
        OK(rohr_physics_position_set(body, (Position){120,-30}));
        OK(rohr_physics_orientation_radians_set(body, -0.35f));
        OK(rohr_physics_hitbox_set(body, shape));
        OK(rohr_physics_mass_set(body, 12));
        OK(rohr_physics_velocity_set(body, (Velocity){0}));
        OK(rohr_physics_angular_velocity_radians_set(body, -0.8f));
        OK(rohr_physics_dynamic_set(body));
        OK(rohr_physics_center_of_mass_local_position_set(body, (Position){30,15}));
        JointAnchorIdResult created_anchor = rohr_physics_joint_anchor_create(body, (Position){100,30});
        OK(created_anchor); anchor = created_anchor.result.value;
    }
    Position initial = rohr_physics_center_of_mass_world_position_get(body).result.value;
    for(int i = 0; i < 120; i += 1) OK(rohr_system_physics_update(1.0 / 120.0));
    snapshot->origin = rohr_physics_position_get(body).result.value;
    snapshot->com = rohr_physics_center_of_mass_world_position_get(body).result.value;
    snapshot->anchor = rohr_physics_joint_anchor_world_position_get(anchor).result.value;
    snapshot->angle = orientations[rohr_entity_index_get(body).result.value];
    CHECK(near(initial, snapshot->com) && !near(snapshot->origin, (Position){120,-30}));
    CHECK(fabsf(rohr_math_degrees_to_radians(snapshot->angle) + 1.15f) < 0.002f);
    Shape world = rohr_physics_shape_world_translate(shape, snapshot->origin, snapshot->angle);
    ShapeResult actual_world = rohr_physics_global_hit_box_get(body); OK(actual_world);
    CHECK(near(actual_world.result.value.vertices[0], world.vertices[0]));
    Vec2D lever = rohr_math_vector_rotate((Vec2D){100,30}, snapshot->angle);
    CHECK(near(snapshot->anchor, (Position){snapshot->origin.x + lever.x, snapshot->origin.y + lever.y}));
    CHECK(near(rohr_physics_joint_anchor_local_position_get(anchor).result.value, (Position){100,30}));
    CHECK(fabsf(rohr_physics_moment_of_inertia_get(body).result.value - 21500) < 0.01f);
    /* Invalid geometry leaves the current mass description usable. */
    CHECK(rohr_error_check(rohr_physics_hitbox_set(body, (Shape){0})));
    CHECK(near(rohr_physics_center_of_mass_world_position_get(body).result.value, initial));
    Position moved = {snapshot->origin.x + 10, snapshot->origin.y - 20};
    OK(rohr_physics_position_set(body, moved));
    CHECK(near(rohr_physics_center_of_mass_local_position_get(body).result.value, (Position){30,15}));
    CHECK(near(rohr_physics_joint_anchor_local_position_get(anchor).result.value, (Position){100,30}));
    CHECK(near(rohr_physics_center_of_mass_world_position_get(body).result.value,
        (Position){initial.x + 10, initial.y - 20}));
    OK(rohr_physics_orientation_set(body, 0));
    CHECK(near(rohr_physics_position_get(body).result.value, moved));
    CHECK(near(rohr_physics_center_of_mass_world_position_get(body).result.value,
        (Position){moved.x + 30, moved.y + 15}));
    if(generated) mass_demo_destroy(&demo);
    rohr_engine_stop();
    return true;
}

int main(void) {
    Snapshot generated, direct;
    return authoring_check() && simulation_check(true, &generated) &&
        simulation_check(false, &direct) && near(generated.origin, direct.origin) &&
        near(generated.com, direct.com) && near(generated.anchor, direct.anchor) &&
        fabsf(generated.angle - direct.angle) < 0.002f ? 0 : 1;
}
