/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_workspace.h"
#include "editor_starter_main.h"
#include "editor_starter_main_legacy.h"
#include "editor_scene_source.h"
#include "editor_log.h"
#include "editor_object_catalog.h"
#include "editor_soft_area.h"
#include "physics/soft_body/soft_body_motion.h"

#include "editor_config.h"

#include "yyjson/yyjson.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const EditorSprite *editor_workspace_sprite_get(const EditorObject *object,
        EditorSpriteId id) {
    if(object == NULL || id == 0) return NULL;
    for(size_t i = 0; i < object->sprite_count; i += 1)
        if(object->sprites[i].id == id) return &object->sprites[i];
    return NULL;
}

static void editor_workspace_c_string_write(FILE *file, const char *value) {
    fputc('"', file);
    for(const unsigned char *at = (const unsigned char *)value;
            at != NULL && *at != '\0'; at += 1) {
        if(*at == '\\' || *at == '"') fputc('\\', file);
        if(*at == '\n') fputs("\\n", file);
        else if(*at == '\r') fputs("\\r", file);
        else if(*at == '\t') fputs("\\t", file);
        else fputc(*at, file);
    }
    fputc('"', file);
}

static bool editor_workspace_path_join(char *output, size_t capacity,
    const char *directory, const char *relative) {
    return !editor_result_check(editor_workspace_path_resolve(directory, relative, output, capacity));
}

/* Stable generated filenames are mapped through the workspace's storage paths. */
static bool editor_workspace_file_path(const EditorWorkspace *workspace, char *output,
        size_t capacity, const char *canonical) {
    const char *suffix = canonical;
    EditorWorkspacePath kind = EDITOR_PATH_PROJECT;
    if(!strncmp(canonical, "src/generated", 13) && (!canonical[13] || canonical[13] == '/')) {
        kind = EDITOR_PATH_GENERATED; suffix += 13;
    } else if(!strncmp(canonical, "src/", 4)) { kind = EDITOR_PATH_SOURCE; suffix += 3; }
    if(*suffix == '/' && kind != EDITOR_PATH_PROJECT) ++suffix;
    char base[EDITOR_WORKSPACE_PATH_MAX * 2];
    return !editor_result_check(editor_workspace_path_get(workspace, kind, base, sizeof(base))) &&
        editor_workspace_path_join(output, capacity, base, suffix);
}

static const char *editor_workspace_basename(const char *path) {
    const char *name = path;

    if(path == NULL) return "Game";
    for(const char *cursor = path; *cursor != '\0'; cursor += 1) {
        if(*cursor == '/' || *cursor == '\\') name = cursor + 1;
    }
    return name[0] == '\0' ? "Game" : name;
}

static bool editor_workspace_file_write(const char *path, const char *contents) {
    FILE *file;
    size_t length;

    if(path == NULL || contents == NULL) return false;
    file = fopen(path, "wb");
    if(file == NULL) return false;
    length = strlen(contents);
    if(fwrite(contents, 1, length, file) != length) {
        fclose(file);
        return false;
    }
    return fclose(file) == 0;
}

static bool editor_workspace_directory_create(const char *root, const char *relative) {
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];

    return editor_workspace_path_join(path, sizeof(path), root, relative) &&
        SDL_CreateDirectory(path);
}

static SDL_EnumerationResult SDLCALL editor_workspace_not_empty(void *userdata,
    const char *dirname, const char *filename) {
    bool *empty = userdata;

    (void)dirname;
    (void)filename;
    if(empty != NULL) *empty = false;
    return SDL_ENUM_SUCCESS;
}

static bool editor_workspace_directory_empty(const char *directory) {
    bool empty = true;

    return directory != NULL &&
        SDL_EnumerateDirectory(directory, editor_workspace_not_empty, &empty) && empty;
}

static EditorResult editor_workspace_manifest_load(EditorWorkspace *workspace,
    const char *directory, bool *recognized);

static EditorResult editor_workspace_root_find(char *root, size_t capacity,
        const char *selection, EditorWorkspace *workspace) {
    SDL_PathInfo info;
    size_t length;
    EditorResult result;
    bool recognized;

    if(root == NULL || capacity == 0 || selection == NULL || selection[0] == '\0' ||
            workspace == NULL ||
            snprintf(root, capacity, "%s", selection) >= (int)capacity)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "Workspace root search received an invalid or oversized path");
    if(SDL_GetPathInfo(root, &info) && info.type == SDL_PATHTYPE_FILE) {
        char *separator = strrchr(root, '/');
#if defined(_WIN32)
        char *backslash = strrchr(root, '\\');
        if(backslash != NULL && (separator == NULL || backslash > separator))
            separator = backslash;
#endif
        if(separator == NULL) return editor_result_error(
            EDITOR_ERROR_PROJECT_ROOT_NOT_FOUND,
            "Could not determine the parent directory of: %s", selection);
        *separator = '\0';
    }
    for(;;) {
        recognized = false;
        result = editor_workspace_manifest_load(workspace, root, &recognized);
        if(recognized || !editor_result_check(result)) return result;
        length = strlen(root);
        while(length > 1 && (root[length - 1] == '/' || root[length - 1] == '\\'))
            root[--length] = '\0';
        while(length > 0 && root[length - 1] != '/' && root[length - 1] != '\\')
            length -= 1;
        if(length == 0) break;
        if(length == 1) {
            if(root[0] == '/') break;
            root[0] = '\0';
            break;
        }
        root[length - 1] = '\0';
    }
    return editor_result_error(EDITOR_ERROR_PROJECT_ROOT_NOT_FOUND,
        "Could not find a valid project manifest at or above: %s", selection);
}

static const EditorRigidBody *editor_workspace_body_get(
    const EditorObject *object, EditorRigidBodyId id) {
    if(object == NULL || id == 0) return NULL;
    for(size_t i = 0; i < object->rigid_body_count; i += 1)
        if(object->rigid_bodies[i].id == id) return &object->rigid_bodies[i];
    return NULL;
}

static const EditorAnchor *editor_workspace_anchor_get(
    const EditorObject *object, EditorAnchorId id) {
    if(object == NULL || id == 0) return NULL;
    for(size_t i = 0; i < object->anchor_count; i += 1)
        if(object->anchors[i].id == id) return &object->anchors[i];
    return NULL;
}

static const EditorSoftNode *editor_workspace_soft_node_get(
    const EditorSoftBody *body, EditorSoftNodeId id) {
    if(body == NULL || id == 0) return NULL;
    for(size_t i = 0; i < body->node_count; i += 1)
        if(body->nodes[i].id == id) return &body->nodes[i];
    return NULL;
}

static const EditorSoftBody *editor_workspace_soft_body_get(
        const EditorObject *object, EditorSoftBodyId id) {
    if(object == NULL || id == 0) return NULL;
    for(size_t i = 0; i < object->soft_body_count; i += 1)
        if(object->soft_body_items[i].id == id) return &object->soft_body_items[i];
    return NULL;
}

static void editor_workspace_hitbox_rectangle_set(EditorProject *project,
    EditorHitbox *hitbox, float width, float height) {
    Position vertices[4] = {
        {width * 0.5f, height * 0.5f},
        {width * 0.5f, height * -0.5f},
        {width * -0.5f, height * -0.5f},
        {width * -0.5f, height * 0.5f}
    };

    if(project == NULL || hitbox == NULL) return;
    if(hitbox->vertex_count < 4) {
        hitbox->vertices[3].id = project->next_vertex_id++;
    }
    hitbox->vertex_count = 4;
    for(uint32_t i = 0; i < 4; i += 1) {
        hitbox->vertices[i].position = vertices[i];
        snprintf(hitbox->vertices[i].name, sizeof(hitbox->vertices[i].name),
            "vertex_%u", i + 1);
        snprintf(hitbox->line_names[i], sizeof(hitbox->line_names[i]),
            "line_%u", i + 1);
    }
}

static bool editor_workspace_tug_init(EditorProject *project) {
    EditorObject *object = editor_project_object_add(project, (Position){0});
    if(object == NULL) return false;
    snprintf(object->name, sizeof(object->name), "Tug");
    EditorAnchorId anchors[2];
    for(size_t i = 0; i < 2; ++i) {
        EditorRigidBody *body = editor_project_rigid_body_add(project, object);
        if(body == NULL) return false;
        snprintf(body->name, sizeof(body->name), "box_%zu", i + 1);
        body->position = (Position){i == 0 ? -70.0f : 70.0f, 260.0f};
        body->mass_value = 2.0f;
        body->gravity_enabled = false;
        body->surface_color = i == 0 ? UINT32_C(0x589bd5ff) : UINT32_C(0xd57a58ff);
        editor_workspace_hitbox_rectangle_set(project, &body->hitboxes[0], 40, 40);
        EditorAnchor *anchor = editor_project_anchor_add(project, object,
            (Position){0}, body->id);
        if(anchor == NULL) return false;
        snprintf(anchor->name, sizeof(anchor->name), "box_%zu_anchor", i + 1);
        anchors[i] = anchor->id;
        EditorForce *force = editor_project_force_add(project, object);
        if(force == NULL) return false;
        snprintf(force->name, sizeof(force->name), "pull_%zu", i + 1);
        ObjectSourceReference target = {.kind = OBJECT_SOURCE_BODY, .id = body->id};
        force->description = (ObjectForceDescription){
            .target = target,
            .location = {.kind = ROHR_FORCE_ORIGIN_POSITION, .reference = target},
            .orientation = {.kind = ROHR_FORCE_WORLD_ORIENTATION,
                .offset = i == 0 ? -90.0f : 90.0f},
            .magnitude = 100.0f
        };
    }
    EditorJoint *spring = editor_project_joint_add(project, object, EDITOR_JOINT_SPRING);
    if(spring == NULL) return false;
    snprintf(spring->name, sizeof(spring->name), "tug_spring");
    spring->anchor_a = anchors[0];
    spring->anchor_b = anchors[1];
    spring->rest_length = 140.0f;
    spring->stiffness = 10.0f;
    spring->damping = 2.0f;
    return true;
}

