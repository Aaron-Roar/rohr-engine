/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_EDITOR_COMMAND_H
#define ROHR_EDITOR_COMMAND_H

#include "editor_auto_shape.h"
#include "editor_object_commands.h"

typedef enum EditorCommandType {
    EDITOR_COMMAND_OBJECT_ADD,
    EDITOR_COMMAND_OBJECT_RENAME,
    EDITOR_COMMAND_OBJECT_REMOVE,
    EDITOR_COMMAND_OBJECT_POSITION,
    EDITOR_COMMAND_RIGID_BODY_TRANSFORM,
    EDITOR_COMMAND_VERTEX_POSITION,
    EDITOR_COMMAND_ANCHOR_TRANSFORM,
    EDITOR_COMMAND_SOFT_BODY_TRANSFORM,
    EDITOR_COMMAND_SOFT_NODE_POSITION,
    EDITOR_COMMAND_CAMERA_TRANSFORM,
    EDITOR_COMMAND_CAMERA_DIMENSIONS_SET,
    EDITOR_COMMAND_CAMERA_ZOOM_SET,
    EDITOR_COMMAND_CAMERA_ATTACHMENT_SET,
    EDITOR_COMMAND_AUTO_SHAPE,
    EDITOR_COMMAND_RIGID_BODY_ORIGIN,
    EDITOR_COMMAND_SOFT_BODY_ORIGIN,
    EDITOR_COMMAND_VIEWPORT_CAMERA,
    EDITOR_COMMAND_VIEWPORT_COORDINATES,
    EDITOR_COMMAND_PROJECT_PHYSICS_SETTINGS_SET,
    EDITOR_COMMAND_SIMULATION_OBJECT_SET,
    EDITOR_COMMAND_SIMULATION_VIEWPORT_SET,
    EDITOR_COMMAND_VISIBILITY,
    EDITOR_COMMAND_NAVIGATION_SET,
    EDITOR_COMMAND_ITEM_ADD,
    EDITOR_COMMAND_ITEM_REMOVE,
    EDITOR_COMMAND_ITEM_RENAME,
    EDITOR_COMMAND_PROPERTY_SET,
    EDITOR_COMMAND_RELATIONSHIP_SET,
    EDITOR_COMMAND_COLLISION_MASK_ADD,
    EDITOR_COMMAND_COLLISION_FILTER_SET,
    EDITOR_COMMAND_INPUT_CONTROLLER_ADD,
    EDITOR_COMMAND_INPUT_CONTROLLER_REMOVE,
    EDITOR_COMMAND_INPUT_CONTROLLER_SET,
    EDITOR_COMMAND_INPUT_ACTION_ADD,
    EDITOR_COMMAND_INPUT_ACTION_REMOVE,
    EDITOR_COMMAND_INPUT_ACTION_SET,
    EDITOR_COMMAND_INPUT_BINDING_ADD,
    EDITOR_COMMAND_INPUT_BINDING_REMOVE,
    EDITOR_COMMAND_INPUT_BINDING_SET,
    EDITOR_COMMAND_SPRITE_ADD,
    EDITOR_COMMAND_SPRITE_REMOVE,
    EDITOR_COMMAND_SPRITE_RENAME,
    EDITOR_COMMAND_SPRITE_PATH_SET,
    EDITOR_COMMAND_SPRITE_POSITION_SET,
    EDITOR_COMMAND_SPRITE_ROTATION_SET,
    EDITOR_COMMAND_SPRITE_SIZE_SET,
    EDITOR_COMMAND_SPRITE_BODY_SET,
    EDITOR_COMMAND_SPRITE_FOLLOW_ROTATION_SET,
    EDITOR_COMMAND_SPRITE_VISIBILITY_SET,
    EDITOR_COMMAND_ANIMATED_SPRITE_ADD,
    EDITOR_COMMAND_ANIMATED_SPRITE_REMOVE,
    EDITOR_COMMAND_ANIMATED_SPRITE_RENAME,
    EDITOR_COMMAND_ANIMATED_SPRITE_BODY_SET,
    EDITOR_COMMAND_ANIMATED_SPRITE_POSITION_SET,
    EDITOR_COMMAND_ANIMATED_SPRITE_ROTATION_SET,
    EDITOR_COMMAND_ANIMATED_SPRITE_SCALE_SET,
    EDITOR_COMMAND_ANIMATED_SPRITE_TIMING_SET,
    EDITOR_COMMAND_ANIMATED_SPRITE_STARTING_FRAME_SET,
    EDITOR_COMMAND_ANIMATED_SPRITE_DIRECTION_SET,
    EDITOR_COMMAND_ANIMATED_SPRITE_FOLLOW_ROTATION_SET,
    EDITOR_COMMAND_ANIMATED_SPRITE_VISIBILITY_SET,
    EDITOR_COMMAND_ANIMATED_SPRITE_PLAYING_SET,
    EDITOR_COMMAND_ANIMATION_FRAME_ADD,
    EDITOR_COMMAND_ANIMATION_FRAME_REMOVE,
    EDITOR_COMMAND_ANIMATION_FRAME_RENAME,
    EDITOR_COMMAND_ANIMATION_FRAME_PATH_SET,
    EDITOR_COMMAND_ANIMATION_FRAME_SCALE_SET,
    EDITOR_COMMAND_ANIMATION_FRAME_TRANSFORM_SET,
    EDITOR_COMMAND_SOFT_AREA_LOOP_SET,
    EDITOR_COMMAND_SOFT_AREA_ORDER_SET,
    EDITOR_COMMAND_SOFT_AREA_LAYER_SET,
    EDITOR_COMMAND_SOURCE,
    EDITOR_COMMAND_OBJECT_DUPLICATE,
    EDITOR_COMMAND_APP_VISIBILITY,
    EDITOR_COMMAND_APPLICATION_WINDOW_SET,
    EDITOR_COMMAND_AUDIO_ADD,
    EDITOR_COMMAND_AUDIO_SET,
    EDITOR_COMMAND_AUDIO_REMOVE,
    EDITOR_COMMAND_JOINT_PLACE,
    EDITOR_COMMAND_LAYER_SET,
    EDITOR_COMMAND_TRANSFORM_BATCH
} EditorCommandType;

