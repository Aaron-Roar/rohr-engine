/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "entity_components.h"
#include "report.h"
#include "engine.h"
#include "physics.h"
#include "math2d.h"
#include "math.h"

/* All orientation fields and default scalar angles are clockwise degrees.
 * World +Y points up; viewport/screen +Y points down. Zero preserves content.
 * Explicit radians scalar APIs convert at their boundary; structs stay degrees.
 */

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720
#define MAX_TEXTURES 50
#define MAX_ANIMATIONS_FRAMES 20
#define MAX_TEXTURE_ASSETS 1024
#define MAX_ANIMATION_ASSETS 1024
#define MAX_FONT_ASSETS 1024
#define MAX_TEXT_ASSETS 2048
#define MAX_ANIMATION_SETS 10
#define MAX_CAMERAS 16
#define MAX_SCREENS 16
#define MAX_VIEWPORTS 16
#define MAX_VIEWPORT_ITEMS 32
#define MAX_GRAPHICS_UI_ELEMENTS 128
#define MAX_GRAPHICS_LAYERS 128
#define GRAPHICS_LAYER_NAME_MAX 64

/** Diagnostic context within a graphics pipeline; zero means unspecified. */
typedef enum GraphicsReportStage {
    ROHR_GRAPHICS_SUBMISSION = 1,
    ROHR_GRAPHICS_CAMERA_RENDER,
    ROHR_GRAPHICS_CAMERA_CALLBACK = ROHR_GRAPHICS_CAMERA_RENDER,
    ROHR_GRAPHICS_COMMAND_EXECUTION,
    ROHR_GRAPHICS_VIEWPORT_COMPOSITION,
    ROHR_GRAPHICS_RECORDING_CAPTURE,
    ROHR_GRAPHICS_RECORDING_WRITE,
    ROHR_GRAPHICS_PRESENTATION,
    ROHR_GRAPHICS_ANIMATION,
    ROHR_GRAPHICS_RESTORE,
} GraphicsReportStage;

#define RECORDING_WIDTH  WINDOW_WIDTH
#define RECORDING_HEIGHT WINDOW_HEIGHT

typedef enum GraphicsWindowMode {
    GRAPHICS_WINDOW_MODE_WINDOWED,
    GRAPHICS_WINDOW_MODE_BORDERLESS_FULLSCREEN,
    GRAPHICS_WINDOW_MODE_FULLSCREEN
} GraphicsWindowMode;

typedef struct WindowPresentationDescription {
    GraphicsWindowMode mode;
    int window_width;
    int window_height;
    int logical_width;
    int logical_height;
    bool aspect_ratio_auto;
} WindowPresentationDescription;

/** 2D scale factor for textures and sprites. */
typedef struct {
    /** Horizontal scale. */
    float x;
    /** Vertical scale. */
    float y;
} Scale;

/** Stable handle for an engine-owned camera. */
typedef uint32_t CameraId;
ERROR_DECLARE_RESULT_TYPE(AnimationFrameIndexResult, size_t);
typedef void (*CameraRenderCallback)(CameraId camera, void *context);

/** Invalid camera handle. */
#define CAMERA_INVALID 0

/** Logical window rectangle occupied by a viewport. */
typedef struct ViewportRectangle {
    float x;
    float y;
    float width;
    float height;
} ViewportRectangle;

/** World-space camera transform used by the renderer. */
typedef struct {
    /** World position shown at the center of the viewport. */
    Position position;
    /** Clockwise world orientation in degrees; zero keeps local +Y up. */
    Orientation orientation;
    /** Logical world-view dimensions before magnification. */
    Vec2D dimensions;
    /** Magnification where values above one zoom in. */
    float zoom;
} Camera;

/** Complete camera configuration; attachment and animation progress are separate.
 * Position/orientation are resolved world values. Callback context is borrowed. */
typedef struct CameraDescription {
    Position position;
    Orientation orientation;
    Vec2D dimensions;
    float zoom;
    bool enabled;
    bool pause_with_engine;
    CameraRenderCallback render_callback;
    void *render_context;
} CameraDescription;
ERROR_DECLARE_RESULT_TYPE(CameraDescriptionResult, CameraDescription);

ERROR_DECLARE_RESULT_TYPE(CameraIdResult, CameraId);
ERROR_DECLARE_RESULT_TYPE(CameraResult, Camera);
ERROR_DECLARE_RESULT_TYPE(CameraZoomResult, float);

/** Non-owning description of an entity-following camera attachment. */
typedef struct {
    /** Entity transform followed by the camera. */
    Entity entity;
    /**
     * Local offset when following both position and orientation; world-space
     * offset when following position alone; fixed world position otherwise.
     */
    Vec2D position_offset;
    /** Relative or fixed clockwise orientation in degrees. */
    Orientation orientation_offset;
    /** Whether the camera inherits the entity position. */
    bool follow_position;
    /** Whether the camera inherits the entity orientation. */
    bool follow_orientation;
} CameraAttachmentDescription;

/** Result type for functions that return a CameraAttachmentDescription. */
ERROR_DECLARE_RESULT_TYPE(CameraAttachmentDescriptionResult, CameraAttachmentDescription);

typedef uint32_t ViewportId;
typedef uint32_t ViewportItemId;
/** Reusable UI definition that may be mounted in multiple viewports. */
typedef uint32_t GraphicsUiId;
typedef uint32_t GraphicsLayerId;
typedef uint32_t ScreenId;

#define VIEWPORT_INVALID 0
#define VIEWPORT_ITEM_INVALID 0
#define GRAPHICS_UI_INVALID 0
#define GRAPHICS_LAYER_INVALID 0
#define SCREEN_INVALID 0

/** A globally named render order. Names are copied on input and borrowed on get. */
typedef struct LayerDescription {
    const char *name;
    int value;
} LayerDescription;
ERROR_DECLARE_RESULT_TYPE(LayerDescriptionResult, LayerDescription);

typedef enum ScreenContentMode {
    SCREEN_CONTENT_CALLBACK,
    SCREEN_CONTENT_AUTOMATIC,
} ScreenContentMode;

typedef struct ScreenDescription {
    CameraId camera;
    int width;
    int height;
    ScreenContentMode content_mode;
    Color background_color;
    /** Also clear before a legacy callback; automatic content always clears. */
    bool clear_background;
    /** Optional borrowed entity owning camera. Direct camera assignment clears it. */
    Entity camera_entity;
} ScreenDescription;

ERROR_DECLARE_RESULT_TYPE(ScreenIdResult, ScreenId);
ERROR_DECLARE_RESULT_TYPE(ScreenDescriptionResult, ScreenDescription);

typedef enum ScreenFit {
    SCREEN_FIT_NONE,
    SCREEN_FIT_STRETCH,
    SCREEN_FIT_CONTAIN,
    SCREEN_FIT_COVER,
} ScreenFit;

