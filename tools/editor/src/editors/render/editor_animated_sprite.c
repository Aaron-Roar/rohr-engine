/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_animated_sprite.h"

#include "editors/editor_mode_controls.h"

#include <stdio.h>

bool editor_animated_sprite_editor_create(EditorAnimatedSpriteEditor *editor,
        FontAsset *font) {
    if(editor == NULL || font == NULL) return false;
    *editor = (EditorAnimatedSpriteEditor){.font = font};
#define CREATE(value, member) \
    if(!editor_mode_text_create(font, value, &editor->member)) goto fail
    CREATE("Name", name_label); CREATE("Rigid Body", body_label);
    CREATE("X", x_label); CREATE("Y", y_label); CREATE("Rotation", rotation_label);
    CREATE("Scale X", scale_x_label); CREATE("Scale Y", scale_y_label);
    CREATE("Ticks / Frame", ticks_label); CREATE("Time / Frame", time_label);
    CREATE("Starting Frame", starting_label); CREATE("Direction", direction_label);
    CREATE("Left", left_label); CREATE("Right", right_label);
    CREATE("Follow Rotation", follow_label); CREATE("Playing", playing_label);
    CREATE("Add Frame", add_frame_label); CREATE("Visibility", visibility_label);
    CREATE("[X]", visible_label); CREATE("[ ]", hidden_label);
    CREATE("None", none_label);
    CREATE("Delete Animation", delete_label); CREATE("", x_field);
    CREATE("", y_field); CREATE("", rotation_field); CREATE("", scale_x_field);
    CREATE("", scale_y_field); CREATE("", ticks_field); CREATE("", time_field);
    CREATE("", starting_field);
#undef CREATE
    if(!editor_mode_accordion_section_create(&editor->transform_section, font,
            "Transform", true) ||
            !editor_mode_accordion_section_create(&editor->attachment_section, font,
                "Attachment", false) ||
            !editor_mode_accordion_section_create(&editor->playback_section, font,
                "Playback", false) ||
            !editor_mode_accordion_section_create(&editor->frames_section, font,
                "Frames", false)) goto fail;
    return true;
fail:
    editor_animated_sprite_editor_destroy(editor);
    return false;
}

void editor_animated_sprite_editor_destroy(EditorAnimatedSpriteEditor *editor) {
    if(editor == NULL) return;
#define DESTROY(member) rohr_graphics_text_destroy(&editor->member)
    DESTROY(name_label); DESTROY(body_label); DESTROY(x_label); DESTROY(y_label);
    DESTROY(rotation_label); DESTROY(scale_x_label); DESTROY(scale_y_label);
    DESTROY(ticks_label); DESTROY(time_label); DESTROY(starting_label);
    DESTROY(direction_label); DESTROY(left_label); DESTROY(right_label);
    DESTROY(follow_label); DESTROY(playing_label); DESTROY(add_frame_label);
    DESTROY(visibility_label); DESTROY(visible_label); DESTROY(hidden_label);
    DESTROY(none_label);
    DESTROY(delete_label); DESTROY(x_field); DESTROY(y_field);
    DESTROY(rotation_field); DESTROY(scale_x_field); DESTROY(scale_y_field);
    DESTROY(ticks_field); DESTROY(time_field); DESTROY(starting_field);
#undef DESTROY
    editor_mode_accordion_section_destroy(&editor->transform_section);
    editor_mode_accordion_section_destroy(&editor->attachment_section);
    editor_mode_accordion_section_destroy(&editor->playback_section);
    editor_mode_accordion_section_destroy(&editor->frames_section);
    for(size_t i = 0; i < 32; i += 1)
        rohr_graphics_text_destroy(&editor->name_values[i]);
    for(size_t i = 0; i < EDITOR_RIGID_BODY_MAX; i += 1)
        rohr_graphics_text_destroy(&editor->body_names[i]);
    for(size_t i = 0; i < 64; i += 1)
        rohr_graphics_text_destroy(&editor->frame_names[i]);
    *editor = (EditorAnimatedSpriteEditor){0};
}

