/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#if !defined(_WIN32) && !defined(_XOPEN_SOURCE)
#define _XOPEN_SOURCE 700
#endif

#include "editor_command.h"
#include "editor_config.h"
#include "editor_build.h"
#include <signal.h>
#include <errno.h>
#include "editor_document.h"
#include "editor_workspace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ROHR_DEVELOPMENT_SOURCE_DIR
#define ROHR_DEVELOPMENT_SOURCE_DIR ""
#endif

#if defined(_WIN32)
#include <direct.h>
#endif

static int cli_error(EditorResult result) {
    editor_result_stderr_print(result);
    return 1;
}

static void cli_usage_print(void);

typedef struct CliHelpDomain {
    const char *selector;
    const char *name;
    const char *properties;
    const char *example;
    unsigned int depth;
} CliHelpDomain;

static const CliHelpDomain cli_help_domains[] = {
    {"--force", "force", "description <JSON>, mode <persistent|initial>, magnitude <number>, target <JSON>, location <JSON>, orientation <JSON>; operations add, get, list, rename, duplicate, remove",
        "--object car --force thrust --property magnitude 10", 1},
    {"--torque", "torque", "description <JSON>, mode <persistent|initial>, magnitude <number>, target <JSON>, marker_offset <{\"x\":number,\"y\":number}>, visible <bool>, visible_in_app <bool>; operations add, get, list, rename, duplicate, remove",
        "--object car --torque drive --property magnitude 10", 1},
    {"--camera", "camera guide", "layer <signed integer>, layer-id <id>",
        "--object car --camera view --property layer 10", 1},
    {"--object", "object", "position <x> <y>, visibility <true|false>",
        "--object car --property position 10 20", 0},
    {"--body", "rigid body",
        "position <x> <y>, rotation <degrees>, initial-angular-velocity <degrees/s>, origin <x> <y>, center-of-mass automatic|explicit <x> <y>, mass <number>, "
        "friction <number>, restitution <0..1>, gravity <true|false>, "
        "static <true|false>, rotation-locked <true|false>, collision <true|false>, "
        "particle <true|false>, particle-radius <number>, "
        "particle-origin-x <number>, particle-origin-y <number>, "
        "particle-auto-fit <true|false>, outline-color <hex>, surface-color <hex>, "
        "particle-ring-color <hex>, particle-fill-color <hex>, visibility <true|false>, "
        "category <mask> <true|false>, collide-with <mask> <true|false>",
        "--object car --body chassis --property mass 5", 1},
    {"--hitbox", "hitbox", "visibility <true|false>, "
        "auto-shape <shape> <triangle-kind> <width> <height> <radius> <apex-offset>",
        "--body chassis --hitbox chassis_hitbox --property visibility true", 2},
    {"--joint", "joint", "visibility <true|false>, kind <revolute|weld|spring>, "
        "visual-size <0.25..3>, rest-length <number>, stiffness <number>, "
        "damping <number>, anchor-a <anchor|none>, anchor-b <anchor|none>",
        "--joint suspension --property stiffness 40", 1},
    {"--anchor", "anchor", "position <x> <y>, rotation <degrees>, "
        "position-follows-body <true|false>, rotation-follows-body <true|false>, "
        "rigid-body <body|none>, visibility <true|false>",
        "--anchor wheel_anchor --property position 12 8", 1},
    {"--soft-body", "soft body", "position <x> <y>, rotation <degrees>, initial-angular-velocity <degrees/s>, "
        "origin <x> <y>, mass <number>, center-of-mass <x> <y>, node-color <hex>, beam-color <hex>, "
        "visibility <true|false>, auto-shape <shape> <triangle-kind> <width> "
        "<height> <radius> <apex-offset>",
        "--soft-body cloth --property beam-color ff8800ff", 1},
    {"--node", "soft-body node", "position <x> <y>, mass <number>, "
        "friction <number>, restitution <0..1>, gravity <true|false>, "
        "collision <true|false>, node-radius <number>, color <hex>, "
        "visibility <true|false>",
        "--soft-body cloth --node corner --property mass 1", 2},
    {"--beam", "soft-body beam", "friction <number>, restitution <0..1>, "
        "stiffness <number>, damping <number>, "
        "color <hex>, visibility <true|false>, node-a <node|none>, node-b <node|none>",
        "--soft-body cloth --beam edge_1 --property damping 0.2", 2},
    {"--area", "soft-body area", "nodes <ordered names...>, node-ids <ordered ids...>, "
        "color <rrggbbaa>, visibility <true|false>, layer <number|inherit>, layer-id <id>, order <zero-based index>",
        "--object cloth --soft-body fabric --area patch --property nodes top_left top_right bottom_right bottom_left", 2},
    {"--hole", "soft-body area hole", "nodes <ordered names...>, node-ids <ordered ids...> (maximum 16 holes per area)",
        "--object cloth --soft-body fabric --area patch --hole window add", 3},
    {"--vertex", "hitbox vertex", "position <x> <y>, position-locked <true|false>",
        "--body chassis --vertex vertex_1 --property position 4 8", 3},
    {"--line", "hitbox line", "length <number>",
        "--body chassis --line line_1 --property length 20", 3},
    {"--sprite", "sprite", "path <file>, position <x> <y>, rotation <degrees>, "
        "size <width> <height>, body <body|none>, "
        "follow-body-rotation <true|false>, visibility <true|false>",
        "--object car --sprite wheel --property path assets/wheel.png", 1},
    {"--animated-sprite", "animated sprite", "body <body|none>, "
        "position <x> <y>, rotation <degrees>, scale <x> <y>, "
        "timing <ticks> <seconds>, starting-frame <index>, "
        "direction <left|right>, follow-body-rotation <true|false>, "
        "visibility <true|false>",
        "--object car --animated-sprite wheel_animation --property scale 2 2", 2},
    {"--controller", "input controller", "controller-set <name> <true|false>",
        "--controller gameplay add true", 0},
    {"--action", "input action",
        "action-set <name> button <momentary|persistent> <initial-state>, or "
        "action-set <name> <axis-1d|axis-2d>",
        "--controller gameplay --action move add axis-2d", 1},
    {"--binding", "input binding",
        "binding-set <name> <source> <input> <modifiers> [<scale> <inverted> "
        "| <affects-x> <affects-y> <scale-x> <scale-y> "
        "<inverted-x> <inverted-y> "
        "<direction-x> <direction-y>]",
        "--controller gameplay --action move --binding move_up "
        "binding-set move_forward key W 0 false true 1 1 false false 0 -1", 2}
};