typedef enum EditorItemKind {
    EDITOR_ITEM_OBJECT,
    EDITOR_ITEM_RIGID_BODY,
    EDITOR_ITEM_HITBOX,
    EDITOR_ITEM_JOINT,
    EDITOR_ITEM_ANCHOR,
    EDITOR_ITEM_SOFT_BODY,
    EDITOR_ITEM_SOFT_NODE,
    EDITOR_ITEM_SOFT_BEAM,
    EDITOR_ITEM_VERTEX,
    EDITOR_ITEM_LINE,
    EDITOR_ITEM_CAMERA,
    EDITOR_ITEM_LAYOUT_VIEWPORT,
    EDITOR_ITEM_VIEWPORT_CAMERA,
    EDITOR_ITEM_INPUT_CONTROLLER,
    EDITOR_ITEM_INPUT_ACTION,
    EDITOR_ITEM_INPUT_BINDING,
    EDITOR_ITEM_SPRITE,
    EDITOR_ITEM_ANIMATED_SPRITE,
    EDITOR_ITEM_ANIMATION_FRAME,
    EDITOR_ITEM_SOFT_AREA,
    EDITOR_ITEM_SOFT_HOLE,
    EDITOR_ITEM_FORCE, EDITOR_ITEM_TORQUE, EDITOR_ITEM_VIEWPORT_UI,
    EDITOR_ITEM_UI_FONT, EDITOR_ITEM_UI_DEFINITION
} EditorItemKind;

