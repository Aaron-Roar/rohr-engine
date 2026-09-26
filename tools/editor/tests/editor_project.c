/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_project.h"
#include "editor_viewport.h"
#include "editor_workspace.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static bool position_equal(Position a, Position b) {
    return fabsf(a.x - b.x) < 0.002f && fabsf(a.y - b.y) < 0.002f;
}

static void workspace_fixture_remove(const char *root) {
    char path[2048];
    static const char *files[] = {
        "project.rohr.json", "objects/project.rohr.json", "src/main.c",
        "src/generated/project_objects.c", "src/generated/project_objects.h",
        "src/generated/project_viewports.c", "src/generated/project_viewports.h",
        "CMakeLists.txt", ".gitignore", "editor.lua",
        "assets/tutorial_frame_1.png", "assets/tutorial_frame_2.png"
    };
    static const char *directories[] = {
        "src/generated", "src", "assets", "objects"
    };

    for(size_t i = 0; i < sizeof(files) / sizeof(files[0]); i += 1) {
        snprintf(path, sizeof(path), "%s/%s", root, files[i]);
        (void)SDL_RemovePath(path);
    }
    for(size_t i = 0; i < sizeof(directories) / sizeof(directories[0]); i += 1) {
        snprintf(path, sizeof(path), "%s/%s", root, directories[i]);
        (void)SDL_RemovePath(path);
    }
    (void)SDL_RemovePath(root);
}

static bool file_contains(const char *path, const char *text) {
    char *contents;
    bool found;

    if(path == NULL || text == NULL) return false;
    contents = SDL_LoadFile(path, NULL);
    if(contents == NULL) return false;
    found = strstr(contents, text) != NULL;
    SDL_free(contents);
    return found;
}

static size_t file_occurrence_count(const char *path, const char *text) {
    char *contents;
    char *at;
    size_t count = 0;
    if(path == NULL || text == NULL || text[0] == '\0') return 0;
    contents = SDL_LoadFile(path, NULL);
    if(contents == NULL) return 0;
    at = contents;
    while((at = strstr(at, text)) != NULL) {
        count += 1;
        at += strlen(text);
    }
    SDL_free(contents);
    return count;
}

static bool file_replace(const char *path, const char *text) {
    FILE *file;
    size_t length;
    bool written;

    if(path == NULL || text == NULL) return false;
    file = fopen(path, "wb");
    if(file == NULL) return false;
    length = strlen(text);
    written = fwrite(text, 1, length, file) == length;
    return fclose(file) == 0 && written;
}

static bool file_json_number_replace(const char *path, const char *key,
        const char *value) {
    char needle[128];
    char *contents, *at, *end, *replacement;
    size_t size, prefix_length, suffix_length, value_length;
    bool replaced;
    if(snprintf(needle, sizeof(needle), "\"%s\":", key) < 0) return false;
    contents = SDL_LoadFile(path, &size);
    if(contents == NULL) return false;
    at = strstr(contents, needle);
    if(at == NULL) { SDL_free(contents); return false; }
    at += strlen(needle);
    end = at;
    while(*end != '\0' && *end != ',' && *end != '}') end += 1;
    prefix_length = (size_t)(at - contents);
    suffix_length = size - (size_t)(end - contents);
    value_length = strlen(value);
    replacement = SDL_malloc(prefix_length + value_length + suffix_length + 1);
    if(replacement == NULL) { SDL_free(contents); return false; }
    memcpy(replacement, contents, prefix_length);
    memcpy(replacement + prefix_length, value, value_length);
    memcpy(replacement + prefix_length + value_length, end, suffix_length);
    replacement[prefix_length + value_length + suffix_length] = '\0';
    replaced = file_replace(path, replacement);
    SDL_free(replacement);
    SDL_free(contents);
    return replaced;
}

static bool file_text_replace_first(const char *path, const char *before,
        const char *after) {
    char *contents, *at, *replacement;
    size_t size, prefix_length, before_length, after_length, suffix_length;
    bool replaced;
    if(path == NULL || before == NULL || before[0] == '\0' || after == NULL)
        return false;
    contents = SDL_LoadFile(path, &size);
    if(contents == NULL) return false;
    at = strstr(contents, before);
    if(at == NULL) { SDL_free(contents); return false; }
    before_length = strlen(before);
    after_length = strlen(after);
    prefix_length = (size_t)(at - contents);
    suffix_length = size - prefix_length - before_length;
    replacement = SDL_malloc(prefix_length + after_length + suffix_length + 1);
    if(replacement == NULL) { SDL_free(contents); return false; }
    memcpy(replacement, contents, prefix_length);
    memcpy(replacement + prefix_length, after, after_length);
    memcpy(replacement + prefix_length + after_length, at + before_length,
        suffix_length);
    replacement[prefix_length + after_length + suffix_length] = '\0';
    replaced = file_replace(path, replacement);
    SDL_free(replacement);
    SDL_free(contents);
    return replaced;
}

static bool file_property_line_remove_after(const char *path,
        const char *section, const char *property) {
    char *contents;
    char *at;
    char *line_begin;
    char *line_end;
    bool success;

    if(path == NULL || section == NULL || property == NULL) return false;
    contents = SDL_LoadFile(path, NULL);
    if(contents == NULL) return false;
    at = strstr(contents, section);
    if(at != NULL) at = strstr(at, property);
    if(at == NULL) {
        SDL_free(contents);
        return false;
    }
    line_begin = at;
    while(line_begin > contents && line_begin[-1] != '\n') line_begin -= 1;
    line_end = strchr(at, '\n');
    if(line_end == NULL) line_end = contents + strlen(contents);
    else line_end += 1;
    memmove(line_begin, line_end, strlen(line_end) + 1);
    success = file_replace(path, contents);
    SDL_free(contents);
    return success;
}