static UIFieldResult float_field(const char *id, const TextAsset *label,
        TextAsset *field, float *value, float x, float y, float width,
        float label_width) {
    rohr_ui_label(label, (UIRect){x + 8.0f, y, label_width, 28.0f});
    UIButtonStyle style = editor_mode_section_field_style_get();
    return editor_mode_field(id,
        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = value}, field,
        (UIRect){x + label_width + 10.0f, y,
            width - label_width - 20.0f, 28.0f}, &style);
}

bool editor_animated_sprite_editor_draw(EditorAnimatedSpriteEditor *editor,
        const EditorModeContext *context, EditorBodyPreviewFunction preview,
        void *preview_context, EditorAnimationFrameBrowserOpenFunction browser_open,
        void *browser_context, bool additive_selection,
        bool *frame_multi_edit_open) {
    EditorObject *object;
    EditorAnimatedSprite *sprite;
    size_t index, body_selected = 0;
    char name[EDITOR_OBJECT_NAME_MAX];
    Position position;
    float rotation, scale_x, scale_y, ticks, seconds, starting;
    bool visible, follow, playing;
    const TextAsset *body_options[EDITOR_RIGID_BODY_MAX + 1];
    const TextAsset *direction_options[2] = {&editor->left_label, &editor->right_label};
    UIFieldResult name_result, x_result = {0}, y_result = {0}, rotation_result = {0};
    UIFieldResult sx_result = {0}, sy_result = {0}, ticks_result = {0},
        time_result = {0}, start_result = {0};
    UIDropdownResult body_result = {0}, direction_result = {0};
    bool follow_changed = false, playing_changed = false, layer_active = false;
    bool transform_open, attachment_open, playback_open, frames_open;
    float attachment_y;
    float transform_row_y[7] = {0}, playback_row_y[5] = {0};
    float add_frame_y = 0.0f, frame_list_y = 0.0f, frame_row_stride = 0.0f;
    float section_y = 80.0f;
    if(editor == NULL || context == NULL || context->project == NULL ||
            context->viewport == NULL) return false;
    object = editor_project_selected_get(context->project);
    sprite = editor_project_animated_sprite_get(object,
        context->viewport->selected_animated_sprite);
    if(sprite == NULL || object == NULL) return false;
    index = (size_t)(sprite - object->animated_sprite_items);
    if(index >= 32) return false;
    snprintf(name, sizeof(name), "%s", sprite->name);
    if(!editor_mode_named_text_sync(editor->font, sprite->name,
            &editor->name_values[index], editor->name_cache[index],
            EDITOR_OBJECT_NAME_MAX)) return false;
    visible = sprite->visible;
    bool visible_changed = false;
    rohr_ui_label(&editor->name_label,
        (UIRect){context->x + 8.0f, 42.0f, 74.0f, 28.0f});
    name_result = editor_mode_field("editor.animated_sprite.name",
        (UIFieldBinding){.kind = UI_FIELD_STRING, .string = name,
            .string_capacity = sizeof(name)}, &editor->name_values[index],
        (UIRect){context->x + 88.0f, 42.0f, context->width - 96.0f, 28.0f}, NULL);
    visible_changed = editor_mode_visibility_field(
        "editor.animated_sprite.visible", &editor->visibility_label,
        &editor->visible_label, &editor->hidden_label,
        (UIRect){context->x + 10.0f, 80.0f,
            context->width - 20.0f, 28.0f}, &visible);
    section_y = 118.0f;
    {
        float layer_height = sprite->graphics_layer.layer == 0 ||
            (context->layer_control != NULL &&
                (context->layer_control->adding ||
                    context->layer_control->edited_layer != 0)) ? 86.0f : 48.0f;
        const float transform_rows[] = {
            28.0f, 28.0f, 28.0f, 28.0f, layer_height, 28.0f, 28.0f};
        const float attachment_rows[] = {28.0f};
        const float playback_rows[] = {
            28.0f, 28.0f, 28.0f, 28.0f, 28.0f};
        const EditorModeAccordionLayoutGroup frame_groups[] = {
            {.row_count = 1, .row_height = 28.0f},
            {.row_count = sprite->frame_count, .row_height = 26.0f,
                .row_gap = 4.0f, .gap_before = 8.0f}};
        EditorModeAccordionLayoutCursor accordion =
            editor_mode_accordion_layout_cursor_get(
                context->x, context->width, section_y);
        EditorModeAccordionLayoutResult transform =
            editor_mode_accordion_layout_section(&accordion,
                &editor->transform_section,
                "editor.animated_sprite.section.transform",
                transform_rows, 7, 10.0f);
        EditorModeAccordionLayoutResult attachment =
            editor_mode_accordion_layout_section(&accordion,
                &editor->attachment_section,
                "editor.animated_sprite.section.attachment",
                attachment_rows, 1, 10.0f);
        EditorModeAccordionLayoutResult playback =
            editor_mode_accordion_layout_section(&accordion,
                &editor->playback_section,
                "editor.animated_sprite.section.playback",
                playback_rows, 5, 10.0f);
        EditorModeAccordionLayoutResult frames =
            editor_mode_accordion_layout_nested_section(&accordion,
                &editor->frames_section,
                "editor.animated_sprite.section.frames",
                frame_groups, 2);
        transform_open = transform.expanded;
        attachment_open = attachment.expanded;
        attachment_y = attachment.content_y;
        playback_open = playback.expanded;
        frames_open = frames.expanded;
        for(size_t row = 0; row < 7; row += 1)
            transform_row_y[row] = editor_mode_accordion_layout_row_y(
                &transform, transform_rows, row, 10.0f);
        for(size_t row = 0; row < 5; row += 1)
            playback_row_y[row] = editor_mode_accordion_layout_row_y(
                &playback, playback_rows, row, 10.0f);
        add_frame_y = editor_mode_accordion_layout_group_row_y(
            &frames, frame_groups, 2, 0, 0);
        if(sprite->frame_count > 0)
            frame_list_y = editor_mode_accordion_layout_group_row_y(
                &frames, frame_groups, 2, 1, 0);
        frame_row_stride = frame_groups[1].row_height +
            frame_groups[1].row_gap;
    }
    body_options[0] = &editor->none_label;
    for(size_t i = 0; i < object->rigid_body_count &&
            i < EDITOR_RIGID_BODY_MAX; i += 1) {
        if(!editor_mode_named_text_sync(editor->font, object->rigid_bodies[i].name,
                &editor->body_names[i], editor->body_cache[i],
                EDITOR_OBJECT_NAME_MAX)) return false;
        body_options[i + 1] = &editor->body_names[i];
        if(object->rigid_bodies[i].id == sprite->rigid_body) body_selected = i + 1;
    }
    body_result.selected_index = body_selected;
    if(attachment_open) {
        UIButtonStyle style = editor_mode_section_field_style_get();
        rohr_ui_label(&editor->body_label,
            (UIRect){context->x + 8.0f, attachment_y, 90.0f, 28.0f});
        body_result = editor_mode_dropdown("editor.animated_sprite.body", body_options,
            object->rigid_body_count + 1, body_selected,
            (UIRect){context->x + 100.0f, attachment_y,
                context->width - 110.0f, 28.0f}, &style);
        if(preview != NULL)
            preview(preview_context, object, body_result, sprite->rigid_body);
    }
    position = sprite->editor_position; rotation = sprite->editor_rotation;
    scale_x = sprite->scale.x; scale_y = sprite->scale.y;
    ticks = (float)sprite->ticks_per_frame; seconds = (float)sprite->time_per_frame;
    starting = (float)sprite->starting_frame;
    {
        EditorRigidBody *attached = editor_project_rigid_body_get(object,
            sprite->rigid_body);
        follow = sprite->follow_body_rotation &&
            (attached == NULL || !attached->particle);
    }
    playing = sprite->playing;
    if(transform_open) {
        x_result = float_field("editor.animated_sprite.x", &editor->x_label,
            &editor->x_field, &position.x, context->x, transform_row_y[0],
            context->width, 90.0f);
        y_result = float_field("editor.animated_sprite.y", &editor->y_label,
            &editor->y_field, &position.y, context->x, transform_row_y[1],
            context->width, 90.0f);
        rotation_result = float_field("editor.animated_sprite.rotation",
            &editor->rotation_label, &editor->rotation_field, &rotation,
            context->x, transform_row_y[2], context->width, 90.0f);
        follow_changed = editor_mode_checkbox_left("editor.animated_sprite.follow",
            &editor->follow_label, (UIRect){context->x + 10.0f,
                transform_row_y[3], context->width - 20.0f, 28.0f}, &follow);
        if(context->layer_control != NULL)
            layer_active = editor_mode_layer_control_draw(context->layer_control,
                "editor.animated_sprite", context->project,
                &sprite->graphics_layer, NULL, context->x,
                transform_row_y[4], context->width);
        sx_result = float_field("editor.animated_sprite.scale_x",
            &editor->scale_x_label, &editor->scale_x_field, &scale_x,
            context->x, transform_row_y[5],
            context->width, 90.0f);
        sy_result = float_field("editor.animated_sprite.scale_y",
            &editor->scale_y_label, &editor->scale_y_field, &scale_y,
            context->x, transform_row_y[6],
            context->width, 90.0f);
    }
    if(playback_open) {
        UIButtonStyle style = editor_mode_section_field_style_get();
        ticks_result = float_field("editor.animated_sprite.ticks",
            &editor->ticks_label, &editor->ticks_field, &ticks, context->x,
            playback_row_y[0], context->width, 110.0f);
        time_result = float_field("editor.animated_sprite.seconds",
            &editor->time_label, &editor->time_field, &seconds, context->x,
            playback_row_y[1], context->width, 110.0f);
        start_result = float_field("editor.animated_sprite.start",
            &editor->starting_label, &editor->starting_field, &starting,
            context->x, playback_row_y[2], context->width, 110.0f);
        rohr_ui_label(&editor->direction_label,
            (UIRect){context->x + 8.0f, playback_row_y[3], 90.0f, 28.0f});
        direction_result = editor_mode_dropdown("editor.animated_sprite.direction",
            direction_options, 2, sprite->direction == DIRECTION_LEFT ? 0 : 1,
            (UIRect){context->x + 100.0f, playback_row_y[3],
                context->width - 110.0f, 28.0f}, &style);
        playing_changed = editor_mode_checkbox_left(
            "editor.animated_sprite.playing", &editor->playing_label,
            (UIRect){context->x + 10.0f, playback_row_y[4],
                context->width - 20.0f, 28.0f}, &playing);
    }
    if(name_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ANIMATED_SPRITE_RENAME,
            .data.animated_sprite_rename = {object->id, sprite->id}};
        snprintf(command.data.animated_sprite_rename.name,
            sizeof(command.data.animated_sprite_rename.name), "%s", name);
        (void)editor_command_execute(context->project, &command);
    }
    if(body_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ANIMATED_SPRITE_BODY_SET,
            .data.animated_sprite_body_set = {object->id, sprite->id,
                body_result.selected_index == 0 ? 0 :
                    object->rigid_bodies[body_result.selected_index - 1].id}};
        (void)editor_command_execute(context->project, &command);
    }
    if(x_result.changed || y_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ANIMATED_SPRITE_POSITION_SET,
            .data.animated_sprite_position_set = {object->id, sprite->id, position}};
        (void)editor_command_execute(context->project, &command);
    }
    if(rotation_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ANIMATED_SPRITE_ROTATION_SET,
            .data.animated_sprite_rotation_set = {object->id, sprite->id, rotation}};
        (void)editor_command_execute(context->project, &command);
    }
    if(sx_result.changed || sy_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ANIMATED_SPRITE_SCALE_SET,
            .data.animated_sprite_scale_set = {object->id, sprite->id,
                {scale_x, scale_y}}};
        (void)editor_command_execute(context->project, &command);
    }
    if(ticks_result.changed || time_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ANIMATED_SPRITE_TIMING_SET,
            .data.animated_sprite_timing_set = {object->id, sprite->id,
                ticks < 0.0f ? 0 : (Tick)ticks,
                seconds < 0.0f ? 0.0 : (Time)seconds}};
        (void)editor_command_execute(context->project, &command);
    }
    if(start_result.changed && starting >= 0.0f) {
        EditorCommand command = {.type = EDITOR_COMMAND_ANIMATED_SPRITE_STARTING_FRAME_SET,
            .data.animated_sprite_starting_frame_set = {object->id, sprite->id,
                (uint32_t)starting}};
        (void)editor_command_execute(context->project, &command);
    }
    if(direction_result.changed) {
        EditorCommand command = {.type = EDITOR_COMMAND_ANIMATED_SPRITE_DIRECTION_SET,
            .data.animated_sprite_direction_set = {object->id, sprite->id,
                direction_result.selected_index == 0 ? DIRECTION_LEFT : DIRECTION_RIGHT}};
        (void)editor_command_execute(context->project, &command);
    }
    if(follow_changed || visible_changed || playing_changed) {
        EditorCommand command = {.type = follow_changed ?
            EDITOR_COMMAND_ANIMATED_SPRITE_FOLLOW_ROTATION_SET : visible_changed ?
                EDITOR_COMMAND_ANIMATED_SPRITE_VISIBILITY_SET :
                EDITOR_COMMAND_ANIMATED_SPRITE_PLAYING_SET,
            .data.animated_sprite_boolean_set = {object->id, sprite->id,
                follow_changed ? follow : visible_changed ? visible : playing}};
        (void)editor_command_execute(context->project, &command);
    }
    if(frames_open && rohr_ui_button("editor.animated_sprite.add_frame",
            &editor->add_frame_label, (UIRect){context->x + 10.0f, add_frame_y,
                context->width - 20.0f, 28.0f}, NULL).clicked && browser_open != NULL)
        browser_open(browser_context, object->id, sprite->id);
    if(frames_open) for(size_t frame = 0; frame < sprite->frame_count; frame += 1) {
        EditorAnimationFrame *asset = &sprite->frames[frame];
        size_t asset_index = frame < 64 ? frame : 63;
        EditorSelectionRef ref = {EDITOR_SELECTION_ANIMATION_FRAME,
            object->id, sprite->id, 0, asset->id};
        UIButtonStyle style = {.idle = {118, 96, 35, 255},
            .hovered = {145, 119, 45, 255}, .pressed = {94, 75, 26, 255},
            .disabled = {60, 52, 30, 255}};
        UIRect bounds = {context->x + 10.0f,
            frame_list_y + (float)frame * frame_row_stride,
            context->width - 20.0f, 26.0f};
        char id[80];
        if(!editor_mode_named_text_sync(editor->font, asset->name,
                &editor->frame_names[asset_index], editor->frame_cache[asset_index],
                EDITOR_OBJECT_NAME_MAX)) return false;
        snprintf(id, sizeof(id), "editor.animated_sprite.frame.%zu", frame);
        UIButtonResult result = rohr_ui_button(id, &editor->frame_names[asset_index],
            bounds, editor_viewport_selection_contains(context->viewport, ref) ?
                &style : NULL);
        editor_mode_element_icon_draw(EDITOR_SELECTION_ANIMATION_FRAME,
            (UIRect){bounds.x + 3.0f, bounds.y + 3.0f, 20.0f, 20.0f});
        if(context->hierarchy_row != NULL)
            context->hierarchy_row(context->hierarchy_context, context->viewport,
                ref, bounds, result, frame + 1 == sprite->frame_count);
        if(result.double_clicked && context->viewport->selected_item_count > 1 &&
                editor_viewport_selection_contains(context->viewport, ref)) {
            if(frame_multi_edit_open != NULL) *frame_multi_edit_open = true;
        } else if(result.clicked || result.focus_changed) {
            if(!result.clicked || !additive_selection ||
                    context->hierarchy_row == NULL)
                (void)editor_viewport_selection_set(context->project,
                    context->viewport, ref,
                    result.clicked ? additive_selection : false);
            if(frame_multi_edit_open != NULL) *frame_multi_edit_open = false;
        }
        if(result.double_clicked &&
                (frame_multi_edit_open == NULL || !*frame_multi_edit_open)) {
            context->viewport->mode = EDITOR_VIEWPORT_ANIMATION_FRAME;
            context->viewport->selected_animation_frame = asset->id;
            break;
        }
    }
    if(context->delete_open_item != NULL && !context->delete_footer) {
        UIButtonStyle style = editor_mode_delete_style_get();
        if(rohr_ui_button("editor.animated_sprite.delete", &editor->delete_label,
                (UIRect){context->x + 10.0f,
                    context->delete_y_get != NULL ?
                        context->delete_y_get(context->delete_context) : 650.0f,
                    context->width - 20.0f, 34.0f}, &style).clicked)
            (void)context->delete_open_item(context->delete_context);
    }
    return name_result.active || x_result.active || y_result.active ||
        rotation_result.active || sx_result.active || sy_result.active ||
        ticks_result.active || time_result.active || start_result.active ||
        layer_active;
}
