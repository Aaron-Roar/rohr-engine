/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_history.h"
#include <editor_soft_area.h>
#include "editor_mass_properties.h"
#include "viewport/controls/editor_rotation_control.h"
#include "editor_layout.h"
#include "editor_navigation.h"
#include "editor_shortcuts.h"
#include "editor_viewport.h"

#include <assert.h>
#include <math.h>
#include "../../../tests/test_png.h"
#include <stdio.h>
#include <string.h>

float editor_viewport_width = 1024.0f;
float editor_window_width = 1280.0f;
float editor_window_height = 720.0f;
float editor_viewport_bottom = 720.0f;

static EditorHistory *callback_history;

static void history_begin(const EditorProject *project,
        const EditorCommand *command, void *context) {
    (void)context;
    editor_history_command_begin(callback_history, project, command);
}

static void history_finish(const EditorCommand *command,
        const EditorCommandResult *result, void *context) {
    (void)context;
    editor_history_command_finish(callback_history, command, result);
}

static Position test_world_to_screen(Position world) {
    return (Position){editor_viewport_width * 0.5f + world.x,
        EDITOR_MENU_HEIGHT +
            (editor_viewport_bottom - EDITOR_MENU_HEIGHT) * 0.5f - world.y};
}

static Position test_layout_to_screen(const EditorProject *project,
        const EditorLayoutViewport *viewport, Position local) {
    Position center = {editor_viewport_width * 0.5f,
        EDITOR_MENU_HEIGHT +
            (editor_viewport_bottom - EDITOR_MENU_HEIGHT) * 0.5f};
    return (Position){center.x + project->viewport_camera_offset.x +
            (viewport->config.rectangle.x + local.x) *
                project->viewport_camera_zoom,
        center.y + project->viewport_camera_offset.y +
            (viewport->config.rectangle.y + local.y) *
                project->viewport_camera_zoom};
}

static bool viewport_pointer_update(EditorHistory *history,
        EditorViewportState *viewport, EditorProject *project, Position pointer,
        MouseButtonState button) {
    bool active = editor_viewport_transform_active_check(viewport);
    bool consumed = editor_viewport_update(viewport, project, pointer, button,
        MOUSE_BUTTON_STATE_UP, false, 0.0f, false);
    assert(editor_navigation_viewport_transform_history_update(
        project, viewport, history, active));
    return consumed;
}

static bool auto_shape_pointer_update(EditorHistory *history,
        EditorViewportState *viewport, EditorProject *project,
        EditorAutoShapeConfig *config, Position pointer,
        MouseButtonState button) {
    bool active = editor_viewport_transform_active_check(viewport);
    bool consumed = editor_viewport_auto_shape_update(viewport, project, config,
        pointer, button, MOUSE_BUTTON_STATE_UP, false, 0.0f, false);
    assert(editor_navigation_viewport_transform_history_update(
        project, viewport, history, active));
    return consumed;
}

static void shortcut_apply(EditorHistory *history, SDL_Keycode key) {
    SDL_Event shortcut = {0};
    EditorHistoryShortcutResult result;
    shortcut.type = SDL_EVENT_KEY_DOWN;
    shortcut.key.key = key;
    shortcut.key.mod = SDL_KMOD_CTRL;
    result = editor_history_shortcut_handle(&shortcut, true, history);
    assert(result.consumed && result.restored);
}

static void animation_frame_history_check(void) {
    const float zooms[] = {0.1f, 1.0f, 10.0f};
    for(size_t zoom = 0; zoom < 3; zoom += 1) {
        EditorProject project;
        EditorHistory history;
        EditorViewportState viewport = {0};
        editor_project_init(&project);
        project.viewport_local_view = false;
        project.viewport_camera_zoom = zooms[zoom];
        project.viewport_camera_offset = (Vec2D){15 * zooms[zoom], -20 * zooms[zoom]};
        EditorObject *object = editor_project_object_add(&project, (Position){0});
        EditorRigidBody *body = editor_project_rigid_body_add(&project, object);
        assert(body != NULL);
        body->position = (Position){-40,20};
        body->rotation = 90;
        body->visible = false;
        EditorAnimatedSprite *animation = editor_project_animated_sprite_add(&project, object);
        assert(animation != NULL);
        animation->rigid_body = body->id;
        animation->editor_position = (Position){10,5};
        animation->follow_body_rotation = true;
        animation->scale = (Scale){2,3};
        assert(editor_project_animation_frame_add(&project, animation, "first", "editor_history_frame.png",
            (Scale){20,20}));
        assert(editor_project_animation_frame_add(&project, animation, "second", "editor_history_frame.png",
            (Scale){20,20}));
        animation->frames[1].offset = (Position){30,20};
        animation->frames[1].rotation = 450;
        EditorAnimatedSpriteId id = animation->id;
        assert(editor_history_init(&history, &project));
        callback_history = &history;
        editor_command_executing_callback_set(history_begin, NULL);
        editor_command_finished_callback_set(history_finish, NULL);
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_ANIMATION_FRAME;
        viewport.selection = EDITOR_SELECTION_ANIMATION_FRAME;
        viewport.selected_animated_sprite = id;
        viewport.selected_animation_frame = animation->frames[1].id;
        Position screen_origin = test_world_to_screen((Position){0});
        screen_origin.x += project.viewport_camera_offset.x;
        screen_origin.y += project.viewport_camera_offset.y;
        Position center = {-15,-20};
        Position press = {screen_origin.x + center.x * zooms[zoom] + 1,
            screen_origin.y - center.y * zooms[zoom] + 1};
        Position moved = {press.x + 30 * zooms[zoom], press.y + 20 * zooms[zoom]};
        EditorSelectionRef context_selection;
        assert(editor_viewport_selection_at_get(&project, &viewport, press, &context_selection));
        assert(context_selection.kind == EDITOR_SELECTION_ANIMATION_FRAME &&
            context_selection.item == animation->frames[1].id);
        assert(viewport_pointer_update(&history, &viewport, &project, press,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.dragged_animation_frame && !viewport.dragged_animated_sprite);
        assert(viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_DOWN));
        (void)viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_RELEASED);
        assert(history.undo_count == 1);
        assert(fabsf(animation->frames[1].offset.x - 50) < 0.002f);
        assert(fabsf(animation->frames[1].offset.y - 50) < 0.002f);
        assert(animation->frames[0].offset.x == 0 && animation->editor_position.x == 10);
        assert(editor_history_undo(&history));
        animation = editor_project_animated_sprite_get(&project.objects[0], id);
        assert(animation->frames[1].offset.x == 30);
        assert(editor_history_redo(&history));
        animation = editor_project_animated_sprite_get(&project.objects[0], id);
        assert(fabsf(animation->frames[1].offset.x - 50) < 0.002f);
        center = (Position){15,-40};
        for(int step = 0; step <= 5; step += 1) {
            Position handle = editor_rotation_control_position_get(center,
                540 + 90 * step, EDITOR_VIEWPORT_ROTATION_ARM_LENGTH / zooms[zoom]);
            Position pointer = {screen_origin.x + handle.x * zooms[zoom],
                screen_origin.y - handle.y * zooms[zoom]};
            assert(viewport_pointer_update(&history, &viewport, &project, pointer,
                step == 0 ? MOUSE_BUTTON_STATE_PRESSED : MOUSE_BUTTON_STATE_DOWN));
            assert(viewport.rotated_animation_frame);
            assert(fabsf(animation->frames[1].rotation - (450 + 90 * step)) < 0.002f);
            assert(fabsf(animation->frames[1].offset.x - 50) < 0.002f);
        }
        editor_history_transaction_cancel(&history);
        editor_viewport_transform_cancel(&viewport);
        animation = editor_project_animated_sprite_get(&project.objects[0], id);
        assert(animation->frames[1].rotation == 450 && history.undo_count == 1);
        if(zoom == 1) {
            /* The origin ring must not swallow double-clicks on a centered frame. */
            animation->playing = false;
            animation->starting_frame = 0;
            editor_viewport_state_destroy(&viewport);
            editor_viewport_state_init(&viewport);
            viewport.mode = EDITOR_VIEWPORT_OBJECT;
            Position origin = {screen_origin.x - 35, screen_origin.y - 10};
            for(int click = 0; click < 2; click += 1) {
                assert(viewport_pointer_update(&history, &viewport, &project, origin,
                    MOUSE_BUTTON_STATE_PRESSED));
                (void)viewport_pointer_update(&history, &viewport, &project, origin,
                    MOUSE_BUTTON_STATE_RELEASED);
            }
            assert(viewport.mode == EDITOR_VIEWPORT_ANIMATION_FRAME);
            assert(viewport.selected_animation_frame == animation->frames[0].id);
            assert(animation->frames[1].offset.x > 49.99f);
        }
        size_t parent_undo_count = history.undo_count;
        EditorCommand parent_scale = {.type = EDITOR_COMMAND_ANIMATED_SPRITE_SCALE_SET,
            .data.animated_sprite_scale_set = {project.objects[0].id, id, {.5f,1.5f}}};
        assert(editor_command_execute(&project, &parent_scale).kind == ERROR_RESULT_VALUE);
        assert(history.undo_count == parent_undo_count + 1);
        assert(animation->frames[1].scale.x == 20 &&
            fabsf(animation->frames[1].offset.x - 50) < 0.002f &&
            fabsf(animation->frames[1].offset.y - 50) < 0.002f);
        assert(editor_history_undo(&history));
        animation = editor_project_animated_sprite_get(&project.objects[0], id);
        assert(animation->scale.x == 2 && animation->scale.y == 3);
        assert(editor_history_redo(&history));
        animation = editor_project_animated_sprite_get(&project.objects[0], id);
        assert(animation->scale.x == .5f && animation->scale.y == 1.5f &&
            fabsf(animation->frames[1].offset.x - 50) < 0.002f &&
            fabsf(animation->frames[1].offset.y - 50) < 0.002f);
        size_t undo_count = history.undo_count;
        EditorCommand scale_command = {.type = EDITOR_COMMAND_ANIMATION_FRAME_SCALE_SET,
            .data.animation_frame_scale_set = {project.objects[0].id, id, 1, {-2,0}}};
        assert(editor_command_execute(&project, &scale_command).kind == ERROR_RESULT_VALUE);
        animation = editor_project_animated_sprite_get(&project.objects[0], id);
        Position preserved_offset = animation->frames[1].offset;
        assert(animation->frames[1].scale.x == -2 && animation->frames[1].scale.y == 0);
        assert(history.undo_count == undo_count + 1);
        scale_command.data.animation_frame_scale_set.scale.x = NAN;
        assert(editor_command_execute(&project, &scale_command).kind == ERROR_RESULT_ERROR);
        assert(history.undo_count == undo_count + 1 && animation->frames[1].scale.x == -2);
        assert(editor_history_undo(&history));
        animation = editor_project_animated_sprite_get(&project.objects[0], id);
        assert(animation->frames[1].scale.x == 20 && animation->frames[1].scale.y == 20);
        assert(editor_history_redo(&history));
        animation = editor_project_animated_sprite_get(&project.objects[0], id);
        assert(animation->frames[1].scale.x == -2 && animation->frames[1].scale.y == 0 &&
            animation->frames[1].offset.x == preserved_offset.x &&
            animation->frames[1].offset.y == preserved_offset.y);
        editor_command_executing_callback_set(NULL, NULL);
        editor_command_finished_callback_set(NULL, NULL);
        callback_history = NULL;
        editor_viewport_state_destroy(&viewport);
        editor_history_destroy(&history);
        editor_project_destroy(&project);
    }
}