static bool cli_help_flag_check(const char *argument) {
    return argument != NULL && (strcmp(argument, "--help") == 0 ||
        strcmp(argument, "-help") == 0 || strcmp(argument, "-h") == 0 ||
        strcmp(argument, "--h") == 0);
}

static const CliHelpDomain *cli_help_domain_get(int count, char **arguments) {
    const CliHelpDomain *selected = NULL;
    for(int i = 1; i < count; i += 1) {
        for(size_t j = 0; j < sizeof(cli_help_domains) / sizeof(cli_help_domains[0]);
                j += 1) {
            if(strcmp(arguments[i], cli_help_domains[j].selector) == 0 ||
                    (strncmp(arguments[i], cli_help_domains[j].selector,
                        strlen(cli_help_domains[j].selector)) == 0 &&
                     strcmp(arguments[i] + strlen(cli_help_domains[j].selector),
                        "-id") == 0) ||
                    (strcmp(cli_help_domains[j].selector, "--line") == 0 &&
                     strcmp(arguments[i], "--line-index") == 0) ||
                    (strcmp(cli_help_domains[j].selector, "--binding") == 0 &&
                     strcmp(arguments[i], "--binding-index") == 0)) {
                if(selected == NULL || cli_help_domains[j].depth >= selected->depth)
                    selected = &cli_help_domains[j];
            }
        }
    }
    return selected;
}

static const char *cli_help_property_get(int count, char **arguments) {
    for(int i = 1; i + 1 < count; i += 1)
        if(strcmp(arguments[i], "--property") == 0 &&
                !cli_help_flag_check(arguments[i + 1])) return arguments[i + 1];
    return NULL;
}