typedef enum ViewportItemDragMode {
    VIEWPORT_ITEM_DRAG_NONE,
    VIEWPORT_ITEM_DRAG_X,
    VIEWPORT_ITEM_DRAG_Y,
    VIEWPORT_ITEM_DRAG_XY,
} ViewportItemDragMode;

ERROR_DECLARE_RESULT_TYPE(ViewportItemDragModeResult, ViewportItemDragMode);

typedef struct ViewportDescription {
    /** Window-space destination and clipping rectangle. */
    ViewportRectangle rectangle;
    /** How assigned screen content is fitted into the rectangle. */
    ScreenFit fit;
    /** Color drawn inside the viewport before its items are composed. */
    Color background_color;
    bool enabled;
} ViewportDescription;

/** Mounted content and viewport-local placement. Exactly one content handle is required. */
typedef struct ViewportItemDescription {
    ScreenId screen;
    GraphicsUiId ui;
    ViewportRectangle rectangle;
    ScreenFit fit;
    Orientation orientation;
    Position content_offset;
    Scale content_scale;
    Orientation content_orientation;
    int layer;
    /** A nonzero named layer takes precedence over the literal layer value. */
    GraphicsLayerId layer_id;
    bool visible;
    /** Runtime pointer-drag constraint for mounted UI items. */
    ViewportItemDragMode drag_mode;
    /** Clip this item to a viewport-local rectangle. */
    bool clip_enabled;
    /** Viewport-local clip rectangle, independent from item placement. */
    ViewportRectangle clip_rectangle;
} ViewportItemDescription;

typedef enum ViewportUiBorderType {
    VIEWPORT_UI_BORDER_LINE,
    VIEWPORT_UI_BORDER_HASHED,
} ViewportUiBorderType;

/** Generation-checked handle for an engine-owned rendered-text instance. */
typedef uint32_t TextHandle;

/** Invalid rendered-text handle. */
#define TEXT_HANDLE_INVALID UINT32_C(0)

/**
 * Unique mutable rendered text. Copying this value creates a borrowed alias,
 * not another owner.
 */
typedef struct TextAsset {
    TextHandle handle;
    /** Logical screen-space dimensions of the current rendered text. */
    Scale size;
} TextAsset;

/** Result type for functions that return a TextAsset. */
ERROR_DECLARE_RESULT_TYPE(TextAssetResult, TextAsset);

typedef struct ViewportUiTextDescription {
    /** Borrowed on input; persistent UI acquires an internal dependency. */
    TextAsset text;
    Position position;
    Position offset;
    Scale scale;
    Orientation orientation;
} ViewportUiTextDescription;

typedef struct ViewportUiShapeDescription {
    Shape shape;
    Position position;
    Orientation orientation;
    bool border_enabled;
    ViewportUiBorderType border_type;
    float border_thickness;
    float border_hash_spacing;
    float border_corner_radius;
    Color border_color;
    Color fill_color;
    bool button_enabled;
    Color hover_border_color;
    Color hover_fill_color;
    Color click_border_color;
    Color click_fill_color;
    ViewportUiTextDescription text;
} ViewportUiShapeDescription;

typedef enum ViewportUiSliderThumbShape {
    VIEWPORT_UI_SLIDER_THUMB_RECTANGLE,
    VIEWPORT_UI_SLIDER_THUMB_CIRCLE,
} ViewportUiSliderThumbShape;

/** Reusable viewport slider definition. Placement and rotation belong to its item. */
typedef struct ViewportUiSliderDescription {
    float minimum;
    float maximum;
    float value;
    /** Zero disables value snapping. */
    float step;
    float length;
    float track_thickness;
    float thumb_width;
    float thumb_height;
    float thumb_radius;
    float thumb_offset;
    ViewportUiSliderThumbShape thumb_shape;
    Color track_color;
    Color filled_track_color;
    Color thumb_color;
    Color hover_thumb_color;
    Color pressed_thumb_color;
    bool enabled;
} ViewportUiSliderDescription;

ERROR_DECLARE_RESULT_TYPE(ViewportIdResult, ViewportId);
ERROR_DECLARE_RESULT_TYPE(ViewportDescriptionResult, ViewportDescription);
ERROR_DECLARE_RESULT_TYPE(ViewportItemIdResult, ViewportItemId);
ERROR_DECLARE_RESULT_TYPE(ViewportItemDescriptionResult, ViewportItemDescription);
ERROR_DECLARE_RESULT_TYPE(GraphicsUiIdResult, GraphicsUiId);
ERROR_DECLARE_RESULT_TYPE(ViewportUiShapeDescriptionResult, ViewportUiShapeDescription);
ERROR_DECLARE_RESULT_TYPE(ViewportUiTextDescriptionResult, ViewportUiTextDescription);
ERROR_DECLARE_RESULT_TYPE(ViewportUiSliderDescriptionResult, ViewportUiSliderDescription);
ERROR_DECLARE_RESULT_TYPE(GraphicsUiSliderValueResult, float);
ERROR_DECLARE_RESULT_TYPE(GraphicsLayerIdResult, GraphicsLayerId);
ERROR_DECLARE_RESULT_TYPE(GraphicsLayerValueResult, int);

/** Description for loading a texture from disk. */
typedef struct {
  /** Path to the texture file. */
  const char *file;
  /** Per-use logical drawing size; independent of native pixel dimensions. */
  Scale size;
} TextureDescription;
ERROR_DECLARE_RESULT_TYPE(TextureDescriptionResult, TextureDescription);

/** Description for loading a shared image catalog with default timing. */
typedef struct {
  /** Stable project identity. Zero selects the compatibility default of one. */
  AnimationId id;
  /** Image paths for each catalog frame. Instance transforms are set after creation. */
  const char *frame_files[MAX_ANIMATIONS_FRAMES];
  /** Stable frame IDs. Zero entries default to their one-based frame index. */
  AnimationFrameId frame_ids[MAX_ANIMATIONS_FRAMES];

  /** Number of ordered frames; zero creates a valid empty catalog. */
  uint8_t frame_count;
  /** Frame duration measured in engine ticks. */
  Tick ticks_per_frame;
  /** Frame duration measured in engine time. */
  Time time_per_frame;
} AnimationDescription;
ERROR_DECLARE_RESULT_TYPE(AnimationDescriptionResult, AnimationDescription);

/** Generation-checked handle for an engine-owned texture resource. */
typedef uint32_t TextureHandle;

/** Invalid texture-resource handle. */
#define TEXTURE_HANDLE_INVALID UINT32_C(0)

/** Shared texture resource and per-use logical size metadata. */
typedef struct {
    /** Engine-owned shared texture resource. */
    TextureHandle handle;
    /** Logical drawing size, independent from the shared GPU resource. */
    Scale size;
} TextureAsset;

/** Result type for functions that return a TextureAsset. */
ERROR_DECLARE_RESULT_TYPE(TextureAssetResult, TextureAsset);
ERROR_DECLARE_RESULT_TYPE(TextureSizeResult, Scale);

