/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_viewport.h"
#include "editor_layout.h"
#include "editor_command.h"
#include "editor_mass_properties.h"
#include "editor_soft_area.h"
#include <limits.h>
#include <math.h>
#include "../../../tests/test_png.h"
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
    CHECK(editor_project_animation_frame_add(&project, animation, "frame", "editor_layers_frame.png", (Scale){100,100}));
    animation->graphics_layer.value = 12;
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit));
    CHECK(hit.kind == EDITOR_SELECTION_ANIMATED_SPRITE && hit.item == animation->id);
    animation->graphics_layer.value = 9;
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit));
    CHECK(hit.kind == EDITOR_SELECTION_SPRITE && hit.item == sprite->id);
    sprite->visible = false;
    animation->visible = false;

    /* COM overlays the largest scene layer; automatic selection is yellow too. */
    EditorGraphicsLayerBinding previous_a = a->graphics_layer, previous_b = b->graphics_layer;
    a->graphics_layer = (EditorGraphicsLayerBinding){.value=INT_MAX};
    b->graphics_layer = (EditorGraphicsLayerBinding){.value=INT_MIN};
    state.mode = EDITOR_VIEWPORT_RIGID_BODY;
    state.selected_rigid_body = b->id;
    for(int explicit = 0; explicit < 2; explicit += 1) {
        b->center_of_mass_explicit = explicit != 0;
        EditorMassProperties properties = editor_mass_properties_get(b);
        CHECK(properties.center_available);
        Position marker = {center.x + properties.center.x + 2, center.y - properties.center.y - 4};
        state.selection = EDITOR_SELECTION_RIGID_BODY;
        draw(&project, &state);
        CHECK(pixel_check(marker, (Color){45,140,255,255}));
        state.selection = EDITOR_SELECTION_CENTER_OF_MASS;
        draw(&project, &state);
        CHECK(pixel_check(marker, (Color){255,215,70,255}));
        rohr_graphics_layer_active_set(EDITOR_GRAPHICS_LAYER_TOP_MENU);
        (void)rohr_graphics_screen_rect_draw(marker.x-2, marker.y-2, 4, 4, (Color){0,255,0,255});
        draw(&project, &state);
        CHECK(pixel_check(marker, (Color){0,255,0,255}));
    }
    b->center_of_mass_explicit = false;
    a->graphics_layer = previous_a;
    b->graphics_layer = previous_b;
    editor_viewport_selection_clear(&state);

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

static bool direction_pixels_check(Position center, float rotation,
        Direction direction, unsigned allowed_frames, unsigned *seen) {
    const Color colors[2][2] = {{{255,0,0,255}, {0,0,255,255}},
        {{0,255,0,255}, {255,255,0,255}}};
    int count = 0;
    SDL_Window **windows = SDL_GetWindows(&count);
    CHECK(windows != NULL && count > 0);
    SDL_Renderer *renderer = SDL_GetRenderer(windows[0]);
    SDL_free(windows);
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    CHECK(surface != NULL);
    Color found[2] = {0};
    float angle = math_degrees_to_radians(rotation);
    bool read = true;
    for(size_t side = 0; side < 2; side += 1) {
        float distance = side == 0 ? -16 : 16;
        read = SDL_ReadSurfacePixel(surface,
            (int)roundf(center.x + cosf(angle) * distance),
            (int)roundf(center.y + sinf(angle) * distance),
            &found[side].red, &found[side].green, &found[side].blue,
            &found[side].alpha) && read;
    }
    SDL_DestroySurface(surface);
    CHECK(read);
    for(size_t frame = 0; frame < 2; frame += 1) {
        if(!(allowed_frames & (1u << frame))) continue;
        bool matched = true;
        for(size_t side = 0; side < 2; side += 1) {
            Color expected = colors[frame][direction == DIRECTION_LEFT ? 1 - side : side];
            matched = matched && found[side].red == expected.red &&
                found[side].green == expected.green && found[side].blue == expected.blue;
        }
        if(matched) { *seen |= 1u << frame; return true; }
    }
    fprintf(stderr, "animation direction %d at %.0f,%.0f rotation %.0f: %u,%u,%u / %u,%u,%u\n",
        direction, center.x, center.y, rotation, found[0].red, found[0].green,
        found[0].blue, found[1].red, found[1].green, found[1].blue);
    return false;
}

