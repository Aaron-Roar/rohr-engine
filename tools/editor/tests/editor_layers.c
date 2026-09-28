/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_viewport.h"
#include "editor_layout.h"
#include <limits.h>
#include <stdio.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "editor layers line %d: %s\n", \
    __LINE__, #c); return false; } } while(0)
#define OK(c) do { EngineResult result = (c); CHECK(!rohr_error_check(result)); } while(0)

static bool pixel_check(Position point, Color color) {
    int count = 0;
    SDL_Window **windows = SDL_GetWindows(&count);
    CHECK(windows != NULL && count > 0);
    SDL_Renderer *renderer = SDL_GetRenderer(windows[0]);
    SDL_free(windows);
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    CHECK(surface != NULL);
    Color found = {0};
    bool read = SDL_ReadSurfacePixel(surface, (int)point.x, (int)point.y,
        &found.red, &found.green, &found.blue, &found.alpha);
    SDL_DestroySurface(surface);
    CHECK(read);
    if(found.red != color.red || found.green != color.green || found.blue != color.blue) {
        fprintf(stderr, "pixel %.0f,%.0f: %u,%u,%u expected %u,%u,%u\n", point.x,
            point.y, found.red, found.green, found.blue, color.red, color.green, color.blue);
        return false;
    }
    return true;
}

static void draw(EditorProject *project, EditorViewportState *state) {
    rohr_graphics_layer_active_set(EDITOR_GRAPHICS_LAYER_BACKGROUND);
    rohr_graphics_background_draw((Color){0,0,0,255});
    editor_viewport_draw(project, state, false);
    rohr_graphics_show();
}

