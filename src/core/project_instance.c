/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "project_instance.h"
#include "core/project_internal.h"
#include "core/engine_internal.h"
#include "entity/object_internal.h"
#include "graphics/viewport_creation.h"
#include "graphics/layer_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Creation/lookup/teardown only; no per-frame traversal or retained recipes. */
typedef struct ProjectInstance {
    ProjectId id;
    Entity root;
    struct { char name[ENTITY_NAME_MAX]; ViewportId id; } viewports[MAX_VIEWPORTS];
    struct { char name[ROHR_INPUT_NAME_MAX]; InputControllerId id; } controllers[ROHR_INPUT_CONTROLLER_LIMIT];
    struct { char name[ENTITY_NAME_MAX]; Sound id; } sounds[ROHR_AUDIO_SOUND_LIMIT];
    struct { char name[ENTITY_NAME_MAX]; Music id; } music[ROHR_AUDIO_MUSIC_LIMIT];
    size_t viewport_count, controller_count, sound_count, music_count;
} ProjectInstance;
static ProjectInstance *instances[ROHR_PROJECT_LIMIT];
/* Never reset or wrap: an old project ID cannot become another owner. */
static ProjectId next_id;

static ProjectInstance *instance_get(ProjectId id) {
    if(id) for(size_t i = 0; i < ROHR_PROJECT_LIMIT; ++i)
        if(instances[i] && instances[i]->id == id) return instances[i];
    return NULL;
}
static bool name_check(const char *name) {
    return name && name[0] && strlen(name) < ENTITY_NAME_MAX;
}
/* Bound the tree before walking it for prerequisite/resource resolution. The
 * shared object creator performs detailed physics/reference validation. */
