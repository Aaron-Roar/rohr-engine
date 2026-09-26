/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_command.h"
#include "editor_workspace.h"
#include "editor_document.h"
#include "rohr.h"
#include "yyjson/yyjson.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "COM persistence line %d: %s\n", __LINE__, #c); return false; } } while(0)
#define OK(c) CHECK(!rohr_error_check(c))
static bool point_equal(Position a, Position b) {
    return fabsf(a.x-b.x) < 0.002f && fabsf(a.y-b.y) < 0.002f;
}
static bool write_file(const char *path, const char *text) {
    FILE *file = fopen(path, "wb");
    if(file == NULL) return false;
    bool result = fwrite(text, 1, strlen(text), file) == strlen(text);
    return fclose(file) == 0 && result;
}
static Entity named_entity(const char *name) {
    EntityResult result = rohr_entity_by_name_get(name);
    return rohr_error_check(result) ? ENTITY_INVALID : result.result.value;
}

static bool runtime_check(void) {
    const char *source = "com_state_source.json";
    const char *saved = "com_state_saved.json";
    const char *authored = "com_state_template.json";
    const char *invalid = "com_state_invalid.json";
    const char *initial = "{\"version\":3,\"entities\":["
        "{\"name\":\"explicit_body\",\"components\":{\"mass\":12,"
        "\"position\":{\"x\":10,\"y\":20},\"orientation\":-450,\"angular_velocity\":810,"
        "\"hit_box\":[{\"x\":2,\"y\":3},{\"x\":6,\"y\":3},{\"x\":6,\"y\":5},{\"x\":2,\"y\":5}],"
        "\"center_of_mass\":{\"mode\":\"explicit\",\"offset\":{\"x\":3,\"y\":-2}}}},"
        "{\"name\":\"automatic_body\",\"components\":{\"mass\":12,"
        "\"hit_box\":[{\"x\":2,\"y\":3},{\"x\":6,\"y\":3},{\"x\":6,\"y\":5},{\"x\":2,\"y\":5}]}},"
        "{\"name\":\"empty_body\",\"components\":{\"center_of_mass\":{\"mode\":\"explicit\","
        "\"offset\":{\"x\":-4,\"y\":8}}}}]}";
    CHECK(write_file(source, initial));
    OK(rohr_engine_start());
    OK(rohr_game_state_file_load(source));
    Entity body = named_entity("explicit_body");
    CHECK(body != ENTITY_INVALID);
    CHECK(orientations[rohr_entity_index_get(body).result.value] == -450);
    CHECK(rohr_physics_angular_velocity_get(body).result.value == 810);
    CHECK(point_equal(rohr_physics_center_of_mass_local_position_get(body).result.value,
        (Position){3,-2}));
    OK(rohr_physics_center_of_mass_local_position_set(body, (Position){8,9}));
    Entity empty_body = named_entity("empty_body");
    OK(rohr_physics_collision_category_set(empty_body, UINT64_C(32)));
    OK(rohr_physics_collision_with_set(empty_body, UINT64_C(64)));
    CHECK(!rohr_entity_components_check(empty_body, ROHR_HIT_BOX));
    OK(rohr_physics_particle_origin_set(named_entity("automatic_body"), (Position){0}));
    ParticleConfig config = rohr_physics_particle_config_default_get();
    config.local_origin = (Position){4,2};
    config.radius = 2;
    config.collision_enabled = false;
    config.collision_filter = (CollisionFilterConfig){.category=UINT64_C(8), .collides_with=UINT64_C(16)};
    EntityResult particle = rohr_physics_particle_create(config);
    OK(particle);
    OK(rohr_entity_name_set(particle.result.value, "pure_particle"));
    CHECK(rohr_error_check(rohr_physics_center_of_mass_local_position_set(
        particle.result.value, (Position){0})));
    CHECK(point_equal(rohr_physics_center_of_mass_world_position_get(particle.result.value).result.value,
        (Position){4,2}));
    OK(rohr_physics_particle_origin_set(particle.result.value, (Position){5,3}));
    CHECK(point_equal(rohr_physics_center_of_mass_local_position_get(particle.result.value).result.value,
        (Position){5,3}));
    const char *bad[] = {
        "{\"mode\":\"unknown\"}", "{\"mode\":\"explicit\"}",
        "{\"mode\":\"explicit\",\"offset\":{\"x\":1}}",
        "{\"mode\":\"explicit\",\"offset\":{\"x\":1e100,\"y\":0}}",
        "{\"mode\":\"automatic\",\"offset\":{\"x\":0,\"y\":0}}"
    };
    uint32_t count = entity_alive_count_get();
    for(size_t i=0; i<sizeof(bad)/sizeof(bad[0]); i+=1) {
        char json[1024];
        snprintf(json, sizeof(json), "{\"version\":3,\"entities\":[{\"name\":\"failed_body\","
            "\"components\":{\"center_of_mass\":%s}}]}", bad[i]);
        CHECK(write_file(invalid, json));
        CHECK(rohr_error_check(rohr_game_state_file_load(invalid)));
        CHECK(entity_alive_count_get() == count && named_entity("failed_body") == ENTITY_INVALID);
        CHECK(point_equal(rohr_physics_center_of_mass_local_position_get(body).result.value,
            (Position){8,9}));
    }
    CHECK(write_file(invalid, "{\"version\":2,\"entities\":[]}"));
    CHECK(rohr_error_check(rohr_game_state_file_load(invalid)));
    CHECK(entity_alive_count_get() == count &&
        orientations[rohr_entity_index_get(body).result.value] == -450);
    OK(rohr_game_state_file_save(saved));
    OK(rohr_game_state_template_file_save(authored));
    rohr_engine_stop();
    OK(rohr_engine_start());
    OK(rohr_game_state_file_load(saved));
    CHECK(orientations[rohr_entity_index_get(named_entity("explicit_body")).result.value] == -450);
    CHECK(rohr_physics_angular_velocity_get(named_entity("explicit_body")).result.value == 810);
    CHECK(point_equal(rohr_physics_center_of_mass_local_position_get(named_entity("explicit_body")).result.value,
        (Position){8,9}));
    CHECK(rohr_physics_center_of_mass_automatic_check(named_entity("automatic_body")).result.value);
    CHECK(point_equal(rohr_physics_center_of_mass_local_position_get(named_entity("automatic_body")).result.value,
        (Position){4,4}));
    OK(rohr_physics_hitbox_set(named_entity("automatic_body"), rohr_math_square_create(8,2)));
    CHECK(fabsf(rohr_physics_particle_radius_get(named_entity("automatic_body")).result.value - sqrtf(17)) < 0.002f);
    CHECK(point_equal(rohr_physics_center_of_mass_local_position_get(named_entity("empty_body")).result.value,
        (Position){-4,8}));
    CHECK(rohr_error_check(rohr_physics_moment_of_inertia_get(named_entity("empty_body"))));
    CHECK(rohr_physics_collision_filter_get(named_entity("empty_body")).result.value.category == UINT64_C(32));
    CHECK(rohr_physics_collision_filter_get(named_entity("empty_body")).result.value.collides_with == UINT64_C(64));
    body = named_entity("pure_particle");
    CHECK(!rohr_entity_components_check(body, ROHR_COLLISION));
    CHECK(rohr_physics_collision_filter_get(body).result.value.category == UINT64_C(8));
    CHECK(rohr_physics_collision_filter_get(body).result.value.collides_with == UINT64_C(16));
    CHECK(rohr_physics_center_of_mass_automatic_check(body).result.value);
    CHECK(rohr_error_check(rohr_physics_center_of_mass_local_position_set(body, (Position){1,2})));
    CHECK(point_equal(rohr_physics_particle_origin_get(body).result.value, (Position){5,3}));
    CHECK(point_equal(rohr_physics_center_of_mass_local_position_get(body).result.value, (Position){5,3}));
    CHECK(fabsf(rohr_physics_particle_radius_get(body).result.value - 2) < 0.001f);
    OK(rohr_physics_angular_velocity_set(body, 5));
    OK(rohr_system_physics_update(0.01));
    EntityIndex index = rohr_entity_index_get(body).result.value;
    CHECK(orientations[index] == 0 && angular_velocities[index] == 0);
    OK(rohr_entity_delete(body));
    EntityResult reused = rohr_entity_add(); OK(reused);
    OK(rohr_physics_center_of_mass_local_position_set(reused.result.value, (Position){1,2}));
    rohr_engine_stop();
    OK(rohr_engine_start());
    OK(rohr_game_state_file_load(authored));
    CHECK(orientations[rohr_entity_index_get(named_entity("explicit_body")).result.value] == -450);
    CHECK(rohr_physics_angular_velocity_get(named_entity("explicit_body")).result.value == 810);
    CHECK(point_equal(rohr_physics_center_of_mass_local_position_get(named_entity("explicit_body")).result.value,
        (Position){3,-2}));
    CHECK(named_entity("pure_particle") == ENTITY_INVALID);
    rohr_engine_stop();
    remove(source); remove(saved); remove(authored); remove(invalid);
    return true;
}