static bool editor_workspace_starter_project_init(EditorProject *project) {
    EditorObject *starter;
    EditorRigidBody *floor_body;
    EditorRigidBody *box_body;
    EditorRigidBody *chassis_body;
    EditorRigidBody *wheel_body;
    EditorRigidBody *welded_body;
    EditorRigidBody *spring_body;
    EditorRigidBody *particle_body;
    EditorAnchor *chassis_pin;
    EditorAnchor *wheel_pin;
    EditorAnchor *chassis_weld;
    EditorAnchor *welded_anchor;
    EditorAnchor *chassis_spring;
    EditorAnchor *spring_anchor;
    EditorJoint *joint;
    EditorSoftBody *soft_body;
    EditorSoftNode *soft_nodes[4];
    EditorSprite *sprite;
    EditorAnimatedSprite *animation;
    EditorCamera *camera;
    EditorLayoutViewport *layout_viewport;
    EditorViewportCameraItem *screen_item;
    EditorViewportUiItem *ui_shape;
    EditorRigidBodyId box_id;
    EditorRigidBodyId chassis_id;
    EditorRigidBodyId wheel_id;
    EditorRigidBodyId welded_id;
    EditorRigidBodyId spring_id;
    EditorRigidBodyId particle_id;
    EditorAnchorId chassis_pin_id;
    EditorAnchorId wheel_pin_id;
    EditorAnchorId chassis_weld_id;
    EditorAnchorId welded_anchor_id;
    EditorAnchorId chassis_spring_id;
    EditorAnchorId spring_anchor_id;

    if(project == NULL) return false;
    editor_project_init(project);
    starter = editor_project_object_add(project, (Position){0});
    if(starter == NULL) return false;
    snprintf(starter->name, sizeof(starter->name), "Starter");
    camera = editor_project_camera_add(project, starter);
    if(camera == NULL) return false;
    snprintf(camera->name, sizeof(camera->name), "main_camera");
    camera->position = (Position){0.0f, 0.0f};
    camera->dimensions = (Scale){WINDOW_WIDTH, WINDOW_HEIGHT};
    layout_viewport = editor_project_layout_viewport_add(project);
    if(layout_viewport == NULL) return false;
    project->simulation_viewport = layout_viewport->id;
    layout_viewport->config.rectangle = (ViewportRectangle){
        0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT};
    snprintf(layout_viewport->name, sizeof(layout_viewport->name),
        "initial_viewport");
    screen_item = editor_viewport_camera_add(project, layout_viewport,
        starter->id, camera->id);
    if(screen_item == NULL) return false;
    screen_item->placement.drag_mode = VIEWPORT_ITEM_DRAG_Y;
    ui_shape = editor_viewport_ui_add(project, layout_viewport,
        EDITOR_VIEWPORT_UI_SHAPE);
    if(ui_shape == NULL) return false;
    snprintf(ui_shape->name, sizeof(ui_shape->name), "sample_ui");
    ui_shape->position = (Position){24.0f, 24.0f};
    ui_shape->drag_mode = VIEWPORT_ITEM_DRAG_X;
    floor_body = editor_project_rigid_body_add(project, starter);
    if(floor_body == NULL) return false;
    snprintf(floor_body->name, sizeof(floor_body->name), "floor");
    floor_body->position = (Position){0.0f, -200.0f};
    floor_body->static_body = true;
    floor_body->friction = 0.5f;
    floor_body->restitution = 0.0f;
    editor_workspace_hitbox_rectangle_set(project, &floor_body->hitboxes[0],
        600.0f, 40.0f);

    box_body = editor_project_rigid_body_add(project, starter);
    if(box_body == NULL) return false;
    snprintf(box_body->name, sizeof(box_body->name), "box");
    box_body->position = (Position){0.0f, 120.0f};
    box_body->mass_value = 5.0f;
    box_body->friction = 0.5f;
    box_body->restitution = 0.0f;
    box_body->gravity_enabled = true;
    editor_workspace_hitbox_rectangle_set(project, &box_body->hitboxes[0],
        50.0f, 50.0f);
    box_id = box_body->id;

    chassis_body = editor_project_rigid_body_add(project, starter);
    if(chassis_body == NULL) return false;
    chassis_id = chassis_body->id;
    wheel_body = editor_project_rigid_body_add(project, starter);
    if(wheel_body == NULL) return false;
    wheel_id = wheel_body->id;
    welded_body = editor_project_rigid_body_add(project, starter);
    if(welded_body == NULL) return false;
    welded_id = welded_body->id;
    spring_body = editor_project_rigid_body_add(project, starter);
    if(spring_body == NULL) return false;
    spring_id = spring_body->id;
    particle_body = editor_project_rigid_body_add(project, starter);
    if(particle_body == NULL) return false;
    particle_id = particle_body->id;
    chassis_body = editor_project_rigid_body_get(starter, chassis_id);
    wheel_body = editor_project_rigid_body_get(starter, wheel_id);
    welded_body = editor_project_rigid_body_get(starter, welded_id);
    spring_body = editor_project_rigid_body_get(starter, spring_id);
    particle_body = editor_project_rigid_body_get(starter, particle_id);
    snprintf(chassis_body->name, sizeof(chassis_body->name), "joint_chassis");
    chassis_body->position = (Position){-220.0f, 20.0f};
    editor_workspace_hitbox_rectangle_set(project, &chassis_body->hitboxes[0],
        120.0f, 30.0f);
    snprintf(wheel_body->name, sizeof(wheel_body->name), "revolute_wheel");
    wheel_body->position = (Position){-160.0f, 20.0f};
    editor_workspace_hitbox_rectangle_set(project, &wheel_body->hitboxes[0],
        42.0f, 42.0f);
    snprintf(welded_body->name, sizeof(welded_body->name), "welded_body");
    welded_body->position = (Position){-280.0f, 20.0f};
    editor_workspace_hitbox_rectangle_set(project, &welded_body->hitboxes[0],
        38.0f, 50.0f);
    snprintf(spring_body->name, sizeof(spring_body->name), "spring_body");
    spring_body->position = (Position){-220.0f, 125.0f};
    editor_workspace_hitbox_rectangle_set(project, &spring_body->hitboxes[0],
        42.0f, 42.0f);
    snprintf(particle_body->name, sizeof(particle_body->name), "particle_body");
    particle_body->position = (Position){100.0f, 20.0f};
    particle_body->particle = true;
    particle_body->standalone_particle = true;
    particle_body->particle_auto_fit = false;
    particle_body->particle_radius = 28.0f;
    particle_body->rotation_locked = true;
    if(!editor_project_particle_hitbox_sync(project, particle_body)) return false;

    chassis_pin = editor_project_anchor_add(project, starter,
        (Position){60.0f, 0.0f}, chassis_body->id);
    if(chassis_pin == NULL) return false;
    chassis_pin_id = chassis_pin->id;
    wheel_pin = editor_project_anchor_add(project, starter,
        (Position){0.0f, 0.0f}, wheel_body->id);
    if(wheel_pin == NULL) return false;
    wheel_pin_id = wheel_pin->id;
    chassis_weld = editor_project_anchor_add(project, starter,
        (Position){-60.0f, 0.0f}, chassis_body->id);
    if(chassis_weld == NULL) return false;
    chassis_weld_id = chassis_weld->id;
    welded_anchor = editor_project_anchor_add(project, starter,
        (Position){0.0f, 0.0f}, welded_body->id);
    if(welded_anchor == NULL) return false;
    welded_anchor_id = welded_anchor->id;
    chassis_spring = editor_project_anchor_add(project, starter,
        (Position){0.0f, 0.0f}, chassis_body->id);
    if(chassis_spring == NULL) return false;
    chassis_spring_id = chassis_spring->id;
    spring_anchor = editor_project_anchor_add(project, starter,
        (Position){0.0f, 0.0f}, spring_body->id);
    if(spring_anchor == NULL) return false;
    spring_anchor_id = spring_anchor->id;
    if(editor_project_anchor_add(project, starter,
            (Position){300.0f, 190.0f}, 0) == NULL) return false;
    chassis_pin = editor_project_anchor_get(starter, chassis_pin_id);
    wheel_pin = editor_project_anchor_get(starter, wheel_pin_id);
    chassis_weld = editor_project_anchor_get(starter, chassis_weld_id);
    welded_anchor = editor_project_anchor_get(starter, welded_anchor_id);
    chassis_spring = editor_project_anchor_get(starter, chassis_spring_id);
    spring_anchor = editor_project_anchor_get(starter, spring_anchor_id);
    snprintf(chassis_pin->name, sizeof(chassis_pin->name), "chassis_pin");
    snprintf(wheel_pin->name, sizeof(wheel_pin->name), "wheel_pin");
    snprintf(chassis_weld->name, sizeof(chassis_weld->name), "chassis_weld");
    snprintf(welded_anchor->name, sizeof(welded_anchor->name), "welded_anchor");
    snprintf(chassis_spring->name, sizeof(chassis_spring->name), "chassis_spring");
    snprintf(spring_anchor->name, sizeof(spring_anchor->name), "spring_anchor");
    snprintf(starter->anchors[starter->anchor_count - 1].name,
        sizeof(starter->anchors[starter->anchor_count - 1].name), "world_anchor");

    joint = editor_project_joint_add(project, starter, EDITOR_JOINT_REVOLUTE);
    if(joint == NULL) return false;
    snprintf(joint->name, sizeof(joint->name), "revolute_joint");
    joint->anchor_a = chassis_pin_id;
    joint->anchor_b = wheel_pin_id;
    joint = editor_project_joint_add(project, starter, EDITOR_JOINT_WELD);
    if(joint == NULL) return false;
    snprintf(joint->name, sizeof(joint->name), "weld_joint");
    joint->anchor_a = chassis_weld_id;
    joint->anchor_b = welded_anchor_id;
    joint = editor_project_joint_add(project, starter, EDITOR_JOINT_SPRING);
    if(joint == NULL) return false;
    snprintf(joint->name, sizeof(joint->name), "spring_joint");
    joint->anchor_a = chassis_spring_id;
    joint->anchor_b = spring_anchor_id;
    joint->rest_length = 105.0f;

    soft_body = editor_project_soft_body_add(project, starter);
    if(soft_body == NULL) return false;
    snprintf(soft_body->name, sizeof(soft_body->name), "soft_body");
    soft_body->position = (Position){220.0f, 80.0f};
    soft_nodes[0] = editor_project_soft_node_add(project, soft_body,
        (Position){-45.0f, -45.0f});
    soft_nodes[1] = editor_project_soft_node_add(project, soft_body,
        (Position){45.0f, -45.0f});
    soft_nodes[2] = editor_project_soft_node_add(project, soft_body,
        (Position){45.0f, 45.0f});
    soft_nodes[3] = editor_project_soft_node_add(project, soft_body,
        (Position){-45.0f, 45.0f});
    for(size_t i = 0; i < 4; i += 1) if(soft_nodes[i] == NULL) return false;
    for(size_t i = 0; i < 4; i += 1)
        if(editor_project_soft_beam_add(project, soft_body, soft_nodes[i]->id,
                soft_nodes[(i + 1) % 4]->id) == NULL) return false;
    if(editor_project_soft_beam_add(project, soft_body,
            soft_nodes[0]->id, soft_nodes[2]->id) == NULL) return false;

    if(editor_project_soft_beam_add(project, soft_body,
            soft_nodes[1]->id, soft_nodes[3]->id) == NULL) return false;
    EditorSoftArea *default_area = editor_soft_area_add(soft_body);
    if(default_area == NULL) return false;
    default_area->outer.node_count = 4;
    for(size_t i = 0; i < 4; i += 1) default_area->outer.nodes[i] = soft_nodes[i]->id;

    sprite = editor_project_sprite_add(project, starter, "standalone_sprite",
        "assets/tutorial_frame_1.png");
    if(sprite == NULL) return false;
    sprite->position = (Position){220.0f, -100.0f};
    sprite->size = (Scale){64.0f, 64.0f};

    animation = editor_project_animated_sprite_add(project, starter);
    if(animation == NULL) return false;
    snprintf(animation->name, sizeof(animation->name), "attached_animation");
    animation->rigid_body = box_id;
    animation->time_per_frame = 0.2;
    animation->playing = true;
    if(!editor_project_animation_frame_add(project, animation, "frame_1",
            "assets/tutorial_frame_1.png", (Scale){0.16f, 0.16f}) ||
            !editor_project_animation_frame_add(project, animation, "frame_2",
                "assets/tutorial_frame_2.png", (Scale){0.16f, 0.16f})) return false;
    animation = editor_project_animated_sprite_add(project, starter);
    if(animation == NULL) return false;
    snprintf(animation->name, sizeof(animation->name), "free_animation");
    animation->editor_position = (Position){340.0f, 80.0f};
    animation->time_per_frame = 0.2;
    animation->playing = true;
    if(!editor_project_animation_frame_add(project, animation, "frame_1",
            "assets/tutorial_frame_1.png", (Scale){0.16f, 0.16f}) ||
            !editor_project_animation_frame_add(project, animation, "frame_2",
                "assets/tutorial_frame_2.png", (Scale){0.16f, 0.16f})) return false;
    if(!editor_workspace_tug_init(project)) return false;
    editor_project_selection_clear(project);
    return true;
}

EditorWorkspaceConfig editor_workspace_config_default_get(void) {
    EditorWorkspaceConfig config = {
        .format_version = EDITOR_WORKSPACE_FORMAT_VERSION
    };

    snprintf(config.name, sizeof(config.name), "Game");
    snprintf(config.source_directory, sizeof(config.source_directory), "src");
    snprintf(config.generated_directory, sizeof(config.generated_directory),
        "src/generated");
    snprintf(config.asset_directory, sizeof(config.asset_directory), "assets");
    snprintf(config.object_directory, sizeof(config.object_directory), "objects");
    snprintf(config.editor_state_file, sizeof(config.editor_state_file),
        "objects/project.rohr.json");
    return config;
}

static bool editor_workspace_manifest_save(const EditorWorkspace *workspace) {
    yyjson_mut_doc *document;
    yyjson_mut_val *root;
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    bool success;

    if(workspace == NULL || !workspace->open ||
            !editor_workspace_path_join(path, sizeof(path), workspace->directory,
                "project.rohr.json")) return false;
    document = yyjson_mut_doc_new(NULL);
    if(document == NULL) return false;
    root = yyjson_mut_obj(document);
    yyjson_mut_doc_set_root(document, root);
    yyjson_mut_obj_add_uint(document, root, "format_version",
        workspace->config.format_version);
    yyjson_mut_obj_add_strcpy(document, root, "name", workspace->config.name);
    yyjson_mut_obj_add_strcpy(document, root, "source_directory",
        workspace->config.source_directory);
    yyjson_mut_obj_add_strcpy(document, root, "generated_directory",
        workspace->config.generated_directory);
    yyjson_mut_obj_add_strcpy(document, root, "asset_directory",
        workspace->config.asset_directory);
    yyjson_mut_obj_add_strcpy(document, root, "object_directory",
        workspace->config.object_directory);
    yyjson_mut_obj_add_strcpy(document, root, "editor_state_file",
        workspace->config.editor_state_file);
    yyjson_mut_obj_add_strcpy(document, root, "engine_directory", workspace->config.engine_directory);
    yyjson_mut_obj_add_strcpy(document, root, "build_directory", workspace->config.build_directory);
    yyjson_mut_obj_add_strcpy(document, root, "application_executable", workspace->config.application_executable);
    yyjson_mut_obj_add_strcpy(document, root, "scene_executable", workspace->config.scene_executable);
    success = yyjson_mut_write_file(path, document, YYJSON_WRITE_PRETTY, NULL, NULL);
    yyjson_mut_doc_free(document);
    return success;
}

static bool editor_workspace_json_string(yyjson_val *root, const char *key,
    char *output, size_t capacity) {
    yyjson_val *value = yyjson_obj_get(root, key);
    size_t length;

    if(!yyjson_is_str(value) || output == NULL || capacity == 0) return false;
    length = yyjson_get_len(value);
    if(length == 0 || length >= capacity) return false;
    memcpy(output, yyjson_get_str(value), length + 1);
    return true;
}

