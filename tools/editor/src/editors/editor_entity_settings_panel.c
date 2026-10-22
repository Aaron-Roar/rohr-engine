/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_entity_settings_panel.h"
#include "editor_navigation.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct SettingField {
    EditorPropertyKind property;
    const char *name, *label;
    EditorPropertyValueKind type;
    size_t offset;
} SettingField;
static const SettingField setting_fields[] = {
#define FIELD(symbol, name, label, type, member) \
    {EDITOR_PROPERTY_##symbol, name, label, EDITOR_PROPERTY_VALUE_##type, offsetof(ObjectEntitySettings, member)},
    EDITOR_ENTITY_SETTING_PROPERTIES(FIELD)
#undef FIELD
};
_Static_assert(sizeof(setting_fields) / sizeof(setting_fields[0]) == EDITOR_ENTITY_SETTING_COUNT,
    "entity settings panel field count");

bool editor_entity_settings_panel_create(EditorEntitySettingsPanel *p, FontAsset *font) {
    if(!p || !font) return false;
    *p = (EditorEntitySettingsPanel){.font = font};
    p->references = calloc(MAX_ENTITIES, sizeof(*p->references));
    p->options = calloc(MAX_ENTITIES, sizeof(*p->options));
    p->selections = calloc(MAX_ENTITIES, sizeof(*p->selections));
    if(!p->references || !p->options || !p->selections) goto fail;
    const char *sections[] = {"Lifetime", "Angular Motion", "Gameplay Reference", "Axis Lock", "Transform Lock"};
    for(size_t i = 0; i < 5; ++i)
        if(!editor_mode_accordion_section_create(&p->sections[i], font, sections[i], false)) goto fail;
    for(size_t i = 0; i < EDITOR_ENTITY_SETTING_COUNT; ++i)
        if(!editor_mode_text_create(font, setting_fields[i].label, &p->labels[i]) ||
                !editor_mode_text_create(font, "", &p->fields[i])) goto fail;
    return true;
fail:
    editor_entity_settings_panel_destroy(p); return false;
}
void editor_entity_settings_panel_destroy(EditorEntitySettingsPanel *p) {
    if(!p) return;
    for(size_t i = 0; i < 5; ++i) editor_mode_accordion_section_destroy(&p->sections[i]);
    for(size_t i = 0; i < EDITOR_ENTITY_SETTING_COUNT; ++i) {
        rohr_graphics_text_destroy(&p->labels[i]); rohr_graphics_text_destroy(&p->fields[i]);
    }
    if(p->references) for(size_t i = 0; i < MAX_ENTITIES; ++i)
        rohr_graphics_text_destroy(&p->references[i].text);
    free(p->references); free(p->options); free(p->selections);
    *p = (EditorEntitySettingsPanel){0};
}

static bool selection_kind_get(EditorHierarchySelection selection, EditorItemKind *kind) {
    switch(selection) {
        case EDITOR_SELECTION_OBJECT: *kind = EDITOR_ITEM_OBJECT; break;
        case EDITOR_SELECTION_RIGID_BODY: case EDITOR_SELECTION_PARTICLE: *kind = EDITOR_ITEM_RIGID_BODY; break;
        case EDITOR_SELECTION_SOFT_BODY: *kind = EDITOR_ITEM_SOFT_BODY; break;
        case EDITOR_SELECTION_SOFT_NODE: *kind = EDITOR_ITEM_SOFT_NODE; break;
        case EDITOR_SELECTION_SOFT_BEAM: *kind = EDITOR_ITEM_SOFT_BEAM; break;
        case EDITOR_SELECTION_SOFT_AREA: *kind = EDITOR_ITEM_SOFT_AREA; break;
        case EDITOR_SELECTION_ANCHOR: *kind = EDITOR_ITEM_ANCHOR; break;
        case EDITOR_SELECTION_JOINT: *kind = EDITOR_ITEM_JOINT; break;
        case EDITOR_SELECTION_SPRITE: *kind = EDITOR_ITEM_SPRITE; break;
        case EDITOR_SELECTION_ANIMATED_SPRITE: *kind = EDITOR_ITEM_ANIMATED_SPRITE; break;
        case EDITOR_SELECTION_CAMERA: *kind = EDITOR_ITEM_CAMERA; break;
        case EDITOR_SELECTION_FORCE: *kind = EDITOR_ITEM_FORCE; break;
        case EDITOR_SELECTION_TORQUE: *kind = EDITOR_ITEM_TORQUE; break;
        default: return false;
    }
    return true;
}
static void reference_add(EditorEntitySettingsPanel *p, size_t *count,
        const char *name, ObjectEntityReference reference, EditorSelectionRef selection) {
    if(*count >= MAX_ENTITIES) return;
    EditorEntityReferenceOption *entry = &p->references[*count];
    if(!editor_mode_named_text_sync(p->font, name, &entry->text, entry->name, sizeof(entry->name))) return;
    entry->reference = reference; entry->selection = selection;
    p->options[*count] = &entry->text; p->selections[*count] = selection;
    ++*count;
}
static size_t references_get(EditorEntitySettingsPanel *p, EditorObject *o) {
    size_t count = 0;
    reference_add(p, &count, "None", (ObjectEntityReference){0}, (EditorSelectionRef){0});
    reference_add(p, &count, o->name, (ObjectEntityReference){OBJECT_CONTAINER, o->id, 0},
        (EditorSelectionRef){EDITOR_SELECTION_OBJECT, o->id, 0, 0, o->id});
#define ADD(array, amount, type, selection_type, parent_id) \
    for(size_t i = 0; i < (amount); ++i) { \
        reference_add(p, &count, (array)[i].name, \
            (ObjectEntityReference){type, (array)[i].id, parent_id}, \
            (EditorSelectionRef){selection_type, o->id, parent_id, 0, (array)[i].id}); \
    }
    for(size_t i = 0; i < o->rigid_body_count; ++i) {
        EditorRigidBody *body = &o->rigid_bodies[i];
        reference_add(p, &count, body->name, (ObjectEntityReference){OBJECT_BODY, body->id, 0},
            (EditorSelectionRef){body->standalone_particle ? EDITOR_SELECTION_PARTICLE :
                EDITOR_SELECTION_RIGID_BODY, o->id, 0, 0, body->id});
    }
    ADD(o->soft_body_items, o->soft_body_count, OBJECT_SOFT_BODY, EDITOR_SELECTION_SOFT_BODY, 0)
    ADD(o->anchors, o->anchor_count, OBJECT_ANCHOR, EDITOR_SELECTION_ANCHOR, 0)
    ADD(o->joint_items, o->joint_count, OBJECT_JOINT, EDITOR_SELECTION_JOINT, 0)
    ADD(o->sprites, o->sprite_count, OBJECT_SPRITE, EDITOR_SELECTION_SPRITE, 0)
    ADD(o->animated_sprite_items, o->animated_sprite_count, OBJECT_ANIMATION, EDITOR_SELECTION_ANIMATED_SPRITE, 0)
    ADD(o->cameras, o->camera_count, OBJECT_CAMERA, EDITOR_SELECTION_CAMERA, 0)
    ADD(o->force_items, o->force_count, OBJECT_FORCE, EDITOR_SELECTION_FORCE, 0)
    ADD(o->torque_items, o->torque_count, OBJECT_TORQUE, EDITOR_SELECTION_TORQUE, 0)
    for(size_t j = 0; j < o->soft_body_count; ++j) {
        EditorSoftBody *b = &o->soft_body_items[j];
        ADD(b->nodes, b->node_count, OBJECT_NODE, EDITOR_SELECTION_SOFT_NODE, b->id)
        ADD(b->beams, b->beam_count, OBJECT_BEAM, EDITOR_SELECTION_SOFT_BEAM, b->id)
        ADD(b->areas, b->area_count, OBJECT_AREA, EDITOR_SELECTION_SOFT_AREA, b->id)
    }
#undef ADD
    return count;
}

bool editor_entity_settings_panel_draw(EditorEntitySettingsPanel *p,
        const EditorModeContext *context, float y) {
    EditorSelectionRef selected;
    EditorItemKind kind;
    if(!p || !context || !context->project || !context->viewport ||
            context->viewport->selected_item_count > 1) return false;
    EditorViewportState opened = *context->viewport;
    if(!editor_navigation_open_item_selection_set(&opened) ||
            !editor_viewport_selection_ref_get(context->project, &opened, &selected) ||
            !selection_kind_get(selected.kind, &kind)) return false;
    ObjectEntitySettings *settings = editor_command_entity_settings_get(context->project,
        kind, selected.object, selected.parent, selected.item);
    EditorObject *object = editor_object_query_get(context->project, selected.object);
    if(!settings || !object) return false;
    bool active = false;
    const size_t starts[] = {0, 6, 9, 10, 16}, counts[] = {6, 3, 1, 6, 7};
    const float heights[] = {28, 28, 28, 28, 28, 28, 28};
    EditorModeAccordionLayoutCursor cursor =
        editor_mode_accordion_layout_cursor_get(context->x, context->width, y);
    for(size_t section = 0; section < 5; ++section) {
        if(section == 1 && (kind != EDITOR_ITEM_RIGID_BODY ||
                selected.kind == EDITOR_SELECTION_PARTICLE)) continue;
        if(section == 2 && (kind == EDITOR_ITEM_FORCE || kind == EDITOR_ITEM_TORQUE)) continue;
        char section_id[64];
        snprintf(section_id, sizeof(section_id), "editor.entity_settings.section.%zu", section);
        EditorModeAccordionLayoutResult layout = editor_mode_accordion_layout_section(
            &cursor, &p->sections[section], section_id, heights, counts[section], 6);
        if(!layout.expanded) continue;
        size_t reference_count = section == 2 || section == 4 ? references_get(p, object) : 0;
        for(size_t row = 0; row < counts[section]; ++row) {
            size_t i = starts[section] + row;
            SettingField field = setting_fields[i];
            EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET,
                .data.property_set = {.kind = kind, .object = selected.object,
                    .parent = selected.parent, .item = selected.item,
                    .property = field.property, .value_kind = field.type}};
            EditorPropertySetCommand *set = &command.data.property_set;
            void *value = (char *)settings + field.offset;
            float row_y = editor_mode_accordion_layout_row_y(&layout, heights, row, 6);
            char id[96]; snprintf(id, sizeof(id), "editor.entity_settings.%s", field.name);
            UIRect bounds = {context->x + 148, row_y, context->width - 158, 28};
            bool changed = false;
            if(field.type == EDITOR_PROPERTY_VALUE_BOOL) {
                set->value.boolean = *(bool *)value;
                changed = editor_mode_checkbox_left(id, &p->labels[i],
                    (UIRect){context->x + 10, row_y, context->width - 20, 28}, &set->value.boolean);
            } else {
                rohr_ui_label(&p->labels[i], (UIRect){context->x + 8, row_y, 136, 28});
                if(field.type == EDITOR_PROPERTY_VALUE_REFERENCE) {
                    ObjectEntityReference current = *(ObjectEntityReference *)value;
                    size_t index = 0;
                    for(size_t r = 1; r < reference_count; ++r)
                        if((current.kind == p->references[r].reference.kind ||
                                (current.kind == OBJECT_PARTICLE && p->references[r].reference.kind == OBJECT_BODY)) &&
                                current.id == p->references[r].reference.id &&
                                current.soft_body == p->references[r].reference.soft_body) index = r;
                    UIDropdownResult result = editor_mode_reference_dropdown(id, p->options,
                        reference_count, index, bounds, NULL, context, p->selections);
                    if(result.changed) {
                        set->value.reference = p->references[result.selected_index].reference;
                        changed = true;
                    }
                } else if(field.type == EDITOR_PROPERTY_VALUE_FLOAT) {
                    set->value.number = *(float *)value;
                    UIFieldResult result = editor_mode_field(id,
                        (UIFieldBinding){.kind = UI_FIELD_FLOAT, .number = &set->value.number},
                        &p->fields[i], bounds, NULL);
                    changed = result.changed; active |= result.active;
                } else {
                    char text[64];
                    if(field.type == EDITOR_PROPERTY_VALUE_DOUBLE)
                        snprintf(text, sizeof(text), "%.17g", *(double *)value);
                    else snprintf(text, sizeof(text), "%llu", (unsigned long long)*(uint64_t *)value);
                    UIFieldResult result = editor_mode_field(id, (UIFieldBinding){
                        .kind = UI_FIELD_STRING, .string = text, .string_capacity = sizeof(text)},
                        &p->fields[i], bounds, NULL);
                    changed = result.changed && editor_command_entity_setting_value_parse(text, set);
                    active |= result.active;
                }
            }
            if(changed) (void)editor_command_execute(context->project, &command);
        }
    }
    return active;
}