static void cli_help_print(int count, char **arguments) {
    const CliHelpDomain *domain = cli_help_domain_get(count, arguments);
    const char *property = cli_help_property_get(count, arguments);
    cli_usage_print();
    puts("\nGeneral examples:\n"
        "  rohr-cli --project ./objects/project.rohr.json --object car "
            "--body chassis --property mass 5\n"
        "  rohr-cli --project ./game validate\n"
        "  rohr-cli --project ./game build");
    if(domain == NULL) {
        puts("\nSelectors:\n"
            "  --object, --body, --hitbox, --joint, --anchor, --soft-body,\n"
            "  --node, --beam, --area, --hole, --vertex, --line, --sprite,\n"
            "  --animated-sprite, --frame-index, --controller, --action,\n"
            "  --binding, --binding-index, --force, --torque, --camera\n"
            "  Every named selector also accepts its -id form.");
        return;
    }
    printf("\nSelected depth: %s\n", domain->name);
    if(property != NULL) printf("Value required:\n  --property %s\n", property);
    printf("What can be set:\n  %s\n\nExample:\n  rohr-cli %s\n",
        domain->properties, domain->example);
    if(strcmp(domain->selector, "--controller") == 0)
        puts("Operations at this depth:\n"
            "  add [true|false], delete, controller-set <name> <true|false>");
    else if(strcmp(domain->selector, "--action") == 0)
        puts("Operations at this depth:\n"
            "  add button <momentary|persistent> <initial-state>, "
            "add <axis-1d|axis-2d>, delete, action-set; use --binding to add bindings");
    else if(strcmp(domain->selector, "--binding") == 0)
        puts("Operations at this depth:\n"
            "  add <binding>, binding-set <new-name> <binding>, delete\n"
            "  Button: source input modifiers\n"
            "  Axis 1D: source input modifiers scale inverted\n"
            "  Axis 2D: source input modifiers affects-x affects-y scale-x "
            "scale-y inverted-x inverted-y direction-x direction-y");
    else if(!strcmp(domain->selector, "--force") || !strcmp(domain->selector, "--torque"))
        puts("Operations at this depth:\n  add [name], get, list, set <JSON>, rename <name>, duplicate [name], delete");
    else puts("Operations at this depth:\n  rename <new-name>, delete");
    const char *layer_selectors[] = {"--body", "--anchor", "--camera", "--joint",
        "--force", "--torque", "--sprite", "--animated-sprite", "--soft-body", "--node", "--beam", "--area"};
    for(size_t i = 0; i < sizeof(layer_selectors)/sizeof(*layer_selectors); ++i)
        if(!strcmp(domain->selector, layer_selectors[i])) {
            puts("Appearance layer properties:\n  layer <signed integer>, layer-id <named layer id>");
            if(!strcmp(domain->selector, "--node") || !strcmp(domain->selector, "--beam") ||
                    !strcmp(domain->selector, "--area"))
                puts("  layer inherit, layer-inherit <true|false>");
            break;
        }
    if(!strcmp(domain->selector, "--force") || !strcmp(domain->selector, "--torque") ||
            !strcmp(domain->selector, "--joint")) {
        puts("Visual properties:\n"
            "  visual-fill-color <rrggbbaa>, visual-border-color <rrggbbaa>,\n"
            "  visual-border-thickness <nonnegative number>, visual-scale-x <number>,\n"
            "  visual-scale-y <number>, visual-offset-x <number>, visual-offset-y <number>,\n"
            "  visual-rotation <clockwise degrees>");
        if(!strcmp(domain->selector, "--joint"))
            puts("  Spring endpoint properties use visual-a- and visual-b- prefixes.");
    }
}

static bool cli_help_request_check(int count, char **arguments) {
    for(int i = 1; i < count; i += 1)
        if(cli_help_flag_check(arguments[i])) return true;
    if(count < 2) return true;
    if(strcmp(arguments[count - 1], "--property") == 0) return true;
    for(int i = 1; i + 1 < count; i += 1)
        if(strcmp(arguments[i], "--property") == 0 && i + 2 == count) return true;
    if(cli_help_domain_get(count, arguments) != NULL &&
            cli_help_property_get(count, arguments) == NULL) {
        bool operation = false;
        for(int i = 1; i < count; i += 1) {
            if(arguments[i][0] == '-') {
                if((!strcmp(arguments[i], "--force") || !strcmp(arguments[i], "--torque")) && i + 1 < count &&
                        (!strcmp(arguments[i+1], "list") || !strcmp(arguments[i+1], "add"))) return false;
                if(i + 1 < count) i += 1;
            } else {
                operation = true;
                break;
            }
        }
        if(!operation) return true;
    }
    return false;
}