static EditorResult editor_workspace_manifest_load(EditorWorkspace *workspace,
        const char *directory, bool *recognized) {
    yyjson_doc *document;
    yyjson_val *root;
    yyjson_val *version;
    yyjson_read_err read_error = {0};
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    EditorResult result;

    if(recognized != NULL) *recognized = false;
    if(workspace == NULL || directory == NULL || recognized == NULL ||
            !editor_workspace_path_join(path, sizeof(path), directory,
                "project.rohr.json")) return editor_result_error(
                    EDITOR_ERROR_INVALID_ARGUMENT,
                    "Workspace manifest load received an invalid path");
    document = yyjson_read_file(path, 0, NULL, &read_error);
    if(document == NULL) return editor_result_error(
        read_error.code == YYJSON_READ_ERROR_FILE_OPEN ||
                read_error.code == YYJSON_READ_ERROR_FILE_READ ?
            EDITOR_ERROR_FILE_IO : EDITOR_ERROR_JSON_PARSE,
        "Could not parse workspace manifest '%s': %s at byte %zu", path,
        read_error.msg == NULL ? "unknown JSON error" : read_error.msg,
        read_error.pos);
    root = yyjson_doc_get_root(document);
    version = yyjson_obj_get(root, "format_version");
    *recognized = yyjson_is_obj(root) &&
        yyjson_obj_get(root, "editor_state_file") != NULL;
    result = editor_result_error(EDITOR_ERROR_SCHEMA_INVALID,
        "Workspace manifest '%s' does not match the current schema", path);
    if(!yyjson_is_obj(root)) goto done;
    if(!yyjson_is_uint(version)) {
        result = editor_result_error(EDITOR_ERROR_SCHEMA_VERSION,
            "Workspace manifest '%s' is missing integer format_version; expected %u",
            path, EDITOR_WORKSPACE_FORMAT_VERSION);
        goto done;
    }
    if(yyjson_get_uint(version) != EDITOR_WORKSPACE_FORMAT_VERSION) {
        result = editor_result_error(EDITOR_ERROR_SCHEMA_VERSION,
            "Workspace manifest '%s' uses format_version %llu; this editor requires %u",
            path, (unsigned long long)yyjson_get_uint(version),
            EDITOR_WORKSPACE_FORMAT_VERSION);
        goto done;
    }
    workspace->config = editor_workspace_config_default_get();
    workspace->config.format_version = (uint32_t)yyjson_get_uint(version);
#define EDITOR_MANIFEST_STRING_READ(Field) \
    if(!editor_workspace_json_string(root, #Field, workspace->config.Field, \
            sizeof(workspace->config.Field))) { \
        result = editor_result_error(EDITOR_ERROR_SCHEMA_INVALID, \
            "Workspace manifest '%s' has missing, empty, or oversized field '%s'", \
            path, #Field); \
        goto done; \
    }
#define EDITOR_OPTIONAL_PATH(Field) do { \
    yyjson_val *v = yyjson_obj_get(root, #Field); \
    if(v) { const char *text = yyjson_get_str(v); \
        if(!text || strlen(text) >= sizeof(workspace->config.Field)) { \
            result = editor_result_error(EDITOR_ERROR_SCHEMA_INVALID, "Invalid workspace path field: %s", #Field); goto done; } \
        SDL_strlcpy(workspace->config.Field, text, sizeof(workspace->config.Field)); } \
} while(0)
    EDITOR_OPTIONAL_PATH(engine_directory);
    EDITOR_OPTIONAL_PATH(build_directory);
    EDITOR_OPTIONAL_PATH(application_executable);
    EDITOR_OPTIONAL_PATH(scene_executable);
#undef EDITOR_OPTIONAL_PATH
    EDITOR_MANIFEST_STRING_READ(name);
    EDITOR_MANIFEST_STRING_READ(source_directory);
    EDITOR_MANIFEST_STRING_READ(generated_directory);
    EDITOR_MANIFEST_STRING_READ(asset_directory);
    EDITOR_MANIFEST_STRING_READ(object_directory);
    EDITOR_MANIFEST_STRING_READ(editor_state_file);
#undef EDITOR_MANIFEST_STRING_READ
    if(strlen(directory) >= sizeof(workspace->directory)) {
        result = editor_result_error(EDITOR_ERROR_FILE_IO,
            "Workspace directory path is too long: %s", directory);
        goto done;
    }
    snprintf(workspace->directory, sizeof(workspace->directory), "%s", directory);
    workspace->open = true;
    result = editor_result_value(true);
done:
    yyjson_doc_free(document);
    return result;
}

static void editor_workspace_input_controller_variable(char *output, size_t capacity,
        const EditorInputController *controller) {
    char name[ROHR_INPUT_NAME_MAX];
    editor_project_property_name_format(name, sizeof(name), controller->name);
    snprintf(output, capacity, "controller_%s_%u", name, controller->id);
}

static void editor_workspace_input_action_variable(char *output, size_t capacity,
        const EditorInputController *controller, const EditorInputAction *action) {
    char controller_name[ROHR_INPUT_NAME_MAX];
    char action_name[ROHR_INPUT_NAME_MAX];
    editor_project_property_name_format(controller_name, sizeof(controller_name), controller->name);
    editor_project_property_name_format(action_name, sizeof(action_name),
        action->name);
    snprintf(output, capacity, "action_%s_%s_%u", controller_name, action_name,
        action->id);
}

static void editor_workspace_input_binding_write(FILE *source,
        const InputBinding *binding) {
    fprintf(source, "        {.name = ");
    editor_workspace_c_string_write(source, binding->name);
    fprintf(source, ", .source = %s, .input.",
        binding->source == INPUT_BINDING_KEY ? "INPUT_BINDING_KEY" :
        binding->source == INPUT_BINDING_MOUSE_BUTTON ?
            "INPUT_BINDING_MOUSE_BUTTON" :
        binding->source == INPUT_BINDING_MOUSE_MOTION ?
            "INPUT_BINDING_MOUSE_MOTION" : "INPUT_BINDING_MOUSE_WHEEL");
    if(binding->source == INPUT_BINDING_KEY)
        fprintf(source, "key = (SDL_Scancode)%u", (unsigned)binding->input.key);
    else if(binding->source == INPUT_BINDING_MOUSE_BUTTON)
        fprintf(source, "mouse_button = (InputMouseButton)%u",
            (unsigned)binding->input.mouse_button);
    else fprintf(source, "axis_component = %s",
        binding->input.axis_component == INPUT_AXIS_COMPONENT_X ?
            "INPUT_AXIS_COMPONENT_X" :
        binding->input.axis_component == INPUT_AXIS_COMPONENT_Y ?
            "INPUT_AXIS_COMPONENT_Y" : "INPUT_AXIS_COMPONENT_XY");
    fprintf(source,
        ", .modifiers = (SDL_Keymod)%u, .affects_x = %s, .affects_y = %s, "
        ".scale = {%#.9gf, %#.9gf}, "
        ".inverted_x = %s, .inverted_y = %s, "
        ".direction = {%#.9gf, %#.9gf}}",
        (unsigned)binding->modifiers,
        binding->affects_x ? "true" : "false",
        binding->affects_y ? "true" : "false",
        binding->scale.x, binding->scale.y,
        binding->inverted_x ? "true" : "false",
        binding->inverted_y ? "true" : "false",
        binding->direction.x, binding->direction.y);
}

static void editor_workspace_particle_config_name(char *output, size_t capacity,
        const char *object_name, const EditorRigidBody *body) {
    snprintf(output, capacity, "%s_%s_config", object_name, body->name);
}

static void editor_workspace_particle_config_write(FILE *source,
        const char *name, const EditorRigidBody *body) {
    float radius = body->particle_auto_fit ?
        editor_project_particle_auto_radius_get(body) : body->particle_radius;

    fprintf(source,
        "const ParticleDescription %s = {\n"
        "    .position = {%#.9gf, %#.9gf},\n"
        "    .local_origin = {%#.9gf, %#.9gf},\n"
        "    .velocity = {%#.9gf, %#.9gf},\n"
        "    .acceleration = {%#.9gf, %#.9gf},\n"
        "    .radius = %#.9gf,\n"
        "    .rigid_vertices = UINT32_C(%u),\n"
        "    .mass_value = %#.9gf,\n"
        "    .friction = %#.9gf,\n"
        "    .restitution = %#.9gf,\n"
        "    .collision_filter = {.category = UINT64_C(%llu), "
            ".responds_to = UINT64_C(%llu)},\n"
        "    .static_body = %s,\n"
        "    .kinematic_driven = %s,\n"
        "    .gravity_enabled = %s,\n"
        "    .collision_enabled = %s\n"
        "};\n\n",
        name, body->position.x, body->position.y,
        body->particle_origin.x, body->particle_origin.y,
        body->initial_velocity.x, body->initial_velocity.y,
        body->initial_acceleration.x, body->initial_acceleration.y,
        radius, body->particle_rigid_vertices, body->mass_value,
        body->friction, body->restitution,
        (unsigned long long)body->collision_category,
        (unsigned long long)body->responds_to,
        body->static_body ? "true" : "false",
        body->kinematic_driven ? "true" : "false",
        body->gravity_enabled ? "true" : "false",
        body->collision_enabled ? "true" : "false");
}

static void catalog_binding_write(FILE *f, const char *field, const char *name,
        const char *resource, const char *scope) {
    fprintf(f, "    { EntityResult member = rohr_entity_object_member_by_name_get(%s, ", scope);
    editor_workspace_c_string_write(f, name);
    fputs(");\n      if(rohr_error_check(member)) { result = rohr_error_result_error(member.result.error); goto fail; }\n", f);
    if(resource) {
        fprintf(f, "      %sIdResult handle = rohr_entity_object_%s_get(member.result.value);\n"
            "      if(rohr_error_check(handle)) { result = rohr_error_result_error(handle.result.error); goto fail; }\n"
            "      object->%s = handle.result.value; }\n", !strcmp(resource,"camera") ? "Camera" : "JointAnchor", resource, field);
    } else fprintf(f, "      object->%s = member.result.value; }\n", field);
}
static void catalog_instance_binding_write(FILE *f, const EditorObject *o,
        const char *variable) {
    fprintf(f, "EngineResult %s_instance_bind(%s *output, Entity instance) {\n"
        "    if(output == NULL) return rohr_error_result_error(ERROR_MEMORY_POOL_NULL_POINTER);\n"
        "    if(!rohr_entity_alive_check(instance)) return rohr_error_result_error(ERROR_ENGINE_ENTITY_NOT_FOUND);\n"
        "    %s bound = {.source_instance = instance};\n"
        "    %s *object = &bound;\n"
        "    EngineResult result = rohr_error_result_value(true);\n",
        variable, o->name, o->name, o->name);
    char field[160], scope[160];
    for(size_t i=0;i<o->rigid_body_count;++i)
        catalog_binding_write(f,o->rigid_bodies[i].name,o->rigid_bodies[i].name,NULL,"object->source_instance");
    for(size_t i=0;i<o->soft_body_count;++i) {
        const EditorSoftBody *b=&o->soft_body_items[i];
        catalog_binding_write(f,b->name,b->name,NULL,"object->source_instance");
        snprintf(scope,sizeof(scope),"object->%s",b->name);
        for(size_t n=0;n<b->node_count;++n) catalog_binding_write(f,b->nodes[n].name,b->nodes[n].name,NULL,scope);
        for(size_t n=0;n<b->beam_count;++n) catalog_binding_write(f,b->beams[n].name,b->beams[n].name,NULL,scope);
        for(size_t n=0;n<b->area_count;++n) {
            snprintf(field,sizeof(field),"%s_areas[%zu]",b->name,n);
            catalog_binding_write(f,field,b->areas[n].name,NULL,scope);
        }
    }
    for(size_t i=0;i<o->anchor_count;++i) {
        const EditorAnchor *a=&o->anchors[i];
        snprintf(field,sizeof(field),"anchor_%s",a->name);
        catalog_binding_write(f,field,a->name,"anchor","object->source_instance");
        if(a->attachment_kind==EDITOR_ANCHOR_ATTACHMENT_NONE || !a->position_follows_body) {
            snprintf(field,sizeof(field),"anchor_%s_owner",a->name);
            catalog_binding_write(f,field,a->name,NULL,"object->source_instance");
        }
    }
    for(size_t i=0;i<o->joint_count;++i) {
        snprintf(field,sizeof(field),"joint_%s",o->joint_items[i].name);
        catalog_binding_write(f,field,o->joint_items[i].name,NULL,"object->source_instance");
    }
    for(size_t i=0;i<o->sprite_count;++i) {
        snprintf(field,sizeof(field),"sprite_%s",o->sprites[i].name);
        catalog_binding_write(f,field,o->sprites[i].name,NULL,"object->source_instance");
    }
    for(size_t i=0;i<o->animated_sprite_count;++i) {
        const EditorAnimatedSprite *a=&o->animated_sprite_items[i];
        if(editor_workspace_body_get(o,a->rigid_body)) continue;
        snprintf(field,sizeof(field),"animation_%s",a->name);
        catalog_binding_write(f,field,a->name,NULL,"object->source_instance");
    }
    for(size_t i=0;i<o->camera_count;++i) {
        snprintf(field,sizeof(field),"camera_%s",o->cameras[i].name);
        catalog_binding_write(f,field,o->cameras[i].name,"camera","object->source_instance");
    }
    for(size_t i=0;i<o->force_count;++i) if(o->force_items[i].description.mode==OBJECT_SOURCE_PERSISTENT) {
        snprintf(field,sizeof(field),"force_%s",o->force_items[i].name);
        catalog_binding_write(f,field,o->force_items[i].name,NULL,"object->source_instance");
    }
    for(size_t i=0;i<o->torque_count;++i) if(o->torque_items[i].description.mode==OBJECT_SOURCE_PERSISTENT) {
        snprintf(field,sizeof(field),"torque_%s",o->torque_items[i].name);
        catalog_binding_write(f,field,o->torque_items[i].name,NULL,"object->source_instance");
    }
    fprintf(f, "    (void)object;\n    if(rohr_error_check(result)) goto fail;\n    *output = bound;\n    return result;\nfail:\n    return result;\n}\n\n");
}

static void catalog_constructor_write(FILE *f, const EditorObject *o, const char *variable) {
    fprintf(f, "EngineResult %s_create(%s *object, Position position) {\n"
        "    if(!object) return rohr_error_result_error(ERROR_MEMORY_POOL_NULL_POINTER);\n"
        "    *object = (%s){0};\n"
        "    const ObjectDescription *description = rohr_object_get(objects.%s);\n"
        "    EntityResult added = rohr_object_create_with_options(description,\n"
        "        ((ObjectCreateOptions){.flags=ROHR_ENTITY_OBJECT_ALL,\n"
        "            .offset={position.x - description->position.x, position.y - description->position.y}}));\n"
        "    if(rohr_error_check(added)) return rohr_error_result_error(added.result.error);\n"
        "    object->source_instance = added.result.value;\n"
        "    EngineResult result = rohr_error_result_value(true);\n", variable,o->name,o->name,variable);
    fprintf(f, "    result = %s_instance_bind(object, added.result.value);\n"
        "    if(rohr_error_check(result)) goto fail;\n", variable);
    fprintf(f,"    return result;\nfail:\n    %s_destroy(object);\n    return result;\n}\n\n"
        "void %s_draw(const %s *object) {\n    if(object && object->source_instance != ENTITY_INVALID) (void)rohr_entity_object_draw(object->source_instance);\n}\n\n"
        "void %s_destroy(%s *object) {\n    if(!object) return;\n"
        "    if(rohr_entity_alive_check(object->source_instance)) (void)rohr_entity_delete(object->source_instance);\n"
        "    *object = (%s){0};\n}\n\n",variable,variable,o->name,variable,o->name,o->name);
}

static void editor_workspace_audio_write(FILE *header, FILE *source,
        const EditorProject *project) {
    fprintf(header, "/* Recipes only; the game creates, controls, and destroys players. */\n"
        "typedef struct ProjectAudio {\n");
    for(int music = 0; music < 2; ++music) {
        size_t count = 0;
        fprintf(header, "    struct {\n");
        for(size_t i = 0; i < project->audio_definition_count; ++i) {
            const EditorAudioDefinition *a = &project->audio_definitions[i];
            if(a->music != (bool)music) continue;
            fprintf(header, "        %sDescription %s;\n", music ? "Music" : "Sound", a->name);
            ++count;
        }
        if(!count) fprintf(header, "        unsigned char unused;\n");
        fprintf(header, "    } %s;\n", music ? "music" : "sounds");
    }
    fprintf(header, "} ProjectAudio;\nextern const ProjectAudio project_audio;\n\n");
    fprintf(source, "const ProjectAudio project_audio = {\n");
    for(int music = 0; music < 2; ++music) {
        size_t count = 0;
        fprintf(source, "    .%s = {\n", music ? "music" : "sounds");
        for(size_t i = 0; i < project->audio_definition_count; ++i) {
            const EditorAudioDefinition *a = &project->audio_definitions[i];
            if(a->music != (bool)music) continue;
            fprintf(source, "        .%s = {.path = ", a->name);
            editor_workspace_c_string_write(source, a->path);
            fprintf(source, ", .volume = %.9ef, .loop = %s, .playback_rate = %.9ef",
                a->volume, a->loop ? "true" : "false", a->playback_rate);
            if(!music) fprintf(source, ", .pan = %.9ef", a->pan);
            fprintf(source, "},\n");
            ++count;
        }
        if(!count) fprintf(source, "        .unused = 0,\n");
        fprintf(source, "    },\n");
    }
    fprintf(source, "};\n\n");
}

static bool editor_workspace_generated_objects_write(const EditorWorkspace *workspace,
    const EditorProject *project, bool scene) {
    char header_path[EDITOR_WORKSPACE_PATH_MAX * 2];
    char source_path[EDITOR_WORKSPACE_PATH_MAX * 2];
    FILE *header;
    FILE *source;
    bool generated_has_input_actions = false;

    if(workspace == NULL || project == NULL ||
            !editor_workspace_file_path(workspace, header_path, sizeof(header_path), scene ? "src/generated/scene/project_objects.h" : "src/generated/project_objects.h") ||
            !editor_workspace_file_path(workspace, source_path, sizeof(source_path), scene ? "src/generated/scene/project_objects.c" : "src/generated/project_objects.c")) return false;
    for(size_t i = 0; i < project->input_controller_count; i += 1)
        if(project->input_controllers[i].action_count > 0)
            generated_has_input_actions = true;
    header = fopen(header_path, "wb");
    if(header == NULL) return false;
    source = fopen(source_path, "wb");
    if(source == NULL) {
        fclose(header);
        return false;
    }
    fprintf(header,
        "/*\n"
        " * Generated by Rohr Engine.\n"
        " * This generated file is not covered by Rohr Engine's LGPL-3.0-only license.\n"
        " * The project's developer may use, modify, and distribute it without restriction.\n"
        " */\n\n"
        "#ifndef ROHR_GENERATED_PROJECT_OBJECTS_H\n"
        "#define ROHR_GENERATED_PROJECT_OBJECTS_H\n\n"
        "#include \"rohr.h\"\n\n");
    fprintf(source,
        "/*\n"
        " * Generated by Rohr Engine.\n"
        " * This generated file is not covered by Rohr Engine's LGPL-3.0-only license.\n"
        " * The project's developer may use, modify, and distribute it without restriction.\n"
        " */\n\n"
        "#include \"project_objects.h\"\n\n"
        "/* Legacy instance wrappers use the shared descriptor creators. */\n");
    for(size_t object_index = 0; object_index < project->object_count; object_index += 1) {
        const EditorObject *object = &project->objects[object_index];
        char function_name[EDITOR_OBJECT_NAME_MAX];

        editor_project_property_name_format(function_name, sizeof(function_name),
            object->name);
        for(size_t body_index = 0; body_index < object->rigid_body_count;
                body_index += 1) {
            const EditorRigidBody *body = &object->rigid_bodies[body_index];
            char config_name[EDITOR_OBJECT_NAME_MAX * 2 + 32];
            if(!body->particle || !body->standalone_particle) continue;
            editor_workspace_particle_config_name(config_name,
                sizeof(config_name), function_name, body);
            fprintf(header, "extern const ParticleDescription %s;\n", config_name);
            editor_workspace_particle_config_write(source, config_name, body);
        }
        fprintf(header, "\n");
        fprintf(header, "typedef struct %s {\n", object->name);
        fprintf(header, "    Entity source_instance;\n");
        if(object->force_count || object->torque_count) {
            for(size_t s=0;s<object->force_count;++s)
                if(object->force_items[s].description.mode==OBJECT_SOURCE_PERSISTENT)
                    fprintf(header,"    Entity force_%s;\n",object->force_items[s].name);
            for(size_t s=0;s<object->torque_count;++s)
                if(object->torque_items[s].description.mode==OBJECT_SOURCE_PERSISTENT)
                    fprintf(header,"    Entity torque_%s;\n",object->torque_items[s].name);
        }
        for(size_t body_index = 0; body_index < object->rigid_body_count; body_index += 1) {
            fprintf(header, "    Entity %s;\n", object->rigid_bodies[body_index].name);
        }
        for(size_t anchor_index = 0; anchor_index < object->anchor_count; anchor_index += 1) {
            const EditorAnchor *anchor = &object->anchors[anchor_index];
            fprintf(header, "    JointAnchorId anchor_%s;\n", anchor->name);
            if(anchor->attachment_kind == EDITOR_ANCHOR_ATTACHMENT_NONE ||
                    !anchor->position_follows_body)
                fprintf(header, "    Entity anchor_%s_owner;\n", anchor->name);
        }
        for(size_t joint_index = 0; joint_index < object->joint_count; joint_index += 1)
            fprintf(header, "    Entity joint_%s;\n", object->joint_items[joint_index].name);
        for(size_t soft_body_index = 0; soft_body_index < object->soft_body_count;
                soft_body_index += 1) {
            const EditorSoftBody *body = &object->soft_body_items[soft_body_index];
            fprintf(header, "    Entity %s;\n", body->name);
            if(body->area_count > 0)
                fprintf(header, "    Entity %s_areas[%zu];\n", body->name, body->area_count);
            for(size_t node_index = 0; node_index < body->node_count; node_index += 1)
                fprintf(header, "    Entity %s;\n", body->nodes[node_index].name);
            for(size_t beam_index = 0; beam_index < body->beam_count; beam_index += 1)
                fprintf(header, "    Entity %s;\n", body->beams[beam_index].name);
        }
        for(size_t sprite_index = 0; sprite_index < object->sprite_count;
                sprite_index += 1)
            fprintf(header, "    Entity sprite_%s;\n",
                object->sprites[sprite_index].name);
        for(size_t sprite_index = 0; sprite_index < object->animated_sprite_count;
                sprite_index += 1)
            if(editor_workspace_body_get(object,
                    object->animated_sprite_items[sprite_index].rigid_body) == NULL)
                fprintf(header, "    Entity animation_%s;\n",
                    object->animated_sprite_items[sprite_index].name);
        for(size_t camera_index = 0; camera_index < object->camera_count;
                camera_index += 1)
            fprintf(header, "    CameraId camera_%s;\n",
                object->cameras[camera_index].name);
        fprintf(header,
            "} %s;\n\n"
            "EngineResult %s_create(%s *object, Position position);\n"
            "void %s_draw(const %s *object);\n"
            "void %s_destroy(%s *object);\n\n",
            object->name, function_name, object->name, function_name, object->name,
            function_name, object->name);
        fprintf(header, "/* Bind a complete owned instance; failure leaves output unchanged.\n"
            " * On success, %s_destroy owns its destruction. Do not bind twice. */\n"
            "EngineResult %s_instance_bind(%s *output, Entity instance);\n\n",
            function_name, function_name, object->name);
        catalog_instance_binding_write(source, object, function_name);
        catalog_constructor_write(source,object,function_name);
    }
    fprintf(header, "typedef struct ProjectControllers {\n");
    for(size_t controller_index = 0;
            controller_index < project->input_controller_count;
            controller_index += 1) {
        const EditorInputController *controller =
            &project->input_controllers[controller_index];
        char controller_variable[ROHR_INPUT_NAME_MAX * 2];
        editor_workspace_input_controller_variable(controller_variable,
            sizeof(controller_variable), controller);
        fprintf(header, "    InputControllerId %s;\n", controller_variable);
        for(size_t action_index = 0; action_index < controller->action_count;
                action_index += 1) {
            char action_variable[ROHR_INPUT_NAME_MAX * 3];
            editor_workspace_input_action_variable(action_variable,
                sizeof(action_variable), controller,
                &controller->actions[action_index]);
            fprintf(header, "    InputActionId %s;\n", action_variable);
        }
    }
    fprintf(header,
        "    bool created;\n"
        "} ProjectControllers;\n\n"
        "EngineResult project_controllers_create(ProjectControllers *controllers);\n"
        "void project_controllers_destroy(ProjectControllers *controllers);\n\n");
    fprintf(source,
        "EngineResult project_controllers_create(ProjectControllers *controllers) {\n");
    if(project->input_controller_count > 0)
        fprintf(source, "    InputControllerIdResult controller_result;\n");
    if(generated_has_input_actions)
        fprintf(source, "    InputActionIdResult action_result;\n");
    if(project->input_controller_count > 0)
        fprintf(source, "    EngineResult result;\n");
    fprintf(source,
        "    if(controllers == NULL) return rohr_error_result_error("
            "ERROR_MEMORY_POOL_NULL_POINTER);\n"
        "    *controllers = (ProjectControllers){0};\n");
    for(size_t controller_index = 0;
            controller_index < project->input_controller_count;
            controller_index += 1) {
        const EditorInputController *controller =
            &project->input_controllers[controller_index];
        char controller_variable[ROHR_INPUT_NAME_MAX * 2];
        editor_workspace_input_controller_variable(controller_variable,
            sizeof(controller_variable), controller);
        fprintf(source, "    controller_result = rohr_input_controller_create("
            "(InputControllerDescription){.name = ");
        editor_workspace_c_string_write(source, controller->name);
        fprintf(source,
            ", .enabled = %s});\n"
            "    if(rohr_error_check(controller_result)) { result = "
                "rohr_error_result_error(controller_result.result.error); "
                "goto fail; }\n"
            "    controllers->%s = controller_result.result.value;\n",
            controller->enabled ? "true" : "false", controller_variable);
        for(size_t action_index = 0; action_index < controller->action_count;
                action_index += 1) {
            const EditorInputAction *action = &controller->actions[action_index];
            char action_variable[ROHR_INPUT_NAME_MAX * 3];
            editor_workspace_input_action_variable(action_variable,
                sizeof(action_variable), controller, action);
            fprintf(source,
                "    action_result = rohr_input_action_create((InputActionDescription){\n"
                "        .controller = controllers->%s, .name = ", controller_variable);
            editor_workspace_c_string_write(source, action->name);
            fprintf(source, ", .type = %s,\n",
                action->type == INPUT_ACTION_BUTTON ? "INPUT_ACTION_BUTTON" :
                action->type == INPUT_ACTION_AXIS_1D ? "INPUT_ACTION_AXIS_1D" :
                    "INPUT_ACTION_AXIS_2D");
            if(action->type == INPUT_ACTION_BUTTON) fprintf(source,
                "        .button_mode = %s, .button_initial_state = %s,\n",
                action->button_mode == INPUT_BUTTON_PERSISTENT ?
                    "INPUT_BUTTON_PERSISTENT" : "INPUT_BUTTON_MOMENTARY",
                action->button_initial_state ? "true" : "false");
            fprintf(source, "        .bindings_default = {.count = %zu", action->binding_count);
            if(action->binding_count > 0) {
                fprintf(source, ", .values = {\n");
                for(size_t binding_index = 0;
                        binding_index < action->binding_count;
                        binding_index += 1) {
                    editor_workspace_input_binding_write(source,
                        &action->bindings[binding_index]);
                    fprintf(source, "%s\n",
                        binding_index + 1 == action->binding_count ? "" : ",");
                }
                fprintf(source, "        }");
            }
            fprintf(source,
                "}\n    });\n"
                "    if(rohr_error_check(action_result)) { result = "
                    "rohr_error_result_error(action_result.result.error); goto fail; }\n"
                "    controllers->%s = action_result.result.value;\n", action_variable);
        }
    }
    fprintf(source,
        "    controllers->created = true;\n"
        "    return rohr_error_result_value(true);\n");
    if(project->input_controller_count > 0) fprintf(source,
        "fail:\n"
        "    project_controllers_destroy(controllers);\n"
        "    return result;\n");
    fprintf(source,
        "}\n\n"
        "void project_controllers_destroy(ProjectControllers *controllers) {\n"
        "    if(controllers == NULL) return;\n");
    for(size_t controller_index = project->input_controller_count;
            controller_index > 0; controller_index -= 1) {
        const EditorInputController *controller =
            &project->input_controllers[controller_index - 1];
        char controller_variable[ROHR_INPUT_NAME_MAX * 2];
        editor_workspace_input_controller_variable(controller_variable,
            sizeof(controller_variable), controller);
        fprintf(source,
            "    if(controllers->%s != INPUT_CONTROLLER_INVALID) "
                "(void)rohr_input_controller_destroy(controllers->%s);\n",
            controller_variable, controller_variable);
    }
    fprintf(source,
        "    *controllers = (ProjectControllers){0};\n}\n\n");

    fprintf(header, "typedef struct ProjectObjects {\n");
    for(size_t object_index = 0; object_index < project->object_count; object_index += 1) {
        char variable[EDITOR_OBJECT_NAME_MAX];
        editor_project_property_name_format(variable, sizeof(variable),
            project->objects[object_index].name);
        fprintf(header, "    %s %s;\n", project->objects[object_index].name, variable);
    }
    fprintf(header,
        "    ProjectControllers controllers;\n"
        "    bool created;\n"
        "} ProjectObjects;\n\n"
        "EngineResult project_objects_create_all(ProjectObjects *objects);\n"
        "void project_objects_draw_all(const ProjectObjects *objects);\n"
        "void project_objects_destroy_all(ProjectObjects *objects);\n\n");
    editor_workspace_audio_write(header, source, project);
    fprintf(header, "EngineResult project_settings_apply(void);\n\n");
    fprintf(source, "EngineResult project_settings_apply(void) {\n    EngineResult result;\n");
    fprintf(source,
        "    result = rohr_engine_time_per_tick_set(%.17g);\n"
        "    if(rohr_error_check(result)) goto fail;\n"
        "    result = rohr_physics_substeps_set(%u);\n"
        "    if(rohr_error_check(result)) goto fail;\n"
        "    result = rohr_physics_gravity_set((Acceleration){%.9ff, %.9ff});\n"
        "    if(rohr_error_check(result)) goto fail;\n"
        "    result = rohr_physics_solver_iterations_set(%u);\n"
        "    if(rohr_error_check(result)) goto fail;\n",
        project->engine_time_per_tick, project->physics_substeps,
        project->physics_gravity.x, project->physics_gravity.y,
        project->physics_solver_iterations);
    if(project->application_window_enabled) {
        const WindowPresentationDescription *w = &project->application_window;
        fprintf(source,
            "    result = rohr_graphics_window_presentation_description_set((WindowPresentationDescription){"
            ".mode = %d, .window_width = %d, .window_height = %d, "
            ".logical_width = %d, .logical_height = %d, .aspect_ratio_auto = %s});\n"
            "    if(rohr_error_check(result)) goto fail;\n",
            w->mode, w->window_width, w->window_height, w->logical_width,
            w->logical_height, w->aspect_ratio_auto ? "true" : "false");
    }
    if(project->physics_timestep_override)
        fprintf(source,
            "    result = rohr_physics_dt_per_tick_set(%.17g);\n"
            "    if(rohr_error_check(result)) goto fail;\n",
            project->physics_dt_per_tick);
    else
        fprintf(source, "    rohr_physics_engine_time_per_tick_use();\n");
    fprintf(source, "    return rohr_error_result_value(true);\nfail:\n    return result;\n}\n\n");
    fprintf(source,
        "EngineResult project_objects_create_all(ProjectObjects *instances) {\n"
        "    EngineResult result;\n"
        "    if(instances == NULL) return rohr_error_result_error("
            "ERROR_MEMORY_POOL_NULL_POINTER);\n"
        "    *instances = (ProjectObjects){0};\n"
        "    result = project_controllers_create(&instances->controllers);\n"
        "    if(rohr_error_check(result)) goto fail;\n");
    fprintf(source, "    result = project_settings_apply();\n"
        "    if(rohr_error_check(result)) goto fail;\n");
    for(size_t object_index = 0; object_index < project->object_count; object_index += 1) {
        const EditorObject *object = &project->objects[object_index];
        char variable[EDITOR_OBJECT_NAME_MAX];
        editor_project_property_name_format(variable, sizeof(variable), object->name);
        fprintf(source,
            "    { EntityResult added = rohr_object_create(&objects.%s);\n"
            "      if(rohr_error_check(added)) { result = rohr_error_result_error(added.result.error); goto fail; }\n"
            "      instances->%s.source_instance = added.result.value;\n"
            "      result = %s_instance_bind(&instances->%s, added.result.value);\n"
            "      if(rohr_error_check(result)) goto fail; }\n",
            variable, variable, variable, variable);
    }
    fprintf(source,
        "    instances->created = true;\n"
        "    return rohr_error_result_value(true);\n"
        "fail:\n"
        "    project_objects_destroy_all(instances);\n"
        "    return result;\n"
        "}\n\n"
        "void project_objects_draw_all(const ProjectObjects *objects) {\n"
        "    if(objects == NULL) return;\n");
    for(size_t object_index = 0; object_index < project->object_count; object_index += 1) {
        char variable[EDITOR_OBJECT_NAME_MAX];
        editor_project_property_name_format(variable, sizeof(variable),
            project->objects[object_index].name);
        fprintf(source, "    %s_draw(&objects->%s);\n", variable, variable);
    }
    fprintf(source,
        "}\n\n"
        "void project_objects_destroy_all(ProjectObjects *objects) {\n"
        "    if(objects == NULL) return;\n");
    for(size_t object_index = project->object_count; object_index > 0; object_index -= 1) {
        char variable[EDITOR_OBJECT_NAME_MAX];
        editor_project_property_name_format(variable, sizeof(variable),
            project->objects[object_index - 1].name);
        fprintf(source, "    %s_destroy(&objects->%s);\n", variable, variable);
    }
    fprintf(source,
        "    project_controllers_destroy(&objects->controllers);\n"
        "    *objects = (ProjectObjects){0};\n}\n\n");
    bool catalog_written = editor_object_catalog_write(header, source, project);
    char project_header_path[EDITOR_WORKSPACE_PATH_MAX * 2];
    FILE *project_header = NULL;
    if(editor_workspace_file_path(workspace, project_header_path, sizeof(project_header_path),
            scene ? "src/generated/scene/project.h" : "src/generated/project.h"))
        project_header = fopen(project_header_path, "wb");
    if(project_header) {
        catalog_written = editor_project_catalog_write(project_header, source, project) && catalog_written;
        catalog_written = fclose(project_header) == 0 && catalog_written;
    } else catalog_written = false;
    fprintf(header, "#endif\n");
    {
        bool header_closed = fclose(header) == 0;
        bool source_closed = fclose(source) == 0;
        return catalog_written && header_closed && source_closed;
    }
}

static bool editor_workspace_viewport_modules_remove(const EditorWorkspace *workspace) {
    const char *files[] = {"src/generated/project_viewports.c", "src/generated/project_viewports.h",
        "src/generated/scene/project_viewports.c", "src/generated/scene/project_viewports.h"};
    for(size_t i = 0; i < SDL_arraysize(files); ++i) {
        char path[EDITOR_WORKSPACE_PATH_MAX * 2]; SDL_PathInfo info;
        if(!editor_workspace_file_path(workspace, path, sizeof(path), files[i])) return false;
        if(SDL_GetPathInfo(path, &info) && (info.type != SDL_PATHTYPE_FILE || !SDL_RemovePath(path))) return false;
    }
    return true;
}

static const char editor_workspace_main_source[] =
    "/*\n"
    " * Generated by Rohr Engine.\n"
    " * This generated file is not covered by Rohr Engine's LGPL-3.0-only license.\n"
    " * The project's developer may use, modify, and distribute it without restriction.\n"
    " */\n"
    "\n"
    "#include \"project.h\"\n"
    "\n"
    "#include <stdio.h>\n"
    "\n"
    "static bool ok(EngineResult result) {\n"
    "    if(!rohr_error_check(result)) return true;\n"
    "    fprintf(stderr, \"error %d: %s\\n\", (int)result.result.error,\n"
    "        rohr_error_message_get(result));\n"
    "    return false;\n"
    "}\n"
    "\n"
    "/* Bind each authored camera object to this scene's chosen runtime instance. */\n"
    "static EngineResult viewport_create(const ViewportCreateDescription *recipe,\n"
    "        Entity root, ViewportId *output) {\n"
    "    ViewportObjectBinding bindings[MAX_VIEWPORT_ITEMS];\n"
    "    size_t count = 0;\n"
    "    for(size_t i = 0; i < recipe->item_count; ++i) {\n"
    "        const ObjectDescription *object = recipe->items[i].camera_object;\n"
    "        if(!object) continue;\n"
    "        bool found = false;\n"
    "        for(size_t j = 0; j < count; ++j) if(bindings[j].description == object) found = true;\n"
    "        if(found) continue;\n"
    "        if(count == MAX_VIEWPORT_ITEMS) return rohr_error_result_error(ERROR_MEMORY_POOL_FULL);\n"
    "        EntityResult instance = rohr_object_member_get(root, object->name);\n"
    "        if(rohr_error_check(instance)) return rohr_error_result_error(instance.result.error);\n"
    "        bindings[count++] = (ViewportObjectBinding){object, instance.result.value};\n"
    "    }\n"
    "    ViewportCreateDescription description = {.template = recipe,\n"
    "        .objects = bindings, .object_count = count};\n"
    "    ViewportIdResult created = rohr_viewport_create(&description);\n"
    "    if(rohr_error_check(created)) return rohr_error_result_error(created.result.error);\n"
    "    *output = created.result.value;\n"
    "    return rohr_error_result_value(true);\n"
    "}\n"
    "\n"
    "int main(void) {\n"
    "    if(!ok(rohr_directory_working_set(rohr_directory_base_get()))) return 1;\n"
    "    Entity objects = ENTITY_INVALID;\n"
    "    ProjectControllers controllers = {0};\n"
    "    ViewportId viewports[MAX_VIEWPORTS] = {0};\n"
    "    if(!ok(rohr_engine_start()) || !ok(rohr_graphics_start())) goto fail;\n"
    "    if(!ok(project_settings_apply()) || !ok(project_controllers_create(&controllers))) goto fail;\n"
    "    EntityResult created = rohr_object_create(&project);\n"
    "    if(rohr_error_check(created)) goto fail;\n"
    "    objects = created.result.value;\n"
    "    if(project.viewport_count > MAX_VIEWPORTS) goto fail;\n"
    "    for(size_t i = 0; i < project.viewport_count; ++i)\n"
    "        if(!ok(viewport_create(project.viewports[i], objects, &viewports[i]))) goto fail;\n"
    "    while(true) {\n"
    "        SDL_Event event;\n"
    "        rohr_input_frame_begin();\n"
    "        while((event = rohr_engine_event_poll()).type != 0) {\n"
    "            if(event.type == SDL_EVENT_QUIT) goto done;\n"
    "        }\n"
    "        if(rohr_input_key_pressed_check(SDL_SCANCODE_ESCAPE)) break;\n"
    "        if(rohr_engine_update().header.status == ROHR_REPORT_FAILED) goto fail;\n"
    "        if(rohr_physics_update(rohr_engine_tick_delta_get()).header.status == ROHR_REPORT_FAILED) goto fail;\n"
    "        rohr_graphics_sprite_frames_update(rohr_engine_tick_get(), rohr_engine_time_get());\n"
    "        if(rohr_graphics_viewports_show(viewports, project.viewport_count).header.status == ROHR_REPORT_FAILED) goto fail;\n"
    "    }\n"
    "done:\n"
    "    for(size_t i = 0; i < MAX_VIEWPORTS; ++i)\n"
    "        if(viewports[i]) (void)rohr_viewport_destroy(viewports[i]);\n"
    "    if(rohr_entity_alive_check(objects)) (void)rohr_entity_delete(objects);\n"
    "    project_controllers_destroy(&controllers);\n"
    "    rohr_graphics_stop();\n"
    "    rohr_engine_stop();\n"
    "    return 0;\n"
    "fail:\n"
    "    fprintf(stderr, \"Game initialization failed\\n\");\n"
    "    for(size_t i = 0; i < MAX_VIEWPORTS; ++i)\n"
    "        if(viewports[i]) (void)rohr_viewport_destroy(viewports[i]);\n"
    "    if(rohr_entity_alive_check(objects)) (void)rohr_entity_delete(objects);\n"
    "    project_controllers_destroy(&controllers);\n"
    "    rohr_graphics_stop();\n"
    "    rohr_engine_stop();\n"
    "    return 1;\n"
    "}\n"
    ;

static bool editor_workspace_main_write(const EditorWorkspace *workspace,
    const EditorProject *project, bool tutorial) {
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    FILE *file;

    if(workspace == NULL || project == NULL ||
            !editor_workspace_file_path(workspace, path, sizeof(path), "src/main.c")) return false;
    file = fopen(path, "wb");
    if(file == NULL) return false;
    bool written = fputs(tutorial ? editor_starter_main_source : editor_workspace_main_source, file) != EOF;
    bool closed = fclose(file) == 0;
    return written && closed;
}

/* Upgrade only a byte-for-byte match of the preceding generated template. */
static bool editor_workspace_tick_main_check(const char *contents) {
    static const char old_tick[] =
        "        if(rohr_physics_update(rohr_system_tick_update()).header.status == ROHR_REPORT_FAILED) goto fail;\n";
    static const char new_tick[] =
        "        if(rohr_engine_update().header.status == ROHR_REPORT_FAILED) goto fail;\n"
        "        if(rohr_physics_update(rohr_engine_tick_delta_get()).header.status == ROHR_REPORT_FAILED) goto fail;\n";
    const char *start = strstr(editor_workspace_main_legacy_viewport_source, new_tick);
    if(contents == NULL || start == NULL) return false;
    size_t prefix = (size_t)(start - editor_workspace_main_legacy_viewport_source);
    size_t old_length = strlen(old_tick);
    return strlen(contents) >= prefix + old_length &&
        strncmp(contents, editor_workspace_main_legacy_viewport_source, prefix) == 0 &&
        strncmp(contents + prefix, old_tick, old_length) == 0 &&
        strcmp(contents + prefix + old_length, start + strlen(new_tick)) == 0;
}

static bool editor_workspace_cmake_legacy_format(const EditorWorkspace *workspace,
        char *cmake, size_t capacity) {
    int cmake_length;

    cmake_length = snprintf(cmake, capacity,
        "cmake_minimum_required(VERSION 3.20)\n"
        "project(%s LANGUAGES C)\n\n"
        "set(CMAKE_C_STANDARD 11)\n"
        "set(CMAKE_C_STANDARD_REQUIRED ON)\n"
        "if(DEFINED ROHR_ENGINE_SOURCE_ROOT AND "
            "EXISTS \"${ROHR_ENGINE_SOURCE_ROOT}/CMakeLists.txt\")\n"
        "    set(ROHR_BUILD_EXAMPLES OFF CACHE BOOL \"\" FORCE)\n"
        "    set(ROHR_BUILD_TESTS OFF CACHE BOOL \"\" FORCE)\n"
        "    set(ROHR_BUILD_EDITOR OFF CACHE BOOL \"\" FORCE)\n"
        "    set(ROHR_BUILD_CLI OFF CACHE BOOL \"\" FORCE)\n"
        "    add_subdirectory(\"${ROHR_ENGINE_SOURCE_ROOT}\" rohr-engine)\n"
        "    set(ROHR_ENGINE_TARGET rohr_engine)\n"
        "else()\n"
        "    if(DEFINED ROHR_ENGINE_SOURCE_ROOT)\n"
        "        list(PREPEND CMAKE_PREFIX_PATH \"${ROHR_ENGINE_SOURCE_ROOT}\")\n"
        "    endif()\n"
        "    find_package(Rohr CONFIG REQUIRED)\n"
        "    set(ROHR_ENGINE_TARGET Rohr::Engine)\n"
        "endif()\n\n"
        "file(GLOB ROHR_GENERATED_SOURCES CONFIGURE_DEPENDS src/generated/*.c)\n"
        "add_executable(${PROJECT_NAME} src/main.c ${ROHR_GENERATED_SOURCES})\n"
        "target_include_directories(${PROJECT_NAME} PRIVATE src/generated)\n"
        "target_link_libraries(${PROJECT_NAME} PRIVATE ${ROHR_ENGINE_TARGET})\n"
        "add_custom_target(${PROJECT_NAME}_assets ALL\n"
        "    COMMAND ${CMAKE_COMMAND} -E copy_directory\n"
        "        \"${CMAKE_SOURCE_DIR}/%s\"\n"
        "        \"$<TARGET_FILE_DIR:${PROJECT_NAME}>/%s\"\n"
        "    VERBATIM)\n"
        "add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_assets)\n",
        workspace->config.name,
        workspace->config.asset_directory,
        workspace->config.asset_directory);
    return cmake_length >= 0 && (size_t)cmake_length < capacity;
}

static bool editor_workspace_cmake_format(const EditorWorkspace *workspace, char *output, size_t capacity) {
    int n = snprintf(output, capacity,
        "# Rohr managed project scaffold v2\n"
        "cmake_minimum_required(VERSION 3.20)\nproject(%s LANGUAGES C)\n"
        "set(CMAKE_C_STANDARD 11)\nset(CMAKE_C_STANDARD_REQUIRED ON)\n"
        "include(.rohr/project_paths.cmake)\n"
        "if(DEFINED ROHR_ENGINE_SOURCE_ROOT AND EXISTS \"${ROHR_ENGINE_SOURCE_ROOT}/CMakeLists.txt\")\n"
        "  set(ROHR_BUILD_EXAMPLES OFF CACHE BOOL \"\" FORCE)\n"
        "  set(ROHR_BUILD_TESTS OFF CACHE BOOL \"\" FORCE)\n"
        "  set(ROHR_BUILD_EDITOR OFF CACHE BOOL \"\" FORCE)\n"
        "  set(ROHR_BUILD_CLI OFF CACHE BOOL \"\" FORCE)\n"
        "  add_subdirectory(\"${ROHR_ENGINE_SOURCE_ROOT}\" rohr-engine)\n"
        "  set(ROHR_ENGINE_TARGET rohr_engine)\nelse()\n"
        "  if(DEFINED ROHR_ENGINE_SOURCE_ROOT)\n    list(PREPEND CMAKE_PREFIX_PATH \"${ROHR_ENGINE_SOURCE_ROOT}\")\n  endif()\n"
        "  find_package(Rohr CONFIG REQUIRED)\n  set(ROHR_ENGINE_TARGET Rohr::Engine)\nendif()\n"
        "file(GLOB ROHR_GENERATED_SOURCES CONFIGURE_DEPENDS \"${ROHR_GENERATED_DIRECTORY}/*.c\")\n"
        "list(FILTER ROHR_GENERATED_SOURCES EXCLUDE REGEX \"(^|/)project_scene[.]c$\")\n"
        "add_executable(${PROJECT_NAME} \"${ROHR_SOURCE_DIRECTORY}/main.c\" ${ROHR_GENERATED_SOURCES})\n"
        "target_include_directories(${PROJECT_NAME} PRIVATE \"${ROHR_GENERATED_DIRECTORY}\")\n"
        "target_link_libraries(${PROJECT_NAME} PRIVATE ${ROHR_ENGINE_TARGET})\n"
        "rohr_project_output(${PROJECT_NAME} APPLICATION)\n"
        "add_custom_target(${PROJECT_NAME}_assets ALL COMMAND ${CMAKE_COMMAND} -E copy_directory\n"
        "  \"${ROHR_ASSET_DIRECTORY}\" \"$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets\" VERBATIM)\n"
        "add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_assets)\n", workspace->config.name);
    return n >= 0 && (size_t)n < capacity;
}
static bool editor_workspace_scene_cmake_previous_format(const EditorWorkspace *workspace, char *output, size_t capacity) {
    (void)workspace;
    const char *text =
        "# Rohr managed scene scaffold v2\ninclude(\"${CMAKE_SOURCE_DIR}/.rohr/project_paths.cmake\")\n"
        "configure_file(\"${CMAKE_SOURCE_DIR}/.rohr/project_paths.cmake\" \"${CMAKE_BINARY_DIR}/rohr_paths_configured.cmake\" COPYONLY)\n"
        "get_target_property(_rohr_app_sources ${PROJECT_NAME} SOURCES)\n"
        "list(FILTER _rohr_app_sources EXCLUDE REGEX \"(^|/)project_scene[.]c$\")\n"
        "set_property(TARGET ${PROJECT_NAME} PROPERTY SOURCES \"${_rohr_app_sources}\")\n"
        "add_executable(${PROJECT_NAME}_scene \"${ROHR_GENERATED_DIRECTORY}/project_scene.c\"\n"
        "  \"${ROHR_GENERATED_DIRECTORY}/scene/project_objects.c\" \"${ROHR_GENERATED_DIRECTORY}/scene/project_viewports.c\")\n"
        "target_include_directories(${PROJECT_NAME}_scene PRIVATE \"${ROHR_GENERATED_DIRECTORY}/scene\")\n"
        "target_link_libraries(${PROJECT_NAME}_scene PRIVATE ${ROHR_ENGINE_TARGET})\n"
        "rohr_project_output(${PROJECT_NAME}_scene SCENE)\n"
        "add_custom_target(${PROJECT_NAME}_scene_assets ALL COMMAND ${CMAKE_COMMAND} -E copy_directory\n"
        "  \"${ROHR_ASSET_DIRECTORY}\" \"$<TARGET_FILE_DIR:${PROJECT_NAME}_scene>/assets\" VERBATIM)\n"
        "if(TARGET ${PROJECT_NAME}_assets)\n  add_dependencies(${PROJECT_NAME}_scene_assets ${PROJECT_NAME}_assets)\nendif()\n"
        "add_dependencies(${PROJECT_NAME}_scene ${PROJECT_NAME}_scene_assets)\n"
        "add_custom_target(${PROJECT_NAME}_project DEPENDS ${PROJECT_NAME} ${PROJECT_NAME}_scene)\n"
        "file(WRITE \"${CMAKE_BINARY_DIR}/rohr_target_application.txt\" \"${PROJECT_NAME}\")\n"
        "file(WRITE \"${CMAKE_BINARY_DIR}/rohr_target_scene.txt\" \"${PROJECT_NAME}_scene\")\n"
        "file(WRITE \"${CMAKE_BINARY_DIR}/rohr_target_project.txt\" \"${PROJECT_NAME}_project\")\n"
        "file(GENERATE OUTPUT \"${CMAKE_BINARY_DIR}/rohr_application_$<CONFIG>.path\" CONTENT \"$<TARGET_FILE:${PROJECT_NAME}>\")\n"
        "file(GENERATE OUTPUT \"${CMAKE_BINARY_DIR}/rohr_scene_$<CONFIG>.path\" CONTENT \"$<TARGET_FILE:${PROJECT_NAME}_scene>\")\n";
    return SDL_strlcpy(output, text, capacity) < capacity;
}

static bool editor_workspace_scene_cmake_format(const EditorWorkspace *workspace, char *output, size_t capacity) {
    (void)workspace;
    const char *text =
        "# Rohr managed scene scaffold v2\ninclude(\"${CMAKE_SOURCE_DIR}/.rohr/project_paths.cmake\")\n"
        "configure_file(\"${CMAKE_SOURCE_DIR}/.rohr/project_paths.cmake\" \"${CMAKE_BINARY_DIR}/rohr_paths_configured.cmake\" COPYONLY)\n"
        "get_target_property(_rohr_app_sources ${PROJECT_NAME} SOURCES)\n"
        "list(FILTER _rohr_app_sources EXCLUDE REGEX \"(^|/)project_scene[.]c$\")\n"
        "set_property(TARGET ${PROJECT_NAME} PROPERTY SOURCES \"${_rohr_app_sources}\")\n"
        "add_executable(${PROJECT_NAME}_scene \"${ROHR_GENERATED_DIRECTORY}/project_scene.c\"\n"
        "  \"${ROHR_GENERATED_DIRECTORY}/scene/project_objects.c\")\n"
        "target_include_directories(${PROJECT_NAME}_scene PRIVATE \"${ROHR_GENERATED_DIRECTORY}/scene\")\n"
        "target_link_libraries(${PROJECT_NAME}_scene PRIVATE ${ROHR_ENGINE_TARGET})\n"
        "rohr_project_output(${PROJECT_NAME}_scene SCENE)\n"
        "add_custom_target(${PROJECT_NAME}_scene_assets ALL COMMAND ${CMAKE_COMMAND} -E copy_directory\n"
        "  \"${ROHR_ASSET_DIRECTORY}\" \"$<TARGET_FILE_DIR:${PROJECT_NAME}_scene>/assets\" VERBATIM)\n"
        "if(TARGET ${PROJECT_NAME}_assets)\n  add_dependencies(${PROJECT_NAME}_scene_assets ${PROJECT_NAME}_assets)\nendif()\n"
        "add_dependencies(${PROJECT_NAME}_scene ${PROJECT_NAME}_scene_assets)\n"
        "add_custom_target(${PROJECT_NAME}_project DEPENDS ${PROJECT_NAME} ${PROJECT_NAME}_scene)\n"
        "file(WRITE \"${CMAKE_BINARY_DIR}/rohr_target_application.txt\" \"${PROJECT_NAME}\")\n"
        "file(WRITE \"${CMAKE_BINARY_DIR}/rohr_target_scene.txt\" \"${PROJECT_NAME}_scene\")\n"
        "file(WRITE \"${CMAKE_BINARY_DIR}/rohr_target_project.txt\" \"${PROJECT_NAME}_project\")\n"
        "file(GENERATE OUTPUT \"${CMAKE_BINARY_DIR}/rohr_application_$<CONFIG>.path\" CONTENT \"$<TARGET_FILE:${PROJECT_NAME}>\")\n"
        "file(GENERATE OUTPUT \"${CMAKE_BINARY_DIR}/rohr_scene_$<CONFIG>.path\" CONTENT \"$<TARGET_FILE:${PROJECT_NAME}_scene>\")\n";
    return SDL_strlcpy(output, text, capacity) < capacity;
}

static const char scene_cmake_include[] =
    "\ninclude(\"${ROHR_GENERATED_DIRECTORY}/project_scene.cmake\")\n";

static bool editor_workspace_scaffold_write(const EditorWorkspace *workspace) {
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    char cmake[EDITOR_WORKSPACE_PATH_MAX * 8];
    if(!editor_workspace_cmake_format(workspace, cmake, sizeof(cmake)) ||
            strlen(cmake) + strlen(scene_cmake_include) >= sizeof(cmake)) return false;
    strcat(cmake, scene_cmake_include);
    return editor_workspace_path_join(path, sizeof(path), workspace->directory,
            "CMakeLists.txt") && editor_workspace_file_write(path, cmake) &&
        editor_workspace_path_join(path, sizeof(path), workspace->directory,
            ".gitignore") && editor_workspace_file_write(path, "build/\nlogs/\n");
}

static bool editor_workspace_scene_cmake_legacy_format(const EditorWorkspace *workspace,
        char *cmake, size_t capacity) {
    int count = snprintf(cmake, capacity,
        "# Generated by Rohr Engine. Preserved when present; restored if missing.\n"
        "# Older application scaffolds glob src/generated/*.c. Keep the scene entry point separate.\n"
        "get_target_property(_rohr_app_sources ${PROJECT_NAME} SOURCES)\n"
        "list(FILTER _rohr_app_sources EXCLUDE REGEX \"(^|/)project_scene[.]c$\")\n"
        "set_property(TARGET ${PROJECT_NAME} PROPERTY SOURCES \"${_rohr_app_sources}\")\n"
        "unset(_rohr_app_sources)\n"
        "add_executable(${PROJECT_NAME}_scene src/generated/project_scene.c\n"
        "    src/generated/scene/project_objects.c src/generated/scene/project_viewports.c)\n"
        "target_include_directories(${PROJECT_NAME}_scene PRIVATE src/generated/scene)\n"
        "target_link_libraries(${PROJECT_NAME}_scene PRIVATE ${ROHR_ENGINE_TARGET})\n"
        "add_custom_target(${PROJECT_NAME}_scene_assets ALL\n"
        "    COMMAND ${CMAKE_COMMAND} -E copy_directory\n"
        "        \"${CMAKE_SOURCE_DIR}/%s\"\n"
        "        \"$<TARGET_FILE_DIR:${PROJECT_NAME}_scene>/%s\"\n"
        "    VERBATIM)\n"
        "# Serialize staging when both executables share an output directory.\n"
        "if(TARGET ${PROJECT_NAME}_assets)\n"
        "    add_dependencies(${PROJECT_NAME}_scene_assets ${PROJECT_NAME}_assets)\n"
        "endif()\n"
        "add_dependencies(${PROJECT_NAME}_scene ${PROJECT_NAME}_scene_assets)\n"
        "add_custom_target(${PROJECT_NAME}_project DEPENDS ${PROJECT_NAME} ${PROJECT_NAME}_scene)\n"
        "# Record actual targets and output paths, including multi-config builds.\n"
        "file(WRITE \"${CMAKE_BINARY_DIR}/rohr_target_application.txt\" \"${PROJECT_NAME}\")\n"
        "file(WRITE \"${CMAKE_BINARY_DIR}/rohr_target_scene.txt\" \"${PROJECT_NAME}_scene\")\n"
        "file(WRITE \"${CMAKE_BINARY_DIR}/rohr_target_project.txt\" \"${PROJECT_NAME}_project\")\n"
        "file(GENERATE OUTPUT \"${CMAKE_BINARY_DIR}/rohr_application_$<CONFIG>.path\"\n"
        "    CONTENT \"$<TARGET_FILE:${PROJECT_NAME}>\")\n"
        "file(GENERATE OUTPUT \"${CMAKE_BINARY_DIR}/rohr_scene_$<CONFIG>.path\"\n"
        "    CONTENT \"$<TARGET_FILE:${PROJECT_NAME}_scene>\")\n",
        workspace->config.asset_directory, workspace->config.asset_directory);
    return count >= 0 && (size_t)count < capacity;
}


EditorResult editor_workspace_build_paths_write(const EditorWorkspace *workspace) {
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    if(!editor_workspace_directory_create(workspace->directory, ".rohr") ||
            !editor_workspace_path_join(path, sizeof(path), workspace->directory, ".rohr/project_paths.cmake"))
        return editor_result_error(EDITOR_ERROR_FILE_IO, "Could not prepare project path settings");
    char temporary[sizeof(path) + 48];
    snprintf(temporary, sizeof(temporary), "%s.tmp_%llu", path, (unsigned long long)SDL_GetPerformanceCounter());
    FILE *file = fopen(temporary, "wbx");
    if(!file) return editor_result_error(EDITOR_ERROR_FILE_IO, "Could not write %s", temporary);
    const EditorWorkspacePath kinds[] = {EDITOR_PATH_SOURCE, EDITOR_PATH_GENERATED, EDITOR_PATH_ASSETS,
        EDITOR_PATH_ENGINE, EDITOR_PATH_APPLICATION, EDITOR_PATH_SCENE};
    const char *variables[] = {"ROHR_SOURCE_DIRECTORY", "ROHR_GENERATED_DIRECTORY", "ROHR_ASSET_DIRECTORY",
        "ROHR_ENGINE_SOURCE_ROOT", "ROHR_APPLICATION_EXECUTABLE", "ROHR_SCENE_EXECUTABLE"};
    bool ok = fputs("# Generated project paths. Edit these through Build Settings.\n", file) >= 0;
    for(size_t i = 0; ok && i < SDL_arraysize(kinds); ++i) {
        const char *configured = editor_workspace_path_value_get((EditorWorkspace *)workspace, kinds[i]);
        if(i >= 3 && !*configured) continue;
        char resolved[EDITOR_WORKSPACE_PATH_MAX * 2], root[EDITOR_WORKSPACE_PATH_MAX * 2];
        EditorResult r = editor_workspace_path_get(workspace, kinds[i], resolved, sizeof(resolved));
        if(editor_result_check(r) || editor_result_check(editor_workspace_path_get(workspace,
                EDITOR_PATH_PROJECT, root, sizeof(root)))) { ok = false; break; }
        if(editor_workspace_path_contains_check(root, resolved))
            ok = fprintf(file, "set(%s \"${CMAKE_SOURCE_DIR}%s\")\n", variables[i], resolved + strlen(root)) >= 0;
        else ok = fprintf(file, "set(%s \"%s\")\n", variables[i], resolved) >= 0;
    }
    if(ok) ok = fputs(
        "function(rohr_project_output target kind)\n"
        "  if(DEFINED ROHR_${kind}_EXECUTABLE)\n"
        "    get_filename_component(directory \"${ROHR_${kind}_EXECUTABLE}\" DIRECTORY)\n"
        "    get_filename_component(filename \"${ROHR_${kind}_EXECUTABLE}\" NAME)\n"
        "    set_target_properties(${target} PROPERTIES OUTPUT_NAME \"${filename}\" SUFFIX \"\"\n"
        "      RUNTIME_OUTPUT_DIRECTORY \"$<1:${directory}>\")\n"
        "  endif()\nendfunction()\n", file) >= 0;
    bool closed = fclose(file) == 0;
    if(ok && closed) {
        size_t before_size = 0, after_size = 0;
        void *before = SDL_LoadFile(path, &before_size), *after = SDL_LoadFile(temporary, &after_size);
        bool same = before && after && before_size == after_size && !memcmp(before, after, before_size);
        SDL_free(before); SDL_free(after);
        if(same) { (void)SDL_RemovePath(temporary); return editor_result_value(true); }
        if(SDL_RenamePath(temporary, path)) return editor_result_value(true);
    }
    (void)SDL_RemovePath(temporary);
    return editor_result_error(EDITOR_ERROR_FILE_IO, "Could not write %s", path);
}

static bool scaffold_known(const EditorWorkspace *workspace, const char *text, bool scene) {
    char expected[EDITOR_WORKSPACE_PATH_MAX * 8];
    if(scene) {
        if(editor_workspace_scene_cmake_previous_format(workspace, expected, sizeof(expected)) && !strcmp(text, expected)) return true;
        if(editor_workspace_scene_cmake_legacy_format(workspace, expected, sizeof(expected)) && !strcmp(text, expected)) return true;
        return editor_workspace_scene_cmake_format(workspace, expected, sizeof(expected)) && !strcmp(text, expected);
    }
    if(editor_workspace_cmake_legacy_format(workspace, expected, sizeof(expected))) {
        if(!strcmp(text, expected)) return true;
        SDL_strlcat(expected, "\ninclude(src/generated/project_scene.cmake)\n", sizeof(expected));
        if(!strcmp(text, expected)) return true;
    }
    if(!editor_workspace_cmake_format(workspace, expected, sizeof(expected))) return false;
    if(!strcmp(text, expected)) return true;
    SDL_strlcat(expected, scene_cmake_include, sizeof(expected));
    return !strcmp(text, expected);
}
EditorResult editor_workspace_scaffold_paths_check(const EditorWorkspace *workspace) {
    const char *names[] = {"CMakeLists.txt", "src/generated/project_scene.cmake"};
    for(size_t i = 0; i < SDL_arraysize(names); ++i) {
        char path[EDITOR_WORKSPACE_PATH_MAX * 2]; SDL_PathInfo info;
        if(!editor_workspace_file_path(workspace, path, sizeof(path), names[i]))
            return editor_result_error(EDITOR_ERROR_CAPACITY, "CMake path is too long");
        if(!SDL_GetPathInfo(path, &info)) continue;
        char *text = SDL_LoadFile(path, NULL);
        bool ok = text && (scaffold_known(workspace, text, i != 0) ||
            strstr(text, "# Rohr path settings compatible") != NULL);
        SDL_free(text);
        if(!ok) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "Customized %s must use .rohr/project_paths.cmake and ROHR_SOURCE_DIRECTORY, ROHR_GENERATED_DIRECTORY, "
            "ROHR_ASSET_DIRECTORY and rohr_project_output; mark it '# Rohr path settings compatible' after adapting it. No files moved.", path);
    }
    return editor_result_value(true);
}
EditorResult editor_workspace_scaffold_paths_write(const EditorWorkspace *workspace) {
    EditorResult r = editor_workspace_build_paths_write(workspace);
    if(editor_result_check(r)) return r;
    const char *names[] = {"CMakeLists.txt", "src/generated/project_scene.cmake"};
    for(size_t i = 0; i < SDL_arraysize(names); ++i) {
        char path[EDITOR_WORKSPACE_PATH_MAX * 2], text[EDITOR_WORKSPACE_PATH_MAX * 8];
        if(!editor_workspace_file_path(workspace, path, sizeof(path), names[i]))
            return editor_result_error(EDITOR_ERROR_CAPACITY, "CMake path is too long");
        char *existing = SDL_LoadFile(path, NULL);
        bool custom = existing && strstr(existing, "# Rohr path settings compatible");
        SDL_free(existing); if(custom) continue;
        bool ok = i ? editor_workspace_scene_cmake_format(workspace, text, sizeof(text)) :
            editor_workspace_cmake_format(workspace, text, sizeof(text));
        if(!i) SDL_strlcat(text, scene_cmake_include, sizeof(text));
        if(!ok || !editor_workspace_file_write(path, text))
            return editor_result_error(EDITOR_ERROR_FILE_IO, "Could not update %s", path);
    }
    return editor_result_value(true);
}

/* Exclusive creation keeps existing customizations and dangling links intact. */
static EditorResult editor_workspace_cmake_restore_file(const EditorWorkspace *workspace,
        const char *relative, const char *contents, bool *restored) {
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    SDL_PathInfo info;
    *restored = false;
    if(!editor_workspace_file_path(workspace, path, sizeof(path), relative))
        return editor_result_error(EDITOR_ERROR_CAPACITY, "CMake path is too long: %s", relative);
    if(SDL_GetPathInfo(path, &info)) {
        if(info.type != SDL_PATHTYPE_FILE) return editor_result_error(EDITOR_ERROR_FILE_IO,
            "Expected a CMake file: %s", path);
        FILE *existing = fopen(path, "rb");
        if(!existing) return editor_result_error(EDITOR_ERROR_FILE_IO,
            "Could not read existing CMake file: %s", path);
        bool readable = fgetc(existing) != EOF || !ferror(existing);
        bool closed = fclose(existing) == 0;
        if(!readable || !closed) return editor_result_error(EDITOR_ERROR_FILE_IO,
            "Could not read existing CMake file: %s", path);
        if(!strcmp(relative, "src/generated/project_scene.cmake")) {
            char *text = SDL_LoadFile(path, NULL);
            bool upgrade = text && scaffold_known(workspace, text, true) && strcmp(text, contents);
            SDL_free(text);
            if(upgrade) {
                if(!editor_workspace_file_write(path, contents))
                    return editor_result_error(EDITOR_ERROR_FILE_IO, "Could not update scene build %s", path);
                *restored = true;
            }
        }
        return editor_result_value(true);
    }
    FILE *file = fopen(path, "wbx");
    if(!file) return editor_result_error(EDITOR_ERROR_FILE_IO,
        "Could not restore missing CMake file %s: %s", path, strerror(errno));
    bool written = fwrite(contents, 1, strlen(contents), file) == strlen(contents);
    bool closed = fclose(file) == 0;
    if(!written || !closed) {
        (void)SDL_RemovePath(path); /* Only the file exclusively created above. */
        return editor_result_error(EDITOR_ERROR_FILE_IO, "Could not write restored CMake file: %s", path);
    }
    *restored = true;
    return editor_result_value(true);
}

EditorResult editor_workspace_cmake_restore(const EditorWorkspace *workspace,
        bool *root_restored, bool *scene_restored) {
    if(!workspace || !workspace->open || !root_restored || !scene_restored)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "CMake recovery needs an open project");
    *root_restored = *scene_restored = false;
    EditorResult paths = editor_workspace_build_paths_write(workspace);
    if(editor_result_check(paths)) return paths;
    char cmake[EDITOR_WORKSPACE_PATH_MAX * 8];
    if(!editor_workspace_cmake_format(workspace, cmake, sizeof(cmake)) ||
            strlen(cmake) + strlen(scene_cmake_include) >= sizeof(cmake))
        return editor_result_error(EDITOR_ERROR_CAPACITY, "Project CMake template is too long");
    strcat(cmake, scene_cmake_include);
    EditorResult result = editor_workspace_cmake_restore_file(workspace, "CMakeLists.txt", cmake, root_restored);
    if(editor_result_check(result)) return result;
    if(!editor_workspace_directory_create(workspace->directory, workspace->config.generated_directory))
        return editor_result_error(EDITOR_ERROR_FILE_IO,
            "Could not prepare CMake directory: %s/src/generated", workspace->directory);
    if(!editor_workspace_scene_cmake_format(workspace, cmake, sizeof(cmake)))
        return editor_result_error(EDITOR_ERROR_CAPACITY, "Scene CMake template is too long");
    return editor_workspace_cmake_restore_file(workspace, "src/generated/project_scene.cmake", cmake, scene_restored);
}

/* The filtered view borrows authored data; only the top-level object array and
 * camera-item array below are owned here. Never destroy it as an EditorProject. */
static bool editor_workspace_scene_modules_write(const EditorWorkspace *workspace,
        const EditorProject *project, const EditorSceneSelection *selection) {
    EditorProject filtered = *project;
    EditorLayoutViewport viewport = {0};
    EditorObject *objects = NULL;
    EditorViewportCameraItem *items = NULL;
    bool written = false;
    char scene_directory[EDITOR_WORKSPACE_PATH_MAX * 2];
    if(!editor_workspace_file_path(workspace, scene_directory, sizeof(scene_directory), "src/generated/scene") ||
            !SDL_CreateDirectory(scene_directory)) return false;
    if(selection && selection->objects_selected) {
        if(selection->object_count) {
            objects = calloc(selection->object_count, sizeof(*objects));
            if(!objects) return false;
        }
        filtered.objects = objects; filtered.object_count = 0;
        for(size_t i = 0; i < project->object_count; ++i)
            for(size_t j = 0; j < selection->object_count; ++j)
                if(project->objects[i].id == selection->objects[j]) {
                    objects[filtered.object_count++] = project->objects[i]; break;
                }
    }
    filtered.layout_viewports = NULL; filtered.layout_viewport_count = 0;
    for(size_t i = 0; i < project->layout_viewport_count; ++i) {
        if(selection && selection->viewport_selected && !selection->viewport) break;
        if(selection && selection->viewport && project->layout_viewports[i].id != selection->viewport) continue;
        viewport = project->layout_viewports[i];
        filtered.layout_viewports = &viewport; filtered.layout_viewport_count = 1;
        if(viewport.camera_item_count) {
            items = calloc(viewport.camera_item_count, sizeof(*items));
            if(!items) goto done;
        }
        viewport.camera_items = items; viewport.camera_item_count = 0;
        for(size_t j = 0; j < project->layout_viewports[i].camera_item_count; ++j) {
            const EditorViewportCameraItem *item = &project->layout_viewports[i].camera_items[j];
            for(size_t k = 0; k < filtered.object_count; ++k)
                if(filtered.objects[k].id == item->object &&
                        editor_project_camera_get(&filtered.objects[k], item->camera)) {
                    items[viewport.camera_item_count++] = *item; break;
                }
        }
        break;
    }
    written = editor_workspace_generated_objects_write(workspace, &filtered, true);
done:
    free(items); free(objects); return written;
}

/* Only exact former scene scaffolds are removed. Customized legacy source is
 * preserved but no longer compiled by the generated scene target. */
static bool editor_workspace_scene_write(const EditorWorkspace *workspace,
        const EditorProject *project, const EditorSceneSelection *selection) {
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    char cmake[EDITOR_WORKSPACE_PATH_MAX * 8];
    SDL_PathInfo info;
    if(!editor_workspace_scene_modules_write(workspace, project, selection) ||
            !editor_workspace_file_path(workspace, path, sizeof(path), "src/generated/project_scene.c") ||
            !editor_workspace_file_write(path, editor_scene_source)) return false;
    if(!editor_workspace_file_path(workspace, path, sizeof(path), "src/scene.c")) return false;
    if(SDL_GetPathInfo(path, &info)) {
        if(info.type != SDL_PATHTYPE_FILE) return false;
        char *existing_scene = SDL_LoadFile(path, NULL);
        if(!existing_scene) return false;
        bool known = strcmp(existing_scene, editor_workspace_main_source) == 0 ||
            strcmp(existing_scene, editor_workspace_main_legacy_viewport_source) == 0 ||
            strcmp(existing_scene, editor_scene_source_legacy) == 0;
        SDL_free(existing_scene);
        if(known) { if(!SDL_RemovePath(path)) return false; }
        else console_write(LOG_ENGINE,
            "Preserved customized %s. The scene target now uses disposable generated "
            "src/generated/project_scene.c; this legacy source is unused.\n", path);
    }
    if(!editor_workspace_scene_cmake_format(workspace, cmake, sizeof(cmake))) return false;
    bool restored;
    EditorResult result = editor_workspace_cmake_restore_file(workspace,
        "src/generated/project_scene.cmake", cmake, &restored);
    if(editor_result_check(result)) {
        editor_log_error("Scene build integration", "%s", result.result.error.message);
        return false;
    }
    if(!editor_workspace_path_join(path, sizeof(path), workspace->directory,
            "CMakeLists.txt")) return false;
    char *existing = SDL_LoadFile(path, NULL);
    if(existing == NULL) {
        editor_log_error("Scene build integration",
            "No readable %s; include src/generated/project_scene.cmake from your build "
            "after defining ROHR_GENERATED_SOURCES and ROHR_ENGINE_TARGET.", path);
        return true; /* Code generation also supports externally managed builds. */
    }
    bool success = editor_workspace_cmake_format(workspace, cmake, sizeof(cmake));
    if(success && strcmp(existing, cmake) == 0) {
        if(strlen(cmake) + strlen(scene_cmake_include) >= sizeof(cmake)) success = false;
        else {
            strcat(cmake, scene_cmake_include);
            success = editor_workspace_file_write(path, cmake);
        }
    } else if(success && strstr(existing, "project_scene.cmake") == NULL) {
        editor_log_error("Scene build integration",
            "Preserved customized %s. To add the scene target, include(src/generated/project_scene.cmake) "
            "after defining ROHR_GENERATED_SOURCES and ROHR_ENGINE_TARGET.", path);
    }
    SDL_free(existing);
    return success;
}

static bool editor_workspace_config_template_copy(const EditorWorkspace *workspace) {
    char source[EDITOR_WORKSPACE_PATH_MAX * 2];
    char destination[EDITOR_WORKSPACE_PATH_MAX * 2];
    size_t size = 0;
    void *contents;
    FILE *file;
    EditorResult result = editor_config_sdk_path_get(source, sizeof(source),
        "project_editor.lua", true);
    if(editor_result_check(result) || !editor_workspace_path_join(destination,
            sizeof(destination), workspace->directory, "editor.lua")) return false;
    contents = SDL_LoadFile(source, &size);
    if(contents == NULL) return false;
    file = fopen(destination, "wb");
    if(file == NULL) {
        SDL_free(contents);
        return false;
    }
    bool written = fwrite(contents, 1, size, file) == size;
    bool closed = fclose(file) == 0;
    SDL_free(contents);
    return written && closed;
}

static bool editor_workspace_starter_assets_copy(const EditorWorkspace *workspace) {
    static const char *names[] = {"tutorial_frame_1.png", "tutorial_frame_2.png"};
    static const char *development_names[] = {
        "examples/flies-around-ball/assets/elder-fly/flying/f1.png",
        "examples/flies-around-ball/assets/elder-fly/flying/f2.png"
    };
    char relative[EDITOR_WORKSPACE_PATH_MAX];
    char source[EDITOR_WORKSPACE_PATH_MAX * 2];
    char destination[EDITOR_WORKSPACE_PATH_MAX * 2];
    for(size_t i = 0; i < sizeof(names) / sizeof(names[0]); i += 1) {
        EditorResult result;
        if(snprintf(relative, sizeof(relative), "starter-assets/%s", names[i]) >=
                (int)sizeof(relative)) return false;
        result = editor_config_sdk_path_get(source, sizeof(source), relative, false);
        if(editor_result_check(result) || source[0] == '\0') {
            if(snprintf(source, sizeof(source), "%s/%s", ROHR_DEVELOPMENT_SOURCE_DIR,
                    development_names[i]) >= (int)sizeof(source)) return false;
        }
        if(editor_result_check(editor_workspace_path_get(workspace, EDITOR_PATH_ASSETS, relative, sizeof(relative))) ||
                !editor_workspace_path_join(destination, sizeof(destination), relative, names[i]) ||
                !SDL_CopyFile(source, destination)) return false;
    }
    return true;
}

bool editor_workspace_save(const EditorWorkspace *workspace,
    const EditorProject *project) {
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];

    return workspace != NULL && workspace->open && project != NULL &&
        editor_workspace_manifest_save(workspace) &&
        editor_workspace_path_join(path, sizeof(path), workspace->directory,
        workspace->config.editor_state_file) && editor_project_save(project, path);
}

