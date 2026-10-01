/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_command.h"

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLI_MAX 128
#define CLI_TEXT 1024

static const char *cli_input_type_name(InputActionType type);
static const char *cli_input_button_mode_name(InputButtonMode mode);

static bool cli_selector_flag(const char *value) {
    static const char *flags[] = {"--object", "--object-id", "--body", "--body-id",
        "--hitbox", "--hitbox-id", "--joint", "--joint-id", "--anchor",
        "--anchor-id", "--soft-body", "--soft-body-id", "--node", "--node-id",
        "--beam", "--beam-id", "--vertex", "--vertex-id", "--line",
        "--line-index", "--node-a", "--node-a-id", "--node-b", "--node-b-id",
        "--collision-mask", "--sprite", "--sprite-id", "--animated-sprite",
        "--animated-sprite-id", "--frame-index"};
    for(size_t i = 0; i < sizeof(flags) / sizeof(flags[0]); i += 1)
        if(strcmp(value, flags[i]) == 0) return true;
    return false;
}

static bool cli_text_add(char *output, size_t capacity, size_t *used,
        const char *value) {
    size_t length = strlen(value);
    if(*used >= capacity || length >= capacity - *used) return false;
    memcpy(output + *used, value, length + 1); *used += length; return true;
}

static bool cli_safe(const char *value) {
    if(value == NULL || value[0] == '\0') return false;
    for(const unsigned char *at = (const unsigned char *)value; *at; at += 1)
        if(!( (*at >= 'a' && *at <= 'z') || (*at >= 'A' && *at <= 'Z') ||
                (*at >= '0' && *at <= '9') || strchr("_-./:", *at))) return false;
    return true;
}

static bool cli_token_add(char *output, size_t capacity, size_t *used,
        const char *value) {
    if(*used && !cli_text_add(output, capacity, used, " ")) return false;
    if(cli_safe(value)) return cli_text_add(output, capacity, used, value);
    if(!cli_text_add(output, capacity, used, "'")) return false;
    for(const char *at = value; *at; at += 1) {
        char character[2] = {*at, '\0'};
        if(!cli_text_add(output, capacity, used,
                *at == '\'' ? "'\\''" : character)) return false;
    }
    return cli_text_add(output, capacity, used, "'");
}

static size_t cli_tokens(const char *text, char tokens[][CLI_TEXT]) {
    size_t count = 0;
    while(*text && count < CLI_MAX) {
        size_t used = 0; bool quoted = false;
        while(*text == ' ') text += 1;
        if(!*text) break;
        while(*text && (quoted || *text != ' ')) {
            if(*text == '\'') { quoted = !quoted; text += 1; continue; }
            if(*text == '\\' && !quoted && text[1]) text += 1;
            if(used + 1 < CLI_TEXT) tokens[count][used++] = *text;
            text += 1;
        }
        tokens[count][used] = '\0'; count += 1;
    }
    return count;
}

static const char *cli_item_flag(EditorItemKind kind) {
    static const char *flags[] = {"--object", "--body", "--hitbox", "--joint",
        "--anchor", "--soft-body", "--node", "--beam", "--vertex", "--line"};
    return kind <= EDITOR_ITEM_LINE ? flags[kind] : NULL;
}

static const char *cli_property(const char *domain, const char *action) {
    if(strcmp(action, "visibility") == 0) return "visibility";
    if(strcmp(action, "position") == 0) return "position";
    if(strcmp(action, "transform") == 0) return "transform";
    if(strcmp(action, "origin") == 0) return "origin";
    if(strcmp(action, "auto-shape") == 0) return "auto-shape";
    if(strcmp(action, "camera") == 0) return "camera";
    if(strcmp(action, "coordinates") == 0) return "coordinates";
    if(strcmp(domain, "navigation") == 0) return "navigation";
    return NULL;
}

static const EditorObject *cli_object_get(const EditorProject *project,
        EditorObjectId id) {
    if(project != NULL) for(size_t i = 0; i < project->object_count; i += 1)
        if(project->objects[i].id == id) return &project->objects[i];
    return NULL;
}

static bool cli_named_selector_add(char *output, size_t capacity, size_t *used,
        const char *name_flag, const char *id_flag, const char *name, uint32_t id,
        size_t matches) {
    char number[16];
    if(name != NULL && name[0] != '\0' && matches == 1)
        return cli_token_add(output, capacity, used, name_flag) &&
            cli_token_add(output, capacity, used, name);
    snprintf(number, sizeof(number), "%u", id);
    return cli_token_add(output, capacity, used, id_flag) &&
        cli_token_add(output, capacity, used, number);
}

static bool cli_sprite_selector_add(const EditorObject *object, EditorSpriteId id,
        char *output, size_t capacity, size_t *used) {
    const char *name = NULL; size_t matches = 0;
    for(size_t i = 0; object != NULL && i < object->sprite_count; i += 1)
        if(object->sprites[i].id == id) name = object->sprites[i].name;
    if(name != NULL) for(size_t i = 0; i < object->sprite_count; i += 1)
        if(strcmp(object->sprites[i].name, name) == 0) matches += 1;
    return cli_named_selector_add(output, capacity, used, "--sprite", "--sprite-id",
        name, id, matches);
}

static bool cli_object_selector_add(const EditorProject *project, EditorObjectId id,
        char *output, size_t capacity, size_t *used) {
    const char *name = NULL; size_t matches = 0;
    for(size_t i = 0; i < project->object_count; i += 1)
        if(project->objects[i].id == id) name = project->objects[i].name;
    if(name != NULL) for(size_t i = 0; i < project->object_count; i += 1)
        if(strcmp(project->objects[i].name, name) == 0) matches += 1;
    return cli_named_selector_add(output, capacity, used, "--object", "--object-id",
        name, id, matches);
}

static bool cli_animated_selector_add(const EditorObject *object,
        EditorAnimatedSpriteId id, char *output, size_t capacity, size_t *used) {
    const char *name = NULL; size_t matches = 0;
    if(object != NULL) for(size_t i = 0; i < object->animated_sprite_count; i += 1)
        if(object->animated_sprite_items[i].id == id)
            name = object->animated_sprite_items[i].name;
    if(name != NULL) for(size_t i = 0; i < object->animated_sprite_count; i += 1)
        if(strcmp(object->animated_sprite_items[i].name, name) == 0) matches += 1;
    return cli_named_selector_add(output, capacity, used, "--animated-sprite",
        "--animated-sprite-id", name, id, matches);
}

