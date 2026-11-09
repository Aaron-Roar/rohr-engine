/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_GRAPHICS_CREATION_H
#define ROHR_GRAPHICS_CREATION_H
/* Included by graphics.h after its types and primitive declarations. Native
 * descriptor expressions are passed by value. Authored catalogs are borrowed
 * at their canonical addresses and must be lvalues. Generic selectors never
 * evaluate their operands; the selected argument is evaluated exactly once.
 * There is no catch-all recipe conversion: pointers/unrelated types fail. */
#ifndef ROHR_INTERNAL_OBJECT_VIEW_TYPES
#define ROHR_INTERNAL_OBJECT_VIEW_TYPES(apply)
#endif
#ifndef ROHR_INTERNAL_PROJECT_VIEW_TYPES
#define ROHR_INTERNAL_PROJECT_VIEW_TYPES(apply)
#endif
#define ROHR_INTERNAL_VIEWPORT_VIEW_TYPES(apply)
#define ROHR_INTERNAL_GRAPHICS_RECIPE_TYPE(type, adapter) \
    , type *: adapter, const type *: adapter
static inline const ViewportCreateDescription *rohr_internal_graphics_viewport_ref(const ViewportCreateDescription *value) { return value; }
static inline const ViewportCreateItemDescription *rohr_internal_graphics_item_ref(const ViewportCreateItemDescription *value) { return value; }
static inline const struct ObjectDescription *rohr_internal_graphics_object_ref(const struct ObjectDescription *value) { return value; }

ViewportIdResult rohr_internal_viewport_recipe_create(const ViewportCreateDescription *description);
static inline const ViewportDescription *rohr_internal_viewport_create_native_ref(const ViewportDescription *value) { return value; }
#define ROHR_INTERNAL_VIEWPORT_CREATE_REF(value) _Generic(&(value), \
    ViewportDescription *: rohr_internal_viewport_create_native_ref, \
    const ViewportDescription *: rohr_internal_viewport_create_native_ref, \
    ViewportCreateDescription *: rohr_internal_graphics_viewport_ref, \
    const ViewportCreateDescription *: rohr_internal_graphics_viewport_ref \
    ROHR_INTERNAL_VIEWPORT_VIEW_TYPES(ROHR_INTERNAL_GRAPHICS_RECIPE_TYPE))(&(value))
#define graphics_viewport_create(...) \
    _Generic((__VA_ARGS__), ViewportDescription: (graphics_viewport_create), default: rohr_internal_viewport_recipe_create)( \
        _Generic((__VA_ARGS__), ViewportDescription: (__VA_ARGS__), \
            default: ROHR_INTERNAL_VIEWPORT_CREATE_REF( \
                _Generic((__VA_ARGS__), ViewportDescription: (ViewportDescription){0}, default: (__VA_ARGS__)))))

ScreenIdResult rohr_internal_screen_create_recipe(const ViewportCreateItemDescription *description);
static inline const ScreenDescription *rohr_internal_screen_create_native_ref(const ScreenDescription *value) { return value; }
#define ROHR_INTERNAL_SCREEN_CREATE_REF(value) _Generic(&(value), \
    ScreenDescription *: rohr_internal_screen_create_native_ref, \
    const ScreenDescription *: rohr_internal_screen_create_native_ref, \
    ViewportCreateItemDescription *: rohr_internal_graphics_item_ref, \
    const ViewportCreateItemDescription *: rohr_internal_graphics_item_ref)(&(value))
#define graphics_screen_create(...) \
    _Generic((__VA_ARGS__), ScreenDescription: (graphics_screen_create), default: rohr_internal_screen_create_recipe)( \
        _Generic((__VA_ARGS__), ScreenDescription: (__VA_ARGS__), \
            default: ROHR_INTERNAL_SCREEN_CREATE_REF( \
                _Generic((__VA_ARGS__), ScreenDescription: (ScreenDescription){0}, default: (__VA_ARGS__)))))

GraphicsUiIdResult rohr_internal_ui_shape_create_recipe(const ViewportCreateItemDescription *description);
static inline const ViewportUiShapeDescription *rohr_internal_ui_shape_create_native_ref(const ViewportUiShapeDescription *value) { return value; }
#define ROHR_INTERNAL_UI_SHAPE_CREATE_REF(value) _Generic(&(value), \
    ViewportUiShapeDescription *: rohr_internal_ui_shape_create_native_ref, \
    const ViewportUiShapeDescription *: rohr_internal_ui_shape_create_native_ref, \
    ViewportCreateItemDescription *: rohr_internal_graphics_item_ref, \
    const ViewportCreateItemDescription *: rohr_internal_graphics_item_ref)(&(value))