static bool editor_workspace_legacy_main_upgrade(const EditorWorkspace *workspace,
        const EditorProject *project) {
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    char *contents;
    bool legacy;
    if(!editor_workspace_file_path(workspace, path, sizeof(path), "src/main.c")) return false;
    contents = SDL_LoadFile(path, NULL);
    if(contents == NULL) return false;
    bool legacy_joints = strcmp(contents, editor_starter_main_legacy_joint_source) == 0;
    bool legacy_viewport = !strcmp(contents, editor_starter_main_legacy_viewport_source);
    legacy = legacy_joints || legacy_viewport || !strcmp(contents, editor_workspace_main_legacy_viewport_source) ||
        editor_workspace_tick_main_check(contents);
    if(!legacy && (strstr(contents, "project_viewports.h") || strstr(contents, "project_viewports_create(")))
        console_write(LOG_ENGINE, "Preserved developer-owned %s. Generated viewport helpers were removed; "
            "migrate to project.h, rohr_viewport_create and rohr_graphics_viewports_show.\n", path);
    SDL_free(contents);
    return !legacy || editor_workspace_main_write(workspace, project, legacy_joints || legacy_viewport);
}

static bool editor_workspace_c_generate_selected(const EditorWorkspace *workspace,
    const EditorProject *project, const EditorSceneSelection *selection) {
    if(!project || !workspace || editor_result_check(editor_workspace_build_paths_write(workspace))) return false;
    if(!editor_workspace_directory_create(workspace->directory, workspace->config.generated_directory)) return false;
    for(size_t i = 0; i < project->object_count; ++i)
        if(!editor_sources_check(&project->objects[i])) return false;
    return workspace != NULL && workspace->open &&
        editor_project_center_of_mass_check(project) &&
        editor_project_presentation_check(project) &&
        editor_soft_areas_generation_validate(project).kind != ERROR_RESULT_ERROR &&
        editor_workspace_generated_objects_write(workspace, project, false) &&
        editor_workspace_legacy_main_upgrade(workspace, project) &&
        editor_workspace_scene_write(workspace, project, selection) &&
        editor_workspace_viewport_modules_remove(workspace);
}