int main(void) {
    static EditorProject project;
    EditorObject *object;
    EditorRigidBody *chassis;
    EditorRigidBody *wheel;
    EditorHitbox *hitbox;
    EditorJoint *joint;
    EditorJoint *joint_two;
    EditorAnchor *manual_anchor;
    EditorAnchor *second_anchor;
    EditorAnchorId manual_anchor_id;
    EditorAnchorId second_anchor_id;
    EditorJointId joint_two_id;
    EditorSoftBody *soft_body;
    EditorSoftNode *node_a;
    EditorSoftNode *node_b;
    EditorSoftBeam *beam;
    Position first;
    Position second;
    char formatted[EDITOR_OBJECT_NAME_MAX];

    {
        static EditorProject workspace_project;
        static EditorProject loaded_project;
        EditorWorkspace workspace = {0};
        EditorWorkspace loaded_workspace = {0};
        EditorWorkspaceConfig defaults = editor_workspace_config_default_get();
        EditorWorkspaceCommand workspace_command = {0};
        const char *fixture = "/tmp/rohr_editor_workspace_test";
        SDL_PathInfo info;
        char path[2048];
        char cli_command[4096];
        EditorResult command_result;
        EditorResult create_cli_result;

        workspace_fixture_remove(fixture);
        workspace_command.type = EDITOR_WORKSPACE_COMMAND_CREATE;
        snprintf(workspace_command.directory, sizeof(workspace_command.directory),
            "%s", fixture);
        command_result = editor_workspace_command_execute(
            &workspace, &workspace_project, &workspace_command);
        create_cli_result = editor_workspace_command_cli_write(
            &workspace_command, cli_command, sizeof(cli_command));
        if(editor_result_check(command_result) ||
                editor_result_check(create_cli_result) ||
                strstr(cli_command, "rohr-cli --project ") != cli_command ||
                strstr(cli_command, " create") == NULL) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        {
            EditorGraphicsLayer *hud = editor_project_graphics_layer_add(
                &workspace_project, "hud", 500);
            workspace_project.engine_time_per_tick = 1.0 / 120.0;
            workspace_project.physics_timestep_override = true;
            workspace_project.physics_dt_per_tick = 1.0 / 240.0;
            workspace_project.physics_substeps = 4;
            workspace_project.physics_gravity = (Acceleration){12.0f, 345.0f};
            workspace_project.physics_solver_iterations = 16;
            EditorInputController *gameplay =
                editor_project_input_controller_add(&workspace_project,
                    "gameplay");
            EditorInputAction *move = gameplay == NULL ? NULL :
                editor_project_input_action_add(&workspace_project, gameplay->id,
                    "move", INPUT_ACTION_AXIS_2D);
            EditorInputActionId move_id = move == NULL ? 0 : move->id;
            EditorInputAction *click = gameplay == NULL ? NULL :
                editor_project_input_action_add(&workspace_project, gameplay->id,
                    "click", INPUT_ACTION_BUTTON);
            EditorInputActionId click_id = click == NULL ? 0 : click->id;
            if(hud == NULL || workspace_project.layout_viewport_count == 0 ||
                    workspace_project.layout_viewports[0].ui_item_count == 0 ||
                    gameplay == NULL || move == NULL || click == NULL ||
                    !editor_project_input_controller_set(&workspace_project,
                        gameplay->id, "gameplay", false) ||
                    !editor_project_input_action_set(&workspace_project,
                        gameplay->id, click_id, "click", INPUT_ACTION_BUTTON,
                        INPUT_BUTTON_PERSISTENT, true) ||
                    !editor_project_input_binding_add(&workspace_project,
                        gameplay->id, move_id,
                        (InputBinding){.source = INPUT_BINDING_KEY,
                            .input.key = SDL_SCANCODE_W,
                            .affects_y = true,
                            .scale = {1.0f, 0.5f},
                            .inverted_x = true,
                            .direction = {0.5f, -1.0f}}) ||
                    !editor_project_input_binding_add(&workspace_project,
                        gameplay->id, move_id,
                        (InputBinding){.source = INPUT_BINDING_MOUSE_MOTION,
                            .input.axis_component = INPUT_AXIS_COMPONENT_XY,
                            .modifiers = SDL_KMOD_SHIFT,
                            .affects_x = true, .affects_y = true,
                            .scale = {0.25f, 0.75f},
                            .inverted_x = true}) ||
                    !editor_project_input_binding_add(&workspace_project,
                        gameplay->id, click_id,
                        (InputBinding){.source = INPUT_BINDING_MOUSE_BUTTON,
                            .input.mouse_button = INPUT_MOUSE_BUTTON_LEFT,
                            .scale = {1.0f, 1.0f}})) {
                workspace_fixture_remove(fixture);
                return 1;
            }
            move = editor_project_input_action_get(&workspace_project,
                gameplay->id, move_id);
            click = editor_project_input_action_get(&workspace_project,
                gameplay->id, click_id);
            if(move == NULL || click == NULL ||
                    !editor_project_input_binding_name_set(&workspace_project,
                        gameplay->id, move_id, move->binding_ids[0],
                        "move_up") ||
                    !editor_project_input_binding_name_set(&workspace_project,
                        gameplay->id, click_id, click->binding_ids[0],
                        "primary_click")) {
                workspace_fixture_remove(fixture);
                return 1;
            }
            workspace_project.navigation = (EditorNavigationState){
                .mode = EDITOR_VIEWPORT_INPUT_BINDING,
                .selection = EDITOR_SELECTION_INPUT_BINDING,
                .input_controller = gameplay->id,
                .input_action = click_id,
                .input_binding = click->binding_ids[0]};
            workspace_project.layout_viewports[0].ui_items[0].graphics_layer = hud->id;
            workspace_project.layout_viewports[0].camera_items[0].graphics_layer =
                hud->id;
            EditorViewportUiItem *slider = editor_viewport_ui_add(
                &workspace_project, &workspace_project.layout_viewports[0],
                EDITOR_VIEWPORT_UI_SLIDER);
            EditorViewportUiItem *text;
            EditorViewportUiDefinitionId slider_definition;
            EditorViewportUiDefinitionId text_definition;
            if(slider == NULL) {
                workspace_fixture_remove(fixture);
                return 1;
            }
            slider->position = (Position){320.0f, 240.0f};
            slider->rotation = 0.15f;
            slider->layer = 7;
            slider->value.slider.minimum = -10.0f;
            slider->value.slider.maximum = 30.0f;
            slider->value.slider.value = 12.5f;
            slider->value.slider.step = 0.5f;
            slider->value.slider.length = 240.0f;
            slider->value.slider.thumb_shape = VIEWPORT_UI_SLIDER_THUMB_CIRCLE;
            slider->value.slider.thumb_radius = 14.0f;
            slider->value.slider.thumb_offset = -18.0f;
            if(!editor_project_ui_definition_sync_from_item(
                    &workspace_project, slider->id)) {
                workspace_fixture_remove(fixture);
                return 1;
            }
            slider_definition = slider->definition;
            text = editor_viewport_ui_add(&workspace_project,
                &workspace_project.layout_viewports[0], EDITOR_VIEWPORT_UI_TEXT);
            if(text == NULL) {
                workspace_fixture_remove(fixture);
                return 1;
            }
            snprintf(text->name, sizeof(text->name), "instance_text");
            text->position = (Position){80.0f, 420.0f};
            snprintf(text->value.text.text, sizeof(text->value.text.text),
                "independent text");
            text_definition = text->definition;
            if(!editor_project_ui_definition_sync_from_item(
                    &workspace_project, text->id)) {
                workspace_fixture_remove(fixture);
                return 1;
            }
            workspace_project.objects[0].rigid_bodies[0].graphics_layer.layer = hud->id;
            if(editor_viewport_ui_mount(&workspace_project,
                    &workspace_project.layout_viewports[0],
                    workspace_project.layout_viewports[0].ui_items[0].definition) ==
                    NULL || editor_viewport_ui_mount(&workspace_project,
                    &workspace_project.layout_viewports[0], slider_definition) == NULL ||
                    editor_viewport_ui_mount(&workspace_project,
                    &workspace_project.layout_viewports[0], text_definition) == NULL) {
                workspace_fixture_remove(fixture);
                return 1;
            }
            if(!editor_workspace_save(&workspace, &workspace_project) ||
                    !editor_workspace_c_generate(&workspace, &workspace_project)) {
                workspace_fixture_remove(fixture);
                return 1;
            }
        }
        workspace_command = (EditorWorkspaceCommand){
            .type = EDITOR_WORKSPACE_COMMAND_LOAD};
        snprintf(workspace_command.directory, sizeof(workspace_command.directory),
            "%s", fixture);
        if(editor_result_check(editor_workspace_command_execute(
                    &loaded_workspace, &loaded_project, &workspace_command))) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        EditorViewportUiItem *loaded_slider = NULL;
        for(size_t i = 0; i < loaded_project.layout_viewports[0].ui_item_count; i += 1)
            if(loaded_project.layout_viewports[0].ui_items[i].kind ==
                    EDITOR_VIEWPORT_UI_SLIDER)
                loaded_slider = &loaded_project.layout_viewports[0].ui_items[i];
        if(defaults.format_version != EDITOR_WORKSPACE_FORMAT_VERSION ||
                strcmp(defaults.source_directory, "src") != 0 ||
                strcmp(defaults.generated_directory, "src/generated") != 0 ||
                strcmp(defaults.editor_state_file,
                    "objects/project.rohr.json") != 0 ||
                !loaded_workspace.open || strcmp(loaded_workspace.config.name,
                    "RohrEditorWorkspaceTest") != 0 ||
                loaded_project.object_count != 1 ||
                fabs(loaded_project.engine_time_per_tick - 1.0 / 120.0) > 0.000001 ||
                !loaded_project.physics_timestep_override ||
                fabs(loaded_project.physics_dt_per_tick - 1.0 / 240.0) > 0.000001 ||
                loaded_project.physics_substeps != 4 ||
                fabsf(loaded_project.physics_gravity.x - 12.0f) > 0.001f ||
                fabsf(loaded_project.physics_gravity.y - 345.0f) > 0.001f ||
                loaded_project.physics_solver_iterations != 16 ||
                loaded_project.input_controller_count != 1 ||
                strcmp(loaded_project.input_controllers[0].name,
                    "gameplay") != 0 ||
                loaded_project.input_controllers[0].enabled ||
                loaded_project.input_controllers[0].action_count != 2 ||
                loaded_project.input_controllers[0].actions[0].type !=
                    INPUT_ACTION_AXIS_2D ||
                loaded_project.input_controllers[0].actions[0].binding_count != 2 ||
                strcmp(loaded_project.input_controllers[0].actions[0].
                    bindings[0].name, "move_up") != 0 ||
                loaded_project.input_controllers[0].actions[0].binding_ids[0] ==
                    EDITOR_INPUT_BINDING_INVALID ||
                loaded_project.input_controllers[0].actions[0].bindings[1].source !=
                    INPUT_BINDING_MOUSE_MOTION ||
                loaded_project.input_controllers[0].actions[0].bindings[0].
                    affects_x ||
                !loaded_project.input_controllers[0].actions[0].bindings[0].
                    affects_y ||
                !loaded_project.input_controllers[0].actions[0].bindings[0].
                    inverted_x ||
                fabsf(loaded_project.input_controllers[0].actions[0].bindings[0].
                    direction.x - 0.5f) > 0.001f ||
                !loaded_project.input_controllers[0].actions[0].bindings[1].
                    affects_x ||
                !loaded_project.input_controllers[0].actions[0].bindings[1].
                    affects_y ||
                fabsf(loaded_project.input_controllers[0].actions[0].bindings[1].
                    scale.x - 0.25f) > 0.001f ||
                fabsf(loaded_project.input_controllers[0].actions[0].bindings[1].
                    scale.y - 0.75f) > 0.001f ||
                !loaded_project.input_controllers[0].actions[0].bindings[1].
                    inverted_x ||
                loaded_project.input_controllers[0].actions[0].bindings[1].
                    inverted_y ||
                loaded_project.input_controllers[0].actions[1].button_mode !=
                    INPUT_BUTTON_PERSISTENT ||
                !loaded_project.input_controllers[0].actions[1].
                    button_initial_state ||
                loaded_project.navigation.mode != EDITOR_VIEWPORT_INPUT_BINDING ||
                loaded_project.navigation.selection !=
                    EDITOR_SELECTION_INPUT_BINDING ||
                loaded_project.navigation.input_controller !=
                    loaded_project.input_controllers[0].id ||
                loaded_project.navigation.input_action !=
                    loaded_project.input_controllers[0].actions[1].id ||
                loaded_project.navigation.input_binding !=
                    loaded_project.input_controllers[0].actions[1].binding_ids[0] ||
                strcmp(loaded_project.input_controllers[0].actions[1].
                    bindings[0].name, "primary_click") != 0 ||
                loaded_project.input_controllers[0].actions[1].bindings[0].input.
                    mouse_button != INPUT_MOUSE_BUTTON_LEFT ||
                strcmp(loaded_project.objects[0].name, "Starter") != 0 ||
                !position_equal(loaded_project.objects[0].position,
                    (Position){0.0f, 0.0f}) ||
                loaded_project.objects[0].rigid_body_count != 7 ||
                loaded_project.objects[0].camera_count != 1 ||
                loaded_project.layout_viewport_count != 1 ||
                strcmp(loaded_project.layout_viewports[0].name,
                    "InitialViewport") != 0 ||
                loaded_project.layout_viewports[0].config.rectangle.width !=
                    WINDOW_WIDTH ||
                loaded_project.layout_viewports[0].config.rectangle.height !=
                    WINDOW_HEIGHT ||
                loaded_project.layout_viewports[0].background_color !=
                    0x000000FFu ||
                loaded_project.layout_viewports[0].camera_item_count != 1 ||
                loaded_project.graphics_layer_count != 1 ||
                strcmp(loaded_project.graphics_layers[0].name, "hud") != 0 ||
                loaded_project.graphics_layers[0].value != 500 ||
                loaded_project.layout_viewports[0].ui_item_count != 6 ||
                loaded_project.ui_definition_count != 3 ||
                loaded_project.layout_viewports[0].ui_items[0].definition !=
                    loaded_project.layout_viewports[0].ui_items[3].definition ||
                loaded_project.layout_viewports[0].ui_items[0].graphics_layer !=
                    loaded_project.graphics_layers[0].id ||
                loaded_project.layout_viewports[0].ui_items[0].drag_mode !=
                    VIEWPORT_ITEM_DRAG_X ||
                loaded_project.layout_viewports[0].ui_items[3].drag_mode !=
                    VIEWPORT_ITEM_DRAG_NONE ||
                loaded_slider == NULL ||
                fabsf(loaded_slider->value.slider.value - 12.5f) > 0.001f ||
                loaded_slider->value.slider.thumb_shape !=
                    VIEWPORT_UI_SLIDER_THUMB_CIRCLE ||
                fabsf(loaded_slider->value.slider.thumb_radius - 14.0f) > 0.001f ||
                fabsf(loaded_slider->value.slider.thumb_offset + 18.0f) > 0.001f ||
                loaded_project.layout_viewports[0].camera_items[0].graphics_layer !=
                    loaded_project.graphics_layers[0].id ||
                loaded_project.layout_viewports[0].camera_items[0].placement.
                    drag_mode != VIEWPORT_ITEM_DRAG_Y ||
                loaded_project.objects[0].rigid_bodies[0].graphics_layer.layer !=
                    loaded_project.graphics_layers[0].id ||
                loaded_project.layout_viewports[0].camera_items[0].object !=
                    loaded_project.objects[0].id ||
                loaded_project.layout_viewports[0].camera_items[0].camera !=
                    loaded_project.objects[0].cameras[0].id ||
                strcmp(loaded_project.objects[0].cameras[0].name,
                    "main_camera") != 0 ||
                !position_equal(loaded_project.objects[0].rigid_bodies[0].position,
                    (Position){0.0f, -200.0f}) ||
                !position_equal(loaded_project.objects[0].rigid_bodies[1].position,
                    (Position){0.0f, 120.0f}) ||
                !loaded_project.objects[0].rigid_bodies[0].static_body ||
                !loaded_project.objects[0].rigid_bodies[1].gravity_enabled ||
                fabsf(loaded_project.objects[0].rigid_bodies[1].mass_value - 5.0f) >
                    0.001f ||
                loaded_project.objects[0].rigid_bodies[0].hitboxes[0].vertex_count != 4 ||
                loaded_project.objects[0].rigid_bodies[1].hitboxes[0].vertex_count != 4) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/src/main.c", fixture);
        if(!SDL_GetPathInfo(path, &info) || info.type != SDL_PATHTYPE_FILE ||
                !file_contains(path, "rohr_directory_working_set(") ||
                !file_contains(path, "rohr_directory_base_get()") ||
                file_contains(path, "project_chdir") ||
                file_contains(path, "SDL_GetBasePath()") ||
                !file_contains(path, "project_objects_create_all(&objects") ||
                !file_contains(path, "project_viewports_create(&viewports") ||
                !file_contains(path, "project_viewports_destroy(&viewports") ||
                !file_contains(path, "project_objects_destroy_all(&objects") ||
                !file_contains(path, "rohr_input_frame_begin()") ||
                !file_contains(path,
                    "rohr_input_key_pressed_check(SDL_SCANCODE_ESCAPE)")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/src/generated/project_objects.h", fixture);
        if(!file_contains(path, "typedef struct ProjectControllers") ||
                !file_contains(path, "typedef struct ProjectObjects") ||
                file_occurrence_count(path,
                    "ProjectControllers controllers;") != 1) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/src/generated/project_objects.c", fixture);
        if(!file_contains(path, "rohr_engine_time_per_tick_set(") ||
                !file_contains(path, "rohr_physics_dt_per_tick_set(") ||
                !file_contains(path, "rohr_physics_substeps_set(4)") ||
                !file_contains(path,
                    "rohr_physics_gravity_set((Acceleration){12.000000000f, 345.000000000f})") ||
                !file_contains(path, "rohr_physics_solver_iterations_set(16)") ||
                !file_contains(path, "project_controllers_create") ||
                !file_contains(path,
                    "rohr_input_controller_create(\"gameplay\")") ||
                !file_contains(path,
                    "rohr_input_controller_enabled_set(controllers->controller_gameplay_") ||
                !file_contains(path,
                    "rohr_input_action_create(controllers->controller_gameplay_") ||
                !file_contains(path,
                    "rohr_input_action_button_mode_set(controllers->action_gameplay_click_") ||
                !file_contains(path, "INPUT_BUTTON_PERSISTENT") ||
                !file_contains(path,
                    "rohr_input_action_button_initial_state_set(") ||
                !file_contains(path, "controllers->created = true") ||
                !file_contains(path,
                    "rohr_input_action_bindings_default_set") ||
                !file_contains(path, ".name = \"move_up\"") ||
                !file_contains(path, ".name = \"primary_click\"") ||
                !file_contains(path, "INPUT_BINDING_MOUSE_MOTION") ||
                !file_contains(path, ".modifiers = (SDL_Keymod)3") ||
                !file_contains(path,
                    ".affects_x = false, .affects_y = true") ||
                !file_contains(path,
                    ".affects_x = true, .affects_y = true") ||
                !file_contains(path, ".scale = {0.250000000f, 0.750000000f}") ||
                !file_contains(path, ".inverted_x = true")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/src/generated/project_viewports.c", fixture);
        if(!SDL_GetPathInfo(path, &info) || info.type != SDL_PATHTYPE_FILE ||
                !file_contains(path, "project_objects_draw_all(objects") ||
                !file_contains(path, "rohr_camera_render_callback_set") ||
                !file_contains(path, "rohr_screen_create") ||
                !file_contains(path, "rohr_viewport_screen_add") ||
                !file_contains(path, "rohr_graphics_ui_shape_create") ||
                file_occurrence_count(path,
                    "rohr_graphics_ui_shape_create") != 2 ||
                file_occurrence_count(path, "rohr_graphics_ui_text_create") != 2 ||
                file_occurrence_count(path, "rohr_graphics_ui_slider_create") != 2 ||
                file_occurrence_count(path, "rohr_viewport_ui_add") != 6 ||
                !file_contains(path, "rohr_viewport_ui_add") ||
                !file_contains(path, "rohr_graphics_ui_slider_create") ||
                !file_contains(path, ".minimum=-10.00000000f") ||
                !file_contains(path, ".thumb_shape=1") ||
                !file_contains(path, ".thumb_radius=14.00000000f") ||
                !file_contains(path, ".thumb_offset=-18.00000000f") ||
                !file_contains(path, "rohr_graphics_layer_create(\"hud\", 500)") ||
                !file_contains(path, "rohr_graphics_layer_ui_id_set") ||
                !file_contains(path, "rohr_graphics_layer_entity_id_set") ||
                !file_contains(path, ".drag_mode=1") ||
                !file_contains(path, ".drag_mode=2") ||
                !file_contains(path, ".background_color=") ||
                !file_contains(path, "rohr_graphics_font_default_get") ||
                !file_contains(path, "rohr_graphics_font_release") ||
                file_contains(path, "rohr_graphics_font_destroy") ||
                !file_contains(path, "sample text") ||
                !file_contains(path, "independent text") ||
                !file_contains(path,
                    ".text=resources->texts[resources->text_count - 1]") ||
                file_contains(path,
                    ".text=&resources->texts[resources->text_count - 1]") ||
                file_occurrence_count(path,
                    "resources->ui_items[resources->ui_item_count++]") != 6 ||
                !file_contains(path, "rohr_viewport_enable_set")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/src/main.c", fixture);
        if(!file_replace(path,
                "/* Generated by Rohr Engine. */\n"
                "static void render_scene(void) {}\n"
                "int main(void) { ViewportId viewports[MAX_VIEWPORTS] = {0}; }\n") ||
                !editor_workspace_c_generate(&loaded_workspace, &loaded_project) ||
                !file_contains(path, "project_viewports_create(&viewports") ||
                !file_contains(path, "#include \"project_viewports.h\"")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        if(!file_replace(path, "/* developer-owned main */\n")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/src/generated/project_objects.c", fixture);
        if(!SDL_GetPathInfo(path, &info) || info.type != SDL_PATHTYPE_FILE ||
                !file_contains(path, "EngineResult starter_create") ||
                !file_contains(path,
                    "generated_body_create(&object->box") ||
                !file_contains(path,
                    "const ParticleConfig starter_particle_body_config") ||
                !file_contains(path,
                    "created = rohr_physics_particle_create(config)") ||
                !file_contains(path,
                    ".rigid_vertices = UINT32_C(16)") ||
                !file_contains(path,
                    ".radius = 28.0000000f") ||
                !file_contains(path,
                    "rohr_physics_hitbox_set(*output, hitbox)") ||
                !file_contains(path, "rohr_physics_collision_category_set") ||
                !file_contains(path, "ROHR_COLLISION_CATEGORY_NONE") ||
                !file_contains(path,
                    "rohr_entity_components_add(*output, ROHR_COLLISION)")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        loaded_project.objects[0].rigid_bodies[1].mass_value = 7.0f;
        loaded_project.objects[0].rigid_bodies[1].particle = true;
        {
            EditorObject *generated_object = &loaded_project.objects[0];
            EditorRigidBody *generated_body = &generated_object->rigid_bodies[1];
            EditorRigidBody *generated_particle = NULL;
            for(size_t body_index = 0;
                    body_index < generated_object->rigid_body_count;
                    body_index += 1)
                if(strcmp(generated_object->rigid_bodies[body_index].name,
                        "particle_body") == 0)
                    generated_particle =
                        &generated_object->rigid_bodies[body_index];
            generated_body->initial_velocity = (Velocity){11.0f, 12.0f};
            generated_body->initial_acceleration = (Acceleration){13.0f, 14.0f};
            generated_body->initial_angular_velocity = 15.0f;
            if(generated_particle != NULL) {
                generated_particle->particle_auto_fit = false;
                generated_particle->particle_radius = 9.0f;
                generated_particle->particle_rigid_vertices = 12;
                generated_particle->particle_origin = (Position){3.0f, -4.0f};
                generated_particle->static_body = true;
                generated_particle->particle_fill_color = UINT32_C(0x11223344);
                generated_particle->particle_ring_color = UINT32_C(0x55667788);
                (void)editor_project_particle_hitbox_sync(
                    &loaded_project, generated_particle);
            }
            EditorAnchor *body_anchor = editor_project_anchor_add(&loaded_project,
                generated_object, (Position){12.0f, 0.0f}, generated_body->id);
            EditorAnchor *world_anchor = editor_project_anchor_add(&loaded_project,
                generated_object, (Position){80.0f, 20.0f}, 0);
            EditorJoint *generated_joint = editor_project_joint_add(&loaded_project,
                generated_object, EDITOR_JOINT_SPRING);
            EditorSprite *generated_sprite = editor_project_sprite_add(&loaded_project,
                generated_object, "fly_frame", "assets/fly frame.png");
            EditorAnimatedSprite *generated_animation =
                editor_project_animated_sprite_add(&loaded_project, generated_object);
            EditorSoftBody *generated_soft_body = editor_project_soft_body_add(
                &loaded_project, generated_object);
            EditorCamera *generated_camera = editor_project_camera_add(
                &loaded_project, generated_object);
            if(generated_soft_body != NULL) {
                generated_soft_body->rotation = -450.0f;
                generated_soft_body->initial_velocity = (Velocity){21.0f, 22.0f};
                generated_soft_body->initial_acceleration =
                    (Acceleration){23.0f, 24.0f};
                generated_soft_body->initial_angular_velocity = 810.0f;
            }
            if(generated_camera != NULL) {
                generated_camera->position = (Position){2.5f, -3.25f};
                generated_camera->rotation = 810.0f;
                generated_camera->dimensions = (Scale){1280.0f, 720.0f};
                generated_camera->attachment_kind =
                    EDITOR_CAMERA_ATTACHMENT_RIGID_BODY;
                generated_camera->attachment = generated_body->id;
                generated_camera->inherit_orientation = true;
            }
            EditorSoftNode *generated_node_a = editor_project_soft_node_add(
                &loaded_project, generated_soft_body, (Position){0.0f, 40.0f});
            EditorSoftNode *generated_node_b = editor_project_soft_node_add(
                &loaded_project, generated_soft_body, (Position){20.0f, 40.0f});
            EditorSoftNode *generated_node_c = editor_project_soft_node_add(
                &loaded_project, generated_soft_body, (Position){10.0f, 60.0f});
            EditorSoftNode *generated_node_d = editor_project_soft_node_add(
                &loaded_project, generated_soft_body, (Position){-30.0f, 20.0f});
            EditorSoftNode *generated_node_e = editor_project_soft_node_add(
                &loaded_project, generated_soft_body, (Position){-10.0f, 20.0f});
            EditorSoftNode *generated_node_f = editor_project_soft_node_add(
                &loaded_project, generated_soft_body, (Position){-20.0f, 40.0f});
            EditorSoftBeam *generated_beam = generated_node_a == NULL ||
                generated_node_b == NULL ? NULL : editor_project_soft_beam_add(
                    &loaded_project, generated_soft_body, generated_node_a->id,
                    generated_node_b->id);
            EditorSoftBeam *generated_beam_b = generated_node_b == NULL ||
                generated_node_c == NULL ? NULL : editor_project_soft_beam_add(
                    &loaded_project, generated_soft_body, generated_node_b->id,
                    generated_node_c->id);
            EditorSoftBeam *generated_beam_c = generated_node_c == NULL ||
                generated_node_a == NULL ? NULL : editor_project_soft_beam_add(
                    &loaded_project, generated_soft_body, generated_node_c->id,
                    generated_node_a->id);
            EditorSoftBeam *generated_beam_d = generated_node_d == NULL ||
                generated_node_e == NULL ? NULL : editor_project_soft_beam_add(
                    &loaded_project, generated_soft_body, generated_node_d->id,
                    generated_node_e->id);
            EditorSoftBeam *generated_beam_e = generated_node_e == NULL ||
                generated_node_f == NULL ? NULL : editor_project_soft_beam_add(
                    &loaded_project, generated_soft_body, generated_node_e->id,
                    generated_node_f->id);
            EditorSoftBeam *generated_beam_f = generated_node_f == NULL ||
                generated_node_d == NULL ? NULL : editor_project_soft_beam_add(
                    &loaded_project, generated_soft_body, generated_node_f->id,
                    generated_node_d->id);
            if(generated_node_a != NULL) {
                generated_node_a->radius = 6.5f;
                generated_node_a->friction = 0.6f;
                generated_node_a->restitution = 0.4f;
                generated_node_a->graphics_layer_inherited = false;
                generated_node_a->graphics_layer.value = 41;
                generated_node_a->initial_motion_inherited = false;
                generated_node_a->initial_velocity = (Velocity){31.0f, 32.0f};
                generated_node_a->initial_acceleration =
                    (Acceleration){33.0f, 34.0f};
            }
            if(generated_sprite != NULL) {
                generated_sprite->size = (Scale){32.0f, 24.0f};
                generated_sprite->position = (Position){4.0f, 5.0f};
                generated_sprite->rotation = 810.0f;
                generated_sprite->graphics_layer.value = 44;
            }
            if(generated_animation != NULL && generated_sprite != NULL) {
                generated_animation->rigid_body = generated_object->rigid_bodies[2].id;
                generated_animation->scale = (Scale){2.0f, 3.0f};
                generated_animation->time_per_frame = 0.125;
                generated_animation->editor_position = (Position){6.0f, 7.0f};
                generated_animation->editor_rotation = -450.0f;
                generated_animation->follow_body_rotation = false;
                generated_animation->graphics_layer.value = 45;
                (void)editor_project_animation_frame_add(&loaded_project,
                    generated_animation, "first_frame", "assets/box.png",
                    generated_sprite->size);
                (void)editor_project_hitbox_animation_binding_set(
                    &generated_object->rigid_bodies[2], generated_animation->id,
                    generated_animation->frames[0].id,
                    generated_object->rigid_bodies[2].hitboxes[0].id, true);
            }
            if(generated_beam != NULL) {
                generated_beam->damping = 0.3f;
                generated_beam->collision_enabled = false;
                generated_beam->collision_thickness = 4.0f;
                generated_beam->collision_category = UINT64_C(1);
                generated_beam->collision_with = UINT64_C(1);
                generated_beam->graphics_layer_inherited = false;
                generated_beam->graphics_layer.value = 42;
            }
            if(generated_node_a != NULL) {
                generated_node_a->color = UINT32_C(0xff0000ff);
                generated_node_a->color_overridden = true;
            }
            if(generated_soft_body != NULL && generated_soft_body->area_count == 2) {
                generated_soft_body->areas[0].surface_enabled = false;
                generated_soft_body->areas[1].color = UINT32_C(0x00ff00ff);
                generated_soft_body->areas[1].color_overridden = true;
                generated_soft_body->areas[1].graphics_layer_inherited = false;
                generated_soft_body->areas[1].graphics_layer.value = 43;
            }
            if(body_anchor == NULL || world_anchor == NULL || generated_joint == NULL ||
                    generated_soft_body == NULL || generated_node_a == NULL ||
                    generated_node_b == NULL || generated_node_c == NULL ||
                    generated_node_d == NULL || generated_node_e == NULL ||
                    generated_node_f == NULL ||
                    generated_beam == NULL || generated_beam_b == NULL ||
                    generated_beam_c == NULL || generated_beam_d == NULL ||
                    generated_beam_e == NULL || generated_beam_f == NULL ||
                    generated_soft_body->area_count != 2 ||
                    generated_camera == NULL ||
                    generated_particle == NULL ||
                    generated_sprite == NULL || generated_animation == NULL ||
                    generated_animation->frame_count != 1 ||
                    !editor_project_joint_anchor_set(generated_object,
                        generated_joint, 0, body_anchor->id) ||
                    !editor_project_joint_anchor_set(generated_object,
                        generated_joint, 1, world_anchor->id)) {
                workspace_fixture_remove(fixture);
                return 1;
            }
        }
        workspace_command.type = EDITOR_WORKSPACE_COMMAND_SAVE;
        snprintf(workspace_command.directory, sizeof(workspace_command.directory),
            "%s", fixture);
        if(editor_result_check(editor_workspace_command_execute(
                    &loaded_workspace, &loaded_project, &workspace_command)) ||
                !file_contains(path, "5.00000000f") ||
                file_contains(path, "7.00000000f")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        workspace_command.type = EDITOR_WORKSPACE_COMMAND_GENERATE_C;
        EditorResult generation_result = editor_workspace_command_execute(
            &loaded_workspace, &loaded_project, &workspace_command);
        EditorResult cli_result = editor_workspace_command_cli_write(
            &workspace_command, cli_command, sizeof(cli_command));
        if(editor_result_check(generation_result) || editor_result_check(cli_result) ||
                strstr(cli_command, "rohr-cli --project ") != cli_command ||
                strstr(cli_command, " generate-c") == NULL ||
                !file_contains(path, "7.00000000f") ||
                !file_contains(path,
                    "This generated file is not covered by Rohr Engine's "
                    "LGPL-3.0-only license.") ||
                !file_contains(path, "rohr_physics_particle_origin_set") ||
                !file_contains(path, "rohr_physics_particle_radius_set") ||
                !file_contains(path,
                    ".local_origin = {3.00000000f, -4.00000000f}") ||
                !file_contains(path, ".radius = 9.00000000f") ||
                !file_contains(path, ".rigid_vertices = UINT32_C(12)") ||
                !file_contains(path, ".static_body = true") ||
                !file_contains(path, "UINT32_C(0x11223344)") ||
                !file_contains(path, "UINT32_C(0x55667788)") ||
                !file_contains(path, "generated_world_anchor_create") ||
                !file_contains(path, "rohr_physics_joint_anchor_create") ||
                !file_contains(path, "rohr_camera_create") ||
                !file_contains(path, "rohr_camera_attach") ||
                !file_contains(path, "rohr_physics_joint_spring_set") ||
                !file_contains(path, "rohr_physics_soft_body_create") ||
                !file_contains(path, "rohr_physics_soft_body_node_create") ||
                !file_contains(path, "(Velocity){11.0000000f, 12.0000000f}") ||
                !file_contains(path, "(Acceleration){13.0000000f, 14.0000000f}") ||
                !file_contains(path, "15.0000000f, (Shape)") ||
                !file_contains(path, "(Velocity){31.0000000f, 32.0000000f}") ||
                !file_contains(path, "(Acceleration){33.0000000f, 34.0000000f}") ||
                !file_contains(path, "6.50000000f") ||
                !file_contains(path, "rohr_physics_friction_set") ||
                !file_contains(path, "rohr_physics_restitution_set") ||
                !file_contains(path,
                    "rohr_physics_soft_body_node_collision_filter_set") ||
                !file_contains(path, "rohr_physics_soft_body_beam_create") ||
                !file_contains(path,
                    "rohr_physics_soft_body_beam_collision_config_set") ||
                !file_contains(path,
                    ".enabled = false, .thickness = 4.00000000f") ||
                /* Two starter surfaces plus one enabled new surface. */
                file_occurrence_count(path,
                    "rohr_physics_soft_body_triangle_create") != 3 ||
                !file_contains(path, "rohr_graphics_soft_body_node_color_set") ||
                !file_contains(path, "rohr_graphics_soft_body_area_color_set") ||
                !file_contains(path, "rohr_graphics_animation_load") ||
                !file_contains(path, "rohr_graphics_animation_release") ||
                !file_contains(path, "rohr_graphics_texture_release") ||
                !file_contains(path,
                    "rohr_physics_hitbox_animation_binding_set") ||
                !file_contains(path,
                    "AnimationDescriptor descriptor = {.id = UINT32_C(") ||
                !file_contains(path, ".frame_ids = {UINT32_C(") ||
                !file_contains(path, "animated.player.frame_index = ") ||
                !file_contains(path, "assets/fly frame.png") ||
                !file_contains(path, "animated.follow_entity_rotation = false") ||
                !file_contains(path, "animated.body_offset = (Position){6.00000000f, 7.00000000f}") ||
                !file_contains(path, "animated.orientation_offset = -450.000000f") ||
                !file_contains(path,
                    "(Scale){32.0000000f, 24.0000000f}, true, 810.000000f, true") ||
                !file_contains(path, "(Scale){2.00000000f, 3.00000000f}") ||
                !file_contains(path, "void starter_draw") ||
                !file_contains(path, "EngineResult project_objects_create_all") ||
                !file_contains(path, "void project_objects_draw_all") ||
                !file_contains(path, "rohr_graphics_sprites_draw();") ||
                !file_contains(path, "rohr_graphics_animated_sprites_draw();") ||
                !file_contains(path, "void project_objects_destroy_all")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/src/generated/project_viewports.c", fixture);
        if(!file_contains(path,
                    "rohr_graphics_layer_entity_set(objects->starter.node_") ||
                !file_contains(path,
                    "rohr_graphics_layer_entity_set(objects->starter.beam_") ||
                !file_contains(path, "[0], 43)") ||
                !file_contains(path, "rohr_graphics_layer_sprite_set") ||
                !file_contains(path, "rohr_graphics_layer_animation_set")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/src/generated/project_objects.h", fixture);
        if(!file_contains(path,
                    "This generated file is not covered by Rohr Engine's "
                    "LGPL-3.0-only license.") ||
                !file_contains(path,
                    "extern const ParticleConfig starter_particle_body_config;") ||
                !file_contains(path, "Entity soft_body;") ||
                file_contains(path, "Entity soft_body_soft_body_1;") ||
                !file_contains(path, "typedef struct ProjectObjects") ||
                !file_contains(path, "EngineResult project_objects_create_all")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/src/main.c", fixture);
        if(!file_contains(path, "/* developer-owned main */")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/objects", fixture);
        EditorResult nested_load = editor_workspace_load(
            &loaded_workspace, &loaded_project, path);
        if(editor_result_check(nested_load) ||
                !loaded_workspace.open || loaded_project.object_count != 1 ||
                !position_equal(loaded_project.objects[0].rigid_bodies[1].
                    initial_velocity, (Position){11.0f, 12.0f}) ||
                loaded_project.objects[0].soft_body_count == 0 ||
                !position_equal(loaded_project.objects[0].soft_body_items[1].
                    initial_acceleration, (Position){23.0f, 24.0f}) ||
                loaded_project.objects[0].soft_body_items[1].node_count == 0 ||
                loaded_project.objects[0].soft_body_items[1].nodes[0].
                    initial_motion_inherited ||
                !position_equal(loaded_project.objects[0].soft_body_items[1].
                    nodes[0].initial_velocity, (Position){31.0f, 32.0f})) {
            fprintf(stderr, "nested workspace load failed: %s: %s\n", path,
                nested_load.result.error.message);
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/project.rohr.json", fixture);
        if(editor_result_check(editor_workspace_load(
                    &loaded_workspace, &loaded_project, path)) ||
                !loaded_workspace.open || loaded_project.object_count != 1) {
            fprintf(stderr, "manifest workspace load failed: %s\n", path);
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/CMakeLists.txt", fixture);
        if(!file_contains(path,
                "add_executable(${PROJECT_NAME} src/main.c") ||
                !file_contains(path,
                    "target_link_libraries(${PROJECT_NAME} PRIVATE ${ROHR_ENGINE_TARGET})") ||
                !file_contains(path,
                    "add_custom_target(${PROJECT_NAME}_assets ALL") ||
                !file_contains(path, "${CMAKE_SOURCE_DIR}/assets") ||
                !file_contains(path,
                    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets") ||
                !file_contains(path,
                    "add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_assets)")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        editor_workspace_close(&loaded_workspace, &loaded_project);
        if(loaded_workspace.open || loaded_project.object_count != 0) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        workspace_fixture_remove(fixture);
    }

    editor_project_object_name_format(formatted, sizeof(formatted), "fast car");
    if(strcmp(formatted, "FastCar") != 0) return 1;
    editor_project_object_name_format(formatted, sizeof(formatted), "3d box");
    if(strcmp(formatted, "Object3dBox") != 0) return 1;
    editor_project_property_name_format(formatted, sizeof(formatted), "carBody");
    if(strcmp(formatted, "car_body") != 0) return 1;
    editor_project_property_name_format(formatted, sizeof(formatted), "HTTPServer");
    if(strcmp(formatted, "http_server") != 0) return 1;
    editor_project_property_name_format(formatted, sizeof(formatted), "struct");
    if(strcmp(formatted, "item_struct") != 0) return 1;

    editor_project_init(&project);
    object = editor_project_object_add(&project, (Position){10.0f, 20.0f});
    if(object == NULL || strcmp(object->name, "Object1") != 0 ||
            !object->visible || project.selected != object->id) return 1;
    chassis = editor_project_rigid_body_add(&project, object);
    wheel = editor_project_rigid_body_add(&project, object);
    if(chassis == NULL || wheel == NULL || chassis->id == wheel->id ||
            !chassis->visible || chassis->hitbox_count != 1 ||
            wheel->hitbox_count != 1 || fabsf(chassis->mass_value - 1.0f) > 0.001f ||
            fabsf(chassis->friction - 0.5f) > 0.001f ||
            fabsf(chassis->restitution) > 0.001f || chassis->static_body ||
            chassis->rotation_locked || chassis->gravity_enabled ||
            !chassis->particle_auto_fit) return 1;
    if(project.collision_mask_count != 1 ||
            strcmp(project.collision_masks[0].name, "default") != 0 ||
            !chassis->collision_enabled || chassis->collision_category != UINT64_C(1) ||
            chassis->collision_with != UINT64_C(1)) return 1;
    {
        size_t mask_index = SIZE_MAX;
        if(!editor_project_collision_mask_add(&project, "Enemy", &mask_index) ||
                mask_index != 1 || project.collision_mask_count != 2 ||
                strcmp(project.collision_masks[1].name, "enemy") != 0 ||
                editor_project_collision_mask_add(&project, "enemy", &mask_index) ||
                mask_index != 1 || project.collision_mask_count != 2) return 1;
    }
    chassis->collision_category |= UINT64_C(1) << 1;
    chassis->particle = true;
    chassis->standalone_particle = true;
    editor_project_particle_auto_fit_update(&project);
    if(fabsf(chassis->particle_radius -
            editor_project_particle_auto_radius_get(chassis)) > 0.001f ||
            chassis->particle_radius <= 0.0f) return 1;
    chassis->particle_auto_fit = false;
    chassis->particle_radius = 42.0f;
    chassis->particle_rigid_vertices = 20;
    chassis->particle_origin = (Position){3.0f, -4.0f};
    chassis->particle_ring_color = UINT32_C(0xff8800ff);
    chassis->particle_fill_color = UINT32_C(0x22446680);
    chassis->border_color = UINT32_C(0xabcdef12);
    chassis->surface_color = UINT32_C(0x12345678);
    hitbox = &chassis->hitboxes[0];
    if(hitbox == NULL || !hitbox->visible || hitbox->vertex_count != 3) return 1;
    editor_project_property_name_format(hitbox->vertices[0].name,
        sizeof(hitbox->vertices[0].name), "front Point");
    editor_project_property_name_format(hitbox->line_names[0],
        sizeof(hitbox->line_names[0]), "upperEdge");
    if(strcmp(hitbox->vertices[0].name, "front_point") != 0 ||
            strcmp(hitbox->line_names[0], "upper_edge") != 0) return 1;
    {
        Position local_before = hitbox->vertices[0].position;
        if(!editor_project_rigid_body_origin_set(
                    object, chassis, (Position){5.0f, 7.0f}) ||
                !position_equal(hitbox->vertices[0].position, local_before) ||
                !position_equal(chassis->particle_origin, (Position){3, -4}) ||
                !editor_project_rigid_body_origin_set(
                        object, chassis, (Position){0.0f, 0.0f})) return 1;
    }
    first = hitbox->vertices[0].position;
    second = hitbox->vertices[1].position;
    {
        float original_length = editor_project_hitbox_line_length_get(hitbox, 0);
        if(!editor_project_hitbox_line_length_set(hitbox, 0,
                    ROHR_WORLD_COORDINATE_MAX) ||
                editor_project_hitbox_line_length_set(hitbox, 0,
                    ROHR_WORLD_COORDINATE_MAX + 1.0f) ||
                !editor_project_hitbox_line_length_set(hitbox, 0,
                    original_length)) return 1;
    }
    if(!editor_project_hitbox_vertex_insert(&project, hitbox, 0) ||
            hitbox->vertex_count != 4 ||
            !position_equal(hitbox->vertices[0].position, first) ||
            !position_equal(hitbox->vertices[1].position, (Position){
                (first.x + second.x) * 0.5f, (first.y + second.y) * 0.5f}) ||
            !position_equal(hitbox->vertices[2].position, second) ||
            strcmp(hitbox->line_names[0], "upper_edge") != 0 ||
            strcmp(hitbox->vertices[1].name, "vertex_4") != 0 ||
            !editor_project_hitbox_line_remove(hitbox, 0) ||
            hitbox->vertex_count != 3 ||
            strcmp(hitbox->line_names[0], "upper_edge") != 0) return 1;

    joint = editor_project_joint_add(&project, object, EDITOR_JOINT_SPRING);
    if(joint == NULL || object->anchor_count != 0 || joint->anchor_a != 0 ||
            joint->anchor_b != 0 || fabsf(joint->rest_length) > 0.001f ||
            fabsf(joint->stiffness - 100.0f) > 0.001f ||
            fabsf(joint->damping - 10.0f) > 0.001f ||
            fabsf(joint->visual_size - 1.0f) > 0.001f) return 1;
    joint_two = editor_project_joint_add(&project, object, EDITOR_JOINT_WELD);
    joint_two_id = joint_two == NULL ? 0 : joint_two->id;
    if(joint_two == NULL || joint_two->anchor_a != 0 || joint_two->anchor_b != 0) return 1;
    manual_anchor = editor_project_anchor_add(&project, object,
        (Position){5.0f, 6.0f}, chassis->id);
    second_anchor = editor_project_anchor_add(&project, object,
        (Position){25.0f, 6.0f}, wheel->id);
    manual_anchor_id = manual_anchor == NULL ? 0 : manual_anchor->id;
    second_anchor_id = second_anchor == NULL ? 0 : second_anchor->id;
    if(manual_anchor == NULL || second_anchor == NULL ||
            !editor_project_joint_anchor_set(object, joint, 0, manual_anchor_id) ||
            !editor_project_joint_anchor_set(object, joint, 1, second_anchor_id) ||
            fabsf(joint->rest_length - 20.0f) > 0.001f ||
            !editor_project_joint_remove(object, joint->id) ||
            editor_project_anchor_get(object, manual_anchor_id) == NULL ||
            editor_project_anchor_get(object, second_anchor_id) == NULL) return 1;
    if(!editor_project_joint_anchor_set(object, joint_two, 0, manual_anchor_id) ||
            !editor_project_joint_anchor_set(object, joint_two, 1, second_anchor_id) ||
            !editor_project_joint_remove(object, joint_two_id) ||
            object->anchor_count != 2 ||
            editor_project_anchor_get(object, manual_anchor_id) == NULL ||
            editor_project_anchor_get(object, second_anchor_id) == NULL) return 1;
    manual_anchor = editor_project_anchor_get(object, manual_anchor_id);
    if(manual_anchor == NULL || !editor_project_anchor_position_lock_set(
            object, manual_anchor, false) || !editor_project_anchor_rotation_lock_set(
            object, manual_anchor, false)) return 1;
    chassis->position = (Position){10.0f, 0.0f};
    chassis->rotation = -90.0f;
    if(!editor_project_anchor_position_lock_set(object, manual_anchor, true) ||
            !position_equal(manual_anchor->position, (Position){6.0f, 5.0f}) ||
            !editor_project_anchor_position_lock_set(object, manual_anchor, false) ||
            !position_equal(manual_anchor->position, (Position){5.0f, 6.0f}) ||
            !editor_project_anchor_rotation_lock_set(object, manual_anchor, true) ||
            fabsf(manual_anchor->rotation - 90.0f) > 0.001f ||
            !editor_project_anchor_rotation_lock_set(object, manual_anchor, false) ||
            fabsf(manual_anchor->rotation) > 0.001f) return 1;
    joint = editor_project_joint_add(&project, object, EDITOR_JOINT_SPRING);
    if(joint == NULL || !editor_project_joint_anchor_set(
            object, joint, 0, manual_anchor_id) ||
            !editor_project_joint_anchor_set(object, joint, 1, second_anchor_id) ||
            !editor_project_rigid_body_remove(object, wheel->id) ||
            object->joint_count != 1 ||
            editor_project_anchor_get(object, second_anchor_id) == NULL ||
            editor_project_anchor_get(object, second_anchor_id)->rigid_body != 0 ||
            !editor_project_joint_remove(object, joint->id)) return 1;

    soft_body = editor_project_soft_body_add(&project, object);
    if(soft_body != NULL) {
        soft_body->position = (Position){3.0f, 4.0f};
        soft_body->rotation = -450.0f;
    }
    node_a = editor_project_soft_node_add(&project, soft_body, (Position){0});
    node_b = editor_project_soft_node_add(&project, soft_body, (Position){20.0f, 0.0f});
    if(soft_body == NULL || node_a == NULL || node_b == NULL) return 1;
    node_b->collision_category = UINT64_C(1) | (UINT64_C(1) << 1);
    node_b->collision_with = UINT64_C(1);
    node_b->radius = 7.25f;
    node_b->friction = 0.7f;
    node_b->restitution = 0.35f;
    node_b->graphics_layer_inherited = false;
    node_b->graphics_layer.value = 71;
    {
        float cosine = cosf(math_degrees_to_radians(soft_body->rotation));
        float sine = -sinf(math_degrees_to_radians(soft_body->rotation));
        Position world_before = {
            soft_body->position.x + node_b->position.x * cosine -
                node_b->position.y * sine,
            soft_body->position.y + node_b->position.x * sine +
                node_b->position.y * cosine
        };
        if(!editor_project_soft_body_origin_set(
                    soft_body, (Position){8.0f, 9.0f})) return 1;
        if(!position_equal((Position){soft_body->position.x +
                    node_b->position.x * cosine - node_b->position.y * sine,
                soft_body->position.y + node_b->position.x * sine +
                    node_b->position.y * cosine},
                    (Position){world_before.x + 5, world_before.y + 5}) ||
                !position_equal(node_b->position, (Position){20, 0})) return 1;
    }
    if(node_a->gravity_enabled || node_b->gravity_enabled) return 1;
    beam = editor_project_soft_beam_add(&project, soft_body, 0, 0);
    if(beam == NULL || !beam->visible || beam->node_a != 0 || beam->node_b != 0) return 1;
    beam->node_a = node_a->id;
    beam->node_b = node_b->id;
    if(!editor_project_soft_beam_remove(&project, soft_body, beam->id) ||
            soft_body->node_count != 2) return 1;
    beam = editor_project_soft_beam_add(&project, soft_body, node_a->id, node_b->id);
    if(beam != NULL) {
        beam->damping = 0.45f;
        beam->collision_enabled = false;
        beam->collision_thickness = 3.5f;
        beam->collision_category = UINT64_C(2);
        beam->collision_with = UINT64_C(3);
        beam->graphics_layer_inherited = false;
        beam->graphics_layer.value = 72;
    }
    if(beam == NULL ||
            !editor_project_soft_node_remove(&project, soft_body, node_a->id) ||
            soft_body->beam_count != 1 || beam->node_a != 0 ||
            soft_body->node_count != 1 || beam->node_b != soft_body->nodes[0].id) return 1;

    {
        static EditorProject weld_project;
        EditorObject *weld_object;
        EditorRigidBody *body_a;
        EditorRigidBody *body_b;
        EditorAnchor *anchor_a;
        EditorAnchor *anchor_b;
        EditorJoint *weld;

        editor_project_init(&weld_project);
        weld_object = editor_project_object_add(&weld_project, (Position){0});
        body_a = editor_project_rigid_body_add(&weld_project, weld_object);
        body_b = editor_project_rigid_body_add(&weld_project, weld_object);
        if(body_a == NULL || body_b == NULL) return 1;
        body_a->rotation = 0.25f;
        body_b->rotation = 1.0f;
        anchor_a = editor_project_anchor_add(&weld_project, weld_object,
            (Position){0.0f, 0.0f}, body_a->id);
        anchor_b = editor_project_anchor_add(&weld_project, weld_object,
            (Position){20.0f, 0.0f}, body_b->id);
        weld = editor_project_joint_add(&weld_project, weld_object, EDITOR_JOINT_WELD);
        if(anchor_a == NULL || anchor_b == NULL || weld == NULL ||
                !editor_project_joint_anchor_set(
                    weld_object, weld, 0, anchor_a->id) ||
                !editor_project_joint_anchor_set(
                    weld_object, weld, 1, anchor_b->id) ||
                fabsf(weld->rest_angle - 0.75f) > 0.001f) return 1;
        body_a->rotation = 0.5f;
        editor_project_rigid_body_constraints_apply(weld_object, body_a->id);
        if(fabsf(body_b->rotation - 1.25f) > 0.001f) return 1;
    }

    {
        static EditorProject loaded;
        const char *path = "editor_project_round_trip.json";
        EditorObject *loaded_object;

        if(editor_project_hitbox_add(&project, chassis) == NULL) return 1;
        chassis->active_hitbox_index = 1;

        if(!editor_project_save(&project, path) ||
                editor_result_check(editor_project_load(&loaded, path))) return 1;
        (void)remove(path);
        loaded_object = editor_project_selected_get(&loaded);
        if(loaded_object == NULL || loaded.object_count != project.object_count ||
                loaded_object->id != object->id ||
                loaded_object->rigid_body_count != object->rigid_body_count ||
                loaded_object->anchor_count != object->anchor_count ||
                loaded_object->joint_count != object->joint_count ||
                loaded_object->soft_body_count != object->soft_body_count ||
                loaded_object->hierarchy_count != object->hierarchy_count ||
                memcmp(loaded_object->hierarchy, object->hierarchy,
                    object->hierarchy_count * sizeof(object->hierarchy[0])) != 0 ||
                loaded_object->soft_body_items[0].hierarchy_count !=
                    object->soft_body_items[0].hierarchy_count ||
                memcmp(loaded_object->soft_body_items[0].hierarchy,
                    object->soft_body_items[0].hierarchy,
                    object->soft_body_items[0].hierarchy_count *
                        sizeof(object->soft_body_items[0].hierarchy[0])) != 0 ||
                !position_equal(loaded_object->soft_body_items[0].position,
                    (Position){8.0f, 9.0f}) ||
                fabsf(loaded_object->soft_body_items[0].rotation + 450.0f) > 0.001f ||
                loaded_object->soft_body_items[0].nodes[0].collision_category !=
                    (UINT64_C(1) | (UINT64_C(1) << 1)) ||
                loaded_object->soft_body_items[0].nodes[0].collision_with != UINT64_C(1) ||
                fabsf(loaded_object->soft_body_items[0].nodes[0].radius - 7.25f) >
                    0.001f ||
                fabsf(loaded_object->soft_body_items[0].nodes[0].friction - 0.7f) >
                    0.001f ||
                fabsf(loaded_object->soft_body_items[0].nodes[0].restitution - 0.35f) >
                    0.001f ||
                loaded_object->soft_body_items[0].nodes[0].graphics_layer_inherited ||
                loaded_object->soft_body_items[0].nodes[0].graphics_layer.value != 71 ||
                fabsf(loaded_object->soft_body_items[0].beams[0].damping - 0.45f) >
                    0.001f ||
                loaded_object->soft_body_items[0].beams[0].collision_enabled ||
                fabsf(loaded_object->soft_body_items[0].beams[0].
                    collision_thickness - 3.5f) > 0.001f ||
                loaded_object->soft_body_items[0].beams[0].collision_category !=
                    UINT64_C(2) ||
                loaded_object->soft_body_items[0].beams[0].collision_with !=
                    UINT64_C(3) ||
                loaded_object->soft_body_items[0].beams[0].graphics_layer_inherited ||
                loaded_object->soft_body_items[0].beams[0].graphics_layer.value != 72 ||
                strcmp(loaded_object->name, object->name) != 0 ||
                !position_equal(loaded_object->position, object->position) ||
                loaded.next_id != project.next_id ||
                loaded.next_vertex_id != project.next_vertex_id ||
                loaded.next_rigid_body_id != project.next_rigid_body_id ||
                loaded.next_anchor_id != project.next_anchor_id ||
                loaded.next_soft_node_id != project.next_soft_node_id ||
                loaded.next_soft_beam_id != project.next_soft_beam_id ||
                loaded.collision_mask_count != 2 ||
                strcmp(loaded.collision_masks[1].name, "enemy") != 0 ||
                loaded_object->rigid_bodies[0].collision_with != UINT64_C(1) ||
                loaded_object->rigid_bodies[0].collision_category !=
                    (UINT64_C(1) | (UINT64_C(1) << 1)) ||
                !loaded_object->rigid_bodies[0].particle ||
                !loaded_object->rigid_bodies[0].standalone_particle ||
                loaded_object->rigid_bodies[0].particle_auto_fit ||
                fabsf(loaded_object->rigid_bodies[0].particle_radius - 42.0f) > 0.001f ||
                loaded_object->rigid_bodies[0].particle_rigid_vertices != 20 ||
                !position_equal(loaded_object->rigid_bodies[0].particle_origin,
                    (Position){3.0f, -4.0f}) ||
                loaded_object->rigid_bodies[0].particle_ring_color !=
                    UINT32_C(0xff8800ff) ||
                loaded_object->rigid_bodies[0].particle_fill_color !=
                    UINT32_C(0x22446680) ||
                loaded_object->rigid_bodies[0].border_color != UINT32_C(0xabcdef12) ||
                loaded_object->rigid_bodies[0].surface_color != UINT32_C(0x12345678) ||
                loaded_object->rigid_bodies[0].hitbox_count != 2 ||
                loaded_object->rigid_bodies[0].active_hitbox_index != 1 ||
                strcmp(loaded_object->rigid_bodies[0].hitboxes[0].vertices[0].name,
                    "front_point") != 0 ||
                strcmp(loaded_object->rigid_bodies[0].hitboxes[0].line_names[0],
                    "upper_edge") != 0) return 1;
    }

    {
        static EditorProject topology_project;
        EditorObject *topology_object;
        EditorSoftBody *topology_body;
        EditorSoftBeam *divider;
        EditorSoftNode *nodes[6];
        const Position node_positions[6] = {
            {20.0f, 0.0f}, {10.0f, 17.0f}, {-10.0f, 17.0f},
            {-20.0f, 0.0f}, {-10.0f, -17.0f}, {10.0f, -17.0f}
        };
        uint32_t triangles[EDITOR_SOFT_AREA_NODE_MAX - 2][3];
        editor_project_init(&topology_project);
        topology_object = editor_project_object_add(&topology_project, (Position){0});
        topology_body = editor_project_soft_body_add(&topology_project, topology_object);
        if(topology_object == NULL || topology_body == NULL) return 1;
        for(size_t i = 0; i < 6; i += 1) {
            nodes[i] = editor_project_soft_node_add(
                &topology_project, topology_body, node_positions[i]);
            if(nodes[i] == NULL) return 1;
        }
        for(size_t i = 0; i < 6; i += 1) {
            if(editor_project_soft_beam_add(&topology_project, topology_body,
                    nodes[i]->id, nodes[(i + 1) % 6]->id) == NULL) return 1;
        }
        if(topology_body->area_count != 1 || topology_body->areas[0].node_count != 6 ||
                editor_project_soft_area_triangulate(topology_body,
                    &topology_body->areas[0], triangles,
                    EDITOR_SOFT_AREA_NODE_MAX - 2) != 4) return 1;
        {
            static EditorProject topology_loaded;
            const char *path = "editor_soft_area_layer_round_trip.json";
            topology_body->areas[0].graphics_layer_inherited = false;
            topology_body->areas[0].graphics_layer.value = 73;
            topology_body->areas[0].surface_enabled = false;
            if(!editor_project_save(&topology_project, path) ||
                    editor_result_check(editor_project_load(&topology_loaded, path)))
                return 1;
            (void)remove(path);
            if(topology_loaded.objects[0].soft_body_items[0].areas[0].
                        graphics_layer_inherited ||
                    topology_loaded.objects[0].soft_body_items[0].areas[0].
                        graphics_layer.value != 73) return 1;
            if(topology_loaded.objects[0].soft_body_items[0].areas[0].
                    surface_enabled) return 1;
        }
        divider = editor_project_soft_beam_add(&topology_project, topology_body,
            nodes[0]->id, nodes[3]->id);
        if(divider == NULL ||
                topology_body->area_count != 2 ||
                topology_body->areas[0].node_count != 4 ||
                topology_body->areas[1].node_count != 4 ||
                topology_body->areas[0].surface_enabled ||
                topology_body->areas[1].surface_enabled ||
                topology_body->areas[0].graphics_layer_inherited ||
                topology_body->areas[1].graphics_layer_inherited ||
                topology_body->areas[0].graphics_layer.value != 73 ||
                topology_body->areas[1].graphics_layer.value != 73) return 1;
        if(!editor_project_soft_beam_remove(
                    &topology_project, topology_body, divider->id) ||
                topology_body->area_count != 1 ||
                topology_body->areas[0].surface_enabled ||
                topology_body->areas[0].graphics_layer_inherited ||
                topology_body->areas[0].graphics_layer.value != 73) return 1;
    }

    {
        static EditorProject concave_project;
        EditorObject *concave_object;
        EditorSoftBody *concave_body;
        EditorSoftNode *nodes[5];
        const Position node_positions[5] = {
            {-30.0f, -20.0f}, {30.0f, -20.0f}, {5.0f, 0.0f},
            {30.0f, 20.0f}, {-30.0f, 20.0f}
        };
        uint32_t triangles[EDITOR_SOFT_AREA_NODE_MAX - 2][3];

        editor_project_init(&concave_project);
        concave_object = editor_project_object_add(&concave_project, (Position){0});
        concave_body = editor_project_soft_body_add(&concave_project, concave_object);
        if(concave_object == NULL || concave_body == NULL) return 1;
        for(size_t i = 0; i < 5; i += 1) {
            nodes[i] = editor_project_soft_node_add(
                &concave_project, concave_body, node_positions[i]);
            if(nodes[i] == NULL) return 1;
        }
        for(size_t i = 0; i < 5; i += 1) {
            if(editor_project_soft_beam_add(&concave_project, concave_body,
                    nodes[i]->id, nodes[(i + 1) % 5]->id) == NULL) return 1;
        }
        if(concave_body->area_count != 1 || concave_body->areas[0].node_count != 5 ||
                editor_project_soft_area_triangulate(concave_body,
                    &concave_body->areas[0], triangles,
                    EDITOR_SOFT_AREA_NODE_MAX - 2) != 3) return 1;
    }

    {
        static EditorProject disconnected_project;
        EditorObject *disconnected_object;
        EditorSoftBody *disconnected_body;
        EditorSoftNode *nodes[6];
        const Position node_positions[6] = {
            {-50.0f, -10.0f}, {-30.0f, -10.0f}, {-40.0f, 10.0f},
            {30.0f, -10.0f}, {50.0f, -10.0f}, {40.0f, 10.0f}
        };

        editor_project_init(&disconnected_project);
        disconnected_object = editor_project_object_add(
            &disconnected_project, (Position){0});
        disconnected_body = editor_project_soft_body_add(
            &disconnected_project, disconnected_object);
        if(disconnected_object == NULL || disconnected_body == NULL) return 1;
        for(size_t i = 0; i < 6; i += 1) {
            nodes[i] = editor_project_soft_node_add(
                &disconnected_project, disconnected_body, node_positions[i]);
            if(nodes[i] == NULL) return 1;
        }
        for(size_t triangle = 0; triangle < 2; triangle += 1) {
            size_t first_node = triangle * 3;
            for(size_t edge = 0; edge < 3; edge += 1) {
                if(editor_project_soft_beam_add(&disconnected_project,
                        disconnected_body, nodes[first_node + edge]->id,
                        nodes[first_node + (edge + 1) % 3]->id) == NULL) return 1;
            }
        }
        if(disconnected_body->area_count != 2 ||
                disconnected_body->areas[0].node_count != 3 ||
                disconnected_body->areas[1].node_count != 3) return 1;
    }

    {
        static EditorProject nested_project;
        EditorObject *nested_object;
        EditorSoftBody *nested_body;
        EditorSoftNode *nodes[8];
        const Position node_positions[8] = {
            {-40.0f, -40.0f}, {40.0f, -40.0f}, {40.0f, 40.0f}, {-40.0f, 40.0f},
            {-10.0f, -10.0f}, {10.0f, -10.0f}, {10.0f, 10.0f}, {-10.0f, 10.0f}
        };

        editor_project_init(&nested_project);
        nested_object = editor_project_object_add(&nested_project, (Position){0});
        nested_body = editor_project_soft_body_add(&nested_project, nested_object);
        if(nested_object == NULL || nested_body == NULL) return 1;
        for(size_t i = 0; i < 8; i += 1) {
            nodes[i] = editor_project_soft_node_add(
                &nested_project, nested_body, node_positions[i]);
            if(nodes[i] == NULL) return 1;
        }
        for(size_t loop = 0; loop < 2; loop += 1) {
            size_t first_node = loop * 4;
            for(size_t edge = 0; edge < 4; edge += 1) {
                if(editor_project_soft_beam_add(&nested_project, nested_body,
                        nodes[first_node + edge]->id,
                        nodes[first_node + (edge + 1) % 4]->id) == NULL) return 1;
            }
        }
        if(nested_body->area_count != 2 || nested_body->areas[0].node_count != 4 ||
                nested_body->areas[1].node_count != 4) return 1;
    }

    {
        static EditorProject invalid_version_project;
        const char *path = "editor_project_invalid_version.json";
        EditorResult result;

        if(!file_replace(path, "{\"format_version\":99}")) return 1;
        result = editor_project_load(&invalid_version_project, path);
        (void)remove(path);
        if(!editor_result_check(result) ||
                result.result.error.code != EDITOR_ERROR_SCHEMA_VERSION ||
                strstr(result.result.error.message, "format_version 99") == NULL ||
                strstr(result.result.error.message, "requires 4") == NULL) return 1;
    }

    {
        static EditorProject invalid_project;
        EditorObject *invalid_object;
        EditorSoftBody *invalid_body;
        EditorSoftNode *nodes[4];
        const Position node_positions[4] = {
            {-20.0f, 0.0f}, {0.0f, 20.0f}, {20.0f, 0.0f}, {0.0f, -20.0f}
        };

        editor_project_init(&invalid_project);
        invalid_object = editor_project_object_add(&invalid_project, (Position){0});
        invalid_body = editor_project_soft_body_add(&invalid_project, invalid_object);
        if(invalid_object == NULL || invalid_body == NULL) return 1;
        for(size_t i = 0; i < 4; i += 1) {
            nodes[i] = editor_project_soft_node_add(
                &invalid_project, invalid_body, node_positions[i]);
            if(nodes[i] == NULL) return 1;
        }
        for(size_t i = 0; i < 3; i += 1) {
            if(editor_project_soft_beam_add(&invalid_project, invalid_body,
                    nodes[i]->id, nodes[i + 1]->id) == NULL) return 1;
        }
        if(invalid_body->area_count != 0 ||
                editor_project_soft_beam_add(&invalid_project, invalid_body,
                    nodes[3]->id, 0) == NULL || invalid_body->area_count != 0)
            return 1;
    }

    {
        static EditorProject invalid_project;
        const char *path = "editor_project_invalid.json";
        EditorResult result;

        if(!file_replace(path, "{not valid json")) return 1;
        result = editor_project_load(&invalid_project, path);
        if(!editor_result_check(result) ||
                result.result.error.code != EDITOR_ERROR_JSON_PARSE ||
                strstr(result.result.error.message, "byte") == NULL) return 1;
        if(!file_replace(path, "{\"objects\":[]}")) return 1;
        result = editor_project_load(&invalid_project, path);
        (void)remove(path);
        if(!editor_result_check(result) ||
                result.result.error.code != EDITOR_ERROR_SCHEMA_VERSION ||
                strstr(result.result.error.message, "missing integer format_version") == NULL)
            return 1;
    }

    {
        static EditorProject malformed_project;
        static EditorProject loaded_project;
        static const struct { const char *key; const char *value; } cases[] = {
            {"engine_time_per_tick", "0"},
            {"engine_time_per_tick", "-0.01"},
            {"dt_per_tick", "0"},
            {"dt_per_tick", "-0.01"},
            {"substeps", "0"},
            {"substeps", "-1"},
            {"solver_iterations", "0"},
            {"solver_iterations", "-1"}
        };
        const char *path = "editor_project_invalid_physics.json";
        editor_project_init(&malformed_project);
        for(size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i += 1) {
            EditorResult result;
            if(!editor_project_save(&malformed_project, path) ||
                    !file_json_number_replace(path, cases[i].key, cases[i].value))
                return 1;
            result = editor_project_load(&loaded_project, path);
            if(!editor_result_check(result)) return 1;
        }
        (void)remove(path);
        editor_project_destroy(&loaded_project);
        editor_project_destroy(&malformed_project);
    }

    {
        static EditorProject input_project;
        static EditorProject ignored_project;
        EditorInputController *controller;
        EditorInputAction *action;
        const char *path = "editor_project_invalid_input.json";
        EditorResult result;

        editor_project_init(&input_project);
        controller = editor_project_input_controller_add(&input_project, "gameplay");
        action = controller == NULL ? NULL : editor_project_input_action_add(
            &input_project, controller->id, "jump", INPUT_ACTION_BUTTON);
        if(action == NULL || !editor_project_input_binding_add(
                &input_project, controller->id, action->id,
                (InputBinding){.source = INPUT_BINDING_KEY,
                    .input.key = SDL_SCANCODE_SPACE,
                    .scale = {1.0f, 1.0f}}) ||
                !editor_project_save(&input_project, path) ||
                !file_text_replace_first(path, "\"source\": \"key\"",
                    "\"source\": \"mouse_motion\"")) return 1;
        result = editor_project_load(&ignored_project, path);
        (void)remove(path);
        editor_project_destroy(&input_project);
        if(!editor_result_check(result) ||
                result.result.error.code != EDITOR_ERROR_SCHEMA_INVALID)
            return 1;
    }

    {
        static EditorProject input_project;
        static EditorProject loaded_project;
        EditorInputController *controller;
        EditorInputAction *action;
        const char *path = "editor_project_legacy_input.json";
        EditorResult result;

        editor_project_init(&input_project);
        controller = editor_project_input_controller_add(&input_project,
            "gameplay");
        action = controller == NULL ? NULL : editor_project_input_action_add(
            &input_project, controller->id, "move", INPUT_ACTION_AXIS_2D);
        if(action == NULL || !editor_project_input_binding_add(
                &input_project, controller->id, action->id,
                (InputBinding){.source = INPUT_BINDING_KEY,
                    .input.key = SDL_SCANCODE_W, .affects_y = true,
                    .scale = {2.0f, 2.0f},
                    .inverted_x = true, .inverted_y = true,
                    .direction = {0.0f, -1.0f}}) ||
                !editor_project_save(&input_project, path) ||
                !file_text_replace_first(path,
                    "\"affects_x\": false,\n                            \"affects_y\": true,\n                            \"scale_x\": 2.0,\n                            \"scale_y\": 2.0,\n                            \"inverted_x\": true,\n                            \"inverted_y\": true,",
                    "\"scale\": 2.0,\n                            \"inverted\": true,"))
            return 1;
        result = editor_project_load(&loaded_project, path);
        (void)remove(path);
        editor_project_destroy(&input_project);
        if(editor_result_check(result) ||
                loaded_project.input_controller_count != 1 ||
                loaded_project.input_controllers[0].action_count != 1 ||
                loaded_project.input_controllers[0].actions[0].binding_count !=
                    1 ||
                loaded_project.input_controllers[0].actions[0].bindings[0].
                    scale.x != 2.0f ||
                loaded_project.input_controllers[0].actions[0].bindings[0].
                    scale.y != 2.0f ||
                loaded_project.input_controllers[0].actions[0].bindings[0].
                    affects_x ||
                !loaded_project.input_controllers[0].actions[0].bindings[0].
                    affects_y ||
                !loaded_project.input_controllers[0].actions[0].bindings[0].
                    inverted_x ||
                !loaded_project.input_controllers[0].actions[0].bindings[0].
                    inverted_y)
            return 1;
        editor_project_destroy(&loaded_project);
    }

    {
        static EditorProject input_project;
        static EditorProject ignored_project;
        EditorInputController *controller;
        EditorInputAction *action;
        const char *path = "editor_project_invalid_button_mode.json";
        EditorResult result;

        editor_project_init(&input_project);
        controller = editor_project_input_controller_add(&input_project, "ui");
        action = controller == NULL ? NULL : editor_project_input_action_add(
            &input_project, controller->id, "console", INPUT_ACTION_BUTTON);
        if(action == NULL || !editor_project_input_action_set(&input_project,
                controller->id, action->id, "console", INPUT_ACTION_BUTTON,
                INPUT_BUTTON_PERSISTENT, true) ||
                !editor_project_save(&input_project, path) ||
                !file_text_replace_first(path,
                    "\"button_mode\": \"persistent\"",
                    "\"button_mode\": \"momentary\"")) return 1;
        result = editor_project_load(&ignored_project, path);
        if(!editor_result_check(result) ||
                result.result.error.code != EDITOR_ERROR_SCHEMA_INVALID)
            return 1;
        if(!editor_project_save(&input_project, path) ||
                !file_text_replace_first(path,
                    "\"button_mode\": \"persistent\"",
                    "\"button_mode\": \"invalid\"")) return 1;
        result = editor_project_load(&ignored_project, path);
        (void)remove(path);
        editor_project_destroy(&input_project);
        editor_project_destroy(&ignored_project);
        if(!editor_result_check(result) ||
                result.result.error.code != EDITOR_ERROR_SCHEMA_INVALID)
            return 1;
    }

    {
        static EditorProject reference_project;
        static EditorProject ignored_project;
        EditorObject *reference_object;
        EditorRigidBody *reference_body;
        const char *path = "editor_project_invalid_reference.json";
        EditorResult result;

        editor_project_init(&reference_project);
        reference_object = editor_project_object_add(
            &reference_project, (Position){0});
        if(reference_object == NULL) return 1;
        reference_body = editor_project_rigid_body_add(
            &reference_project, reference_object);
        if(reference_body == NULL) return 1;
        reference_body->collision_category = UINT64_C(1) << 4;
        if(!editor_project_save(&reference_project, path)) return 1;
        result = editor_project_load(&ignored_project, path);
        (void)remove(path);
        if(!editor_result_check(result) ||
                result.result.error.code != EDITOR_ERROR_REFERENCE_INVALID ||
                strstr(result.result.error.message, "collision-mask reference") == NULL)
            return 1;
    }

    {
        static EditorProject manifest_project;
        EditorWorkspace manifest_workspace = {0};
        const char *fixture = "/tmp/rohr_editor_manifest_error_test";
        char path[2048];
        EditorResult result;

        workspace_fixture_remove(fixture);
        if(!SDL_CreateDirectory(fixture)) return 1;
        snprintf(path, sizeof(path), "%s/project.rohr.json", fixture);
        if(!file_replace(path,
                "{\"format_version\":1,\"editor_state_file\":"
                "\"objects/project.rohr.json\"}")) return 1;
        result = editor_workspace_load(&manifest_workspace, &manifest_project, fixture);
        workspace_fixture_remove(fixture);
        if(!editor_result_check(result) ||
                result.result.error.code != EDITOR_ERROR_SCHEMA_INVALID ||
                strstr(result.result.error.message, "field 'name'") == NULL) return 1;
    }

    {
        static EditorProject creation_project;
        EditorWorkspace creation_workspace = {0};
        const char *fixture = "/tmp/rohr_editor_creation_error_test";
        char path[2048];
        EditorResult result;

        workspace_fixture_remove(fixture);
        if(!SDL_CreateDirectory(fixture)) return 1;
        snprintf(path, sizeof(path), "%s/project.rohr.json", fixture);
        if(!file_replace(path, "occupied")) return 1;
        result = editor_workspace_create(
            &creation_workspace, &creation_project, fixture);
        workspace_fixture_remove(fixture);
        if(!editor_result_check(result) || result.result.error.code != EDITOR_ERROR_FILE_IO ||
                strstr(result.result.error.message, "not empty") == NULL) return 1;
    }

    {
        static EditorProject creation_project;
        static EditorProject reloaded_project;
        EditorWorkspace creation_workspace = {0};
        EditorWorkspace reloaded_workspace = {0};
        const char *fixture = "/tmp/rohr_editor_config_creation_test";
        char path[2048];
        SDL_PathInfo info;
        EditorResult result;

        workspace_fixture_remove(fixture);
        result = editor_workspace_create(&creation_workspace, &creation_project, fixture);
        snprintf(path, sizeof(path), "%s/editor.lua", fixture);
        if(editor_result_check(result) || creation_project.object_count != 1 ||
                creation_project.objects[0].rigid_body_count != 7 ||
                creation_project.objects[0].joint_count != 3 ||
                creation_project.objects[0].anchor_count != 7 ||
                creation_project.objects[0].soft_body_count != 1 ||
                creation_project.objects[0].sprite_count != 1 ||
                creation_project.objects[0].animated_sprite_count != 2 ||
                creation_project.objects[0].camera_count != 1 ||
                creation_project.objects[0].animated_sprite_items[0].rigid_body == 0 ||
                creation_project.objects[0].animated_sprite_items[1].rigid_body != 0 ||
                !SDL_GetPathInfo(path, &info) ||
                info.type != SDL_PATHTYPE_FILE) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/assets/tutorial_frame_1.png", fixture);
        if(!SDL_GetPathInfo(path, &info) || info.type != SDL_PATHTYPE_FILE) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        snprintf(path, sizeof(path), "%s/src/generated/project_objects.h", fixture);
        if(!file_contains(path, "Entity animation_free_animation;")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        {
            EditorObject *generated_object = &creation_project.objects[0];
            EditorSoftBody *generated_soft = &generated_object->soft_body_items[0];
            EditorAnchor *generated_anchor = &generated_object->anchors[0];
            if(generated_soft->node_count == 0 ||
                    !editor_project_anchor_soft_node_set(generated_object,
                        generated_anchor, generated_soft->id,
                        generated_soft->nodes[0].id) ||
                    !editor_workspace_c_generate(&creation_workspace,
                        &creation_project)) {
                workspace_fixture_remove(fixture);
                return 1;
            }
            snprintf(path, sizeof(path), "%s/src/generated/project_objects.c", fixture);
            {
                char expected[EDITOR_OBJECT_NAME_MAX + 64];
                snprintf(expected, sizeof(expected),
                    "rohr_physics_joint_anchor_create(object->%s",
                    generated_soft->nodes[0].name);
                if(!file_contains(path, expected)) {
                    return 1;
                }
            }
        }
        result = editor_workspace_load(&reloaded_workspace, &reloaded_project, fixture);
        if(editor_result_check(result) || reloaded_project.object_count != 1 ||
                reloaded_project.objects[0].rigid_body_count != 7 ||
                reloaded_project.objects[0].joint_count != 3 ||
                reloaded_project.objects[0].anchor_count != 7 ||
                reloaded_project.objects[0].soft_body_count != 1 ||
                reloaded_project.objects[0].sprite_count != 1 ||
                reloaded_project.objects[0].animated_sprite_count != 2 ||
                reloaded_project.objects[0].camera_count != 1 ||
                reloaded_project.objects[0].animated_sprite_items[0].rigid_body == 0 ||
                reloaded_project.objects[0].animated_sprite_items[1].rigid_body != 0) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        editor_project_destroy(&reloaded_project);
        snprintf(path, sizeof(path), "%s/objects/project.rohr.json", fixture);
        if(!file_property_line_remove_after(path, "\"sprites\"", "\"rotation\"") ||
                !file_property_line_remove_after(path, "\"animated_sprites\"",
                    "\"editor_rotation\"")) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        result = editor_workspace_load(&reloaded_workspace, &reloaded_project, fixture);
        if(editor_result_check(result) || reloaded_project.object_count != 1 ||
                reloaded_project.objects[0].sprite_count != 1 ||
                reloaded_project.objects[0].sprites[0].rotation != 0.0f ||
                reloaded_project.objects[0].animated_sprite_count != 2 ||
                reloaded_project.objects[0].animated_sprite_items[0].editor_rotation !=
                    0.0f) {
            workspace_fixture_remove(fixture);
            return 1;
        }
        editor_project_destroy(&reloaded_project);
        editor_project_destroy(&creation_project);
        workspace_fixture_remove(fixture);
    }

    {
        EditorProject dynamic_project;
        EditorObject *dynamic_object;
        editor_project_init(&dynamic_project);
        dynamic_object = editor_project_object_add(&dynamic_project, (Position){0});
        if(dynamic_object == NULL) return 1;
        for(size_t i = 0; i < EDITOR_RIGID_BODY_MAX + 1; i += 1)
            if(editor_project_rigid_body_add(&dynamic_project,
                    dynamic_object) == NULL) return 1;
        for(size_t i = 0; i < EDITOR_SOFT_BODY_MAX + 1; i += 1)
            if(editor_project_soft_body_add(&dynamic_project,
                    dynamic_object) == NULL) return 1;
        for(size_t i = 0; i < EDITOR_SOFT_NODE_MAX + 1; i += 1)
            if(editor_project_soft_node_add(&dynamic_project,
                    &dynamic_object->soft_body_items[0],
                    (Position){(float)i, 0.0f}) == NULL) return 1;
        if(dynamic_object->rigid_body_count != EDITOR_RIGID_BODY_MAX + 1 ||
                dynamic_object->soft_body_count != EDITOR_SOFT_BODY_MAX + 1 ||
                dynamic_object->soft_body_items[0].node_count !=
                    EDITOR_SOFT_NODE_MAX + 1) return 1;
        editor_project_destroy(&dynamic_project);
    }

    {
        const char *camera_path = "/tmp/rohr_editor_camera_roundtrip.json";
        EditorProject camera_project, loaded_camera_project;
        EditorObject *camera_object;
        EditorCamera *camera;
        editor_project_init(&camera_project);
        editor_project_init(&loaded_camera_project);
        camera_object = editor_project_object_add(&camera_project, (Position){3, 4});
        camera = editor_project_camera_add(&camera_project, camera_object);
        if(camera == NULL) return 1;
        camera->position = (Position){12.123456f, -9.654321f};
        camera->rotation = 810.0f;
        camera->dimensions = (Scale){1920, 1080};
        if(!editor_project_save(&camera_project, camera_path) ||
                editor_result_check(editor_project_load(&loaded_camera_project,
                    camera_path)) || loaded_camera_project.object_count != 1 ||
                loaded_camera_project.objects[0].camera_count != 1 ||
                loaded_camera_project.objects[0].cameras[0].dimensions.x != 1920 ||
                loaded_camera_project.objects[0].cameras[0].rotation != 810.0f)
            return 1;
        editor_project_destroy(&camera_project);
        editor_project_destroy(&loaded_camera_project);
        (void)remove(camera_path);
    }

    {
        const char *anchor_path = "/tmp/rohr_editor_soft_node_anchor.json";
        EditorProject anchor_project, loaded_anchor_project;
        EditorObject *anchor_object;
        EditorSoftBody *soft_body;
        EditorSoftNode *soft_node;
        EditorAnchor *anchor;
        editor_project_init(&anchor_project);
        editor_project_init(&loaded_anchor_project);
        anchor_object = editor_project_object_add(&anchor_project, (Position){0});
        soft_body = editor_project_soft_body_add(&anchor_project, anchor_object);
        soft_node = editor_project_soft_node_add(&anchor_project, soft_body,
            (Position){12.0f, 34.0f});
        anchor = editor_project_anchor_add(&anchor_project, anchor_object,
            (Position){0}, 0);
        if(anchor == NULL || soft_node == NULL ||
                !editor_project_anchor_soft_node_set(anchor_object, anchor,
                    soft_body->id, soft_node->id) ||
                !editor_project_save(&anchor_project, anchor_path) ||
                editor_result_check(editor_project_load(&loaded_anchor_project,
                    anchor_path)) || loaded_anchor_project.object_count != 1 ||
                loaded_anchor_project.objects[0].anchor_count != 1 ||
                loaded_anchor_project.objects[0].anchors[0].attachment_kind !=
                    EDITOR_ANCHOR_ATTACHMENT_SOFT_NODE ||
                loaded_anchor_project.objects[0].anchors[0].attachment_soft_node !=
                    soft_node->id) return 1;
        editor_project_destroy(&anchor_project);
        editor_project_destroy(&loaded_anchor_project);
        (void)remove(anchor_path);
    }

    {
        const char *hierarchy_path = "/tmp/rohr_editor_project_hierarchy.json";
        EditorProject hierarchy_project, loaded_hierarchy_project;
        EditorObject *hierarchy_object;
        EditorLayoutViewport *hierarchy_viewport;
        EditorInputController *hierarchy_controller;
        editor_project_init(&hierarchy_project);
        editor_project_init(&loaded_hierarchy_project);
        hierarchy_object = editor_project_object_add(&hierarchy_project,
            (Position){0});
        hierarchy_viewport = editor_project_layout_viewport_add(&hierarchy_project);
        hierarchy_controller = editor_project_input_controller_add(
            &hierarchy_project, "gameplay");
        if(hierarchy_object == NULL || hierarchy_viewport == NULL ||
                hierarchy_controller == NULL ||
                hierarchy_project.hierarchy_count != 3) return 1;
        hierarchy_project.hierarchy[0] = (EditorProjectHierarchyItem){
            EDITOR_PROJECT_HIERARCHY_INPUT_CONTROLLER,
            hierarchy_controller->id};
        hierarchy_project.hierarchy[1] = (EditorProjectHierarchyItem){
            EDITOR_PROJECT_HIERARCHY_VIEWPORT, hierarchy_viewport->id};
        hierarchy_project.hierarchy[2] = (EditorProjectHierarchyItem){
            EDITOR_PROJECT_HIERARCHY_OBJECT, hierarchy_object->id};
        hierarchy_viewport->background_color = 0x12345678u;
        hierarchy_viewport->overview_position = (Position){91.0f, -27.0f};
        hierarchy_object->overview_position = (Position){-42.0f, 63.0f};
        hierarchy_object->visible = false;
        if(!hierarchy_viewport->enabled ||
                !editor_project_save(&hierarchy_project, hierarchy_path) ||
                editor_result_check(editor_project_load(&loaded_hierarchy_project,
                    hierarchy_path)) || loaded_hierarchy_project.hierarchy_count != 3 ||
                loaded_hierarchy_project.hierarchy[0].kind !=
                    EDITOR_PROJECT_HIERARCHY_INPUT_CONTROLLER ||
                loaded_hierarchy_project.hierarchy[0].id !=
                    hierarchy_controller->id ||
                loaded_hierarchy_project.hierarchy[1].kind !=
                    EDITOR_PROJECT_HIERARCHY_VIEWPORT ||
                loaded_hierarchy_project.hierarchy[1].id != hierarchy_viewport->id ||
                loaded_hierarchy_project.objects[0].visible ||
                !position_equal(loaded_hierarchy_project.objects[0].overview_position,
                    (Position){-42.0f, 63.0f}) ||
                !loaded_hierarchy_project.layout_viewports[0].enabled ||
                !position_equal(loaded_hierarchy_project.layout_viewports[0].
                    overview_position, (Position){91.0f, -27.0f}) ||
                loaded_hierarchy_project.layout_viewports[0].background_color !=
                    0x12345678u) return 1;
        editor_project_destroy(&hierarchy_project);
        editor_project_destroy(&loaded_hierarchy_project);
        (void)remove(hierarchy_path);
    }

    {
        EditorProject layer_project;
        EditorObject *layer_object;
        EditorRigidBody *layer_body;
        EditorLayoutViewport *layer_viewport;
        EditorViewportUiItem *layer_ui;
        EditorGraphicsLayer *layer;
        editor_project_init(&layer_project);
        layer_object = editor_project_object_add(&layer_project, (Position){0});
        layer_body = editor_project_rigid_body_add(&layer_project, layer_object);
        layer_viewport = editor_project_layout_viewport_add(&layer_project);
        layer_ui = editor_viewport_ui_add(&layer_project, layer_viewport,
            EDITOR_VIEWPORT_UI_SHAPE);
        layer = editor_project_graphics_layer_add(&layer_project, "foreground", 73);
        if(layer_object == NULL || layer_body == NULL || layer_viewport == NULL ||
                layer_ui == NULL || layer == NULL) return 1;
        layer_body->graphics_layer.layer = layer->id;
        layer_ui->graphics_layer = layer->id;
        if(!editor_project_graphics_layer_set(&layer_project, layer->id,
                    "actors", 91) || strcmp(layer->name, "actors") != 0 ||
                layer_body->graphics_layer.layer != layer->id ||
                layer_ui->graphics_layer != layer->id ||
                !editor_project_graphics_layer_remove(&layer_project, layer->id) ||
                layer_body->graphics_layer.layer != 0 ||
                layer_body->graphics_layer.value != 91 ||
                layer_ui->graphics_layer != 0 || layer_ui->layer != 91) return 1;
        editor_project_destroy(&layer_project);
    }

    {
        EditorProject input_project, input_clone = {0};
        EditorInputController *controller;
        EditorInputAction *action;
        editor_project_init(&input_project);
        controller = editor_project_input_controller_add(&input_project, "gameplay");
        action = controller == NULL ? NULL : editor_project_input_action_add(
            &input_project, controller->id, "jump", INPUT_ACTION_BUTTON);
        if(controller == NULL || action == NULL ||
                !editor_project_input_binding_add(&input_project, controller->id,
                    action->id, (InputBinding){.source = INPUT_BINDING_KEY,
                        .input.key = SDL_SCANCODE_SPACE,
                        .scale = {1.0f, 1.0f}}) ||
                !editor_project_clone(&input_clone, &input_project)) return 1;
        input_clone.input_controllers[0].actions[0].bindings[0].input.key =
            SDL_SCANCODE_RETURN;
        snprintf(input_clone.input_controllers[0].actions[0].name,
            sizeof(input_clone.input_controllers[0].actions[0].name), "confirm");
        if(input_project.input_controllers[0].actions[0].bindings[0].input.key !=
                    SDL_SCANCODE_SPACE ||
                strcmp(input_project.input_controllers[0].actions[0].name,
                    "jump") != 0) return 1;
        editor_project_destroy(&input_project);
        editor_project_destroy(&input_clone);
    }

    editor_project_selection_clear(&project);
    if(editor_project_selected_get(&project) != NULL ||
            !editor_project_object_select(&project, object->id) ||
            !editor_project_object_remove(&project, object->id) ||
            project.object_count != 0) return 1;
    return 0;
}