static EditorResult cli_sprite_command_write(const EditorProject *project,
        const EditorCommand *command, const char *path, char *output, size_t capacity,
        bool *handled) {
    size_t used = 0; char number[64]; EditorObjectId object_id = 0;
    EditorAnimatedSpriteId animated_id = 0; const EditorObject *object;
    *handled = command->type >= EDITOR_COMMAND_SPRITE_ADD &&
        command->type <= EDITOR_COMMAND_ANIMATION_FRAME_TRANSFORM_SET;
    if(!*handled) return editor_result_value(true);
    output[0] = '\0';
#define ADD(value) do { if(!cli_token_add(output, capacity, &used, (value))) goto full; } while(0)
    ADD("rohr-cli"); ADD("--project"); ADD(path);
    if(command->type >= EDITOR_COMMAND_SPRITE_ADD &&
            command->type <= EDITOR_COMMAND_SPRITE_VISIBILITY_SET) {
        object_id = command->type == EDITOR_COMMAND_SPRITE_ADD ?
            command->data.sprite_add.object : command->type == EDITOR_COMMAND_SPRITE_REMOVE ?
            command->data.sprite_remove.object : command->type == EDITOR_COMMAND_SPRITE_RENAME ?
            command->data.sprite_rename.object : command->type == EDITOR_COMMAND_SPRITE_PATH_SET ?
            command->data.sprite_path_set.object :
            command->type == EDITOR_COMMAND_SPRITE_POSITION_SET ?
                command->data.sprite_position_set.object :
            command->type == EDITOR_COMMAND_SPRITE_ROTATION_SET ?
                command->data.sprite_rotation_set.object :
            command->type == EDITOR_COMMAND_SPRITE_SIZE_SET ?
                command->data.sprite_size_set.object :
            command->type == EDITOR_COMMAND_SPRITE_BODY_SET ?
                command->data.sprite_body_set.object :
            command->type == EDITOR_COMMAND_SPRITE_FOLLOW_ROTATION_SET ?
                command->data.sprite_boolean_set.object :
                command->data.sprite_visibility_set.object;
        object = cli_object_get(project, object_id);
        if(!cli_object_selector_add(project, object_id, output, capacity, &used)) goto full;
        EditorSpriteId id = command->type == EDITOR_COMMAND_SPRITE_REMOVE ?
            command->data.sprite_remove.sprite : command->type == EDITOR_COMMAND_SPRITE_RENAME ?
            command->data.sprite_rename.sprite : command->type == EDITOR_COMMAND_SPRITE_PATH_SET ?
            command->data.sprite_path_set.sprite :
            command->type == EDITOR_COMMAND_SPRITE_POSITION_SET ?
                command->data.sprite_position_set.sprite :
            command->type == EDITOR_COMMAND_SPRITE_ROTATION_SET ?
                command->data.sprite_rotation_set.sprite :
            command->type == EDITOR_COMMAND_SPRITE_SIZE_SET ?
                command->data.sprite_size_set.sprite :
            command->type == EDITOR_COMMAND_SPRITE_BODY_SET ?
                command->data.sprite_body_set.sprite :
            command->type == EDITOR_COMMAND_SPRITE_FOLLOW_ROTATION_SET ?
                command->data.sprite_boolean_set.sprite :
                command->data.sprite_visibility_set.sprite;
        if(command->type == EDITOR_COMMAND_SPRITE_ADD) {
            ADD("--sprite"); ADD(command->data.sprite_add.name); ADD("add");
            ADD(command->data.sprite_add.path);
            snprintf(number, sizeof(number), "%.9g", command->data.sprite_add.size.x); ADD(number);
            snprintf(number, sizeof(number), "%.9g", command->data.sprite_add.size.y); ADD(number);
        } else {
            if(!cli_sprite_selector_add(object, id, output, capacity, &used)) goto full;
            if(command->type == EDITOR_COMMAND_SPRITE_REMOVE) ADD("delete");
            else if(command->type == EDITOR_COMMAND_SPRITE_RENAME) {
                ADD("rename"); ADD(command->data.sprite_rename.name);
            } else {
                ADD("--property");
                if(command->type == EDITOR_COMMAND_SPRITE_PATH_SET) {
                    ADD("path"); ADD(command->data.sprite_path_set.path);
                } else if(command->type == EDITOR_COMMAND_SPRITE_POSITION_SET) {
                    ADD("position");
                    snprintf(number, sizeof(number), "%.9g",
                        command->data.sprite_position_set.position.x); ADD(number);
                    snprintf(number, sizeof(number), "%.9g",
                        command->data.sprite_position_set.position.y); ADD(number);
                } else if(command->type == EDITOR_COMMAND_SPRITE_ROTATION_SET) {
                    ADD("rotation");
                    snprintf(number, sizeof(number), "%.9g",
                        command->data.sprite_rotation_set.rotation); ADD(number);
                } else if(command->type == EDITOR_COMMAND_SPRITE_SIZE_SET) {
                    ADD("size");
                    snprintf(number, sizeof(number), "%.9g", command->data.sprite_size_set.size.x); ADD(number);
                    snprintf(number, sizeof(number), "%.9g", command->data.sprite_size_set.size.y); ADD(number);
                } else if(command->type == EDITOR_COMMAND_SPRITE_BODY_SET) {
                    ADD("body");
                    if(command->data.sprite_body_set.body == 0) ADD("none");
                    else {
                        const EditorRigidBody *body = editor_project_rigid_body_get(
                            (EditorObject *)object, command->data.sprite_body_set.body);
                        ADD(body != NULL ? body->name : "none");
                    }
                } else if(command->type == EDITOR_COMMAND_SPRITE_FOLLOW_ROTATION_SET) {
                    ADD("follow-body-rotation");
                    ADD(command->data.sprite_boolean_set.enabled ? "true" : "false");
                } else {
                    ADD("visibility");
                    ADD(command->data.sprite_visibility_set.visible ? "true" : "false");
                }
            }
        }
        return editor_result_value(true);
    }
    object_id = command->data.animated_sprite_remove.object;
    animated_id = command->data.animated_sprite_remove.sprite;
    if(command->type == EDITOR_COMMAND_ANIMATED_SPRITE_ADD)
        object_id = command->data.animated_sprite_add.object;
    object = cli_object_get(project, object_id);
    if(!cli_object_selector_add(project, object_id, output, capacity, &used)) goto full;
    if(command->type == EDITOR_COMMAND_ANIMATED_SPRITE_ADD) {
        ADD("--animated-sprite"); ADD(command->data.animated_sprite_add.name); ADD("add");
        return editor_result_value(true);
    }
    if(!cli_animated_selector_add(object, animated_id, output, capacity, &used)) goto full;
    if(command->type == EDITOR_COMMAND_ANIMATED_SPRITE_REMOVE) ADD("delete");
    else if(command->type == EDITOR_COMMAND_ANIMATED_SPRITE_RENAME) {
        ADD("rename"); ADD(command->data.animated_sprite_rename.name);
    } else if(command->type == EDITOR_COMMAND_ANIMATION_FRAME_ADD) {
        ADD("frame-add");
        ADD(command->data.animation_frame_add.name);
        ADD(command->data.animation_frame_add.path);
        snprintf(number, sizeof(number), "%.9g", command->data.animation_frame_add.scale.x);
        ADD(number);
        snprintf(number, sizeof(number), "%.9g", command->data.animation_frame_add.scale.y);
        ADD(number);
    } else if(command->type == EDITOR_COMMAND_ANIMATION_FRAME_REMOVE) {
        ADD("--frame-index");
        snprintf(number, sizeof(number), "%zu", command->data.animation_frame_remove.index); ADD(number);
        ADD("frame-delete");
    } else if(command->type == EDITOR_COMMAND_ANIMATION_FRAME_RENAME ||
            command->type == EDITOR_COMMAND_ANIMATION_FRAME_PATH_SET ||
            command->type == EDITOR_COMMAND_ANIMATION_FRAME_SCALE_SET ||
            command->type == EDITOR_COMMAND_ANIMATION_FRAME_TRANSFORM_SET) {
        size_t index = command->type == EDITOR_COMMAND_ANIMATION_FRAME_RENAME ?
            command->data.animation_frame_rename.index :
            command->type == EDITOR_COMMAND_ANIMATION_FRAME_PATH_SET ?
                command->data.animation_frame_path_set.index :
                command->type == EDITOR_COMMAND_ANIMATION_FRAME_TRANSFORM_SET ?
                    command->data.animation_frame_transform_set.index :
                    command->data.animation_frame_scale_set.index;
        ADD("--frame-index");
        snprintf(number, sizeof(number), "%zu", index); ADD(number);
        if(command->type == EDITOR_COMMAND_ANIMATION_FRAME_RENAME) {
            ADD("frame-rename"); ADD(command->data.animation_frame_rename.name);
        } else if(command->type == EDITOR_COMMAND_ANIMATION_FRAME_PATH_SET) {
            ADD("frame-path-set"); ADD(command->data.animation_frame_path_set.path);
        } else if(command->type == EDITOR_COMMAND_ANIMATION_FRAME_TRANSFORM_SET) {
            ADD("frame-transform-set");
            snprintf(number, sizeof(number), "%.9g", command->data.animation_frame_transform_set.offset.x); ADD(number);
            snprintf(number, sizeof(number), "%.9g", command->data.animation_frame_transform_set.offset.y); ADD(number);
            snprintf(number, sizeof(number), "%.9g", command->data.animation_frame_transform_set.rotation); ADD(number);
        } else {
            ADD("frame-scale-set");
            snprintf(number, sizeof(number), "%.9g",
                command->data.animation_frame_scale_set.scale.x); ADD(number);
            snprintf(number, sizeof(number), "%.9g",
                command->data.animation_frame_scale_set.scale.y); ADD(number);
        }
    } else {
        ADD("--property");
        if(command->type == EDITOR_COMMAND_ANIMATED_SPRITE_BODY_SET) {
            ADD("body");
            if(command->data.animated_sprite_body_set.body == 0) ADD("none");
            else {
                const EditorRigidBody *body = editor_project_rigid_body_get(
                    (EditorObject *)object, command->data.animated_sprite_body_set.body);
                ADD(body != NULL ? body->name : "none");
            }
        } else if(command->type == EDITOR_COMMAND_ANIMATED_SPRITE_POSITION_SET) {
            ADD("position");
            snprintf(number, sizeof(number), "%.9g",
                command->data.animated_sprite_position_set.position.x); ADD(number);
            snprintf(number, sizeof(number), "%.9g",
                command->data.animated_sprite_position_set.position.y); ADD(number);
        } else if(command->type == EDITOR_COMMAND_ANIMATED_SPRITE_ROTATION_SET) {
            ADD("rotation");
            snprintf(number, sizeof(number), "%.9g",
                command->data.animated_sprite_rotation_set.rotation); ADD(number);
        } else if(command->type == EDITOR_COMMAND_ANIMATED_SPRITE_SCALE_SET) {
            ADD("scale");
            snprintf(number, sizeof(number), "%.9g", command->data.animated_sprite_scale_set.scale.x); ADD(number);
            snprintf(number, sizeof(number), "%.9g", command->data.animated_sprite_scale_set.scale.y); ADD(number);
        } else if(command->type == EDITOR_COMMAND_ANIMATED_SPRITE_TIMING_SET) {
            ADD("timing");
            snprintf(number, sizeof(number), "%llu", (unsigned long long)command->data.animated_sprite_timing_set.ticks); ADD(number);
            snprintf(number, sizeof(number), "%.17g", (double)command->data.animated_sprite_timing_set.time); ADD(number);
        } else if(command->type == EDITOR_COMMAND_ANIMATED_SPRITE_STARTING_FRAME_SET) {
            ADD("starting-frame"); snprintf(number, sizeof(number), "%u", command->data.animated_sprite_starting_frame_set.frame); ADD(number);
        } else if(command->type == EDITOR_COMMAND_ANIMATED_SPRITE_DIRECTION_SET) {
            ADD("direction"); ADD(command->data.animated_sprite_direction_set.direction == DIRECTION_LEFT ? "left" : "right");
        } else {
            ADD(command->type == EDITOR_COMMAND_ANIMATED_SPRITE_VISIBILITY_SET ?
                "visibility" : command->type ==
                    EDITOR_COMMAND_ANIMATED_SPRITE_PLAYING_SET ?
                    "playing" : "follow-body-rotation");
            ADD(command->data.animated_sprite_boolean_set.enabled ? "true" : "false");
        }
    }
    return editor_result_value(true);
full:
    return editor_result_error(EDITOR_ERROR_CAPACITY,
        "Selector-first sprite command exceeds output capacity");
#undef ADD
}

