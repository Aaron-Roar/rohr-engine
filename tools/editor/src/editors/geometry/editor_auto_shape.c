/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_auto_shape_editor.h"

#include "editors/editor_mode_controls.h"
#include "editor_navigation.h"

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <string.h>

static EditorSoftBody *soft_body_get(EditorObject *object, EditorSoftBodyId id) {
    if(object == NULL) return NULL;
    for(size_t i = 0; i < object->soft_body_count; i += 1)
        if(object->soft_body_items[i].id == id) return &object->soft_body_items[i];
    return NULL;
}

#define EDITOR_AUTO_SHAPE_POINT_MAX \
    (EDITOR_HITBOX_VERTEX_MAX > EDITOR_SOFT_NODE_MAX ? \
        EDITOR_HITBOX_VERTEX_MAX : EDITOR_SOFT_NODE_MAX)

static double shape_cross_get(Position a, Position b, Position c) {
    return ((double)b.x - a.x) * ((double)c.y - a.y) -
        ((double)b.y - a.y) * ((double)c.x - a.x);
}

static double shape_area_get(const Position *points, size_t count) {
    double twice_area = 0;
    for(size_t i = 1; i + 1 < count; i += 1)
        twice_area += shape_cross_get(points[0], points[i], points[i + 1]);
    return fabs(twice_area) * 0.5;
}

static double shape_hull_area_get(const Position *points, size_t count) {
    Position sorted[EDITOR_AUTO_SHAPE_POINT_MAX];
    Position hull[2 * EDITOR_AUTO_SHAPE_POINT_MAX];
    memcpy(sorted, points, count * sizeof(*points));
    for(size_t i = 1; i < count; i += 1) {
        Position point = sorted[i];
        size_t at = i;
        while(at > 0 && (sorted[at - 1].x > point.x ||
                (sorted[at - 1].x == point.x && sorted[at - 1].y > point.y))) {
            sorted[at] = sorted[at - 1];
            at -= 1;
        }
        sorted[at] = point;
    }
    size_t used = 0;
    for(size_t i = 0; i < count; i += 1) {
        while(used >= 2 && shape_cross_get(hull[used - 2], hull[used - 1],
                sorted[i]) <= 0) used -= 1;
        hull[used++] = sorted[i];
    }
    size_t lower = used;
    for(size_t i = count - 1; i > 0; i -= 1) {
        while(used > lower && shape_cross_get(hull[used - 2], hull[used - 1],
                sorted[i - 1]) <= 0) used -= 1;
        hull[used++] = sorted[i - 1];
    }
    return shape_area_get(hull, used - 1);
}

