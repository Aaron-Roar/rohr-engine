/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editors/geometry/editor_hitbox.h"
#include "editors/soft_body/editor_soft_body.h"
#include "editors/multi/editor_bulk_panel.h"
#include "editor_history.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <string.h>

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

static double polygon_area_get(const Position *points, size_t count) {
    double area = 0;
    for(size_t i = 0; i < count; i += 1) {
        Position a = points[i], b = points[(i + 1) % count];
        area += (double)a.x * b.y - (double)b.x * a.y;
    }
    return fabs(area) * 0.5;
}

static bool area_near(double actual, double expected) {
    return fabs(actual - expected) <= expected * 0.00002;
}

static bool sizing_check(void) {
    bool passed = true;
    /* Concave L: boundary area 7, convex hull area 11.5. */
    Position concave[] = {{0,0}, {4,0}, {4,1}, {1,1}, {1,4}, {0,4}};
    Position generated[6];
    EditorAutoShapeConfig config = {.kind = EDITOR_AUTO_SHAPE_CIRCLE,
        .triangle_kind = EDITOR_AUTO_TRIANGLE_ISOSCELES,
        .width = 100, .height = 100, .radius = 50};
    EditorAutoShapeConfig sized;
    REQUIRE(!editor_result_check(editor_auto_shape_size_get(&config,
        concave, 6, true, &sized)));
    REQUIRE(!editor_result_check(editor_auto_shape_positions_get(&sized, generated, 6)));
    REQUIRE(area_near(polygon_area_get(generated, 6), 7));
    REQUIRE(!editor_result_check(editor_auto_shape_size_get(&config,
        concave, 6, false, &sized)));
    REQUIRE(!editor_result_check(editor_auto_shape_positions_get(&sized, generated, 6)));
    REQUIRE(area_near(polygon_area_get(generated, 6), 11.5));
    Position rectangle[] = {{-400,-100}, {400,-100}, {400,100}, {-400,100}};
    for(int kind = 0; kind < 3; kind += 1) {
        config.kind = (EditorAutoShapeKind)kind;
        for(int triangle = 0; triangle < 3; triangle += 1) {
            config.triangle_kind = (EditorAutoTriangleKind)triangle;
            config.apex_offset = 20;
            REQUIRE(!editor_result_check(editor_auto_shape_size_get(&config,
                rectangle, 4, true, &sized)));
            REQUIRE(!editor_result_check(editor_auto_shape_positions_get(&sized, generated, 4)));
            REQUIRE(area_near(polygon_area_get(generated, 4), 160000));
            if(kind != EDITOR_AUTO_SHAPE_CIRCLE &&
                    triangle != EDITOR_AUTO_TRIANGLE_EQUILATERAL)
                REQUIRE(fabsf(sized.width / sized.height - 4) < 0.0001f);
        }
    }
    /* Repeated conversions use polygon area, including a four-vertex circle. */
    memcpy(generated, rectangle, sizeof(rectangle));
    for(int i = 0; i < 150; i += 1) {
        config.kind = (EditorAutoShapeKind)(i % 3);
        config.triangle_kind = EDITOR_AUTO_TRIANGLE_ISOSCELES;
        REQUIRE(!editor_result_check(editor_auto_shape_size_get(&config,
            generated, 4, true, &sized)));
        REQUIRE(!editor_result_check(editor_auto_shape_positions_get(&sized, generated, 4)));
        REQUIRE(area_near(polygon_area_get(generated, 4), 160000));
    }
    /* Unordered duplicate/interior points must not affect hull area. */
    Position unordered[] = {{4,1}, {0,4}, {0,0}, {4,0}, {1,1}, {0,0}};
    REQUIRE(!editor_result_check(editor_auto_shape_size_get(&config,
        unordered, 6, false, &sized)));
    REQUIRE(!editor_result_check(editor_auto_shape_positions_get(&sized, generated, 6)));
    REQUIRE(area_near(polygon_area_get(generated, 6), 10));
    Position line[] = {{0,0}, {100,0}, {300,0}, {300,0}};
    for(int kind = 0; kind < 3; kind += 1) {
        config.kind = (EditorAutoShapeKind)kind;
        REQUIRE(!editor_result_check(editor_auto_shape_size_get(&config, line, 4, false, &sized)));
        REQUIRE(!editor_result_check(editor_auto_shape_positions_get(&sized, generated, 4)));
        REQUIRE(polygon_area_get(generated, 4) > 0);
        float min_x = generated[0].x, max_x = min_x;
        float min_y = generated[0].y, max_y = min_y;
        for(size_t i = 1; i < 4; i += 1) {
            min_x = fminf(min_x, generated[i].x); max_x = fmaxf(max_x, generated[i].x);
            min_y = fminf(min_y, generated[i].y); max_y = fmaxf(max_y, generated[i].y);
        }
        REQUIRE(fabsf(fmaxf(max_x - min_x, max_y - min_y) - 300) < 0.001f);
    }
    /* Reverse winding, translate, and test well above/below default dimensions. */
    const float scales[] = {0.00001f, 1, 1000000};
    for(size_t scale = 0; scale < 3; scale += 1) {
        Position points[4];
        for(size_t i = 0; i < 4; i += 1)
            points[i] = (Position){(rectangle[3-i].x + 2000) * scales[scale],
                (rectangle[3-i].y - 3000) * scales[scale]};
        REQUIRE(!editor_result_check(editor_auto_shape_size_get(&config, points, 4, true, &sized)));
        REQUIRE(!editor_result_check(editor_auto_shape_positions_get(&sized, generated, 4)));
        REQUIRE(area_near(polygon_area_get(generated, 4), polygon_area_get(points, 4)));
    }
    EditorAutoShapeConfig saved = sized;
    Position invalid[] = {{0,0}, {0,0}, {0,0}, {0,0}};
    REQUIRE(editor_result_check(editor_auto_shape_size_get(&config, invalid, 4, true, &sized)));
    REQUIRE(memcmp(&saved, &sized, sizeof(saved)) == 0);
    invalid[1].x = NAN;
    REQUIRE(editor_result_check(editor_auto_shape_size_get(&config, invalid, 4, false, &sized)));
    REQUIRE(memcmp(&saved, &sized, sizeof(saved)) == 0);
    invalid[0] = (Position){-FLT_MAX, -FLT_MAX};
    invalid[1] = (Position){FLT_MAX, -FLT_MAX};
    invalid[2] = (Position){FLT_MAX, FLT_MAX};
    invalid[3] = (Position){-FLT_MAX, FLT_MAX};
    REQUIRE(editor_result_check(editor_auto_shape_size_get(&config, invalid, 4, false, &sized)));
    REQUIRE(memcmp(&saved, &sized, sizeof(saved)) == 0);
done:
    return passed;
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
    body->position = (Position){37, -21};
    body->rotation = 123;
    body->center_of_mass_explicit = true;
    body->center_of_mass_offset = (Position){8, 12};
    soft->position = (Position){-9, 18};
    soft->rotation = 321;
    while(hitbox->vertex_count < 6)
        REQUIRE(editor_project_hitbox_vertex_insert(&fixture.project, hitbox, 0));
    for(size_t i = 0; i < 6; i += 1) {
        float angle = (float)i * 6.28318530718f / 6.0f;
        if(kind == 0 && !scrolled) angle = -angle;
        before[i] = invalid ? (Position){0} :
            (Position){1500 + cosf(angle) * 200, 1000 + sinf(angle) * 200};
        if(kind == 0 && scrolled) {
            const Position concave[] = {{0,0}, {400,0}, {400,100},
                {100,100}, {100,400}, {0,400}};
            before[i] = concave[i];
        }
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
    EditorAutoShapeConfig previous_config = fixture.shape.config;
    panel_frame(&fixture, circle, MOUSE_BUTTON_STATE_PRESSED);
    REQUIRE(*open && fixture.state.mode != EDITOR_VIEWPORT_AUTO_SHAPE);
    panel_frame(&fixture, circle, MOUSE_BUTTON_STATE_RELEASED);
    if(invalid) {
        REQUIRE(*open && fixture.state.mode != EDITOR_VIEWPORT_AUTO_SHAPE);
        REQUIRE(fixture.history.undo_count == 0);
        REQUIRE(memcmp(&fixture.shape.config, &previous_config, sizeof(previous_config)) == 0);
        for(size_t i = 0; i < 6; i += 1)
            REQUIRE(position_equal(before[i], fixture_point_get(&fixture, i)));
        goto done;
    }
    REQUIRE(!*open && fixture.state.mode == EDITOR_VIEWPORT_AUTO_SHAPE);
    REQUIRE(fixture.history.undo_count == 1);
    for(size_t i = 0; i < 6; i += 1) {
        after[i] = fixture_point_get(&fixture, i);
        if(kind >= 2 && i >= 3) REQUIRE(position_equal(before[i], after[i]));
    }
    size_t changed_count = kind < 2 ? 6 : 3;
    REQUIRE(area_near(polygon_area_get(after, changed_count),
        polygon_area_get(before, changed_count)));
    REQUIRE(position_equal(body->position, (Position){37, -21}) && body->rotation == 123);
    REQUIRE(body->center_of_mass_explicit &&
        position_equal(body->center_of_mass_offset, (Position){8, 12}));
    REQUIRE(position_equal(soft->position, (Position){-9, 18}) && soft->rotation == 321);
    REQUIRE(editor_history_undo(&fixture.history));
    for(size_t i = 0; i < 6; i += 1)
        REQUIRE(position_equal(before[i], fixture_point_get(&fixture, i)));
    REQUIRE(editor_history_redo(&fixture.history));
    for(size_t i = 0; i < 6; i += 1)
        REQUIRE(position_equal(after[i], fixture_point_get(&fixture, i)));
    /* Explicit edits resize normally; only choosing a shape performs sizing. */
    fixture.shape.config.radius *= 2;
    REQUIRE(editor_auto_shape_editor_apply(&fixture.shape, &fixture.project,
        &fixture.state, fixture.state.auto_shape_parent_mode));
    for(size_t i = 0; i < 6; i += 1) after[i] = fixture_point_get(&fixture, i);
    double target_area = polygon_area_get(before, changed_count) * 4;
    REQUIRE(area_near(polygon_area_get(after, changed_count), target_area));
    for(size_t iteration = 0; iteration < 30; iteration += 1) {
        EditorAutoShapeKind shape = iteration % 2 ? EDITOR_AUTO_SHAPE_CIRCLE :
            EDITOR_AUTO_SHAPE_TRIANGLE;
        REQUIRE(editor_auto_shape_editor_begin(&fixture.shape, &fixture.project,
            &fixture.state, fixture.state.auto_shape_parent_mode, shape));
        for(size_t i = 0; i < 6; i += 1) after[i] = fixture_point_get(&fixture, i);
        REQUIRE(area_near(polygon_area_get(after, changed_count), target_area));
        if(kind >= 2) for(size_t i = 3; i < 6; i += 1)
            REQUIRE(position_equal(before[i], after[i]));
    }
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
    bool passed = sizing_check() && picker_check(&font);
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