static EditorResult editor_workspace_screen_references_validate(const EditorProject *project) {
    EditorResult catalog = editor_project_catalog_validate(project);
    if(editor_result_check(catalog)) return catalog;
    for(size_t v = 0; v < project->layout_viewport_count; ++v) {
        const EditorLayoutViewport *viewport = &project->layout_viewports[v];
        for(size_t i = 0; i < viewport->camera_item_count; ++i) {
            const EditorViewportCameraItem *screen = &viewport->camera_items[i];
            const EditorObject *object = NULL;
            for(size_t o = 0; o < project->object_count; ++o)
                if(project->objects[o].id == screen->object) object = &project->objects[o];
            if(!object) return editor_result_error(EDITOR_ERROR_NOT_FOUND,
                "Viewport '%s' screen %u references missing object %u (camera %u)",
                viewport->name, screen->id, screen->object, screen->camera);
            if(!editor_project_camera_get((EditorObject *)object, screen->camera))
                return editor_result_error(EDITOR_ERROR_NOT_FOUND,
                    "Viewport '%s' screen %u references missing camera %u in object '%s' (%u)",
                    viewport->name, screen->id, screen->camera, object->name, object->id);
        }
    }
    return editor_result_value(true);
}

EditorResult editor_workspace_scene_generate(const EditorWorkspace *workspace,
        const EditorProject *project, const EditorSceneSelection *selection) {
    if(!workspace || !workspace->open || !project)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Scene generation needs an open project");
    EditorResult references = editor_workspace_screen_references_validate(project);
    if(editor_result_check(references)) return references;
    if(selection) {
        if(selection->object_count && (!selection->objects_selected || !selection->objects))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Invalid scene object selection");
        for(size_t i = 0; i < selection->object_count; ++i) {
            bool found = false;
            for(size_t j = 0; j < project->object_count; ++j)
                found |= project->objects[j].id == selection->objects[i];
            if(!found) return editor_result_error(EDITOR_ERROR_NOT_FOUND, "Scene object %u was not found", selection->objects[i]);
            for(size_t j = 0; j < i; ++j)
                if(selection->objects[j] == selection->objects[i])
                    return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Scene object %u was selected twice", selection->objects[i]);
        }
        if(selection->viewport) {
            bool found = false;
            for(size_t i = 0; i < project->layout_viewport_count; ++i) found |= project->layout_viewports[i].id == selection->viewport;
            if(!found) return editor_result_error(EDITOR_ERROR_NOT_FOUND, "Scene viewport %u was not found", selection->viewport);
        }
    }
    return editor_workspace_c_generate_selected(workspace, project, selection) ? editor_result_value(true) :
        editor_result_error(EDITOR_ERROR_FILE_IO, "Could not generate scene C under: %s", workspace->directory);
}