static bool animation_extent_check(Position center, float rotation, Scale size,
        Color color) {
    /* The center stays fixed while each scaled edge moves outward/inward. */
    Vec2D samples[] = {{size.x * .5f - 4, 0}, {size.x * .5f + 4, 0},
        {size.x * .25f, size.y * .5f - 4},
        {size.x * .25f, size.y * .5f + 4}};
    for(size_t i = 0; i < 4; i += 1) {
        Vec2D point = math_vector_rotate(samples[i], -rotation);
        CHECK(pixel_check((Position){center.x + point.x, center.y + point.y},
            i % 2 == 0 ? color : (Color){0,0,0,255}));
    }
    return true;
}

static bool animation_direction_check(void) {
    const char *paths[] = {"editor_direction_first.png", "editor_direction_second.png"};
    const Uint32 colors[2][2] = {{0xff0000ff, 0x0000ffff}, {0x00ff00ff, 0xffff00ff}};
    const Scale scales[] = {{1,1}, {2,3}, {.5f,1.5f}};
    for(size_t frame = 0; frame < 2; frame += 1) {
        SDL_Surface *surface = SDL_CreateSurface(8, 8, SDL_PIXELFORMAT_RGBA8888);
        CHECK(surface != NULL);
        SDL_Rect left = {0,0,4,8}, right = {4,0,4,8};
        bool saved = SDL_FillSurfaceRect(surface, &left, colors[frame][0]) &&
            SDL_FillSurfaceRect(surface, &right, colors[frame][1]) &&
            SDL_SavePNG(surface, paths[frame]);
        SDL_DestroySurface(surface);
        CHECK(saved);
    }
    /* Check the app's draw path using the same asymmetric assets. */
    AnimationDescriptor descriptor = {.amount_of_descriptors = 2,
        .frame_files = {paths[0], paths[1]}};
    AnimationAssetResult asset = rohr_graphics_animation_load(descriptor);
    CHECK(!rohr_error_check(asset));
    CameraResult camera = rohr_camera_get(rohr_camera_active_get());
    CHECK(!rohr_error_check(camera));
    camera.result.value.position = (Position){0};
    camera.result.value.orientation = 0;
    camera.result.value.zoom = 1;
    camera.result.value.dimensions = (Vec2D){1280,720};
    OK(rohr_camera_set(rohr_camera_active_get(), camera.result.value));
    for(int left = 0; left < 2; left += 1) {
        AnimatedSprite sprite = rohr_graphics_animated_sprite_create(asset.result.value,
            (Scale){1,1});
        for(size_t f = 0; f < sprite.frame_count; f += 1) sprite.frames[f].scale = (Scale){10,8};
        sprite.direction = left ? DIRECTION_LEFT : DIRECTION_RIGHT;
        EntityResult entity = rohr_entity_add();
        CHECK(!rohr_error_check(entity));
        OK(rohr_graphics_animated_sprite_add(entity.result.value, sprite));
        rohr_graphics_layer_active_set(EDITOR_GRAPHICS_LAYER_BACKGROUND);
        rohr_graphics_background_draw((Color){0,0,0,255});
        rohr_graphics_layer_active_set(EDITOR_GRAPHICS_LAYER_ANIMATION);
        CHECK(rohr_graphics_animated_sprite_draw(entity.result.value));
        rohr_graphics_show();
        unsigned seen = 0;
        CHECK(direction_pixels_check((Position){640,360}, 0, sprite.direction, 1, &seen));
        OK(rohr_entity_delete(entity.result.value));
    }
    OK(rohr_graphics_animation_release(&asset.result.value));
    asset = rohr_graphics_animation_load(descriptor);
    CHECK(!rohr_error_check(asset));
    for(size_t scale_index = 0; scale_index < 3; scale_index += 1)
    for(int left = 0; left < 2; left += 1) {
        AnimatedSprite sprite = rohr_graphics_animated_sprite_create(asset.result.value,
            scales[scale_index]);
        sprite.frames[0].offset = (Position){30,20};
        sprite.frames[0].rotation = 45;
        sprite.orientation_offset = 90;
        for(size_t f = 0; f < sprite.frame_count; f += 1) sprite.frames[f].scale = (Scale){10,8};
        sprite.direction = left ? DIRECTION_LEFT : DIRECTION_RIGHT;
        EntityResult entity = rohr_entity_add();
        CHECK(!rohr_error_check(entity));
        OK(rohr_graphics_animated_sprite_add(entity.result.value, sprite));
        rohr_graphics_layer_active_set(EDITOR_GRAPHICS_LAYER_BACKGROUND);
        rohr_graphics_background_draw((Color){0,0,0,255});
        rohr_graphics_layer_active_set(EDITOR_GRAPHICS_LAYER_ANIMATION);
        CHECK(rohr_graphics_animated_sprite_draw(entity.result.value));
        rohr_graphics_show();
        unsigned seen = 0;
        CHECK(direction_pixels_check((Position){660,390}, 135, sprite.direction, 1, &seen));
        CHECK(animation_extent_check((Position){660,390}, 135,
            (Scale){80 * scales[scale_index].x, 64 * scales[scale_index].y},
            left ? (Color){255,0,0,255} : (Color){0,0,255,255}));
        OK(rohr_entity_delete(entity.result.value));
    }
    OK(rohr_graphics_animation_release(&asset.result.value));

    EditorProject project;
    EditorViewportState state = {.mode = EDITOR_VIEWPORT_OBJECT};
    editor_project_init(&project);
    project.viewport_local_view = false;
    project.viewport_camera_zoom = 1;
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    CHECK(object != NULL);
    EditorAnimatedSprite *animation = editor_project_animated_sprite_add(&project, object);
    CHECK(animation != NULL);
    for(size_t frame = 0; frame < 2; frame += 1)
        CHECK(editor_project_animation_frame_add(&project, animation, "frame",
            paths[frame], (Scale){10,8}));
    animation->ticks_per_frame = 1;
    animation->time_per_frame = 0;
    Position center = {EDITOR_VIEWPORT_WIDTH * .5f,
        EDITOR_MENU_HEIGHT + (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * .5f};
    EditorCamera *preview_camera = editor_project_camera_add(&project, object);
    CHECK(preview_camera != NULL);
    preview_camera->dimensions = (Scale){200,200};
    EditorLayoutViewport *viewport = editor_project_layout_viewport_add(&project);
    CHECK(viewport != NULL);
    viewport->config.rectangle = (ViewportRectangle){0,0,420,240};
    for(size_t screen_index = 0; screen_index < 2; screen_index += 1) {
        EditorViewportCameraItem *screen = editor_viewport_camera_add(&project, viewport,
            object->id, preview_camera->id);
        CHECK(screen != NULL);
        screen->placement.rectangle = screen_index == 0 ?
            (ViewportRectangle){0,0,200,160} : (ViewportRectangle){220,20,180,200};
        screen->placement.orientation = screen_index == 0 ? 0 : 90;
        screen->placement.fit = SCREEN_FIT_STRETCH;
        screen->placement.visible = true;
    }
    for(int preview = 0; preview < 3; preview += 1) {
        state.mode = preview == 2 ? EDITOR_VIEWPORT_LAYOUT :
            preview == 1 ? EDITOR_VIEWPORT_ANIMATION_FRAME : EDITOR_VIEWPORT_OBJECT;
        state.selected_layout_viewport = viewport->id;
        state.selected_animated_sprite = animation->id;
        state.selected_animation_frame = animation->frames[0].id;
        for(int left = 0; left < 2; left += 1) {
            EditorCommand command = {.type = EDITOR_COMMAND_ANIMATED_SPRITE_DIRECTION_SET,
                .data.animated_sprite_direction_set = {object->id, animation->id,
                    left ? DIRECTION_LEFT : DIRECTION_RIGHT}};
            CHECK(editor_command_execute(&project, &command).kind == ERROR_RESULT_VALUE);
            animation->editor_rotation = preview == 1 ? 90 : 0;
            animation->playing = false;
            for(int playing = 0; playing < 2; playing += 1) {
                unsigned seen = 0;
                animation->playing = playing != 0;
                for(int frame = 0; frame < 3; frame += 1) {
                    draw(&project, &state);
                    if(preview == 2) {
                        CHECK(direction_pixels_check((Position){center.x+100, center.y+80},
                            0, animation->direction, playing ? 3 : 1, &seen));
                        CHECK(direction_pixels_check((Position){center.x+310, center.y+120},
                            90, animation->direction, playing ? 3 : 1, &seen));
                    } else CHECK(direction_pixels_check(center, animation->editor_rotation,
                        animation->direction, playing && preview != 1 ? 3 : 1, &seen));
                }
                CHECK(seen == (playing && preview != 1 ? 3u : 1u));
            }
        }
    }
    /* Selecting a later frame freezes its preview and uses that frame's alignment. */
    state.mode = EDITOR_VIEWPORT_ANIMATION_FRAME;
    state.selection = EDITOR_SELECTION_ANIMATION_FRAME;
    state.selected_animation_frame = animation->frames[1].id;
    animation->frames[1].offset = (Position){30,20};
    animation->frames[1].rotation = 45;
    animation->editor_rotation = 90;
    animation->playing = true;
    for(size_t scale_index = 0; scale_index < 3; scale_index += 1)
    for(int left = 0; left < 2; left += 1) {
        EditorCommand scale_command = {.type = EDITOR_COMMAND_ANIMATED_SPRITE_SCALE_SET,
            .data.animated_sprite_scale_set = {object->id, animation->id, scales[scale_index]}};
        CHECK(editor_command_execute(&project, &scale_command).kind == ERROR_RESULT_VALUE);
        CHECK(animation->frames[1].offset.x == 30 && animation->frames[1].offset.y == 20);
        CHECK(animation->frames[1].scale.x == 10 && animation->frames[1].scale.y == 8);
        animation->direction = left ? DIRECTION_LEFT : DIRECTION_RIGHT;
        for(int repeat = 0; repeat < 3; repeat += 1) {
            draw(&project, &state);
            unsigned seen = 0;
            CHECK(direction_pixels_check((Position){center.x+20,center.y+30}, 135,
                animation->direction, 2, &seen));
            CHECK(animation_extent_check((Position){center.x+20,center.y+30}, 135,
                (Scale){80 * scales[scale_index].x, 64 * scales[scale_index].y},
                left ? (Color){0,255,0,255} : (Color){255,255,0,255}));
        }
    }
    animation->scale = (Scale){1,1};
    animation->editor_rotation = 0;
    animation->playing = false;
    animation->frames[0].offset = (Position){20,10};
    animation->frames[0].rotation = 90;
    state.mode = EDITOR_VIEWPORT_LAYOUT;
    state.selection = EDITOR_SELECTION_NONE;
    for(size_t scale_index = 0; scale_index < 3; scale_index += 1)
    for(int left = 0; left < 2; left += 1) {
        animation->scale = scales[scale_index];
        animation->direction = left ? DIRECTION_LEFT : DIRECTION_RIGHT;
        draw(&project, &state);
        unsigned seen = 0;
        CHECK(direction_pixels_check((Position){center.x+120,center.y+72}, 90,
            animation->direction, 1, &seen));
        CHECK(direction_pixels_check((Position){center.x+320,center.y+138}, 180,
            animation->direction, 1, &seen));
    }
    editor_viewport_assets_destroy();
    editor_viewport_state_destroy(&state);
    editor_project_destroy(&project);
    for(size_t i = 0; i < 2; i += 1) CHECK(SDL_RemovePath(paths[i]));
    return true;
}

static bool frame_flips_check(void) {
    const char *path = "editor_frame_flips.png";
    const Uint32 pixels[] = {0xff0000ff,0x00ff00ff,0x0000ffff,0xffff00ff};
    const Color colors[] = {{255,0,0,255},{0,255,0,255},{0,0,255,255},{255,255,0,255}};
    SDL_Surface *surface = SDL_CreateSurface(8, 8, SDL_PIXELFORMAT_RGBA8888);
    CHECK(surface != NULL);
    for(int q = 0; q < 4; q += 1) {
        SDL_Rect quadrant = {(q % 2) * 4, (q / 2) * 4, 4, 4};
        CHECK(SDL_FillSurfaceRect(surface, &quadrant, pixels[q]));
    }
    CHECK(SDL_SavePNG(surface, path));
    SDL_DestroySurface(surface);
    TextureAssetResult image = rohr_graphics_texture_load((TextureDescriptor){.file = path});
    CHECK(!rohr_error_check(image));
    AnimatedSprite runtime = rohr_graphics_animated_sprite_create((AnimationAsset){0}, (Scale){1,1});
    AnimationFrame frame = rohr_graphics_animation_frame_create(image.result.value);
    frame.offset = (Position){30,20};
    frame.rotation = 90;
    OK(rohr_graphics_animated_sprite_value_frame_add(&runtime, frame));
    Entity entity = rohr_entity_add().result.value;
    OK(rohr_graphics_animated_sprite_add(entity, runtime));
    OK(rohr_graphics_texture_release(&image.result.value));
    EditorProject project;
    EditorViewportState state = {0};
    editor_project_init(&project);
    editor_viewport_state_init(&state);
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    EditorAnimatedSprite *animation = editor_project_animated_sprite_add(&project, object);
    CHECK(animation != NULL);
    CHECK(editor_project_animation_frame_add(&project, animation, "frame", path, (Scale){1,1}));
    animation->frames[0].offset = frame.offset;
    animation->frames[0].rotation = frame.rotation;
    animation->playing = false;
    state.mode = EDITOR_VIEWPORT_OBJECT;
    project.viewport_local_view = false;
    project.viewport_camera_zoom = 1;
    for(int app = 0; app < 2; app += 1) {
        Position center = app ? (Position){670,340} :
            (Position){EDITOR_VIEWPORT_WIDTH*.5f+30,
                EDITOR_MENU_HEIGHT+(EDITOR_VIEWPORT_BOTTOM-EDITOR_MENU_HEIGHT)*.5f-20};
        for(int horizontal = 0; horizontal < 2; horizontal += 1)
        for(int vertical = 0; vertical < 2; vertical += 1)
        for(int left = 0; left < 2; left += 1) {
            Scale scale = {horizontal ? -10 : 10, vertical ? -10 : 10};
            Direction direction = left ? DIRECTION_LEFT : DIRECTION_RIGHT;
            if(app) {
                OK(rohr_graphics_animated_sprite_frame_scale_set(entity, 0, scale));
                animated_sprites[rohr_entity_index_get(entity).result.value].direction = direction;
                rohr_graphics_background_draw((Color){0,0,0,255});
                CHECK(rohr_graphics_animated_sprite_draw(entity));
                rohr_graphics_show();
            } else {
                animation->frames[0].scale = scale;
                animation->direction = direction;
                draw(&project, &state);
                EditorSelectionRef selection;
                CHECK(editor_viewport_selection_at_get(&project, &state,
                    (Position){center.x+20,center.y+20}, &selection));
                CHECK(selection.kind == EDITOR_SELECTION_ANIMATED_SPRITE);
            }
            for(int q = 0; q < 4; q += 1) {
                int x = q % 2, y = q / 2;
                /* A clockwise quarter turn maps image (x,y) to screen (-y,x). */
                Position point = {center.x - (y ? 20 : -20), center.y + (x ? 20 : -20)};
                int source = (x ^ horizontal ^ left) + 2 * (y ^ vertical);
                CHECK(pixel_check(point, colors[source]));
            }
        }
        if(app) {
            OK(rohr_graphics_animated_sprite_frame_scale_set(entity, 0, (Scale){0,10}));
            rohr_graphics_background_draw((Color){0,0,0,255});
            CHECK(rohr_graphics_animated_sprite_draw(entity));
            rohr_graphics_show();
        } else {
            animation->frames[0].scale = (Scale){0,10};
            draw(&project, &state);
            EditorSelectionRef selection;
            CHECK(!editor_viewport_selection_at_get(&project, &state,
                (Position){center.x+20,center.y+20}, &selection));
        }
        CHECK(pixel_check((Position){center.x+20,center.y+20}, (Color){0,0,0,255}));
    }
    OK(rohr_entity_delete(entity));
    editor_viewport_assets_destroy();
    editor_viewport_state_destroy(&state);
    editor_project_destroy(&project);
    CHECK(SDL_RemovePath(path));
    return true;
}

static bool soft_area_picking_check(void) {
    EditorProject project;
    EditorViewportState state = {0};
    EditorSelectionRef hit;
    editor_project_init(&project);
    editor_viewport_state_init(&state);
    EditorObject *object = editor_project_object_add(&project, (Position){0});
    CHECK(object != NULL);
    EditorSoftBody *body = editor_project_soft_body_add(&project, object);
    CHECK(body != NULL);
    const Position points[] = {{-70,-70},{70,-70},{70,70},{-70,70},
        {-20,-20},{20,-20},{20,20},{-20,20}};
    for(size_t i = 0; i < 8; i += 1) {
        EditorSoftNode *node = editor_project_soft_node_add(&project, body, points[i]);
        CHECK(node != NULL);
        node->visible = false;
    }
    EditorSoftArea *area = editor_soft_area_add(body);
    CHECK(area != NULL);
    area->outer.node_count = 4;
    EditorSoftHole *hole = editor_soft_hole_add(area);
    CHECK(hole != NULL);
    hole->loop.node_count = 4;
    for(size_t i = 0; i < 4; i += 1) {
        area->outer.nodes[i] = body->nodes[i].id;
        hole->loop.nodes[i] = body->nodes[i + 4].id;
    }
    area->graphics_layer_inherited = false;
    area->graphics_layer.value = 3;
    EditorRigidBody *back = editor_project_rigid_body_add(&project, object);
    CHECK(back != NULL);
    state.mode = EDITOR_VIEWPORT_OBJECT;
    project.viewport_local_view = false;
    project.viewport_camera_zoom = 1;
    Position center = {EDITOR_VIEWPORT_WIDTH * .5f,
        EDITOR_MENU_HEIGHT + (EDITOR_VIEWPORT_BOTTOM - EDITOR_MENU_HEIGHT) * .5f};
    CHECK(editor_viewport_selection_at_get(&project, &state, center, &hit));
    CHECK(hit.kind == EDITOR_SELECTION_RIGID_BODY && hit.item == back->id);
    Position solid = {center.x + 40, center.y};
    CHECK(editor_viewport_selection_at_get(&project, &state, solid, &hit));
    CHECK(hit.kind == EDITOR_SELECTION_SOFT_BODY && hit.item == body->id);
    /* A selected lower body cannot consume the front area's click. */
    state.selection = EDITOR_SELECTION_RIGID_BODY;
    state.selected_rigid_body = back->id;
    CHECK(editor_viewport_update(&state, &project, solid,
        MOUSE_BUTTON_STATE_PRESSED, MOUSE_BUTTON_STATE_UP, false, 0, false));
    CHECK(state.selection == EDITOR_SELECTION_SOFT_BODY && state.selected_soft_body == body->id);
    (void)editor_viewport_update(&state, &project, solid,
        MOUSE_BUTTON_STATE_RELEASED, MOUSE_BUTTON_STATE_UP, false, 0, false);
    editor_viewport_selection_clear(&state);
    state.selected_rigid_body = 0;
    state.selected_soft_body = 0;
    state.mode = EDITOR_VIEWPORT_OBJECT;
    area->visible = false;
    CHECK(editor_viewport_selection_at_get(&project, &state, solid, &hit));
    CHECK(hit.kind == EDITOR_SELECTION_RIGID_BODY);
    area->visible = true;
    area->graphics_layer.value = -1;
    CHECK(editor_viewport_selection_at_get(&project, &state, solid, &hit));
    CHECK(hit.kind == EDITOR_SELECTION_RIGID_BODY);
    area->graphics_layer.value = 3;
    hole->loop.node_count = 2;
    CHECK(editor_viewport_selection_at_get(&project, &state, solid, &hit));
    CHECK(hit.kind == EDITOR_SELECTION_RIGID_BODY);
    /* Exercise the shared scene draw path for a draft, then a transformed fill. */
    draw(&project, &state);
    hole->loop.node_count = 4;
    body->rotation = 90;
    body->position = (Position){10,0};
    project.viewport_camera_zoom = 2;
    draw(&project, &state);
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
    CHECK(SDL_SaveFile("editor_layers_frame.png", test_png, sizeof(test_png)));
    bool passed = body_layers_check();
    editor_viewport_assets_destroy();
    (void)SDL_RemovePath("editor_layers_frame.png");
    if(passed) passed = animation_direction_check();
    if(passed) passed = frame_flips_check();
    if(passed) passed = soft_area_picking_check();
    rohr_graphics_stop();
    rohr_engine_stop();
    return passed;
}
