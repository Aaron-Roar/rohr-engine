/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_workspace.h"
#include "entity/object_internal.h"
#include "../../../tests/test_png.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    assert(argc == 2);
    EditorWorkspace workspace = {0};
    EditorProject project;
    assert(!editor_result_check(editor_workspace_create(&workspace, &project, argv[1])));
    editor_project_destroy(&project);
    editor_project_init(&project);
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    assert(object); snprintf(object->name, sizeof(object->name), "actor");
    EditorRigidBody *body = editor_project_rigid_body_add(&project, object);
    assert(body); snprintf(body->name, sizeof(body->name), "body");
    body->gravity_enabled = false;
    body->rotation = 90;
    assert(editor_project_hitbox_add(&project, body));
    char path[EDITOR_WORKSPACE_PATH_MAX];
    snprintf(path, sizeof(path), "%s/assets/attachment.png", workspace.directory);
    assert(SDL_SaveFile(path, test_png, sizeof(test_png)));
    for(size_t i = 0; i < 40; ++i) {
        EditorAnimatedSprite *animation = editor_project_animated_sprite_add(&project, object);
        assert(animation);
        snprintf(animation->name, sizeof(animation->name), "animation_%zu", i);
        animation->rigid_body = body->id;
        animation->editor_position = (Position){(float)i, 2};
        animation->playing = true; animation->ticks_per_frame = i + 1;
        animation->app_hidden = i == 1;
        animation->graphics_layer.value = (int)i;
        assert(editor_project_animation_frame_add(&project, animation, "first", "assets/attachment.png", (Scale){20,20}));
        assert(editor_project_animation_frame_add(&project, animation, "second", "assets/attachment.png", (Scale){30,30}));
        assert(editor_project_hitbox_animation_binding_set(body, animation->id,
            animation->frames[0].id, body->hitboxes[i % 2].id, true));
        if(i == 39) body->hitbox_animation_driver = animation->id;
    }
    assert(editor_workspace_save(&workspace, &project));
    assert(editor_workspace_c_generate(&workspace, &project));
    snprintf(path, sizeof(path), "%s/src/generated/project_objects.c", workspace.directory);
    size_t before_size = 0, after_size = 0;
    char *before = SDL_LoadFile(path, &before_size); assert(before);
    assert(editor_workspace_c_generate(&workspace, &project));
    char *after = SDL_LoadFile(path, &after_size); assert(after);
    assert(before_size == after_size && !memcmp(before, after, before_size));
    SDL_free(before); SDL_free(after);
    snprintf(path, sizeof(path), "%s/src/main.c", workspace.directory);
    const char edited[] = "/* developer-owned entry point */\nint main(void) { return 0; }\n";
    assert(SDL_SaveFile(path, edited, sizeof(edited) - 1));
    assert(editor_workspace_c_generate(&workspace, &project));
    after = SDL_LoadFile(path, &after_size);
    assert(after && after_size == sizeof(edited) - 1 && !memcmp(after, edited, after_size));
    SDL_free(after);
    editor_project_destroy(&project);
    return 0;
}