static EditorResult cli_input_command_write(const EditorProject *project,
        const EditorCommand *command, const EditorCommandResult *result,
        const char *path, char *output, size_t capacity, bool *handled) {
    size_t used = 0;
    char number[64];
    const EditorInputBindingCommand *binding;
    *handled = command->type >= EDITOR_COMMAND_INPUT_CONTROLLER_ADD &&
        command->type <= EDITOR_COMMAND_INPUT_BINDING_SET;
    if(!*handled) return editor_result_value(true);
    output[0] = '\0';
#define INPUT_ADD(value) do { \
    if(!cli_token_add(output, capacity, &used, (value))) goto full; \
} while(0)
    INPUT_ADD("rohr-cli"); INPUT_ADD("--project"); INPUT_ADD(path);
    if(command->type == EDITOR_COMMAND_INPUT_CONTROLLER_ADD) {
        INPUT_ADD("--controller"); INPUT_ADD(command->data.input_controller.name);
        INPUT_ADD("add");
        INPUT_ADD(command->data.input_controller.enabled ? "true" : "false");
        return editor_result_value(true);
    }
    snprintf(number, sizeof(number), "%u", command->type <=
        EDITOR_COMMAND_INPUT_CONTROLLER_SET ? command->data.input_controller.controller :
        command->type <= EDITOR_COMMAND_INPUT_ACTION_SET ?
            command->data.input_action.controller : command->data.input_binding.controller);
    INPUT_ADD("--controller-id"); INPUT_ADD(number);
    if(command->type == EDITOR_COMMAND_INPUT_CONTROLLER_REMOVE) INPUT_ADD("delete");
    else if(command->type == EDITOR_COMMAND_INPUT_CONTROLLER_SET) {
        INPUT_ADD("controller-set"); INPUT_ADD(command->data.input_controller.name);
        INPUT_ADD(command->data.input_controller.enabled ? "true" : "false");
    } else if(command->type == EDITOR_COMMAND_INPUT_ACTION_ADD) {
        INPUT_ADD("--action"); INPUT_ADD(command->data.input_action.name);
        INPUT_ADD("add"); INPUT_ADD(cli_input_type_name(
            command->data.input_action.type));
        if(command->data.input_action.type == INPUT_ACTION_BUTTON) {
            INPUT_ADD(cli_input_button_mode_name(
                command->data.input_action.button_mode));
            INPUT_ADD(command->data.input_action.button_initial_state ?
                "true" : "false");
        }
    } else {
        snprintf(number, sizeof(number), "%u", command->type <=
            EDITOR_COMMAND_INPUT_ACTION_SET ? command->data.input_action.action :
            command->data.input_binding.action);
        INPUT_ADD("--action-id"); INPUT_ADD(number);
        if(command->type == EDITOR_COMMAND_INPUT_ACTION_REMOVE)
            INPUT_ADD("delete");
        else if(command->type == EDITOR_COMMAND_INPUT_ACTION_SET) {
            INPUT_ADD("action-set");
            INPUT_ADD(command->data.input_action.name);
            INPUT_ADD(cli_input_type_name(command->data.input_action.type));
            if(command->data.input_action.type == INPUT_ACTION_BUTTON) {
                INPUT_ADD(cli_input_button_mode_name(
                    command->data.input_action.button_mode));
                INPUT_ADD(command->data.input_action.button_initial_state ?
                    "true" : "false");
            }
        } else {
            binding = &command->data.input_binding;
            const EditorInputAction *action = project == NULL ? NULL :
                editor_project_input_action_const_get(project,
                    binding->controller, binding->action);
            const char *binding_name = binding->name[0] != '\0' ?
                binding->name : binding->binding.name;
            if(action == NULL)
                return editor_result_error(EDITOR_ERROR_NOT_FOUND,
                    "Cannot serialize a binding without its input action");
            if(command->type == EDITOR_COMMAND_INPUT_BINDING_ADD &&
                    binding_name[0] == '\0' && project != NULL && result != NULL &&
                    result->created.valid) {
                const EditorInputAction *action =
                    editor_project_input_action_const_get(project,
                        binding->controller, binding->action);
                size_t index;
                if(editor_project_input_binding_index_get(action,
                        result->created.item, &index))
                    binding_name = action->bindings[index].name;
            }
            if(command->type == EDITOR_COMMAND_INPUT_BINDING_ADD) {
                INPUT_ADD("--binding"); INPUT_ADD(binding_name);
            } else if(binding->binding_id != EDITOR_INPUT_BINDING_INVALID) {
                INPUT_ADD("--binding-id");
                snprintf(number, sizeof(number), "%u", binding->binding_id);
                INPUT_ADD(number);
            } else {
                snprintf(number, sizeof(number), "%zu", binding->index);
                INPUT_ADD("--binding-index"); INPUT_ADD(number);
            }
            if(command->type == EDITOR_COMMAND_INPUT_BINDING_REMOVE) {
                INPUT_ADD("delete");
                return editor_result_value(true);
            }
            INPUT_ADD(command->type == EDITOR_COMMAND_INPUT_BINDING_ADD ?
                "add" : "binding-set");
            if(command->type == EDITOR_COMMAND_INPUT_BINDING_SET)
                INPUT_ADD(binding_name);
            INPUT_ADD(binding->binding.source == INPUT_BINDING_KEY ? "key" :
                binding->binding.source == INPUT_BINDING_MOUSE_BUTTON ?
                    "mouse-button" :
                binding->binding.source == INPUT_BINDING_MOUSE_MOTION ?
                    "mouse-motion" : "mouse-wheel");
            if(binding->binding.source == INPUT_BINDING_KEY)
                snprintf(number, sizeof(number), "%u",
                    (unsigned)binding->binding.input.key);
            else if(binding->binding.source == INPUT_BINDING_MOUSE_BUTTON)
                snprintf(number, sizeof(number), "%u",
                    (unsigned)binding->binding.input.mouse_button);
            else snprintf(number, sizeof(number), "%s",
                binding->binding.input.axis_component == INPUT_AXIS_COMPONENT_X ?
                    "x" : binding->binding.input.axis_component ==
                        INPUT_AXIS_COMPONENT_Y ? "y" : "xy");
            INPUT_ADD(number);
            snprintf(number, sizeof(number), "%u",
                (unsigned)binding->binding.modifiers); INPUT_ADD(number);
            if(action->type == INPUT_ACTION_AXIS_1D) {
                snprintf(number, sizeof(number), "%#.9g",
                    binding->binding.scale.x); INPUT_ADD(number);
                INPUT_ADD(binding->binding.inverted_x ? "true" : "false");
            } else if(action->type == INPUT_ACTION_AXIS_2D) {
                INPUT_ADD(binding->binding.affects_x ? "true" : "false");
                INPUT_ADD(binding->binding.affects_y ? "true" : "false");
                snprintf(number, sizeof(number), "%#.9g",
                    binding->binding.scale.x); INPUT_ADD(number);
                snprintf(number, sizeof(number), "%#.9g",
                    binding->binding.scale.y); INPUT_ADD(number);
                INPUT_ADD(binding->binding.inverted_x ? "true" : "false");
                INPUT_ADD(binding->binding.inverted_y ? "true" : "false");
                snprintf(number, sizeof(number), "%#.9g",
                    binding->binding.direction.x); INPUT_ADD(number);
                snprintf(number, sizeof(number), "%#.9g",
                    binding->binding.direction.y); INPUT_ADD(number);
            }
        }
    }
    return editor_result_value(true);
full:
    return editor_result_error(EDITOR_ERROR_CAPACITY,
        "Selector-first input command exceeds output capacity");
#undef INPUT_ADD
}

static EditorResult cli_area_command_write(const EditorProject *project, const EditorCommand *command,
    const EditorCommandResult *result, const char *path,
    char *output, size_t capacity, bool *handled);

EditorResult editor_command_cli_standard_write(const EditorProject *project,
        const EditorCommand *command, const EditorCommandResult *result,
        const char *path, char *output, size_t capacity) {
    char legacy[4096], token[CLI_MAX][CLI_TEXT];
    size_t count, at = 4, used = 0;
    const char *property;
    bool handled = false;
    EditorResult area_result = cli_area_command_write(project, command, result, path, output, capacity, &handled);
    if(handled) return area_result;
    EditorResult input = cli_input_command_write(project, command, result, path,
        output, capacity, &handled);
    if(handled) return input;
    EditorResult special = cli_sprite_command_write(project, command, path,
        output, capacity, &handled);
    if(handled) return special;
    EditorResult serialized = editor_command_cli_named_write(project, command,
        path, legacy, sizeof(legacy));
    if(editor_result_check(serialized)) return serialized;
    count = cli_tokens(legacy, token);
    if(count < 4) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "Could not serialize selector-first command");
    output[0] = '\0';
#define ADD(value) do { if(!cli_token_add(output, capacity, &used, (value))) goto full; } while(0)
    ADD("rohr-cli"); ADD("--project"); ADD(token[3]);
    if(command->type == EDITOR_COMMAND_NAVIGATION_SET) {
        if(count != 17) goto invalid;
        for(size_t i = 7; i + 1 < count; i += 2) { ADD(token[i]); ADD(token[i + 1]); }
        ADD("--property"); ADD("navigation"); ADD(token[4]); ADD(token[5]); ADD(token[6]);
        return editor_result_value(true);
    }
    if(command->type == EDITOR_COMMAND_OBJECT_ADD) {
        ADD("--object"); ADD(command->data.object_add.name); at = 5;
    } else if(command->type == EDITOR_COMMAND_COLLISION_MASK_ADD) {
        ADD("--collision-mask"); ADD(command->data.collision_mask_add.name); at = 5;
    } else while(at + 1 < count && cli_selector_flag(token[at])) {
        ADD(token[at]); ADD(token[at + 1]); at += 2;
    }
    if(command->type == EDITOR_COMMAND_ITEM_ADD && result != NULL &&
            result->created.valid) {
        const char *flag = cli_item_flag(result->created.kind);
        if(flag == NULL || result->created.name[0] == '\0') goto invalid;
        ADD(flag); ADD(result->created.name);
    }
    if(strcmp(token[2], "add") == 0 || strcmp(token[2], "delete") == 0 ||
            strcmp(token[2], "rename") == 0) ADD(token[2]);
    else {
        ADD("--property");
        if(strcmp(token[2], "set") == 0 || strcmp(token[2], "filter") == 0 ||
                strcmp(token[2], "connect") == 0) {
            if(at >= count) goto invalid;
            ADD(token[at++]);
        } else {
            property = cli_property(token[1], token[2]);
            if(property == NULL) goto invalid;
            ADD(property);
        }
    }
    for(; at < count; at += 1) ADD(token[at]);