static void cli_usage_print(void) {
    puts("usage:\n"
        "  rohr-cli [--project <project.rohr.json>] <selectors> <operation>\n"
        "  Selectors are order-independent and precede the operation.\n"
        "  Properties: --property <name> <values...>\n"
        "  Structure: add, delete, or rename [new-name]\n"
        "  Project: rohr-cli [--project <project-directory>] "
            "<operation>\n"
        "  Project operations: create, validate, generate-c, compile, build\n"
        "  Scene operations: build-scene, simulate-scene\n"
        "  Scene selectors: --object <name> or --object-id <id>; --viewport <name> or --viewport-id <id>\n"
        "  Scene defaults: all objects and the first viewport\n"
        "  Example: rohr-cli --object car --body chassis --property mass 5\n"
        "  Example: rohr-cli --body chassis --property position 10 20\n"
        "  Entity settings: lifetime-time-enabled, lifetime-time, lifetime-tick,\n"
        "    angular-acceleration, angular-maximum, gameplay-target, axis-lock-enabled,\n"
        "    transform-driver and transform-position (see docs/description_api_contract.md)\n"
        "  Explicit placement: rohr-cli joint place <project-file> <object-id> <joint-id>\n"
        "  Audio recipe: rohr-cli audio add <project-file> <music-bool> <name> <asset-path>\n"
        "  Application window: rohr-cli application-window set <project-file>\n"
        "    <enabled> <mode> <width> <height> <logical-width> <logical-height> <auto-aspect>\n"
        "  Example: rohr-cli --project ./game validate\n"
        "  Example: rohr-cli --project ./game create");
}


static bool cli_sdk_root_get(char *output, size_t capacity) {
    const char *base = SDL_GetBasePath();
    char root[EDITOR_WORKSPACE_PATH_MAX * 2];
    char config[EDITOR_WORKSPACE_PATH_MAX * 2];
    size_t length;
    SDL_PathInfo info;
    if(output == NULL || capacity == 0) return false;
    if(base != NULL && strlen(base) < sizeof(root)) {
        snprintf(root, sizeof(root), "%s", base);
        length = strlen(root);
        while(length > 0 && (root[length - 1] == '/' || root[length - 1] == '\\'))
            root[--length] = '\0';
        while(length > 0 && root[length - 1] != '/' && root[length - 1] != '\\')
            length -= 1;
        if(length > 0) root[length - 1] = '\0';
        const char *library_directories[] = {"lib", "lib64"};
        for(size_t i = 0; i < sizeof(library_directories) /
                sizeof(library_directories[0]); i += 1) {
            int count = snprintf(config, sizeof(config), "%s/%s/cmake/Rohr/RohrConfig.cmake",
                root, library_directories[i]);
            if(count >= 0 && (size_t)count < sizeof(config) &&
                    SDL_GetPathInfo(config, &info) && info.type == SDL_PATHTYPE_FILE) {
                count = snprintf(output, capacity, "%s", root);
                return count >= 0 && (size_t)count < capacity;
            }
        }
    }
    if(ROHR_DEVELOPMENT_SOURCE_DIR[0] != '\0') {
        int count = snprintf(output, capacity, "%s", ROHR_DEVELOPMENT_SOURCE_DIR);
        return count >= 0 && (size_t)count < capacity;
    }
    output[0] = '\0';
    return false;
}

static bool cli_rohr_cmake_option_get(char *output, size_t capacity) {
    char sdk[EDITOR_WORKSPACE_PATH_MAX * 2];
    int count;
    if(cli_sdk_root_get(sdk, sizeof(sdk))) {
        count = snprintf(output, capacity, "-DCMAKE_PREFIX_PATH=%s", sdk);
        return count >= 0 && (size_t)count < capacity;
    }
    if(ROHR_DEVELOPMENT_SOURCE_DIR[0] == '\0') return false;
    count = snprintf(output, capacity, "-DROHR_ENGINE_SOURCE_ROOT=%s",
        ROHR_DEVELOPMENT_SOURCE_DIR);
    return count >= 0 && (size_t)count < capacity;
}