EditorResult editor_auto_shape_size_get(const EditorAutoShapeConfig *config,
        const Position *points, size_t count, bool ordered_polygon,
        EditorAutoShapeConfig *output) {
    Position generated[EDITOR_AUTO_SHAPE_POINT_MAX];
    if(config == NULL || points == NULL || output == NULL ||
            count < 3 || count > EDITOR_AUTO_SHAPE_POINT_MAX)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "auto-shape sizing requires at least three supported points");
    double min_x = points[0].x, max_x = points[0].x;
    double min_y = points[0].y, max_y = points[0].y;
    for(size_t i = 0; i < count; i += 1) {
        if(!isfinite(points[i].x) || !isfinite(points[i].y))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "auto-shape sizing requires finite points");
        min_x = fmin(min_x, points[i].x); max_x = fmax(max_x, points[i].x);
        min_y = fmin(min_y, points[i].y); max_y = fmax(max_y, points[i].y);
    }
    double width = max_x - min_x, height = max_y - min_y;
    double span = fmax(width, height);
    if(span <= 0)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "coincident points have no size to preserve");
    double area = ordered_polygon ? shape_area_get(points, count) :
        shape_hull_area_get(points, count);
    EditorAutoShapeConfig sized = *config;
    /* Work at unit scale before expanding, avoiding overflow for large shapes. */
    sized.width = area > 0 ? (float)(width / span) : 1;
    sized.height = area > 0 ? (float)(height / span) : 1;
    sized.radius = 1;
    sized.apex_offset = config->kind == EDITOR_AUTO_SHAPE_TRIANGLE &&
        config->triangle_kind == EDITOR_AUTO_TRIANGLE_SCALENE &&
        isfinite(config->width) && config->width > 0 ?
        (float)((double)config->apex_offset / config->width * sized.width) : 0;
    EditorResult result = editor_auto_shape_positions_get(&sized, generated, count);
    if(editor_result_check(result)) return result;
    double generated_area = shape_area_get(generated, count);
    if(!(generated_area > 0) || !isfinite(generated_area))
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "auto-shape dimensions cannot represent this shape");
    double scale;
    if(area > 0) scale = sqrt(area / generated_area);
    else {
        /* Give a line a nonzero shape whose largest bound matches its span. */
        min_x = max_x = generated[0].x; min_y = max_y = generated[0].y;
        for(size_t i = 1; i < count; i += 1) {
            min_x = fmin(min_x, generated[i].x); max_x = fmax(max_x, generated[i].x);
            min_y = fmin(min_y, generated[i].y); max_y = fmax(max_y, generated[i].y);
        }
        scale = span / fmax(max_x - min_x, max_y - min_y);
    }
    double values[] = {sized.width * scale, sized.height * scale,
        sized.radius * scale, sized.apex_offset * scale};
    for(size_t i = 0; i < 4; i += 1)
        if(!isfinite(values[i]) || fabs(values[i]) > FLT_MAX ||
                (i < 3 && (float)values[i] <= 0))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "auto-shape dimensions exceed the supported range");
    sized.width = (float)values[0]; sized.height = (float)values[1];
    sized.radius = (float)values[2]; sized.apex_offset = (float)values[3];
    result = editor_auto_shape_positions_get(&sized, generated, count);
    if(editor_result_check(result)) return result;
    for(size_t i = 0; i < count; i += 1)
        if(!isfinite(generated[i].x) || !isfinite(generated[i].y))
            return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "auto-shape vertices exceed the supported range");
    if(shape_area_get(generated, count) <= 0)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "auto-shape dimensions are too small to represent");
    *output = sized;
    return editor_result_value(true);
}

static bool shape_point_selected_check(const EditorViewportState *viewport,
        uint32_t id) {
    if(viewport->auto_shape_point_count == 0) return true;
    for(size_t i = 0; i < viewport->auto_shape_point_count; i += 1)
        if(viewport->auto_shape_points[i] == id) return true;
    return false;
}

bool editor_auto_shape_editor_begin(EditorAutoShapeEditor *editor,
        EditorProject *project, EditorViewportState *viewport,
        EditorViewportMode parent_mode, EditorAutoShapeKind kind) {
    if(editor == NULL || project == NULL || viewport == NULL) return false;
    EditorObject *object = editor_project_selected_get(project);
    if(object == NULL) return false;
    Position points[EDITOR_AUTO_SHAPE_POINT_MAX];
    size_t count = 0;
    bool ordered_polygon = false;
    if(parent_mode == EDITOR_VIEWPORT_HITBOX) {
        EditorRigidBody *body = editor_project_rigid_body_get(object,
            viewport->selected_rigid_body);
        EditorHitbox *box = body == NULL ? NULL : editor_project_hitbox_get(body,
            viewport->selected_hitbox);
        if(box == NULL || box->vertex_count > EDITOR_AUTO_SHAPE_POINT_MAX) return false;
        for(size_t i = 0; i < box->vertex_count; i += 1)
            if(shape_point_selected_check(viewport, box->vertices[i].id))
                points[count++] = box->vertices[i].position;
        ordered_polygon = count == box->vertex_count;
    } else if(parent_mode == EDITOR_VIEWPORT_SOFT_BODY) {
        EditorSoftBody *body = soft_body_get(object, viewport->selected_soft_body);
        if(body == NULL || body->node_count > EDITOR_AUTO_SHAPE_POINT_MAX) return false;
        for(size_t i = 0; i < body->node_count; i += 1)
            if(shape_point_selected_check(viewport, body->nodes[i].id))
                points[count++] = body->nodes[i].position;
    } else return false;
    EditorAutoShapeConfig previous = editor->config, sized = previous;
    sized.kind = kind;
    EditorResult result = editor_auto_shape_size_get(&sized, points, count,
        ordered_polygon, &sized);
    if(editor_result_check(result)) {
        fprintf(stderr, "%s\n", result.result.error.message);
        return false;
    }
    editor->config = sized;
    if(!editor_auto_shape_editor_apply(editor, project, viewport, parent_mode)) {
        editor->config = previous;
        return false;
    }
    if(ordered_polygon) {
        /* Corner controls refer to the converted boundary, not its old notches. */
        EditorRigidBody *body = editor_project_rigid_body_get(object,
            viewport->selected_rigid_body);
        EditorHitbox *box = editor_project_hitbox_get(body, viewport->selected_hitbox);
        (void)editor_auto_shape_hitbox_points_capture(viewport, object, body, box);
    }
    editor->first_was_active = editor->second_was_active = editor->third_was_active = false;
    return true;
}

