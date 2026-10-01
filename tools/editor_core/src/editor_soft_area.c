/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_soft_area.h"
#include "editor_array.h"
#include "math/node_loop_geometry.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct EditorSoftAreaCache {
    NodeLoopPoints poses[NODE_LOOP_MAX_LOOPS];
    size_t loop_count;
    NodeLoopFill fill;
    bool initialized;
};

EditorSoftBody *editor_soft_body_get(EditorObject *object, EditorSoftBodyId id) {
    if(object != NULL) for(size_t i = 0; i < object->soft_body_count; i += 1)
        if(object->soft_body_items[i].id == id) return &object->soft_body_items[i];
    return NULL;
}
EditorSoftArea *editor_soft_area_get(EditorSoftBody *body, EditorSoftAreaId id) {
    if(body != NULL) for(size_t i = 0; i < body->area_count; i += 1)
        if(body->areas[i].id == id) return &body->areas[i];
    return NULL;
}
EditorSoftHole *editor_soft_hole_get(EditorSoftArea *area, EditorSoftHoleId id) {
    if(area != NULL) for(uint32_t i = 0; i < area->hole_count; i += 1)
        if(area->holes[i].id == id) return &area->holes[i];
    return NULL;
}
bool editor_soft_hole_remove(EditorSoftArea *area, EditorSoftHoleId id) {
    EditorSoftHole *hole = editor_soft_hole_get(area, id);
    if(hole == NULL) return false;
    size_t index = (size_t)(hole - area->holes);
    memmove(hole, hole + 1, (area->hole_count - index - 1) * sizeof(*hole));
    area->holes[--area->hole_count] = (EditorSoftHole){0};
    return true;
}
bool editor_soft_area_loop_set(EditorSoftBody *body, EditorSoftAreaId id,
        EditorSoftHoleId hole_id, EditorSoftAreaLoop loop) {
    EditorSoftArea *area = editor_soft_area_get(body, id);
    EditorSoftHole *hole = editor_soft_hole_get(area, hole_id);
    if(area == NULL || (hole_id != 0 && hole == NULL)) return false;
    EditorSoftArea candidate = *area;
    if(hole_id == 0) candidate.outer = loop;
    else candidate.holes[hole - area->holes].loop = loop;
    if(!editor_soft_area_references_check(body, &candidate)) return false;
    if(hole_id == 0) area->outer = loop;
    else hole->loop = loop;
    return true;
}
bool editor_soft_area_order_set(EditorSoftBody *body, EditorSoftAreaId id,
        size_t index) {
    EditorSoftArea *area = editor_soft_area_get(body, id);
    if(area == NULL || index >= body->area_count) return false;
    size_t old = (size_t)(area - body->areas);
    EditorSoftArea value = *area;
    if(old < index) memmove(area, area + 1, (index - old) * sizeof(*area));
    else if(index < old) memmove(&body->areas[index + 1], &body->areas[index],
        (old - index) * sizeof(*area));
    body->areas[index] = value;
    return true;
}

EditorSoftArea *editor_soft_area_add(EditorSoftBody *body) {
    if(body == NULL || body->area_count >= SOFT_BODY_MAX_AREAS ||
            body->next_area_id == UINT32_MAX ||
            !EDITOR_ARRAY_RESERVE(body->areas, body->area_capacity,
                body->area_count + 1)) return NULL;
    if(body->next_area_id == 0) body->next_area_id = 1;
    EditorSoftArea *area = &body->areas[body->area_count++];
    *area = (EditorSoftArea){.id = body->next_area_id++, .next_hole_id = 1,
        .visible = true, .graphics_layer_inherited = true,
        .color = UINT32_C(0x468cdcff)};
    snprintf(area->name, sizeof(area->name), "area_%u", area->id);
    return area;
}

EditorSoftHole *editor_soft_hole_add(EditorSoftArea *area) {
    if(area == NULL || area->hole_count >= SOFT_BODY_MAX_AREA_HOLES ||
            area->next_hole_id == UINT32_MAX) return NULL;
    if(area->next_hole_id == 0) area->next_hole_id = 1;
    EditorSoftHole *hole = &area->holes[area->hole_count++];
    *hole = (EditorSoftHole){.id = area->next_hole_id++};
    snprintf(hole->name, sizeof(hole->name), "hole_%u", hole->id);
    return hole;
}

void editor_soft_area_cache_destroy(EditorSoftArea *area) {
    if(area == NULL || area->cache == NULL) return;
    node_loop_fill_destroy(&area->cache->fill);
    free(area->cache);
    area->cache = NULL;
}

bool editor_soft_area_remove(EditorSoftBody *body, EditorSoftAreaId id) {
    if(body == NULL) return false;
    for(size_t i = 0; i < body->area_count; i += 1) {
        if(body->areas[i].id != id) continue;
        editor_soft_area_cache_destroy(&body->areas[i]);
        memmove(&body->areas[i], &body->areas[i + 1],
            (body->area_count - i - 1) * sizeof(*body->areas));
        body->areas[--body->area_count] = (EditorSoftArea){0};
        return true;
    }
    return false;
}