static volatile sig_atomic_t cli_build_interrupted;
static void cli_build_interrupt(int signal_number) { cli_build_interrupted = signal_number; }
static void cli_build_output(void *context, const char *bytes, size_t count) {
    (void)context;
    (void)fwrite(bytes, 1, count, stdout);
    (void)fflush(stdout);
}
static int cli_process_run(const char *directory, const char *const *arguments) {
    EditorBuildProcess *process = editor_build_process_create();
    int exit_code = 1;
    if(!process) return cli_error(editor_result_error(EDITOR_ERROR_CAPACITY, "Could not allocate build process"));
    cli_build_interrupted = 0;
    void (*previous_int)(int) = signal(SIGINT, cli_build_interrupt);
    void (*previous_term)(int) = signal(SIGTERM, cli_build_interrupt);
    EditorResult result = editor_build_process_start(process, directory, arguments);
    bool finished = false;
    while(!editor_result_check(result) && !finished) {
        if(cli_build_interrupted) editor_build_process_interrupt(process);
        result = editor_build_process_poll(process, cli_build_output, NULL, &finished, &exit_code);
        if(!finished) SDL_Delay(1);
    }
    editor_build_process_destroy(process);
    (void)signal(SIGINT, previous_int); (void)signal(SIGTERM, previous_term);
    if(editor_result_check(result)) return cli_error(result);
    return cli_build_interrupted ? 128 + cli_build_interrupted : exit_code;
}
static int cli_project_cmake_run(const EditorWorkspace *workspace,
        const EditorConfig *config, bool configure, bool scene_only) {
    char root[EDITOR_WORKSPACE_PATH_MAX * 2];
    char build[EDITOR_WORKSPACE_PATH_MAX * 2];
    char rohr_option[EDITOR_WORKSPACE_PATH_MAX * 2];
    const char *configure_arguments[] = {
        "cmake", "-S", root, "-B", build, rohr_option, NULL};
    const char *compile_arguments[] = {"cmake", "--build", build, NULL};
    char configured_arguments[EDITOR_CONFIG_ARGUMENT_MAX]
        [EDITOR_CONFIG_ARGUMENT_LENGTH_MAX];
    const char *configured_output[EDITOR_CONFIG_ARGUMENT_MAX + 1];
    const char *const *process_arguments;
    const EditorConfigCommand *configured_command;
    char sdk[EDITOR_WORKSPACE_PATH_MAX * 2] = {0};
    EditorWorkspace absolute;
    EditorResult resolved = editor_build_workspace_get(workspace, &absolute);
    if(editor_result_check(resolved)) return cli_error(resolved);
    workspace = &absolute;
    SDL_strlcpy(root, workspace->directory, sizeof(root));
    resolved = editor_workspace_path_get(workspace, EDITOR_PATH_BUILD, build, sizeof(build));
    if(editor_result_check(resolved)) return cli_error(resolved);
    configured_command = editor_config_command_get(config,
        EDITOR_CONFIG_FRONTEND_CLI, configure ? EDITOR_CONFIG_OPERATION_CONFIGURE :
        EDITOR_CONFIG_OPERATION_COMPILE);
    if(configure) {
        const EditorConfigCommand *compile = editor_config_command_get(config,
            EDITOR_CONFIG_FRONTEND_CLI, EDITOR_CONFIG_OPERATION_COMPILE);
        EditorResult prepared = editor_build_prepare(workspace,
            !configured_command || configured_command->count != 0, compile, cli_build_output, NULL);
        if(editor_result_check(prepared)) return cli_error(prepared);
    }
    if(configured_command != NULL && configured_command->count == 0) return 0;
    if(configured_command != NULL || (!configure && scene_only)) {
        EditorResult result;
        if(workspace->config.engine_directory[0]) {
            result = editor_workspace_path_get(workspace, EDITOR_PATH_ENGINE, sdk, sizeof(sdk));
            if(editor_result_check(result)) return cli_error(result);
        } else (void)cli_sdk_root_get(sdk, sizeof(sdk));
        char target[EDITOR_CONFIG_ARGUMENT_LENGTH_MAX];
        result = editor_build_target_name_get(workspace, scene_only ? EDITOR_BUILD_SCENE : EDITOR_BUILD_PROJECT, target, sizeof(target));
        if(editor_result_check(result)) return cli_error(result);
        result = !configure && scene_only ?
            editor_build_compile_arguments_get(configured_command, root, build, sdk, target, configured_arguments, configured_output) :
            editor_config_command_target_expand(configured_command, root, build, sdk, target, configured_arguments, configured_output);
        if(editor_result_check(result)) return cli_error(result);
        result = editor_config_workspace_arguments_expand(workspace, configured_arguments, configured_output);
        if(editor_result_check(result)) return cli_error(result);
        process_arguments = configured_output;
    } else {
        if(configure && !cli_rohr_cmake_option_get(rohr_option,
                sizeof(rohr_option))) return cli_error(editor_result_error(
            EDITOR_ERROR_FILE_IO, "Could not locate the Rohr SDK or source tree"));
        if(configure && workspace->config.engine_directory[0]) {
            resolved = editor_workspace_path_get(workspace, EDITOR_PATH_ENGINE, sdk, sizeof(sdk));
            if(editor_result_check(resolved)) return cli_error(resolved);
            if(snprintf(rohr_option, sizeof(rohr_option), "-DROHR_ENGINE_SOURCE_ROOT=%s", sdk) >= (int)sizeof(rohr_option))
                return cli_error(editor_result_error(EDITOR_ERROR_CAPACITY, "Engine path is too long"));
        }
        process_arguments = configure ? configure_arguments : compile_arguments;
    }
    int code = cli_process_run(root, process_arguments);
    if(!code && configure) {
        EditorResult completed = editor_build_cache_configured(workspace);
        if(editor_result_check(completed)) return cli_error(completed);
    }
    return code;
}