#undef ADD
    return editor_result_value(true);
invalid:
    return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "Command cannot be represented by selector-first grammar");
full:
    return editor_result_error(EDITOR_ERROR_CAPACITY,
        "Selector-first command exceeds output capacity");
}

typedef struct CliInput {
    char *selectors[CLI_MAX];
    size_t selector_count;
    const char *path;
    int terminal;
    const char *domain;
    const char *target_flag;
    const char *target_name;
    int rank;
} CliInput;

static void cli_target_consider(CliInput *input, const char *flag, const char *name) {
    int rank; const char *domain;
    if(strstr(flag, "frame-index")) return;
    if(strstr(flag, "animated-sprite")) { rank = 2; domain = "animated-sprite"; }
    else if(strstr(flag, "sprite")) { rank = 1; domain = "sprite"; }
    else if(strstr(flag, "vertex")) { rank = 3; domain = "vertex"; }
    else if(strstr(flag, "line")) { rank = 3; domain = "line"; }
    else if(strstr(flag, "hitbox")) { rank = 2; domain = "hitbox"; }
    else if(strstr(flag, "collision-mask")) { rank = 1; domain = "collision-mask"; }
    else if(strstr(flag, "node-a") || strstr(flag, "node-b")) return;
    else if(strstr(flag, "node")) { rank = 2; domain = "soft-node"; }
    else if(strstr(flag, "beam")) { rank = 2; domain = "soft-beam"; }
    else if(strstr(flag, "soft-body")) { rank = 1; domain = "soft-body"; }
    else if(strstr(flag, "body")) { rank = 1; domain = "rigid-body"; }
    else if(strstr(flag, "joint")) { rank = 1; domain = "joint"; }
    else if(strstr(flag, "anchor")) { rank = 1; domain = "anchor"; }
    else { rank = 0; domain = "object"; }
    if(rank >= input->rank) {
        input->rank = rank; input->domain = domain;
        input->target_flag = flag; input->target_name = name;
    }
}

static EditorResult cli_input_get(int count, char **arguments, CliInput *input) {
    *input = (CliInput){.path = "./objects/project.rohr.json", .terminal = -1, .rank = -1};
    for(int i = 1; i < count; i += 1) {
        if(strcmp(arguments[i], "--project") == 0) {
            if(++i >= count) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "--project requires a path");
            input->path = arguments[i];
        } else if(strcmp(arguments[i], "--property") == 0 || arguments[i][0] != '-') {
            input->terminal = i; break;
        } else if(cli_selector_flag(arguments[i])) {
            const char *flag = arguments[i];
            if(++i >= count) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "%s requires a value", flag);
            input->selectors[input->selector_count++] = (char *)flag;
            input->selectors[input->selector_count++] = arguments[i];
            cli_target_consider(input, flag, arguments[i]);
        } else return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "Unknown selector %s", arguments[i]);
    }
    if(input->terminal < 0)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "Expected an operation");
    return editor_result_value(true);
}

static bool cli_selector_target_check(const CliInput *input, size_t index) {
    return input->selectors[index] == input->target_flag;
}

static bool cli_input_uint_parse(const char *value, uint32_t *output) {
    char *end;
    unsigned long number;
    if(value == NULL || output == NULL || value[0] == '\0') return false;
    errno = 0;
    number = strtoul(value, &end, 10);
    if(errno != 0 || *end != '\0' || number > UINT32_MAX) return false;
    *output = (uint32_t)number;
    return true;
}

static bool cli_input_float_parse(const char *value, float *output) {
    char *end;
    float number;
    if(value == NULL || output == NULL || value[0] == '\0') return false;
    errno = 0;
    number = strtof(value, &end);
    if(errno != 0 || *end != '\0' || !isfinite(number)) return false;
    *output = number;
    return true;
}

static bool cli_input_bool_parse(const char *value, bool *output) {
    if(value == NULL || output == NULL) return false;
    if(strcmp(value, "true") == 0) *output = true;
    else if(strcmp(value, "false") == 0) *output = false;
    else return false;
    return true;
}

static bool cli_input_type_parse(const char *value, InputActionType *type) {
    if(value == NULL || type == NULL) return false;
    if(strcmp(value, "button") == 0) *type = INPUT_ACTION_BUTTON;
    else if(strcmp(value, "axis-1d") == 0) *type = INPUT_ACTION_AXIS_1D;
    else if(strcmp(value, "axis-2d") == 0) *type = INPUT_ACTION_AXIS_2D;
    else return false;
    return true;
}

static const char *cli_input_type_name(InputActionType type) {
    return type == INPUT_ACTION_BUTTON ? "button" :
        type == INPUT_ACTION_AXIS_1D ? "axis-1d" : "axis-2d";
}

static bool cli_input_button_mode_parse(const char *value,
        InputButtonMode *mode) {
    if(value == NULL || mode == NULL) return false;
    if(strcmp(value, "momentary") == 0) *mode = INPUT_BUTTON_MOMENTARY;
    else if(strcmp(value, "persistent") == 0) *mode = INPUT_BUTTON_PERSISTENT;
    else return false;
    return true;
}

static const char *cli_input_button_mode_name(InputButtonMode mode) {
    return mode == INPUT_BUTTON_PERSISTENT ? "persistent" : "momentary";
}

static EditorResult cli_input_binding_parse(int count, char **arguments,
        int at, InputActionType type, InputBinding *binding) {
    int value_count = count - at;
    bool effects_authored = false;
    uint32_t number, modifiers;
    if(binding == NULL || value_count < 3)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "input binding requires source, input, and modifiers");
    *binding = (InputBinding){.affects_x = true, .affects_y = true,
        .scale = {1.0f, 1.0f}};
    if(strcmp(arguments[at], "key") == 0) {
        binding->source = INPUT_BINDING_KEY;
        if(cli_input_uint_parse(arguments[at + 1], &number))
            binding->input.key = (SDL_Scancode)number;
        else binding->input.key = SDL_GetScancodeFromName(arguments[at + 1]);
    } else if(strcmp(arguments[at], "mouse-button") == 0) {
        binding->source = INPUT_BINDING_MOUSE_BUTTON;
        if(cli_input_uint_parse(arguments[at + 1], &number))
            binding->input.mouse_button = (InputMouseButton)number;
        else if(strcmp(arguments[at + 1], "left") == 0)
            binding->input.mouse_button = INPUT_MOUSE_BUTTON_LEFT;
        else if(strcmp(arguments[at + 1], "middle") == 0)
            binding->input.mouse_button = INPUT_MOUSE_BUTTON_MIDDLE;
        else if(strcmp(arguments[at + 1], "right") == 0)
            binding->input.mouse_button = INPUT_MOUSE_BUTTON_RIGHT;
        else if(strcmp(arguments[at + 1], "x1") == 0)
            binding->input.mouse_button = INPUT_MOUSE_BUTTON_X1;
        else if(strcmp(arguments[at + 1], "x2") == 0)
            binding->input.mouse_button = INPUT_MOUSE_BUTTON_X2;
    } else {
        binding->source = strcmp(arguments[at], "mouse-motion") == 0 ?
            INPUT_BINDING_MOUSE_MOTION : INPUT_BINDING_MOUSE_WHEEL;
        if(strcmp(arguments[at], "mouse-motion") != 0 &&
                strcmp(arguments[at], "mouse-wheel") != 0)
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "unknown input binding source: %s", arguments[at]);
        if(strcmp(arguments[at + 1], "x") == 0)
            binding->input.axis_component = INPUT_AXIS_COMPONENT_X;
        else if(strcmp(arguments[at + 1], "y") == 0)
            binding->input.axis_component = INPUT_AXIS_COMPONENT_Y;
        else if(strcmp(arguments[at + 1], "xy") == 0)
            binding->input.axis_component = INPUT_AXIS_COMPONENT_XY;
        else return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "mouse axes use x, y, or xy");
    }
    if(!cli_input_uint_parse(arguments[at + 2], &modifiers))
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "input binding contains invalid modifiers");
    binding->modifiers = (SDL_Keymod)modifiers;
    if(value_count == 7) {
        float scale;
        bool inverted;
        if(!cli_input_float_parse(arguments[at + 3], &scale) ||
                !cli_input_bool_parse(arguments[at + 4], &inverted) ||
                !cli_input_float_parse(arguments[at + 5],
                    &binding->direction.x) ||
                !cli_input_float_parse(arguments[at + 6],
                    &binding->direction.y))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "legacy input binding contains an invalid scale, inversion, or direction");
        binding->scale = (Vec2D){scale, scale};
        binding->inverted_x = inverted;
        binding->inverted_y = inverted;
    } else if(type == INPUT_ACTION_BUTTON) {
        if(value_count != 3)
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "button binding requires source, input, and modifiers");
    } else if(type == INPUT_ACTION_AXIS_1D) {
        if(value_count != 5 ||
                !cli_input_float_parse(arguments[at + 3],
                    &binding->scale.x) ||
                !cli_input_bool_parse(arguments[at + 4],
                    &binding->inverted_x))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "Axis 1D binding requires scale and inverted after modifiers");
    } else if(value_count == 9) {
        if(!cli_input_float_parse(arguments[at + 3], &binding->scale.x) ||
                !cli_input_float_parse(arguments[at + 4],
                    &binding->scale.y) ||
                !cli_input_bool_parse(arguments[at + 5],
                    &binding->inverted_x) ||
                !cli_input_bool_parse(arguments[at + 6],
                    &binding->inverted_y) ||
                !cli_input_float_parse(arguments[at + 7],
                    &binding->direction.x) ||
                !cli_input_float_parse(arguments[at + 8],
                    &binding->direction.y))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "legacy Axis 2D binding contains an invalid scale, inversion, or direction");
    } else if(value_count != 11 ||
            !cli_input_bool_parse(arguments[at + 3], &binding->affects_x) ||
            !cli_input_bool_parse(arguments[at + 4], &binding->affects_y) ||
            !cli_input_float_parse(arguments[at + 5], &binding->scale.x) ||
            !cli_input_float_parse(arguments[at + 6], &binding->scale.y) ||
            !cli_input_bool_parse(arguments[at + 7], &binding->inverted_x) ||
            !cli_input_bool_parse(arguments[at + 8], &binding->inverted_y) ||
            !cli_input_float_parse(arguments[at + 9],
                &binding->direction.x) ||
            !cli_input_float_parse(arguments[at + 10],
                &binding->direction.y))
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "Axis 2D binding requires affects-x, affects-y, scale-x, scale-y, inverted-x, inverted-y, direction-x, and direction-y after modifiers");
    else effects_authored = true;
    if(type == INPUT_ACTION_AXIS_2D && !effects_authored) {
        if(binding->source == INPUT_BINDING_KEY ||
                binding->source == INPUT_BINDING_MOUSE_BUTTON) {
            binding->affects_x = binding->direction.x != 0.0f;
            binding->affects_y = binding->direction.y != 0.0f;
        } else {
            binding->affects_x = binding->input.axis_component !=
                INPUT_AXIS_COMPONENT_Y;
            binding->affects_y = binding->input.axis_component !=
                INPUT_AXIS_COMPONENT_X;
        }
    }
    if(!rohr_input_binding_valid_check(type, binding))
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "input binding is invalid for the selected action type");
    return editor_result_value(true);
}