static bool body_layers_check(void) {
    EditorProject project;
    EditorViewportState state = {0};
    EditorSelectionRef hit;
    editor_project_init(&project);
    editor_viewport_state_init(&state);
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    CHECK(object != NULL);
    CHECK(editor_project_rigid_body_add(&project, object) != NULL);
    CHECK(editor_project_rigid_body_add(&project, object) != NULL);
    EditorRigidBody *a = &object->rigid_bodies[0], *b = &object->rigid_bodies[1];
    const Position vertices[] = {{-60,-50},{60,-50},{0,70}};
    for(size_t i = 0; i < 3; i += 1) {
        a->hitboxes[0].vertices[i].position = vertices[i];
        b->hitboxes[0].vertices[i].position = vertices[i];
    }
    a->surface_color = UINT32_C(0xff0000ff);
    b->surface_color = UINT32_C(0x0000ffff);
    a->graphics_layer.value = INT_MAX;
    b->graphics_layer.value = INT_MIN;
    state.mode = EDITOR_VIEWPORT_OBJECT;
    project.viewport_local_view = false;
    project.viewport_camera_zoom = 1;
    Position center = {EDITOR_VIEWPORT_WIDTH * .5f,
        EDITOR_MENU_HEIGHT + (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * .5f};
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit));
    CHECK(hit.kind == EDITOR_SELECTION_RIGID_BODY && hit.item == a->id);
    draw(&project, &state);
    CHECK(pixel_check((Position){center.x + 20, center.y}, (Color){255,0,0,255}));
    /* A selected lower body must not retain clicks through the higher layer. */
    state.selection = EDITOR_SELECTION_RIGID_BODY;
    state.selected_rigid_body = b->id;
    CHECK(editor_viewport_update(&state, &project, center,
        MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP, false, 0, false));
    CHECK(state.selected_rigid_body == a->id && state.dragged_body);
    (void)editor_viewport_update(&state, &project, center,
        MOUSE_BUTTON_STATE_RELEASED, MOUSE_BUTTON_STATE_UP, false, 0, false);
    editor_viewport_selection_clear(&state);
    state.mode = EDITOR_VIEWPORT_OBJECT;

    EditorGraphicsLayer *named = editor_project_graphics_layer_add(&project, "front", 25);
    CHECK(named != NULL);
    a->graphics_layer = (EditorGraphicsLayerBinding){.value=-100, .layer=named->id};
    b->graphics_layer.value = 10;
    draw(&project, &state);
    CHECK(pixel_check((Position){center.x + 20, center.y}, (Color){255,0,0,255}));
    CHECK(editor_project_graphics_layer_set(&project, named->id, "front", -25));
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit) && hit.item == b->id);
    draw(&project, &state);
    CHECK(pixel_check((Position){center.x + 20, center.y}, (Color){0,0,255,255}));
    CHECK(editor_project_graphics_layer_set(&project, named->id, "front", 10));
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit) && hit.item == b->id);
    b->visible = false;
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit) && hit.item == a->id);
    draw(&project, &state);
    CHECK(pixel_check((Position){center.x + 20, center.y}, (Color){255,0,0,255}));
    b->visible = true;

    EditorSoftBody *soft = editor_project_soft_body_add(&project, object);
    CHECK(soft != NULL);
    EditorSoftNode *node = editor_project_soft_node_add(&project, soft, (Position){0});
    CHECK(node != NULL);
    soft->graphics_layer.value = 20;
    node->graphics_layer_inherited = true;
    node->color_overridden = true;
    node->color = UINT32_C(0x00ff00ff);
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit));
    CHECK(hit.kind == EDITOR_SELECTION_SOFT_BODY && hit.item == soft->id);
    draw(&project, &state);
    CHECK(pixel_check(center, (Color){0,255,0,255}));
    node->graphics_layer_inherited = false;
    node->graphics_layer.value = -20;
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit) && hit.item == b->id);
    node->graphics_layer = a->graphics_layer;
    CHECK(editor_project_graphics_layer_set(&project, named->id, "front", 30));
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit));
    CHECK(hit.kind == EDITOR_SELECTION_SOFT_BODY && hit.item == soft->id);
    soft->visible = false;
    CHECK(editor_project_graphics_layer_set(&project, named->id, "front", 10));

    EditorSprite *sprite = editor_project_sprite_add(&project, object, "layered", "unused.png");
    CHECK(sprite != NULL);
    sprite->size = (Scale){100,100};
    sprite->graphics_layer.value = -1;
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit) && hit.item == b->id);
    sprite->graphics_layer.value = 11;
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit));
    CHECK(hit.kind == EDITOR_SELECTION_SPRITE && hit.item == sprite->id);
    EditorAnimatedSprite *animation = editor_project_animated_sprite_add(&project, object);
    CHECK(animation != NULL);
    CHECK(editor_project_animation_frame_add(&project, animation, "frame", "unused.png", (Scale){100,100}));
    animation->graphics_layer.value = 12;
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit));
    CHECK(hit.kind == EDITOR_SELECTION_ANIMATED_SPRITE && hit.item == animation->id);
    animation->graphics_layer.value = 9;
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit));
    CHECK(hit.kind == EDITOR_SELECTION_SPRITE && hit.item == sprite->id);
    sprite->visible = false;
    animation->visible = false;

    /* Camera previews retain authored scene layers and keep editor overlays on top. */
    EditorCamera *camera = editor_project_camera_add(&project, object);
    CHECK(camera != NULL);
    camera->dimensions = (Scale){200,200};
    EditorLayoutViewport *viewport = editor_project_layout_viewport_add(&project);
    CHECK(viewport != NULL);
    viewport->config.rectangle = (ViewportRectangle){0,0,200,200};
    EditorViewportCameraItem *screen = editor_viewport_camera_add(&project, viewport,
        object->id, camera->id);
    CHECK(screen != NULL);
    screen->object = object->id; screen->camera = camera->id;
    screen->placement.rectangle = (ViewportRectangle){0,0,200,200};
    screen->placement.visible = true;
    state.mode = EDITOR_VIEWPORT_LAYOUT;
    state.selected_layout_viewport = viewport->id;
    Position preview_center = {center.x+100, center.y+100};
    draw(&project, &state);
    CHECK(pixel_check(preview_center, (Color){0,0,255,255}));
    a->graphics_layer = (EditorGraphicsLayerBinding){.value=INT_MAX};
    draw(&project, &state);
    CHECK(pixel_check(preview_center, (Color){255,0,0,255}));

    /* Editor menus remain above even the largest authored scene layer. */
    rohr_graphics_layer_active_set(EDITOR_GRAPHICS_LAYER_TOP_MENU);
    (void)rohr_graphics_screen_rect_draw(preview_center.x - 3, preview_center.y - 3,
        6, 6, (Color){0,255,0,255});
    draw(&project, &state);
    CHECK(pixel_check(preview_center, (Color){0,255,0,255}));

    editor_viewport_state_destroy(&state);
    editor_project_destroy(&project);
    return true;
}

bool editor_layers_check(void) {
    OK(rohr_engine_start());
    OK(rohr_graphics_start());
    ViewportConfig config = rohr_viewport_config_default_get();
    ViewportIdResult viewport = rohr_viewport_create(config);
    CHECK(!rohr_error_check(viewport));
    OK(rohr_viewport_camera_clear(viewport.result.value));
    OK(rohr_viewport_disable_set(viewport.result.value));
    bool passed = body_layers_check();
    editor_viewport_assets_destroy();
    rohr_graphics_stop();
    rohr_engine_stop();
    return passed;
}