static int cli_project_compile(const EditorWorkspace *workspace,
        const EditorConfig *config) {
    int result = cli_project_cmake_run(workspace, config, true, false);
    return result == 0 ? cli_project_cmake_run(workspace, config, false, false) : result;
}

static EditorResult cli_project_config_load(EditorConfig *config,
        const EditorWorkspace *workspace) {
    char path[EDITOR_WORKSPACE_PATH_MAX * 2];
    EditorResult result;
    editor_config_init(config);
    result = editor_config_sdk_path_get(path, sizeof(path), "editor.lua", true);
    if(editor_result_check(result)) return result;
    result = editor_config_file_merge(config, path, true);
    if(editor_result_check(result)) return result;
    if(snprintf(path, sizeof(path), "%s/editor.lua", workspace->directory) >=
            (int)sizeof(path)) return editor_result_error(EDITOR_ERROR_CAPACITY,
        "Project editor.lua path is too long: %s", workspace->directory);
    return editor_config_file_merge(config, path, false);
}

static int cli_workspace_action_command(int count, char **arguments) {
    static EditorProject project;
    EditorWorkspace workspace = {0};
    EditorWorkspaceCommand load = {.type = EDITOR_WORKSPACE_COMMAND_LOAD};
    EditorWorkspaceCommand command = {0};
    const char *directory = ".";
    const char *operation = arguments[count - 1];
    bool project_set = false;
    EditorResult result;
    EditorConfig config;
    for(int i = 1; i + 1 < count; i += 1) {
        if(strcmp(arguments[i], "--project") == 0) {
            if(i + 1 >= count - 1) return cli_error(editor_result_error(
                EDITOR_ERROR_INVALID_ARGUMENT, "--project requires a path"));
            if(project_set) return cli_error(editor_result_error(
                EDITOR_ERROR_INVALID_ARGUMENT, "--project may only be specified once"));
            directory = arguments[++i];
            project_set = true;
        } else {
            return cli_error(editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "Unexpected project operation argument: %s", arguments[i]));
        }
    }
    if(strlen(directory) >= sizeof(load.directory))
        return cli_error(editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "Project operation path is invalid or too long"));
    if(strcmp(operation, "create") == 0) {
        command.type = EDITOR_WORKSPACE_COMMAND_CREATE;
        snprintf(command.directory, sizeof(command.directory), "%s", directory);
        result = editor_workspace_command_execute(&workspace, &project, &command);
        return editor_result_check(result) ? cli_error(result) : 0;
    }
    snprintf(load.directory, sizeof(load.directory), "%s", directory);
    result = editor_workspace_command_execute(&workspace, &project, &load);
    if(editor_result_check(result)) return cli_error(result);
    if(strcmp(operation, "validate") == 0) {
        puts("valid");
        return 0;
    }
    if(strcmp(operation, "generate-c") == 0 || strcmp(operation, "build") == 0) {
        command.type = EDITOR_WORKSPACE_COMMAND_GENERATE_C;
        snprintf(command.directory, sizeof(command.directory), "%s",
            workspace.directory);
        result = editor_workspace_command_execute(&workspace, &project, &command);
        if(editor_result_check(result)) return cli_error(result);
    }
    if(strcmp(operation, "generate-c") == 0) return 0;
    result = cli_project_config_load(&config, &workspace);
    if(editor_result_check(result)) return cli_error(result);
    return cli_project_compile(&workspace, &config);
}