static void auto_shape_ids_order(uint32_t *ids, Position *points, size_t count) {
    Position centroid = {0};
    for(size_t i = 0; i < count; i += 1) {
        centroid.x += points[i].x;
        centroid.y += points[i].y;
    }
    centroid.x /= (float)count;
    centroid.y /= (float)count;
    for(size_t i = 1; i < count; i += 1) {
        uint32_t id = ids[i];
        Position point = points[i];
        float angle = atan2f(point.y - centroid.y, point.x - centroid.x);
        size_t at = i;
        while(at > 0 && atan2f(points[at - 1].y - centroid.y,
                points[at - 1].x - centroid.x) > angle) {
            ids[at] = ids[at - 1];
            points[at] = points[at - 1];
            at -= 1;
        }
        ids[at] = id;
        points[at] = point;
    }
}

static void icon_line_draw(Position start, Position end, Color color) {
    Vec2D delta = {end.x - start.x, end.y - start.y};
    float length = sqrtf(delta.x * delta.x + delta.y * delta.y);
    if(length <= 0.0f) return;
    rohr_ui_quad(
        (Position){(start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f},
        length, 1.5f, math_radians_to_degrees(atan2f(delta.y, delta.x)), color);
}

static void icon_draw(UIRect bounds, EditorAutoShapeKind kind, Color color) {
    Position center = {bounds.x + bounds.width * 0.5f, bounds.y + 18.0f};
    if(kind == EDITOR_AUTO_SHAPE_TRIANGLE) {
        Position points[] = {{center.x, center.y - 9.0f},
            {center.x + 11.0f, center.y + 8.0f},
            {center.x - 11.0f, center.y + 8.0f}};
        for(size_t i = 0; i < 3; i += 1)
            icon_line_draw(points[i], points[(i + 1) % 3], color);
    } else if(kind == EDITOR_AUTO_SHAPE_RECTANGLE) {
        Position points[] = {{center.x - 11.0f, center.y - 8.0f},
            {center.x + 11.0f, center.y - 8.0f},
            {center.x + 11.0f, center.y + 8.0f},
            {center.x - 11.0f, center.y + 8.0f}};
        for(size_t i = 0; i < 4; i += 1)
            icon_line_draw(points[i], points[(i + 1) % 4], color);
    } else {
        Position previous = {center.x, center.y - 10.0f};
        for(size_t i = 1; i <= 16; i += 1) {
            float angle = -1.57079632679f + 6.28318530718f * (float)i / 16.0f;
            Position current = {center.x + cosf(angle) * 10.0f,
                center.y + sinf(angle) * 10.0f};
            icon_line_draw(previous, current, color);
            previous = current;
        }
    }
}

void editor_auto_shape_picker_frame_begin(EditorAutoShapeEditor *editor,
        float translation_y, UIRect clip) {
    if(editor == NULL) return;
    editor->picker_open = NULL;
    editor->picker_translation_y = translation_y;
    editor->picker_clip = clip;
}

static bool picker_point_inside_check(Position point, UIRect bounds) {
    return bounds.width > 0.0f && bounds.height > 0.0f &&
        point.x >= bounds.x && point.x < bounds.x + bounds.width &&
        point.y >= bounds.y && point.y < bounds.y + bounds.height;
}

void editor_auto_shape_picker_frame_end(EditorAutoShapeEditor *editor,
        Position pointer, MouseButtonState primary, bool escape) {
    if(editor == NULL || editor->picker_open == NULL) return;
    if(escape || (primary == MOUSE_BUTTON_STATE_PRESSED &&
            (!picker_point_inside_check(pointer, editor->picker_clip) ||
                (!picker_point_inside_check(pointer, editor->picker_button_bounds) &&
                 !picker_point_inside_check(pointer, editor->picker_bounds)))))
        *editor->picker_open = false;
    editor->picker_open = NULL;
}

int editor_auto_shape_picker_draw(EditorAutoShapeEditor *editor,
        const char *id_prefix, UIRect button_bounds, UIRect bounds,
        size_t point_count, bool *open) {
    const TextAsset *labels[3];
    float gap = 4.0f;
    float width = (bounds.width - gap * 2.0f) / 3.0f;
    if(editor == NULL || id_prefix == NULL || open == NULL || !*open) return -1;
    editor->picker_open = open;
    editor->picker_button_bounds = button_bounds;
    editor->picker_button_bounds.y += editor->picker_translation_y;
    editor->picker_bounds = bounds;
    editor->picker_bounds.y += editor->picker_translation_y;
    labels[0] = &editor->triangle_label;
    labels[1] = &editor->rectangle_label;
    labels[2] = &editor->circle_label;
    rohr_ui_surface(bounds, (Color){28, 31, 38, 255});
    rohr_ui_border(bounds, 2.0f, (Color){5, 6, 8, 255});
    for(size_t i = 0; i < 3; i += 1) {
        char id[96];
        UIRect button = {bounds.x + (width + gap) * (float)i, bounds.y,
            width, bounds.height};
        bool enabled = point_count >= (i == EDITOR_AUTO_SHAPE_RECTANGLE ? 4u : 3u);
        snprintf(id, sizeof(id), "%s.%zu", id_prefix, i);
        if(enabled) {
            UIButtonResult result = rohr_ui_button(id, labels[i], button, NULL);
            icon_draw(button, (EditorAutoShapeKind)i, (Color){230, 234, 242, 255});
            if(result.clicked) return (int)i;
        } else {
            rohr_ui_button_disabled(button, NULL);
            rohr_ui_label(labels[i], button);
            icon_draw(button, (EditorAutoShapeKind)i, (Color){105, 108, 116, 255});
        }
    }
    return -1;
}

bool editor_auto_shape_editor_create(EditorAutoShapeEditor *editor,
        FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorAutoShapeEditor){.font = font,
        .config = {.kind = EDITOR_AUTO_SHAPE_CIRCLE,
            .triangle_kind = EDITOR_AUTO_TRIANGLE_ISOSCELES,
            .width = 100.0f, .height = 100.0f, .radius = 50.0f}};
#define CREATE(value, member) \
    if(!editor_mode_text_create(font, value, &editor->member)) goto fail
    CREATE("Triangle", triangle_label);
    CREATE("Rectangle", rectangle_label);
    CREATE("Circle", circle_label);
    CREATE("Equilateral", equilateral_label);
    CREATE("Isosceles", isosceles_label);
    CREATE("Scalene", scalene_label);
    CREATE("Width", width_label);
    CREATE("Height", height_label);
    CREATE("Length", length_label);
    CREATE("Radius", radius_label);
    CREATE("Apex X", apex_offset_label);
    CREATE("", first_field);
    CREATE("", second_field);
    CREATE("", third_field);
#undef CREATE
    return true;
fail:
    editor_auto_shape_editor_destroy(editor);
    return false;
}