typedef struct EditorInputControllerCommand {
    EditorInputControllerId controller;
    char name[ROHR_INPUT_NAME_MAX];
    bool enabled;
} EditorInputControllerCommand;

typedef struct EditorInputActionCommand {
    EditorInputControllerId controller;
    EditorInputActionId action;
    char name[ROHR_INPUT_NAME_MAX];
    InputActionType type;
    InputButtonMode button_mode;
    bool button_initial_state;
} EditorInputActionCommand;

typedef struct EditorInputBindingCommand {
    EditorInputControllerId controller;
    EditorInputActionId action;
    EditorInputBindingId binding_id;
    size_t index;
    char name[ROHR_INPUT_NAME_MAX];
    InputBinding binding;
} EditorInputBindingCommand;

typedef struct EditorItemAddCommand {
    EditorItemKind kind;
    EditorObjectId object;
    uint32_t parent;
    uint32_t first;
    uint32_t second;
    uint32_t index;
    uint32_t option;
    Position position;
    char name[EDITOR_OBJECT_NAME_MAX];
} EditorItemAddCommand;

typedef struct EditorItemRemoveCommand {
    EditorItemKind kind;
    EditorObjectId object;
    uint32_t parent;
    uint32_t item;
    uint32_t index;
} EditorItemRemoveCommand;

typedef struct EditorItemRenameCommand {
    EditorItemKind kind;
    EditorObjectId object;
    uint32_t parent;
    uint32_t item;
    uint32_t index;
    char name[EDITOR_OBJECT_NAME_MAX];
} EditorItemRenameCommand;

#include "editor_entity_settings.h"
typedef struct EditorSoftBodyPhysicsValues {
    bool gravity_all, gravity_any;
    float friction, restitution;
    bool friction_mixed, restitution_mixed;
} EditorSoftBodyPhysicsValues;
EditorSoftBodyPhysicsValues editor_soft_body_physics_get(const EditorSoftBody *body);

typedef enum EditorMotionMode {
    EDITOR_MOTION_DYNAMIC,
    EDITOR_MOTION_STATIC,
    EDITOR_MOTION_KINEMATIC,
    /** Display-only value; not accepted by motion commands. */
    EDITOR_MOTION_MIXED
} EditorMotionMode;

/** Common node mode, or the authored default when the body is empty. */
EditorMotionMode editor_soft_body_motion_get(const EditorSoftBody *body);