/** Load a custom font, or select the built-in font with a zero description. */
typedef struct FontDescription {
    /** Path to a font file supported by SDL3_ttf. */
    const char *file;
    /** Font point size. */
    float point_size;
} FontDescription;
ERROR_DECLARE_RESULT_TYPE(FontDescriptionResult, FontDescription);

/** Generation-checked handle for an engine-owned font resource. */
typedef uint32_t FontHandle;

/** Invalid font-resource handle. */
#define FONT_HANDLE_INVALID UINT32_C(0)

/**
 * Opaque shared font resource. Copying this value does not retain it;
 * explicitly retain every independently owned copy.
 */
typedef struct FontAsset {
    FontHandle handle;
} FontAsset;

/** Result type for functions that return a FontAsset. */
ERROR_DECLARE_RESULT_TYPE(FontAssetResult, FontAsset);

/** Complete text configuration. Font handle zero selects the built-in font.
 * Value must be non-null (empty is valid) and is copied on create/set.
 * Omitted color is transparent zero; wrap_width zero disables wrapping.
 * Built-in text uses fixed ASCII glyphs, word wrapping and hard breaks for long
 * words; widths smaller than one glyph still display one glyph per line. */
typedef struct TextDescription {
    FontAsset font;
    const char *value;
    Color color;
    int wrap_width;
} TextDescription;
ERROR_DECLARE_RESULT_TYPE(TextDescriptionResult, TextDescription);

struct ObjectDescription;
typedef enum ViewportCreateItemKind {
    VIEWPORT_CREATE_SCREEN, VIEWPORT_CREATE_SHAPE,
    VIEWPORT_CREATE_TEXT, VIEWPORT_CREATE_SLIDER,
} ViewportCreateItemKind;

/** Immutable recipe. Runtime handles in placement and UI descriptions must be zero.
 * Screen content is automatic; camera references use the creation descriptor's bindings.
 * Without a runtime binding, a screen shows only its background, even when its
 * recipe names an authored camera. Authored references are still validated. */
typedef struct ViewportCreateItemDescription {
    const char *name;
    ViewportCreateItemKind kind;
    ViewportItemDescription placement;
    const char *layer_name;
    int render_width, render_height;
    Color background_color;
    const struct ObjectDescription *camera_object;
    const struct ObjectDescription *camera_member;
    ViewportUiShapeDescription shape;
    ViewportUiTextDescription text;
    ViewportUiSliderDescription slider;
    /** Null value means no text. Font file null selects the built-in font. */
    const char *text_value;
    FontDescription font;
    Color text_color;
    int text_wrap_width;
} ViewportCreateItemDescription;

typedef struct ViewportObjectBinding {
    const struct ObjectDescription *description;
    Entity instance;
} ViewportObjectBinding;
typedef struct ViewportCameraBinding {
    const char *item_name;
    /** Exactly one of these borrowed references must be supplied. */
    Entity entity;
    CameraId camera;
} ViewportCameraBinding;
/** Complete creation input. A local descriptor may reference an immutable template
 * and supply only runtime bindings. Template chains and bindings in templates are
 * rejected. Input storage is borrowed for this call only. */
typedef struct ViewportCreateDescription {
    const struct ViewportCreateDescription *template;
    ViewportDescription viewport;
    const ViewportCreateItemDescription *items;
    size_t item_count;
    const LayerDescription *layers;
    size_t layer_count;
    const ViewportObjectBinding *objects;
    size_t object_count;
    const ViewportCameraBinding *cameras;
    size_t camera_count;
} ViewportCreateDescription;

ViewportIdResult graphics_viewport_create_description(const ViewportCreateDescription *description);
ViewportItemIdResult graphics_viewport_item_by_name_get(ViewportId viewport, const char *name);

/** Generation-checked handle for an engine-owned animation resource. */
typedef uint32_t AnimationHandle;

/** Invalid animation-resource handle. */
#define ANIMATION_HANDLE_INVALID UINT32_C(0)

/**
 * Opaque shared animation resource. Copying this value does not retain it;
 * explicitly retain every independently owned copy.
 */
typedef struct {
    AnimationHandle handle;
} AnimationAsset;

/** Immutable animation metadata. */
typedef struct {
    AnimationId id;
    size_t frame_count;
    Tick ticks_per_frame;
    Time time_per_frame;
} AnimationInfo;

/** Reusable frame value. The texture is borrowed; copies have independent transforms. */
typedef struct {
    AnimationFrameId id;
    TextureAsset texture;
    Scale scale;
    Position offset;
    Orientation rotation;
} AnimationFrameDescription;

/** Result type for functions that return an AnimationAsset. */
ERROR_DECLARE_RESULT_TYPE(AnimationAssetResult, AnimationAsset);
ERROR_DECLARE_RESULT_TYPE(AnimationInfoResult, AnimationInfo);
ERROR_DECLARE_RESULT_TYPE(AnimationFrameDescriptionResult, AnimationFrameDescription);

/**
 * Mutable playback state owned independently by each consumer. Creating a
 * player does not retain its animation; its owner must keep the asset alive.
 */
typedef struct AnimationPlayer {
    AnimationAsset animation;
    size_t frame_index;
    size_t frame_count;
    Tick last_update_tick;
    Time last_update_time;
    /** Last supplied update clocks; used when resetting a standalone player. */
    Tick observed_tick;
    Time observed_time;
    Tick ticks_per_frame;
    Time time_per_frame;
} AnimationPlayer;

/** Player configuration only; index and clocks remain runtime state.
 * Zero timing disables that advancement clock. Both clocks retain their existing
 * either-threshold stepping behavior. The animation handle is borrowed. */
typedef struct AnimationPlayerDescription {
    AnimationAsset animation;
    Tick ticks_per_frame;
    Time time_per_frame;
} AnimationPlayerDescription;
ERROR_DECLARE_RESULT_TYPE(AnimationPlayerDescriptionResult, AnimationPlayerDescription);
ERROR_DECLARE_RESULT_TYPE(AnimationPlayerResult, AnimationPlayer);

/** Horizontal facing direction for sprite drawing. */
typedef enum {DIRECTION_LEFT, DIRECTION_RIGHT} Direction;

/** Runtime static sprite state attached to an entity. */
typedef struct SpriteDescription {
    TextureAsset texture;
    Direction direction;
    Scale scale;
    Position body_offset;
    Orientation orientation_offset;
    bool follow_entity_rotation;
    bool visible;
} SpriteDescription;
/** Stored static sprite configuration; has no separate playback state. */
typedef SpriteDescription Sprite;
ERROR_DECLARE_RESULT_TYPE(SpriteDescriptionResult, SpriteDescription);

MEMORY_DECLARE_OBJECT_POOL(SpritePool, Sprite);
extern SpritePool sprites_pool;
#define sprite_components sprites_pool.objects