void editor_auto_shape_editor_destroy(EditorAutoShapeEditor *editor) {
    if(editor == NULL) return;
#define DESTROY(member) rohr_graphics_text_destroy(&editor->member)
    DESTROY(triangle_label); DESTROY(rectangle_label); DESTROY(circle_label);
    DESTROY(equilateral_label); DESTROY(isosceles_label); DESTROY(scalene_label);
    DESTROY(width_label); DESTROY(height_label); DESTROY(length_label);
    DESTROY(radius_label); DESTROY(apex_offset_label);
    DESTROY(first_field); DESTROY(second_field); DESTROY(third_field);
#undef DESTROY
    *editor = (EditorAutoShapeEditor){0};
}

bool editor_auto_shape_editor_apply(EditorAutoShapeEditor *editor,
        EditorProject *project, EditorViewportState *viewport,
        EditorViewportMode parent_mode) {
    EditorObject *object;
    EditorCommand command = {.type = EDITOR_COMMAND_AUTO_SHAPE};
    EditorCommandResult result;
    if(editor == NULL || project == NULL || viewport == NULL) return false;
    object = editor_project_selected_get(project);
    if(object == NULL) return false;
    command.data.auto_shape.object = object->id;
    command.data.auto_shape.config = editor->config;
    if(parent_mode == EDITOR_VIEWPORT_HITBOX) {
        EditorRigidBody *body = editor_project_rigid_body_get(object,
            viewport->selected_rigid_body);
        EditorHitbox *hitbox = body == NULL ? NULL : editor_project_hitbox_get(body,
            viewport->selected_hitbox);
        if(hitbox == NULL) return false;
        command.data.auto_shape.kind = EDITOR_ITEM_HITBOX;
        command.data.auto_shape.parent = body->id;
        command.data.auto_shape.item = hitbox->id;
        if(viewport->auto_shape_point_count < hitbox->vertex_count)
            command.data.auto_shape.point_count = viewport->auto_shape_point_count;
    } else if(parent_mode == EDITOR_VIEWPORT_SOFT_BODY) {
        EditorSoftBody *body = soft_body_get(object, viewport->selected_soft_body);
        if(body == NULL) return false;
        command.data.auto_shape.kind = EDITOR_ITEM_SOFT_BODY;
        command.data.auto_shape.item = body->id;
        if(viewport->auto_shape_point_count < body->node_count)
            command.data.auto_shape.point_count = viewport->auto_shape_point_count;
    } else return false;
    memcpy(command.data.auto_shape.points, viewport->auto_shape_points,
        command.data.auto_shape.point_count * sizeof(*viewport->auto_shape_points));
    result = editor_command_execute(project, &command);
    if(result.kind == ERROR_RESULT_ERROR) {
        fprintf(stderr, "%s\n", result.result.error.message);
        return false;
    }
    return true;
}

