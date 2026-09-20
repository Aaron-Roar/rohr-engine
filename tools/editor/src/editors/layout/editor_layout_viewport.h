/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef EDITOR_LAYOUT_VIEWPORT_H
#define EDITOR_LAYOUT_VIEWPORT_H

#include "editors/editor_mode_context.h"
#include "editors/editor_mode_controls.h"

typedef struct EditorLayoutViewportEditor {
    FontAsset *font;
    TextAsset name_label, x_label, y_label, width_label, height_label;
    TextAsset enabled_label, background_color_label;
    TextAsset cameras_label, add_label, delete_label, remove_label;
    TextAsset layer_label, direct_layer_label, visible_label, rotation_label;
    TextAsset draggable_label, drag_axis_label;
    TextAsset drag_x_label, drag_y_label, drag_xy_label;
    TextAsset visible_icon, hidden_icon;
    TextAsset border_label, border_type_label, border_line_label;
    TextAsset border_hashed_label, border_thickness_label;
    TextAsset hash_spacing_label, corner_radius_label, border_color_label;
    TextAsset fill_color_label;
    TextAsset hover_border_color_label, hover_fill_color_label;
    TextAsset click_border_color_label, click_fill_color_label;
    TextAsset add_shape_label, add_slider_label, button_label;
    TextAsset minimum_label, maximum_label, value_label, step_label;
    TextAsset orientation_label, horizontal_label, vertical_label;
    TextAsset track_thickness_label, thumb_shape_label;
    TextAsset rectangle_label, circle_label;
    TextAsset thumb_width_label, thumb_height_label, thumb_radius_label;
    TextAsset thumb_offset_label;
    TextAsset track_color_label, filled_track_color_label, thumb_color_label;
    TextAsset hover_thumb_color_label, pressed_thumb_color_label;
    TextAsset text_label, font_file_label, font_color_label;
    TextAsset default_font_label, load_font_label;
    TextAsset add_vertex_label, length_label;
    TextAsset width_scale_label, height_scale_label;
    TextAsset content_x_label, content_y_label, content_rotation_label;
    TextAsset content_width_scale_label, content_height_scale_label;
    TextAsset source_label;
    TextAsset text_offset_x_label, text_offset_y_label;
    TextAsset name_field, x_field, y_field, width_field, height_field, layer_field;
    TextAsset rotation_field;
    TextAsset text_field, font_file_field, width_scale_field, height_scale_field;
    TextAsset length_field;
    TextAsset minimum_field, maximum_field, value_field, step_field;
    TextAsset track_thickness_field;
    TextAsset thumb_width_field, thumb_height_field, thumb_radius_field;
    TextAsset thumb_offset_field;
    TextAsset content_x_field, content_y_field, content_rotation_field;
    TextAsset border_thickness_field, hash_spacing_field, corner_radius_field;
    TextAsset camera_names[EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX];
    char camera_cache[EDITOR_LAYOUT_VIEWPORT_CAMERA_MAX][EDITOR_OBJECT_NAME_MAX];
    TextAsset ui_names[EDITOR_LAYOUT_VIEWPORT_UI_MAX];
    char ui_cache[EDITOR_LAYOUT_VIEWPORT_UI_MAX][EDITOR_OBJECT_NAME_MAX];
    TextAsset font_names[EDITOR_UI_FONT_MAX];
    char font_cache[EDITOR_UI_FONT_MAX][EDITOR_OBJECT_NAME_MAX];
    EditorModeAccordionSection transform_section, appearance_section;
    EditorModeAccordionSection source_section, placement_section;
    EditorModeAccordionSection interaction_section, content_section;
    EditorModeAccordionSection ui_transform_section, ui_interaction_section;
    EditorModeAccordionSection ui_appearance_section, ui_content_section;
} EditorLayoutViewportEditor;

bool editor_layout_viewport_editor_create(EditorLayoutViewportEditor *editor,
    FontAsset *font);
void editor_layout_viewport_editor_destroy(EditorLayoutViewportEditor *editor);
bool editor_layout_viewport_editor_draw(EditorLayoutViewportEditor *editor,
    const EditorModeContext *context);
bool editor_layout_camera_editor_draw(EditorLayoutViewportEditor *editor,
    const EditorModeContext *context);
bool editor_ui_shape_editor_draw(EditorLayoutViewportEditor *editor,
    const EditorModeContext *context);
bool editor_ui_text_editor_draw(EditorLayoutViewportEditor *editor,
    const EditorModeContext *context);
bool editor_ui_slider_editor_draw(EditorLayoutViewportEditor *editor,
    const EditorModeContext *context);
bool editor_ui_vertex_editor_draw(EditorLayoutViewportEditor *editor,
    const EditorModeContext *context);
bool editor_ui_line_editor_draw(EditorLayoutViewportEditor *editor,
    const EditorModeContext *context);

#endif