static EditorResult cli_input_command_parse(const EditorProject *project,
        int count, char **arguments, const char **path, EditorCommand *command,
        bool *handled) {
    const char *controller_name = NULL;
    const char *action_name = NULL;
    const char *binding_name = NULL;
    const char *operation = NULL;
    uint32_t controller_id = 0, action_id = 0, binding_id = 0;
    uint32_t binding_index = 0;
    bool controller_id_set = false, action_id_set = false;
    bool binding_id_set = false, binding_index_set = false;
    int operation_index = -1;
    *handled = false;
    if(project == NULL || arguments == NULL || path == NULL || command == NULL)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "input command parse received an invalid argument");
    *path = "./objects/project.rohr.json";
    for(int i = 1; i < count; i += 1) {
        if(strcmp(arguments[i], "--project") == 0 && i + 1 < count)
            *path = arguments[++i];
        else if(strcmp(arguments[i], "--controller") == 0 && i + 1 < count) {
            *handled = true; controller_name = arguments[++i];
        } else if(strcmp(arguments[i], "--controller-id") == 0 && i + 1 < count) {
            *handled = true;
            if(!cli_input_uint_parse(arguments[++i], &controller_id)) goto invalid_selector;
            controller_id_set = true;
        } else if(strcmp(arguments[i], "--action") == 0 && i + 1 < count) {
            *handled = true; action_name = arguments[++i];
        } else if(strcmp(arguments[i], "--action-id") == 0 && i + 1 < count) {
            *handled = true;
            if(!cli_input_uint_parse(arguments[++i], &action_id)) goto invalid_selector;
            action_id_set = true;
        } else if(strcmp(arguments[i], "--binding") == 0 && i + 1 < count) {
            *handled = true; binding_name = arguments[++i];
        } else if(strcmp(arguments[i], "--binding-id") == 0 && i + 1 < count) {
            *handled = true;
            if(!cli_input_uint_parse(arguments[++i], &binding_id))
                goto invalid_selector;
            binding_id_set = true;
        } else if(strcmp(arguments[i], "--binding-index") == 0 && i + 1 < count) {
            *handled = true;
            if(!cli_input_uint_parse(arguments[++i], &binding_index))
                goto invalid_selector;
            binding_index_set = true;
        } else {
            operation = arguments[i]; operation_index = i; break;
        }
    }
    if(!*handled) return editor_result_value(true);
    if(operation == NULL) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "input selector requires an operation");
    if(controller_id_set) {
        if(editor_project_input_controller_const_get(project, controller_id) == NULL)
            return editor_result_error(EDITOR_ERROR_NOT_FOUND,
                "input controller %u was not found", controller_id);
    } else if(controller_name != NULL &&
            (strcmp(operation, "add") != 0 || action_name != NULL)) {
        for(size_t i = 0; i < project->input_controller_count; i += 1)
            if(strcmp(project->input_controllers[i].name, controller_name) == 0)
                controller_id = project->input_controllers[i].id;
        if(controller_id == 0) return editor_result_error(EDITOR_ERROR_NOT_FOUND,
            "input controller '%s' was not found", controller_name);
    }
    const EditorInputController *controller =
        editor_project_input_controller_const_get(project, controller_id);
    if(action_id_set) {
        if(controller == NULL || editor_project_input_action_const_get(project, controller_id,
                action_id) == NULL)
            return editor_result_error(EDITOR_ERROR_NOT_FOUND,
                "input action %u was not found in the selected controller", action_id);
    } else if(action_name != NULL) {
        if(controller == NULL && strcmp(operation, "add") != 0)
            return editor_result_error(EDITOR_ERROR_NOT_FOUND,
                "input action requires an existing controller");
        if(controller != NULL) for(size_t i = 0; i < controller->action_count; i += 1)
            if(strcmp(controller->actions[i].name, action_name) == 0)
                action_id = controller->actions[i].id;
        if(action_id == 0 && strcmp(operation, "add") != 0)
            return editor_result_error(EDITOR_ERROR_NOT_FOUND,
                "input action '%s' was not found", action_name);
    }
    const EditorInputAction *action = controller == NULL ? NULL :
        editor_project_input_action_const_get(project, controller_id, action_id);
    size_t selected_binding_index = binding_index;
    bool binding_add = strcmp(operation, "binding-add") == 0 ||
        (strcmp(operation, "add") == 0 && binding_name != NULL &&
            !binding_id_set && !binding_index_set);
    bool binding_selected = binding_name != NULL || binding_id_set ||
        binding_index_set;
    if(action_name == NULL && action != NULL) action_name = action->name;
    if(controller_name == NULL && controller != NULL) controller_name = controller->name;
    if(action_name == NULL) {
        bool enabled;
        if(controller_name == NULL) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "input controller selector is required");
        if(strcmp(operation, "add") == 0) {
            enabled = true;
            if(operation_index + 1 < count &&
                    !cli_input_bool_parse(arguments[operation_index + 1], &enabled))
                return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                    "input controller enabled value must be true or false");
            if(operation_index + (operation_index + 1 < count ? 2 : 1) != count)
                return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                    "input controller add accepts only an optional enabled value");
            *command = (EditorCommand){.type = EDITOR_COMMAND_INPUT_CONTROLLER_ADD,
                .data.input_controller = {.enabled = enabled}};
            snprintf(command->data.input_controller.name,
                sizeof(command->data.input_controller.name), "%s", controller_name);
            return editor_result_value(true);
        }
        if(controller == NULL) return editor_result_error(EDITOR_ERROR_NOT_FOUND,
            "input controller was not found");
        if(strcmp(operation, "delete") == 0 && operation_index + 1 == count) {
            *command = (EditorCommand){.type = EDITOR_COMMAND_INPUT_CONTROLLER_REMOVE,
                .data.input_controller = {.controller = controller_id, .enabled = controller->enabled}};
            snprintf(command->data.input_controller.name,
                sizeof(command->data.input_controller.name), "%s", controller->name);
            return editor_result_value(true);
        }
        if(strcmp(operation, "controller-set") == 0 && operation_index + 3 == count &&
                cli_input_bool_parse(arguments[operation_index + 2], &enabled)) {
            *command = (EditorCommand){.type = EDITOR_COMMAND_INPUT_CONTROLLER_SET,
                .data.input_controller = {.controller = controller_id, .enabled = enabled}};
            snprintf(command->data.input_controller.name,
                sizeof(command->data.input_controller.name), "%s",
                arguments[operation_index + 1]);
            return editor_result_value(true);
        }
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "input controller supports add, delete, or controller-set <name> <enabled>");
    }
    if(action == NULL && strcmp(operation, "add") == 0) {
        InputActionType type;
        InputButtonMode mode = INPUT_BUTTON_MOMENTARY;
        bool initial_state = false;
        if(controller == NULL || operation_index + 2 > count ||
                !cli_input_type_parse(arguments[operation_index + 1], &type))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "input action add requires button, axis-1d, or axis-2d");
        if(type == INPUT_ACTION_BUTTON) {
            if(operation_index + 4 != count ||
                    !cli_input_button_mode_parse(
                        arguments[operation_index + 2], &mode) ||
                    !cli_input_bool_parse(arguments[operation_index + 3],
                        &initial_state))
                return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                    "Button action add requires momentary|persistent and an initial boolean state");
            if(mode == INPUT_BUTTON_MOMENTARY && initial_state)
                return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                    "Momentary Button actions cannot have an active initial state");
        } else if(operation_index + 2 != count) {
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "Axis action add accepts only its action type");
        }
        *command = (EditorCommand){.type = EDITOR_COMMAND_INPUT_ACTION_ADD,
            .data.input_action = {.controller = controller_id, .type = type,
                .button_mode = mode,
                .button_initial_state = initial_state}};
        snprintf(command->data.input_action.name,
            sizeof(command->data.input_action.name), "%s", action_name);
        return editor_result_value(true);
    }
    if(action == NULL) return editor_result_error(EDITOR_ERROR_NOT_FOUND,
        "input action was not found");
    if(binding_id_set) {
        if(!editor_project_input_binding_index_get(action, binding_id,
                &selected_binding_index))
            return editor_result_error(EDITOR_ERROR_NOT_FOUND,
                "input binding %u was not found", binding_id);
    } else if(binding_name != NULL && !binding_add) {
        selected_binding_index = SIZE_MAX;
        for(size_t i = 0; i < action->binding_count; i += 1)
            if(strcmp(action->bindings[i].name, binding_name) == 0) {
                selected_binding_index = i;
                break;
            }
        if(selected_binding_index == SIZE_MAX)
            return editor_result_error(EDITOR_ERROR_NOT_FOUND,
                "input binding '%s' was not found", binding_name);
    } else if(binding_index_set && selected_binding_index >= action->binding_count) {
        return editor_result_error(EDITOR_ERROR_NOT_FOUND,
            "input binding index %u was not found", binding_index);
    }
    if(strcmp(operation, "delete") == 0 && !binding_selected &&
            operation_index + 1 == count) {
        *command = (EditorCommand){.type = EDITOR_COMMAND_INPUT_ACTION_REMOVE,
            .data.input_action = {.controller = controller_id, .action = action_id,
                .type = action->type, .button_mode = action->button_mode,
                .button_initial_state = action->button_initial_state}};
        snprintf(command->data.input_action.name,
            sizeof(command->data.input_action.name), "%s", action->name);
        return editor_result_value(true);
    }
    if(strcmp(operation, "action-set") == 0) {
        InputActionType type;
        InputButtonMode mode = INPUT_BUTTON_MOMENTARY;
        bool initial_state = false;
        if(operation_index + 3 > count ||
                !cli_input_type_parse(arguments[operation_index + 2], &type))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "input action type must be button, axis-1d, or axis-2d");
        if(type == INPUT_ACTION_BUTTON) {
            if(operation_index + 5 != count ||
                    !cli_input_button_mode_parse(
                        arguments[operation_index + 3], &mode) ||
                    !cli_input_bool_parse(arguments[operation_index + 4],
                        &initial_state))
                return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                    "Button action-set requires name, type, momentary|persistent, and initial state");
            if(mode == INPUT_BUTTON_MOMENTARY && initial_state)
                return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                    "Momentary Button actions cannot have an active initial state");
        } else if(operation_index + 3 != count) {
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "Axis action-set accepts only name and action type");
        }
        *command = (EditorCommand){.type = EDITOR_COMMAND_INPUT_ACTION_SET,
            .data.input_action = {.controller = controller_id, .action = action_id,
                .type = type, .button_mode = mode,
                .button_initial_state = initial_state}};
        snprintf(command->data.input_action.name,
            sizeof(command->data.input_action.name), "%s",
            arguments[operation_index + 1]);
        return editor_result_value(true);
    }
    if((strcmp(operation, "binding-delete") == 0 ||
            strcmp(operation, "delete") == 0) && binding_selected &&
            operation_index + 1 == count) {
        *command = (EditorCommand){.type = EDITOR_COMMAND_INPUT_BINDING_REMOVE,
            .data.input_binding = {.controller = controller_id, .action = action_id,
                .binding_id = action->binding_ids[selected_binding_index],
                .index = selected_binding_index,
                .binding = action->bindings[selected_binding_index]}};
        snprintf(command->data.input_binding.name,
            sizeof(command->data.input_binding.name), "%s",
            action->bindings[selected_binding_index].name);
        return editor_result_value(true);
    }
    if(binding_add || (strcmp(operation, "binding-set") == 0 &&
            binding_selected)) {
        EditorResult parsed;
        int value_index = operation_index + 1;
        int binding_value_count = action->type == INPUT_ACTION_BUTTON ? 3 :
            action->type == INPUT_ACTION_AXIS_1D ? 5 : 11;
        *command = (EditorCommand){
            .type = binding_add ? EDITOR_COMMAND_INPUT_BINDING_ADD :
                EDITOR_COMMAND_INPUT_BINDING_SET,
            .data.input_binding = {.controller = controller_id, .action = action_id,
                .binding_id = binding_add ? EDITOR_INPUT_BINDING_INVALID :
                    action->binding_ids[selected_binding_index],
                .index = binding_add ? 0 : selected_binding_index}};
        if(binding_add) {
            if(binding_name != NULL) snprintf(command->data.input_binding.name,
                sizeof(command->data.input_binding.name), "%s", binding_name);
        } else if(count - value_index == binding_value_count + 1 ||
                count - value_index == 8 || count - value_index == 10) {
            snprintf(command->data.input_binding.name,
                sizeof(command->data.input_binding.name), "%s",
                arguments[value_index++]);
        } else {
            snprintf(command->data.input_binding.name,
                sizeof(command->data.input_binding.name), "%s",
                action->bindings[selected_binding_index].name);
        }
        parsed = cli_input_binding_parse(count, arguments, value_index,
            action->type, &command->data.input_binding.binding);
        if(!editor_result_check(parsed)) snprintf(
            command->data.input_binding.binding.name,
            sizeof(command->data.input_binding.binding.name), "%s",
            command->data.input_binding.name);
        return parsed;
    }
    return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "input action supports add, delete, action-set, binding-add, binding-set, or binding-delete");
