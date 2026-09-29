/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editors/geometry/editor_hitbox.h"
#include "editors/soft_body/editor_soft_body.h"
#include "editors/multi/editor_bulk_panel.h"
#include "editor_history.h"
#include <math.h>
#include <stdio.h>

float editor_viewport_width = 1024.0f;
float editor_window_width = 1280.0f;
float editor_window_height = 720.0f;
float editor_viewport_bottom = 720.0f;

#define REQUIRE(condition) do { if(!(condition)) { \
    fprintf(stderr, "auto-shape check failed at line %d: %s\n", __LINE__, #condition); \
    passed = false; goto done; } } while(0)

static int picker_frame(EditorAutoShapeEditor *editor, bool *open,
        float translation, UIRect clip, Position pointer, MouseButtonState primary,
        size_t count, bool escape) {
    rohr_ui_frame_begin((UIInput){pointer, primary});
    rohr_ui_clip_begin(clip);
    rohr_ui_translation_y_push(translation);
    editor_auto_shape_picker_frame_begin(editor, translation, clip);
    int shape = editor_auto_shape_picker_draw(editor, "picker",
        (UIRect){20, 116, 300, 28}, (UIRect){20, 148, 300, 62}, count, open);
    rohr_ui_translation_y_pop();
    rohr_ui_clip_end();
    editor_auto_shape_picker_frame_end(editor, pointer, primary, escape);
    rohr_ui_frame_end();
    return shape;
}

static bool picker_check(FontAsset *font) {
    EditorAutoShapeEditor editor = {0};
    bool passed = true, open = true;
    UIRect clip = {0, 40, 400, 400};
    REQUIRE(editor_auto_shape_editor_create(&editor, font));
    const float offsets[] = {96, 58, -80};
    for(size_t i = 0; i < 3; i += 1) {
        float offset = offsets[i];
        Position pointer = {70, 179 + offset};
        open = true;
        REQUIRE(picker_frame(&editor, &open, offset, clip, pointer,
            MOUSE_BUTTON_STATE_PRESSED, 4, false) == -1 && open);
        REQUIRE(picker_frame(&editor, &open, offset, clip, pointer,
            MOUSE_BUTTON_STATE_RELEASED, 4, false) == EDITOR_AUTO_SHAPE_TRIANGLE && open);
        /* Releasing outside cancels a click; only an outside press dismisses. */
        REQUIRE(picker_frame(&editor, &open, offset, clip, pointer,
            MOUSE_BUTTON_STATE_PRESSED, 4, false) == -1 && open);
        REQUIRE(picker_frame(&editor, &open, offset, clip, (Position){390, 430},
            MOUSE_BUTTON_STATE_RELEASED, 4, false) == -1 && open);
        REQUIRE(picker_frame(&editor, &open, offset, clip, (Position){390, 430},
            MOUSE_BUTTON_STATE_PRESSED, 4, false) == -1 && !open);
        open = true;
        (void)picker_frame(&editor, &open, offset, clip, pointer,
            MOUSE_BUTTON_STATE_UP, 4, true);
        REQUIRE(!open);
    }
    /* Geometry inside the logical picker but outside the visible clip is inactive. */
    open = true;
    REQUIRE(picker_frame(&editor, &open, -120, clip, (Position){70, 30},
        MOUSE_BUTTON_STATE_PRESSED, 4, false) == -1 && !open);
    /* Disabled tiles cannot apply or dismiss the picker. */
    for(size_t count = 0; count < 4; count += 1) {
        for(size_t shape = 0; shape < 3; shape += 1) {
            if(count >= (shape == 1 ? 4u : 3u)) continue;
            Position pointer = {70 + (float)shape * 101, 275};
            open = true;
            REQUIRE(picker_frame(&editor, &open, 96, clip, pointer,
                MOUSE_BUTTON_STATE_PRESSED, count, false) == -1 && open);
            REQUIRE(picker_frame(&editor, &open, 96, clip, pointer,
                MOUSE_BUTTON_STATE_RELEASED, count, false) == -1 && open);
        }
    }
    editor_auto_shape_picker_frame_begin(&editor, 0, clip);
    editor_auto_shape_picker_frame_end(&editor, (Position){-10,-10},
        MOUSE_BUTTON_STATE_PRESSED, true);
    REQUIRE(open && editor.picker_open == NULL);
done:
    editor_auto_shape_editor_destroy(&editor);
    return passed;
}

typedef struct Fixture {
    EditorProject project;
    EditorViewportState state;
    EditorHistory history;
    EditorAutoShapeEditor shape;
    EditorHitboxEditor hitbox;
    EditorSoftBodyEditor soft;
    EditorBulkPanel bulk;
    int kind;
    float translation;
    float scroll;
} Fixture;