typedef enum EditorPropertyKind {
    EDITOR_PROPERTY_CENTER_OF_MASS,
    EDITOR_PROPERTY_MASS,
    EDITOR_PROPERTY_FRICTION,
    EDITOR_PROPERTY_RESTITUTION,
    EDITOR_PROPERTY_GRAVITY,
    EDITOR_PROPERTY_STATIC,
    EDITOR_PROPERTY_MOTION,
    EDITOR_PROPERTY_ROTATION_LOCKED,
    EDITOR_PROPERTY_COLLISION,
    EDITOR_PROPERTY_PARTICLE,
    EDITOR_PROPERTY_PARTICLE_RADIUS,
    EDITOR_PROPERTY_PARTICLE_RIGID_VERTICES,
    EDITOR_PROPERTY_PARTICLE_ORIGIN_X,
    EDITOR_PROPERTY_PARTICLE_ORIGIN_Y,
    EDITOR_PROPERTY_PARTICLE_AUTO_FIT,
    EDITOR_PROPERTY_ACTIVE_HITBOX,
    EDITOR_PROPERTY_HITBOX_FRAME_BINDING,
    EDITOR_PROPERTY_NODE_RADIUS,
    EDITOR_PROPERTY_POSITION_LOCKED,
    EDITOR_PROPERTY_VISUAL_SIZE,
    EDITOR_PROPERTY_JOINT_KIND,
    EDITOR_PROPERTY_REST_LENGTH,
    EDITOR_PROPERTY_REST_LENGTH_EXPLICIT,
    EDITOR_PROPERTY_EDITOR_REST_LENGTH_LOCKED,
    EDITOR_PROPERTY_REST_ANGLE,
    EDITOR_PROPERTY_REST_ANGLE_EXPLICIT,
    EDITOR_PROPERTY_FOLLOW_POSITION,
    EDITOR_PROPERTY_PAUSE_WITH_ENGINE,
    EDITOR_PROPERTY_STIFFNESS,
    EDITOR_PROPERTY_DAMPING,
    EDITOR_PROPERTY_BEAM_COLLISION_THICKNESS,
    EDITOR_PROPERTY_POSITION_FOLLOWS_BODY,
    EDITOR_PROPERTY_ROTATION_FOLLOWS_BODY,
    EDITOR_PROPERTY_LINE_LENGTH,
    EDITOR_PROPERTY_OUTLINE_COLOR,
    EDITOR_PROPERTY_SURFACE_COLOR,
    EDITOR_PROPERTY_PARTICLE_RING_COLOR,
    EDITOR_PROPERTY_PARTICLE_FILL_COLOR,
    EDITOR_PROPERTY_NODE_COLOR,
    EDITOR_PROPERTY_BEAM_COLOR,
    EDITOR_PROPERTY_COLOR,
    EDITOR_PROPERTY_INITIAL_VELOCITY_X,
    EDITOR_PROPERTY_INITIAL_VELOCITY_Y,
    EDITOR_PROPERTY_INITIAL_ACCELERATION_X,
    EDITOR_PROPERTY_INITIAL_ACCELERATION_Y,
    EDITOR_PROPERTY_INITIAL_ANGULAR_VELOCITY,
    EDITOR_PROPERTY_INITIAL_MOTION_INHERITED,
    EDITOR_PROPERTY_VISUAL_FILL_COLOR,
    EDITOR_PROPERTY_VISUAL_BORDER_COLOR,
    EDITOR_PROPERTY_VISUAL_BORDER_THICKNESS,
    EDITOR_PROPERTY_VISUAL_SCALE_X,
    EDITOR_PROPERTY_VISUAL_SCALE_Y,
    EDITOR_PROPERTY_VISUAL_OFFSET_X,
    EDITOR_PROPERTY_VISUAL_OFFSET_Y,
    EDITOR_PROPERTY_VISUAL_ROTATION,
    EDITOR_PROPERTY_VISUAL_A_FILL_COLOR,
    EDITOR_PROPERTY_VISUAL_A_BORDER_COLOR,
    EDITOR_PROPERTY_VISUAL_A_BORDER_THICKNESS,
    EDITOR_PROPERTY_VISUAL_A_SCALE_X,
    EDITOR_PROPERTY_VISUAL_A_SCALE_Y,
    EDITOR_PROPERTY_VISUAL_A_OFFSET_X,
    EDITOR_PROPERTY_VISUAL_A_OFFSET_Y,
    EDITOR_PROPERTY_VISUAL_A_ROTATION,
    EDITOR_PROPERTY_VISUAL_B_FILL_COLOR,
    EDITOR_PROPERTY_VISUAL_B_BORDER_COLOR,
    EDITOR_PROPERTY_VISUAL_B_BORDER_THICKNESS,
    EDITOR_PROPERTY_VISUAL_B_SCALE_X,
    EDITOR_PROPERTY_VISUAL_B_SCALE_Y,
    EDITOR_PROPERTY_VISUAL_B_OFFSET_X,
    EDITOR_PROPERTY_VISUAL_B_OFFSET_Y,
    EDITOR_PROPERTY_VISUAL_B_ROTATION,
#define ENTITY_SETTING_ENUM(symbol, name, label, type, member) EDITOR_PROPERTY_##symbol,
    EDITOR_ENTITY_SETTING_PROPERTIES(ENTITY_SETTING_ENUM)
#undef ENTITY_SETTING_ENUM
    EDITOR_PROPERTY_FONT_POINT_SIZE,
    EDITOR_PROPERTY_TEXT_WRAP_WIDTH,
    EDITOR_PROPERTY_TEXT_ROTATION,
    EDITOR_PROPERTY_RENDER_SIZE_EXPLICIT,
    EDITOR_PROPERTY_RENDER_WIDTH,
    EDITOR_PROPERTY_RENDER_HEIGHT,
    EDITOR_PROPERTY_AXIS_ANGLE
} EditorPropertyKind;