invalid_selector:
    return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "input selector ID or binding index must be an unsigned integer");
}

typedef struct AreaCliSelector { const char *value; bool id; } AreaCliSelector;
static bool area_cli_match(AreaCliSelector selector, const char *name, uint32_t id) {
    uint32_t parsed;
    return selector.value != NULL && (selector.id ?
        cli_input_uint_parse(selector.value, &parsed) && parsed == id : strcmp(selector.value, name) == 0);
}
static EditorResult cli_area_command_parse(const EditorProject *project,
        int count, char **arguments, const char **path, EditorCommand *command, bool *handled) {
    *handled = false;
    for(int i = 1; i < count; i += 1)
        if(strcmp(arguments[i], "--area") == 0 || strcmp(arguments[i], "--area-id") == 0 ||
                strcmp(arguments[i], "--hole") == 0 || strcmp(arguments[i], "--hole-id") == 0) *handled = true;
    if(!*handled) return editor_result_value(false);
    if(project == NULL || path == NULL || command == NULL)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Area command requires a project and output");
    AreaCliSelector object_s = {0}, body_s = {0}, area_s = {0}, hole_s = {0};
    int operation = -1;
    *path = NULL;
    for(int i = 1; i < count; i += 1) {
        const char *flag = arguments[i];
        if(strcmp(flag, "add") == 0 || strcmp(flag, "delete") == 0 ||
                strcmp(flag, "rename") == 0 || strcmp(flag, "--property") == 0) { operation = i; break; }
        if(i + 1 >= count) goto invalid;
        const char *value = arguments[++i];
        if(strcmp(flag, "--project") == 0) { *path = value; continue; }
        AreaCliSelector *selector = NULL;
        if(strcmp(flag, "--object") == 0 || strcmp(flag, "--object-id") == 0) selector = &object_s;
        else if(strcmp(flag, "--soft-body") == 0 || strcmp(flag, "--soft-body-id") == 0) selector = &body_s;
        else if(strcmp(flag, "--area") == 0 || strcmp(flag, "--area-id") == 0) selector = &area_s;
        else if(strcmp(flag, "--hole") == 0 || strcmp(flag, "--hole-id") == 0) selector = &hole_s;
        if(selector == NULL || selector->value != NULL) goto invalid;
        *selector = (AreaCliSelector){value, strstr(flag, "-id") != NULL};
    }
    if(operation < 0 || *path == NULL || object_s.value == NULL || body_s.value == NULL || area_s.value == NULL) goto invalid;
    const EditorObject *object = NULL; const EditorSoftBody *body = NULL;
    const EditorSoftArea *area = NULL; const EditorSoftHole *hole = NULL;
    size_t matches = 0;
    for(size_t i = 0; i < project->object_count; i += 1)
        if(area_cli_match(object_s, project->objects[i].name, project->objects[i].id)) { object = &project->objects[i]; matches++; }
    if(matches != 1) goto missing;
    matches = 0;
    for(size_t i = 0; i < object->soft_body_count; i += 1)
        if(area_cli_match(body_s, object->soft_body_items[i].name, object->soft_body_items[i].id)) { body = &object->soft_body_items[i]; matches++; }
    if(matches != 1) goto missing;
    matches = 0;
    for(size_t i = 0; i < body->area_count; i += 1)
        if(area_cli_match(area_s, body->areas[i].name, body->areas[i].id)) { area = &body->areas[i]; matches++; }
    bool add = strcmp(arguments[operation], "add") == 0;
    bool is_hole = hole_s.value != NULL;
    if(add && !is_hole) {
        if(operation + 1 != count || area_s.id || matches != 0) goto invalid;
        *command = (EditorCommand){.type = EDITOR_COMMAND_ITEM_ADD,
            .data.item_add = {.kind = EDITOR_ITEM_SOFT_AREA, .object = object->id, .parent = body->id}};
        snprintf(command->data.item_add.name, sizeof(command->data.item_add.name), "%s", area_s.value);
        return editor_result_value(true);
    }
    if(matches != 1) goto missing;
    matches = 0;
    if(is_hole) for(uint32_t i = 0; i < area->hole_count; i += 1)
        if(area_cli_match(hole_s, area->holes[i].name, area->holes[i].id)) { hole = &area->holes[i]; matches++; }
    if(add) {
        if(operation + 1 != count || hole_s.id || matches != 0) goto invalid;
        *command = (EditorCommand){.type = EDITOR_COMMAND_ITEM_ADD,
            .data.item_add = {.kind = EDITOR_ITEM_SOFT_HOLE, .object = object->id,
                .parent = body->id, .first = area->id}};
        snprintf(command->data.item_add.name, sizeof(command->data.item_add.name), "%s", hole_s.value);
        return editor_result_value(true);
    }
    if(is_hole && matches != 1) goto missing;
    EditorItemKind kind = is_hole ? EDITOR_ITEM_SOFT_HOLE : EDITOR_ITEM_SOFT_AREA;
    uint32_t item = is_hole ? hole->id : area->id;
    if(strcmp(arguments[operation], "delete") == 0 && operation + 1 == count) {
        *command = (EditorCommand){.type = EDITOR_COMMAND_ITEM_REMOVE,
            .data.item_remove = {kind, object->id, body->id, item, is_hole ? area->id : 0}};
        return editor_result_value(true);
    }
    if(strcmp(arguments[operation], "rename") == 0 && operation + 2 == count) {
        *command = (EditorCommand){.type = EDITOR_COMMAND_ITEM_RENAME,
            .data.item_rename = {.kind = kind, .object = object->id, .parent = body->id,
                .item = item, .index = is_hole ? area->id : 0}};
        snprintf(command->data.item_rename.name, sizeof(command->data.item_rename.name), "%s", arguments[operation + 1]);
        return editor_result_value(true);
    }
    if(strcmp(arguments[operation], "--property") != 0 || operation + 1 >= count) goto invalid;
    const char *property = arguments[operation + 1];
    int at = operation + 2;
    if(strcmp(property, "nodes") == 0 || strcmp(property, "node-ids") == 0) {
        if(count - at > SOFT_BODY_MAX_NODES) goto invalid;
        *command = (EditorCommand){.type = EDITOR_COMMAND_SOFT_AREA_LOOP_SET,
            .data.soft_area_loop = {.object = object->id, .body = body->id, .area = area->id,
                .hole = is_hole ? hole->id : 0}};
        for(int i = at; i < count; i += 1) {
            uint32_t node = 0; size_t found = 0;
            AreaCliSelector selector = {arguments[i], strcmp(property, "node-ids") == 0};
            for(size_t n = 0; n < body->node_count; n += 1)
                if(area_cli_match(selector, body->nodes[n].name, body->nodes[n].id)) { node = body->nodes[n].id; found++; }
            if(found != 1) goto missing;
            command->data.soft_area_loop.loop.nodes[command->data.soft_area_loop.loop.node_count++] = node;
        }
        return editor_result_value(true);
    }
    if(is_hole || at + 1 != count) goto invalid;
    const char *value = arguments[at]; uint32_t number;
    if(strcmp(property, "order") == 0 && cli_input_uint_parse(value, &number)) {
        *command = (EditorCommand){.type = EDITOR_COMMAND_SOFT_AREA_ORDER_SET,
            .data.soft_area_order = {object->id, body->id, area->id, number}};
        return editor_result_value(true);
    }
    if(strcmp(property, "color") == 0) {
        char *end; errno = 0; unsigned long parsed = strtoul(value, &end, 16);
        if(value[0] == '\0' || *end != '\0' || errno != 0 || parsed > UINT32_MAX) goto invalid;
        *command = (EditorCommand){.type = EDITOR_COMMAND_PROPERTY_SET,
            .data.property_set = {EDITOR_ITEM_SOFT_AREA, object->id, body->id, area->id, 0,
                EDITOR_PROPERTY_COLOR, EDITOR_PROPERTY_VALUE_UINT, {.integer = (uint32_t)parsed}}};
        return editor_result_value(true);
    }
    if(strcmp(property, "visibility") == 0) {
        bool visible; if(!cli_input_bool_parse(value, &visible)) goto invalid;
        *command = (EditorCommand){.type = EDITOR_COMMAND_VISIBILITY,
            .data.visibility = {EDITOR_VISIBILITY_SOFT_AREA, object->id, body->id, area->id, visible}};
        return editor_result_value(true);
    }
    if(strcmp(property, "layer") == 0 || strcmp(property, "layer-id") == 0) {
        EditorGraphicsLayerBinding binding = {0}; bool inherited = strcmp(value, "inherit") == 0;
        if(!inherited && strcmp(property, "layer-id") == 0) {
            if(!cli_input_uint_parse(value, &binding.layer) || binding.layer == 0) goto invalid;
        } else if(!inherited) {
            char *end; errno = 0; long parsed = strtol(value, &end, 10);
            if(value[0] == '\0' || *end != '\0' || errno != 0 || parsed < INT_MIN || parsed > INT_MAX) goto invalid;
            binding.value = (int)parsed;
        }
        *command = (EditorCommand){.type = EDITOR_COMMAND_SOFT_AREA_LAYER_SET,
            .data.soft_area_layer = {object->id, body->id, area->id, binding, inherited}};
        return editor_result_value(true);
    }
invalid:
    return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "Area/hole syntax: --object <name> --soft-body <name> --area <name> [--hole <name>] "
        "add|delete|rename <name>|--property nodes|node-ids|color|visibility|layer|layer-id|order <values>");