static void history_begin(const EditorProject *project,
        const EditorCommand *command, void *context) {
    editor_history_command_begin(context, project, command);
}

static void history_finish(const EditorCommand *command,
        const EditorCommandResult *result, void *context) {
    editor_history_command_finish(context, command, result);
}

static bool *fixture_open_get(Fixture *fixture) {
    return fixture->kind >= 2 ? &fixture->bulk.auto_shape_picker_open :
        fixture->kind == 1 ? &fixture->soft.auto_shape_picker_open :
        &fixture->hitbox.auto_shape_picker_open;
}

static void panel_frame(Fixture *fixture, Position pointer, MouseButtonState primary) {
    UIRect clip = {0, 40, 400, 660};
    rohr_ui_frame_begin((UIInput){pointer, primary});
    float scroll = rohr_ui_scroll_region_begin("panel", clip, 2000,
        fixture->scroll, 42).offset;
    rohr_ui_translation_y_push(fixture->translation);
    editor_auto_shape_picker_frame_begin(&fixture->shape,
        fixture->translation - scroll, clip);
    EditorModeContext context = {.project = &fixture->project,
        .viewport = &fixture->state, .x = 0, .width = 400, .delete_footer = true};
    if(fixture->kind >= 2)
        (void)editor_bulk_panel_draw(&fixture->bulk, &fixture->project,
            &fixture->state, &fixture->history, &fixture->shape,
            0, 400, 650, NULL, NULL);
    else if(fixture->kind == 1)
        (void)editor_soft_body_editor_draw(&fixture->soft, &fixture->shape, &context);
    else
        (void)editor_hitbox_editor_draw(&fixture->hitbox, &fixture->shape, &context);
    rohr_ui_translation_y_pop();
    rohr_ui_scroll_region_end();
    editor_auto_shape_picker_frame_end(&fixture->shape, pointer, primary, false);
    rohr_ui_frame_end();
}

static Position fixture_point_get(Fixture *fixture, size_t index) {
    EditorObject *object = editor_project_selected_get(&fixture->project);
    return fixture->kind % 2 ? object->soft_body_items[0].nodes[index].position :
        object->rigid_bodies[0].hitboxes[0].vertices[index].position;
}

static bool position_equal(Position a, Position b) {
    return fabsf(a.x - b.x) < 0.001f && fabsf(a.y - b.y) < 0.001f;
}

