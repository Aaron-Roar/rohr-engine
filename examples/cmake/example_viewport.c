/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "example_viewport.h"

bool example_viewport_create(CameraId camera, Color background, ViewportId *viewport) {
    if(viewport == NULL || camera == CAMERA_INVALID) return false;
    *viewport = VIEWPORT_INVALID;
    ViewportCreateItemDescription screen = {.name="world", .kind=VIEWPORT_CREATE_SCREEN,
        .placement=rohr_viewport_item_description_default_get(),
        .render_width=WINDOW_WIDTH, .render_height=WINDOW_HEIGHT, .background_color=background};
    screen.placement.rectangle=(ViewportRectangle){0,0,WINDOW_WIDTH,WINDOW_HEIGHT};
    ViewportCameraBinding binding = {.item_name="world", .camera=camera};
    ViewportCreateDescription config = {.viewport=rohr_viewport_description_default_get(),
        .items=&screen,.item_count=1,.cameras=&binding,.camera_count=1};
    config.viewport.rectangle=screen.placement.rectangle;
    config.viewport.enabled=true;
    ViewportIdResult created = rohr_viewport_create(&config);
    if(rohr_error_check(created)) return false;
    *viewport = created.result.value;
    return true;
}

bool example_body_appearance_set(Entity entity, Color fill, Color border, int layer) {
    return !rohr_error_check(rohr_entity_appearance_set(entity, (ObjectAppearance){
        .kind=OBJECT_BODY,.color=fill,.border_color=border,.visible=true,.enabled=true})) &&
        !rohr_error_check(rohr_graphics_layer_entity_set(entity,layer));
}

bool example_ui_viewport_create(Color background, ViewportId *viewport) {
    if(!viewport) return false;
    *viewport=VIEWPORT_INVALID;
    ViewportCreateDescription description={.viewport=rohr_viewport_description_default_get()};
    description.viewport.rectangle=(ViewportRectangle){0,0,WINDOW_WIDTH,WINDOW_HEIGHT};
    description.viewport.background_color=background;
    description.viewport.enabled=true;
    ViewportIdResult result=rohr_viewport_create(&description);
    if(rohr_error_check(result)) return false;
    *viewport=result.result.value;
    return true;
}

ViewportItemId example_label_add(ViewportId viewport, TextAsset text, UIRect bounds) {
    ViewportItemDescription placement=rohr_viewport_item_description_default_get();
    placement.fit=SCREEN_FIT_NONE;
    placement.rectangle=(ViewportRectangle){bounds.x,bounds.y,0,0};
    placement.layer=10;
    placement.clip_enabled=true;
    placement.clip_rectangle=(ViewportRectangle){bounds.x,bounds.y,bounds.width,bounds.height};
    ViewportUiTextDescription label={.text=text,.position={bounds.width*.5f,bounds.height*.5f},.scale={1,1}};
    ViewportItemIdResult result=rohr_viewport_ui_text_add(viewport,label,placement);
    return rohr_error_check(result) ? VIEWPORT_ITEM_INVALID : result.result.value;
}

ViewportItemId example_button_add(ViewportId viewport, TextAsset text, UIRect bounds,
        const UIButtonStyle *style) {
    UIButtonStyle colors=style ? *style : rohr_ui_button_style_default_get();
    ViewportUiShapeDescription button=rohr_graphics_ui_shape_description_default_get();
    button.shape=rohr_math_square_create(bounds.width,bounds.height);
    button.fill_color=colors.idle; button.hover_fill_color=colors.hovered; button.click_fill_color=colors.pressed;
    button.button_enabled=true; button.text.text=text;
    ViewportItemDescription placement=rohr_viewport_item_description_default_get();
    placement.fit=SCREEN_FIT_NONE;
    placement.rectangle=(ViewportRectangle){bounds.x+bounds.width*.5f,bounds.y+bounds.height*.5f,0,0};
    placement.layer=10;
    ViewportItemIdResult result=rohr_viewport_ui_shape_add(viewport,button,placement);
    return rohr_error_check(result) ? VIEWPORT_ITEM_INVALID : result.result.value;
}

ViewportItemId example_rectangle_add(ViewportId viewport, UIRect bounds, Color color, int layer) {
    ViewportUiShapeDescription shape=rohr_graphics_ui_shape_description_default_get();
    shape.shape=rohr_math_square_create(bounds.width,bounds.height);
    shape.fill_color=color;
    ViewportItemDescription placement=rohr_viewport_item_description_default_get();
    placement.fit=SCREEN_FIT_NONE;
    placement.rectangle=(ViewportRectangle){bounds.x+bounds.width*.5f,bounds.y+bounds.height*.5f,0,0};
    placement.layer=layer;
    ViewportItemIdResult result=rohr_viewport_ui_shape_add(viewport,shape,placement);
    return rohr_error_check(result) ? VIEWPORT_ITEM_INVALID : result.result.value;
}

bool example_button_clicked_check(ViewportItemId item, bool *armed) {
    if(!armed) return false;
    if(rohr_input_mouse_button_pressed_check(INPUT_MOUSE_BUTTON_LEFT))
        *armed=rohr_viewport_ui_pressed_check(item);
    bool clicked=*armed && rohr_input_mouse_button_released_check(INPUT_MOUSE_BUTTON_LEFT) &&
        rohr_viewport_ui_hovered_check(item);
    if(!rohr_input_mouse_button_down_check(INPUT_MOUSE_BUTTON_LEFT)) *armed=false;
    return clicked;
}

void example_viewport_destroy(ViewportId *viewport) {
    if(viewport == NULL || *viewport == VIEWPORT_INVALID) return;
    (void)rohr_viewport_destroy(*viewport);
    *viewport = VIEWPORT_INVALID;
}
