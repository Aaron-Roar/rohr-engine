/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_soft_area.h"
#include "editor_command.h"
#include "editor_workspace.h"
#include "yyjson/yyjson.h"
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "editor areas line %d: %s\n", __LINE__, #c); return false; } } while(0)

static bool areas_check(void) {
    EditorProject project, loaded;
    editor_project_init(&project);
    editor_project_init(&loaded);
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    CHECK(object != NULL);
    snprintf(object->name, sizeof(object->name), "AreaFixture");
    EditorCommand add = {.type = EDITOR_COMMAND_ITEM_ADD,
        .data.item_add = {.kind = EDITOR_ITEM_SOFT_BODY, .object = object->id}};
    CHECK(editor_command_execute(&project, &add).kind != ERROR_RESULT_ERROR);
    EditorSoftBody *body = &object->soft_body_items[0];
    CHECK(body->node_count == 4 && body->beam_count == 6 && body->area_count == 1);
    snprintf(body->name, sizeof(body->name), "cloth");
    EditorSoftArea *area = &body->areas[0];
    CHECK(area->outer.node_count == 4 && area->visible && area->graphics_layer_inherited);
    const Position square[] = {{-70,-70},{70,-70},{70,70},{-70,70}};
    for(size_t i = 0; i < 4; i += 1) body->nodes[i].position = square[i];
    EditorSoftHole *hole = editor_soft_hole_add(area);
    CHECK(hole != NULL);
    hole->loop.node_count = 4;
    for(size_t i = 0; i < 4; i += 1) {
        EditorSoftNode *node = editor_project_soft_node_add(&project, body,
            (Position){square[i].x / 3, square[i].y / 3});
        CHECK(node != NULL);
        hole->loop.nodes[i] = node->id;
    }
    CHECK(editor_soft_area_fill_get(body, area).complete);
    CHECK(!editor_soft_area_point_check(body, area, (Position){0,0}));
    CHECK(editor_soft_area_point_check(body, area, (Position){50,0}));
    CHECK(!editor_soft_area_point_check(body, area, (Position){80,0}));
    const Vec2D *cached = editor_soft_area_fill_get(body, area).vertices;
    CHECK(cached != NULL && cached == editor_soft_area_fill_get(body, area).vertices);
    area->color = UINT32_C(0x22448880);
    area->graphics_layer_inherited = false;
    EditorGraphicsLayer *layer = editor_project_graphics_layer_add(&project, "cloth_layer", 9);
    CHECK(layer != NULL);
    area->graphics_layer.layer = layer->id;
    CHECK(cached == editor_soft_area_fill_get(body, area).vertices);
    /* Moving an owned hole outside restores the outer fill. */
    for(size_t i = 4; i < 8; i += 1) body->nodes[i].position.x += 200;
    CHECK(editor_soft_area_point_check(body, area, (Position){0,0}));
    for(size_t i = 4; i < 8; i += 1) body->nodes[i].position.x -= 200;
    /* Swapping bottom endpoints gives an hourglass, not an exterior triangle. */
    body->nodes[0].position = square[1]; body->nodes[1].position = square[0];
    CHECK(editor_soft_area_fill_get(body, area).complete);
    CHECK(!editor_soft_area_point_check(body, area, (Position){60,0}));
    body->nodes[0].position = square[0]; body->nodes[1].position = square[1];
    /* Draft holes suppress the entire fill, survive persistence, and identify the error. */
    hole->loop.node_count = 2;
    CHECK(!editor_soft_area_fill_get(body, area).complete);
    CHECK(!editor_soft_area_point_check(body, area, (Position){50,0}));
    project.navigation = (EditorNavigationState){.mode = EDITOR_NAVIGATION_MODE_MAX,
        .selection = EDITOR_NAVIGATION_SELECTION_MAX, .object = object->id,
        .soft_body = body->id, .soft_area = area->id, .soft_hole = hole->id};
    CHECK(editor_project_save(&project, "editor_soft_areas.json"));
    CHECK(!editor_result_check(editor_project_load(&loaded, "editor_soft_areas.json")));
    CHECK(loaded.navigation.soft_area == area->id && loaded.navigation.soft_hole == hole->id);
    EditorSoftArea *saved = &loaded.objects[0].soft_body_items[0].areas[0];
    CHECK(saved->id == area->id && saved->holes[0].id == hole->id &&
        saved->holes[0].loop.node_count == 2 && saved->color == area->color &&
        saved->graphics_layer.layer == layer->id && saved->cache == NULL);
    EditorResult validation = editor_soft_areas_generation_validate(&loaded);
    CHECK(editor_result_check(validation));
    CHECK(strstr(validation.result.error.message, "AreaFixture / cloth / area_1 / hole_1") != NULL);
    hole->loop.node_count = 4;
    /* Full model copies own their caches; deleting hole nodes removes dependent areas only. */
    EditorSoftBody copy = {0};
    CHECK(editor_project_soft_body_clone(&copy, body));
    CHECK(copy.areas != body->areas && copy.areas[0].cache == NULL);
    CHECK(editor_soft_area_fill_get(&copy, &copy.areas[0]).complete);
    CHECK(copy.areas[0].cache != area->cache);
    CHECK(editor_project_soft_node_remove(&project, &copy, hole->loop.nodes[0]));
    CHECK(copy.area_count == 0 && body->area_count == 1);
    CHECK(editor_project_soft_body_copy_set(&copy, body));
    CHECK(copy.area_count == 1 && copy.areas[0].cache == NULL);
    editor_project_soft_body_destroy(&copy);
    /* Hole limits include drafts. */
    for(size_t i = 1; i < SOFT_BODY_MAX_AREA_HOLES; i += 1)
        CHECK(editor_soft_hole_add(area) != NULL);
    CHECK(editor_soft_hole_add(area) == NULL && area->hole_count == 16);
    area->hole_count = 1;
    /* Generation consumes the ordered node IDs, not reference poses. */
    EditorWorkspace workspace = {.config = editor_workspace_config_default_get(), .open = true};
    snprintf(workspace.directory, sizeof(workspace.directory), "editor_soft_areas_generated");
    CHECK(SDL_CreateDirectory("editor_soft_areas_generated/src/generated"));
    CHECK(SDL_SaveFile("editor_soft_areas_generated/src/main.c", "/* developer entry point */", 27));
    CHECK(editor_workspace_c_generate(&workspace, &project));
    char *first = SDL_LoadFile("editor_soft_areas_generated/src/generated/project_objects.c", NULL);
    CHECK(first != NULL && strstr(first, "geometry.holes[0].node_count = 4") != NULL);
    CHECK(strstr(first, "cloth_areas[0]") != NULL);
    hole->loop.node_count = 0;
    EditorWorkspaceCommand generate = {.type = EDITOR_WORKSPACE_COMMAND_GENERATE_C};
    validation = editor_workspace_command_execute(&workspace, &project, &generate);
    CHECK(editor_result_check(validation) && strstr(validation.result.error.message, "hole_1") != NULL);
    char *second = SDL_LoadFile("editor_soft_areas_generated/src/generated/project_objects.c", NULL);
    CHECK(second != NULL && strcmp(first, second) == 0);
    SDL_free(first); SDL_free(second);
    hole->loop.node_count = 4;
    /* Empty outer drafts also round-trip; malformed references never load. */
    EditorSoftArea *draft = editor_soft_area_add(body);
    CHECK(draft != NULL && editor_project_save(&project, "editor_soft_areas.json"));
    CHECK(!editor_result_check(editor_project_load(&loaded, "editor_soft_areas.json")));
    CHECK(loaded.objects[0].soft_body_items[0].area_count == 2);
    draft->outer = (EditorSoftAreaLoop){.node_count = 1, .nodes = {UINT32_MAX}};
    CHECK(editor_project_save(&project, "editor_soft_areas.json"));
    CHECK(editor_result_check(editor_project_load(&loaded, "editor_soft_areas.json")));
    CHECK(loaded.objects[0].soft_body_items[0].area_count == 2);
    CHECK(editor_soft_area_remove(body, draft->id));
    CHECK(editor_project_graphics_layer_remove(&project, layer->id));
    CHECK(body->areas[0].graphics_layer.layer == 0 && body->areas[0].graphics_layer.value == 9);
    editor_project_destroy(&project); editor_project_destroy(&loaded);
    CHECK(SDL_RemovePath("editor_soft_areas.json"));
    return true;
}