/** Runtime animated sprite state. */
typedef struct {
    /** Per-sprite playback state for a shared animation asset. */
    AnimationPlayer player;
    /** Instance frame values; value construction borrows images, ECS insertion retains them. */
    AnimationFrameDescription frames[MAX_ANIMATIONS_FRAMES];
    size_t frame_count;
    /** Current draw direction. */
    Direction direction;
    /** Draw scale. */
    Scale scale;
    /** Local position relative to the attached entity. */
    Position body_offset;
    /** Additional orientation relative to the attached entity. */
    Orientation orientation_offset;
    /** Whether drawing inherits the attached entity's orientation. */
    bool follow_entity_rotation;
    /** Whether bulk sprite drawing includes this sprite. */
    bool visible;
} AnimatedSprite;

/** Complete instance configuration. Frames are copied and authoritative;
 * zero frame_count means empty even when a catalog is supplied. The catalog
 * supplies identity/provenance, not implicit replacement frames or timing.
 * Zero frame IDs normalize to their one-based index. Angles are degrees. */
typedef struct AnimatedSpriteDescription {
    AnimationPlayerDescription player;
    AnimationFrameDescription frames[MAX_ANIMATIONS_FRAMES];
    size_t frame_count;
    Direction direction;
    Scale scale;
    Position body_offset;
    Orientation orientation_offset;
    bool follow_entity_rotation;
    bool visible;
} AnimatedSpriteDescription;
ERROR_DECLARE_RESULT_TYPE(AnimatedSpriteDescriptionResult, AnimatedSpriteDescription);
ERROR_DECLARE_RESULT_TYPE(AnimatedSpriteResult, AnimatedSprite);

/** Result type for sprite orientation offsets. */
ERROR_DECLARE_RESULT_TYPE(SpriteOrientationResult, Orientation);

/** Fixed set of animated sprites. */
typedef struct {
    /** Sprite entries. */
    AnimatedSprite sprite_set[MAX_ANIMATION_SETS];
    /** Number of valid sprite entries. */
    uint8_t amount_of_sets;
} AnimatedSpriteSet;

/** Pool storing animated sprites by EntityIndex. */
MEMORY_DECLARE_OBJECT_POOL(AnimatedSpritePool, AnimatedSprite);

/** Pool backing the animated_sprites table. */
extern AnimatedSpritePool animated_sprites_pool;

/** Animated sprite table indexed by EntityIndex. */
#define animated_sprites animated_sprites_pool.objects

/** Shape fill mode for debug drawing. */
typedef enum Fill {
    /** Draw only the outline. */
    GRAPHICS_OUTLINE,
    /** Draw filled geometry. */
    GRAPHICS_FILLED
} Fill;

/**
 * Create a color from a 32-bit RRGGBBAA hex value.
 */
Color graphics_color_hex_create(uint32_t hex_color_code);

/**
 * Create the SDL window and renderer.
 */
EngineResult graphics_start(void);

/**
 * Destroy graphics resources and stop SDL video.
 */
void graphics_stop(void);

/**
 * Poll graphics events and report whether the window should remain open.
 */
bool graphics_events_poll(SDL_Event *event);

/** Clear the render target with a background color. */
void graphics_background_draw(Color color);

/** Create a uniquely named layer. Different layers may share a numeric value. */
GraphicsLayerIdResult graphics_layer_create(LayerDescription description);
LayerDescription graphics_layer_description_default_get(void);
EngineResult graphics_layer_description_set(GraphicsLayerId layer, LayerDescription description);
LayerDescriptionResult graphics_layer_description_get(GraphicsLayerId layer);
EngineResult graphics_layer_destroy(GraphicsLayerId layer);
EngineResult graphics_layer_set(GraphicsLayerId layer, int value);
GraphicsLayerValueResult graphics_layer_get(GraphicsLayerId layer);
EngineResult graphics_layer_name_set(const char *name, int value);
GraphicsLayerValueResult graphics_layer_name_get(const char *name);
GraphicsLayerIdResult graphics_layer_name_id_get(const char *name);

EngineResult graphics_layer_entity_set(Entity entity, int value);
EngineResult graphics_layer_entity_id_set(Entity entity, GraphicsLayerId layer);
EngineResult graphics_layer_entity_name_set(Entity entity, const char *name);
GraphicsLayerValueResult graphics_layer_entity_get(Entity entity);
GraphicsLayerIdResult graphics_layer_entity_id_get(Entity entity);
EngineResult graphics_layer_entity_clear(Entity entity);
EngineResult graphics_layer_sprite_set(Entity entity, int value);
EngineResult graphics_layer_sprite_id_set(Entity entity, GraphicsLayerId layer);
EngineResult graphics_layer_sprite_name_set(Entity entity, const char *name);
GraphicsLayerValueResult graphics_layer_sprite_get(Entity entity);
GraphicsLayerIdResult graphics_layer_sprite_id_get(Entity entity);
EngineResult graphics_layer_sprite_clear(Entity entity);
EngineResult graphics_layer_animation_set(Entity entity, int value);
EngineResult graphics_layer_animation_id_set(Entity entity, GraphicsLayerId layer);
EngineResult graphics_layer_animation_name_set(Entity entity, const char *name);
GraphicsLayerValueResult graphics_layer_animation_get(Entity entity);
GraphicsLayerIdResult graphics_layer_animation_id_get(Entity entity);
EngineResult graphics_layer_animation_clear(Entity entity);

EngineResult graphics_layer_ui_set(ViewportItemId item, int value);
EngineResult graphics_layer_ui_id_set(ViewportItemId item, GraphicsLayerId layer);
EngineResult graphics_layer_ui_name_set(ViewportItemId item, const char *name);
GraphicsLayerValueResult graphics_layer_ui_get(ViewportItemId item);
GraphicsLayerIdResult graphics_layer_ui_id_get(ViewportItemId item);
EngineResult graphics_layer_ui_clear(ViewportItemId item);

/** Set the explicit ordering layer captured by subsequent direct draw commands. */
void graphics_layer_active_set(int value);
int graphics_layer_active_get(void);
EngineResult graphics_layer_active_id_set(GraphicsLayerId layer);
EngineResult graphics_layer_active_name_set(const char *name);

/** Draw a filled rectangle in logical screen coordinates. */
bool graphics_screen_rect_draw(float x, float y, float width, float height, Color color);
bool graphics_screen_shape_filled_draw(Shape shape, Color color);
bool graphics_screen_clip_set(float x, float y, float width, float height);
void graphics_screen_clip_clear(void);
bool graphics_screen_clip_push(float x, float y, float width, float height);
void graphics_screen_clip_pop(void);

/** Draw a centered, rotated rectangle in logical screen coordinates. */
bool graphics_screen_quad_draw(
    Position center,
    float width,
    float height,
    float angle,
    Color color
);
/** Clockwise-positive radians alternative; other parameters retain their units. */
bool graphics_screen_quad_radians_draw(
    Position center,
    float width,
    float height,
    float angle,
    Color color
);