typedef enum EditorPropertyValueKind {
    EDITOR_PROPERTY_VALUE_CENTER_OF_MASS,
    EDITOR_PROPERTY_VALUE_FLOAT,
    EDITOR_PROPERTY_VALUE_BOOL,
    EDITOR_PROPERTY_VALUE_UINT,
    EDITOR_PROPERTY_VALUE_DOUBLE,
    EDITOR_PROPERTY_VALUE_UINT64,
    EDITOR_PROPERTY_VALUE_REFERENCE
} EditorPropertyValueKind;

typedef struct EditorPropertySetCommand {
    EditorItemKind kind;
    EditorObjectId object;
    uint32_t parent;
    uint32_t item;
    uint32_t index;
    EditorPropertyKind property;
    EditorPropertyValueKind value_kind;
    union {
        struct { bool explicit_mode; Position offset; } center_of_mass;
        float number;
        bool boolean;
        uint32_t integer;
        double real;
        uint64_t wide_integer;
        ObjectEntityReference reference;
    } value;
} EditorPropertySetCommand;
ObjectEntitySettings *editor_command_entity_settings_get(EditorProject *project,
    EditorItemKind kind, EditorObjectId object, uint32_t parent, uint32_t item);
bool editor_command_entity_setting_value_parse(const char *text, EditorPropertySetCommand *set);

typedef enum EditorRelationshipKind {
    EDITOR_RELATIONSHIP_JOINT_ANCHOR,
    EDITOR_RELATIONSHIP_ANCHOR_RIGID_BODY,
    EDITOR_RELATIONSHIP_SOFT_BEAM_NODE,
    EDITOR_RELATIONSHIP_ENTITY_PARENT,
    EDITOR_RELATIONSHIP_ANCHOR_SOFT_NODE
} EditorRelationshipKind;

typedef struct EditorRelationshipSetCommand {
    EditorRelationshipKind kind;
    EditorObjectId object;
    uint32_t parent;
    uint32_t item;
    uint32_t endpoint;
    uint32_t target;
} EditorRelationshipSetCommand;

typedef enum EditorCollisionFilterKind {
    EDITOR_COLLISION_FILTER_CATEGORY,
    EDITOR_COLLISION_FILTER_RESPONDS_TO
} EditorCollisionFilterKind;

typedef struct EditorCollisionFilterSetCommand {
    EditorItemKind kind;
    EditorObjectId object;
    uint32_t parent;
    uint32_t item;
    EditorCollisionFilterKind filter;
    char mask[EDITOR_OBJECT_NAME_MAX];
    bool enabled;
} EditorCollisionFilterSetCommand;

typedef enum EditorVisibilityKind {
    EDITOR_VISIBILITY_OBJECT,
    EDITOR_VISIBILITY_RIGID_BODY,
    EDITOR_VISIBILITY_HITBOX,
    EDITOR_VISIBILITY_JOINT,
    EDITOR_VISIBILITY_ANCHOR,
    EDITOR_VISIBILITY_SOFT_BODY,
    EDITOR_VISIBILITY_SOFT_NODE,
    EDITOR_VISIBILITY_SOFT_BEAM,
    EDITOR_VISIBILITY_CAMERA,
    EDITOR_VISIBILITY_SOFT_AREA,
    EDITOR_VISIBILITY_FORCE,
    EDITOR_VISIBILITY_TORQUE
} EditorVisibilityKind;