bool editor_auto_shape_editor_draw(EditorAutoShapeEditor *editor,
        const EditorModeContext *context) {
    UIFieldResult first = {0}, second = {0}, third = {0};
    bool changed = false;
    bool first_active, second_active, third_active;
    const TextAsset *title;
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    title = editor->config.kind == EDITOR_AUTO_SHAPE_TRIANGLE ?
        &editor->triangle_label : editor->config.kind == EDITOR_AUTO_SHAPE_RECTANGLE ?
        &editor->rectangle_label : &editor->circle_label;
    rohr_ui_label(title, (UIRect){context->x + 10.0f, 44.0f,
        context->width - 20.0f, 30.0f});
    if(editor->config.kind == EDITOR_AUTO_SHAPE_CIRCLE) {
        rohr_ui_label(&editor->radius_label,
            (UIRect){context->x + 10.0f, 88.0f, 80.0f, 28.0f});
        first = editor_mode_field("editor.auto_shape.radius",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT,
                .number = &editor->config.radius}, &editor->first_field,
            (UIRect){context->x + 94.0f, 88.0f,
                context->width - 104.0f, 28.0f}, NULL);
    } else {
        rohr_ui_label(&editor->width_label,
            (UIRect){context->x + 10.0f, 88.0f, 80.0f, 28.0f});
        first = editor_mode_field("editor.auto_shape.width",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &editor->config.width},
            &editor->first_field, (UIRect){context->x + 94.0f, 88.0f,
                context->width - 104.0f, 28.0f}, NULL);
        rohr_ui_label(editor->config.kind == EDITOR_AUTO_SHAPE_RECTANGLE ?
                &editor->length_label : &editor->height_label,
            (UIRect){context->x + 10.0f, 124.0f, 80.0f, 28.0f});
        if(editor->config.kind == EDITOR_AUTO_SHAPE_TRIANGLE &&
                editor->config.triangle_kind == EDITOR_AUTO_TRIANGLE_EQUILATERAL) {
            editor_mode_numeric_disabled_draw(&editor->second_field,
                editor->config.width * sqrtf(3.0f) * 0.5f,
                (UIRect){context->x + 94.0f, 124.0f,
                    context->width - 104.0f, 28.0f});
        } else second = editor_mode_field("editor.auto_shape.height",
            (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &editor->config.height},
            &editor->second_field, (UIRect){context->x + 94.0f, 124.0f,
                context->width - 104.0f, 28.0f}, NULL);
        if(editor->config.kind == EDITOR_AUTO_SHAPE_TRIANGLE) {
            const TextAsset *options[] = {&editor->equilateral_label,
                &editor->isosceles_label, &editor->scalene_label};
            UIDropdownResult result = editor_mode_dropdown("editor.auto_shape.triangle_kind",
                options, 3, (size_t)editor->config.triangle_kind,
                (UIRect){context->x + 10.0f, 164.0f,
                    context->width - 20.0f, 28.0f}, NULL);
            if(result.changed) {
                editor->config.triangle_kind =
                    (EditorAutoTriangleKind)result.selected_index;
                changed = true;
            }
            if(editor->config.triangle_kind == EDITOR_AUTO_TRIANGLE_SCALENE) {
                rohr_ui_label(&editor->apex_offset_label,
                    (UIRect){context->x + 10.0f, 202.0f, 80.0f, 28.0f});
                third = editor_mode_field("editor.auto_shape.apex_offset",
                    (UIFieldBinding){.kind = UI_FIELD_FLOAT,
                        .number = &editor->config.apex_offset}, &editor->third_field,
                    (UIRect){context->x + 94.0f, 202.0f,
                        context->width - 104.0f, 28.0f}, NULL);
            }
        }
    }
    first_active = first.active && !first.submitted;
    second_active = second.active && !second.submitted;
    third_active = third.active && !third.submitted;
    changed = changed || (editor->first_was_active && !first_active) ||
        (editor->second_was_active && !second_active) ||
        (editor->third_was_active && !third_active) || first.submitted ||
        second.submitted || third.submitted;
    if(changed) (void)editor_auto_shape_editor_apply(editor, context->project,
        context->viewport, context->viewport->auto_shape_parent_mode);
    editor->first_was_active = first_active;
    editor->second_was_active = second_active;
    editor->third_was_active = third_active;
    return first_active || second_active || third_active;
}