/** Return a composed frame report. Recording failure still permits presentation.
 * Standalone order: cameras, commands, viewports, recording, presentation.
 * Frame pipelines must not be called recursively from camera callbacks.
 */
Report graphics_show(void);
Report graphics_viewport_show(ViewportId viewport);
Report graphics_viewports_show(const ViewportId *viewports, size_t count);
Report graphics_pipeline_cameras_update_for_viewports(const ViewportId *viewports, size_t count);
Report graphics_pipeline_viewports_draw_selected(const ViewportId *viewports, size_t count);
/** Update camera motion and queue selected screen content. */
Report graphics_pipeline_cameras_update(void);
/** Consume pending submission diagnostics and execute queued commands. */
Report graphics_pipeline_commands_execute(void);
/** Compose screens and viewport UI after command execution. */
Report graphics_pipeline_viewports_draw(void);
/** Capture the composed frame; inactive recording returns NOT_RUN. */
Report graphics_pipeline_recording_update(void);
/** Apply frame limiting and present the composed frame. Software limiting spaces
 * presentation starts, including prior presentation time in the frame budget.
 * Late frames present immediately without accumulating catch-up work.
 */
Report graphics_pipeline_present(void);
/** VSync owns pacing when enabled; a failed change preserves the current mode. */
EngineResult graphics_vsync_set(bool enabled);
/** Software limit used with VSync off. Zero selects the 120 FPS fallback;
 * negative values are rejected. Changing the limit resets the pacing interval.
 */
EngineResult graphics_frame_limit_set(int frames_per_second);

/** Draw one entity hitbox. */
void graphics_hit_box_draw(Entity entity, Fill fill_type);

/** Draw one entity hitbox with a caller supplied color. */
void graphics_hit_box_colored_draw(Entity entity, Fill fill_type, Color color);

/** Draw every live entity hitbox. */
void graphics_hit_boxes_draw(void);

/** Load or share a texture and return one caller-owned reference. */
TextureAssetResult graphics_texture_load(TextureDescription text_desc);
/** Zero defaults; a file is required to load. */
TextureDescription graphics_texture_description_default_get(void);
/** Inspect without retaining. Paths remain borrowed until this resource
 * loses its final owning reference or graphics shuts down.
 * Logical size comes from the supplied asset value. */
TextureDescriptionResult graphics_texture_description_get(TextureAsset asset);
/** Add one owning reference to a loaded texture asset. */
EngineResult graphics_texture_retain(TextureAsset asset);
/**
 * Release one owning reference and clear the caller's asset value. Copying a
 * TextureAsset does not retain it; explicitly retain every independently
 * owned copy.
 */
EngineResult graphics_texture_release(TextureAsset *asset);
/** Return whether a texture asset currently names a live owned resource. */
bool graphics_texture_valid_check(TextureAsset asset);
void graphics_texture_draw(TextureAsset texture, Position position,
    Orientation orientation);
/** Clockwise-positive radians alternative; other parameters retain their units. */
void graphics_texture_radians_draw(TextureAsset texture, Position position,
    Orientation orientation);
void graphics_screen_texture_draw(TextureAsset texture, Position center,
    Scale size, Orientation orientation);
/** Clockwise-positive radians alternative; other parameters retain their units. */
void graphics_screen_texture_radians_draw(TextureAsset texture, Position center,
    Scale size, Orientation orientation);

/** Load or share a font and return one owning reference. */
FontAssetResult graphics_font_load(FontDescription descriptor);
/** Zero defaults select the engine-pinned built-in font. */
FontDescription graphics_font_description_default_get(void);
/** Inspect without retaining. Paths remain borrowed until this resource
 * loses its final owning reference or graphics shuts down.
 * Built-in inspection returns file NULL and point_size zero. */
FontDescriptionResult graphics_font_description_get(FontAsset asset);

/** Return the engine-pinned built-in font for this graphics lifetime. */
FontAsset graphics_font_default_get(void);

/** Add one owning reference to a loaded custom font; built-in retains are no-ops. */
EngineResult graphics_font_retain(FontAsset asset);
/** Release one owning reference and clear the caller's asset value. */
EngineResult graphics_font_release(FontAsset *asset);
/** Return whether a font asset names a live resource. */
bool graphics_font_valid_check(FontAsset asset);

/** Create reusable text. An empty string produces an empty text asset. */
TextAssetResult graphics_text_create(TextDescription description);
/** Built-in font, empty string, opaque white, and no wrapping. */
TextDescription graphics_text_description_default_get(void);
/** Atomic full replacement; updates dimensions without changing the handle.
 * Failure preserves configuration, dependencies, and queued payloads. */
EngineResult graphics_text_description_set(TextAsset *text, TextDescription description);
/** Borrowed font and string; no retain. Valid until successful changed
 * configuration, public text destruction, or graphics shutdown. */
TextDescriptionResult graphics_text_description_get(TextAsset text);
bool graphics_text_value_set(TextAsset *text, const char *value);
/** Set text wrapping width in logical pixels; zero disables wrapping. */
bool graphics_text_wrap_width_set(TextAsset *text, int wrap_width);

/** Destroy the unique text owner and clear its value. */
EngineResult graphics_text_destroy(TextAsset *text);
/** Return whether a text asset names a live publicly owned instance. */
bool graphics_text_valid_check(TextAsset text);

/** Draw reusable text with its top-left corner in logical screen space. */
bool graphics_text_draw(const TextAsset *text, Position position);
bool graphics_text_scaled_draw(const TextAsset *text, Position position, Scale scale);
bool graphics_screen_text_scaled_rotated_draw(const TextAsset *text,
    Position center, Scale scale, Orientation orientation);
/** Clockwise-positive radians alternative; other parameters retain their units. */
bool graphics_screen_text_scaled_rotated_radians_draw(const TextAsset *text,
    Position center, Scale scale, Orientation orientation);

/** Load or share an immutable animation and return one owning reference. */
AnimationAssetResult graphics_animation_load(AnimationDescription anim_desc);
/** Zero defaults create an empty catalog; zero IDs normalize on load. */
AnimationDescription graphics_animation_description_default_get(void);
/** Inspect without retaining. Paths remain borrowed until this resource
 * loses its final owning reference or graphics shuts down.
 * Frame arrays are copied; paths are borrowed from retained frame textures. */
AnimationDescriptionResult graphics_animation_description_get(AnimationAsset asset);


/** Scale one, facing right, visible, and following entity rotation. */
SpriteDescription graphics_sprite_description_default_get(void);
/** Replace all configuration atomically; the component must already exist. */
EngineResult graphics_sprite_description_set(Entity entity, SpriteDescription description);
/** Copied configuration with borrowed texture; no retain. */
SpriteDescriptionResult graphics_sprite_description_get(Entity entity);
/** Scale one with zero ID, offset, and rotation. */
AnimationFrameDescription graphics_animation_frame_description_default_get(void);
/** No catalog and zero timing. */
AnimationPlayerDescription graphics_animation_player_description_default_get(void);
/** Borrowed catalog and its timing; zero handle selects empty defaults. */
AnimationPlayerDescriptionResult graphics_animation_player_description_from_animation_get(AnimationAsset animation);
/** Preserves clocks for the same catalog; a changed catalog resets to frame zero
 * at the last observed update clocks. No resource retain or release. */