typedef enum EditorSourceOperation {
    EDITOR_SOURCE_ADD, EDITOR_SOURCE_SET, EDITOR_SOURCE_REMOVE,
    EDITOR_SOURCE_RENAME, EDITOR_SOURCE_DUPLICATE, EDITOR_SOURCE_GET, EDITOR_SOURCE_LIST,
    EDITOR_SOURCE_MARKER_SET
} EditorSourceOperation;
typedef struct EditorSourceCommand {
    EditorObjectId object;
    uint32_t id;
    bool torque;
    EditorSourceOperation operation;
    char name[EDITOR_OBJECT_NAME_MAX];
    ObjectForceDescription force;
    ObjectTorqueDescription torque_description;
    Position marker_offset;
} EditorSourceCommand;

typedef struct EditorCommand {
    EditorCommandType type;
    /* Pointer transforms project onto axis constraints and fail silently in UI. */
    bool pointer_transform;
    union {
        struct {
            EditorObjectId object;
            const struct EditorCommand *commands;
            size_t count;
        } transform_batch;
        EditorSourceCommand source;
        struct {
            char name[EDITOR_OBJECT_NAME_MAX];
            Position position;
        } object_add;
        struct {
            EditorObjectId object;
            char name[EDITOR_OBJECT_NAME_MAX];
        } object_rename;
        struct {
            EditorObjectId object;
        } object_remove;
        struct {
            EditorObjectId object;
            Position position;
        } object_position;
        struct {
            EditorObjectId object;
            EditorRigidBodyId body;
            Position position;
            float rotation;
        } rigid_body_transform;
        struct {
            EditorObjectId object;
            EditorRigidBodyId body;
            EditorHitboxId hitbox;
            EditorVertexId vertex;
            Position position;
        } vertex_position;
        struct {
            EditorObjectId object;
            EditorAnchorId anchor;
            Position position;
            float rotation;
        } anchor_transform;
        struct {
            EditorObjectId object;
            EditorSoftBodyId body;
            Position position;
            float rotation;
        } soft_body_transform;
        struct {
            EditorObjectId object;
            EditorSoftBodyId body;
            EditorSoftNodeId node;
            Position position;
        } soft_node_position;
        struct { EditorObjectId object; EditorCameraId camera;
            Position position; Orientation rotation; } camera_transform;
        struct { EditorObjectId object; EditorCameraId camera;
            Scale dimensions; } camera_dimensions_set;
        struct { EditorObjectId object; EditorCameraId camera;
            float zoom; } camera_zoom_set;
        struct { EditorObjectId object; EditorCameraId camera;
            EditorCameraAttachmentKind kind; uint32_t target;
            EditorSoftBodyId soft_body; bool inherit_orientation; }
            camera_attachment_set;
        struct {
            EditorItemKind kind;
            EditorObjectId object;
            uint32_t parent;
            uint32_t item;
            EditorAutoShapeConfig config;
            uint32_t points[EDITOR_SOFT_NODE_MAX];
            size_t point_count;
        } auto_shape;
        struct {
            EditorObjectId object;
            uint32_t body;
            Position position;
        } origin;
        struct {
            Vec2D offset;
            float zoom;
        } viewport_camera;
        struct {
            bool local;
        } viewport_coordinates;
        struct {
            Time engine_time_per_tick;
            bool physics_timestep_override;
            Time physics_dt_per_tick;
            uint32_t physics_substeps;
            Acceleration physics_gravity;
            uint32_t physics_solver_iterations;
        } project_physics_settings_set;
        struct { bool enabled; WindowPresentationDescription description; }
            application_window;
        EditorAudioDefinition audio;
        struct { EditorObjectId object; EditorJointId joint; } joint_place;
        struct { EditorObjectId object; bool included; } simulation_object;
        struct { EditorLayoutViewportId viewport; } simulation_viewport;
        struct {
            EditorItemKind kind;
            EditorObjectId object;
            uint32_t parent, item;
            bool visible;
        } app_visibility;
        struct {
            EditorItemKind kind;
            EditorObjectId object;
            uint32_t parent, item;
            EditorGraphicsLayerBinding binding;
            bool inherited;
        } layer_set;
        struct {
            EditorVisibilityKind kind;
            EditorObjectId object;
            uint32_t parent;
            uint32_t item;
            bool visible;
        } visibility;
        struct {
            EditorObjectId object;
            EditorSoftBodyId body;
            EditorSoftAreaId area;
            EditorSoftHoleId hole;
            EditorSoftAreaLoop loop;
        } soft_area_loop;
        struct { EditorObjectId object; EditorSoftBodyId body;
            EditorSoftAreaId area; uint32_t index; } soft_area_order;
        struct { EditorObjectId object; EditorSoftBodyId body;
            EditorSoftAreaId area; EditorGraphicsLayerBinding binding;
            bool inherited; } soft_area_layer;
        EditorNavigationState navigation;
        EditorItemAddCommand item_add;
        EditorItemRemoveCommand item_remove;
        EditorItemRenameCommand item_rename;
        EditorPropertySetCommand property_set;
        EditorRelationshipSetCommand relationship_set;
        struct {
            char name[EDITOR_OBJECT_NAME_MAX];
        } collision_mask_add;
        EditorCollisionFilterSetCommand collision_filter_set;
        EditorInputControllerCommand input_controller;
        EditorInputActionCommand input_action;
        EditorInputBindingCommand input_binding;
        struct {
            EditorObjectId object;
            char name[EDITOR_OBJECT_NAME_MAX];
            char path[EDITOR_ASSET_PATH_MAX];
            Scale size;
        } sprite_add;
        struct { EditorObjectId object; EditorSpriteId sprite; } sprite_remove;
        struct { EditorObjectId object; EditorSpriteId sprite;
            char name[EDITOR_OBJECT_NAME_MAX]; } sprite_rename;
        struct { EditorObjectId object; EditorSpriteId sprite;
            char path[EDITOR_ASSET_PATH_MAX]; } sprite_path_set;
        struct { EditorObjectId object; EditorSpriteId sprite; Position position; }
            sprite_position_set;
        struct { EditorObjectId object; EditorSpriteId sprite; Orientation rotation; }
            sprite_rotation_set;
        struct { EditorObjectId object; EditorSpriteId sprite; Scale size; }
            sprite_size_set;
        struct { EditorObjectId object; EditorSpriteId sprite;
            EditorRigidBodyId body; } sprite_body_set;
        struct { EditorObjectId object; EditorSpriteId sprite; bool visible; }
            sprite_visibility_set;
        struct { EditorObjectId object; EditorSpriteId sprite; bool enabled; }
            sprite_boolean_set;
        struct { EditorObjectId object; char name[EDITOR_OBJECT_NAME_MAX]; }
            animated_sprite_add;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite; }
            animated_sprite_remove;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite;
            char name[EDITOR_OBJECT_NAME_MAX]; } animated_sprite_rename;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite;
            EditorRigidBodyId body; } animated_sprite_body_set;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite;
            Position position; } animated_sprite_position_set;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite;
            Orientation rotation; } animated_sprite_rotation_set;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite; Scale scale; }
            animated_sprite_scale_set;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite;
            Tick ticks; Time time; } animated_sprite_timing_set;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite; uint32_t frame; }
            animated_sprite_starting_frame_set;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite;
            Direction direction; } animated_sprite_direction_set;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite; bool enabled; }
            animated_sprite_boolean_set;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite;
            char name[EDITOR_OBJECT_NAME_MAX]; char path[EDITOR_ASSET_PATH_MAX];
            Scale scale; } animation_frame_add;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite; size_t index; }
            animation_frame_remove;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite; size_t index;
            char name[EDITOR_OBJECT_NAME_MAX]; } animation_frame_rename;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite; size_t index;
            char path[EDITOR_ASSET_PATH_MAX]; } animation_frame_path_set;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite; size_t index;
            Scale scale; } animation_frame_scale_set;
        struct { EditorObjectId object; EditorAnimatedSpriteId sprite;
            size_t index; Position offset; Orientation rotation;
        } animation_frame_transform_set;
    } data;
} EditorCommand;