bool editor_workspace_c_generate(const EditorWorkspace *workspace, const EditorProject *project) {
    return !editor_result_check(editor_workspace_scene_generate(workspace, project, NULL));
}

EditorResult editor_workspace_editor_generate(const EditorWorkspace *workspace,
        const EditorProject *project) {
    if(!project) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "No project is open");
    EditorResult validation = editor_soft_areas_generation_validate(project);
    if(editor_result_check(validation)) return validation;
    EditorObjectId *ids = project->object_count ? malloc(project->object_count * sizeof(*ids)) : NULL;
    if(project->object_count && !ids)
        return editor_result_error(EDITOR_ERROR_CAPACITY, "Could not allocate simulation selection");
    EditorSceneSelection selection = {.objects_selected = true, .objects = ids,
        .viewport_selected = true, .viewport = project->simulation_viewport};
    for(size_t i = 0; i < project->object_count; ++i)
        if(!project->objects[i].simulation_excluded) ids[selection.object_count++] = project->objects[i].id;
    EditorResult result = editor_workspace_scene_generate(workspace, project, &selection);
    free(ids);
    return result;
}

EditorResult editor_workspace_create(EditorWorkspace *workspace, EditorProject *project,
    const char *directory) {
    EditorWorkspace created = {0};

    if(workspace == NULL || project == NULL || directory == NULL || directory[0] == '\0' ||
            strlen(directory) >= sizeof(created.directory))
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "Project creation received an invalid or oversized path");
    if(!SDL_CreateDirectory(directory)) return editor_result_error(
        EDITOR_ERROR_FILE_IO, "Could not create project directory: %s", directory);
    if(!editor_workspace_directory_empty(directory)) return editor_result_error(
        EDITOR_ERROR_FILE_IO, "Project directory is not empty: %s", directory);
    created.config = editor_workspace_config_default_get();
    editor_project_object_name_format(created.config.name,
        sizeof(created.config.name), editor_workspace_basename(directory));
    snprintf(created.directory, sizeof(created.directory), "%s", directory);
    created.open = true;
    if(!editor_workspace_directory_create(directory, created.config.source_directory) ||
            !editor_workspace_directory_create(directory,
                created.config.generated_directory) ||
            !editor_workspace_directory_create(directory, created.config.asset_directory) ||
            !editor_workspace_directory_create(directory, created.config.object_directory)) {
        return editor_result_error(EDITOR_ERROR_FILE_IO,
            "Could not create the project directory structure under: %s", directory);
    }
    if(!editor_workspace_starter_project_init(project)) return editor_result_error(
        EDITOR_ERROR_SCHEMA_INVALID, "Could not initialize the starter project");
    if(!editor_workspace_starter_assets_copy(&created)) return editor_result_error(
        EDITOR_ERROR_FILE_IO, "Could not copy starter assets under: %s", directory);
    if(!editor_workspace_save(&created, project)) return editor_result_error(
        EDITOR_ERROR_FILE_IO, "Could not save the initial project state under: %s", directory);
    if(editor_result_check(editor_workspace_build_paths_write(&created)) || !editor_workspace_scaffold_write(&created)) return editor_result_error(
        EDITOR_ERROR_FILE_IO, "Could not write the project build scaffold under: %s", directory);
    if(!editor_workspace_config_template_copy(&created)) return editor_result_error(
        EDITOR_ERROR_FILE_IO, "Could not copy editor.lua under: %s", directory);
    if(!editor_workspace_generated_objects_write(&created, project, false))
        return editor_result_error(EDITOR_ERROR_FILE_IO,
            "Could not write initial generated C under: %s", directory);
    if(!editor_workspace_main_write(&created, project, true)) return editor_result_error(
        EDITOR_ERROR_FILE_IO, "Could not write initial main.c under: %s", directory);
    if(!editor_workspace_scene_write(&created, project, NULL)) return editor_result_error(
        EDITOR_ERROR_FILE_IO, "Could not write scene build scaffold under: %s", directory);
    *workspace = created;
    return editor_result_value(true);
}