EngineResult graphics_animation_player_description_set(AnimationPlayer *player, AnimationPlayerDescription description);
/** Configuration only; the caller keeps the borrowed catalog alive. */
AnimationPlayerDescriptionResult graphics_animation_player_description_get(const AnimationPlayer *player);
/** Empty frames, scale one, facing right, visible, following entity rotation. */
AnimatedSpriteDescription graphics_animated_sprite_description_default_get(void);
/** Copies catalog frames and timing into independent instance configuration.
 * Images and catalog are borrowed until catalog release or graphics shutdown. */
AnimatedSpriteDescriptionResult graphics_animated_sprite_description_from_animation_get(AnimationAsset animation);
/** Copied value configuration. Images/catalog remain borrowed from the value's owners. */
AnimatedSpriteDescription graphics_animated_sprite_value_description_get(AnimatedSprite sprite);
/** For standalone unowned values only; validates atomically without retaining.
 * Preserve current frame by ID and clocks. Removed current ID or changed catalog
 * resets to frame zero at the last observed update clocks. */
EngineResult graphics_animated_sprite_value_description_set(AnimatedSprite *sprite, AnimatedSpriteDescription description);
/** Atomic component replacement, acquiring new references before releasing old.
 * Same preservation rules as value replacement; resets use engine clocks. */
EngineResult graphics_animated_sprite_description_set(Entity entity, AnimatedSpriteDescription description);
/** Copied frames and borrowed handles; valid until replacement/removal/deletion
 * or graphics shutdown. The getter never retains. */
AnimatedSpriteDescriptionResult graphics_animated_sprite_description_get(Entity entity);

/** Native pixel dimensions, independent of TextureAsset.size. */
TextureSizeResult graphics_texture_size_get(TextureAsset asset);
/** Non-owning value builder; copies fields and resolves native texture size.
 * It does not validate or retain; add/set validates before publication. */
AnimationFrameDescription graphics_animation_frame_create(AnimationFrameDescription description);
/** Non-owning builders: copy values; keep image owners alive until component add.
 * Use only on unattached values, never directly on an installed component.
 * ID zero assigns an unused ID on add and preserves the current ID on set.
 * Replacing the current ID resets playback to the first frame. */
EngineResult graphics_animated_sprite_value_frame_add(AnimatedSprite *sprite, AnimationFrameDescription frame);
EngineResult graphics_animated_sprite_value_frame_description_set(AnimatedSprite *sprite, size_t index, AnimationFrameDescription frame);
/** Get a copied instance frame with a borrowed image. Do not release its texture. */
AnimationFrameDescriptionResult graphics_animated_sprite_frame_description_get(Entity entity, size_t index);
/** Instance mutation retains images and rolls back on error. Playback is
 * preserved unless the current frame ID changes, which resets to frame zero.
 * Signed scale flips content; zero collapses the image. All transforms must be finite. */
EngineResult graphics_animated_sprite_frame_description_set(Entity entity, size_t index, AnimationFrameDescription frame);
EngineResult graphics_animated_sprite_frame_add(Entity entity, AnimationFrameDescription frame);
EngineResult graphics_animated_sprite_frame_scale_set(Entity entity, size_t index, Scale scale);
EngineResult graphics_animated_sprite_frame_offset_set(Entity entity, size_t index, Position offset);
EngineResult graphics_animated_sprite_frame_rotation_set(Entity entity, size_t index, Orientation rotation);
EngineResult graphics_animated_sprite_frame_rotation_radians_set(Entity entity, size_t index, Orientation rotation);
TextureSizeResult graphics_animated_sprite_frame_scale_get(Entity entity, size_t index);
PositionResult graphics_animated_sprite_frame_offset_get(Entity entity, size_t index);
SpriteOrientationResult graphics_animated_sprite_frame_rotation_get(Entity entity, size_t index);
SpriteOrientationResult graphics_animated_sprite_frame_rotation_radians_get(Entity entity, size_t index);
/** Add one owning reference to a loaded animation asset. */
EngineResult graphics_animation_retain(AnimationAsset asset);
/** Release one owning reference and clear the caller's asset value. */
EngineResult graphics_animation_release(AnimationAsset *asset);
/** Return whether an animation asset names a live owned resource. */
bool graphics_animation_valid_check(AnimationAsset asset);
/** Return immutable metadata stored for an animation asset. */
AnimationInfoResult graphics_animation_info_get(AnimationAsset asset);
/** Return a reusable catalog frame with scale one and a borrowed texture. */
AnimationFrameDescriptionResult graphics_animation_frame_description_get(AnimationAsset asset,
    size_t frame_index);

/** Create independent mutable playback state for a shared animation. */
AnimationPlayerResult graphics_animation_player_create(AnimationPlayerDescription description);
/** Advance one player's frame state. */
void graphics_animation_player_update(AnimationPlayer *player,
    Tick current_tick, Time current_time);

/** Validate and create non-owning sprite runtime state from a description. */
AnimatedSpriteResult graphics_animated_sprite_create(AnimatedSpriteDescription description);

/** Advance one animated sprite's frame state. */
void graphics_animated_sprite_update(AnimatedSprite *sprite, Tick current_tick,
    Time current_time);

/** Attach an animated sprite to an entity. */
EngineResult graphics_animated_sprite_add(Entity entity, AnimatedSpriteDescription description);
EngineResult graphics_animated_sprite_frame_index_set(Entity entity,
    size_t frame_index);
AnimationFrameIndexResult graphics_animated_sprite_frame_index_get(Entity entity);

/** Copy static sprite configuration into a non-owning value; add/set validates. */
Sprite graphics_sprite_create(SpriteDescription description);

/** Attach a static sprite to an entity. */
EngineResult graphics_sprite_add(Entity entity, SpriteDescription description);

/** Set the local position offset of an attached static sprite. */
EngineResult graphics_sprite_body_offset_set(Entity entity, Position offset);
/** Get the local position offset of an attached static sprite. */
PositionResult graphics_sprite_body_offset_get(Entity entity);
/** Set the orientation offset of an attached static sprite. */
EngineResult graphics_sprite_orientation_offset_set(Entity entity,
    Orientation offset);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult graphics_sprite_orientation_offset_radians_set(Entity entity,
    Orientation offset);
/** Get the orientation offset of an attached static sprite. */
SpriteOrientationResult graphics_sprite_orientation_offset_get(Entity entity);
/** Clockwise-positive radians alternative; other parameters retain their units. */
SpriteOrientationResult graphics_sprite_orientation_offset_radians_get(Entity entity);

/** Set the local position offset of an attached animated sprite. */
EngineResult graphics_animated_sprite_body_offset_set(Entity entity,
    Position offset);