missing:
    return editor_result_error(EDITOR_ERROR_NOT_FOUND,
        "Area command selector is missing or ambiguous; use explicit --object-id, --soft-body-id, --area-id, --hole-id, or node-ids");
}

static EditorResult cli_area_command_write(const EditorProject *project, const EditorCommand *command,
    const EditorCommandResult *result, const char *path,
        char *output, size_t capacity, bool *handled) {
    EditorObjectId object = 0; EditorSoftBodyId body = 0; EditorSoftAreaId area = 0;
    EditorSoftHoleId hole = 0; bool add = false;
    const char *operation = "--property", *property = NULL, *value = NULL, *name = NULL;
    char number[32]; size_t used = 0;
    *handled = true;
    switch(command->type) {
    case EDITOR_COMMAND_SOFT_AREA_LOOP_SET:
        object = command->data.soft_area_loop.object; body = command->data.soft_area_loop.body;
        area = command->data.soft_area_loop.area; hole = command->data.soft_area_loop.hole;
        property = "node-ids"; break;
    case EDITOR_COMMAND_SOFT_AREA_ORDER_SET:
        object = command->data.soft_area_order.object; body = command->data.soft_area_order.body;
        area = command->data.soft_area_order.area; property = "order";
        snprintf(number, sizeof(number), "%u", command->data.soft_area_order.index); value = number; break;
    case EDITOR_COMMAND_SOFT_AREA_LAYER_SET:
        object = command->data.soft_area_layer.object; body = command->data.soft_area_layer.body;
        area = command->data.soft_area_layer.area;
        property = command->data.soft_area_layer.binding.layer == 0 ? "layer" : "layer-id";
        if(command->data.soft_area_layer.inherited) { property = "layer"; value = "inherit"; }
        else {
            if(command->data.soft_area_layer.binding.layer == 0)
                snprintf(number, sizeof(number), "%d", command->data.soft_area_layer.binding.value);
            else snprintf(number, sizeof(number), "%u", command->data.soft_area_layer.binding.layer);
            value = number;
        } break;
    case EDITOR_COMMAND_ITEM_ADD:
        if(command->data.item_add.kind != EDITOR_ITEM_SOFT_AREA && command->data.item_add.kind != EDITOR_ITEM_SOFT_HOLE) goto unhandled;
        object = command->data.item_add.object; body = command->data.item_add.parent;
        area = command->data.item_add.first; hole = command->data.item_add.kind == EDITOR_ITEM_SOFT_HOLE;
        operation = "add"; add = true; name = command->data.item_add.name;
        if(name[0] == '\0' && result != NULL && result->kind == ERROR_RESULT_VALUE) {
            const EditorObject *owner = NULL;
            for(size_t i = 0; i < project->object_count; i += 1)
                if(project->objects[i].id == object) owner = &project->objects[i];
            if(owner != NULL) for(size_t b = 0; b < owner->soft_body_count; b += 1)
                if(owner->soft_body_items[b].id == body)
                    for(size_t a = 0; a < owner->soft_body_items[b].area_count; a += 1) {
                        const EditorSoftArea *candidate = &owner->soft_body_items[b].areas[a];
                        if(!hole && candidate->id == result->result.object) name = candidate->name;
                        if(hole && candidate->id == area)
                            for(uint32_t h = 0; h < candidate->hole_count; h += 1)
                                if(candidate->holes[h].id == result->result.object) name = candidate->holes[h].name;
                    }
        }
        if(name[0] == '\0') goto full;
        break;
    case EDITOR_COMMAND_ITEM_REMOVE:
        if(command->data.item_remove.kind != EDITOR_ITEM_SOFT_AREA && command->data.item_remove.kind != EDITOR_ITEM_SOFT_HOLE) goto unhandled;
        object = command->data.item_remove.object; body = command->data.item_remove.parent;
        hole = command->data.item_remove.kind == EDITOR_ITEM_SOFT_HOLE ? command->data.item_remove.item : 0;
        area = hole ? command->data.item_remove.index : command->data.item_remove.item;
        operation = "delete"; break;
    case EDITOR_COMMAND_ITEM_RENAME:
        if(command->data.item_rename.kind != EDITOR_ITEM_SOFT_AREA && command->data.item_rename.kind != EDITOR_ITEM_SOFT_HOLE) goto unhandled;
        object = command->data.item_rename.object; body = command->data.item_rename.parent;
        hole = command->data.item_rename.kind == EDITOR_ITEM_SOFT_HOLE ? command->data.item_rename.item : 0;
        area = hole ? command->data.item_rename.index : command->data.item_rename.item;
        operation = "rename"; value = command->data.item_rename.name; break;
    case EDITOR_COMMAND_VISIBILITY:
        if(command->data.visibility.kind != EDITOR_VISIBILITY_SOFT_AREA) goto unhandled;
        object = command->data.visibility.object; body = command->data.visibility.parent;
        area = command->data.visibility.item; property = "visibility";
        value = command->data.visibility.visible ? "true" : "false"; break;
    case EDITOR_COMMAND_PROPERTY_SET:
        if(command->data.property_set.kind != EDITOR_ITEM_SOFT_AREA) goto unhandled;
        object = command->data.property_set.object; body = command->data.property_set.parent;
        area = command->data.property_set.item; property = "color";
        snprintf(number, sizeof(number), "%08x", command->data.property_set.value.integer); value = number; break;
    default: goto unhandled;
    }
    output[0] = '\0';
#define AREA_ADD(v) do { if(!cli_token_add(output, capacity, &used, v)) goto full; } while(0)
#define AREA_ID(flag, id) do { char buffer[32]; snprintf(buffer, sizeof(buffer), "%u", id); AREA_ADD(flag); AREA_ADD(buffer); } while(0)
    AREA_ADD("rohr-cli"); AREA_ADD("--project"); AREA_ADD(path);
    AREA_ID("--object-id", object); AREA_ID("--soft-body-id", body);
    if(add && !hole) { AREA_ADD("--area"); AREA_ADD(name); }
    else { AREA_ID("--area-id", area); }
    if(hole) { if(add) { AREA_ADD("--hole"); AREA_ADD(name); } else { AREA_ID("--hole-id", hole); } }
    AREA_ADD(operation);
    if(property != NULL) AREA_ADD(property);
    if(value != NULL) AREA_ADD(value);
    if(command->type == EDITOR_COMMAND_SOFT_AREA_LOOP_SET)
        for(uint32_t i = 0; i < command->data.soft_area_loop.loop.node_count; i += 1) {
            char buffer[32]; snprintf(buffer, sizeof(buffer), "%u", command->data.soft_area_loop.loop.nodes[i]); AREA_ADD(buffer);
        }
#undef AREA_ADD
#undef AREA_ID
    return editor_result_value(true);
unhandled:
    *handled = false; return editor_result_value(false);
full:
    return editor_result_error(EDITOR_ERROR_CAPACITY, "Area command exceeds output capacity");
}