EditorResult editor_workspace_load(EditorWorkspace *workspace, EditorProject *project,
    const char *directory) {
    EditorWorkspace loaded = {0};
    static EditorProject loaded_project;
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    char root[EDITOR_WORKSPACE_PATH_MAX];
    EditorResult result;

    if(workspace == NULL || project == NULL || directory == NULL)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "Workspace load received an invalid argument");
    result = editor_workspace_root_find(root, sizeof(root), directory, &loaded);
    if(editor_result_check(result)) return result;
    if(!editor_workspace_path_join(path, sizeof(path), root,
            loaded.config.editor_state_file))
        return editor_result_error(EDITOR_ERROR_FILE_IO,
            "Project editor-state path is too long under: %s", root);
    result = editor_project_load(&loaded_project, path);
    if(editor_result_check(result)) return result;
    *workspace = loaded;
    editor_project_destroy(project);
    *project = loaded_project;
    loaded_project = (EditorProject){0};
    return editor_result_value(true);
}

void editor_workspace_close(EditorWorkspace *workspace, EditorProject *project) {
    if(workspace != NULL) *workspace = (EditorWorkspace){0};
    editor_project_destroy(project);
    editor_project_init(project);
}

EditorResult editor_workspace_command_execute(EditorWorkspace *workspace,
        EditorProject *project, const EditorWorkspaceCommand *command) {
    if(workspace == NULL || project == NULL || command == NULL)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "Workspace command requires a workspace, project, and command");
    switch(command->type) {
        case EDITOR_WORKSPACE_COMMAND_CREATE:
            return editor_workspace_create(workspace, project, command->directory);
        case EDITOR_WORKSPACE_COMMAND_LOAD:
            return editor_workspace_load(workspace, project, command->directory);
        case EDITOR_WORKSPACE_COMMAND_SAVE:
            if(!editor_workspace_save(workspace, project))
                return editor_result_error(EDITOR_ERROR_FILE_IO,
                    "Could not save project workspace: %s", workspace->directory);
            return editor_result_value(true);
        case EDITOR_WORKSPACE_COMMAND_GENERATE_C: {
            EditorResult validation = editor_soft_areas_generation_validate(project);
            if(validation.kind == ERROR_RESULT_ERROR) return validation;
            return editor_workspace_scene_generate(workspace, project, NULL);
        }
    }
    return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "Unknown workspace command");
}