static EditorResult cli_scene_id_parse(const char *value, uint32_t *id) {
    if(!value || !*value) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Expected a positive selector ID");
    for(const char *c = value; *c; ++c)
        if(*c < '0' || *c > '9') return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Invalid selector ID: %s", value);
    errno = 0; char *end; unsigned long parsed = strtoul(value, &end, 10);
    if(errno || *end || !parsed || parsed > UINT32_MAX)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Invalid selector ID: %s", value);
    *id = (uint32_t)parsed; return editor_result_value(true);
}
static EditorResult cli_scene_name_resolve(const EditorProject *project, const char *name,
        bool viewport, uint32_t *id) {
    char formatted[EDITOR_OBJECT_NAME_MAX];
    editor_project_object_name_format(formatted, sizeof(formatted), name);
    size_t matches = 0, count = viewport ? project->layout_viewport_count : project->object_count;
    for(size_t i = 0; i < count; ++i) {
        const char *stored = viewport ? project->layout_viewports[i].name : project->objects[i].name;
        if(strcmp(stored, name) && strcmp(stored, formatted)) continue;
        *id = viewport ? project->layout_viewports[i].id : project->objects[i].id; ++matches;
    }
    if(matches != 1) return editor_result_error(matches ? EDITOR_ERROR_INVALID_ARGUMENT : EDITOR_ERROR_NOT_FOUND,
        "%s '%s' is %s; use --%s-id for an explicit ID", viewport ? "Viewport" : "Object", name,
        matches ? "ambiguous" : "unknown", viewport ? "viewport" : "object");
    return editor_result_value(true);
}
static int cli_scene_action_command(int count, char **arguments) {
    const char *directory = ".", *object_name = NULL, *viewport_name = NULL;
    bool project_set = false, object_set = false, viewport_set = false;
    EditorObjectId object_id = 0;
    EditorSceneSelection selection = {0};
    EditorResult result = editor_result_value(true);
    for(int i = 1; i < count - 1; ++i) {
        const char *flag = arguments[i];
        if(i + 1 >= count - 1) return cli_error(editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "%s requires a value", flag));
        const char *value = arguments[++i];
        if(!strcmp(flag, "--project") && !project_set) { directory = value; project_set = true; }
        else if((!strcmp(flag, "--object") || !strcmp(flag, "--object-id")) && !object_set) {
            object_set = true;
            if(!strcmp(flag, "--object")) object_name = value;
            else result = cli_scene_id_parse(value, &object_id);
        } else if((!strcmp(flag, "--viewport") || !strcmp(flag, "--viewport-id")) && !viewport_set) {
            viewport_set = true;
            if(!strcmp(flag, "--viewport")) viewport_name = value;
            else result = cli_scene_id_parse(value, &selection.viewport);
        } else return cli_error(editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Unknown or repeated scene selector: %s", flag));
        if(editor_result_check(result)) return cli_error(result);
    }
    EditorWorkspace workspace = {0};
    static EditorProject project;
    editor_project_init(&project);
    int code = 1;
    result = editor_workspace_load(&workspace, &project, directory);
    if(editor_result_check(result)) goto done;
    if(object_name) { result = cli_scene_name_resolve(&project, object_name, false, &object_id); if(editor_result_check(result)) goto done; }
    if(viewport_name) { result = cli_scene_name_resolve(&project, viewport_name, true, &selection.viewport); if(editor_result_check(result)) goto done; }
    if(object_set) { selection.objects_selected = true; selection.objects = &object_id; selection.object_count = 1; }
    result = editor_workspace_scene_generate(&workspace, &project, &selection);
    if(editor_result_check(result)) goto done;
    EditorConfig config;
    result = cli_project_config_load(&config, &workspace);
    if(editor_result_check(result)) goto done;
    const EditorConfigCommand *compile = editor_config_command_get(&config, EDITOR_CONFIG_FRONTEND_CLI, EDITOR_CONFIG_OPERATION_COMPILE);
    if(compile && !compile->count) {
        result = editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Scene requests require compilation; the CLI compile command is disabled"); goto done;
    }
    code = cli_project_cmake_run(&workspace, &config, true, true);
    if(!code) code = cli_project_cmake_run(&workspace, &config, false, true);
    if(!code && !strcmp(arguments[count - 1], "simulate-scene")) {
        char executable[EDITOR_WORKSPACE_PATH_MAX * 2];
        result = editor_build_executable_get(&workspace, EDITOR_BUILD_SCENE, executable, sizeof(executable));
        if(editor_result_check(result)) goto done;
        const char *run[] = {executable, NULL};
        code = cli_process_run(workspace.directory, run);
    }
done:
    editor_project_destroy(&project);
    return editor_result_check(result) ? cli_error(result) : code;
}