EditorObjectId editor_command_placement_target_get(const EditorCommand *command,
    ObjectEntityReference *driver, EditorJointId *joint, bool *transform);

typedef struct EditorCommandResult {
    EditorJointPlacementResult placement;
    ErrorResultKind kind;
    union {
        EditorObjectId object;
        EditorError error;
    } result;
    struct {
        bool valid;
        EditorItemKind kind;
        EditorObjectId object;
        uint32_t parent;
        uint32_t container;
        uint32_t item;
        char name[EDITOR_OBJECT_NAME_MAX];
    } created;
} EditorCommandResult;

typedef void (*EditorCommandExecuted)(const EditorCommand *command,
    const EditorCommandResult *result, void *context);
typedef void (*EditorCommandExecuting)(const EditorProject *project,
    const EditorCommand *command, void *context);
typedef void (*EditorCommandFinished)(const EditorCommand *command,
    const EditorCommandResult *result, void *context);

EditorCommandResult editor_command_execute(EditorProject *project,
    const EditorCommand *command);
void editor_command_executed_callback_set(EditorCommandExecuted callback,
    void *context);
void editor_command_executing_callback_set(EditorCommandExecuting callback,
    void *context);
void editor_command_finished_callback_set(EditorCommandFinished callback,
    void *context);