static void angular_history_check(void) {
    Position up = editor_rotation_control_position_get((Position){0}, 0, 80);
    Position right = editor_rotation_control_position_get((Position){0}, 90, 80);
    assert(fabsf(up.x) < 0.001f && fabsf(up.y - 80) < 0.001f);
    assert(fabsf(right.x - 80) < 0.001f && fabsf(right.y) < 0.001f);
    float grab_offset;
    Position grabbed = {right.x + 2, right.y + 3};
    assert(editor_rotation_control_begin((Position){0}, 810, 80, grabbed, 10, &grab_offset));
    assert(fabsf(editor_rotation_control_orientation_get((Position){0}, grabbed,
        grab_offset, 810) - 810) < 0.001f);
    assert(editor_rotation_control_orientation_get((Position){0}, (Position){0},
        grab_offset, 810) == 810);
    EditorProject project;
    EditorHistory history;
    EditorViewportState viewport = {0};
    editor_project_init(&project);
    project.viewport_local_view = false;
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    EditorRigidBody *body = editor_project_rigid_body_add(&project, object);
    assert(body != NULL);
    body->position = (Position){0};
    body->rotation = -450;
    EditorRigidBodyId id = body->id;
    assert(editor_history_init(&history, &project));
    callback_history = &history;
    editor_command_executing_callback_set(history_begin, NULL);
    editor_command_finished_callback_set(history_finish, NULL);
    editor_viewport_state_init(&viewport);
    viewport.mode = EDITOR_VIEWPORT_RIGID_BODY;
    viewport.selection = EDITOR_SELECTION_RIGID_BODY;
    viewport.selected_rigid_body = id;
    Position pointer = editor_rotation_control_position_get((Position){0}, -450,
        EDITOR_VIEWPORT_ROTATION_ARM_LENGTH);
    assert(viewport_pointer_update(&history, &viewport, &project,
        test_world_to_screen(pointer), MOUSE_BUTTON_STATE_PRESSED));
    assert(viewport.rotated_body);
    for(int step = 0; step <= 14; step += 1) {
        float angle = -450 + step * 90;
        pointer = editor_rotation_control_position_get((Position){0}, angle,
            EDITOR_VIEWPORT_ROTATION_ARM_LENGTH);
        assert(viewport_pointer_update(&history, &viewport, &project,
            test_world_to_screen(pointer), MOUSE_BUTTON_STATE_DOWN));
        assert(fabsf(body->rotation - angle) < 0.001f);
    }
    (void)viewport_pointer_update(&history, &viewport, &project,
        test_world_to_screen(pointer), MOUSE_BUTTON_STATE_RELEASED);
    assert(history.undo_count == 1);
    assert(editor_history_undo(&history));
    body = editor_project_rigid_body_get(&project.objects[0], id);
    assert(fabsf(body->rotation + 450) < 0.001f);
    assert(editor_history_redo(&history));
    body = editor_project_rigid_body_get(&project.objects[0], id);
    assert(fabsf(body->rotation - 810) < 0.001f);
    assert(viewport_pointer_update(&history, &viewport, &project,
        test_world_to_screen(pointer), MOUSE_BUTTON_STATE_PRESSED));
    pointer = editor_rotation_control_position_get((Position){0}, 900,
        EDITOR_VIEWPORT_ROTATION_ARM_LENGTH);
    assert(viewport_pointer_update(&history, &viewport, &project,
        test_world_to_screen(pointer), MOUSE_BUTTON_STATE_DOWN));
    editor_history_transaction_cancel(&history);
    editor_viewport_transform_cancel(&viewport);
    body = editor_project_rigid_body_get(&project.objects[0], id);
    assert(fabsf(body->rotation - 810) < 0.001f && history.undo_count == 1);
    editor_command_executing_callback_set(NULL, NULL);
    editor_command_finished_callback_set(NULL, NULL);
    callback_history = NULL;
    editor_history_destroy(&history);
    editor_project_destroy(&project);
}

static void center_of_mass_interaction_check(void) {
    EditorProject project;
    EditorHistory history;
    EditorViewportState viewport = {0};
    editor_project_init(&project);
    EditorObject *object = editor_project_object_add(&project, (Position){10, 20});
    assert(object != NULL);
    EditorRigidBody *body = editor_project_rigid_body_add(&project, object);
    assert(body != NULL);
    project.viewport_local_view = false;
    body->position = (Position){40, -30};
    body->rotation = -90.0f;
    EditorRigidBodyId id = body->id;
    EditorHitboxId hitbox_id = body->hitboxes[0].id;
    Position original_vertex = body->hitboxes[0].vertices[0].position;
    assert(editor_center_of_mass_set(&project, object->id, id, true,
        (Position){0}).kind == ERROR_RESULT_VALUE);
    assert(editor_history_init(&history, &project));
    callback_history = &history;
    editor_command_executing_callback_set(history_begin, NULL);
    editor_command_finished_callback_set(history_finish, NULL);
    editor_viewport_state_init(&viewport);
    viewport.mode = EDITOR_VIEWPORT_VERTEX;
    viewport.selection = EDITOR_SELECTION_VERTEX;
    viewport.selected_rigid_body = id;
    viewport.selected_hitbox = hitbox_id;
    Position origin = {object->position.x + body->position.x,
        object->position.y + body->position.y};
    Position press = test_world_to_screen((Position){origin.x + 3, origin.y + 2});
    /* COM wins over the coincident origin, even while editing a child. */
    assert(viewport_pointer_update(&history, &viewport, &project, press,
        MOUSE_BUTTON_STATE_PRESSED));
    assert(viewport.dragged_center_of_mass && !viewport.dragged_origin &&
        !viewport.dragged_body && viewport.selection == EDITOR_SELECTION_CENTER_OF_MASS);
    Position drag = {press.x + 20, press.y - 30};
    assert(viewport_pointer_update(&history, &viewport, &project, drag,
        MOUSE_BUTTON_STATE_DOWN));
    drag.x += 10;
    assert(viewport_pointer_update(&history, &viewport, &project, drag,
        MOUSE_BUTTON_STATE_DOWN));
    assert(!viewport_pointer_update(&history, &viewport, &project, drag,
        MOUSE_BUTTON_STATE_RELEASED));
    assert(!editor_viewport_transform_active_check(&viewport));
    assert(history.undo_count == 1);
    assert(fabsf(body->center_of_mass_offset.x - 30) < 0.001f &&
        fabsf(body->center_of_mass_offset.y + 30) < 0.001f);
    assert(body->position.x == 40 && body->position.y == -30 &&
        body->hitboxes[0].vertices[0].position.x == original_vertex.x &&
        body->hitboxes[0].vertices[0].position.y == original_vertex.y);
    assert(editor_history_undo(&history));
    body = editor_project_rigid_body_get(&project.objects[0], id);
    assert(body->center_of_mass_explicit && body->center_of_mass_offset.x == 0);
    assert(viewport.mode == EDITOR_VIEWPORT_CENTER_OF_MASS);
    assert(editor_history_redo(&history));
    object = &project.objects[0];
    body = editor_project_rigid_body_get(object, id);
    assert(fabsf(body->center_of_mass_offset.y + 30) < 0.001f);
    /* Cancel restores a continuous edit and leaves no extra history entry. */
    Position center = test_world_to_screen((Position){origin.x + 30, origin.y + 30});
    assert(viewport_pointer_update(&history, &viewport, &project, center,
        MOUSE_BUTTON_STATE_PRESSED));
    assert(viewport_pointer_update(&history, &viewport, &project,
        (Position){center.x + 15, center.y}, MOUSE_BUTTON_STATE_DOWN));
    editor_history_transaction_cancel(&history);
    editor_viewport_transform_cancel(&viewport);
    assert(history.undo_count == 1);
    object = &project.objects[0];
    body = editor_project_rigid_body_get(object, id);
    assert(fabsf(body->center_of_mass_offset.y + 30) < 0.001f);
    /* Automatic mode discards the override and is undoable. */
    assert(editor_center_of_mass_mode_set(&project, object->id, id, false).kind ==
        ERROR_RESULT_VALUE);
    assert(!body->center_of_mass_explicit && body->center_of_mass_offset.x == 0);
    assert(editor_history_undo(&history));
    body = editor_project_rigid_body_get(&project.objects[0], id);
    assert(body->center_of_mass_explicit && fabsf(body->center_of_mass_offset.x - 30) < 0.001f);
    editor_command_executing_callback_set(NULL, NULL);
    editor_command_finished_callback_set(NULL, NULL);
    callback_history = NULL;
    /* COM overlay wins over covering geometry, regardless of scene order. */
    object = &project.objects[0];
    EditorRigidBody *front = editor_project_rigid_body_add(&project, object);
    assert(front != NULL);
    front->position = (Position){70, 0};
    viewport.mode = EDITOR_VIEWPORT_RIGID_BODY;
    viewport.selection = EDITOR_SELECTION_RIGID_BODY;
    viewport.selected_rigid_body = id;
    assert(editor_viewport_update(&viewport, &project, center,
        MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP, false, 0, false));
    assert(viewport.dragged_center_of_mass && viewport.selected_rigid_body == id);
    editor_viewport_transform_cancel(&viewport);
    front->visible = false;
    viewport.mode = EDITOR_VIEWPORT_RIGID_BODY;
    viewport.selected_rigid_body = id;
    assert(editor_viewport_update(&viewport, &project, center,
        MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP, false, 0, false));
    assert(viewport.dragged_center_of_mass);
    editor_viewport_transform_cancel(&viewport);
    /* Modal capture must prevent a new handle drag. */
    assert(!editor_viewport_update(&viewport, &project, center,
        MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP, false, 0, true));
    assert(!viewport.dragged_center_of_mass);
    EditorSprite *cover = editor_project_sprite_add(&project, object, "cover", "unused.png");
    assert(cover != NULL);
    cover->position = (Position){70, 0};
    cover->size = (Scale){40, 40};
    cover->visible = true;
    /* A sprite with a higher authored layer cannot cover COM interaction. */
    cover->graphics_layer.value = 1000000;
    (void)editor_viewport_update(&viewport, &project, center,
        MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP, false, 0, false);
    assert(viewport.dragged_center_of_mass && viewport.selection == EDITOR_SELECTION_CENTER_OF_MASS);
    editor_viewport_transform_cancel(&viewport);
    cover->visible = false;
    viewport.mode = EDITOR_VIEWPORT_RIGID_BODY;
    viewport.selection = EDITOR_SELECTION_RIGID_BODY;
    viewport.selected_rigid_body = id;
    body = editor_project_rigid_body_get(object, id);
    body->center_of_mass_explicit = false;
    body->center_of_mass_offset = (Position){0};
    Position automatic = editor_mass_properties_get(body).center;
    Vec2D offset = math_vector_rotate(automatic, body->rotation);
    center = test_world_to_screen((Position){origin.x + offset.x, origin.y + offset.y});
    (void)editor_viewport_update(&viewport, &project, center,
        MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP, false, 0, false);
    assert(!viewport.dragged_center_of_mass && !body->center_of_mass_explicit);
    assert(viewport.selection == EDITOR_SELECTION_CENTER_OF_MASS && !editor_viewport_transform_active_check(&viewport));
    Position previous_position = body->position;
    size_t previous_undo = history.undo_count;
    (void)editor_viewport_update(&viewport, &project, (Position){center.x + 30, center.y},
        MOUSE_BUTTON_STATE_DOWN, MOUSE_BUTTON_STATE_UP, false, 0, false);
    assert(!body->center_of_mass_explicit && body->position.x == previous_position.x &&
        body->position.y == previous_position.y && history.undo_count == previous_undo);

    editor_viewport_transform_cancel(&viewport);
    body->center_of_mass_explicit = true;
    body->center_of_mass_offset = (Position){150, 120};
    center = test_world_to_screen((Position){origin.x - 120, origin.y + 150});
    viewport.mode = EDITOR_VIEWPORT_RIGID_BODY;
    viewport.selected_rigid_body = id;
    /* Exterior COM remains reachable without geometry under the handle. */
    assert(editor_viewport_update(&viewport, &project, center,
        MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP, false, 0, false));
    assert(viewport.dragged_center_of_mass);
    (void)editor_viewport_update(&viewport, &project, (Position){-20, 20},
        MOUSE_BUTTON_STATE_RELEASED, MOUSE_BUTTON_STATE_UP, false, 0, true);
    assert(!viewport.dragged_center_of_mass);
    body->visible = false;
    (void)editor_viewport_update(&viewport, &project, center,
        MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP, false, 0, false);
    assert(!viewport.dragged_center_of_mass);
    body->visible = true;
    body->particle = body->standalone_particle = true;
    body->center_of_mass_explicit = false;
    body->center_of_mass_offset = (Position){0};
    body->particle_origin = (Position){0};
    (void)editor_viewport_update(&viewport, &project, test_world_to_screen(origin),
        MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP, false, 0, false);
    assert(!viewport.dragged_center_of_mass && viewport.mode == EDITOR_VIEWPORT_PARTICLE);
    editor_history_destroy(&history);
    editor_viewport_state_destroy(&viewport);
    editor_project_destroy(&project);
}

static void square_check(const Position *points, float side) {
    for(size_t i = 0; i < 4; i += 1) {
        float x = points[(i + 1) % 4].x - points[i].x;
        float y = points[(i + 1) % 4].y - points[i].y;
        assert((fabsf(x) < .001f && fabsf(fabsf(y) - side) < .001f) ||
            (fabsf(y) < .001f && fabsf(fabsf(x) - side) < .001f));
        for(size_t j = i + 1; j < 4; j += 1)
            assert(points[i].x != points[j].x || points[i].y != points[j].y);
    }
}