/** Get the local position offset of an attached animated sprite. */
PositionResult graphics_animated_sprite_body_offset_get(Entity entity);
/** Set the orientation offset of an attached animated sprite. */
EngineResult graphics_animated_sprite_orientation_offset_set(Entity entity,
    Orientation offset);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult graphics_animated_sprite_orientation_offset_radians_set(Entity entity,
    Orientation offset);
/** Get the orientation offset of an attached animated sprite. */
SpriteOrientationResult graphics_animated_sprite_orientation_offset_get(
    Entity entity);
/** Clockwise-positive radians alternative; other parameters retain their units. */
SpriteOrientationResult graphics_animated_sprite_orientation_offset_radians_get(
    Entity entity);

/** Draw the static sprite attached to one entity. */
bool graphics_sprite_draw(Entity entity);

/** Draw all live static sprites. */
void graphics_sprites_draw(void);

/** Draw the animated sprite attached to one entity. */
bool graphics_animated_sprite_draw(Entity entity);

/** Draw all live animated sprites. */
void graphics_animated_sprites_draw(void);

/** Update frame state for all live animated sprites. */
Report graphics_sprite_frames_update(Tick current_tick, Time current_time);

/** Scale an entity's animated sprite textures. */
void graphics_textures_scale(Entity entity, Scale scale);

/** Translate the active camera in world space. */
void graphics_camera_move(Vec2D translation);

/** Rotate the active camera clockwise by degrees. */
void graphics_camera_rotate(Orientation degrees);
/** Clockwise-positive radians alternative; other parameters retain their units. */
void graphics_camera_radians_rotate(Orientation radians);

/** Attach the active camera; all angles are clockwise degrees. */
EngineResult graphics_camera_attach(CameraAttachmentDescription description);
/** Explicitly detach the active camera, preserving its last resolved world view. */
void graphics_camera_detach(void);
/** Whether the active camera can currently resolve its retained attachment. */
bool graphics_camera_attached_get(void);

/** Enabled, full-window dimensions, unit zoom, rendering allowed while paused. */
CameraDescription graphics_camera_description_default_get(void);
CameraIdResult graphics_camera_create(CameraDescription description);
EngineResult graphics_camera_destroy(CameraId camera);
EngineResult graphics_camera_active_set(CameraId camera);
CameraId graphics_camera_active_get(void);
/** Resolved world view; unavailable attachment targets leave the last view frozen. */
CameraResult graphics_camera_get(CameraId camera);
/** Copy current resolved configuration; callback context remains caller-owned. */
CameraDescriptionResult graphics_camera_description_get(CameraId camera);
/** Atomic replacement. World edits adjust attachment offsets, never detach.
 * Unchanged fields preserve movement; position/zoom edits replace only that motion.
 * Zero dimensions/zoom use defaults; negative/nonfinite values are rejected. */
EngineResult graphics_camera_description_set(CameraId camera, CameraDescription description);
/** Follow both position and orientation; fill the required non-owning entity handle. */
CameraAttachmentDescription graphics_camera_attachment_description_default_get(void);
/** Validate and replace attachment configuration. Unchanged edits preserve motion. */
EngineResult graphics_camera_attachment_description_set(CameraId camera,
    CameraAttachmentDescription description);
/** Copy the stored attachment, even if its target is temporarily unavailable. */
CameraAttachmentDescriptionResult graphics_camera_attachment_description_get(CameraId camera);
EngineResult graphics_camera_attachment_remove(CameraId camera);

/** Register the drawing callback invoked automatically when this camera is needed. */
EngineResult graphics_camera_render_callback_set(
    CameraId camera,
    CameraRenderCallback callback,
    void *context
);
EngineResult graphics_camera_enable_set(CameraId camera);
EngineResult graphics_camera_disable_set(CameraId camera);
/** Skip camera rendering while the engine is paused. */
EngineResult graphics_camera_pause_with_engine_set(CameraId camera);
/** Continue camera rendering while paused; this is the default. */
EngineResult graphics_camera_render_when_paused_set(CameraId camera);
/** Move relatively over engine-tick time; non-positive duration is immediate. */
EngineResult graphics_camera_position_move(CameraId camera, Vec2D translation, Time duration);
/** Move over engine-tick time. Attached cameras interpolate an offset destination
 * computed from this world position at acceptance, retaining target following. */
EngineResult graphics_camera_position_set(CameraId camera, Position position, Time duration);
/** Move toward an entity's current position without attaching. */
EngineResult graphics_camera_position_from_entity_set(CameraId camera, Entity entity, Time duration);
/** Immediately attach to and follow an entity's position. */
EngineResult graphics_camera_entity_attachment_set(CameraId camera, Entity entity);
/** Return success with true while a timed movement is active. */
EngineResult graphics_camera_moving_get(CameraId camera);
/** Scale over engine-tick time; non-positive duration is immediate. */
EngineResult graphics_camera_zoom_set(CameraId camera, float zoom, Time duration);
CameraZoomResult graphics_camera_zoom_get(CameraId camera);

/** Return a disabled, full-window viewport using contain fitting. */
ViewportDescription graphics_viewport_description_default_get(void);
ViewportItemDescription graphics_viewport_item_description_default_get(void);
/** Defaults use the active camera and window dimensions. Zero dimensions normalize;
 * negative dimensions fail without mutation. Camera IDs are borrowed: missing
 * cameras show a construction placeholder and warn during viewport composition.
 * Destroying a camera preserves screens; reassign their camera explicitly. */
ScreenDescription graphics_screen_description_default_get(void);
ScreenIdResult graphics_screen_create(ScreenDescription config);
EngineResult graphics_screen_description_set(ScreenId screen, ScreenDescription description);
ScreenDescriptionResult graphics_screen_description_get(ScreenId screen);
EngineResult graphics_screen_destroy(ScreenId screen);
EngineResult graphics_screen_camera_set(ScreenId screen, CameraId camera);
ViewportIdResult graphics_viewport_create(ViewportDescription config);
EngineResult graphics_viewport_description_set(ViewportId viewport, ViewportDescription description);
ViewportDescriptionResult graphics_viewport_description_get(ViewportId viewport);
/** Mount exactly one existing screen or UI reference without transferring ownership.
 * Full item replacement preserves the handle, acquiring new dependencies before release.
 * Content replacement, hiding, or disabling cancels incompatible pointer interaction. */
ViewportItemIdResult graphics_viewport_item_add(ViewportId viewport, ViewportItemDescription description);
EngineResult graphics_viewport_destroy(ViewportId viewport);
ViewportItemIdResult graphics_viewport_camera_add(ViewportId viewport,
    CameraId camera, ViewportItemDescription config);
ViewportItemIdResult graphics_viewport_screen_add(ViewportId viewport,
    ScreenId screen, ViewportItemDescription config);
