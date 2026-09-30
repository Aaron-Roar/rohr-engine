/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

/** Identifies whether a result object contains a value or an error. */
typedef enum {
    /** The result contains a valid value in result.value. */
    ERROR_RESULT_VALUE,
    /** The result contains an EngineError in result.error. */
    ERROR_RESULT_ERROR,
} ErrorResultKind;

/** Engine-wide error codes returned by fallible APIs. */
typedef enum EngineError {
    /** No error occurred. */
    ERROR_NONE = 0,

    /** A memory pool function received a null pointer. */
    ERROR_MEMORY_POOL_NULL_POINTER,
    /** A requested memory pool capacity would overflow. */
    ERROR_MEMORY_POOL_CAPACITY_OVERFLOW,
    /** A memory pool allocation failed. */
    ERROR_MEMORY_POOL_ALLOCATION_FAILED,
    /** A memory pool has no free slots. */
    ERROR_MEMORY_POOL_FULL,
    /** A pointer does not belong to the target memory pool. */
    ERROR_MEMORY_POOL_INVALID_OBJECT,
    /** A requested memory pool slot is not currently used. */
    ERROR_MEMORY_POOL_OBJECT_NOT_USED,
    /** A shrink request would remove a used memory pool slot. */
    ERROR_MEMORY_POOL_SHRINK_WOULD_REMOVE_USED_OBJECT,

    /** The engine is already initialized and running. */
    ERROR_ENGINE_ALREADY_RUNNING,
    /** SDL initialization failed. */
    ERROR_ENGINE_SDL_INIT_FAILED,
    /** Entity tables failed to initialize. */
    ERROR_ENGINE_ENTITY_TABLES_INIT_FAILED,
    /** Physics tables failed to initialize. */
    ERROR_ENGINE_PHYSICS_TABLES_INIT_FAILED,
    /** Graphics tables failed to initialize. */
    ERROR_ENGINE_GRAPHICS_TABLES_INIT_FAILED,
    /** Graphics window or renderer initialization failed. */
    ERROR_ENGINE_GRAPHICS_INIT_FAILED,
    /** A graphics operation requires an initialized renderer. */
    ERROR_ENGINE_GRAPHICS_NOT_INITIALIZED,
    /** SDL could not apply the requested VSync mode. */
    ERROR_ENGINE_GRAPHICS_VSYNC_SET_FAILED,
    /** SDL could not apply the requested window presentation. */
    ERROR_ENGINE_GRAPHICS_WINDOW_PRESENTATION_FAILED,
    /** A frame limit was negative. */
    ERROR_ENGINE_INVALID_FRAME_LIMIT,
    /** An operation would exceed MAX_ENTITIES. */
    ERROR_ENGINE_MAX_ENTITIES_EXCEEDED,
    /** An area geometry exceeds SOFT_BODY_MAX_AREA_HOLES (16). */
    ERROR_ENGINE_MAX_AREA_HOLES_EXCEEDED,
    /** Entity-indexed subsystem tables could not grow. */
    ERROR_ENGINE_TABLE_EXPANSION_FAILED,
    /** An entity id is invalid or stale. */
    ERROR_ENGINE_INVALID_ENTITY,
    /** No live entity exists for the requested id or index. */
    ERROR_ENGINE_ENTITY_NOT_FOUND,
    /** A required component is missing. */
    ERROR_ENGINE_COMPONENT_MISSING,
    /** An index is outside the valid range for the selected component. */
    ERROR_ENGINE_INDEX_OUT_OF_RANGE,
    /** A polygon shape is degenerate, self-intersecting, or otherwise invalid. */
    ERROR_ENGINE_INVALID_SHAPE,
    /** A physics position is non-finite or outside the supported world range. */
    ERROR_ENGINE_POSITION_OUT_OF_RANGE,
    /** A texture asset could not be loaded. */
    ERROR_ENGINE_TEXTURE_LOAD_FAILED,
    /** A texture handle is invalid, stale, or no longer owned. */
    ERROR_ENGINE_TEXTURE_NOT_FOUND,
    /** The engine texture-resource table is full. */
    ERROR_ENGINE_TEXTURE_CAPACITY_EXCEEDED,
    /** A font asset could not be loaded. */
    ERROR_ENGINE_FONT_LOAD_FAILED,
    /** A font handle is invalid, stale, or no longer owned. */
    ERROR_ENGINE_FONT_NOT_FOUND,
    /** The engine font-resource table is full. */
    ERROR_ENGINE_FONT_CAPACITY_EXCEEDED,
    /** A reusable text asset could not be created. */
    ERROR_ENGINE_TEXT_CREATE_FAILED,
    /** A text handle is invalid, stale, or no longer publicly owned. */
    ERROR_ENGINE_TEXT_NOT_FOUND,
    /** The engine text-instance table is full. */
    ERROR_ENGINE_TEXT_CAPACITY_EXCEEDED,
    /** An animation definition or one of its texture frames could not load. */
    ERROR_ENGINE_ANIMATION_LOAD_FAILED,
    /** An animation handle is invalid, stale, or no longer owned. */
    ERROR_ENGINE_ANIMATION_NOT_FOUND,
    /** The engine animation-resource table is full. */
    ERROR_ENGINE_ANIMATION_CAPACITY_EXCEEDED,
    /** An entity name is null or empty. */
    ERROR_ENGINE_INVALID_ENTITY_NAME,
    /** An entity name exceeds ENTITY_NAME_MAX. */
    ERROR_ENGINE_ENTITY_NAME_TOO_LONG,
    /** An entity name is already assigned to another live entity. */
    ERROR_ENGINE_DUPLICATE_ENTITY_NAME,
    /** A game-state file could not be read or written. */
    ERROR_ENGINE_STATE_IO_FAILED,
    /** A game-state document does not match the supported schema. */
    ERROR_ENGINE_STATE_INVALID,
    /** A named entity relationship could not be resolved. */
    ERROR_ENGINE_STATE_REFERENCE_NOT_FOUND,
    /** A group name is null or empty. */
    ERROR_ENGINE_INVALID_GROUP_NAME,
    /** A group name exceeds GROUP_NAME_MAX. */
    ERROR_ENGINE_GROUP_NAME_TOO_LONG,
    /** A group name is already assigned to another generic group. */
    ERROR_ENGINE_DUPLICATE_GROUP_NAME,
    /** No live generic group has the requested name. */
    ERROR_ENGINE_GROUP_NOT_FOUND,
    /** Loading state files would exceed the retained template document limit. */
    ERROR_ENGINE_STATE_TEMPLATE_DOCUMENT_LIMIT_EXCEEDED,
    /** A named asset is defined more than once in retained state. */
    ERROR_ENGINE_STATE_DUPLICATE_ASSET_DEFINITION,
    /** A state component references an unknown named asset. */
    ERROR_ENGINE_STATE_ASSET_REFERENCE_NOT_FOUND,
    /** No loaded UI button has the requested authored name. */
    ERROR_ENGINE_UI_DEFINITION_NOT_FOUND,
    /** A reusable graphics UI definition is still mounted in a viewport. */
    ERROR_ENGINE_GRAPHICS_UI_IN_USE,
    ERROR_ENGINE_INVALID_GRAPHICS_LAYER_NAME,
    ERROR_ENGINE_GRAPHICS_LAYER_NAME_TOO_LONG,
    ERROR_ENGINE_DUPLICATE_GRAPHICS_LAYER_NAME,
    ERROR_ENGINE_GRAPHICS_LAYER_NOT_FOUND,
    /** An input controller or action name is null or empty. */
    ERROR_ENGINE_INVALID_INPUT_NAME,
    /** An input controller or action name exceeds ROHR_INPUT_NAME_MAX. */
    ERROR_ENGINE_INPUT_NAME_TOO_LONG,
    /** A controller, action, or named binding is duplicated in its namespace. */
    ERROR_ENGINE_DUPLICATE_INPUT_NAME,
    /** Input controller, action, or binding capacity was exceeded. */
    ERROR_ENGINE_INPUT_CAPACITY_EXCEEDED,
    /** An input controller or action handle was not found. */
    ERROR_ENGINE_INPUT_NOT_FOUND,
    /** An input action was queried or configured with the wrong value type. */
    ERROR_ENGINE_INPUT_TYPE_MISMATCH,
    /** An input binding is invalid for its action type. */
    ERROR_ENGINE_INPUT_BINDING_INVALID,
    /** An input operation requires a focused SDL window. */
    ERROR_ENGINE_INPUT_WINDOW_NOT_FOUND,
    /** SDL could not apply an input operation. */
    ERROR_ENGINE_INPUT_OPERATION_FAILED,
    /** An input action configuration contains an unsupported value. */
    ERROR_ENGINE_INPUT_CONFIGURATION_INVALID,
    /** The process working directory could not be changed. */
    ERROR_ENGINE_DIRECTORY_WORKING_SET_FAILED,
    /** The audio service is already started. */
    ERROR_ENGINE_AUDIO_ALREADY_STARTED,
    /** An audio operation requires the audio service to be started. */
    ERROR_ENGINE_AUDIO_NOT_STARTED,
    /** SDL could not initialize or open the audio service. */
    ERROR_ENGINE_AUDIO_INIT_FAILED,
    /** A sound configuration contains no usable WAV path or audio data. */
    ERROR_ENGINE_AUDIO_SOUND_CONFIG_INVALID,
    /** A WAV sound could not be loaded or converted for the mixer. */
    ERROR_ENGINE_AUDIO_SOUND_LOAD_FAILED,
    /** No live sound exists for the requested handle. */
    ERROR_ENGINE_AUDIO_SOUND_NOT_FOUND,
    /** No additional sound playback instances can be created. */
    ERROR_ENGINE_AUDIO_SOUND_CAPACITY_EXCEEDED,
    /** A music configuration contains no usable Ogg Vorbis path. */
    ERROR_ENGINE_AUDIO_MUSIC_CONFIG_INVALID,
    /** An Ogg Vorbis music stream could not be opened or decoded. */
    ERROR_ENGINE_AUDIO_MUSIC_LOAD_FAILED,
    /** No live music resource exists for the requested handle. */
    ERROR_ENGINE_AUDIO_MUSIC_NOT_FOUND,
    /** No additional music resources can be created. */
    ERROR_ENGINE_AUDIO_MUSIC_CAPACITY_EXCEEDED,
    /** A pause or resume operation targeted music that is not active. */
    ERROR_ENGINE_AUDIO_MUSIC_NOT_ACTIVE,
    /** SDL could not perform an audio operation. */
    ERROR_ENGINE_AUDIO_OPERATION_FAILED,
} EngineError;