size_t editor_auto_shape_hitbox_points_capture(EditorViewportState *viewport,
        const EditorObject *object, const EditorRigidBody *body,
        const EditorHitbox *hitbox) {
    if(viewport == NULL || object == NULL || body == NULL || hitbox == NULL) return 0;
    viewport->auto_shape_point_count = 0;
    Position points[EDITOR_HITBOX_VERTEX_MAX];
    for(size_t i = 0; i < hitbox->vertex_count; i += 1)
        if(editor_viewport_selection_contains(viewport,
                (EditorSelectionRef){EDITOR_SELECTION_VERTEX, object->id,
                    body->id, hitbox->id, hitbox->vertices[i].id})) {
            viewport->auto_shape_points[viewport->auto_shape_point_count] =
                hitbox->vertices[i].id;
            points[viewport->auto_shape_point_count++] = hitbox->vertices[i].position;
        }
    if(viewport->auto_shape_point_count == 0)
        for(size_t i = 0; i < hitbox->vertex_count; i += 1) {
            viewport->auto_shape_points[i] = hitbox->vertices[i].id;
            points[i] = hitbox->vertices[i].position;
            viewport->auto_shape_point_count += 1;
        }
    auto_shape_ids_order(viewport->auto_shape_points, points,
        viewport->auto_shape_point_count);
    return viewport->auto_shape_point_count;
}

size_t editor_auto_shape_soft_body_points_capture(EditorViewportState *viewport,
        const EditorObject *object, const EditorSoftBody *body) {
    if(viewport == NULL || object == NULL || body == NULL) return 0;
    viewport->auto_shape_point_count = 0;
    Position points[EDITOR_SOFT_NODE_MAX];
    for(size_t i = 0; i < body->node_count; i += 1)
        if(editor_viewport_selection_contains(viewport,
                (EditorSelectionRef){EDITOR_SELECTION_SOFT_NODE, object->id,
                    body->id, 0, body->nodes[i].id})) {
            viewport->auto_shape_points[viewport->auto_shape_point_count] =
                body->nodes[i].id;
            points[viewport->auto_shape_point_count++] = body->nodes[i].position;
        }
    if(viewport->auto_shape_point_count == 0)
        for(size_t i = 0; i < body->node_count; i += 1) {
            viewport->auto_shape_points[i] = body->nodes[i].id;
            points[i] = body->nodes[i].position;
            viewport->auto_shape_point_count += 1;
        }
    auto_shape_ids_order(viewport->auto_shape_points, points,
        viewport->auto_shape_point_count);
    return viewport->auto_shape_point_count;
}