ViewportUiShapeDescription graphics_ui_shape_description_default_get(void);
GraphicsUiIdResult graphics_ui_shape_create(ViewportUiShapeDescription shape);
ViewportUiTextDescription graphics_ui_text_description_default_get(void);
GraphicsUiIdResult graphics_ui_text_create(ViewportUiTextDescription text);
ViewportUiSliderDescription graphics_ui_slider_description_default_get(void);
GraphicsUiIdResult graphics_ui_slider_create(ViewportUiSliderDescription slider);
EngineResult graphics_ui_shape_description_set(GraphicsUiId ui, ViewportUiShapeDescription shape);
ViewportUiShapeDescriptionResult graphics_ui_shape_description_get(GraphicsUiId ui);
EngineResult graphics_ui_text_description_set(GraphicsUiId ui, ViewportUiTextDescription text);
ViewportUiTextDescriptionResult graphics_ui_text_description_get(GraphicsUiId ui);
EngineResult graphics_ui_slider_description_set(GraphicsUiId ui, ViewportUiSliderDescription slider);
ViewportUiSliderDescriptionResult graphics_ui_slider_description_get(GraphicsUiId ui);
EngineResult graphics_ui_slider_value_set(GraphicsUiId ui, float value);
GraphicsUiSliderValueResult graphics_ui_slider_value_get(GraphicsUiId ui);
EngineResult graphics_ui_destroy(GraphicsUiId ui);
ViewportItemIdResult graphics_viewport_ui_add(ViewportId viewport,
    GraphicsUiId ui, ViewportItemDescription config);
/** Convenience creation: the new UI definition is automatically cleaned up after
 * its final mount is removed or replaced, including additional shared mounts. */
ViewportItemIdResult graphics_viewport_ui_shape_add(ViewportId viewport,
    ViewportUiShapeDescription shape, ViewportItemDescription config);
ViewportItemIdResult graphics_viewport_ui_text_add(ViewportId viewport,
    ViewportUiTextDescription text, ViewportItemDescription config);
ViewportItemIdResult graphics_viewport_ui_slider_add(ViewportId viewport,
    ViewportUiSliderDescription slider, ViewportItemDescription config);
EngineResult graphics_viewport_item_remove(ViewportItemId item);
EngineResult graphics_viewport_item_description_set(ViewportItemId item,
    ViewportItemDescription config);
ViewportItemDescriptionResult graphics_viewport_item_description_get(ViewportItemId item);
ViewportIdResult graphics_viewport_item_viewport_get(ViewportItemId item);
/** Return whether this mounted UI instance was hovered during its latest draw. */
bool graphics_viewport_ui_hovered_check(ViewportItemId item);
/** Return whether this mounted UI instance was pressed during its latest draw. */
bool graphics_viewport_ui_pressed_check(ViewportItemId item);
/** Return whether this slider's value changed during its latest draw. */
bool graphics_viewport_ui_slider_changed_check(ViewportItemId item);
EngineResult graphics_viewport_item_drag_mode_set(ViewportItemId item,
    ViewportItemDragMode mode);
ViewportItemDragModeResult graphics_viewport_item_drag_mode_get(ViewportItemId item);
bool graphics_viewport_item_dragging_check(ViewportItemId item);
/** Assign a camera without transferring ownership. */
EngineResult graphics_viewport_camera_set(ViewportId viewport, CameraId camera);
EngineResult graphics_viewport_camera_clear(ViewportId viewport);
EngineResult graphics_viewport_enable_set(ViewportId viewport);
EngineResult graphics_viewport_disable_set(ViewportId viewport);

/** Convert world coordinates to logical screen coordinates. */
Position graphics_world_to_screen_get(Position pos);

/** Convert logical screen coordinates to world coordinates. */
Position graphics_screen_to_world_get(Position screen);

/** Convert SDL window coordinates to logical screen coordinates. */
Position graphics_window_to_screen_get(Position window);
Scale graphics_render_output_size_get(void);
bool graphics_logical_size_set(int width, int height);
bool graphics_aspect_ratio_set(int width, int height);
bool graphics_aspect_ratio_auto_set(bool enabled);
WindowPresentationDescription graphics_window_presentation_description_default_get(void);
WindowPresentationDescription graphics_window_presentation_description_get(void);
EngineResult graphics_window_presentation_description_set(
    WindowPresentationDescription config);

/** Get the mouse position in logical screen coordinates. */
Position graphics_mouse_screen_position_get(void);

/** Enable or disable AABB-tree debug drawing. */
void graphics_aabb_tree_debug_set(bool enabled);
/** Global diagnostic overlays for automatic screens; disabled at graphics startup. */
void graphics_hit_boxes_debug_set(bool enabled);
bool graphics_hit_boxes_debug_check(void);
void graphics_particles_debug_set(bool enabled);
bool graphics_particles_debug_check(void);
void graphics_local_origins_debug_set(bool enabled);
bool graphics_local_origins_debug_check(void);
/** Return whether AABB-tree debug drawing is enabled. */
bool graphics_aabb_tree_debug_check(void);
/** Draw the current physics AABB-tree bounds when enabled. */
void graphics_aabb_tree_draw(void);
/** Enable or disable current-contact manifold debug drawing. */
void graphics_contacts_debug_set(bool enabled);
/** Return whether current-contact manifold debug drawing is enabled. */
bool graphics_contacts_debug_check(void);
/** Draw current contact manifold points and normals when enabled. */
void graphics_contacts_draw(void);

/** Start recording frames to an output file through ffmpeg. */
bool graphics_recording_start(
    const char *output_path,
    int fps
);
/** Stop recording frames and close the active ffmpeg process. */
void graphics_recording_stop(void);

/** Draw one particle entity using its collision circle. */
void graphics_particle_draw(Entity entity, Fill fill_type);
/** Draw all particle entities. */
void graphics_particles_draw(void);

/** Draw local origin axes for all live hitbox entities. */
void graphics_local_origins_draw(void);
/** Draw one joint using its engineering-style debug symbol. */
bool graphics_joint_draw(Entity joint, Color color);
/** Draw every live joint using engineering-style debug symbols. */
void graphics_joints_draw(Color color);
/** Draw a soft body's deforming surfaces, beams, and collision nodes. */
bool graphics_soft_body_draw(Entity soft_body, Color surface, Color beam, Color node);
EngineResult graphics_soft_body_area_color_set(Entity area, Color color);
EngineResult graphics_soft_body_area_color_clear(Entity area);
EngineResult graphics_soft_body_area_visibility_set(Entity area, bool visible);

EngineResult graphics_soft_body_node_color_set(Entity soft_body, Entity node, Color color);
EngineResult graphics_soft_body_beam_color_set(
    Entity soft_body, Entity node_a, Entity node_b, Color color);
/* Preserve the by-value symbol, including function-pointer callers. */
#define graphics_viewport_create(...) _Generic((__VA_ARGS__), \
    ViewportDescription: (graphics_viewport_create), \
    default: graphics_viewport_create_description)(__VA_ARGS__)
#endif