static void default_shapes_history_check(void) {
    EditorProject project, loaded;
    EditorHistory history;
    editor_project_init(&project);
    editor_project_init(&loaded);
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    assert(object != NULL && editor_history_init(&history, &project));
    callback_history = &history;
    editor_command_executing_callback_set(history_begin, NULL);
    editor_command_finished_callback_set(history_finish, NULL);
    EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
        .data.item_add = {.kind = EDITOR_ITEM_RIGID_BODY, .object = object->id}};
    assert(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    assert(history.undo_count == 1 && object->rigid_body_count == 1);
    EditorHitbox *hitbox = &object->rigid_bodies[0].hitboxes[0];
    assert(hitbox->vertex_count == 4);
    Position points[4];
    for(size_t i = 0; i < 4; i += 1) points[i] = hitbox->vertices[i].position;
    square_check(points, EDITOR_BODY_DEFAULT_SIZE);

    command.data.item_add.kind = EDITOR_ITEM_SOFT_BODY;
    assert(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    assert(history.undo_count == 2 && object->soft_body_count == 1);
    EditorSoftBody *soft = &object->soft_body_items[0];
    assert(soft->node_count == 4 && soft->beam_count == 6 &&
        soft->hierarchy_count == 10 && soft->area_count == 1 &&
        soft->areas[0].outer.node_count == 4);
    EditorSoftNodeId ids[4];
    for(size_t i = 0; i < 4; i += 1) {
        points[i] = soft->nodes[i].position;
        ids[i] = soft->nodes[i].id;
        assert(ids[i] != 0);
    }
    square_check(points, EDITOR_BODY_DEFAULT_SIZE);
    for(size_t i = 0; i < 4; i += 1)
        for(size_t j = i + 1; j < 4; j += 1) {
            size_t matches = 0;
            for(size_t b = 0; b < soft->beam_count; b += 1) {
                EditorSoftBeam *beam = &soft->beams[b];
                if((beam->node_a == ids[i] && beam->node_b == ids[j]) ||
                        (beam->node_a == ids[j] && beam->node_b == ids[i])) matches += 1;
            }
            assert(matches == 1);
        }
    assert(editor_history_undo(&history));
    assert(project.objects[0].soft_body_count == 0);
    assert(editor_history_redo(&history));
    object = &project.objects[0];
    soft = &object->soft_body_items[0];
    assert(soft->node_count == 4 && soft->beam_count == 6 &&
        soft->hierarchy_count == 10 && soft->area_count == 1 &&
        soft->areas[0].outer.node_count == 4);
    for(size_t i = 0; i < 4; i += 1) assert(soft->nodes[i].id == ids[i]);

    assert(editor_history_ui_change_begin(&history));
    EditorLayoutViewport *viewport = editor_project_layout_viewport_add(&project);
    EditorViewportUiItem *item = editor_viewport_ui_add(&project, viewport,
        EDITOR_VIEWPORT_UI_SHAPE);
    assert(item != NULL && item->value.shape.vertex_count == 4);
    for(size_t i = 0; i < 4; i += 1) points[i] = item->value.shape.vertices[i];
    square_check(points, EDITOR_UI_SHAPE_DEFAULT_SIZE);
    assert(editor_history_ui_change_finish(&history));
    assert(history.undo_count == 3);
    assert(editor_history_undo(&history) && project.layout_viewport_count == 0);
    assert(editor_history_redo(&history) && project.layout_viewport_count == 1);

    const char *path = "editor_default_shapes.json";
    assert(editor_project_save(&project, path));
    char *saved = SDL_LoadFile(path, NULL);
    assert(saved != NULL && strstr(saved, "\"areas\"") != NULL &&
        strstr(saved, "area_color") == NULL && strstr(saved, "next_soft_area_id") == NULL);
    SDL_free(saved);
    assert(!editor_result_check(editor_project_load(&loaded, path)));
    assert(loaded.objects[0].rigid_bodies[0].hitboxes[0].vertex_count == 4);
    assert(loaded.objects[0].soft_body_items[0].node_count == 4 &&
        loaded.objects[0].soft_body_items[0].beam_count == 6);
    /* Explicitly authored triangles, rectangles and removed beams survive reload. */
    object = &project.objects[0];
    hitbox = &object->rigid_bodies[0].hitboxes[0];
    assert(editor_project_hitbox_line_remove(hitbox, 0));
    soft = &object->soft_body_items[0];
    assert(editor_project_soft_beam_remove(&project, soft, soft->beams[0].id));
    EditorViewportUiDefinition *definition = &project.ui_definitions[0];
    definition->value.shape.vertices[2].y = 36;
    definition->value.shape.vertices[3].y = 36;
    assert(editor_project_save(&project, path));
    assert(!editor_result_check(editor_project_load(&loaded, path)));
    assert(loaded.objects[0].rigid_bodies[0].hitboxes[0].vertex_count == 3);
    assert(loaded.objects[0].soft_body_items[0].beam_count == 5);
    assert(loaded.ui_definitions[0].value.shape.vertices[2].y == 36);
    command = (EditorCommand){.type = EDITOR_COMMAND_ITEM_REMOVE,
        .data.item_remove = {.kind = EDITOR_ITEM_SOFT_NODE, .object = object->id,
            .parent = soft->id, .item = soft->nodes[0].id}};
    assert(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    assert(project.objects[0].soft_body_items[0].area_count == 0);
    assert(editor_history_undo(&history));
    assert(project.objects[0].soft_body_items[0].area_count == 1);
    assert(project.objects[0].soft_body_items[0].areas[0].outer.node_count == 4);
    assert(editor_history_redo(&history));
    assert(project.objects[0].soft_body_items[0].area_count == 0);
    assert(SDL_RemovePath(path));
    editor_command_executing_callback_set(NULL, NULL);
    editor_command_finished_callback_set(NULL, NULL);
    callback_history = NULL;
    editor_history_destroy(&history);
    editor_project_destroy(&project);
    editor_project_destroy(&loaded);
}


static void area_authoring_history_check(void) {
    EditorProject project;
    EditorHistory history;
    EditorViewportState state;
    editor_project_init(&project);
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
        .data.item_add = {.kind = EDITOR_ITEM_SOFT_BODY, .object = object->id}};
    assert(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    EditorSoftBody *body = &object->soft_body_items[0];
    EditorSoftBodyId body_id = body->id;
    EditorSoftNodeId first = body->nodes[0].id, second = body->nodes[1].id;
    assert(editor_history_init(&history, &project));
    callback_history = &history;
    editor_command_executing_callback_set(history_begin, NULL);
    editor_command_finished_callback_set(history_finish, NULL);
    command = (EditorCommand){.type = EDITOR_COMMAND_ITEM_ADD,
        .data.item_add = {.kind = EDITOR_ITEM_SOFT_AREA, .object = object->id, .parent = body_id}};
    EditorCommandResult created = editor_command_execute(&project, &command);
    assert(created.kind == ERROR_RESULT_VALUE);
    EditorSoftAreaId area_id = created.result.object;
    assert(body->area_count == 2);
    assert(editor_history_undo(&history) && body->area_count == 1);
    assert(editor_history_redo(&history) && body->area_count == 2);
    editor_viewport_state_init(&state);
    EditorSelectionRef ref = {EDITOR_SELECTION_SOFT_AREA, object->id, body_id, 0, area_id};
    assert(editor_viewport_selection_set(&project, &state, ref, false));
    assert(editor_navigation_selected_open(&project, &state));
    assert(state.mode == EDITOR_VIEWPORT_SOFT_AREA && !state.soft_area_picking);
    state.soft_area_picking = true;
    assert(editor_viewport_soft_area_node_toggle(&project, &state, first));
    assert(editor_viewport_soft_area_node_toggle(&project, &state, second));
    assert(editor_viewport_soft_area_node_toggle(&project, &state, first));
    assert(editor_viewport_soft_area_node_toggle(&project, &state, first));
    EditorSoftArea *area = editor_soft_area_get(body, area_id);
    assert(area->outer.node_count == 2 && area->outer.nodes[0] == second && area->outer.nodes[1] == first);
    assert(editor_history_undo(&history));
    area = editor_soft_area_get(body, area_id);
    assert(area->outer.node_count == 1 && area->outer.nodes[0] == second);
    assert(editor_history_redo(&history));
    assert(editor_navigation_selection_name_set(&project, ref, "paint"));
    assert(editor_history_undo(&history));
    assert(strcmp(editor_soft_area_get(body, area_id)->name, "area_2") == 0);
    assert(editor_history_redo(&history));
    assert(editor_navigation_selection_visibility_set(&project, ref, false));
    assert(editor_history_undo(&history) && editor_soft_area_get(body, area_id)->visible);
    command = (EditorCommand){.type = EDITOR_COMMAND_SOFT_AREA_ORDER_SET,
        .data.soft_area_order = {object->id, body_id, area_id, 0}};
    assert(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    assert(body->areas[0].id == area_id);
    assert(editor_history_undo(&history) && body->areas[1].id == area_id);
    EditorSelectionRef target = {EDITOR_SELECTION_SOFT_AREA, object->id, body_id, 0, body->areas[0].id};
    assert(editor_navigation_selection_reorder(&project, &state, ref, target, false, &history));
    body = editor_soft_body_get(&project.objects[0], body_id);
    assert(body->areas[0].id == area_id);
    assert(editor_history_undo(&history));
    object = &project.objects[0];
    body = editor_soft_body_get(object, body_id);
    assert(body->areas[1].id == area_id);
    command = (EditorCommand){.type = EDITOR_COMMAND_ITEM_ADD,
        .data.item_add = {.kind = EDITOR_ITEM_SOFT_HOLE, .object = object->id,
            .parent = body_id, .first = area_id, .name = "window"}};
    created = editor_command_execute(&project, &command);
    assert(created.kind == ERROR_RESULT_VALUE);
    EditorSoftHoleId hole_id = created.result.object;
    ref = (EditorSelectionRef){EDITOR_SELECTION_SOFT_HOLE, object->id, body_id, area_id, hole_id};
    assert(editor_viewport_selection_set(&project, &state, ref, false));
    assert(editor_navigation_selected_open(&project, &state));
    assert(state.mode == EDITOR_VIEWPORT_SOFT_HOLE && !state.soft_area_picking);
    state.soft_area_picking = true;
    assert(editor_viewport_soft_area_node_toggle(&project, &state, first));
    assert(editor_history_undo(&history));
    assert(editor_soft_hole_get(editor_soft_area_get(body, area_id), hole_id)->loop.node_count == 0);
    assert(editor_history_redo(&history));
    editor_viewport_back(&state);
    assert(!state.soft_area_picking && state.mode == EDITOR_VIEWPORT_SOFT_HOLE);
    editor_viewport_back(&state);
    assert(state.mode == EDITOR_VIEWPORT_SOFT_AREA);
    ref = (EditorSelectionRef){EDITOR_SELECTION_SOFT_NODE, object->id, body_id, 0, first};
    assert(editor_viewport_selection_set(&project, &state, ref, false));
    assert(editor_navigation_selected_open(&project, &state));
    assert(state.mode == EDITOR_VIEWPORT_SOFT_NODE);
    assert(editor_soft_hole_get(editor_soft_area_get(body, area_id), hole_id)->loop.node_count == 1);
    command = (EditorCommand){.type = EDITOR_COMMAND_ITEM_REMOVE,
        .data.item_remove = {EDITOR_ITEM_SOFT_HOLE, object->id, body_id, hole_id, area_id}};
    assert(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
    assert(editor_soft_area_get(body, area_id)->hole_count == 0);
    assert(editor_history_undo(&history));
    assert(editor_soft_area_get(body, area_id)->hole_count == 1);
    editor_command_executing_callback_set(NULL, NULL);
    editor_command_finished_callback_set(NULL, NULL);
    callback_history = NULL;
    editor_viewport_state_destroy(&state);
    editor_history_destroy(&history);
    editor_project_destroy(&project);
}

int main(void) {
    area_authoring_history_check();
    default_shapes_history_check();
    if(!SDL_SaveFile("editor_history_frame.png", test_png, sizeof(test_png))) return 1;
    angular_history_check();
    animation_frame_history_check();
    center_of_mass_interaction_check();
    static EditorProject project;
    EditorHistory history;
    EditorRigidBody *body;
    EditorHitbox *hitbox;
    EditorAnchor *anchor;
    EditorSoftBody *soft_body;
    EditorSoftNode *soft_node;
    Position original_vertex;
    EditorCommand command = {.type = EDITOR_COMMAND_ITEM_ADD,
        .data.item_add = {.kind = EDITOR_ITEM_OBJECT}};
    EditorCommandResult result;

    {
        EditorViewportState transform = {0};
#define TRANSFORM_FLAG_CHECK(member) \
        do { \
            editor_viewport_state_init(&transform); \
            transform.member = true; \
            assert(editor_viewport_transform_active_check(&transform)); \
            editor_viewport_transform_cancel(&transform); \
            assert(!editor_viewport_transform_active_check(&transform)); \
        } while(0)
        TRANSFORM_FLAG_CHECK(dragged_body);
        TRANSFORM_FLAG_CHECK(rotated_body);
        TRANSFORM_FLAG_CHECK(dragged_anchor);
        TRANSFORM_FLAG_CHECK(dragged_soft_node);
        TRANSFORM_FLAG_CHECK(dragged_soft_body);
        TRANSFORM_FLAG_CHECK(dragged_sprite);
        TRANSFORM_FLAG_CHECK(dragged_animated_sprite);
        TRANSFORM_FLAG_CHECK(rotated_sprite);
        TRANSFORM_FLAG_CHECK(rotated_animated_sprite);
        TRANSFORM_FLAG_CHECK(rotated_soft_body);
        TRANSFORM_FLAG_CHECK(dragged_origin);
        TRANSFORM_FLAG_CHECK(dragged_center_of_mass);
        TRANSFORM_FLAG_CHECK(group_dragging);
        TRANSFORM_FLAG_CHECK(group_rotating);
        editor_viewport_state_init(&transform);
        transform.dragged_vertex = 0;
        assert(editor_viewport_transform_active_check(&transform));
        editor_viewport_transform_cancel(&transform);
        assert(!editor_viewport_transform_active_check(&transform));
#undef TRANSFORM_FLAG_CHECK
    }
    EditorViewportState viewport = {0};

    editor_project_init(&project);
    assert(editor_history_init(&history, &project));
    snprintf(command.data.item_add.name, sizeof(command.data.item_add.name), "Car");
    editor_history_command_begin(&history, &project, &command);
    result = editor_command_execute(&project, &command);
    editor_history_command_finish(&history, &command, &result);
    assert(result.kind == ERROR_RESULT_VALUE);
    assert(project.object_count == 1);
    assert(editor_history_memory_get(&history) < 8192);
    assert(editor_history_undo(&history));
    assert(project.object_count == 0);
    assert(editor_history_redo(&history));
    assert(project.object_count == 1);
    assert(strcmp(project.objects[0].name, "Car") == 0);

    {
        EditorLayoutViewport *layout = editor_project_layout_viewport_add(&project);
        EditorViewportUiItem *slider;
        Position original = project.objects[0].position;
        assert(layout != NULL);
        editor_history_reset(&history);
        assert(editor_history_ui_change_begin(&history));
        slider = editor_viewport_ui_add(&project, layout,
            EDITOR_VIEWPORT_UI_SLIDER);
        assert(slider != NULL);
        slider->position = (Position){80.0f, 90.0f};
        assert(editor_history_ui_change_finish(&history));
        assert(history.undo_count == 1 && layout->ui_item_count == 1);
        command = (EditorCommand){.type = EDITOR_COMMAND_OBJECT_POSITION,
            .data.object_position = {.object = project.objects[0].id,
                .position = {12.0f, 34.0f}}};
        editor_history_command_begin(&history, &project, &command);
        result = editor_command_execute(&project, &command);
        editor_history_command_finish(&history, &command, &result);
        assert(history.undo_count == 2);
        assert(editor_history_undo(&history));
        assert(!editor_history_last_restore_ui_check(&history));
        assert(project.objects[0].position.x == original.x &&
            project.objects[0].position.y == original.y &&
            project.layout_viewports[0].ui_item_count == 1);
        assert(editor_history_undo(&history));
        assert(editor_history_last_restore_ui_check(&history));
        assert(project.layout_viewports[0].ui_item_count == 0);
        assert(editor_history_redo(&history));
        assert(project.layout_viewports[0].ui_item_count == 1);
        assert(editor_history_redo(&history));
        assert(project.objects[0].position.x == 12.0f &&
            project.objects[0].position.y == 34.0f);
        assert(editor_history_undo(&history));
        assert(editor_history_undo(&history));

        editor_history_reset(&history);
        slider = editor_viewport_ui_add(&project,
            &project.layout_viewports[0], EDITOR_VIEWPORT_UI_SLIDER);
        assert(slider != NULL);
        Position drag_start = slider->position;
        editor_history_continuous_set(&history, true);
        assert(editor_history_ui_change_begin(&history));
        assert(editor_history_ui_change_finish(&history));
        assert(editor_history_ui_change_begin(&history));
        slider->position.x += 10.0f;
        assert(editor_history_ui_change_finish(&history));
        editor_history_continuous_set(&history, true);
        assert(editor_history_ui_change_begin(&history));
        slider->position.x += 15.0f;
        assert(editor_history_ui_change_finish(&history));
        editor_history_continuous_set(&history, true);
        assert(editor_history_ui_change_begin(&history));
        slider->position.x += 25.0f;
        assert(editor_history_ui_change_finish(&history));
        editor_history_continuous_set(&history, false);
        assert(history.undo_count == 1);
        assert(editor_history_undo(&history));
        assert(project.layout_viewports[0].ui_items[0].position.x == drag_start.x);
        assert(editor_history_redo(&history));
        assert(project.layout_viewports[0].ui_items[0].position.x ==
            drag_start.x + 50.0f);

        {
            EditorViewportUiItem *items[3];
            EditorViewportUiItemId shape_id;
            EditorViewportUiItemId text_id;
            EditorHierarchySelection selections[3] = {
                EDITOR_SELECTION_UI_SHAPE,
                EDITOR_SELECTION_UI_TEXT,
                EDITOR_SELECTION_UI_SLIDER};
            EditorViewportMode modes[3] = {
                EDITOR_VIEWPORT_UI_SHAPE_EDITOR,
                EDITOR_VIEWPORT_UI_TEXT_EDITOR,
                EDITOR_VIEWPORT_UI_SLIDER_EDITOR};
            items[0] = editor_viewport_ui_add(&project,
                &project.layout_viewports[0], EDITOR_VIEWPORT_UI_SHAPE);
            assert(items[0] != NULL);
            shape_id = items[0]->id;
            items[1] = editor_viewport_ui_add(&project,
                &project.layout_viewports[0], EDITOR_VIEWPORT_UI_TEXT);
            assert(items[1] != NULL);
            text_id = items[1]->id;
            items[0] = items[1] = items[2] = NULL;
            for(size_t i = 0; i < project.layout_viewports[0].ui_item_count; i++) {
                EditorViewportUiItem *candidate =
                    &project.layout_viewports[0].ui_items[i];
                if(candidate->id == shape_id) items[0] = candidate;
                if(candidate->id == text_id) items[1] = candidate;
                if(candidate->kind == EDITOR_VIEWPORT_UI_SLIDER) items[2] = candidate;
            }
            assert(items[0] != NULL && items[1] != NULL && items[2] != NULL);
            for(size_t rotation_index = 0; rotation_index < 3;
                    rotation_index += 1) {
                EditorViewportUiItem *item = items[rotation_index];
                Position pivot = item->position;
                Orientation *orientation = &item->rotation;
                if(item->kind == EDITOR_VIEWPORT_UI_SHAPE) {
                    pivot.x += EDITOR_UI_SHAPE_DEFAULT_SIZE * 0.5f;
                    pivot.y += EDITOR_UI_SHAPE_DEFAULT_SIZE * 0.5f;
                    orientation = &item->value.shape.rotation;
                }
                *orientation = 0.0f;
                editor_viewport_state_init(&viewport);
                viewport.mode = modes[rotation_index];
                viewport.selection = selections[rotation_index];
                viewport.selected_layout_viewport =
                    project.layout_viewports[0].id;
                viewport.selected_viewport_ui_item = item->id;
                Position handle = editor_rotation_control_screen_position_get(pivot,
                    *orientation, EDITOR_VIEWPORT_ROTATION_ARM_LENGTH);
                assert(editor_viewport_update(&viewport, &project,
                    test_layout_to_screen(&project,
                        &project.layout_viewports[0], handle),
                    MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                    false, 0.0f, false));
                assert(viewport.rotated_viewport_item);
                Position rotated_pointer = {pivot.x +
                    EDITOR_VIEWPORT_ROTATION_ARM_LENGTH, pivot.y};
                assert(editor_viewport_update(&viewport, &project,
                    test_layout_to_screen(&project,
                        &project.layout_viewports[0], rotated_pointer),
                    MOUSE_BUTTON_STATE_DOWN, MOUSE_BUTTON_STATE_UP,
                    false, 0.0f, false));
                assert(fabsf(*orientation - 90.0f) < 0.001f);
                for(int step = 2; step <= 9; step += 1) {
                    rotated_pointer = editor_rotation_control_screen_position_get(pivot,
                        step * 90.0f, EDITOR_VIEWPORT_ROTATION_ARM_LENGTH);
                    assert(editor_viewport_update(&viewport, &project,
                        test_layout_to_screen(&project, &project.layout_viewports[0], rotated_pointer),
                        MOUSE_BUTTON_STATE_DOWN, MOUSE_BUTTON_STATE_UP, false, 0, false));
                    assert(fabsf(*orientation - step * 90.0f) < 0.001f);
                }
                (void)editor_viewport_update(&viewport, &project,
                    test_layout_to_screen(&project,
                        &project.layout_viewports[0], rotated_pointer),
                    MOUSE_BUTTON_STATE_RELEASED, MOUSE_BUTTON_STATE_UP,
                    false, 0.0f, false);
            }
        }
    }

    editor_history_reset(&history);
    command = (EditorCommand){
        .type = EDITOR_COMMAND_PROJECT_PHYSICS_SETTINGS_SET,
        .data.project_physics_settings_set = {
            .engine_time_per_tick = 1.0 / 120.0,
            .physics_timestep_override = true,
            .physics_dt_per_tick = 1.0 / 240.0,
            .physics_substeps = 4,
            .physics_gravity = {2.0f, -18.0f},
            .physics_solver_iterations = 12}};
    editor_history_command_begin(&history, &project, &command);
    result = editor_command_execute(&project, &command);
    editor_history_command_finish(&history, &command, &result);
    assert(result.kind == ERROR_RESULT_VALUE &&
        fabs(project.engine_time_per_tick - 1.0 / 120.0) < 0.000001 &&
        project.physics_timestep_override && project.physics_substeps == 4 &&
        project.physics_solver_iterations == 12);
    assert(editor_history_undo(&history) &&
        fabs(project.engine_time_per_tick - 1.0 / 60.0) < 0.000001 &&
        !project.physics_timestep_override && project.physics_substeps == 1 &&
        project.physics_solver_iterations == PHYSICS_SOLVER_ITERATIONS_DEFAULT);
    assert(editor_history_redo(&history) &&
        fabs(project.physics_dt_per_tick - 1.0 / 240.0) < 0.000001 &&
        fabsf(project.physics_gravity.x - 2.0f) < 0.0001f &&
        fabsf(project.physics_gravity.y + 18.0f) < 0.0001f);

    editor_history_reset(&history);
    command = (EditorCommand){.type = EDITOR_COMMAND_COLLISION_MASK_ADD};
    snprintf(command.data.collision_mask_add.name,
        sizeof(command.data.collision_mask_add.name), "vehicle");
    editor_history_command_begin(&history, &project, &command);
    result = editor_command_execute(&project, &command);
    editor_history_command_finish(&history, &command, &result);
    assert(result.kind == ERROR_RESULT_VALUE && project.collision_mask_count == 2);
    assert(editor_history_undo(&history) && project.collision_mask_count == 1);
    assert(editor_history_redo(&history) && project.collision_mask_count == 2);

    editor_history_reset(&history);
    command = (EditorCommand){.type = EDITOR_COMMAND_ITEM_ADD,
        .data.item_add = {.kind = EDITOR_ITEM_RIGID_BODY,
            .object = project.objects[0].id}};
    editor_history_command_begin(&history, &project, &command);
    result = editor_command_execute(&project, &command);
    editor_history_command_finish(&history, &command, &result);
    assert(result.kind == ERROR_RESULT_VALUE);
    {
        EditorRigidBodyId added_body = result.result.object;
        assert(project.objects[0].rigid_body_count == 1);
        assert(editor_history_undo(&history));
        assert(project.objects[0].rigid_body_count == 0);
        assert(editor_history_redo(&history));
        assert(project.objects[0].rigid_body_count == 1);
        assert(project.objects[0].rigid_bodies[0].id == added_body);
        command = (EditorCommand){.type = EDITOR_COMMAND_ITEM_REMOVE,
            .data.item_remove = {.kind = EDITOR_ITEM_RIGID_BODY,
                .object = project.objects[0].id, .item = added_body}};
        editor_history_command_begin(&history, &project, &command);
        result = editor_command_execute(&project, &command);
        editor_history_command_finish(&history, &command, &result);
        assert(result.kind == ERROR_RESULT_VALUE);
        assert(project.objects[0].rigid_body_count == 0);
        assert(editor_history_undo(&history));
        assert(project.objects[0].rigid_body_count == 1);
        assert(editor_history_redo(&history));
        assert(project.objects[0].rigid_body_count == 0);
    }

    body = editor_project_rigid_body_add(&project, &project.objects[0]);
    assert(body != NULL);
    editor_history_reset(&history);
    command = (EditorCommand){.type = EDITOR_COMMAND_PROPERTY_SET,
        .data.property_set = {EDITOR_ITEM_RIGID_BODY, project.objects[0].id,
            0, body->id, 0, EDITOR_PROPERTY_INITIAL_VELOCITY_X,
            EDITOR_PROPERTY_VALUE_FLOAT, {.number = 42.0f}}};
    editor_history_command_begin(&history, &project, &command);
    result = editor_command_execute(&project, &command);
    editor_history_command_finish(&history, &command, &result);
    assert(result.kind == ERROR_RESULT_VALUE &&
        fabsf(body->initial_velocity.x - 42.0f) < 0.001f);
    assert(editor_history_undo(&history) &&
        fabsf(body->initial_velocity.x) < 0.001f);
    assert(editor_history_redo(&history) &&
        fabsf(body->initial_velocity.x - 42.0f) < 0.001f);
    hitbox = editor_project_hitbox_add(&project, body);
    assert(hitbox != NULL && hitbox->vertex_count > 0);
    EditorHitboxId radius_test_hitbox_id = hitbox->id;
    {
        const EditorViewportMode modes[] = {
            EDITOR_VIEWPORT_RIGID_BODY, EDITOR_VIEWPORT_PARTICLE};
        EditorRigidBodyId body_id = body->id;
        Position local_vertex = hitbox->vertices[0].position;
        for(size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i += 1) {
            Position center = {project.objects[0].position.x + body->position.x,
                project.objects[0].position.y + body->position.y};
            Position handle = test_world_to_screen((Position){center.x,
                center.y + EDITOR_VIEWPORT_ROTATION_ARM_LENGTH});
            Position target = test_world_to_screen((Position){center.x +
                EDITOR_VIEWPORT_ROTATION_ARM_LENGTH, center.y});
            body->particle = true;
            body->standalone_particle = false;
            body->rotation = 0.0f;
            editor_history_reset(&history);
            editor_viewport_state_init(&viewport);
            viewport.mode = modes[i];
            viewport.selection = modes[i] == EDITOR_VIEWPORT_RIGID_BODY ?
                EDITOR_SELECTION_RIGID_BODY : EDITOR_SELECTION_PARTICLE;
            viewport.selected_rigid_body = body_id;
            assert(viewport_pointer_update(&history, &viewport, &project,
                handle, MOUSE_BUTTON_STATE_PRESSED));
            assert(viewport.rotated_body && !viewport.dragged_body);
            assert(viewport_pointer_update(&history, &viewport, &project,
                target, MOUSE_BUTTON_STATE_DOWN));
            assert(fabsf(body->rotation - 90.0f) < 0.001f);
            assert(!viewport_pointer_update(&history, &viewport, &project,
                target, MOUSE_BUTTON_STATE_RELEASED));
            assert(history.undo_count == 1);
            assert(editor_history_undo(&history));
            body = editor_project_rigid_body_get(&project.objects[0], body_id);
            assert(body != NULL && fabsf(body->rotation) < 0.001f);
            assert(body->particle && !body->standalone_particle);
            assert(editor_history_redo(&history));
            body = editor_project_rigid_body_get(&project.objects[0], body_id);
            assert(body != NULL && fabsf(body->rotation - 90.0f) < 0.001f);
            hitbox = editor_project_hitbox_get(body, radius_test_hitbox_id);
            assert(hitbox != NULL &&
                hitbox->vertices[0].position.x == local_vertex.x &&
                hitbox->vertices[0].position.y == local_vertex.y);
        }
        body->rotation = 0.0f;
    }
    {
        Position rotation_handle = test_world_to_screen((Position){
            body->position.x,
            body->position.y + EDITOR_VIEWPORT_ROTATION_ARM_LENGTH});
        body->particle = true;
        body->standalone_particle = true;
        for(size_t i = 0; i < 2; i += 1) {
            editor_viewport_state_init(&viewport);
            viewport.mode = i == 0 ? EDITOR_VIEWPORT_RIGID_BODY :
                EDITOR_VIEWPORT_PARTICLE;
            viewport.selection = EDITOR_SELECTION_PARTICLE;
            viewport.selected_rigid_body = body->id;
            (void)editor_viewport_update(&viewport, &project, rotation_handle,
                MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false);
            assert(!viewport.rotated_body && body->rotation == 0.0f);
            editor_viewport_transform_cancel(&viewport);
        }
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_PARTICLE_RADIUS;
        viewport.selection = EDITOR_SELECTION_PARTICLE;
        viewport.selected_rigid_body = body->id;
        (void)editor_viewport_update(&viewport, &project, rotation_handle,
            MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
            false, 0.0f, false);
        assert(!viewport.rotated_body);
        editor_viewport_transform_cancel(&viewport);
        viewport.mode = EDITOR_VIEWPORT_PARTICLE_RADIUS;
        body->particle_auto_fit = false;
        body->particle_radius = 30.0f;
        EditorRigidBodyId particle_body_id = body->id;
        assert(editor_project_particle_hitbox_sync(&project, body));
        editor_history_reset(&history);
        Position particle_center = {project.objects[0].position.x + body->position.x +
            body->particle_origin.x, project.objects[0].position.y + body->position.y +
            body->particle_origin.y};
        Position radius_start = test_world_to_screen((Position){
            particle_center.x + 30.0f, particle_center.y});
        Position radius_end = test_world_to_screen((Position){
            particle_center.x + 45.0f, particle_center.y});
        assert(viewport_pointer_update(&history, &viewport, &project,
            radius_start, MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.dragged_particle_radius);
        assert(viewport_pointer_update(&history, &viewport, &project,
            radius_end, MOUSE_BUTTON_STATE_DOWN));
        (void)viewport_pointer_update(&history, &viewport, &project,
            radius_end, MOUSE_BUTTON_STATE_RELEASED);
        assert(fabsf(body->particle_radius - 45.0f) < 0.001f);
        assert(editor_history_undo(&history));
        body = editor_project_rigid_body_get(&project.objects[0], particle_body_id);
        assert(body != NULL && fabsf(body->particle_radius - 30.0f) < 0.001f);
        assert(editor_history_redo(&history));
        body = editor_project_rigid_body_get(&project.objects[0], particle_body_id);
        assert(body != NULL && fabsf(body->particle_radius - 45.0f) < 0.001f);
        body->particle = false;
        body->standalone_particle = false;
    }
    hitbox = editor_project_hitbox_get(body, radius_test_hitbox_id);
    assert(hitbox != NULL);
    original_vertex = hitbox->vertices[0].position;
    editor_history_reset(&history);
    command = (EditorCommand){.type = EDITOR_COMMAND_VERTEX_POSITION,
        .data.vertex_position = {.object = project.objects[0].id,
            .body = body->id, .hitbox = hitbox->id,
            .vertex = hitbox->vertices[0].id,
            .position = {10.0f, 0.0f}}};
    editor_history_command_begin(&history, &project, &command);
    result = editor_command_execute(&project, &command);
    editor_history_command_finish(&history, &command, &result);
    assert(result.kind == ERROR_RESULT_VALUE);
    editor_history_continuous_set(&history, true);
    command.data.vertex_position.position.x = 20.0f;
    editor_history_command_begin(&history, &project, &command);
    result = editor_command_execute(&project, &command);
    editor_history_command_finish(&history, &command, &result);
    assert(result.kind == ERROR_RESULT_VALUE);
    editor_history_continuous_set(&history, false);
    assert(editor_history_undo(&history));
    body = editor_project_rigid_body_get(&project.objects[0], body->id);
    hitbox = editor_project_hitbox_get(body, hitbox->id);
    assert(hitbox != NULL);
    assert(hitbox->vertices[0].position.x == original_vertex.x);
    assert(hitbox->vertices[0].position.y == original_vertex.y);
    assert(!editor_history_undo_check(&history));

    editor_history_reset(&history);
    editor_viewport_state_init(&viewport);
    project.objects[0].visible = true;
    body->visible = true;
    hitbox->visible = true;
    project.selected = project.objects[0].id;
    viewport.mode = EDITOR_VIEWPORT_HITBOX;
    viewport.selection = EDITOR_SELECTION_HITBOX;
    viewport.selected_rigid_body = body->id;
    viewport.selected_hitbox = hitbox->id;
    callback_history = &history;
    editor_command_executing_callback_set(history_begin, NULL);
    editor_command_finished_callback_set(history_finish, NULL);
    {
        Position world = {
            project.objects[0].position.x + body->position.x +
                hitbox->vertices[0].position.x,
            project.objects[0].position.y + body->position.y +
                hitbox->vertices[0].position.y
        };
        Position grab = test_world_to_screen(world);
        grab.x += 5.0f;
        grab.y -= 3.0f;
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(history.undo_count == 0);
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_DOWN));
        assert(history.undo_count == 0);
        grab.x += 20.0f;
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_DOWN));
        assert(!viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_RELEASED));
        assert(history.undo_count == 1);
        assert(editor_history_memory_get(&history) < 4096);
        shortcut_apply(&history, SDLK_Z);
        assert(hitbox->vertices[0].position.x == original_vertex.x);
        shortcut_apply(&history, SDLK_Y);
        assert(hitbox->vertices[0].position.x == original_vertex.x + 20.0f);
    }


    anchor = editor_project_anchor_add(&project, &project.objects[0],
        (Position){200.0f, 100.0f}, 0);
    assert(anchor != NULL);
    editor_history_reset(&history);
    editor_viewport_state_init(&viewport);
    viewport.mode = EDITOR_VIEWPORT_OBJECT;
    {
        Position original = anchor->position;
        Position grab = test_world_to_screen((Position){
            project.objects[0].position.x + original.x,
            project.objects[0].position.y + original.y});
        grab.x += 4.0f;
        grab.y -= 2.0f;
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_DOWN));
        assert(history.undo_count == 0);
        grab.x += 20.0f;
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_DOWN));
        assert(!viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_RELEASED));
        assert(history.undo_count == 1);
        shortcut_apply(&history, SDLK_Z);
        assert(anchor->position.x == original.x);
        shortcut_apply(&history, SDLK_Y);
        assert(anchor->position.x == original.x + 20.0f);
    }

    soft_body = editor_project_soft_body_add(&project, &project.objects[0]);
    assert(soft_body != NULL);
    soft_node = editor_project_soft_node_add(&project, soft_body,
        (Position){-200.0f, -100.0f});
    assert(soft_node != NULL);
    editor_history_reset(&history);
    editor_viewport_state_init(&viewport);
    viewport.mode = EDITOR_VIEWPORT_SOFT_NODE;
    viewport.selection = EDITOR_SELECTION_SOFT_NODE;
    viewport.selected_soft_body = soft_body->id;
    viewport.selected_soft_node = soft_node->id;
    {
        Position original = soft_node->position;
        Position grab = test_world_to_screen((Position){
            project.objects[0].position.x + soft_body->position.x + original.x,
            project.objects[0].position.y + soft_body->position.y + original.y});
        grab.x += 3.0f;
        grab.y -= 4.0f;
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_DOWN));
        assert(history.undo_count == 0);
        grab.x += 20.0f;
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_DOWN));
        assert(!viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_RELEASED));
        assert(history.undo_count == 1);
        shortcut_apply(&history, SDLK_Z);
        soft_body = NULL;
        for(size_t i = 0; i < project.objects[0].soft_body_count; i += 1)
            if(project.objects[0].soft_body_items[i].id ==
                    viewport.selected_soft_body)
                soft_body = &project.objects[0].soft_body_items[i];
        soft_node = NULL;
        if(soft_body != NULL) for(size_t i = 0; i < soft_body->node_count; i += 1)
            if(soft_body->nodes[i].id == viewport.selected_soft_node)
                soft_node = &soft_body->nodes[i];
        assert(soft_node != NULL);
        assert(soft_node->position.x == original.x);
        shortcut_apply(&history, SDLK_Y);
        soft_body = NULL;
        for(size_t i = 0; i < project.objects[0].soft_body_count; i += 1)
            if(project.objects[0].soft_body_items[i].id ==
                    viewport.selected_soft_body)
                soft_body = &project.objects[0].soft_body_items[i];
        soft_node = NULL;
        if(soft_body != NULL) for(size_t i = 0; i < soft_body->node_count; i += 1)
            if(soft_body->nodes[i].id == viewport.selected_soft_node)
                soft_node = &soft_body->nodes[i];
        assert(soft_node != NULL);
        assert(soft_node->position.x == original.x + 20.0f);
    }
    editor_history_reset(&history);
    {
        Position before[EDITOR_HITBOX_VERTEX_MAX];
        for(size_t i = 0; i < hitbox->vertex_count; i += 1)
            before[i] = hitbox->vertices[i].position;
        command = (EditorCommand){.type = EDITOR_COMMAND_AUTO_SHAPE,
            .data.auto_shape = {.kind = EDITOR_ITEM_HITBOX,
                .object = project.objects[0].id, .parent = body->id,
                .item = hitbox->id,
                .config = {.kind = EDITOR_AUTO_SHAPE_CIRCLE, .radius = 30.0f}}};
        result = editor_command_execute(&project, &command);
        assert(result.kind == ERROR_RESULT_VALUE);
        assert(history.undo_count == 1);
        shortcut_apply(&history, SDLK_Z);
        for(size_t i = 0; i < hitbox->vertex_count; i += 1) {
            assert(hitbox->vertices[i].position.x == before[i].x);
            assert(hitbox->vertices[i].position.y == before[i].y);
        }
        shortcut_apply(&history, SDLK_Y);
        assert(fabsf(hypotf(hitbox->vertices[0].position.x,
            hitbox->vertices[0].position.y) - 30.0f) < 0.001f);
    }
    editor_history_reset(&history);
    editor_viewport_state_init(&viewport);
    viewport.mode = EDITOR_VIEWPORT_AUTO_SHAPE;
    viewport.auto_shape_parent_mode = EDITOR_VIEWPORT_HITBOX;
    viewport.selection = EDITOR_SELECTION_HITBOX;
    viewport.selected_rigid_body = body->id;
    viewport.selected_hitbox = hitbox->id;
    {
        EditorAutoShapeConfig config = {
            .kind = EDITOR_AUTO_SHAPE_CIRCLE, .radius = 30.0f
        };
        Position grab = test_world_to_screen((Position){
            project.objects[0].position.x + body->position.x,
            project.objects[0].position.y + body->position.y - config.radius});
        assert(auto_shape_pointer_update(&history, &viewport, &project, &config,
            grab, MOUSE_BUTTON_STATE_PRESSED));
        grab.y += 20.0f;
        assert(auto_shape_pointer_update(&history, &viewport, &project, &config,
            grab, MOUSE_BUTTON_STATE_DOWN));
        assert(!auto_shape_pointer_update(&history, &viewport, &project, &config,
            grab, MOUSE_BUTTON_STATE_RELEASED));
        assert(fabsf(config.radius - 50.0f) < 0.001f);
        assert(fabsf(hypotf(hitbox->vertices[0].position.x,
            hitbox->vertices[0].position.y) - 50.0f) < 0.001f);
        for(size_t i = 0; i < hitbox->vertex_count; i += 1) {
            float radius = hypotf(hitbox->vertices[i].position.x,
                hitbox->vertices[i].position.y);
            assert(fabsf(radius - 50.0f) < 0.001f);
        }
        assert(history.undo_count == 1);
        shortcut_apply(&history, SDLK_Z);
        assert(fabsf(hypotf(hitbox->vertices[0].position.x,
            hitbox->vertices[0].position.y) - 30.0f) < 0.001f);
        shortcut_apply(&history, SDLK_Y);
        assert(fabsf(hypotf(hitbox->vertices[0].position.x,
            hitbox->vertices[0].position.y) - 50.0f) < 0.001f);
    }

    assert(editor_project_soft_node_add(&project, soft_body,
        (Position){0.0f, 0.0f}) != NULL);
    assert(editor_project_soft_node_add(&project, soft_body,
        (Position){0.0f, 0.0f}) != NULL);
    {
        EditorAutoShapeConfig config = {
            .kind = EDITOR_AUTO_SHAPE_CIRCLE, .radius = 40.0f
        };
        command = (EditorCommand){.type = EDITOR_COMMAND_AUTO_SHAPE,
            .data.auto_shape = {.kind = EDITOR_ITEM_SOFT_BODY,
                .object = project.objects[0].id, .item = soft_body->id,
                .config = config}};
        result = editor_command_execute(&project, &command);
        assert(result.kind == ERROR_RESULT_VALUE);
        editor_history_reset(&history);
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_AUTO_SHAPE;
        viewport.auto_shape_parent_mode = EDITOR_VIEWPORT_SOFT_BODY;
        viewport.selection = EDITOR_SELECTION_SOFT_BODY;
        viewport.selected_soft_body = soft_body->id;
        {
            Position grab = test_world_to_screen((Position){
                project.objects[0].position.x + soft_body->position.x,
                project.objects[0].position.y + soft_body->position.y - config.radius});
            assert(auto_shape_pointer_update(&history, &viewport, &project,
                &config, grab, MOUSE_BUTTON_STATE_PRESSED));
            grab.y += 10.0f;
            assert(auto_shape_pointer_update(&history, &viewport, &project,
                &config, grab, MOUSE_BUTTON_STATE_DOWN));
            assert(!auto_shape_pointer_update(&history, &viewport, &project,
                &config, grab, MOUSE_BUTTON_STATE_RELEASED));
            assert(fabsf(config.radius - 50.0f) < 0.001f);
            for(size_t i = 0; i < soft_body->node_count; i += 1) {
                float radius = hypotf(soft_body->nodes[i].position.x,
                    soft_body->nodes[i].position.y);
                assert(fabsf(radius - 50.0f) < 0.001f);
            }
            assert(history.undo_count == 1);
            shortcut_apply(&history, SDLK_Z);
            assert(fabsf(soft_body->nodes[0].position.y + 40.0f) < 0.001f);
            shortcut_apply(&history, SDLK_Y);
            assert(fabsf(soft_body->nodes[0].position.y + 50.0f) < 0.001f);
        }
    }
    editor_command_executing_callback_set(NULL, NULL);
    editor_command_finished_callback_set(NULL, NULL);
    callback_history = NULL;

    {
        EditorSelectionRef rigid = {EDITOR_SELECTION_RIGID_BODY,
            project.objects[0].id, 0, 0, body->id};
        EditorSelectionRef soft = {EDITOR_SELECTION_SOFT_BODY,
            project.objects[0].id, 0, 0, soft_body->id};
        Position handle;
        Position target;
        project.objects[0].position = (Position){0};
        body->position = (Position){-10.0f, 0.0f};
        body->rotation = 0.0f;
        soft_body->position = (Position){10.0f, 0.0f};
        soft_body->rotation = 0.0f;
        editor_history_reset(&history);
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_OBJECT;
        assert(editor_viewport_selection_set(&project, &viewport, rigid, false));
        assert(editor_viewport_selection_set(&project, &viewport, soft, true));
        handle = test_world_to_screen((Position){-10.0f, 0.0f});
        target = test_world_to_screen((Position){-8.0f, 0.0f});
        assert(viewport_pointer_update(&history, &viewport, &project, handle,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.group_dragging && viewport.selected_item_count == 2);
        assert(viewport_pointer_update(&history, &viewport, &project, target,
            MOUSE_BUTTON_STATE_DOWN));
        assert(!viewport_pointer_update(&history, &viewport, &project, target,
            MOUSE_BUTTON_STATE_RELEASED));
        assert(fabsf(body->position.x + 8.0f) < 0.001f);
        assert(fabsf(soft_body->position.x - 12.0f) < 0.001f);
        assert(history.undo_count == 1);
        assert(editor_history_undo(&history));
        body = editor_project_rigid_body_get(&project.objects[0], rigid.item);
        soft_body = NULL;
        for(size_t i = 0; i < project.objects[0].soft_body_count; i += 1)
            if(project.objects[0].soft_body_items[i].id == soft.item)
                soft_body = &project.objects[0].soft_body_items[i];
        assert(body != NULL && soft_body != NULL);
        editor_history_reset(&history);
        assert(editor_history_transaction_begin(&history));
        assert(editor_history_transaction_object_track(&history, rigid.object));
        assert(editor_viewport_selection_nudge(&viewport, &project,
            (Vec2D){2.0f, 0.0f}));
        assert(editor_history_transaction_end(&history));
        assert(fabsf(body->position.x + 8.0f) < 0.001f);
        assert(fabsf(soft_body->position.x - 12.0f) < 0.001f);
        assert(history.undo_count == 1);
        assert(editor_history_undo(&history));
        body = editor_project_rigid_body_get(&project.objects[0], rigid.item);
        soft_body = NULL;
        for(size_t i = 0; i < project.objects[0].soft_body_count; i += 1)
            if(project.objects[0].soft_body_items[i].id == soft.item)
                soft_body = &project.objects[0].soft_body_items[i];
        assert(body != NULL && soft_body != NULL);
        editor_history_reset(&history);
        handle = test_world_to_screen((Position){0.0f,
            EDITOR_VIEWPORT_ROTATION_ARM_LENGTH});
        target = test_world_to_screen((Position){
            EDITOR_VIEWPORT_ROTATION_ARM_LENGTH, 0.0f});
        assert(viewport_pointer_update(&history, &viewport, &project, handle,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.group_rotating);
        assert(viewport_pointer_update(&history, &viewport, &project, target,
            MOUSE_BUTTON_STATE_DOWN));
        assert(!viewport_pointer_update(&history, &viewport, &project, target,
            MOUSE_BUTTON_STATE_RELEASED));
        assert(fabsf(body->position.x) < 0.001f &&
            fabsf(body->position.y - 10.0f) < 0.001f);
        assert(fabsf(soft_body->position.x) < 0.001f &&
            fabsf(soft_body->position.y + 10.0f) < 0.001f);
        assert(fabsf(body->rotation - 90.0f) < 0.001f);
        assert(fabsf(soft_body->rotation - 90.0f) < 0.001f);
        assert(history.undo_count == 1);
        assert(editor_history_undo(&history));
        body = editor_project_rigid_body_get(&project.objects[0], rigid.item);
        soft_body = NULL;
        for(size_t i = 0; i < project.objects[0].soft_body_count; i += 1)
            if(project.objects[0].soft_body_items[i].id == soft.item)
                soft_body = &project.objects[0].soft_body_items[i];
        assert(body != NULL && soft_body != NULL);
        assert(fabsf(body->position.x + 10.0f) < 0.001f);
        assert(fabsf(soft_body->position.x - 10.0f) < 0.001f);

        body->rotation = 0.0f;
        soft_body->rotation = 0.0f;
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_RIGID_BODY;
        assert(editor_viewport_selection_set(&project, &viewport, soft, false));
        assert(editor_viewport_selection_set(&project, &viewport, rigid, true));
        handle = test_world_to_screen((Position){body->position.x,
            body->position.y + EDITOR_VIEWPORT_ROTATION_ARM_LENGTH});
        target = test_world_to_screen((Position){body->position.x +
            EDITOR_VIEWPORT_ROTATION_ARM_LENGTH, body->position.y});
        assert(viewport_pointer_update(&history, &viewport, &project, handle,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.rotated_body && !viewport.group_rotating);
        assert(viewport_pointer_update(&history, &viewport, &project, target,
            MOUSE_BUTTON_STATE_DOWN));
        assert(fabsf(body->rotation - 90.0f) < 0.001f);
        assert(fabsf(soft_body->rotation - 90.0f) < 0.001f);
        assert(!viewport_pointer_update(&history, &viewport, &project, target,
            MOUSE_BUTTON_STATE_RELEASED));
        body->rotation = 0.0f;
        soft_body->rotation = 0.0f;
        editor_history_reset(&history);
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_SOFT_BODY;
        assert(editor_viewport_selection_set(&project, &viewport, rigid, false));
        assert(editor_viewport_selection_set(&project, &viewport, soft, true));
        handle = test_world_to_screen((Position){soft_body->position.x,
            soft_body->position.y + EDITOR_VIEWPORT_ROTATION_ARM_LENGTH});
        target = test_world_to_screen((Position){soft_body->position.x +
            EDITOR_VIEWPORT_ROTATION_ARM_LENGTH, soft_body->position.y});
        assert(viewport_pointer_update(&history, &viewport, &project, handle,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.rotated_soft_body && !viewport.group_rotating);
        assert(viewport_pointer_update(&history, &viewport, &project, target,
            MOUSE_BUTTON_STATE_DOWN));
        assert(fabsf(body->rotation - 90.0f) < 0.001f);
        assert(fabsf(soft_body->rotation - 90.0f) < 0.001f);
        assert(!viewport_pointer_update(&history, &viewport, &project, target,
            MOUSE_BUTTON_STATE_RELEASED));
        body->rotation = 0.0f;
        soft_body->rotation = 0.0f;
    }

    {
        EditorObject *object = &project.objects[0];
        EditorRigidBody *connected_body = editor_project_rigid_body_add(
            &project, object);
        EditorAnchor *first_anchor;
        EditorAnchor *second_anchor;
        EditorJoint *joint;
        EditorSelectionRef first;
        EditorSelectionRef second;

        assert(connected_body != NULL);
        body = editor_project_rigid_body_get(object, body->id);
        assert(body != NULL);
        body->position = (Position){-10.0f, 0.0f};
        connected_body->position = (Position){10.0f, 0.0f};
        first_anchor = editor_project_anchor_add(&project, object,
            (Position){10.0f, 0.0f}, body->id);
        second_anchor = editor_project_anchor_add(&project, object,
            (Position){-10.0f, 0.0f}, connected_body->id);
        joint = editor_project_joint_add(&project, object, EDITOR_JOINT_WELD);
        assert(first_anchor != NULL && second_anchor != NULL && joint != NULL);
        assert(editor_project_joint_anchor_set(object, joint, 0, first_anchor->id));
        assert(editor_project_joint_anchor_set(object, joint, 1, second_anchor->id));
        first = (EditorSelectionRef){EDITOR_SELECTION_RIGID_BODY,
            object->id, 0, 0, body->id};
        second = (EditorSelectionRef){EDITOR_SELECTION_RIGID_BODY,
            object->id, 0, 0, connected_body->id};
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_OBJECT;
        assert(editor_viewport_selection_set(&project, &viewport, first, false));
        assert(editor_viewport_selection_set(&project, &viewport, second, true));
        assert(editor_viewport_selection_nudge(&viewport, &project,
            (Vec2D){2.0f, 0.0f}));
        assert(fabsf(body->position.x + 8.0f) < 0.001f);
        assert(fabsf(connected_body->position.x - 12.0f) < 0.001f);

        {
            Position shared_handle = test_world_to_screen((Position){2.0f,
                EDITOR_VIEWPORT_ROTATION_ARM_LENGTH});
            Position shared_target = test_world_to_screen((Position){
                2.0f + EDITOR_VIEWPORT_ROTATION_ARM_LENGTH, 0.0f});
            assert(editor_viewport_update(&viewport, &project, shared_handle,
                MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false));
            assert(viewport.group_rotating);
            assert(editor_viewport_update(&viewport, &project, shared_target,
                MOUSE_BUTTON_STATE_DOWN, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false));
            assert(fabsf(body->rotation - 90.0f) < 0.001f);
            assert(fabsf(connected_body->rotation - 90.0f) < 0.001f);
            assert(!editor_viewport_update(&viewport, &project, shared_target,
                MOUSE_BUTTON_STATE_RELEASED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false));
        }

        body->position = (Position){-10.0f, 0.0f};
        body->rotation = 0.0f;
        connected_body->position = (Position){10.0f, 0.0f};
        connected_body->rotation = 0.0f;
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_RIGID_BODY;
        assert(editor_viewport_selection_set(&project, &viewport, first, false));
        assert(editor_viewport_selection_set(&project, &viewport, second, true));
        {
            EditorRigidBody *rotation_body = editor_project_rigid_body_get(
                object, viewport.selected_rigid_body);
            Position rotation_center;
            assert(rotation_body != NULL);
            rotation_center = rotation_body->position;
            Position own_handle = test_world_to_screen(
                (Position){rotation_center.x, rotation_center.y + EDITOR_VIEWPORT_ROTATION_ARM_LENGTH});
            Position own_target = test_world_to_screen(
                (Position){rotation_center.x +
                    EDITOR_VIEWPORT_ROTATION_ARM_LENGTH, rotation_center.y});
            assert(editor_viewport_update(&viewport, &project, own_handle,
                MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false));
            assert(viewport.rotated_body && !viewport.group_rotating);
            assert(editor_viewport_update(&viewport, &project, own_target,
                MOUSE_BUTTON_STATE_DOWN, MOUSE_BUTTON_STATE_UP,
                false, 0.0f, false));
            assert(fabsf(body->rotation - 90.0f) < 0.001f);
            assert(fabsf(connected_body->rotation - 90.0f) < 0.001f);
        }
    }

    {
        EditorObject *object = &project.objects[0];
        EditorSprite *sprite = editor_project_sprite_add(
            &project, object, "drag_sprite", "sprite.png");
        EditorSpriteId sprite_id;
        Position grab;
        Position moved;
        assert(sprite != NULL);
        sprite_id = sprite->id;
        sprite->position = (Position){300.0f, 150.0f};
        editor_history_reset(&history);
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_OBJECT;
        grab = test_world_to_screen((Position){300.0f, 150.0f});
        moved = grab;
        moved.x += 25.0f;
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.dragged_sprite);
        assert(viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_DOWN));
        moved.x += 15.0f;
        assert(viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_DOWN));
        assert(!viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_RELEASED));
        assert(history.undo_count == 1);
        assert(editor_history_undo(&history));
        object = &project.objects[0];
        sprite = editor_project_sprite_get(object, sprite_id);
        assert(sprite != NULL && sprite->position.x == 300.0f);
        assert(editor_history_redo(&history));
        object = &project.objects[0];
        sprite = editor_project_sprite_get(object, sprite_id);
        assert(sprite != NULL && sprite->position.x == 340.0f);

        editor_history_reset(&history);
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_SPRITE;
        viewport.selection = EDITOR_SELECTION_SPRITE;
        viewport.selected_sprite = sprite_id;
        grab = test_world_to_screen((Position){sprite->position.x,
            sprite->position.y + EDITOR_VIEWPORT_ROTATION_ARM_LENGTH});
        moved = test_world_to_screen((Position){
            sprite->position.x + EDITOR_VIEWPORT_ROTATION_ARM_LENGTH,
            sprite->position.y});
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.rotated_sprite);
        assert(viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_DOWN));
        assert(!viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_RELEASED));
        assert(history.undo_count == 1);
        assert(editor_history_undo(&history));
        object = &project.objects[0];
        sprite = editor_project_sprite_get(object, sprite_id);
        assert(sprite != NULL && fabsf(sprite->rotation) < 0.001f);
        assert(editor_history_redo(&history));
        object = &project.objects[0];
        sprite = editor_project_sprite_get(object, sprite_id);
        assert(sprite != NULL &&
            fabsf(sprite->rotation - 90.0f) < 0.001f);
        assert(editor_project_sprite_remove(object, sprite_id));
    }

    {
        EditorObject *object = &project.objects[0];
        EditorRigidBodyId body_id = body->id;
        Position grab;
        Position moved;
        Position local_vertex;
        body = editor_project_rigid_body_get(object, body_id);
        assert(body != NULL);
        local_vertex = body->hitboxes[0].vertices[0].position;
        body->position = (Position){100.0f, -150.0f};
        /* Keep COM away from the origin under test. */
        body->center_of_mass_explicit = true;
        body->center_of_mass_offset = (Position){200, 0};
        /* Attached anchors and joints render above the origin. Hide them to expose the
         * origin for this transform-history test. */
        for(size_t i = 0; i < object->anchor_count; i += 1)
            object->anchors[i].visible = false;
        for(size_t i = 0; i < object->joint_count; i += 1)
            object->joint_items[i].visible = false;
        editor_history_reset(&history);
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_ORIGIN;
        viewport.selection = EDITOR_SELECTION_ORIGIN;
        viewport.selected_origin_kind = EDITOR_ORIGIN_RIGID_BODY;
        viewport.selected_rigid_body = body_id;
        grab = test_world_to_screen(body->position);
        moved = grab;
        moved.x += 30.0f;
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.dragged_origin);
        assert(viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_DOWN));
        assert(!viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_RELEASED));
        assert(history.undo_count == 1);
        assert(editor_history_undo(&history));
        object = &project.objects[0];
        body = editor_project_rigid_body_get(object, body_id);
        assert(body != NULL && body->position.x == 100.0f);
        assert(body->hitboxes[0].vertices[0].position.x == local_vertex.x &&
            body->hitboxes[0].vertices[0].position.y == local_vertex.y);
        assert(editor_history_redo(&history));
        object = &project.objects[0];
        body = editor_project_rigid_body_get(object, body_id);
        assert(body != NULL && body->position.x == 130.0f);
        assert(body->hitboxes[0].vertices[0].position.x == local_vertex.x &&
            body->hitboxes[0].vertices[0].position.y == local_vertex.y);
        for(size_t i = 0; i < object->anchor_count; i += 1)
            object->anchors[i].visible = true;
    }

    {
        EditorObject *object = &project.objects[0];
        EditorSoftBodyId body_id = soft_body->id;
        Position grab;
        Position moved;
        Position local_node;
        soft_body = NULL;
        for(size_t i = 0; i < object->soft_body_count; i += 1)
            if(object->soft_body_items[i].id == body_id)
                soft_body = &object->soft_body_items[i];
        assert(soft_body != NULL);
        local_node = soft_body->nodes[0].position;
        soft_body->position = (Position){-100.0f, -150.0f};
        editor_history_reset(&history);
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_ORIGIN;
        viewport.selection = EDITOR_SELECTION_ORIGIN;
        viewport.selected_origin_kind = EDITOR_ORIGIN_SOFT_BODY;
        viewport.selected_soft_body = body_id;
        grab = test_world_to_screen(soft_body->position);
        moved = grab;
        moved.x += 30.0f;
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.dragged_origin);
        assert(viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_DOWN));
        assert(!viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_RELEASED));
        assert(history.undo_count == 1);
        assert(editor_history_undo(&history));
        object = &project.objects[0];
        soft_body = NULL;
        for(size_t i = 0; i < object->soft_body_count; i += 1)
            if(object->soft_body_items[i].id == body_id)
                soft_body = &object->soft_body_items[i];
        assert(soft_body != NULL && soft_body->position.x == -100.0f);
        assert(soft_body->nodes[0].position.x == local_node.x &&
            soft_body->nodes[0].position.y == local_node.y);
        assert(editor_history_redo(&history));
        object = &project.objects[0];
        soft_body = NULL;
        for(size_t i = 0; i < object->soft_body_count; i += 1)
            if(object->soft_body_items[i].id == body_id)
                soft_body = &object->soft_body_items[i];
        assert(soft_body != NULL && soft_body->position.x == -70.0f);
        assert(soft_body->nodes[0].position.x == local_node.x &&
            soft_body->nodes[0].position.y == local_node.y);
    }

    {
        EditorObject *object = &project.objects[0];
        EditorAnimatedSprite *animation =
            editor_project_animated_sprite_add(&project, object);
        EditorAnimatedSpriteId animation_id;
        Position grab;
        Position moved;
        assert(animation != NULL);
        animation_id = animation->id;
        animation->editor_position = (Position){-300.0f, 150.0f};
        assert(editor_project_animation_frame_add(&project, animation,
            "frame", "editor_history_frame.png", (Scale){64.0f, 64.0f}));
        editor_history_reset(&history);
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_OBJECT;
        grab = test_world_to_screen((Position){-300.0f, 150.0f});
        moved = grab;
        moved.x += 40.0f;
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.dragged_animated_sprite);
        assert(viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_DOWN));
        assert(!viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_RELEASED));
        assert(history.undo_count == 1);
        assert(editor_history_undo(&history));
        object = &project.objects[0];
        animation = editor_project_animated_sprite_get(object, animation_id);
        assert(animation != NULL && animation->editor_position.x == -300.0f);
        assert(editor_history_redo(&history));
        object = &project.objects[0];
        animation = editor_project_animated_sprite_get(object, animation_id);
        assert(animation != NULL && animation->editor_position.x == -260.0f);

        editor_history_reset(&history);
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_ANIMATED_SPRITE;
        viewport.selection = EDITOR_SELECTION_ANIMATED_SPRITE;
        viewport.selected_animated_sprite = animation_id;
        grab = test_world_to_screen((Position){animation->editor_position.x,
            animation->editor_position.y + EDITOR_VIEWPORT_ROTATION_ARM_LENGTH});
        moved = test_world_to_screen((Position){
            animation->editor_position.x + EDITOR_VIEWPORT_ROTATION_ARM_LENGTH,
            animation->editor_position.y});
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.rotated_animated_sprite);
        assert(viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_DOWN));
        assert(!viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_RELEASED));
        assert(history.undo_count == 1);
        assert(editor_history_undo(&history));
        object = &project.objects[0];
        animation = editor_project_animated_sprite_get(object, animation_id);
        assert(animation != NULL && fabsf(animation->editor_rotation) < 0.001f);
        assert(editor_history_redo(&history));
        object = &project.objects[0];
        animation = editor_project_animated_sprite_get(object, animation_id);
        assert(animation != NULL &&
            fabsf(animation->editor_rotation - 90.0f) < 0.001f);

        body = editor_project_rigid_body_get(object, body->id);
        assert(body != NULL);
        body->rotation = 0.0f;
        animation->editor_rotation = 0.0f;
        animation->rigid_body = body->id;
        animation->follow_body_rotation = false;
        editor_history_reset(&history);
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_ANIMATED_SPRITE;
        assert(editor_viewport_selection_set(&project, &viewport,
            (EditorSelectionRef){EDITOR_SELECTION_RIGID_BODY,
                object->id, 0, 0, body->id}, false));
        assert(editor_viewport_selection_set(&project, &viewport,
            (EditorSelectionRef){EDITOR_SELECTION_ANIMATED_SPRITE,
                object->id, 0, 0, animation->id}, true));
        grab = test_world_to_screen((Position){object->position.x +
            body->position.x + animation->editor_position.x,
            object->position.y + body->position.y + animation->editor_position.y + EDITOR_VIEWPORT_ROTATION_ARM_LENGTH});
        moved = test_world_to_screen((Position){object->position.x +
            body->position.x + animation->editor_position.x +
                EDITOR_VIEWPORT_ROTATION_ARM_LENGTH,
            object->position.y + body->position.y + animation->editor_position.y});
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.rotated_animated_sprite);
        assert(viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_DOWN));
        assert(fabsf(body->rotation - 90.0f) < 0.001f);
        assert(fabsf(animation->editor_rotation - 90.0f) < 0.001f);
        assert(!viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_RELEASED));
        body->rotation = 0.0f;
        animation->editor_rotation = 0.0f;
        editor_viewport_state_init(&viewport);
        viewport.mode = EDITOR_VIEWPORT_RIGID_BODY;
        assert(editor_viewport_selection_set(&project, &viewport,
            (EditorSelectionRef){EDITOR_SELECTION_ANIMATED_SPRITE,
                object->id, 0, 0, animation->id}, false));
        assert(editor_viewport_selection_set(&project, &viewport,
            (EditorSelectionRef){EDITOR_SELECTION_RIGID_BODY,
                object->id, 0, 0, body->id}, true));
        grab = test_world_to_screen((Position){object->position.x + body->position.x,
            object->position.y + body->position.y + EDITOR_VIEWPORT_ROTATION_ARM_LENGTH});
        moved = test_world_to_screen((Position){object->position.x + body->position.x +
            EDITOR_VIEWPORT_ROTATION_ARM_LENGTH,
            object->position.y + body->position.y});
        assert(viewport_pointer_update(&history, &viewport, &project, grab,
            MOUSE_BUTTON_STATE_PRESSED));
        assert(viewport.rotated_body && !viewport.group_rotating);
        assert(viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_DOWN));
        assert(fabsf(body->rotation - 90.0f) < 0.001f);
        assert(fabsf(animation->editor_rotation - 90.0f) < 0.001f);
        assert(!viewport_pointer_update(&history, &viewport, &project, moved,
            MOUSE_BUTTON_STATE_RELEASED));
        assert(editor_project_animated_sprite_remove(object, animation_id));
    }

    command = (EditorCommand){.type = EDITOR_COMMAND_ITEM_REMOVE,
        .data.item_remove = {.kind = EDITOR_ITEM_OBJECT,
            .object = project.objects[0].id}};
    editor_history_command_begin(&history, &project, &command);
    result = editor_command_execute(&project, &command);
    editor_history_command_finish(&history, &command, &result);
    assert(result.kind == ERROR_RESULT_VALUE);
    assert(project.object_count == 0);
    assert(editor_history_undo(&history));
    assert(project.object_count == 1);
    assert(strcmp(project.objects[0].name, "Car") == 0);
    assert(editor_history_redo(&history));
    assert(project.object_count == 0);

    assert(editor_history_undo(&history));
    command = (EditorCommand){.type = EDITOR_COMMAND_ITEM_RENAME,
        .data.item_rename = {.kind = EDITOR_ITEM_OBJECT,
            .object = project.objects[0].id}};
    snprintf(command.data.item_rename.name, sizeof(command.data.item_rename.name),
        "Truck");
    editor_history_command_begin(&history, &project, &command);
    result = editor_command_execute(&project, &command);
    editor_history_command_finish(&history, &command, &result);
    assert(result.kind == ERROR_RESULT_VALUE);
    assert(strcmp(project.objects[0].name, "Truck") == 0);
    assert(!editor_history_redo_check(&history));

    {
        size_t undo_count = history.undo_count;
        command = (EditorCommand){.type = EDITOR_COMMAND_OBJECT_POSITION,
            .data.object_position = {.object = UINT32_MAX,
                .position = {99.0f, 99.0f}}};
        editor_history_command_begin(&history, &project, &command);
        result = editor_command_execute(&project, &command);
        editor_history_command_finish(&history, &command, &result);
        assert(result.kind == ERROR_RESULT_ERROR);
        assert(history.undo_count == undo_count);
    }

    {
        EditorObject *second = editor_project_object_add(&project, (Position){0});
        EditorObjectId first_id = project.objects[0].id;
        EditorObjectId second_id;
        assert(second != NULL);
        second_id = second->id;
        editor_history_reset(&history);
        callback_history = &history;
        editor_command_executing_callback_set(history_begin, NULL);
        editor_command_finished_callback_set(history_finish, NULL);
        assert(editor_history_transaction_begin(&history));
        command = (EditorCommand){.type = EDITOR_COMMAND_OBJECT_POSITION,
            .data.object_position = {.object = first_id,
                .position = {10.0f, 20.0f}}};
        result = editor_command_execute(&project, &command);
        assert(result.kind == ERROR_RESULT_VALUE);
        command.data.object_position.object = second_id;
        command.data.object_position.position = (Position){30.0f, 40.0f};
        result = editor_command_execute(&project, &command);
        assert(result.kind == ERROR_RESULT_VALUE);
        assert(editor_history_transaction_end(&history));
        assert(history.undo_count == 1);
        assert(editor_history_memory_get(&history) < 8192);
        assert(editor_history_undo(&history));
        assert(project.objects[0].position.x == 0.0f);
        assert(project.objects[1].position.x == 0.0f);
        assert(editor_history_redo(&history));
        assert(project.objects[0].position.x == 10.0f);
        assert(project.objects[1].position.x == 30.0f);
        editor_command_executing_callback_set(NULL, NULL);
        editor_command_finished_callback_set(NULL, NULL);
        callback_history = NULL;
    }

    {
        EditorObjectId object_id = project.objects[0].id;
        editor_history_reset(&history);
        project.objects[0].position = (Position){0};
        callback_history = &history;
        editor_command_executing_callback_set(history_begin, NULL);
        editor_command_finished_callback_set(history_finish, NULL);
        assert(editor_history_transaction_begin(&history));
        assert(editor_history_transaction_object_track(&history, object_id));
        editor_history_transaction_commands_suppress_set(&history, true);
        for(int step = 1; step <= 3; step += 1) {
            command = (EditorCommand){.type = EDITOR_COMMAND_OBJECT_POSITION,
                .data.object_position = {.object = object_id,
                    .position = {(float)step * 10.0f, (float)step * 5.0f}}};
            result = editor_command_execute(&project, &command);
            assert(result.kind == ERROR_RESULT_VALUE);
        }
        assert(editor_history_transaction_end(&history));
        assert(history.undo_count == 1);
        assert(project.objects[0].position.x == 30.0f);
        assert(editor_history_undo(&history));
        assert(project.objects[0].position.x == 0.0f);
        assert(editor_history_redo(&history));
        assert(project.objects[0].position.x == 30.0f);
        editor_command_executing_callback_set(NULL, NULL);
        editor_command_finished_callback_set(NULL, NULL);
        callback_history = NULL;
    }

    {
        EditorSpriteId sprite_id;
        editor_history_reset(&history);
        callback_history = &history;
        editor_command_executing_callback_set(history_begin, NULL);
        editor_command_finished_callback_set(history_finish, NULL);
        command = (EditorCommand){.type = EDITOR_COMMAND_SPRITE_ADD,
            .data.sprite_add = {.object = project.objects[0].id,
                .name = "wheel", .path = "assets/wheel.png",
                .size = {32.0f, 32.0f}}};
        result = editor_command_execute(&project, &command);
        assert(result.kind == ERROR_RESULT_VALUE &&
            project.objects[0].sprite_count == 1);
        sprite_id = project.objects[0].sprites[0].id;
        assert(editor_history_undo(&history));
        assert(project.objects[0].sprite_count == 0);
        assert(editor_history_redo(&history));
        assert(project.objects[0].sprite_count == 1 &&
            project.objects[0].sprites[0].id == sprite_id);

        command = (EditorCommand){.type = EDITOR_COMMAND_SPRITE_POSITION_SET,
            .data.sprite_position_set = {.object = project.objects[0].id,
                .sprite = sprite_id, .position = {14.0f, 18.0f}}};
        result = editor_command_execute(&project, &command);
        assert(result.kind == ERROR_RESULT_VALUE &&
            project.objects[0].sprites[0].position.x == 14.0f);
        assert(editor_history_undo(&history));
        assert(project.objects[0].sprites[0].position.x == 0.0f);
        assert(editor_history_redo(&history));
        assert(project.objects[0].sprites[0].position.x == 14.0f);

        command = (EditorCommand){.type = EDITOR_COMMAND_SPRITE_ROTATION_SET,
            .data.sprite_rotation_set = {.object = project.objects[0].id,
                .sprite = sprite_id, .rotation = 0.75f}};
        result = editor_command_execute(&project, &command);
        assert(result.kind == ERROR_RESULT_VALUE &&
            project.objects[0].sprites[0].rotation == 0.75f);
        assert(editor_history_undo(&history));
        assert(project.objects[0].sprites[0].rotation == 0.0f);
        assert(editor_history_redo(&history));
        assert(project.objects[0].sprites[0].rotation == 0.75f);

        command = (EditorCommand){.type = EDITOR_COMMAND_ANIMATED_SPRITE_ADD,
            .data.animated_sprite_add = {.object = project.objects[0].id,
                .name = "rolling"}};
        result = editor_command_execute(&project, &command);
        assert(result.kind == ERROR_RESULT_VALUE &&
            project.objects[0].animated_sprite_count == 1);
        assert(editor_history_undo(&history));
        assert(project.objects[0].animated_sprite_count == 0);
        assert(editor_history_redo(&history));
        assert(project.objects[0].animated_sprite_count == 1);
        command = (EditorCommand){
            .type = EDITOR_COMMAND_ANIMATED_SPRITE_ROTATION_SET,
            .data.animated_sprite_rotation_set = {
                .object = project.objects[0].id,
                .sprite = project.objects[0].animated_sprite_items[0].id,
                .rotation = -0.5f}};
        result = editor_command_execute(&project, &command);
        assert(result.kind == ERROR_RESULT_VALUE &&
            project.objects[0].animated_sprite_items[0].editor_rotation == -0.5f);
        assert(editor_history_undo(&history));
        assert(project.objects[0].animated_sprite_items[0].editor_rotation == 0.0f);
        assert(editor_history_redo(&history));
        assert(project.objects[0].animated_sprite_items[0].editor_rotation == -0.5f);
        editor_command_executing_callback_set(NULL, NULL);
        editor_command_finished_callback_set(NULL, NULL);
        callback_history = NULL;
    }

    {
        EditorInputControllerId controller_id;
        EditorInputActionId action_id;
        size_t hierarchy_count = project.hierarchy_count;
        editor_history_reset(&history);
        callback_history = &history;
        editor_command_executing_callback_set(history_begin, NULL);
        editor_command_finished_callback_set(history_finish, NULL);
        command = (EditorCommand){.type = EDITOR_COMMAND_INPUT_CONTROLLER_ADD,
            .data.input_controller = {.name = "gameplay", .enabled = true}};
        result = editor_command_execute(&project, &command);
        assert(result.kind == ERROR_RESULT_VALUE &&
            project.input_controller_count == 1 &&
            project.hierarchy_count == hierarchy_count + 1);
        controller_id = result.result.object;
        assert(editor_history_undo(&history) &&
            project.input_controller_count == 0 &&
            project.hierarchy_count == hierarchy_count);
        assert(editor_history_redo(&history) &&
            project.input_controller_count == 1 &&
            project.hierarchy_count == hierarchy_count + 1 &&
            project.input_controllers[0].id == controller_id);

        command = (EditorCommand){.type = EDITOR_COMMAND_INPUT_ACTION_ADD,
            .data.input_action = {.controller = controller_id, .name = "jump",
                .type = INPUT_ACTION_BUTTON,
                .button_mode = INPUT_BUTTON_PERSISTENT,
                .button_initial_state = true}};
        result = editor_command_execute(&project, &command);
        assert(result.kind == ERROR_RESULT_VALUE &&
            project.input_controllers[0].action_count == 1 &&
            project.input_controllers[0].actions[0].button_mode ==
                INPUT_BUTTON_PERSISTENT &&
            project.input_controllers[0].actions[0].button_initial_state);
        action_id = result.result.object;
        command = (EditorCommand){.type = EDITOR_COMMAND_INPUT_BINDING_ADD,
            .data.input_binding = {.controller = controller_id, .action = action_id,
                .binding = {.source = INPUT_BINDING_KEY,
                    .input.key = SDL_SCANCODE_SPACE,
                    .scale = {1.0f, 1.0f}}}};
        result = editor_command_execute(&project, &command);
        assert(result.kind == ERROR_RESULT_VALUE &&
            result.result.object != EDITOR_INPUT_BINDING_INVALID &&
            project.input_controllers[0].actions[0].binding_count == 1 &&
            project.input_controllers[0].actions[0].binding_ids[0] ==
                result.result.object &&
            project.input_controllers[0].actions[0].bindings[0].name[0] != '\0');
        EditorInputBindingId binding_id = result.result.object;
        assert(editor_history_undo(&history) &&
            project.input_controllers[0].actions[0].binding_count == 0);
        assert(editor_history_redo(&history) &&
            project.input_controllers[0].actions[0].binding_count == 1 &&
            project.input_controllers[0].actions[0].binding_ids[0] == binding_id &&
            project.input_controllers[0].actions[0].bindings[0].input.key ==
                SDL_SCANCODE_SPACE);
        command = (EditorCommand){.type = EDITOR_COMMAND_INPUT_CONTROLLER_SET,
            .data.input_controller = {.controller = controller_id, .name = "player",
                .enabled = false}};
        result = editor_command_execute(&project, &command);
        assert(result.kind == ERROR_RESULT_VALUE &&
            strcmp(project.input_controllers[0].name, "player") == 0 &&
            !project.input_controllers[0].enabled);
        assert(editor_history_undo(&history) &&
            strcmp(project.input_controllers[0].name, "gameplay") == 0 &&
            project.input_controllers[0].enabled);
        editor_command_executing_callback_set(NULL, NULL);
        editor_command_finished_callback_set(NULL, NULL);
        callback_history = NULL;
    }

    editor_viewport_state_destroy(&viewport);
    editor_history_destroy(&history);
    editor_viewport_assets_destroy();
    (void)SDL_RemovePath("editor_history_frame.png");
    return 0;
}