/**
 * A result type needs an explicit result name in C.
 *
 * C macros cannot reliably build a type name from an arbitrary C type because
 * valid types can include spaces, pointers, and other tokens that cannot be
 * pasted into an identifier. Pass the PascalCase ResultType explicitly.
 */
#define ERROR_DECLARE_RESULT_TYPE(ResultType, ValueType) \
    typedef union Result##ResultType { \
        ValueType value; \
        EngineError error; \
    } Result##ResultType; \
    \
    typedef struct ResultType { \
        ErrorResultKind kind; \
        Result##ResultType result; \
    } ResultType

/** Build a successful result value for a generated result type. */
#define ERROR_RESULT_MAKE_VALUE(ResultType, Value) \
    ((ResultType){ \
        .kind = ERROR_RESULT_VALUE, \
        .result.value = (Value) \
    })

/** Build an error result value for a generated result type. */
#define ERROR_RESULT_MAKE_ERROR(ResultType, ErrorValue) \
    ((ResultType){ \
        .kind = ERROR_RESULT_ERROR, \
        .result.error = (ErrorValue) \
    })

/** Return true when a result value contains an error. */
#define error_check(ResultValue) \
    ((ResultValue).kind == ERROR_RESULT_ERROR)

/** Result type for fallible APIs that return only success or failure. */
ERROR_DECLARE_RESULT_TYPE(EngineResult, bool);

/**
 * Create a successful EngineResult.
 *
 * @param value Success value to store.
 * @return EngineResult containing value.
 */
EngineResult error_result_value(bool value);

/**
 * Create a failed EngineResult.
 *
 * @param error Engine error code to store.
 * @return EngineResult containing error.
 */
EngineResult error_result_error(EngineError error);

/** Create a failed result and copy its deepest diagnostic detail. */
EngineResult error_result_error_detail(EngineError error, const char *detail);

/** Copy diagnostic detail for a non-EngineResult error result. */
void error_detail_set(EngineError error, const char *detail);

/**
 * Get the default human-readable message for an EngineError.
 *
 * @param error Error code to describe.
 * @return Static string owned by the error module.
 */
const char *error_code_message_get(EngineError error);

/**
 * Get the human-readable message for an EngineError.
 *
 * Includes copied low-level diagnostic detail when the failing boundary
 * supplied it. The returned string remains valid until the next error.
 *
 * @param error Error code to describe.
 * @return Static string owned by the error module.
 */
/** Get a result's error message, or a diagnostic for a successful result. */
const char *error_result_message_get(ErrorResultKind kind, EngineError error);

#endif