static bool panel_check(FontAsset *font, int kind, bool scrolled, bool invalid) {
    Fixture fixture = {.kind = kind, .translation = kind == 0 ? 96 : 58,
        .scroll = scrolled ? 70 : 0};
    bool passed = true;
    Position before[6], after[6];
    editor_project_init(&fixture.project);
    REQUIRE(editor_history_init(&fixture.history, &fixture.project));
    REQUIRE(editor_auto_shape_editor_create(&fixture.shape, font));
    REQUIRE(editor_hitbox_editor_create(&fixture.hitbox, font));
    REQUIRE(editor_soft_body_editor_create(&fixture.soft, font));
    REQUIRE(editor_bulk_panel_create(&fixture.bulk, font));
    EditorObject *object = editor_project_object_add(&fixture.project, (Position){0});
    REQUIRE(object != NULL);
    EditorRigidBody *body = editor_project_rigid_body_add(&fixture.project, object);
    EditorSoftBody *soft = editor_project_soft_body_add(&fixture.project, object);
    REQUIRE(body != NULL && soft != NULL);
    EditorHitbox *hitbox = &body->hitboxes[0];
    while(hitbox->vertex_count < 6)
        REQUIRE(editor_project_hitbox_vertex_insert(&fixture.project, hitbox, 0));
    for(size_t i = 0; i < 6; i += 1) {
        float angle = (float)i * 6.28318530718f / 6.0f;
        before[i] = (Position){150 + cosf(angle) * 20, 100 + sinf(angle) * 20};
        hitbox->vertices[i].position = before[i];
        REQUIRE(editor_project_soft_node_add(&fixture.project, soft, before[i]));
    }
    fixture.state = (EditorViewportState){
        .mode = kind % 2 ? EDITOR_VIEWPORT_SOFT_BODY : EDITOR_VIEWPORT_HITBOX,
        .selection = kind % 2 ? EDITOR_SELECTION_SOFT_BODY : EDITOR_SELECTION_HITBOX,
        .selected_rigid_body = body->id, .selected_hitbox = hitbox->id,
        .selected_soft_body = soft->id};
    if(kind >= 2) for(size_t i = 0; i < 3; i += 1) {
        EditorSelectionRef ref = kind % 2 ?
            (EditorSelectionRef){EDITOR_SELECTION_SOFT_NODE, object->id, soft->id,
                0, soft->nodes[i].id} :
            (EditorSelectionRef){EDITOR_SELECTION_VERTEX, object->id, body->id,
                hitbox->id, hitbox->vertices[i].id};
        REQUIRE(editor_viewport_selection_set(&fixture.project, &fixture.state,
            ref, i > 0));
    }
    fixture.soft.transform_section.expanded = scrolled;
    fixture.soft.appearance_section.expanded = false;
    fixture.soft.initial_motion_section.expanded = false;
    fixture.soft.topology_section.expanded = true;
    editor_command_executing_callback_set(history_begin, &fixture.history);
    editor_command_finished_callback_set(history_finish, &fixture.history);
    bool *open = fixture_open_get(&fixture);
    *open = true;
    panel_frame(&fixture, (Position){0}, MOUSE_BUTTON_STATE_UP);
    REQUIRE(fixture.shape.picker_bounds.width > 0);
    UIRect button = fixture.shape.picker_button_bounds;
    Position button_center = {button.x + button.width * 0.5f,
        button.y + button.height * 0.5f};
    *open = false;
    panel_frame(&fixture, button_center, MOUSE_BUTTON_STATE_PRESSED);
    panel_frame(&fixture, button_center, MOUSE_BUTTON_STATE_RELEASED);
    REQUIRE(*open);
    UIRect bounds = fixture.shape.picker_bounds;
    Position circle = {bounds.x + bounds.width * 5.0f / 6.0f,
        bounds.y + bounds.height * 0.5f};
    REQUIRE(circle.y > 40 && circle.y < 700);
    if(invalid) fixture.shape.config.radius = -1;
    panel_frame(&fixture, circle, MOUSE_BUTTON_STATE_PRESSED);
    REQUIRE(*open && fixture.state.mode != EDITOR_VIEWPORT_AUTO_SHAPE);
    panel_frame(&fixture, circle, MOUSE_BUTTON_STATE_RELEASED);
    if(invalid) {
        REQUIRE(*open && fixture.state.mode != EDITOR_VIEWPORT_AUTO_SHAPE);
        REQUIRE(fixture.history.undo_count == 0);
        for(size_t i = 0; i < 6; i += 1)
            REQUIRE(position_equal(before[i], fixture_point_get(&fixture, i)));
        goto done;
    }
    REQUIRE(!*open && fixture.state.mode == EDITOR_VIEWPORT_AUTO_SHAPE);
    REQUIRE(fixture.history.undo_count == 1);
    for(size_t i = 0; i < 6; i += 1) {
        after[i] = fixture_point_get(&fixture, i);
        if(kind < 2 || i < 3)
            REQUIRE(fabsf(hypotf(after[i].x, after[i].y) - 50) < 0.001f);
        else REQUIRE(position_equal(before[i], after[i]));
    }
    REQUIRE(editor_history_undo(&fixture.history));
    for(size_t i = 0; i < 6; i += 1)
        REQUIRE(position_equal(before[i], fixture_point_get(&fixture, i)));
    REQUIRE(editor_history_redo(&fixture.history));
    for(size_t i = 0; i < 6; i += 1)
        REQUIRE(position_equal(after[i], fixture_point_get(&fixture, i)));
done:
    editor_command_executing_callback_set(NULL, NULL);
    editor_command_finished_callback_set(NULL, NULL);
    rohr_ui_field_focus_clear();
    editor_bulk_panel_destroy(&fixture.bulk);
    editor_soft_body_editor_destroy(&fixture.soft);
    editor_hitbox_editor_destroy(&fixture.hitbox);
    editor_auto_shape_editor_destroy(&fixture.shape);
    editor_history_destroy(&fixture.history);
    editor_viewport_state_destroy(&fixture.state);
    editor_project_destroy(&fixture.project);
    return passed;
}

int main(void) {
    if(rohr_error_check(rohr_engine_start())) return 1;
    if(rohr_error_check(rohr_graphics_start())) { rohr_engine_stop(); return 1; }
    FontAsset font = rohr_graphics_font_default_get();
    bool passed = picker_check(&font);
    for(int kind = 0; kind < 4 && passed; kind += 1) {
        passed = panel_check(&font, kind, false, false) &&
            panel_check(&font, kind, true, false);
        if(kind < 2 && passed) passed = panel_check(&font, kind, false, true);
        if(!passed) fprintf(stderr, "auto-shape panel kind %d failed\n", kind);
    }
    rohr_graphics_stop();
    rohr_engine_stop();
    return passed ? 0 : 1;
}
