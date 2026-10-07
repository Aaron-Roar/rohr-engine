/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_collision_controls.h"
#include "editor_mode_controls.h"
#include <stdio.h>
#include <string.h>

enum { COLLISION_ROW_HEIGHT = 28, COLLISION_ROW_STRIDE = 32 };
static bool target_get(EditorProject *project, EditorSelectionRef ref,
        EditorItemKind *kind, bool *enabled, uint64_t *category, uint64_t *with) {
    EditorObject *object = NULL;
    if(project == NULL) return false;
    for(size_t i = 0; i < project->object_count; i += 1)
        if(project->objects[i].id == ref.object) object = &project->objects[i];
    if(object == NULL) return false;
#define TARGET(value, item_kind) do { \
    *kind = item_kind; *enabled = (value)->collision_enabled; \
    *category = (value)->collision_category; *with = (value)->collision_with; return true; \
} while(0)
    if(ref.kind == EDITOR_SELECTION_RIGID_BODY || ref.kind == EDITOR_SELECTION_PARTICLE) {
        EditorRigidBody *body = editor_project_rigid_body_get(object, ref.item);
        if(body != NULL) TARGET(body, EDITOR_ITEM_RIGID_BODY);
    }
    for(size_t b = 0; b < object->soft_body_count; b += 1) {
        EditorSoftBody *body = &object->soft_body_items[b];
        if(body->id != ref.parent) continue;
        if(ref.kind == EDITOR_SELECTION_SOFT_NODE)
            for(size_t i = 0; i < body->node_count; i += 1)
                if(body->nodes[i].id == ref.item) TARGET(&body->nodes[i], EDITOR_ITEM_SOFT_NODE);
        if(ref.kind == EDITOR_SELECTION_SOFT_BEAM)
            for(size_t i = 0; i < body->beam_count; i += 1)
                if(body->beams[i].id == ref.item) TARGET(&body->beams[i], EDITOR_ITEM_SOFT_BEAM);
    }