static bool circle_placement_check(void) {
    const float angles[] = {0, 90, 810, -450};
    const Position expected[] = {{10,0}, {0,-10}, {0,-10}, {0,10}};
    for(size_t i = 0; i < 4; i += 1) {
        char json[512];
        snprintf(json, sizeof(json), "{\"version\":%u,\"entities\":[{\"name\":\"ring\","
            "\"count\":2,\"components\":{\"position\":{\"x\":0,\"y\":0}},"
            "\"placement\":{\"type\":\"circle\",\"radius\":10,\"start_angle\":%g}}]}",
            GAME_STATE_VERSION, angles[i]);
        CHECK(write_file("angle_circle.json", json));
        OK(rohr_engine_start());
        OK(rohr_game_state_file_load("angle_circle.json"));
        CHECK(point_equal(rohr_physics_position_get(named_entity("ring")).result.value, expected[i]));
        CHECK(point_equal(rohr_physics_position_get(named_entity("ring_1")).result.value,
            (Position){-expected[i].x, -expected[i].y}));
        rohr_engine_stop();
    }
    remove("angle_circle.json");
    return true;
}

static bool project_check(void) {
    EditorProject project, loaded;
    EditorWorkspace workspace = {.config = editor_workspace_config_default_get(), .open = true};
    EditorCommand command;
    const char *path;
    char serialized[1024];
    editor_project_init(&project);
    editor_project_init(&loaded);
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    CHECK(object != NULL);
    snprintf(object->name, sizeof(object->name), "Fleet");
    EditorRigidBody *body = editor_project_rigid_body_add(&project, object);
    CHECK(body != NULL);
    snprintf(body->name, sizeof(body->name), "offset_body");
    body->position = (Position){10,20}; body->rotation = -450;
    body->initial_velocity = (Velocity){2,-1}; body->initial_angular_velocity = 810;
    body->mass_value = 12;
    EditorHitbox *hitbox = &body->hitboxes[0];
    CHECK(editor_project_hitbox_vertex_insert(&project, hitbox, 0));
    CHECK(hitbox->vertex_count == 4);
    const Position vertices[] = {{2,3},{6,3},{6,5},{2,5}};
    for(size_t i=0;i<4;i+=1) hitbox->vertices[i].position = vertices[i];
    char *args[] = {"rohr-cli", "--project", "com_project.json", "--object", "Fleet",
        "--body", "offset_body", "--property", "center-of-mass", "explicit", "3", "-2"};
    CHECK(!editor_result_check(editor_command_cli_standard_parse(&project, 12, args, &path, &command)));
    CHECK(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    CHECK(body->center_of_mass_explicit && point_equal(body->center_of_mass_offset, (Position){3,-2}));
    CHECK(!editor_result_check(editor_command_cli_write(&command, path, serialized, sizeof(serialized))));
    CHECK(strstr(serialized, "center-of-mass explicit 3 -2") != NULL);
    CHECK(editor_result_check(editor_command_cli_standard_parse(&project, 11, args, &path, &command)));
    args[11] = "nan";
    CHECK(editor_result_check(editor_command_cli_standard_parse(&project, 12, args, &path, &command)));
    args[11] = "-2";
    char *angle_args[] = {"rohr-cli", "--project", "com_project.json", "--object", "Fleet",
        "--body", "offset_body", "--property", "rotation", "810"};
    CHECK(!editor_result_check(editor_command_cli_standard_parse(&project, 10,
        angle_args, &path, &command)));
    CHECK(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    CHECK(body->rotation == 810);
    CHECK(!editor_result_check(editor_command_cli_write(&command, path, serialized, sizeof(serialized))));
    CHECK(strstr(serialized, "810") != NULL);
    angle_args[9] = "-450";
    CHECK(!editor_result_check(editor_command_cli_standard_parse(&project, 10,
        angle_args, &path, &command)));
    CHECK(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    CHECK(body->rotation == -450);
    angle_args[8] = "initial-angular-velocity";
    angle_args[9] = "810";
    CHECK(!editor_result_check(editor_command_cli_standard_parse(&project, 10,
        angle_args, &path, &command)));
    CHECK(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    CHECK(body->initial_angular_velocity == 810);
    CHECK(editor_project_save(&project, "com_project.json"));
    CHECK(!editor_result_check(editor_project_load(&loaded, "com_project.json")));
    CHECK(loaded.objects[0].rigid_bodies[0].center_of_mass_explicit);
    CHECK(loaded.objects[0].rigid_bodies[0].rotation == -450 &&
        loaded.objects[0].rigid_bodies[0].initial_angular_velocity == 810);
    CHECK(point_equal(loaded.objects[0].rigid_bodies[0].center_of_mass_offset, (Position){3,-2}));

    yyjson_doc *document = yyjson_read_file("com_project.json", 0, NULL, NULL);
    CHECK(document != NULL);
    yyjson_mut_doc *copy = yyjson_doc_mut_copy(document, NULL);
    CHECK(copy != NULL);
    yyjson_mut_val *root = yyjson_mut_doc_get_root(copy);
    yyjson_mut_val *record = yyjson_mut_arr_get(yyjson_mut_obj_get(
        yyjson_mut_arr_get(yyjson_mut_obj_get(root, "objects"), 0), "rigid_bodies"), 0);
    CHECK(record != NULL);
    yyjson_mut_val *com = yyjson_mut_obj_get(record, "center_of_mass");
    CHECK(yyjson_mut_obj_remove_key(com, "offset") != NULL);
    CHECK(yyjson_mut_write_file("com_project_bad.json", copy, 0, NULL, NULL));
    CHECK(editor_result_check(editor_project_load(&loaded, "com_project_bad.json")));
    CHECK(point_equal(loaded.objects[0].rigid_bodies[0].center_of_mass_offset, (Position){3,-2}));
    yyjson_mut_obj_remove_key(record, "center_of_mass");
    CHECK(yyjson_mut_write_file("com_project_bad.json", copy, 0, NULL, NULL));
    CHECK(!editor_result_check(editor_project_load(&loaded, "com_project_bad.json")));
    CHECK(!loaded.objects[0].rigid_bodies[0].center_of_mass_explicit);
    yyjson_mut_obj_put(root, yyjson_mut_str(copy, "format_version"), yyjson_mut_uint(copy, 3));
    CHECK(yyjson_mut_write_file("com_project_bad.json", copy, 0, NULL, NULL));
    CHECK(editor_result_check(editor_project_load(&loaded, "com_project_bad.json")));
    CHECK(loaded.objects[0].rigid_bodies[0].rotation == -450);
    yyjson_mut_doc_free(copy); yyjson_doc_free(document);
    EditorDocument owned;
    CHECK(!editor_result_check(editor_document_create(&owned)));
    CHECK(!editor_result_check(editor_document_load(&owned, "com_project.json")));
    CHECK(!editor_result_check(editor_document_load(&owned, "com_project.json")));
    CHECK(editor_result_check(editor_document_load(&owned, "com_project_bad.json")));
    CHECK(point_equal(owned.project->objects[0].rigid_bodies[0].center_of_mass_offset,
        (Position){3,-2}));
    editor_document_destroy(&owned);

    // Preserve a generated fixture for independent SDK compilation and execution.
    snprintf(workspace.directory, sizeof(workspace.directory), "com_persistence_generated");
    CHECK(SDL_CreateDirectory(workspace.directory));
    CHECK(SDL_CreateDirectory("com_persistence_generated/src"));
    CHECK(SDL_CreateDirectory("com_persistence_generated/src/generated"));
    CHECK(write_file("com_persistence_generated/src/main.c", "/* developer-owned entry point */\n"));
    EditorRigidBody *empty = editor_project_rigid_body_add(&project, object);
    CHECK(empty != NULL);
    snprintf(empty->name, sizeof(empty->name), "empty_body");
    CHECK(editor_project_hitbox_remove(empty, empty->hitboxes[0].id));
    empty->center_of_mass_explicit = true; empty->center_of_mass_offset = (Position){7,-3};
    EditorRigidBody *automatic = editor_project_rigid_body_add(&project, object);
    CHECK(automatic != NULL);
    snprintf(automatic->name, sizeof(automatic->name), "auto_body");
    hitbox = &automatic->hitboxes[0];
    CHECK(editor_project_hitbox_vertex_insert(&project, hitbox, 0));
    for(size_t i=0;i<4;i+=1) hitbox->vertices[i].position = vertices[i];
    EditorRigidBody *particle = editor_project_rigid_body_add(&project, object);
    CHECK(particle != NULL);
    snprintf(particle->name, sizeof(particle->name), "pure_particle");
    particle->particle = true; particle->standalone_particle = true;
    particle->particle_auto_fit = false; particle->particle_origin = (Position){4,2};
    particle->particle_radius = 2;
    CHECK(editor_project_particle_hitbox_sync(&project, particle));
    command = (EditorCommand){.type = EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set = {.kind = EDITOR_ITEM_RIGID_BODY, .object = object->id,
            .item = particle->id, .property = EDITOR_PROPERTY_COLLISION,
            .value_kind = EDITOR_PROPERTY_VALUE_BOOL, .value.boolean = false}};
    CHECK(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    CHECK(particle->particle && particle->standalone_particle && !particle->collision_enabled);
    CHECK(editor_project_save(&project, "com_project.json"));
    CHECK(!editor_result_check(editor_project_load(&loaded, "com_project.json")));
    CHECK(loaded.objects[0].rigid_bodies[3].standalone_particle &&
        loaded.objects[0].rigid_bodies[3].particle &&
        !loaded.objects[0].rigid_bodies[3].collision_enabled);
    CHECK(editor_workspace_c_generate(&workspace, &project));
    char *first = SDL_LoadFile("com_persistence_generated/src/generated/project_objects.c", NULL);
    CHECK(first != NULL);
    CHECK(editor_workspace_c_generate(&workspace, &project));
    char *second = SDL_LoadFile("com_persistence_generated/src/generated/project_objects.c", NULL);
    CHECK(second != NULL && strcmp(first, second) == 0);
    SDL_free(second);
    particle->center_of_mass_explicit = true;
    CHECK(!editor_project_save(&project, "com_project.json"));
    CHECK(!editor_workspace_c_generate(&workspace, &project));
    second = SDL_LoadFile("com_persistence_generated/src/generated/project_objects.c", NULL);
    CHECK(second != NULL && strcmp(first, second) == 0);
    SDL_free(first); SDL_free(second);
    particle->center_of_mass_explicit = false;
    body = &object->rigid_bodies[0];
    args[9] = "automatic";
    CHECK(!editor_result_check(editor_command_cli_standard_parse(&project, 10, args, &path, &command)));
    CHECK(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    CHECK(!body->center_of_mass_explicit && point_equal(body->center_of_mass_offset, (Position){0}));
    args[9] = "explicit"; args[6] = "pure_particle";
    CHECK(!editor_result_check(editor_command_cli_standard_parse(&project, 12, args, &path, &command)));
    CHECK(editor_command_execute(&project, &command).kind == ERROR_RESULT_ERROR);
    CHECK(!particle->center_of_mass_explicit);
    editor_project_destroy(&project); editor_project_destroy(&loaded);
    remove("com_project.json"); remove("com_project_bad.json");
    return true;
}

int main(void) {
    return runtime_check() && circle_placement_check() && project_check() ? 0 : 1;
}
