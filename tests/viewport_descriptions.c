/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(value) do { if(!(value)) { fprintf(stderr, "viewport description check %d: %s\n", __LINE__, #value); return false; } } while(0)
#define OK(value) CHECK(!rohr_error_check(value))
#define BAD(value) CHECK(rohr_error_check(value))

#ifdef ROHR_TEST_VIEWPORT_WRAP
static bool fail_texture;
static float pointer_x, pointer_y;
static SDL_MouseButtonFlags pointer_buttons;
SDL_MouseButtonFlags __wrap_SDL_GetMouseState(float *x, float *y) {
    if(x != NULL) *x = pointer_x;
    if(y != NULL) *y = pointer_y;
    return pointer_buttons;
}
SDL_Texture *__real_SDL_CreateTexture(SDL_Renderer *, SDL_PixelFormat, SDL_TextureAccess, int, int);
SDL_Texture *__wrap_SDL_CreateTexture(SDL_Renderer *renderer, SDL_PixelFormat format,
        SDL_TextureAccess access, int width, int height) {
    if(fail_texture) { fail_texture = false; SDL_SetError("injected texture failure"); return NULL; }
    return __real_SDL_CreateTexture(renderer, format, access, width, height);
}
#endif

#ifdef ROHR_TEST_VIEWPORT_WRAP
static bool draw_at(float x, float y, MouseButtonState state) {
    pointer_x = x; pointer_y = y;
    pointer_buttons = state == MOUSE_BUTTON_STATE_RELEASED ? 0 : SDL_BUTTON_LMASK;
    CHECK(rohr_graphics_show().header.status != ROHR_REPORT_FAILED);
    return true;
}
#endif

static bool descriptions(void) {
    CHECK(!rohr_viewport_description_default_get().enabled);
    CHECK(rohr_viewport_item_description_default_get().visible);
    CHECK(rohr_graphics_layer_description_default_get().name == NULL);
    LayerDescription layer = {.name = "world", .value = 10};
    GraphicsLayerIdResult registered = rohr_graphics_layer_create(layer); OK(registered);
    GraphicsLayerIdResult occupied = rohr_graphics_layer_create((LayerDescription){.name = "other"}); OK(occupied);
    layer.name = "other";
    BAD(rohr_graphics_layer_description_set(registered.result.value, layer));
    CHECK(strcmp(rohr_graphics_layer_description_get(registered.result.value).result.value.name, "world") == 0);
    ViewportDescription view = rohr_viewport_description_default_get(); view.enabled = true;
    ViewportIdResult viewport = rohr_viewport_create(view); OK(viewport);
    ScreenDescription screen_desc = rohr_screen_description_default_get();
    ScreenIdResult screen = rohr_screen_create(screen_desc); OK(screen);
    ScreenIdResult second = rohr_screen_create(screen_desc); OK(second);
    ViewportItemDescription placement = rohr_viewport_item_description_default_get();
    placement.screen = screen.result.value; placement.layer_id = registered.result.value;
    placement.rectangle = (ViewportRectangle){10, 20, 200, 150};
    placement.orientation = 30; placement.content_orientation = 60;
    placement.clip_enabled = true; placement.clip_rectangle = (ViewportRectangle){0, 0, 120, 100};
    ViewportItemIdResult mount = rohr_viewport_item_add(viewport.result.value, placement); OK(mount);
    placement.screen = second.result.value; placement.rectangle = (ViewportRectangle){140, 40, 300, 220};
    placement.layer_id = 0; placement.layer = 30; placement.orientation = 15;
    ViewportItemIdResult sibling = rohr_viewport_item_add(viewport.result.value, placement); OK(sibling);
    layer.name = "scene"; layer.value = 20;
    OK(rohr_graphics_layer_description_set(registered.result.value, layer));
    placement = rohr_viewport_item_description_get(mount.result.value).result.value;
    CHECK(placement.screen == screen.result.value && placement.layer_id == registered.result.value);
    placement.rectangle.x = 42;
    OK(rohr_viewport_item_description_set(mount.result.value, placement));
    CHECK(rohr_viewport_item_description_get(mount.result.value).result.value.layer_id == registered.result.value);
    BAD(rohr_viewport_item_description_set(mount.result.value, (ViewportItemDescription){0}));
    CHECK(rohr_viewport_item_description_get(mount.result.value).result.value.rectangle.x == 42);
    view.rectangle.width = 800; view.background_color.red = 20;
    OK(rohr_viewport_description_set(viewport.result.value, view));
    OK(rohr_viewport_item_description_get(sibling.result.value));
    view.rectangle.width = NAN;
    BAD(rohr_viewport_description_set(viewport.result.value, view));
    CHECK(rohr_viewport_description_get(viewport.result.value).result.value.rectangle.width == 800);
    screen_desc.width = 320;
#ifdef ROHR_TEST_VIEWPORT_WRAP
    fail_texture = true;
    BAD(rohr_screen_description_set(screen.result.value, screen_desc));
    CHECK(rohr_screen_description_get(screen.result.value).result.value.width == WINDOW_WIDTH);
#endif
    OK(rohr_screen_description_set(screen.result.value, screen_desc));
    CHECK(rohr_screen_description_get(screen.result.value).result.value.width == 320);
    OK(rohr_viewport_item_description_get(mount.result.value));
    screen_desc.camera = CAMERA_INVALID;
    OK(rohr_screen_description_set(screen.result.value, screen_desc));
    CHECK(rohr_screen_description_get(screen.result.value).result.value.camera == CAMERA_INVALID);

    ViewportUiSliderDescription slider = rohr_graphics_ui_slider_description_default_get();
    slider.maximum = 10; slider.value = 4; slider.enabled = true;
    GraphicsUiIdResult ui = rohr_graphics_ui_slider_create(slider); OK(ui);
    placement.screen = SCREEN_INVALID; placement.ui = ui.result.value;
    placement.rectangle = (ViewportRectangle){200, 200, 0, 0}; placement.orientation = 0;
    placement.content_orientation = 0; placement.clip_enabled = false;
    OK(rohr_viewport_item_description_set(mount.result.value, placement));
    BAD(rohr_graphics_ui_destroy(ui.result.value));
    ViewportItemDescription bad = placement; bad.screen = screen.result.value;
    BAD(rohr_viewport_item_description_set(mount.result.value, bad));
    bad = placement; bad.content_scale.x = NAN;
    BAD(rohr_viewport_item_description_set(mount.result.value, bad));
    CHECK(rohr_viewport_item_description_get(mount.result.value).result.value.ui == ui.result.value);
#ifdef ROHR_TEST_VIEWPORT_WRAP
    CHECK(draw_at(200, 200, MOUSE_BUTTON_STATE_RELEASED));
    CHECK(draw_at(200, 200, MOUSE_BUTTON_STATE_PRESSED));
    CHECK(rohr_viewport_ui_pressed_check(mount.result.value));
    float value = rohr_graphics_ui_slider_value_get(ui.result.value).result.value;
    slider = rohr_graphics_ui_slider_description_get(ui.result.value).result.value;
    slider.thumb_color.red = 100;
    OK(rohr_graphics_ui_slider_description_set(ui.result.value, slider));
    CHECK(rohr_viewport_ui_pressed_check(mount.result.value));
    CHECK(rohr_graphics_ui_slider_value_get(ui.result.value).result.value == value);
    CHECK(draw_at(220, 200, MOUSE_BUTTON_STATE_PRESSED));
    CHECK(rohr_graphics_ui_slider_value_get(ui.result.value).result.value > value);
    slider = rohr_graphics_ui_slider_description_get(ui.result.value).result.value;
    slider.maximum = 3; slider.step = 1;
    OK(rohr_graphics_ui_slider_description_set(ui.result.value, slider));
    CHECK(rohr_graphics_ui_slider_value_get(ui.result.value).result.value == 3);
    slider.length = INFINITY;
    BAD(rohr_graphics_ui_slider_description_set(ui.result.value, slider));
    CHECK(rohr_viewport_ui_pressed_check(mount.result.value));
    placement.visible = false;
    OK(rohr_viewport_item_description_set(mount.result.value, placement));
    CHECK(!rohr_viewport_ui_pressed_check(mount.result.value));
    placement.visible = true;
    OK(rohr_viewport_item_description_set(mount.result.value, placement));
    CHECK(draw_at(180, 200, MOUSE_BUTTON_STATE_PRESSED));
    CHECK(!rohr_viewport_ui_pressed_check(mount.result.value));
    CHECK(rohr_graphics_ui_slider_value_get(ui.result.value).result.value == 3);
    CHECK(draw_at(180, 200, MOUSE_BUTTON_STATE_RELEASED));
    CHECK(draw_at(180, 200, MOUSE_BUTTON_STATE_PRESSED));
    CHECK(rohr_viewport_ui_pressed_check(mount.result.value));
    OK(rohr_viewport_disable_set(viewport.result.value));
    CHECK(!rohr_viewport_ui_pressed_check(mount.result.value));
    OK(rohr_viewport_enable_set(viewport.result.value));
    placement.drag_mode = VIEWPORT_ITEM_DRAG_XY;
    OK(rohr_viewport_item_description_set(mount.result.value, placement));
    CHECK(draw_at(200, 200, MOUSE_BUTTON_STATE_RELEASED));
    CHECK(draw_at(200, 200, MOUSE_BUTTON_STATE_PRESSED));
    CHECK(rohr_viewport_item_dragging_check(mount.result.value));
    placement = rohr_viewport_item_description_get(mount.result.value).result.value;
    placement.layer = 40; placement.layer_id = 0;
    OK(rohr_viewport_item_description_set(mount.result.value, placement));
    CHECK(rohr_viewport_item_dragging_check(mount.result.value));
    placement.ui = 0; placement.screen = screen.result.value;
    OK(rohr_viewport_item_description_set(mount.result.value, placement));
    CHECK(!rohr_viewport_item_dragging_check(mount.result.value));
#else
    placement.ui = 0; placement.screen = screen.result.value;
    OK(rohr_viewport_item_description_set(mount.result.value, placement));
#endif
    OK(rohr_graphics_ui_destroy(ui.result.value));

    /* Automatically owned content survives its original mount while shared. */
    ViewportUiTextDescription text = rohr_graphics_ui_text_description_default_get();
    TextAssetResult rendered = rohr_graphics_text_create((TextDescription){.value = "shared"}); OK(rendered);
    text.text = rendered.result.value;
    ViewportItemIdResult automatic = rohr_viewport_ui_text_add(viewport.result.value, text,
        rohr_viewport_item_description_default_get()); OK(automatic);
    ViewportItemDescription copy = rohr_viewport_item_description_get(automatic.result.value).result.value;
    GraphicsUiId automatic_ui = copy.ui;
    ViewportItemIdResult shared = rohr_viewport_item_add(viewport.result.value, copy); OK(shared);
    OK(rohr_graphics_text_destroy(&rendered.result.value));
    text.position.x = 15;
    OK(rohr_graphics_ui_text_description_set(automatic_ui, text));
    text.text.handle = UINT32_MAX;
    BAD(rohr_graphics_ui_text_description_set(automatic_ui, text));
    CHECK(rohr_graphics_ui_text_description_get(automatic_ui).result.value.position.x == 15);
    copy.ui = 0; copy.screen = second.result.value;
    OK(rohr_viewport_item_description_set(automatic.result.value, copy));
    OK(rohr_graphics_ui_text_description_get(automatic_ui));
    OK(rohr_viewport_item_remove(shared.result.value));
    BAD(rohr_graphics_ui_text_description_get(automatic_ui));
    OK(rohr_graphics_layer_destroy(registered.result.value));
    OK(rohr_graphics_layer_destroy(occupied.result.value));
    OK(rohr_viewport_destroy(viewport.result.value));
    BAD(rohr_viewport_description_get(viewport.result.value));
    BAD(rohr_viewport_item_description_get(mount.result.value));
    OK(rohr_screen_destroy(screen.result.value)); OK(rohr_screen_destroy(second.result.value));
    return true;
}

int main(void) {
    if(rohr_error_check(rohr_engine_start())) return 1;
    if(rohr_error_check(rohr_graphics_start())) { rohr_engine_stop(); return 1; }
    bool passed = descriptions();
    rohr_graphics_stop(); rohr_engine_stop();
    return passed ? 0 : 1;
}