#undef TARGET
    return false;
}
bool editor_collision_values_get(EditorProject *project, const EditorSelectionRef *selections,
        size_t count, EditorCollisionValues *values) {
    if(selections == NULL || count == 0 || values == NULL) return false;
    EditorCollisionValues result = {.enabled_all = true, .category_all = UINT64_MAX, .with_all = UINT64_MAX};
    for(size_t i = 0; i < count; i += 1) {
        EditorItemKind kind; bool enabled; uint64_t category, with;
        if(!target_get(project, selections[i], &kind, &enabled, &category, &with)) return false;
        result.enabled_all &= enabled; result.enabled_any |= enabled;
        result.category_all &= category; result.category_any |= category;
        result.with_all &= with; result.with_any |= with;
    }
    *values = result;
    return true;
}
EditorResult editor_collision_edit(EditorProject *project, EditorHistory *history,
        const EditorSelectionRef *selections, size_t count, int filter,
        const char *mask, bool enabled, bool create) {
    EditorCollisionValues values;
    if(!editor_collision_values_get(project, selections, count, &values) ||
            filter < -1 || filter > EDITOR_COLLISION_FILTER_COLLIDE_WITH ||
            (filter >= 0 && mask == NULL) || (create && filter < 0))
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Selection does not support this collision edit");
    bool transaction = history != NULL;
    if((count > 1 || create) && !transaction)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Bulk collision edits require history");
    if(transaction && !editor_history_transaction_begin(history))
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Could not begin collision edit");
    if(transaction) for(size_t i = 0; i < count; i += 1)
        if(!editor_history_transaction_object_track(history, selections[i].object)) {
            editor_history_transaction_cancel(history);
            return editor_result_error(EDITOR_ERROR_CAPACITY, "Could not capture collision edit");
        }
    if(create && !editor_history_transaction_collision_track(history)) {
        editor_history_transaction_cancel(history);
        return editor_result_error(EDITOR_ERROR_CAPACITY, "Could not capture collision categories");
    }
    EditorCommandResult executed = {.kind = ERROR_RESULT_VALUE};
    char created_name[EDITOR_OBJECT_NAME_MAX];
    if(create) {
        EditorCommand command = {.type = EDITOR_COMMAND_COLLISION_MASK_ADD};
        snprintf(command.data.collision_mask_add.name, sizeof(command.data.collision_mask_add.name), "%s", mask);
        executed = editor_command_execute(project, &command);
        if(executed.kind == ERROR_RESULT_ERROR) goto fail;
        snprintf(created_name, sizeof(created_name), "%s", project->collision_masks[executed.result.object].name);
        mask = created_name;
    }
    for(size_t i = 0; i < count; i += 1) {
        EditorSelectionRef ref = selections[i];
        EditorItemKind kind; bool prior; uint64_t category, with;
        if(!target_get(project, ref, &kind, &prior, &category, &with)) {
            executed.kind = ERROR_RESULT_ERROR;
            executed.result.error = editor_result_error(EDITOR_ERROR_NOT_FOUND, "Collision target disappeared").result.error;
            goto fail;
        }
        EditorCommand command = {0};
        uint32_t parent = kind == EDITOR_ITEM_RIGID_BODY ? 0 : ref.parent;
        if(filter < 0) command = (EditorCommand){.type = EDITOR_COMMAND_PROPERTY_SET,
            .data.property_set = {kind, ref.object, parent, ref.item, 0,
                EDITOR_PROPERTY_COLLISION, EDITOR_PROPERTY_VALUE_BOOL, {.boolean = enabled}}};
        else {
            command = (EditorCommand){.type = EDITOR_COMMAND_COLLISION_FILTER_SET,
                .data.collision_filter_set = {.kind = kind, .object = ref.object, .parent = parent,
                    .item = ref.item, .filter = (EditorCollisionFilterKind)filter, .enabled = enabled}};
            snprintf(command.data.collision_filter_set.mask, sizeof(command.data.collision_filter_set.mask), "%s", mask);
        }
        executed = editor_command_execute(project, &command);
        if(executed.kind == ERROR_RESULT_ERROR) goto fail;
    }
    if(transaction && !editor_history_transaction_end(history)) {
        editor_history_transaction_cancel(history);
        return editor_result_error(EDITOR_ERROR_CAPACITY, "Could not finish collision edit");
    }
    return editor_result_value(true);
fail:
    if(transaction) editor_history_transaction_cancel(history);
    return (EditorResult){.kind = ERROR_RESULT_ERROR, .result.error = executed.result.error};
}
bool editor_collision_controls_create(EditorCollisionControls *controls, FontAsset *font) {
    if(controls == NULL || font == NULL) return false;
    *controls = (EditorCollisionControls){.font = font};
    if(!editor_mode_text_create(font, "Collision", &controls->toggle) ||
            !editor_mode_text_create(font, "Collision Category", &controls->headers[0]) ||
            !editor_mode_text_create(font, "Collides With", &controls->headers[1]) ||
            !editor_mode_text_create(font, "Add", &controls->add) ||
            !editor_mode_text_create(font, "", &controls->fields[0]) ||
            !editor_mode_text_create(font, "", &controls->fields[1]) ||
            !editor_mode_text_create(font, "", &controls->error)) {
        editor_collision_controls_destroy(controls); return false;
    }
    return true;
}
void editor_collision_controls_destroy(EditorCollisionControls *controls) {
    if(controls == NULL) return;
    rohr_graphics_text_destroy(&controls->toggle); rohr_graphics_text_destroy(&controls->add);
    rohr_graphics_text_destroy(&controls->error);
    for(size_t i = 0; i < 2; i += 1) {
        rohr_graphics_text_destroy(&controls->headers[i]); rohr_graphics_text_destroy(&controls->fields[i]);
    }
    for(size_t i = 0; i < EDITOR_COLLISION_MASK_MAX; i += 1) rohr_graphics_text_destroy(&controls->labels[i]);
    *controls = (EditorCollisionControls){0};
}
void editor_collision_controls_selection_set(EditorCollisionControls *controls,
        const EditorSelectionRef *selections, size_t count) {
    uint64_t key = UINT64_C(14695981039346656037);
    for(size_t i = 0; i < count; i += 1) {
        uint32_t parts[] = {selections[i].kind, selections[i].object, selections[i].parent,
            selections[i].container, selections[i].item};
        for(size_t p = 0; p < 5; p += 1) key = (key ^ parts[p]) * UINT64_C(1099511628211);
    }
    if(key == controls->selection_key) return;
    controls->selection_key = key;
    controls->open[0] = controls->open[1] = false;
    controls->names[0][0] = controls->names[1][0] = '\0';
    controls->error_message[0] = '\0';
}
float editor_collision_controls_height_get(const EditorCollisionControls *controls,
        const EditorProject *project, bool toggle) {
    size_t rows = (toggle ? 1 : 0) + 2;
    for(size_t i = 0; i < 2; i += 1) if(controls->open[i]) rows += project->collision_mask_count + 1;
    if(controls->error_message[0]) rows++;
    return rows * COLLISION_ROW_STRIDE;
}
static bool checkbox(const char *id, TextAsset *label, UIRect bounds, bool all, bool any) {
    UIButtonResult result = rohr_ui_interaction(id, bounds);
    rohr_ui_surface(bounds, result.hovered ? (Color){67,75,90,255} : (Color){48,54,66,255});
    UIRect box = {bounds.x + 4, bounds.y + 4, 20, 20};
    rohr_ui_border(box, 2, (Color){200,205,215,255});
    if(all) rohr_ui_surface((UIRect){box.x + 5,box.y + 5,10,10}, (Color){225,230,240,255});
    else if(any) rohr_ui_surface((UIRect){box.x + 4,box.y + 9,12,2}, (Color){255,215,70,255});
    rohr_ui_label(label, (UIRect){bounds.x + 32,bounds.y,bounds.width - 32,bounds.height});
    return result.clicked;
}
static void error_set(EditorCollisionControls *controls, EditorResult result) {
    snprintf(controls->error_message, sizeof(controls->error_message), "%s",
        editor_result_check(result) ? result.result.error.message : "");
    (void)rohr_graphics_text_value_set(&controls->error, controls->error_message);
}
EditorCollisionDrawResult editor_collision_controls_draw(EditorCollisionControls *controls,
        const char *id, EditorProject *project, EditorHistory *history,
        const EditorSelectionRef *selections, size_t count, float x, float y, float width, bool toggle) {
    EditorCollisionDrawResult result = {.bottom = y};
    EditorCollisionValues values;
    if(!editor_collision_values_get(project, selections, count, &values)) return result;
    char key[128];
    /* Use the expansion state measured at the beginning of this frame. */
    bool open[] = {controls->open[0], controls->open[1]};
    if(toggle) {
        snprintf(key, sizeof(key), "%s.enabled", id);
        if(checkbox(key, &controls->toggle, (UIRect){x,y,width,COLLISION_ROW_HEIGHT}, values.enabled_all, values.enabled_any)) {
            error_set(controls, editor_collision_edit(project, history, selections, count, -1, NULL, !values.enabled_all, false));
            result.changed = true; goto done;
        }
        y += COLLISION_ROW_STRIDE;
    }
    for(size_t f = 0; f < 2; f += 1) {
        snprintf(key, sizeof(key), "%s.filter.%zu", id, f);
        controls->header_bounds[f] = (UIRect){x,y,width,COLLISION_ROW_HEIGHT};
        if(rohr_ui_button(key, &controls->headers[f], controls->header_bounds[f], NULL).clicked)
            controls->open[f] = !controls->open[f];
        y += COLLISION_ROW_STRIDE;
        controls->list_bounds[f] = (UIRect){x,y,width,0};
        if(!open[f]) continue;
        float top = y;
        snprintf(key, sizeof(key), "%s.filter.%zu.name", id, f);
        UIFieldResult field = editor_mode_field(key, (UIFieldBinding){.kind = UI_FIELD_STRING,
            .string = controls->names[f], .string_capacity = sizeof(controls->names[f])},
            &controls->fields[f], (UIRect){x,y,width * .72f,COLLISION_ROW_HEIGHT}, NULL);
        result.active |= field.active;
        snprintf(key, sizeof(key), "%s.filter.%zu.add", id, f);
        if(rohr_ui_button(key, &controls->add, (UIRect){x + width * .72f,y,width * .28f,COLLISION_ROW_HEIGHT}, NULL).clicked) {
            error_set(controls, editor_collision_edit(project, history, selections, count, (int)f, controls->names[f], true, true));
            if(!controls->error_message[0]) controls->names[f][0] = '\0';
            result.changed = true; goto done;
        }
        y += COLLISION_ROW_STRIDE;
        for(size_t m = 0; m < project->collision_mask_count; m += 1) {
            if(!editor_mode_named_text_sync(controls->font, project->collision_masks[m].name,
                    &controls->labels[m], controls->caches[m], sizeof(controls->caches[m]))) continue;
            uint64_t bit = UINT64_C(1) << m;
            bool all = ((f == 0 ? values.category_all : values.with_all) & bit) != 0;
            bool any = ((f == 0 ? values.category_any : values.with_any) & bit) != 0;
            snprintf(key, sizeof(key), "%s.filter.%zu.mask.%zu", id, f, m);
            if(checkbox(key, &controls->labels[m], (UIRect){x,y,width,COLLISION_ROW_HEIGHT}, all, any)) {
                error_set(controls, editor_collision_edit(project, history, selections, count, (int)f,
                    project->collision_masks[m].name, !all, false));
                result.changed = true; goto done;
            }
            y += COLLISION_ROW_STRIDE;
        }
        controls->list_bounds[f].height = y - top;
        rohr_ui_border(controls->list_bounds[f], 1, (Color){184,190,202,255});
    }
    if(controls->error_message[0]) {
        rohr_ui_label(&controls->error, (UIRect){x,y,width,COLLISION_ROW_HEIGHT});
        y += COLLISION_ROW_STRIDE;
    }
done:
    result.bottom = y;
    editor_mode_accordion_layout_measure_include(y);
    return result;
}