static bool editor_workspace_command_text_append(char *output, size_t capacity,
        size_t *used, const char *text) {
    size_t length = strlen(text);
    if(*used >= capacity || length >= capacity - *used) return false;
    memcpy(output + *used, text, length);
    *used += length;
    output[*used] = '\0';
    return true;
}

static bool editor_workspace_command_shell_append(char *output, size_t capacity,
        size_t *used, const char *text) {
    if(!editor_workspace_command_text_append(output, capacity, used, "'")) return false;
    for(const char *at = text; *at != '\0'; at += 1) {
        char character[2] = {*at, '\0'};
        if(!editor_workspace_command_text_append(output, capacity, used,
                *at == '\'' ? "'\\''" : character)) return false;
    }
    return editor_workspace_command_text_append(output, capacity, used, "'");
}

EditorResult editor_workspace_command_cli_write(const EditorWorkspaceCommand *command,
        char *output, size_t output_capacity) {
    const char *action;
    size_t used = 0;
    if(command == NULL || output == NULL || output_capacity == 0 ||
            command->directory[0] == '\0')
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "Workspace command serialization received an invalid argument");
    if(command->type == EDITOR_WORKSPACE_COMMAND_CREATE) action = "create";
    else if(command->type == EDITOR_WORKSPACE_COMMAND_GENERATE_C) action = "generate-c";
    else return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "Unknown workspace command");
    output[0] = '\0';
    if(!editor_workspace_command_text_append(output, output_capacity, &used,
            "rohr-cli --project ") ||
            !editor_workspace_command_shell_append(output, output_capacity, &used,
                command->directory)) goto capacity_error;
    if(!editor_workspace_command_text_append(output, output_capacity, &used, " ") ||
            !editor_workspace_command_text_append(output, output_capacity, &used,
                action)) goto capacity_error;
    return editor_result_value(true);
capacity_error:
    return editor_result_error(EDITOR_ERROR_CAPACITY,
        "Workspace CLI command output buffer is too small");
}