static bool authoring_check(void) {
    EditorProject project;
    editor_project_init(&project);
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    CHECK(object != NULL);
    snprintf(object->name, sizeof(object->name), "cloth");
    EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
        .data.item_add = {.kind = EDITOR_ITEM_SOFT_BODY, .object = object->id, .name = "fabric"}};
    CHECK(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    EditorSoftBody *body = &object->soft_body_items[0];
    const char *operations[] = {
        "--area patch add",
        "--area patch --property nodes node_1 node_2 node_3 node_4",
        "--area patch --property color aa227780",
        "--area patch --property visibility false",
        "--area patch --property layer -3",
        "--area patch --property order 0",
        "--area patch --hole window add",
        "--area patch --hole window --property nodes node_1 node_2 node_3",
        "--area patch --hole window rename opening",
        "--area patch --hole opening --property node-ids",
        "--area patch --hole opening delete",
        "--area patch rename painted",
    };
    for(size_t i = 0; i < sizeof(operations) / sizeof(operations[0]); i += 1) {
        char text[2048]; char *argv[100]; int argc = 0; const char *path;
        snprintf(text, sizeof(text), "rohr-cli --project areas.json --object cloth --soft-body fabric %s", operations[i]);
        for(char *token = strtok(text, " "); token != NULL; token = strtok(NULL, " ")) argv[argc++] = token;
        EditorResult parsed = editor_command_cli_standard_parse(&project, argc, argv, &path, &command);
        if(editor_result_check(parsed)) fprintf(stderr, "%s: %s\n", operations[i], parsed.result.error.message);
        CHECK(!editor_result_check(parsed));
        /* Serialize before mutation and replay the same shared command. */
        char written[2048]; EditorCommand replay;
        CHECK(!editor_result_check(editor_command_cli_standard_write(&project, &command, NULL,
            "areas.json", written, sizeof(written))));
        argc = 0;
        for(char *token = strtok(written, " "); token != NULL; token = strtok(NULL, " ")) argv[argc++] = token;
        CHECK(!editor_result_check(editor_command_cli_standard_parse(&project, argc, argv, &path, &replay)));
        CHECK(replay.type == command.type);
        CHECK(editor_command_execute(&project, &replay).kind == ERROR_RESULT_VALUE);
    }
    CHECK(body->area_count == 2);
    EditorSoftArea *area = &body->areas[0];
    CHECK(strcmp(area->name, "painted") == 0 && area->hole_count == 0);
    CHECK(area->color == UINT32_C(0xaa227780) && !area->visible && area->graphics_layer.value == -3);
    CHECK(!area->graphics_layer_inherited && area->outer.node_count == 4);
    command = (EditorCommand){.type = EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set = {.kind = EDITOR_ITEM_SOFT_AREA, .object = object->id,
            .parent = body->id, .item = area->id, .property = EDITOR_PROPERTY_COLOR,
            .value_kind = EDITOR_PROPERTY_VALUE_BOOL, .value.boolean = true}};
    CHECK(editor_command_execute(&project, &command).kind == ERROR_RESULT_ERROR);
    CHECK(area->color == UINT32_C(0xaa227780));
    EditorSoftAreaLoop previous = area->outer;
    command = (EditorCommand){.type = EDITOR_COMMAND_SOFT_AREA_LOOP_SET,
        .data.soft_area_loop = {object->id, body->id, area->id, 0,
            {.node_count = 3, .nodes = {previous.nodes[0],previous.nodes[0],previous.nodes[1]}}}};
    CHECK(editor_command_execute(&project, &command).kind == ERROR_RESULT_ERROR);
    CHECK(memcmp(&previous, &area->outer, sizeof(previous)) == 0);
    command.data.soft_area_loop.loop.nodes[1] = UINT32_MAX;
    CHECK(editor_command_execute(&project, &command).kind == ERROR_RESULT_ERROR);
    CHECK(memcmp(&previous, &area->outer, sizeof(previous)) == 0);
    command = (EditorCommand){.type = EDITOR_COMMAND_ITEM_ADD,
        .data.item_add = {.kind = EDITOR_ITEM_SOFT_HOLE, .object = object->id,
            .parent = body->id, .first = area->id}};
    for(size_t i = 0; i < 16; i += 1)
        CHECK(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    EditorCommandResult result = editor_command_execute(&project, &command);
    CHECK(result.kind == ERROR_RESULT_ERROR && strstr(result.result.error.message, "maximum 16") != NULL);
    CHECK(area->hole_count == 16);
    editor_project_destroy(&project);
    return true;
}
int main(void) { return areas_check() && authoring_check() ? 0 : 1; }