static bool tree_check(const ObjectDescription *d, unsigned depth, size_t *count, bool *graphics) {
    if(!d || depth > OBJECT_DEPTH_MAX || ++*count > MAX_ENTITIES ||
            !name_check(d->name) || !d->id || d->child_count > MAX_ENTITIES ||
            (d->child_count && !d->children)) return false;
    if((d->categories & ROHR_ENTITY_OBJECT_VISUALS) &&
            (d->kind == OBJECT_CAMERA || d->kind == OBJECT_SPRITE || d->kind == OBJECT_ANIMATION))
        *graphics = true;
    for(size_t i = 0; i < d->child_count; ++i)
        if(!d->children[i] || d->children[i]->parent != d ||
                !tree_check(d->children[i], depth + 1, count, graphics)) return false;
    return true;
}
static EngineResult description_check(const ProjectDescription *d) {
    if(!d || !engine_started_check() || d->root.parent || d->root.kind != OBJECT_CONTAINER ||
            d->viewport_count > MAX_VIEWPORTS || (d->viewport_count && !d->viewports) ||
            d->controller_count > ROHR_INPUT_CONTROLLER_LIMIT || (d->controller_count && !d->controllers) ||
            d->sound_count > ROHR_AUDIO_SOUND_LIMIT || (d->sound_count && !d->sounds) ||
            d->music_count > ROHR_AUDIO_MUSIC_LIMIT || (d->music_count && !d->music))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    size_t count = 0, actions = 0;
    bool graphics = d->viewport_count != 0;
    if(!tree_check(&d->root, 0, &count, &graphics)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(graphics && !graphics_started_check()) return error_result_error(ERROR_ENGINE_GRAPHICS_NOT_INITIALIZED);
    if((d->sound_count || d->music_count) && !audio_started_check())
        return error_result_error(ERROR_ENGINE_AUDIO_NOT_STARTED);
    for(size_t i = 0; i < d->controller_count; ++i) {
        const ProjectControllerDescription *c = &d->controllers[i];
        if(!memchr(c->controller.name, 0, sizeof(c->controller.name)) || !c->controller.name[0] ||
                c->action_count > ROHR_INPUT_ACTION_LIMIT - actions || (c->action_count && !c->actions))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        actions += c->action_count;
        for(size_t j = 0; j < i; ++j)
            if(!strcmp(c->controller.name, d->controllers[j].controller.name))
                return error_result_error(ERROR_ENGINE_DUPLICATE_INPUT_NAME);
    }
#define CHECK_NAMES(field, count_field) \
    for(size_t i = 0; i < d->count_field; ++i) { \
        if(!name_check(d->field[i].name)) return error_result_error(ERROR_ENGINE_STATE_INVALID); \
        for(size_t j = 0; j < i; ++j) \
            if(!strcmp(d->field[i].name, d->field[j].name)) \
                return error_result_error(ERROR_ENGINE_STATE_INVALID); \
    }
    CHECK_NAMES(viewports, viewport_count)
    CHECK_NAMES(sounds, sound_count)
    CHECK_NAMES(music, music_count)
#undef CHECK_NAMES
    for(size_t i = 0; i < d->viewport_count; ++i) {
        const ViewportCreateDescription *v = d->viewports[i].description;
        if(!v || v->template || v->object_count || v->camera_count ||
                v->item_count > MAX_VIEWPORT_ITEMS || (v->item_count && !v->items))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
    }
    return error_result_value(true);
}
static void layers_snapshot(GraphicsLayerId *ids) {
    for(size_t i = 0; i < MAX_GRAPHICS_LAYERS; ++i) {
        LayerDescription d;
        ids[i] = graphics_layer_registry_entry_get(i, &d) ?
            graphics_layer_name_id_get(d.name).result.value : GRAPHICS_LAYER_INVALID;
    }
}
static void layers_rollback(const GraphicsLayerId *before) {
    GraphicsLayerId after[MAX_GRAPHICS_LAYERS];
    layers_snapshot(after);
    for(size_t i = 0; i < MAX_GRAPHICS_LAYERS; ++i)
        if(after[i] && after[i] != before[i]) (void)graphics_layer_destroy(after[i]);
}
static EngineResult resources_destroy(ProjectInstance *p) {
    EngineResult result = error_result_value(true), current;
#define RELEASE(call) do { current = (call); if(error_check(current) && !error_check(result)) result = current; } while(0)
    for(size_t i = p->viewport_count; i > 0; --i)
        if(!error_check(graphics_viewport_description_get(p->viewports[i - 1].id)))
            RELEASE(graphics_viewport_destroy(p->viewports[i - 1].id));
    if(entity_alive_check(p->root)) RELEASE(entity_delete(p->root));
    for(size_t i = p->controller_count; i > 0; --i)
        if(!error_check(input_controller_description_get(p->controllers[i - 1].id)))
            RELEASE(input_controller_destroy(p->controllers[i - 1].id));
    for(size_t i = p->sound_count; i > 0; --i)
        if(audio_sound_valid_check(p->sounds[i - 1].id)) RELEASE(audio_sound_destroy(&p->sounds[i - 1].id));
    for(size_t i = p->music_count; i > 0; --i)
        if(audio_music_valid_check(p->music[i - 1].id)) RELEASE(audio_music_destroy(&p->music[i - 1].id));
#undef RELEASE
    return result;
}
static ViewportIdResult viewport_create(const ProjectDescription *d, Entity root, size_t index) {
    const ViewportCreateDescription *v = d->viewports[index].description;
    ViewportObjectBinding bindings[MAX_VIEWPORT_ITEMS];
    size_t count = 0;
    for(size_t i = 0; i < v->item_count; ++i) {
        const ObjectDescription *object = v->items[i].camera_object;
        if(!object) continue; /* Malformed unmatched camera members fail in the shared creator. */
        size_t j = 0;
        while(j < count && bindings[j].description != object) ++j;
        if(j < count) continue;
        EntityResult entity = object_instance_member_description_get(root, &d->root, object);
        if(error_check(entity)) return ERROR_RESULT_MAKE_ERROR(ViewportIdResult, entity.result.error);
        bindings[count++] = (ViewportObjectBinding){object, entity.result.value};
    }
    ViewportCreateDescription recipe = *v;
    snprintf(recipe.viewport.name, sizeof(recipe.viewport.name), "%s", d->viewports[index].name);
    ViewportCreateDescription input = {.template = &recipe, .objects = bindings, .object_count = count};
    return graphics_viewport_create(input);
}
ProjectIdResult rohr_internal_project_create(const ProjectDescription *d) {
    EngineResult check = description_check(d);
    if(error_check(check)) return ERROR_RESULT_MAKE_ERROR(ProjectIdResult, check.result.error);
    size_t slot = 0;
    while(slot < ROHR_PROJECT_LIMIT && instances[slot]) ++slot;
    if(slot == ROHR_PROJECT_LIMIT || next_id == UINT64_MAX)
        return ERROR_RESULT_MAKE_ERROR(ProjectIdResult, ERROR_MEMORY_POOL_FULL);
    ProjectInstance *p = calloc(1, sizeof(*p));
    if(!p) return ERROR_RESULT_MAKE_ERROR(ProjectIdResult, ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    p->id = ++next_id;
    GraphicsLayerId layers[MAX_GRAPHICS_LAYERS];
    layers_snapshot(layers);
    EngineError failure;
#define ACQUIRE(type, variable, call) type variable = (call); \
    if(error_check(variable)) { failure = variable.result.error; goto rollback; }
    ACQUIRE(EntityResult, root, object_create(d->root));
    p->root = root.result.value;
    for(size_t i = 0; i < d->viewport_count; ++i) {
        ACQUIRE(ViewportIdResult, viewport, viewport_create(d, p->root, i));
        p->viewports[i].id = viewport.result.value;
        strcpy(p->viewports[i].name, d->viewports[i].name);
        ++p->viewport_count;
    }
    for(size_t i = 0; i < d->controller_count; ++i) {
        const ProjectControllerDescription *c = &d->controllers[i];
        ACQUIRE(InputControllerIdResult, controller, controller_create(*c));
        p->controllers[i].id = controller.result.value;
        strcpy(p->controllers[i].name, c->controller.name);
        ++p->controller_count;
    }
    for(size_t i = 0; i < d->sound_count; ++i) {
        SoundDescription description = d->sounds[i].description;
        snprintf(description.name, sizeof(description.name), "%s", d->sounds[i].name);
        ACQUIRE(SoundResult, sound, audio_sound_create(description));
        p->sounds[i].id = sound.result.value;
        strcpy(p->sounds[i].name, d->sounds[i].name);
        ++p->sound_count;
    }
    for(size_t i = 0; i < d->music_count; ++i) {
        MusicDescription description = d->music[i].description;
        snprintf(description.name, sizeof(description.name), "%s", d->music[i].name);
        ACQUIRE(MusicResult, music, audio_music_create(description));
        p->music[i].id = music.result.value;
        strcpy(p->music[i].name, d->music[i].name);
        ++p->music_count;
    }
#undef ACQUIRE
    instances[slot] = p;
    return ERROR_RESULT_MAKE_VALUE(ProjectIdResult, p->id);
rollback:
    (void)resources_destroy(p);
    layers_rollback(layers);
    free(p);
    return ERROR_RESULT_MAKE_ERROR(ProjectIdResult, failure);
}
EngineResult project_destroy(ProjectId project) {
    if(!project) return error_result_value(true);
    ProjectInstance *p = instance_get(project);
    if(!p) return error_result_error(ERROR_ENGINE_STATE_REFERENCE_NOT_FOUND);
    EngineResult result = resources_destroy(p);
    if(error_check(result)) return result; /* Keep ownership available for a retry. */
    for(size_t i = 0; i < ROHR_PROJECT_LIMIT; ++i) if(instances[i] == p) instances[i] = NULL;
    free(p);
    return result;
}
void project_instances_destroy(void) {
    for(size_t i = 0; i < ROHR_PROJECT_LIMIT; ++i) if(instances[i]) {
        (void)resources_destroy(instances[i]);
        free(instances[i]);
        instances[i] = NULL;
    }
}
EntityResult project_root_get(ProjectId project) {
    ProjectInstance *p = instance_get(project);
    if(!p || !entity_alive_check(p->root))
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_REFERENCE_NOT_FOUND);
    return ERROR_RESULT_MAKE_VALUE(EntityResult, p->root);
}
ViewportListResult project_viewports_get(ProjectId project) {
    ProjectInstance *p = instance_get(project);
    if(!p) return ERROR_RESULT_MAKE_ERROR(ViewportListResult, ERROR_ENGINE_STATE_REFERENCE_NOT_FOUND);
    ViewportList list = {.count = p->viewport_count};
    for(size_t i = 0; i < list.count; ++i) {
        if(error_check(graphics_viewport_description_get(p->viewports[i].id)))
            return ERROR_RESULT_MAKE_ERROR(ViewportListResult, ERROR_ENGINE_STATE_REFERENCE_NOT_FOUND);
        list.values[i] = p->viewports[i].id;
    }
    return ERROR_RESULT_MAKE_VALUE(ViewportListResult, list);
}
#define NAMED_GET(function, type, field, count, valid) \
    type function(ProjectId project, const char *name) { \
        ProjectInstance *p = instance_get(project); \
        if(p && name_check(name)) for(size_t i = 0; i < p->count; ++i) \
            if(!strcmp(p->field[i].name, name) && (valid)) \
                return ERROR_RESULT_MAKE_VALUE(type, p->field[i].id); \
        return ERROR_RESULT_MAKE_ERROR(type, ERROR_ENGINE_STATE_REFERENCE_NOT_FOUND); \
    }
NAMED_GET(project_viewport_by_name_get, ViewportIdResult, viewports, viewport_count,
    !error_check(graphics_viewport_description_get(p->viewports[i].id)))
NAMED_GET(project_controller_by_name_get, InputControllerIdResult, controllers, controller_count,
    !error_check(input_controller_description_get(p->controllers[i].id)))
NAMED_GET(project_sound_by_name_get, SoundResult, sounds, sound_count, audio_sound_valid_check(p->sounds[i].id))
NAMED_GET(project_music_by_name_get, MusicResult, music, music_count, audio_music_valid_check(p->music[i].id))
#undef NAMED_GET