static bool cli_workspace_arguments_check(int count, char **arguments) {
    if(count < 2) return false;
    for(int i = 1; i + 1 < count; i += 2) {
        if(strcmp(arguments[i], "--project") != 0 || i + 1 >= count - 1)
            return false;
    }
    return true;
}

static bool cli_workspace_action_check(int count, char **arguments) {
    const char *operation;
    if(!cli_workspace_arguments_check(count, arguments)) return false;
    operation = arguments[count - 1];
    return strcmp(operation, "create") == 0 || strcmp(operation, "validate") == 0 ||
        strcmp(operation, "generate-c") == 0 || strcmp(operation, "compile") == 0 ||
        strcmp(operation, "build") == 0;
}

static int cli_object_command(int count, char **arguments) {
    EditorCommand command;
    EditorCommandResult command_result;
    EditorDocument document;
    EditorResult result;
    const char *action;
    const char *path;
    if(count < 4) {
        cli_usage_print();
        return 1;
    }
    action = count > 2 ? arguments[2] : "";
    path = arguments[1][0] == '-' ? "./objects/project.rohr.json" : arguments[3];
    if(arguments[1][0] == '-') for(int i = 1; i + 1 < count; i += 1)
        if(strcmp(arguments[i], "--project") == 0) path = arguments[i + 1];
    result = editor_document_create(&document);
    if(editor_result_check(result)) return cli_error(result);
    result = editor_document_load(&document, path);
    if(editor_result_check(result)) return cli_error(result);
    if(strcmp(action, "list") == 0) {
        const EditorProject *project = editor_document_project_const_get(&document);
        for(size_t i = 0; i < project->object_count; i += 1)
            printf("%u\t%s\n", project->objects[i].id, project->objects[i].name);
        return 0;
    }
    result = arguments[1][0] == '-' ? editor_command_cli_standard_parse(
        document.project, count, arguments, &path, &command) :
        editor_command_cli_named_parse(document.project, count, arguments,
            &path, &command);
    if(editor_result_check(result)) return cli_error(result);
    if(command.type == EDITOR_COMMAND_SOURCE &&
            (command.data.source.operation == EDITOR_SOURCE_GET || command.data.source.operation == EDITOR_SOURCE_LIST)) {
        char *json = editor_source_query_json(document.project, &command.data.source);
        if(!json) return cli_error(editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Source query failed"));
        puts(json); free(json);
        editor_document_destroy(&document);
        return 0;
    }
    command_result = editor_command_execute(document.project, &command);
    if(command_result.kind == ERROR_RESULT_ERROR)
        return cli_error((EditorResult){.kind = ERROR_RESULT_ERROR,
            .result.error = command_result.result.error});
    if(command.type == EDITOR_COMMAND_OBJECT_ADD)
        printf("added object %u\n", command_result.result.object);
    document.dirty = true;
    result = editor_document_save(&document);
    return editor_result_check(result) ? cli_error(result) : 0;
}

int main(int count, char **arguments) {
    if(cli_help_request_check(count, arguments)) {
        cli_help_print(count, arguments);
        return 0;
    }
    if(count >= 2 && (!strcmp(arguments[count - 1], "build-scene") ||
            !strcmp(arguments[count - 1], "simulate-scene")))
        return cli_scene_action_command(count, arguments);
    if(cli_workspace_action_check(count, arguments))
        return cli_workspace_action_command(count, arguments);
    if(cli_workspace_arguments_check(count, arguments))
        return cli_error(editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "Unknown project operation: %s", arguments[count - 1]));
    if(count >= 2 && arguments[1][0] == '-')
        return cli_object_command(count, arguments);
    cli_usage_print();
    return 1;
}
