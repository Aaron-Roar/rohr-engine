/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "editor_mode_controls.h"
#include "editor_layout.h"

typedef struct EditorReferencePickContext {
    const EditorModeContext *mode;
    const EditorSelectionRef *references;
    size_t count;
} EditorReferencePickContext;

void editor_mode_reference_frame_begin(EditorViewportState *state, bool blocked) {
    if(state == NULL) return;
    state->reference_picker_open = false;
    state->reference_picker_hovered = false;
    state->reference_picker_blocked = blocked;
    state->reference_preview = (EditorSelectionRef){0};
}

static int editor_reference_pick(Position pointer, void *opaque) {
    EditorReferencePickContext *pick = opaque;
    UIRect excluded = pick->mode->viewport->reference_pick_overlay;
    if(pointer.x >= excluded.x && pointer.x < excluded.x + excluded.width &&
            pointer.y >= excluded.y && pointer.y < excluded.y + excluded.height)
        return -1;
    int index = editor_viewport_reference_at_get(pick->mode->project,
        pick->mode->viewport, pointer, pick->references, pick->count);
    pick->mode->viewport->reference_picker_hovered = index >= 0;
    return index;
}

UIDropdownResult editor_mode_reference_dropdown(const char *id,
        const TextAsset *const *options, size_t option_count, size_t selected_index,
        UIRect bounds, const UIButtonStyle *style, const EditorModeContext *context,
        const EditorSelectionRef *references) {
    if(context == NULL || context->viewport == NULL || references == NULL)
        return editor_mode_dropdown(id, options, option_count, selected_index, bounds, style);
    EditorViewportState *state = context->viewport;
    if(!rohr_graphics_text_valid_check(state->reference_hint)) {
        TextAssetResult hint = rohr_graphics_text_create((TextDescription){
            .value = "Choose below or click in the viewport\nEsc to cancel",
            .wrap_width = 260, .color = {200, 210, 225, 255}});
        if(rohr_error_check(hint))
            return editor_mode_dropdown(id, options, option_count, selected_index, bounds, style);
        state->reference_hint = hint.result.value;
    }
    EditorReferencePickContext pick = {context, references, option_count};
    UIButtonStyle baseline = editor_mode_section_field_style_get();
    UIDropdownResult result = rohr_ui_dropdown(id, options, option_count, selected_index,
        (UIDropdownConfig){.visible_row_limit = EDITOR_DROPDOWN_VISIBLE_ROW_LIMIT,
            .fit_window = true, .hint = &state->reference_hint,
            .pick_bounds = state->reference_picker_blocked ? (UIRect){0} :
                (UIRect){0, EDITOR_VIEWPORT_TOP, EDITOR_VIEWPORT_WIDTH,
                    EDITOR_VIEWPORT_BOTTOM - EDITOR_VIEWPORT_TOP},
            .pick_exclusion = context->reference_pick_exclusion,
            .pick = editor_reference_pick, .pick_context = &pick},
        bounds, style == NULL ? &baseline : style);
    state->reference_picker_open |= result.open;
    if(result.hovered_index >= 0 && (size_t)result.hovered_index < option_count) {
        EditorSelectionRef ref = references[result.hovered_index];
        state->reference_preview = ref;
        if(ref.kind == EDITOR_SELECTION_ANCHOR) state->preview_anchor = ref.item;
        if(ref.kind == EDITOR_SELECTION_CAMERA) state->preview_camera = ref.item;
    }
    return result;
}