EditorResult editor_command_cli_standard_parse(const EditorProject *project,
        int count, char **arguments, const char **path, EditorCommand *command) {
    CliInput input;
    char *normalized[CLI_MAX];
    int n = 0;
    const char *operation;
    const char *property = NULL;
    const char *domain;
    bool input_handled = false;
    EditorResult area_result = cli_area_command_parse(project, count, arguments, path, command, &input_handled);
    if(input_handled) return area_result;
    EditorResult input_result = cli_input_command_parse(project, count, arguments,
        path, command, &input_handled);
    if(input_handled) return input_result;
    EditorResult result = cli_input_get(count, arguments, &input);
    if(editor_result_check(result)) return result;
    operation = arguments[input.terminal];
    domain = input.domain;
    if(strcmp(operation, "--property") == 0) {
        if(input.terminal + 1 >= count) return editor_result_error(
            EDITOR_ERROR_INVALID_ARGUMENT, "--property requires a name");
        property = arguments[input.terminal + 1];
        if(strcmp(property, "navigation") == 0) domain = "navigation";
        else if(strcmp(property, "camera") == 0 || strcmp(property, "coordinates") == 0)
            domain = "viewport";
        else if(strcmp(property, "anchor-a") == 0 || strcmp(property, "anchor-b") == 0)
            domain = "joint";
        else if(strcmp(property, "rigid-body") == 0) domain = "anchor";
        else if(strcmp(property, "body") == 0 && domain == NULL)
            domain = "animated-sprite";
        else if(strcmp(property, "node-a") == 0 || strcmp(property, "node-b") == 0)
            domain = "soft-beam";
    }
    if(domain == NULL) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "Operation requires a selected target");
    *path = input.path;
    if(strcmp(operation, "add") == 0 &&
            (strcmp(domain, "object") == 0 || strcmp(domain, "collision-mask") == 0)) {
        normalized[n++] = arguments[0]; normalized[n++] = "object";
        if(strcmp(domain, "collision-mask") == 0) normalized[1] = "collision-mask";
        normalized[n++] = "add"; normalized[n++] = (char *)input.path;
        normalized[n++] = (char *)input.target_name;
        for(int i = input.terminal + 1; i < count; i += 1) normalized[n++] = arguments[i];
        return editor_command_cli_parse(n, normalized, path, command);
    }
    normalized[n++] = arguments[0]; normalized[n++] = (char *)domain;
    normalized[n++] = (char *)(strcmp(operation, "--property") == 0 ?
        (strcmp(property, "visibility") == 0 ? "visibility" :
         strcmp(property, "position") == 0 &&
            (strcmp(domain, "rigid-body") == 0 || strcmp(domain, "soft-body") == 0 ||
             strcmp(domain, "anchor") == 0) ? "transform" :
         strcmp(property, "position") == 0 &&
            (strcmp(domain, "sprite") == 0 ||
             strcmp(domain, "animated-sprite") == 0) ? "set" :
         strcmp(property, "position") == 0 ? "position" :
         strcmp(property, "rotation") == 0 &&
            (strcmp(domain, "sprite") == 0 ||
             strcmp(domain, "animated-sprite") == 0) ? "set" :
         strcmp(property, "rotation") == 0 ? "transform" :
         strcmp(property, "transform") == 0 ? "transform" :
         strcmp(property, "origin") == 0 ? "origin" :
         strcmp(property, "auto-shape") == 0 ? "auto-shape" :
         strcmp(property, "camera") == 0 ? "camera" :
         strcmp(property, "coordinates") == 0 ? "coordinates" :
         strcmp(property, "navigation") == 0 ? "set" :
         strcmp(property, "category") == 0 || strcmp(property, "collide-with") == 0 ?
            "filter" :
         strcmp(property, "anchor-a") == 0 || strcmp(property, "anchor-b") == 0 ||
         strcmp(property, "rigid-body") == 0 || strcmp(property, "node-a") == 0 ||
         strcmp(property, "node-b") == 0 || strcmp(property, "body") == 0 ?
            "connect" : "set") : operation);
    normalized[n++] = (char *)input.path;
    for(size_t i = 0; i < input.selector_count; i += 2) {
        if(strcmp(operation, "add") == 0 && cli_selector_target_check(&input, i)) continue;
        normalized[n++] = input.selectors[i]; normalized[n++] = input.selectors[i + 1];
    }
    if(strcmp(operation, "add") == 0 &&
            (strcmp(domain, "animated-sprite") == 0 ||
                strcmp(domain, "sprite") == 0))
        normalized[n++] = (char *)input.target_name;
    if(strcmp(operation, "--property") == 0) {
        const char *action = normalized[2];
        if(strcmp(action, "set") == 0 || strcmp(action, "filter") == 0)
            normalized[n++] = (char *)property;
        else if(strcmp(action, "connect") == 0) normalized[n++] = (char *)property;
        if(strcmp(property, "rotation") == 0 &&
                strcmp(action, "transform") == 0) {
            if(input.terminal + 2 >= count || input.terminal + 3 != count)
                return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                    "rotation requires one value");
            normalized[n++] = "0"; normalized[n++] = "0";
            normalized[n++] = arguments[input.terminal + 2];
        } else {
            for(int i = input.terminal + 2; i < count; i += 1)
                normalized[n++] = arguments[i];
            if(strcmp(property, "position") == 0 && strcmp(action, "transform") == 0)
                normalized[n++] = "0";
        }
    } else for(int i = input.terminal + 1; i < count; i += 1)
        normalized[n++] = arguments[i];
    result = editor_command_cli_named_parse(project, n, normalized, path, command);
    if(!editor_result_check(result) && strcmp(operation, "--property") == 0 &&
            (strcmp(property, "position") == 0 || strcmp(property, "rotation") == 0) &&
            (strcmp(domain, "rigid-body") == 0 || strcmp(domain, "soft-body") == 0 ||
             strcmp(domain, "anchor") == 0)) {
        EditorObject *object = NULL;
        for(size_t i = 0; i < project->object_count; i += 1) {
            EditorObjectId id = strcmp(domain, "rigid-body") == 0 ?
                command->data.rigid_body_transform.object :
                strcmp(domain, "soft-body") == 0 ? command->data.soft_body_transform.object :
                command->data.anchor_transform.object;
            if(project->objects[i].id == id) object = (EditorObject *)&project->objects[i];
        }
        if(strcmp(domain, "rigid-body") == 0) {
            EditorRigidBody *body = editor_project_rigid_body_get(object,
                command->data.rigid_body_transform.body);
            if(body != NULL) {
                if(strcmp(property, "position") == 0)
                    command->data.rigid_body_transform.rotation = body->rotation;
                else command->data.rigid_body_transform.position = body->position;
            }
        } else if(strcmp(domain, "soft-body") == 0 && object != NULL) {
            EditorSoftBody *body = NULL;
            for(size_t i = 0; i < object->soft_body_count; i += 1)
                if(object->soft_body_items[i].id == command->data.soft_body_transform.body)
                    body = &object->soft_body_items[i];
            if(body != NULL) {
                if(strcmp(property, "position") == 0)
                    command->data.soft_body_transform.rotation = body->rotation;
                else command->data.soft_body_transform.position = body->position;
            }
        } else if(strcmp(domain, "anchor") == 0) {
            EditorAnchor *anchor = editor_project_anchor_get(object,
                command->data.anchor_transform.anchor);
            if(anchor != NULL) {
                if(strcmp(property, "position") == 0)
                    command->data.anchor_transform.rotation = anchor->rotation;
                else command->data.anchor_transform.position = anchor->position;
            }
        }
    }
    if(!editor_result_check(result) && strcmp(operation, "add") == 0)
        snprintf(command->data.item_add.name, sizeof(command->data.item_add.name),
            "%s", input.target_name);
    return result;
}