EditorResult editor_command_cli_parse(int count, char **arguments,
    const char **document_path, EditorCommand *command);
EditorResult editor_command_cli_write(const EditorCommand *command,
    const char *document_path, char *output, size_t output_capacity);
EditorResult editor_command_cli_named_parse(const EditorProject *project,
    int count, char **arguments, const char **document_path,
    EditorCommand *command);
EditorResult editor_command_cli_named_write(const EditorProject *project,
    const EditorCommand *command, const char *document_path,
    char *output, size_t output_capacity);
EditorResult editor_command_cli_standard_parse(const EditorProject *project,
    int count, char **arguments, const char **document_path,
    EditorCommand *command);
EditorResult editor_command_cli_standard_write(const EditorProject *project,
    const EditorCommand *command, const EditorCommandResult *result,
    const char *document_path, char *output, size_t output_capacity);

EditorCommandResult editor_source_command_execute(EditorProject *project, const EditorSourceCommand *command);
EditorResult editor_source_cli_parse(const EditorProject *project, int count, char **arguments, const char **path, EditorCommand *command, bool *handled);
EditorResult editor_source_cli_write(const EditorProject *project, const EditorCommand *command, const char *path, char *output, size_t capacity);
char *editor_source_query_json(const EditorProject *project, const EditorSourceCommand *command);

/* Returns the independently stored app-hidden flag, or NULL for nonvisual items. */
EditorGraphicsLayerBinding *editor_command_layer_get(EditorProject *project,
    EditorItemKind kind, EditorObjectId object, uint32_t parent, uint32_t item,
    bool **inherited);
ObjectVisuals *editor_command_visuals_get(EditorProject *project, EditorItemKind kind,
    EditorObjectId object, uint32_t item);
bool editor_visual_property_check(EditorPropertyKind property);
const char *editor_visual_property_name_get(EditorPropertyKind property);
bool editor_visual_property_parse(const char *name, EditorPropertyKind *property, EditorPropertyValueKind *kind);
bool *editor_command_app_hidden_get(EditorProject *project, EditorItemKind kind,
    EditorObjectId object, uint32_t parent, uint32_t item);

#endif