#define graphics_ui_shape_create(...) \
    _Generic((__VA_ARGS__), ViewportUiShapeDescription: (graphics_ui_shape_create), default: rohr_internal_ui_shape_create_recipe)( \
        _Generic((__VA_ARGS__), ViewportUiShapeDescription: (__VA_ARGS__), \
            default: ROHR_INTERNAL_UI_SHAPE_CREATE_REF( \
                _Generic((__VA_ARGS__), ViewportUiShapeDescription: (ViewportUiShapeDescription){0}, default: (__VA_ARGS__)))))

GraphicsUiIdResult rohr_internal_ui_text_create_recipe(const ViewportCreateItemDescription *description);
static inline const ViewportUiTextDescription *rohr_internal_ui_text_create_native_ref(const ViewportUiTextDescription *value) { return value; }
#define ROHR_INTERNAL_UI_TEXT_CREATE_REF(value) _Generic(&(value), \
    ViewportUiTextDescription *: rohr_internal_ui_text_create_native_ref, \
    const ViewportUiTextDescription *: rohr_internal_ui_text_create_native_ref, \
    ViewportCreateItemDescription *: rohr_internal_graphics_item_ref, \
    const ViewportCreateItemDescription *: rohr_internal_graphics_item_ref)(&(value))
#define graphics_ui_text_create(...) \
    _Generic((__VA_ARGS__), ViewportUiTextDescription: (graphics_ui_text_create), default: rohr_internal_ui_text_create_recipe)( \
        _Generic((__VA_ARGS__), ViewportUiTextDescription: (__VA_ARGS__), \
            default: ROHR_INTERNAL_UI_TEXT_CREATE_REF( \
                _Generic((__VA_ARGS__), ViewportUiTextDescription: (ViewportUiTextDescription){0}, default: (__VA_ARGS__)))))

GraphicsUiIdResult rohr_internal_ui_slider_create_recipe(const ViewportCreateItemDescription *description);
static inline const ViewportUiSliderDescription *rohr_internal_ui_slider_create_native_ref(const ViewportUiSliderDescription *value) { return value; }
#define ROHR_INTERNAL_UI_SLIDER_CREATE_REF(value) _Generic(&(value), \
    ViewportUiSliderDescription *: rohr_internal_ui_slider_create_native_ref, \
    const ViewportUiSliderDescription *: rohr_internal_ui_slider_create_native_ref, \
    ViewportCreateItemDescription *: rohr_internal_graphics_item_ref, \
    const ViewportCreateItemDescription *: rohr_internal_graphics_item_ref)(&(value))
#define graphics_ui_slider_create(...) \
    _Generic((__VA_ARGS__), ViewportUiSliderDescription: (graphics_ui_slider_create), default: rohr_internal_ui_slider_create_recipe)( \
        _Generic((__VA_ARGS__), ViewportUiSliderDescription: (__VA_ARGS__), \
            default: ROHR_INTERNAL_UI_SLIDER_CREATE_REF( \
                _Generic((__VA_ARGS__), ViewportUiSliderDescription: (ViewportUiSliderDescription){0}, default: (__VA_ARGS__)))))

CameraIdResult rohr_internal_camera_create_recipe(const struct ObjectDescription *description);
static inline const CameraDescription *rohr_internal_camera_create_native_ref(const CameraDescription *value) { return value; }
#define ROHR_INTERNAL_CAMERA_CREATE_REF(value) _Generic(&(value), \
    CameraDescription *: rohr_internal_camera_create_native_ref, \
    const CameraDescription *: rohr_internal_camera_create_native_ref, \
    struct ObjectDescription *: rohr_internal_graphics_object_ref, \
    const struct ObjectDescription *: rohr_internal_graphics_object_ref \
    ROHR_INTERNAL_OBJECT_VIEW_TYPES(ROHR_INTERNAL_GRAPHICS_RECIPE_TYPE) \
    ROHR_INTERNAL_PROJECT_VIEW_TYPES(ROHR_INTERNAL_GRAPHICS_RECIPE_TYPE))(&(value))
#define graphics_camera_create(...) \
    _Generic((__VA_ARGS__), CameraDescription: (graphics_camera_create), default: rohr_internal_camera_create_recipe)( \
        _Generic((__VA_ARGS__), CameraDescription: (__VA_ARGS__), \
            default: ROHR_INTERNAL_CAMERA_CREATE_REF( \
                _Generic((__VA_ARGS__), CameraDescription: (CameraDescription){0}, default: (__VA_ARGS__)))))