static const EditorSoftNode *node_get(const EditorSoftBody *body, EditorSoftNodeId id) {
    for(size_t i = 0; i < body->node_count; i += 1)
        if(body->nodes[i].id == id) return &body->nodes[i];
    return NULL;
}

bool editor_soft_area_references_check(const EditorSoftBody *body,
        const EditorSoftArea *area) {
    if(body == NULL || area == NULL || area->hole_count > SOFT_BODY_MAX_AREA_HOLES)
        return false;
    for(uint32_t i = 0; i <= area->hole_count; i += 1) {
        const EditorSoftAreaLoop *loop = i == 0 ? &area->outer : &area->holes[i - 1].loop;
        if(loop->node_count > SOFT_BODY_MAX_NODES) return false;
        for(uint32_t j = 0; j < loop->node_count; j += 1) {
            if(node_get(body, loop->nodes[j]) == NULL) return false;
            for(uint32_t k = 0; k < j; k += 1)
                if(loop->nodes[j] == loop->nodes[k]) return false;
        }
    }
    return true;
}

bool editor_soft_area_node_check(const EditorSoftArea *area, EditorSoftNodeId id) {
    if(area == NULL) return false;
    for(uint32_t i = 0; i <= area->hole_count; i += 1) {
        const EditorSoftAreaLoop *loop = i == 0 ? &area->outer : &area->holes[i - 1].loop;
        for(uint32_t j = 0; j < loop->node_count; j += 1)
            if(loop->nodes[j] == id) return true;
    }
    return false;
}

EditorSoftAreaFill editor_soft_area_fill_get(const EditorSoftBody *body,
        EditorSoftArea *area) {
    EditorSoftAreaFill result = {0};
    NodeLoopPoints poses[NODE_LOOP_MAX_LOOPS] = {0};
    if(!editor_soft_area_references_check(body, area)) return result;
    for(uint32_t i = 0; i <= area->hole_count; i += 1) {
        const EditorSoftAreaLoop *loop = i == 0 ? &area->outer : &area->holes[i - 1].loop;
        if(loop->node_count < 3) { result.incomplete_loop = i; return result; }
        poses[i].count = loop->node_count;
        for(uint32_t j = 0; j < loop->node_count; j += 1)
            poses[i].points[j] = node_get(body, loop->nodes[j])->position;
    }
    if(area->cache == NULL) area->cache = calloc(1, sizeof(*area->cache));
    EditorSoftAreaCache *cache = area->cache;
    if(cache == NULL) return result;
    size_t count = area->hole_count + 1;
    if(!cache->initialized || cache->loop_count != count ||
            memcmp(poses, cache->poses, sizeof(poses)) != 0) {
        NodeLoopWorkspace *workspace = node_loop_workspace_create();
        if(workspace == NULL) return result;
        NodeLoopGeometryStatus status = node_loop_fill_build(poses, count,
            workspace, &cache->fill);
        node_loop_workspace_destroy(workspace);
        if(status != NODE_LOOP_GEOMETRY_OK) return result;
        memcpy(cache->poses, poses, sizeof(poses));
        cache->loop_count = count;
        cache->initialized = true;
    }
    for(uint32_t i = 0; i < count; i += 1)
        if((cache->fill.enclosed_loops & (UINT32_C(1) << i)) == 0) {
            result.incomplete_loop = i;
            return result;
        }
    result.complete = true;
    result.vertices = cache->fill.vertices;
    result.vertex_count = cache->fill.vertex_count;
    return result;
}

bool editor_soft_area_point_check(const EditorSoftBody *body,
        EditorSoftArea *area, Position local) {
    if(area == NULL || !area->visible || (area->color & 255) == 0) return false;
    EditorSoftAreaFill fill = editor_soft_area_fill_get(body, area);
    for(size_t i = 0; i < fill.vertex_count; i += 3) {
        bool positive = false, negative = false;
        for(size_t j = 0; j < 3; j += 1) {
            Vec2D a = fill.vertices[i + j], b = fill.vertices[i + (j + 1) % 3];
            double cross = ((double)b.x - a.x) * ((double)local.y - a.y) -
                ((double)b.y - a.y) * ((double)local.x - a.x);
            positive |= cross > 0; negative |= cross < 0;
        }
        if(!(positive && negative)) return true;
    }
    return false;
}

EditorResult editor_soft_areas_generation_validate(const EditorProject *project) {
    if(project == NULL) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "Area validation requires a project");
    for(size_t o = 0; o < project->object_count; o += 1) {
        const EditorObject *object = &project->objects[o];
        for(size_t b = 0; b < object->soft_body_count; b += 1) {
            const EditorSoftBody *body = &object->soft_body_items[b];
            for(size_t a = 0; a < body->area_count; a += 1) {
                EditorSoftArea *area = &body->areas[a];
                EditorSoftAreaFill fill = editor_soft_area_fill_get(body, area);
                if(!fill.complete) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                    "Cannot generate %s / %s / %s / %s: incomplete or invalid loop; "
                    "select at least three distinct nodes enclosing a region",
                    object->name, body->name, area->name, fill.incomplete_loop == 0 ?
                        "outer" : area->holes[fill.incomplete_loop - 1].name);
            }
        }
    }
    return editor_result_value(true);
}
