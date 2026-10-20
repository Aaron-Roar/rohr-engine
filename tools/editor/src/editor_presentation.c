/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_project.h"
#include "editor_array.h"
#include <math.h>
#include <string.h>

bool editor_audio_definition_check(const EditorAudioDefinition *value) {
    if(!value || !value->id || value->id == UINT32_MAX ||
            !memchr(value->name, 0, sizeof(value->name)) ||
            !memchr(value->path, 0, sizeof(value->path)) ||
            !value->name[0] || !value->path[0]) return false;
    char identifier[EDITOR_OBJECT_NAME_MAX];
    editor_project_property_name_format(identifier, sizeof(identifier), value->name);
    static const char *reserved[] = {"auto", "break", "case", "char", "const", "continue",
        "default", "do", "double", "else", "enum", "extern", "float", "for", "goto",
        "if", "inline", "int", "long", "register", "restrict", "return", "short",
        "signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned",
        "void", "volatile", "while", "bool", "true", "false"};
    for(size_t i = 0; i < sizeof(reserved) / sizeof(*reserved); ++i)
        if(!strcmp(reserved[i], value->name)) return false;
    return !strcmp(identifier, value->name) &&
        isfinite(value->volume) && value->volume >= 0 && value->volume <= 1 &&
        isfinite(value->pan) && value->pan >= -1 && value->pan <= 1 &&
        (!value->music || value->pan == 0) &&
        isfinite(value->playback_rate) && value->playback_rate >= 0;
}

bool editor_project_presentation_check(const EditorProject *project) {
    if(!project) return false;
    const WindowPresentationDescription *window = &project->application_window;
    if(window->mode < GRAPHICS_WINDOW_MODE_WINDOWED ||
            window->mode > GRAPHICS_WINDOW_MODE_FULLSCREEN ||
            window->window_width <= 0 || window->window_height <= 0 ||
            window->logical_width <= 0 || window->logical_height <= 0) return false;
    for(size_t i = 0; i < project->ui_font_count; ++i)
        if(!isfinite(project->ui_fonts[i].point_size) || project->ui_fonts[i].point_size <= 0) return false;
    for(size_t i = 0; i < project->ui_definition_count; ++i) {
        const EditorViewportUiDefinition *definition = &project->ui_definitions[i];
        const EditorViewportUiText *text = definition->kind == EDITOR_VIEWPORT_UI_TEXT ? &definition->value.text :
            definition->kind == EDITOR_VIEWPORT_UI_SHAPE ? &definition->value.shape.text : NULL;
        if(text && (!isfinite(text->rotation) || text->wrap_width < 0)) return false;
    }
    for(size_t i = 0; i < project->layout_viewport_count; ++i)
        for(size_t j = 0; j < project->layout_viewports[i].camera_item_count; ++j) {
            const EditorViewportCameraItem *item = &project->layout_viewports[i].camera_items[j];
            if(item->render_size_explicit && (item->render_width <= 0 || item->render_height <= 0)) return false;
        }
    for(size_t i = 0; i < project->audio_definition_count; ++i) {
        const EditorAudioDefinition *audio = &project->audio_definitions[i];
        if(!editor_audio_definition_check(audio)) return false;
        for(size_t j = 0; j < i; ++j) {
            const EditorAudioDefinition *other = &project->audio_definitions[j];
            if(other->id == audio->id || (other->music == audio->music &&
                    !strcmp(other->name, audio->name))) return false;
        }
    }
    return true;
}

EditorAudioDefinition *editor_project_audio_get(EditorProject *project, uint32_t id) {
    if(project) for(size_t i = 0; i < project->audio_definition_count; ++i)
        if(project->audio_definitions[i].id == id) return &project->audio_definitions[i];
    return NULL;
}

static bool audio_unique(const EditorProject *project, const EditorAudioDefinition *value) {
    if(!editor_audio_definition_check(value)) return false;
    for(size_t i = 0; i < project->audio_definition_count; ++i) {
        const EditorAudioDefinition *other = &project->audio_definitions[i];
        if(other->id != value->id && other->music == value->music &&
                !strcmp(other->name, value->name)) return false;
    }
    return true;
}

EditorAudioDefinition *editor_project_audio_add(EditorProject *project, bool music,
        const char *name, const char *path) {
    if(!project || !name || !path || strlen(name) >= EDITOR_OBJECT_NAME_MAX ||
            strlen(path) >= EDITOR_ASSET_PATH_MAX) return NULL;
    EditorAudioDefinition value = {.id = project->next_audio_definition_id,
        .music = music, .playback_rate = 1};
    memcpy(value.name, name, strlen(name) + 1);
    memcpy(value.path, path, strlen(path) + 1);
    if(!audio_unique(project, &value) ||
            !EDITOR_ARRAY_RESERVE(project->audio_definitions,
                project->audio_definition_capacity, project->audio_definition_count + 1))
        return NULL;
    EditorAudioDefinition *added = &project->audio_definitions[project->audio_definition_count++];
    *added = value;
    ++project->next_audio_definition_id;
    return added;
}

bool editor_project_audio_set(EditorProject *project, EditorAudioDefinition value) {
    EditorAudioDefinition *stored = editor_project_audio_get(project, value.id);
    if(!stored || !audio_unique(project, &value)) return false;
    *stored = value;
    return true;
}

bool editor_project_audio_remove(EditorProject *project, uint32_t id) {
    if(!project) return false;
    for(size_t i = 0; i < project->audio_definition_count; ++i) {
        if(project->audio_definitions[i].id != id) continue;
        memmove(project->audio_definitions + i, project->audio_definitions + i + 1,
            (project->audio_definition_count - i - 1) * sizeof(*project->audio_definitions));
        --project->audio_definition_count;
        return true;
    }
    return false;
}