TextureAssetResult rohr_internal_texture_load_recipe(const struct ObjectDescription *description);
static inline const TextureDescription *rohr_internal_texture_load_native_ref(const TextureDescription *value) { return value; }
#define ROHR_INTERNAL_TEXTURE_LOAD_REF(value) _Generic(&(value), \
    TextureDescription *: rohr_internal_texture_load_native_ref, \
    const TextureDescription *: rohr_internal_texture_load_native_ref, \
    struct ObjectDescription *: rohr_internal_graphics_object_ref, \
    const struct ObjectDescription *: rohr_internal_graphics_object_ref \
    ROHR_INTERNAL_OBJECT_VIEW_TYPES(ROHR_INTERNAL_GRAPHICS_RECIPE_TYPE) \
    ROHR_INTERNAL_PROJECT_VIEW_TYPES(ROHR_INTERNAL_GRAPHICS_RECIPE_TYPE))(&(value))
#define graphics_texture_load(...) \
    _Generic((__VA_ARGS__), TextureDescription: (graphics_texture_load), default: rohr_internal_texture_load_recipe)( \
        _Generic((__VA_ARGS__), TextureDescription: (__VA_ARGS__), \
            default: ROHR_INTERNAL_TEXTURE_LOAD_REF( \
                _Generic((__VA_ARGS__), TextureDescription: (TextureDescription){0}, default: (__VA_ARGS__)))))

AnimationAssetResult rohr_internal_animation_load_recipe(const struct ObjectDescription *description);
static inline const AnimationDescription *rohr_internal_animation_load_native_ref(const AnimationDescription *value) { return value; }
#define ROHR_INTERNAL_ANIMATION_LOAD_REF(value) _Generic(&(value), \
    AnimationDescription *: rohr_internal_animation_load_native_ref, \
    const AnimationDescription *: rohr_internal_animation_load_native_ref, \
    struct ObjectDescription *: rohr_internal_graphics_object_ref, \
    const struct ObjectDescription *: rohr_internal_graphics_object_ref \
    ROHR_INTERNAL_OBJECT_VIEW_TYPES(ROHR_INTERNAL_GRAPHICS_RECIPE_TYPE) \
    ROHR_INTERNAL_PROJECT_VIEW_TYPES(ROHR_INTERNAL_GRAPHICS_RECIPE_TYPE))(&(value))
#define graphics_animation_load(...) \
    _Generic((__VA_ARGS__), AnimationDescription: (graphics_animation_load), default: rohr_internal_animation_load_recipe)( \
        _Generic((__VA_ARGS__), AnimationDescription: (__VA_ARGS__), \
            default: ROHR_INTERNAL_ANIMATION_LOAD_REF( \
                _Generic((__VA_ARGS__), AnimationDescription: (AnimationDescription){0}, default: (__VA_ARGS__)))))

FontAssetResult rohr_internal_font_load_recipe(const ViewportCreateItemDescription *description);
static inline const FontDescription *rohr_internal_font_load_native_ref(const FontDescription *value) { return value; }
#define ROHR_INTERNAL_FONT_LOAD_REF(value) _Generic(&(value), \
    FontDescription *: rohr_internal_font_load_native_ref, \
    const FontDescription *: rohr_internal_font_load_native_ref, \
    ViewportCreateItemDescription *: rohr_internal_graphics_item_ref, \
    const ViewportCreateItemDescription *: rohr_internal_graphics_item_ref)(&(value))
#define graphics_font_load(...) \
    _Generic((__VA_ARGS__), FontDescription: (graphics_font_load), default: rohr_internal_font_load_recipe)( \
        _Generic((__VA_ARGS__), FontDescription: (__VA_ARGS__), \
            default: ROHR_INTERNAL_FONT_LOAD_REF( \
                _Generic((__VA_ARGS__), FontDescription: (FontDescription){0}, default: (__VA_ARGS__)))))

TextAssetResult rohr_internal_text_create_recipe(const ViewportCreateItemDescription *description);
static inline const TextDescription *rohr_internal_text_create_native_ref(const TextDescription *value) { return value; }
#define ROHR_INTERNAL_TEXT_CREATE_REF(value) _Generic(&(value), \
    TextDescription *: rohr_internal_text_create_native_ref, \
    const TextDescription *: rohr_internal_text_create_native_ref, \
    ViewportCreateItemDescription *: rohr_internal_graphics_item_ref, \
    const ViewportCreateItemDescription *: rohr_internal_graphics_item_ref)(&(value))
#define graphics_text_create(...) \
    _Generic((__VA_ARGS__), TextDescription: (graphics_text_create), default: rohr_internal_text_create_recipe)( \
        _Generic((__VA_ARGS__), TextDescription: (__VA_ARGS__), \
            default: ROHR_INTERNAL_TEXT_CREATE_REF( \
                _Generic((__VA_ARGS__), TextDescription: (TextDescription){0}, default: (__VA_ARGS__)))))
#endif
