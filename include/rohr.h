/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef ROHR_H
#define ROHR_H

#include "scene.h"
#include "audio.h"
#include "console.h"
#include "engine.h"
#include "entity_components.h"
#include "entity_info.h"
#include "error.h"
#include "graphics.h"
#include "game_state.h"
#include "input.h"
#include "math2d.h"
#include "physics.h"
#include "object.h"
#include "project_instance.h"
#include "report.h"
#include "systems.h"
#include "tools.h"
#include "ui.h"

/* Generated views and runtime description values share the same typed C interface. */
#define rohr_controller_create controller_create
#define rohr_controller_description_default_get controller_description_default_get
#define rohr_project_create project_create
#define rohr_project_destroy project_destroy
#define rohr_entity_appearance_get entity_appearance_get
#define rohr_entity_appearance_set entity_appearance_set
#define rohr_graphics_hit_boxes_debug_set graphics_hit_boxes_debug_set
#define rohr_graphics_hit_boxes_debug_check graphics_hit_boxes_debug_check
#define rohr_graphics_particles_debug_set graphics_particles_debug_set
#define rohr_graphics_particles_debug_check graphics_particles_debug_check
#define rohr_graphics_local_origins_debug_set graphics_local_origins_debug_set
#define rohr_graphics_local_origins_debug_check graphics_local_origins_debug_check
#define rohr_entity_visible_check entity_visible_check
#define rohr_viewport_item_by_name_get graphics_viewport_item_by_name_get
#define rohr_graphics_viewport_show graphics_viewport_show
#define rohr_graphics_viewports_show graphics_viewports_show
#define rohr_graphics_pipeline_cameras_update_for_viewports graphics_pipeline_cameras_update_for_viewports
#define rohr_graphics_pipeline_viewports_draw_selected graphics_pipeline_viewports_draw_selected
#define rohr_entity_create entity_create
#define rohr_entity_create_with_options entity_create_with_options
#define rohr_entity_create_options_default_get entity_create_options_default_get
#define rohr_object_create object_create
#define rohr_object_create_with_options object_create_with_options
#define rohr_object_create_options_default_get object_create_options_default_get
#define rohr_object_get object_get
#define rohr_object_load object_load
#define rohr_object_copy object_copy
#define rohr_object_copy_with_options object_copy_with_options
#define rohr_object_destroy object_destroy
#define rohr_object_description_by_name_get object_description_by_name_get
#define rohr_object_force_description_set object_force_description_set
#define rohr_object_torque_description_set object_torque_description_set
#define rohr_object_mass_set object_mass_set
#define rohr_object_position_set object_position_set
#define rohr_object_orientation_set object_orientation_set
#define rohr_object_velocity_set object_velocity_set
#define rohr_object_acceleration_set object_acceleration_set
#define rohr_object_angular_velocity_set object_angular_velocity_set
#define rohr_object_material_description_set object_material_description_set
#define rohr_object_material_description_get object_material_description_get
#define rohr_object_collision_description_set object_collision_description_set
#define rohr_object_collision_description_get object_collision_description_get
#define rohr_object_radius_set object_radius_set
#define rohr_object_visibility_set object_visibility_set
#define rohr_object_color_description_set object_color_description_set
#define rohr_object_color_description_get object_color_description_get
#define rohr_object_scale_set object_scale_set
#define rohr_object_spawn_description_default_get object_spawn_description_default_get
#define rohr_entity_object_description_add entity_object_description_add
#define rohr_entity_object_description_by_name_add entity_object_description_by_name_add
#define rohr_entity_object_add entity_object_add
#define rohr_entity_object_add_with_options entity_object_add_with_options
#define rohr_entity_object_by_name_add entity_object_by_name_add
#define rohr_entity_object_by_name_add_with_options entity_object_by_name_add_with_options
#define rohr_entity_object_camera_get entity_object_camera_get
#define rohr_entity_object_anchor_get entity_object_anchor_get
#define rohr_entity_object_draw entity_object_draw

/**
 * @brief Allocate a frozen current-state snapshot for the selected sections.
 * @param entity Live entity to inspect between updates.
 * @param flags Nonzero combination of supported ROHR_ENTITY_INFO selectors.
 * @return Owned EntityInfo or an error. Release every owning reference.
 */
EntityInfoResult rohr_entity_info_get(Entity entity, EntityInfoFlags flags);
/**
 * @brief Acquire another owner of the same immutable snapshot without reinspection.
 * @param info Live snapshot; plain C assignment does not retain ownership.
 * @return Another owning EntityInfo, or an error without changing ownership.
 */
EntityInfoResult rohr_entity_info_retain(const EntityInfo *info);
/**
 * @brief Release one snapshot owner and clear its value; zero is a no-op.
 * @param info Owning value. NULL and stale nonzero handles are errors.
 * @return Success or an error. Final release frees all snapshot arrays.
 */
EngineResult rohr_entity_info_release(EntityInfo *info);
/**
 * @brief Check whether every selected section was captured successfully.
 * @param info Live immutable snapshot.
 * @param flags Nonzero supported selectors; availability is not operation success.
 * @return False for invalid input or any unavailable section.
 */
bool rohr_entity_info_check(const EntityInfo *info, EntityInfoFlags flags);

/**
 * @brief Count captured issues matching the selected sections.
 * @param info Live immutable snapshot.
 * @param flags Supported section selectors; uncaptured sections match no issues.
 * @return Matching count, or an error for invalid input.
 */
EntityInfoIssueCountResult rohr_entity_info_issues_count_get(
    const EntityInfo *info, EntityInfoFlags flags);
/**
 * @brief Copy a prefix of matching issues, including owned explanatory text.
 * @param info Live immutable snapshot.
 * @param flags Nonzero supported section selectors.
 * @param destination Caller array; NULL is permitted only with zero capacity.
 * @param capacity Number of destination entries.
 * @return Written and total counts; insufficient capacity is successful partial output.
 */
EntityInfoIssueReadResult rohr_entity_info_issues_get(
    const EntityInfo *info, EntityInfoFlags flags,
    EntityInfoIssue *destination, size_t capacity);

/**
 * @file rohr.h
 * @brief Public Rohr Engine API facade.
 *
 * Include this header from application code to use the engine through the
 * stable `rohr_`-prefixed API. Entity ids are stable handles, not component
 * table indexes. Use the entity API to validate ids and resolve indexes.
 * Angles and stored angular fields are clockwise degrees, with rates in
 * degrees/second and degrees/second squared. Only explicitly named radians
 * scalar APIs use radians. Zero preserves authored geometry and artwork;
 * local +Y is the zero world heading. Values are not wrapped to one turn.
 * Torque, angular impulse, and inertia retain their physical units.
 */

/**
 * @brief Starts the core engine service.
 * @return EngineResult containing true on success, or an engine error.
 */
EngineResult rohr_engine_start(void);

/** Returns an empty caller-owned report builder, independent of engine startup. */
Report rohr_report_create(void);
/** Return an owned snapshot or compose selectors joined with |. Zero and
 * unsupported bits return an error; ROHR_REPORT selects all supported reports. */
ReportResult rohr_report_get(ReportFlags flags);
/** Checks populated report fields, custom IDs, and reserved-fatal consistency. */
bool rohr_report_valid_check(const Report *report);
/** True for valid successful completion, allowing warnings and evidence loss.
 * NOT_RUN, malformed reports, and failures return false. No output or mutation. */
bool rohr_report_check(Report report);
/** True for valid successful completion with no warnings, truncated diagnostic
 * details, dropped records, or saturated counters. Informational diagnostics
 * and intentionally skipped child groups do not fail a completed parent. */
bool rohr_report_status_check(Report report);
/** Sets report identity, completion status, and measured time; failure is sticky. */
EngineResult rohr_report_header_set(Report *report, ReportHeader header);
/** Adds/aggregates a child summary without changing the parent's elapsed time. */
EngineResult rohr_report_entry_add(Report *report, ReportEntry entry);
/** Adds a statistic using its validated sum/max/latest aggregation rule. */
EngineResult rohr_report_statistic_add(Report *report, ReportStatistic statistic);
/** Copies optional detail text; NULL clears, oversized text sets detail_truncated. */
EngineResult rohr_report_diagnostic_detail_set(ReportDiagnostic *diagnostic, const char *detail);
/** Adds an issue, aggregating repeats and preserving the first fatal when full. */
EngineResult rohr_report_diagnostic_add(Report *report, ReportDiagnostic diagnostic);
/**
 * Combines a child report and its descendants without allocation or simulation.
 * Preserves source values and destination identity/time. First fatal and latest
 * metrics follow merge order. Malformed inputs, self-merge, or incompatible
 * metric rules fail without mutation. Capacity exhaustion succeeds with visible
 * loss counts; the reserved first fatal is not dropped. Merge each child once.
 */
EngineResult rohr_report_merge(Report *destination, const Report *source);

/** Write a validated report through the console; no engine startup required. */
EngineResult rohr_report_console_write(const Report *report);
/** Replace a file with versioned JSON; pretty changes whitespace only.
 * Uses temporary allocation. Invalid reports/paths fail before opening the file.
 * An I/O failure can leave a partial file. The source report is never modified.
 */
EngineResult rohr_report_json_write(const Report *report, const char *path, bool pretty);
/** Replace a file with readable report text, without console prefixes.
 * Does not create directories. An I/O failure can leave a partial file.
 */
EngineResult rohr_report_text_write(const Report *report, const char *path);

/**
 * @brief Stops the core engine service and releases its state.
 */
void rohr_engine_stop(void);

/**
 * @brief Updates accumulated engine time from the platform clock.
 */
void rohr_engine_time_update(void);

/** Advance clock/ticks, then lifecycle cleanup; returns an owned ROHR_ENGINE_REPORT_ID.
 * Requires engine startup. Paused/zero-tick calls still run cleanup.
 * Does not update physics, graphics, audio or UI. */
Report rohr_engine_update(void);
/** Advance clock/ticks only and replace the latest tick delta (possibly zero). */
Tick rohr_engine_tick_update(void);
/** Delete expired entities at current time/ticks; leave clock and delta unchanged.
 * Returns an owned ROHR_ENGINE_LIFECYCLE_REPORT_ID. Requires engine startup. */
Report rohr_engine_lifecycle_update(void);
/** Latest tick-update delta; reads do not consume it. Cleared on start/stop/reset. */
Tick rohr_engine_tick_delta_get(void);

/**
 * @brief Returns the current engine time in seconds.
 * @return Current engine time.
 */
Time rohr_engine_time_get(void);

/**
 * @brief Returns the current engine tick counter.
 * @return Current tick.
 */
Tick rohr_engine_tick_get(void);

/**
 * @brief Pauses engine time-dependent updates.
 */
void rohr_engine_pause(void);

/**
 * @brief Resumes engine time-dependent updates.
 */
void rohr_engine_resume(void);

/** Sets the real-time duration required for one engine tick. */
EngineResult rohr_engine_time_per_tick_set(Time time_per_tick);
/** Returns the real-time duration required for one engine tick. */
Time rohr_engine_time_per_tick_get(void);

/**
 * Start optional callback-driven audio. Runtime controls enqueue requests;
 * call rohr_audio_update to submit them. Concurrent submissions are serialized
 * in enqueue order; getters observe applied state, not pending requests.
 * Creation, destruction, startup and shutdown remain synchronous. Callers
 * synchronize their own handle variables; see docs/audio.md for lifetime rules.
 */
EngineResult rohr_audio_start(void);
/** Stop synchronously and return final evidence without consuming it.
 * The next update returns the same report once. Restart discards that copy. */
Report rohr_audio_stop(void);
/** Submit pending controls and collect completed work; never waits for playback. */
Report rohr_audio_update(void);
/** Return whether the audio service is currently started. */
bool rohr_audio_started_check(void);
/** Set the shared mixer volume, clamped to 0..1. */
EngineResult rohr_audio_volume_set(float volume);
/** Return the shared mixer volume. */
float rohr_audio_volume_get(void);

/** Return muted defaults: volume zero, centered pan, rate one, no looping. */
SoundDescription rohr_audio_sound_description_default_get(void);
/** Load a WAV and create one independently controlled playback instance. */
SoundResult rohr_audio_sound_create(SoundDescription config);
SoundResult rohr_audio_sound_by_name_get(const char *name);
SoundNameResult rohr_audio_sound_name_get(Sound sound);
/** Prepare a complete replacement and enqueue one atomic update. Same-source
 * edits preserve progress; a changed source is installed stopped. Paths are
 * required and copied; failures leave existing state and queued work intact. */
EngineResult rohr_audio_sound_description_set(Sound sound, SoundDescription description);
/** Applied configuration, with a borrowed resolved path. The path remains valid
 * until a subsequent update, creation, destruction, full setter, or shutdown.
 * Synchronize those calls with users of borrowed paths on other threads. */
SoundDescriptionResult rohr_audio_sound_description_get(Sound sound);
/** Stop and release one sound owner, clearing it on success. */
EngineResult rohr_audio_sound_destroy(Sound *sound);
/** Return whether a sound player handle is currently valid. */
bool rohr_audio_sound_valid_check(Sound sound);
/** Restart one sound from its beginning. */
EngineResult rohr_audio_sound_play(Sound sound);
/** Stop one sound and reset it to its beginning. */
EngineResult rohr_audio_sound_stop(Sound sound);
/** Set one sound's volume, clamped to 0..1. */
EngineResult rohr_audio_sound_volume_set(Sound sound, float volume);
/** Return one sound's stored volume, or an error for an invalid handle. */
AudioValueResult rohr_audio_sound_volume_get(Sound sound);
/** Set one sound's stereo pan, clamped to -1..1. */
EngineResult rohr_audio_sound_pan_set(Sound sound, float pan);
/** Return one sound's stored stereo pan. */
AudioValueResult rohr_audio_sound_pan_get(Sound sound);
/** Enable or disable restarting the sound when it reaches its end. */
EngineResult rohr_audio_sound_loop_set(Sound sound, bool loop);
/** Return whether one sound is configured to loop. */
bool rohr_audio_sound_loop_check(Sound sound);
/**
 * Set one sound's speed and pitch ratio.
 * Zero freezes playback and negative values are normalized to zero.
 */
EngineResult rohr_audio_sound_playback_rate_set(
    Sound sound,
    float playback_rate
);
/** Return one sound's normalized speed and pitch ratio. */
AudioValueResult rohr_audio_sound_playback_rate_get(Sound sound);
/** Return true only while the sound is actively advancing. */
bool rohr_audio_sound_playing_check(Sound sound);

/** Return muted defaults: volume zero, rate one, no looping. */
MusicDescription rohr_audio_music_description_default_get(void);
/** Open an Ogg Vorbis file for incremental decoding during playback. */
MusicResult rohr_audio_music_create(MusicDescription config);
MusicResult rohr_audio_music_by_name_get(const char *name);
MusicNameResult rohr_audio_music_name_get(Music music);
/** Prepare a complete replacement and enqueue one atomic update. Same-source
 * edits preserve progress; a changed source is installed stopped. Paths are
 * required and copied; failures leave existing state and queued work intact. */
EngineResult rohr_audio_music_description_set(Music music, MusicDescription description);
/** Applied configuration, with a borrowed resolved path. The path remains valid
 * until a subsequent update, creation, destruction, full setter, or shutdown.
 * Synchronize those calls with users of borrowed paths on other threads. */
MusicDescriptionResult rohr_audio_music_description_get(Music music);
/** Stop and release one music owner, clearing it on success. */
EngineResult rohr_audio_music_destroy(Music *music);
/** Return whether a streamed music handle is currently valid. */
bool rohr_audio_music_valid_check(Music music);
/** Start one track from the beginning, replacing any active track. */
EngineResult rohr_audio_music_play(Music music);
/** Pause the active track without changing its playback position. */
EngineResult rohr_audio_music_pause(Music music);
/** Resume an explicitly paused active track. */
EngineResult rohr_audio_music_resume(Music music);
/** Stop one track and reset it to the beginning. */
EngineResult rohr_audio_music_stop(Music music);
/** Set one music track's volume, clamped to 0..1. */
EngineResult rohr_audio_music_volume_set(Music music, float volume);
/** Return one music track's stored volume. */
AudioValueResult rohr_audio_music_volume_get(Music music);
/** Enable or disable restarting the track when it reaches its end. */
EngineResult rohr_audio_music_loop_set(Music music, bool loop);
/** Return whether one music track is configured to loop. */
bool rohr_audio_music_loop_check(Music music);
/** Set speed and pitch; zero freezes and negative values normalize to zero. */
EngineResult rohr_audio_music_playback_rate_set(
    Music music,
    float playback_rate
);
/** Return one music track's normalized speed and pitch ratio. */
AudioValueResult rohr_audio_music_playback_rate_get(Music music);
/** Return true only while the active track is advancing. */
bool rohr_audio_music_playing_check(Music music);
/** Return whether the active track was explicitly paused. */
bool rohr_audio_music_paused_check(Music music);

/**
 * Returns the platform application base directory.
 * The returned path is SDL-owned and must not be freed.
 */
const char *rohr_directory_base_get(void);
/**
 * Sets the process-wide working directory to a non-empty path.
 * Returns an error without changing the directory when the path is invalid or
 * the platform rejects it.
 */
EngineResult rohr_directory_working_set(const char *directory);

/**
 * @brief Polls one SDL event.
 * @return SDL event value returned by the engine event poller.
 */
SDL_Event rohr_engine_event_poll(void);

/**
 * @brief Checks whether the engine is paused.
 * @return true when paused, false when running.
 */
bool rohr_engine_paused_get(void);

/**
 * @brief Resets the engine clock baseline.
 */
void rohr_engine_clock_reset(void);

/** Clear transient input values before polling events for a new frame. */
void rohr_input_frame_begin(void);
/** Return whether a physical key is currently held. */
bool rohr_input_key_down_check(SDL_Scancode key);
/** Return whether a physical key entered the held state this frame. */
bool rohr_input_key_pressed_check(SDL_Scancode key);
/** Return whether a physical key left the held state this frame. */
bool rohr_input_key_released_check(SDL_Scancode key);
/** Return the modifier state captured from the latest keyboard event. */
SDL_Keymod rohr_input_modifiers_get(void);
/** Return whether a mouse button is currently held. */
bool rohr_input_mouse_button_down_check(InputMouseButton button);
/** Return whether a mouse button entered the held state this frame. */
bool rohr_input_mouse_button_pressed_check(InputMouseButton button);
/** Return whether a mouse button left the held state this frame. */
bool rohr_input_mouse_button_released_check(InputMouseButton button);
/** Return the latest absolute pointer position in window coordinates. */
Vec2D rohr_input_mouse_position_get(void);
/** Return accumulated pointer movement since rohr_input_frame_begin. */
Vec2D rohr_input_mouse_delta_get(void);
/** Return accumulated normalized wheel movement for the current frame. */
Vec2D rohr_input_mouse_wheel_get(void);
/** Enable or disable relative pointer mode on the focused input window. */
EngineResult rohr_input_mouse_relative_mode_set(bool enabled);
/** Return whether the focused input window uses relative pointer mode. */
bool rohr_input_mouse_relative_mode_check(void);
/** Begin SDL UTF-8 and IME text input on the focused keyboard window. */
EngineResult rohr_input_text_start(void);
/** Stop SDL text input and clear its active composition and candidates. */
EngineResult rohr_input_text_stop(void);
/** Set the on-screen text field and cursor location used to place an IME. */
EngineResult rohr_input_text_area_set(SDL_Rect area, int cursor);
/** Return a copied snapshot of committed text, composition, and IME candidates. */
InputTextState rohr_input_text_state_get(void);
/** Return whether a tagged physical binding is valid for an action type. */
bool rohr_input_binding_valid_check(InputActionType type,
    const InputBinding *binding);
/** Create a key binding from an SDL keycode, including required Shift state. */
InputBinding rohr_input_binding_key_create(SDL_Keycode key);
/** Create a physical-layout key binding directly from an SDL scancode. */
InputBinding rohr_input_binding_scancode_create(SDL_Scancode key);
/** Create an engine-owned logical controller from complete configuration. */
InputControllerIdResult rohr_input_controller_create(InputControllerDescription description);
/** Enabled, with an empty name to fill before creation. */
InputControllerDescription rohr_input_controller_description_default_get(void);
/** Replace name/enabled atomically without rebuilding child actions.
 * Renaming and unchanged configuration preserve pending input transitions. */
EngineResult rohr_input_controller_description_set(InputControllerId controller,
    InputControllerDescription description);
/** Return an independent copy of current configuration. */
InputControllerDescriptionResult rohr_input_controller_description_get(InputControllerId controller);
/** Destroy a logical controller and all actions owned by it. */
EngineResult rohr_input_controller_destroy(InputControllerId controller);
/** Find a logical controller by its unique name. */
InputControllerIdResult rohr_input_controller_by_name_get(const char *name);
InputControllerNameResult rohr_input_controller_name_get(InputControllerId controller);
/** Enable or disable a controller without changing its action state. */
EngineResult rohr_input_controller_enabled_set(InputControllerId controller,
    bool enabled);
/** Return whether a valid logical controller is enabled. */
bool rohr_input_controller_enabled_check(InputControllerId controller);
/** Create an engine-owned typed action with a controller-local unique name. */
InputActionIdResult rohr_input_action_create(InputActionDescription description);
/** Momentary Button, false initial state, no bindings, invalid owner, empty name. */
InputActionDescription rohr_input_action_description_default_get(void);
/** Atomic full replacement, including controller reassignment with a stable handle.
 * Compatible overrides survive; incompatible overrides reject the entire edit.
 * Name-only/unchanged edits preserve state and transitions. Rebinding preserves
 * persistent state and resamples physical input without inventing transitions.
 * Changed type/mode/initial state reinitializes only this action. */
EngineResult rohr_input_action_description_set(InputActionId action,
    InputActionDescription description);
/** Copy authored configuration only, excluding runtime overrides and current state. */
InputActionDescriptionResult rohr_input_action_description_get(InputActionId action);
/** Destroy an action while leaving its owning controller intact. */
EngineResult rohr_input_action_destroy(InputActionId action);
/** Find an action by its controller and controller-local name. */
InputActionIdResult rohr_input_action_by_name_get(InputControllerId controller,
    const char *name);
/** Return the logical value type declared for an action. */
InputActionTypeResult rohr_input_action_type_get(InputActionId action);
/** Replace a Button action's momentary or persistent behavior. */
EngineResult rohr_input_action_button_mode_set(InputActionId action,
    InputButtonMode mode);
/** Return a Button action's configured behavior. */
InputButtonModeResult rohr_input_action_button_mode_get(InputActionId action);
/** Set the state restored by a persistent Button reset. */
EngineResult rohr_input_action_button_initial_state_set(InputActionId action,
    bool state);
/** Return a Button action's authored initial state. */
bool rohr_input_action_button_initial_state_check(InputActionId action);
/** Explicitly replace a persistent Button action's logical state. */
EngineResult rohr_input_action_button_state_set(InputActionId action, bool state);
/** Restore a persistent Button action to its authored initial state. */
EngineResult rohr_input_action_button_state_reset(InputActionId action);
/** Replace copied defaults; non-empty names must be unique within the list. */
EngineResult rohr_input_action_bindings_default_set(InputActionId action,
    const InputBinding *bindings, size_t count);
/** Return a copy of an action's default bindings. */
InputBindingListResult rohr_input_action_bindings_default_get(InputActionId action);
/** Replace copied overrides; non-empty names must be unique within the list. */
EngineResult rohr_input_action_bindings_override_set(InputActionId action,
    const InputBinding *bindings, size_t count);
/** Return a copy of an action's runtime override bindings. */
InputBindingListResult rohr_input_action_bindings_override_get(InputActionId action);
/** Return a copy of the active override bindings, or defaults when none exist. */
InputBindingListResult rohr_input_action_bindings_effective_get(InputActionId action);
/** Clear runtime overrides so the action uses its defaults again. */
EngineResult rohr_input_action_bindings_override_clear(InputActionId action);
/** Return whether runtime override bindings are active for an action. */
bool rohr_input_action_bindings_override_check(InputActionId action);
/** Return whether a Button action is currently held. */
bool rohr_input_action_button_down_check(InputActionId action);
/** Return whether a Button action entered the held state this frame. */
bool rohr_input_action_button_pressed_check(InputActionId action);
/** Return whether a Button action left the held state this frame. */
bool rohr_input_action_button_released_check(InputActionId action);
/** Return a combined and clamped Axis 1D action value. */
InputAxis1DResult rohr_input_action_axis_1d_get(InputActionId action);
/** Return a combined and unit-length-clamped Axis 2D action value. */
InputAxis2DResult rohr_input_action_axis_2d_get(InputActionId action);

/**
 * @brief Creates a successful boolean engine result.
 * @param value Boolean value to store in the result.
 * @return EngineResult containing value.
 */
EngineResult rohr_error_result_value(bool value);

/**
 * @brief Creates a failed engine result.
 * @param error Error code to store in the result.
 * @return EngineResult containing error.
 */
EngineResult rohr_error_result_error(EngineError error);

/**
 * @brief Checks whether a result contains an error.
 * @param ResultValue Result value to inspect.
 * @return true when ResultValue contains an error, false otherwise.
 */
#define rohr_error_check(ResultValue) error_check(ResultValue)

/**
 * @brief Returns the static message associated with an engine error code.
 * @param error Error code to describe.
 * @return Static string describing error.
 */
const char *rohr_error_code_message_get(EngineError error);

/**
 * @brief Returns a result's error message with captured low-level detail.
 *
 * ResultValue should be a result variable because this macro evaluates it
 * twice. Every generated Rohr result type supports this shared field layout.
 *
 * @param ResultValue Generated Rohr result value to describe.
 * @return Thread-local string; copy before the next error/message call on this thread.
 */
#define rohr_error_message_get(ResultValue) \
    error_result_message_get((ResultValue).kind, (ResultValue).result.error)

/**
 * @brief Prints buffered console log messages.
 */
void rohr_console_logs_print(void);

/**
 * @brief Starts the engine console service.
 */
void rohr_console_start(void);

/**
 * @brief Stops the engine console service.
 */
void rohr_console_stop(void);

/**
 * @brief Reads one console log string.
 * @param input Destination for the log string.
 * @return true when a log string was read, false otherwise.
 */
bool rohr_console_read(ConsoleLogString *input);

/**
 * @brief Writes a formatted message to the engine console.
 * @param source Source category for the log entry.
 * @param fmt printf-style format string.
 */
void rohr_console_write(LogSourceType source, const char *fmt, ...);

/**
 * @brief Checks whether the console is active.
 * @return true when active, false otherwise.
 */
bool rohr_console_active_get(void);

/**
 * @brief Writes a formatted debug message when debug logging is enabled.
 * @param source Source category for the log entry.
 * @param fmt printf-style format string.
 */
void rohr_console_debug_write(LogSourceType source, const char *fmt, ...);

/**
 * @brief Enables or disables debug console output.
 * @param state true to enable debug logging, false to disable it.
 */
void rohr_console_debug_set(bool state);

/**
 * @brief Checks whether an entity id currently refers to a live entity.
 * @param entity Stable entity id to inspect.
 * @return true when the entity is alive, false otherwise.
 */
bool rohr_entity_alive_check(Entity entity);

/**
 * @brief Checks whether an entity table index currently contains a live entity.
 * @param index Component table index to inspect.
 * @return true when the index contains a live entity, false otherwise.
 */
bool rohr_entity_index_alive_check(EntityIndex index);

/**
 * @brief Returns the number of currently alive entities.
 * @return Number of alive entities.
 */
uint32_t rohr_entity_alive_count_get(void);

/**
 * Copy all live IDs into caller-owned storage, without allocation or sorting.
 * Size storage with rohr_entity_alive_count_get(); count returns the required
 * size even on error. Errors write no array elements. NULL list or NULL data
 * with nonzero capacity is invalid; zero capacity is valid but insufficient
 * when entities exist. Calls must be serialized between updates and mutations.
 * Snapshots own no entities; deleted IDs become stale. Discovery order may
 * change after creation/deletion. See entity_list_get for exact error codes.
 */
EngineResult rohr_entity_list_get(EntityList *list);

/**
 * Copy EntityName values using the entity_list_get storage/error contract.
 * Unnamed entities produce empty names; copies do not change after renaming.
 * Entries correspond to rohr_entity_list_get when no entities are created or
 * deleted between calls. The caller owns storage; no entities are retained.
 */
EngineResult rohr_entity_name_list_get(EntityNameList *list);

/**
 * @brief Returns the entity id stored at a dense alive-list position.
 *
 * The position is not a component table index and can change when entities are
 * deleted.
 *
 * @param position Dense alive-list position.
 * @return EntityResult containing the entity id, or an error.
 */
EntityResult rohr_entity_alive_at_get(uint32_t position);

/**
 * @brief Resolves a stable entity id to its current component table index.
 * @param entity Stable entity id to resolve.
 * @return EntityIndexResult containing the index or an invalid-entity error.
 */
EntityIndexResult rohr_entity_index_get(Entity entity);

/**
 * @brief Returns the stable entity id stored at a component table index.
 * @param index Component table index to inspect.
 * @return EntityResult containing the entity id, or an error.
 */
EntityResult rohr_entity_from_index_get(EntityIndex index);

/**
 * @brief Creates a new entity.
 *
 * Entity ids are stable handles and may not match component table indexes.
 *
 * @return EntityResult containing the new entity id, or an error if the entity
 * limit is reached.
 */
EntityResult rohr_entity_add(void);
/** Create copied metadata and ownership; empty creation names are generated. */
EntityResult rohr_entity_description_add(EntityDescription description);
/** Atomically replace name and owner; the name must be nonempty and unique. */
EngineResult rohr_entity_description_set(Entity entity, EntityDescription description);
/** Get a copied name and current independent owner. */
EntityDescriptionResult rohr_entity_description_get(Entity entity);

/** Assigns a unique fixed-size name to an entity. */
EngineResult rohr_entity_name_set(Entity entity, const char *name);

/** Finds a live entity by its state-file name. */
EntityResult rohr_entity_by_name_get(const char *name);

/** Returns a copy of an entity's fixed-size name component. */
EntityNameResult rohr_entity_name_get(Entity entity);

/** Loads and merges one JSON game-state file. */
EngineResult rohr_game_state_file_load(const char *path);

/** Loads multiple JSON files with cross-file name resolution. */
EngineResult rohr_game_state_files_load(const char *const *paths, size_t path_count);

/** @brief Finds a UI button definition loaded from JSON. */
UIButtonDefinitionResult rohr_ui_button_by_name_get(const char *name);

/** @brief Finds a UI font definition loaded from JSON. */
UIFontDefinitionResult rohr_ui_font_by_name_get(const char *name);

/** @brief Finds a standalone UI label definition loaded from JSON. */
UILabelDefinitionResult rohr_ui_label_by_name_get(const char *name);

/** @brief Finds a UI slider definition loaded from JSON. */
UISliderDefinitionResult rohr_ui_slider_by_name_get(const char *name);

/** Saves all live entities using their unique names to a JSON game-state file. */
EngineResult rohr_game_state_file_save(const char *path);

/** Saves retained authored definitions without expanding prototypes. */
EngineResult rohr_game_state_template_file_save(const char *path);

/**
 * @brief Deletes an entity and releases its slot for reuse.
 * @param entity Stable entity id to delete.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_entity_delete(Entity entity);

/**
 * @brief Adds components to an entity.
 * @param entity Stable entity id to modify.
 * @param mask Component mask to add.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_entity_components_add(Entity entity, RohrComponentMask mask);

/**
 * @brief Checks whether an entity has all requested components.
 * @param entity Stable entity id to inspect.
 * @param components Component mask to test.
 * @return true when entity has every requested component, false otherwise.
 */
bool rohr_entity_components_check(Entity entity, RohrComponentMask components);

/**
 * @brief Checks whether an entity table index has all requested components.
 * @param index Component table index to inspect.
 * @param components Component mask to test.
 * @return true when index has every requested component, false otherwise.
 */
bool rohr_entity_index_components_check(EntityIndex index, RohrComponentMask components);

/**
 * @brief Creates a reusable entity group.
 * @return GroupIdResult containing a group id, or an error.
 */
GroupIdResult rohr_entity_group_create(void);

/** Assigns a unique fixed-size name to a generic group. */
EngineResult rohr_entity_group_name_set(GroupId group, const char *name);

/** Finds a live generic group by name. */
GroupIdResult rohr_entity_group_by_name_get(const char *name);

/** Returns a copy of a generic group's fixed-size name. */
GroupNameResult rohr_entity_group_name_get(GroupId group);

/**
 * @brief Destroys a generic entity group and clears member group references.
 * @param group Group id to destroy.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_entity_group_destroy(GroupId group);

/** Get the immediate lifetime owner; ENTITY_INVALID means independent. */
EntityResult rohr_entity_owner_get(Entity entity);
/**
 * Transfer ownership or release with ENTITY_INVALID, preserving world state,
 * parenting and connections. Updates soft-body membership without converting
 * entity types. May allocate ownership storage; failure leaves ownership intact.
 */
EngineResult rohr_entity_owner_set(Entity entity, Entity owner);

/**
 * @brief Adds an entity to a generic group.
 * @param group Group id to update.
 * @param entity Entity id to add.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_entity_group_add(GroupId group, Entity entity);

/**
 * @brief Removes an entity from a generic group.
 * @param group Group id to update.
 * @param entity Entity id to remove.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_entity_group_remove(GroupId group, Entity entity);

/**
 * @brief Checks whether an entity belongs to a group.
 * @param group Group id to inspect.
 * @param entity Entity id to search for.
 * @return true when entity belongs to the group.
 */
bool rohr_entity_group_entity_check(GroupId group, Entity entity);

/**
 * @brief Returns an entity group.
 * @param group Group id to inspect.
 * @return EntityGroupResult containing group data, or an error.
 */
EntityGroupResult rohr_entity_group_get(GroupId group);

/**
 * @brief Returns the groups assigned to an entity.
 * @param entity Entity id to inspect.
 * @return EntityGroupMembershipResult containing group ids, or an error.
 */
EntityGroupMembershipResult rohr_entity_groups_get(Entity entity);

/**
 * @brief Removes components from an entity.
 * @param entity Stable entity id to modify.
 * @param mask Component mask to remove.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_entity_components_delete(Entity entity, RohrComponentMask mask);

/**
 * @brief Adds a child relationship from parent to child.
 * @param parent Parent entity id.
 * @param child Child entity id.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_entity_child_set(Entity parent, Entity child);

/**
 * @brief Sets an entity parent relationship.
 * @param child Child entity id.
 * @param parent Parent entity id.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_entity_parent_set(Entity child, Entity parent);

/**
 * @brief Removes the parent relationship from an entity.
 * @param child Child entity id.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_entity_parent_remove(Entity child);

/**
 * @brief Removes a child relationship from a parent entity.
 * @param parent Parent entity id.
 * @param child Child entity id.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_entity_child_remove(Entity parent, Entity child);

/**
 * @brief Returns the children group assigned to an entity.
 * @param entity Stable entity id to inspect.
 * @return ChildrenResult containing a group id, or an error.
 */
ChildrenResult rohr_entity_children_get(Entity entity);

/**
 * @brief Returns the parent assigned to an entity.
 * @param entity Stable entity id to inspect.
 * @return ParentResult containing parent id, or an error.
 */
ParentResult rohr_entity_parent_get(Entity entity);

/**
 * @brief Replaces absolute lifetime deadlines; both disabled removes the configuration.
 * @param entity Stable entity id to modify.
 * @param description Explicit flags allow enabled zero deadlines. Time must be finite and nonnegative.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_entity_lifetime_description_set(Entity entity, EntityLifetimeDescription description);
/** Get lifetime configuration, or disabled zeros when absent on a live entity. */
EntityLifetimeDescriptionResult rohr_entity_lifetime_description_get(Entity entity);

/**
 * @brief Removes lifetime data from an entity.
 * @param entity Stable entity id to modify.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_entity_life_time_remove(Entity entity);

/** Sets an explicit simulation delta per engine tick. */
EngineResult rohr_physics_dt_per_tick_set(Time dt);
/** Returns the current physics delta per tick. */
Time rohr_physics_dt_per_tick_get(void);
/** Restores the engine time-per-tick default. */
void rohr_physics_engine_time_per_tick_use(void);
/** Sets constraint-solver iterations. Must be greater than zero. */
EngineResult rohr_physics_solver_iterations_set(uint32_t iterations);
/** Returns constraint-solver iterations. Defaults to 8. */
uint32_t rohr_physics_solver_iterations_get(void);
/** Sets integration and collision-detection substeps. Must be greater than zero. */
EngineResult rohr_physics_substeps_set(uint32_t substeps);
/** Returns physics substeps. Defaults to 1. */
uint32_t rohr_physics_substeps_get(void);
/** Sets the global acceleration applied to ROHR_GRAVITY entities. */
EngineResult rohr_physics_gravity_set(Acceleration gravity);
/** Set one coordinate, preserving the other in the same coordinate space.
 * Validation and transform effects match the full-value setter. */
EngineResult rohr_physics_gravity_x_set(float x);
EngineResult rohr_physics_gravity_y_set(float y);
/** Returns the current global gravity acceleration. */
Acceleration rohr_physics_gravity_get(void);
/** Enables engine gravity for an entity. */
EngineResult rohr_physics_gravity_enable(Entity entity);
/** Disables engine gravity for an entity. */
EngineResult rohr_physics_gravity_disable(Entity entity);
/** Returns whether engine gravity is enabled for an entity. */
bool rohr_physics_gravity_check(Entity entity);
/** Begins a complete custom physics step and advances interaction state. */
void rohr_physics_pipeline_step_begin(void);
/** Clears transient constraints before one custom substep. */
void rohr_physics_pipeline_substep_begin(void);
/** Clears force-derived acceleration from the previous substep. */
void rohr_physics_pipeline_accelerations_clear(void);
/** Applies global gravity to opted-in movable entities. */
void rohr_physics_pipeline_gravity_apply(void);
/** Applies spring-joint and soft-body-beam forces. */
Report rohr_physics_pipeline_forces_apply(void);
/** Integrates rigid-body state and reports world-boundary failures. */
Report rohr_physics_pipeline_integrate(double dt);
/** Detects contacts and gathers contact constraints. */
Report rohr_physics_pipeline_contacts_gather(void);
/** Gathers active pin and weld joint constraints. */
Report rohr_physics_pipeline_joints_gather(void);
/** Solves currently gathered contacts and joints. */
Report rohr_physics_pipeline_constraints_solve(uint32_t iterations);
/** Runs one standard physics substep. */
Report rohr_physics_pipeline_substep(double dt);
/** Runs the standard plug-and-play physics pipeline. */
Report rohr_physics_pipeline_update(double dt);
/** Advances physics and returns any pipeline failure. */
Report rohr_physics_update(Tick ticks);
/** Advances physics once with an explicit exceptional delta. */
Report rohr_physics_dt_update(Time dt);

/**
 * @brief Translates a local shape into world space.
 * @param shape Shape with origin-relative local vertices; no recentering occurs.
 * @param position World position of the local origin.
 * @param angle World orientation in clockwise degrees.
 * @return World-space shape.
 */
Shape rohr_physics_shape_world_translate(Shape shape, Position position, Orientation angle);
/** Clockwise-positive radians alternative; other parameters retain their units. */
Shape rohr_physics_shape_world_radians_translate(Shape shape, Position position, Orientation angle);

/**
 * @brief Calculates polygon moment of inertia.
 * @param shape Polygon shape.
 * @param mass_value Shape mass.
 * @return Moment of inertia value.
 */
float rohr_physics_polygon_moment_of_inertia(Shape shape, Mass mass_value);

/**
 * @brief Gets overlap information using the separating axis theorem.
 * @param shape_1 First shape.
 * @param shape_2 Second shape.
 * @return Geometric overlap information.
 */
OverlapInfo rohr_physics_sat_overlap_get(Shape shape_1, Shape shape_2);

/**
 * @brief Calculates circle moment of inertia.
 * @param circle Circle shape.
 * @param mass_value Circle mass.
 * @return Moment of inertia value.
 */
Vec1D rohr_physics_circle_moment_of_inertia(Shape circle, Mass mass_value);

/**
 * @brief Checks whether an entity index has ROHR_HOLD.
 * @param index Entity table index to inspect.
 * @return true when index is live and held, false otherwise.
 */
bool rohr_physics_entity_held_get(EntityIndex index);

/**
 * @brief Sets an entity acceleration component value.
 * @param entity Entity to modify.
 * @param a Acceleration value.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_acceleration_set(Entity entity, Acceleration a);
/** Set one coordinate, preserving the other in the same coordinate space. Missing storage starts with zero for the other coordinate.
 * Validation and transform effects match the full-value setter. */
EngineResult rohr_physics_acceleration_x_set(Entity entity, float x);
EngineResult rohr_physics_acceleration_y_set(Entity entity, float y);

/**
 * @brief Sets an entity angular acceleration component value.
 * @param entity Entity to modify.
 * @param acceleration Angular acceleration value.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_angular_acceleration_set(
    Entity entity,
    AngularAcceleration acceleration
);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult rohr_physics_angular_acceleration_radians_set(
    Entity entity,
    AngularAcceleration acceleration
);

/**
 * @brief Sets entity acceleration toward a world position.
 * @param entity Entity to modify.
 * @param acceleration_magnitude Acceleration magnitude to apply along the direction to position.
 * @param position Target world position.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_acceleration_toward_position_set(Entity entity, float acceleration_magnitude, Position position);

/**
 * @brief Sets entity acceleration toward another entity's current world position.
 * @param entity Entity to modify.
 * @param acceleration_magnitude Acceleration magnitude to apply along the direction to target.
 * @param target Target entity.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_acceleration_toward_entity_set(Entity entity, float acceleration_magnitude, Entity target);

/**
 * @brief Sets entity acceleration away from a world position.
 * @param entity Entity to modify.
 * @param acceleration_magnitude Acceleration magnitude to apply away from position.
 * @param position Source world position.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_acceleration_away_from_position_set(Entity entity, float acceleration_magnitude, Position position);

/**
 * @brief Sets entity acceleration away from another entity's current world position.
 * @param entity Entity to modify.
 * @param acceleration_magnitude Acceleration magnitude to apply away from target.
 * @param target Source entity.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_acceleration_away_from_entity_set(Entity entity, float acceleration_magnitude, Entity target);

/**
 * @brief Sets acceleration toward an entity for every live entity in a group.
 * @param group Group id to update.
 * @param acceleration_magnitude Acceleration magnitude to apply along the direction to target.
 * @param target Target entity.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_group_acceleration_toward_entity_set(GroupId group, float acceleration_magnitude, Entity target);

/**
 * @brief Sets acceleration away from an entity for every live entity in a group.
 * @param group Group id to update.
 * @param acceleration_magnitude Acceleration magnitude to apply away from target.
 * @param target Source entity.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_group_acceleration_away_from_entity_set(GroupId group, float acceleration_magnitude, Entity target);

/**
 * @brief Sets an entity velocity component value.
 * @param entity Entity to modify.
 * @param v Velocity value.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_velocity_set(Entity entity, Velocity v);
/** Set one coordinate, preserving the other in the same coordinate space. Missing storage starts with zero for the other coordinate.
 * Validation and transform effects match the full-value setter. */
EngineResult rohr_physics_velocity_x_set(Entity entity, float x);
EngineResult rohr_physics_velocity_y_set(Entity entity, float y);

/** Check that both coordinates are finite and inside the supported world. */
bool rohr_physics_world_position_check(Position position);
EngineResult rohr_physics_hitbox_autoshape_circle_set(Entity entity,
    float radius);
EngineResult rohr_physics_hitbox_autoshape_rectangle_set(Entity entity,
    float width, float height);
EngineResult rohr_physics_hitbox_autoshape_triangle_set(Entity entity,
    float width, float height);
EngineResult rohr_physics_soft_body_autoshape_circle_set(Entity soft_body,
    float radius);
EngineResult rohr_physics_soft_body_autoshape_rectangle_set(Entity soft_body,
    float width, float height);
EngineResult rohr_physics_soft_body_autoshape_triangle_set(Entity soft_body,
    float width, float height);

/**
 * @brief Sets entity velocity toward a world position.
 * @param entity Entity to modify.
 * @param speed Speed to apply along the direction to position.
 * @param position Target world position.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_velocity_toward_position_set(Entity entity, float speed, Position position);

/**
 * @brief Sets entity velocity toward another entity's current world position.
 * @param entity Entity to modify.
 * @param speed Speed to apply along the direction to target.
 * @param target Target entity.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_velocity_toward_entity_set(Entity entity, float speed, Entity target);

/**
 * @brief Sets entity velocity away from a world position.
 * @param entity Entity to modify.
 * @param speed Speed to apply away from position.
 * @param position Source world position.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_velocity_away_from_position_set(Entity entity, float speed, Position position);

/**
 * @brief Sets entity velocity away from another entity's current world position.
 * @param entity Entity to modify.
 * @param speed Speed to apply away from target.
 * @param target Source entity.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_velocity_away_from_entity_set(Entity entity, float speed, Entity target);

/**
 * @brief Sets velocity toward an entity for every live entity in a group.
 * @param group Group id to update.
 * @param speed Speed to apply along the direction to target.
 * @param target Target entity.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_group_velocity_toward_entity_set(GroupId group, float speed, Entity target);

/**
 * @brief Sets velocity away from an entity for every live entity in a group.
 * @param group Group id to update.
 * @param speed Speed to apply away from target.
 * @param target Source entity.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_group_velocity_away_from_entity_set(GroupId group, float speed, Entity target);

/**
 * @brief Sets an entity velocity to zero.
 * @param entity Entity to modify.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_entity_stop(Entity entity);

/**
 * @brief Sets velocity to zero for every live entity in a group.
 * @param group Group id to update.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_group_entities_stop(GroupId group);

/**
 * @brief Applies an immediate linear impulse to an entity velocity.
 * @param entity Entity to modify.
 * @param impulse Impulse vector.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_impulse_apply(Entity entity, Vec2D impulse);

/**
 * @brief Adds an immediate angular impulse about the effective COM.
 * Adds degrees(impulse / derived inertia) to clockwise angular velocity without
 * changing motion type. Zero inverse inertia (including particles, static, held, kinematic,
 * massless, geometryless, and fully rotation-locked bodies) is a successful
 * no-op. Nonfinite input or resulting velocity is rejected without mutation.
 * @param entity Body to modify.
 * @param impulse Signed angular impulse in mass * distance squared / second.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_angular_impulse_apply(Entity entity, float impulse);

/**
 * @brief Sets the world origin while preserving local geometry and attachment offsets.
 * @param entity Entity to modify.
 * @param p Position value.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_position_set(Entity entity, Position p);
/** Set one coordinate, preserving the other in the same coordinate space. Missing storage starts with zero for the other coordinate.
 * Validation and transform effects match the full-value setter. */
EngineResult rohr_physics_position_x_set(Entity entity, float x);
EngineResult rohr_physics_position_y_set(Entity entity, float y);
/** Returns an entity's world position. */
PositionResult rohr_physics_position_get(Entity entity);

/**
 * @brief Selects explicit COM using a finite origin-relative offset.
 * Geometry is optional. Standalone particles reject overrides.
 * Soft bodies redistribute positive node masses at fixed total mass to the
 * requested COM, preserving their prior proportions as closely as possible.
 * Unreachable targets and numerical failures leave every node unchanged.
 * Failure preserves configuration; success preserves origin, geometry, and velocities.
 */
EngineResult rohr_physics_center_of_mass_local_position_set(Entity entity, Position local_offset);
/** Set one coordinate, preserving the other in the same coordinate space. Uses the effective center; rigid bodies become explicit.
 * Validation and transform effects match the full-value setter. */
EngineResult rohr_physics_center_of_mass_local_position_x_set(Entity entity, float x);
EngineResult rohr_physics_center_of_mass_local_position_y_set(Entity entity, float y);
/**
 * @brief Returns the effective COM in local origin axes.
 * Automatic COM requires valid active geometry; an explicit offset does not.
 * Standalone particles use their circle centroid.
 * Soft bodies derive COM from current node positions and masses; an empty
 * or zero-total-mass soft body has no available COM.
 */
PositionResult rohr_physics_center_of_mass_local_position_get(Entity entity);
/**
 * @brief Returns origin plus rotated local COM.
 * Requires a transform and an available local COM.
 */
PositionResult rohr_physics_center_of_mass_world_position_get(Entity entity);
/** Discards explicit COM for ordinary bodies. Soft bodies: successful no-op. */
EngineResult rohr_physics_center_of_mass_automatic_set(Entity entity);
/** Returns the COM mode (always true for soft bodies), or an invalid-entity error. */
BoolResult rohr_physics_center_of_mass_automatic_check(Entity entity);
/**
 * @brief Returns derived inertia about the effective COM.
 * Requires valid geometry and stored mass, including zero mass.
 * Static and locked bodies may report inertia without enabling angular response.
 */
MomentOfInertiaResult rohr_physics_moment_of_inertia_get(Entity entity);


/**
 * @brief Sets an entity mass component value.
 * @param entity Entity to modify.
 * @param m Mass value.
 * @return EngineResult describing success or failure.
 */
/** Sets finite non-negative ordinary mass. For soft bodies, proportionally
 * scales positive node masses to a positive total; failures preserve all nodes.
 * No ROHR_MASS component is added to the soft-body container. */
EngineResult rohr_physics_mass_set(Entity entity, Mass m);
/** Returns stored mass, or summed node mass for a soft body (zero when empty). */
MassResult rohr_physics_mass_get(Entity entity);
EngineResult rohr_physics_mass_remove(Entity entity);
bool rohr_physics_mass_check(Entity entity);
EngineResult rohr_physics_kinematic_driven_set(Entity entity);
EngineResult rohr_physics_kinematic_driven_remove(Entity entity);
bool rohr_physics_kinematic_driven_check(Entity entity);

/** Copies a description into one persistent source; delete its entity to stop future contributions. */
EntityResult rohr_physics_force_add(ForceDescription description);
/** Copies a description into one persistent source; delete its entity to stop future contributions. */
EntityResult rohr_physics_torque_add(TorqueDescription description);

/** Returns configuration with the current generic owner; invalid sources return an error. */
ForceDescriptionResult rohr_physics_force_description_get(Entity source);
/** Returns configuration with the current generic owner; invalid sources return an error. */
TorqueDescriptionResult rohr_physics_torque_description_get(Entity source);
/** Atomically replaces configuration and lifetime owner; invalid input leaves both unchanged. */
EngineResult rohr_physics_force_description_set(Entity source, ForceDescription description);
/** Atomically replaces configuration and lifetime owner; invalid input leaves both unchanged. */
EngineResult rohr_physics_torque_description_set(Entity source, TorqueDescription description);

/** Replaces this field on an existing source using full-description validation. */
EngineResult rohr_physics_force_target_set(Entity source, Entity target);
/** Returns copied stored configuration; invalid handles or missing source components return an error. */
EntityResult rohr_physics_force_target_get(Entity source);
/** Replaces this field on an existing source using full-description validation. */
EngineResult rohr_physics_force_location_set(Entity source, ForceLocation location);
/** Returns copied stored configuration; invalid handles or missing source components return an error. */
ForceLocationResult rohr_physics_force_location_get(Entity source);
/** Replaces this field on an existing source using full-description validation. */
EngineResult rohr_physics_force_orientation_set(Entity source, ForceOrientation orientation);
/** Returns copied stored configuration; invalid handles or missing source components return an error. */
ForceOrientationResult rohr_physics_force_orientation_get(Entity source);
/** Replaces this field on an existing source using full-description validation. */
EngineResult rohr_physics_force_magnitude_set(Entity source, float magnitude);
/** Returns copied stored configuration; invalid handles or missing source components return an error. */
ForceMagnitudeResult rohr_physics_force_magnitude_get(Entity source);

/** Replaces this field on an existing source using full-description validation. */
EngineResult rohr_physics_torque_target_set(Entity source, Entity target);
/** Returns copied stored configuration; invalid handles or missing source components return an error. */
EntityResult rohr_physics_torque_target_get(Entity source);
/** Replaces this field on an existing source using full-description validation. */
EngineResult rohr_physics_torque_magnitude_set(Entity source, Torque magnitude);
/** Returns copied stored configuration; invalid handles or missing source components return an error. */
TorqueResult rohr_physics_torque_magnitude_get(Entity source);

/** Copies a source for engine tick T, expiring through lifecycle cleanup at T + 1.
 * Requires owner == ENTITY_INVALID; use add for a persistent owned source. */
EngineResult rohr_physics_force_apply(ForceDescription description);
/** Copies a source for engine tick T, expiring through lifecycle cleanup at T + 1.
 * Requires owner == ENTITY_INVALID; use add for a persistent owned source. */
EngineResult rohr_physics_torque_apply(TorqueDescription description);


/**
 * @brief Sets a simple polygon hitbox and prepares hidden convex decomposition when needed.
 *
 * Degenerate and self-intersecting outlines are rejected. This does not enable
 * physical collision response.
 * @param entity Entity to modify.
 * @param hitbox Local-space hitbox shape.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_hitbox_set(Entity entity, Shape hitbox);
/** Returns the active local-space hitbox variant. */
ShapeResult rohr_physics_hitbox_get(Entity entity);
/** Removes every hitbox variant and the ROHR_HIT_BOX component. */
EngineResult rohr_physics_hitbox_remove(Entity entity);
/** Appends a local-space hitbox variant. The first variant becomes active. */
EngineResult rohr_physics_hitbox_add(Entity entity, Shape hitbox);
/** Returns a local-space hitbox variant by ordered index. */
ShapeResult rohr_physics_hitbox_at_get(Entity entity, size_t index);
/** Replaces a local-space hitbox variant by ordered index. */
EngineResult rohr_physics_hitbox_at_set(Entity entity, size_t index, Shape hitbox);
/** Removes one local-space hitbox variant by ordered index. */
EngineResult rohr_physics_hitbox_at_remove(Entity entity, size_t index);
/** Returns the number of hitbox variants owned by an entity. */
HitboxIndexResult rohr_physics_hitbox_count_get(Entity entity);
/** Returns the active hitbox variant index. */
HitboxIndexResult rohr_physics_hitbox_active_index_get(Entity entity);
/** Selects the hitbox variant used by the next physics tick. */
EngineResult rohr_physics_hitbox_active_index_set(Entity entity, size_t index);
HitboxIdResult rohr_physics_hitbox_id_at_get(Entity entity, size_t index);
EngineResult rohr_physics_hitbox_id_at_set(Entity entity, size_t index,
    HitboxId id);
ShapeResult rohr_physics_hitbox_by_id_get(Entity entity, HitboxId id);
EngineResult rohr_physics_hitbox_by_id_set(Entity entity, HitboxId id,
    Shape hitbox);
EngineResult rohr_physics_hitbox_by_id_remove(Entity entity, HitboxId id);
HitboxIdResult rohr_physics_hitbox_active_id_get(Entity entity);
EngineResult rohr_physics_hitbox_active_id_set(Entity entity, HitboxId id);
EngineResult rohr_physics_hitbox_animation_binding_set(Entity entity,
    AnimationId animation_id, AnimationFrameId frame_id, HitboxId hitbox_id);
EngineResult rohr_physics_hitbox_animation_binding_remove(Entity entity,
    AnimationId animation_id, AnimationFrameId frame_id);
HitboxIdResult rohr_physics_hitbox_animation_binding_get(Entity entity,
    AnimationId animation_id, AnimationFrameId frame_id);
EngineResult rohr_physics_hitbox_animation_binding_at_set(Entity entity,
    size_t frame_index, size_t hitbox_index);
EngineResult rohr_physics_hitbox_animation_binding_at_remove(Entity entity,
    size_t frame_index);
HitboxIndexResult rohr_physics_hitbox_animation_binding_at_get(Entity entity,
    size_t frame_index);
void rohr_physics_hitbox_animation_bindings_update(void);

/** Return the default collision filtering configuration. */
CollisionFilterDescription rohr_physics_collision_filter_description_default_get(void);
/** Replace an entity's collision category and response mask, with or without geometry. */
EngineResult rohr_physics_collision_filter_description_set(Entity entity, CollisionFilterDescription config);
/** Return an entity's collision filtering configuration. */
CollisionFilterDescriptionResult rohr_physics_collision_filter_description_get(Entity entity);
/** Set the collision categories represented by an entity. */
EngineResult rohr_physics_collision_category_set(Entity entity, RohrCollisionCategoryMask category);
/** Set the categories this entity responds to. */
EngineResult rohr_physics_collision_response_set(Entity entity, RohrCollisionCategoryMask categories);
/** Make an entity respond to every category. */
EngineResult rohr_physics_collision_response_all_set(Entity entity);
/** Disable this entity's response to all categories; other entities may still respond to it. */
EngineResult rohr_physics_collision_response_none_set(Entity entity);
/** Check whether the first mask responds to the second category; ignores geometry and motion. */
bool rohr_physics_collision_response_to_check(Entity entity, Entity target);
/** Check whether either directional mask permits response; does not test contact. */
bool rohr_physics_collision_response_between_check(Entity entity_1, Entity entity_2);

/**
 * @brief Sets an entity orientation component value.
 * @param entity Entity to modify.
 * @param angle Orientation in clockwise degrees.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_orientation_set(Entity entity, Orientation angle);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult rohr_physics_orientation_radians_set(Entity entity, Orientation angle);

/**
 * @brief Sets clockwise angular velocity in degrees per second.
 * For a soft body, adjusts node velocities around its mass-weighted COM,
 * preserving COM translation and deformation motion. This is a one-time edit,
 * not a constraint. Node body modes are preserved; no container angular state
 * is created. Empty/degenerate bodies and invalid or overflowing values fail
 * without mutation.
 * @param entity Entity to modify.
 * @param v Angular velocity value.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_angular_velocity_set(Entity entity, AngularVelocity v);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult rohr_physics_angular_velocity_radians_set(Entity entity, AngularVelocity v);

/**
 * @brief Returns an entity angular velocity.
 * For a soft body, derives clockwise angular momentum divided by inertia from
 * current node motion about COM. Empty or degenerate bodies return an error;
 * no single physical orientation is implied.
 * @param entity Entity to inspect.
 * @return AngularVelocityResult containing clockwise degrees per second, or an error.
 */
AngularVelocityResult rohr_physics_angular_velocity_get(Entity entity);
/** Clockwise-positive radians alternative; other parameters retain their units. */
AngularVelocityResult rohr_physics_angular_velocity_radians_get(Entity entity);

/** Sets the absolute angular-velocity limit applied before orientation integration. */
EngineResult rohr_physics_angular_velocity_maximum_set(
    Entity entity,
    AngularVelocity maximum
);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult rohr_physics_angular_velocity_maximum_radians_set(
    Entity entity,
    AngularVelocity maximum
);
/** Returns an entity's configured absolute angular-velocity limit. */
AngularVelocityResult rohr_physics_angular_velocity_maximum_get(Entity entity);
/** Clockwise-positive radians alternative; other parameters retain their units. */
AngularVelocityResult rohr_physics_angular_velocity_maximum_radians_get(Entity entity);

/**
 * @brief Returns the active hitbox at the current origin and orientation, without waiting for a physics tick.
 * @param entity Entity to inspect.
 * @return ShapeResult containing the world-space hitbox, or an error.
 */
ShapeResult rohr_physics_global_hit_box_get(Entity entity);

/**
 * @brief Sets an entity restitution value.
 * For soft-body beams, stores an independent coefficient without enabling
 * collision. Beam values must be finite and in [0, 1]; errors preserve state.
 * @param entity Entity to modify.
 * @param restitution Restitution value.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_restitution_set(Entity entity, Restitution restitution);

/**
 * @brief Marks an entity as dynamic for physics simulation.
 * @param entity Entity to modify.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_dynamic_set(Entity entity);

/**
 * @brief Marks an entity as static for physics simulation.
 * @param entity Entity to modify.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_static_set(Entity entity);

/**
 * @brief Adds ROHR_HOLD so physics update stages preserve current values.
 * @param entity Entity to modify.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_entity_hold(Entity entity);

/**
 * @brief Removes ROHR_HOLD without changing ROHR_STATIC or ROHR_DYNAMIC state.
 * @param entity Entity to modify.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_entity_unhold(Entity entity);

/**
 * @brief Adds ROHR_HOLD to every live entity in a group.
 * @param group Group id to update.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_group_entities_hold(GroupId group);

/**
 * @brief Removes ROHR_HOLD from every live entity in a group.
 * @param group Group id to update.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_group_entities_unhold(GroupId group);

/**
 * @brief Locks an entity orientation between minimum and maximum angles.
 * @param entity Entity to modify.
 * @param min Minimum orientation in clockwise degrees.
 * @param max Maximum orientation in clockwise degrees.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_angle_lock_set(Entity entity, Orientation min, Orientation max);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult rohr_physics_angle_lock_radians_set(Entity entity, Orientation min, Orientation max);

/** Atomically replace the axis constraint; finite nonzero axis required.
 * Stores a normalized axis without changing current motion or ownership. */
EngineResult rohr_physics_axis_lock_description_set(Entity entity, AxisLockDescription description);
/** Get stored configuration; returns component-missing when no axis lock exists. */
AxisLockDescriptionResult rohr_physics_axis_lock_description_get(Entity entity);
/** Remove the axis lock without changing current motion. Idempotent for live entities. */
EngineResult rohr_physics_axis_lock_remove(Entity entity);

/**
 * @brief Sets an entity friction value.
 * For soft-body beams, stores an independent coefficient without enabling
 * collision. Beam values must be finite and nonnegative; errors preserve state.
 * @param entity Entity to modify.
 * @param friction Friction value.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_friction_set(Entity entity, float friction);

/** Atomically replace transform-following configuration using clockwise degrees.
 * Requires a live driver even when all flags are false. Does not capture the
 * current transform, move entities, or alter ownership. Use explicit conversion
 * helpers when supplying radians. */
EngineResult rohr_physics_transform_lock_description_set(Entity driven, TransformLockDescription description);
/** Get stored configuration, including its driver handle; component-missing
 * when absent. A deleted driver can remain in the stored configuration. */
TransformLockDescriptionResult rohr_physics_transform_lock_description_get(Entity driven);

/**
 * @brief Removes an entity transform lock.
 * @param entity Entity to modify.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_transform_lock_remove(Entity entity);

/**
 * @brief Locks one entity to another using their current transform offset.
 * @param driven Entity whose transform is driven.
 * @param driver Entity used as the transform source.
 * @param lock_position true to lock position.
 * @param lock_orientation true to lock orientation.
 * @param inherit_velocity true to inherit driver velocity.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_transform_lock_current_transform_set(
    Entity driven,
    Entity driver,
    bool lock_position,
    bool lock_orientation,
    bool inherit_velocity
);

/**
 * @brief Sets the target used by a force or torque source entity.
 * @param entity Force or torque source entity to modify.
 * @param target Live entity that receives the force or torque.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_target_set(Entity entity, Entity target);

/** Atomically replace the complete configuration and lifetime owner.
 * Invalid anchors disconnect endpoints; stale nonzero handles are rejected.
 * Automatic rest values measure the current pose once, or zero when disconnected.
 * Failure leaves the entity, configuration, and ownership unchanged. */
EngineResult rohr_physics_joint_description_set(Entity entity, JointDescription description);
/** Return the current owner and resolved rest values with explicit flags true.
 * Get-modify-set preserves the resting pose even after the bodies have moved. */
JointDescriptionResult rohr_physics_joint_description_get(Entity entity);
/** Explicit, transactional placement; see physics_joint_place. */
EngineResult rohr_physics_joint_place(Entity entity);

/** Full replacements are atomic; ownership never follows connections. */
SoftBodyDescription rohr_physics_soft_body_description_default_get(void);
/** Atomically replace soft body configuration. */
EngineResult rohr_physics_soft_body_description_set(Entity entity, SoftBodyDescription description);
/** Return current soft body configuration. */
SoftBodyDescriptionResult rohr_physics_soft_body_description_get(Entity entity);
/** Full node descriptions use world positions; local creation remains explicit. */
SoftBodyNodeDescription rohr_physics_soft_body_node_description_default_get(void);
/** Atomically replace soft body node configuration. */
EngineResult rohr_physics_soft_body_node_description_set(Entity entity, SoftBodyNodeDescription description);
/** Return current soft body node configuration. */
SoftBodyNodeDescriptionResult rohr_physics_soft_body_node_description_get(Entity entity);
/** Return defaults for soft body beam configuration. */
SoftBodyBeamDescription rohr_physics_soft_body_beam_description_default_get(void);
/** Atomically replace soft body beam configuration. */
EngineResult rohr_physics_soft_body_beam_description_set(Entity entity, SoftBodyBeamDescription description);
/** Resolved rest length is returned with rest_length_explicit true. */
SoftBodyBeamDescriptionResult rohr_physics_soft_body_beam_description_get(Entity entity);
/** Return defaults for soft body beam collision configuration. */
SoftBodyBeamCollisionDescription rohr_physics_soft_body_beam_collision_description_default_get(void);
/** Return current soft body beam collision configuration. */
SoftBodyBeamCollisionDescriptionResult rohr_physics_soft_body_beam_collision_description_get(Entity entity);
/** Return defaults for soft body area configuration. */
SoftBodyAreaDescription rohr_physics_soft_body_area_description_default_get(void);
/** Atomically replace soft body area configuration. */
EngineResult rohr_physics_soft_body_area_description_set(Entity entity, SoftBodyAreaDescription description);
/** Return current soft body area configuration. */
SoftBodyAreaDescriptionResult rohr_physics_soft_body_area_description_get(Entity entity);
/** Reattach the same anchor handle and all its joint references atomically.
 * Keep joint owners and rest values; offsets are local to the new entity. */
EngineResult rohr_physics_joint_anchor_description_set(JointAnchorId anchor, JointAnchorDescription description);
/** Return current joint anchor configuration. */
JointAnchorDescriptionResult rohr_physics_joint_anchor_description_get(JointAnchorId anchor);

/**
 * @brief Creates an anchor owned by an entity.
 * @param description Attached entity and offset from its stable local origin.
 * @return JointAnchorIdResult containing the stable anchor handle, or an error.
 */
JointAnchorIdResult rohr_physics_joint_anchor_add(JointAnchorDescription description);
/**
 * @brief Returns the anchors owned by an entity.
 * @param entity Entity whose anchors should be listed.
 * @return JointAnchorListResult containing the anchor handles, or an error.
 */
JointAnchorListResult rohr_physics_joint_anchors_get(Entity entity);
/**
 * @brief Returns an anchor offset relative to its owner's stable local origin.
 * @param anchor Anchor to inspect.
 * @return JointAnchorPositionResult containing the origin-relative local offset, or an error.
 */
JointAnchorPositionResult rohr_physics_joint_anchor_local_position_get(JointAnchorId anchor);
/**
 * @brief Returns the current world position of an anchor.
 * @param anchor Anchor to resolve.
 * @return JointAnchorPositionResult containing its world position, or an error.
 */
JointAnchorPositionResult rohr_physics_joint_anchor_world_position_get(JointAnchorId anchor);
/**
 * @brief Sets an anchor offset relative to its owner's stable local origin.
 * @param anchor Anchor to modify.
 * @param local_offset New origin-relative local offset.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_joint_anchor_local_position_set(JointAnchorId anchor, Vec2D local_offset);
/** Set one coordinate, preserving the other in the same coordinate space.
 * Validation and transform effects match the full-value setter. */
EngineResult rohr_physics_joint_anchor_local_position_x_set(JointAnchorId anchor, float x);
EngineResult rohr_physics_joint_anchor_local_position_y_set(JointAnchorId anchor, float y);
/**
 * @brief Removes an anchor and disconnects joint endpoints that reference it.
 * @param anchor Anchor to remove.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_joint_anchor_remove(JointAnchorId anchor);
/**
 * @brief Creates an empty soft body and its origin transform.
 *
 * The returned owner entity is also the body's origin and may be passed to
 * rohr_physics_position_set and rohr_physics_orientation_set. Changing that
 * transform translates or rotates owned nodes without changing their
 * topology. SoftBody.origin exposes the same entity for attachment APIs.
 *
 * @param description Copied owner and initial world pose; zero initializes an
 * independent empty body at the origin. Owner may be any live entity.
 * @return EntityResult containing the owner/origin entity.
 */
EntityResult rohr_physics_soft_body_add(SoftBodyDescription description);
/** @brief Returns soft-body topology. @param soft_body Soft-body owner. @return SoftBodyResult. */
SoftBodyResult rohr_physics_soft_body_get(Entity soft_body);
/**
 * @brief Creates a lightweight point-mass node.
 * @param description Copied owner, world position, positive mass and radius.
 * An invalid owner means independent lifetime; any live owner is accepted.
 * Omitted or zero mass/radius is invalid. Omitted flags/material/filter are zero;
 * use the default description getter for standard node collision and material.
 * @return EntityResult containing the node entity.
 */
EntityResult rohr_physics_soft_body_node_add(SoftBodyNodeDescription description);
EngineResult rohr_physics_soft_body_beam_endpoints_set(Entity beam, Entity node_a, Entity node_b);
/** Creates a node from an owner-local position. Requires a live soft-body owner;
 * uses its origin position and orientation exactly once. */
EntityResult rohr_physics_soft_body_node_local_add(SoftBodyNodeDescription description);
/** @brief Returns soft-body node data. @param node Node entity. @return SoftBodyNodeResult. */
SoftBodyNodeResult rohr_physics_soft_body_node_get(Entity node);
/** Returns a node position relative to its soft-body origin. */
PositionResult rohr_physics_soft_body_node_local_position_get(Entity node);
/** Sets a node position relative to its soft-body origin. */
EngineResult rohr_physics_soft_body_node_local_position_set(
    Entity node, Position local_position);
/** Set one coordinate, preserving the other in the same coordinate space.
 * Validation and transform effects match the full-value setter. */
EngineResult rohr_physics_soft_body_node_local_position_x_set(Entity entity, float x);
EngineResult rohr_physics_soft_body_node_local_position_y_set(Entity entity, float y);
/**
 * @brief Sets node-versus-rigid collision filtering.
 * @param node Node entity.
 * @param description Categories represented and accepted by the node.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_soft_body_node_collision_filter_description_set(Entity node, CollisionFilterDescription description);
/** Return current soft body node collision filter configuration. */
CollisionFilterDescriptionResult rohr_physics_soft_body_node_collision_filter_description_get(Entity node);

/**
 * @brief Applies an immediate impulse to one soft-body node.
 * @param node Soft-body node entity.
 * @param impulse Impulse to apply.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_physics_soft_body_node_impulse_apply(Entity node, Vec2D impulse);
/**
 * @brief Pins a soft-body node to an existing anchor.
 * @param description Joint owner, soft-body node, and existing target anchor.
 * @return SoftBodyNodeAnchorPinResult containing the joint and node-owned anchor.
 * Delete the joint and remove node_anchor separately to release the attachment.
 * Removing an anchor disconnects referencing joints; it does not delete them.
 */
SoftBodyNodeAnchorPinResult rohr_physics_soft_body_node_to_anchor_pin_add(SoftBodyNodeAnchorPinDescription description);
/**
 * @brief Creates an elastic beam between two nodes.
 * Collision defaults off; thickness and filters are explicit and never derived
 * from endpoints. Use the default getter for fixed material/filter/width defaults.
 * @param description Copied owner, endpoints, spring, material, collision and color.
 * Endpoints may have different owners or be ENTITY_INVALID. Rest length is
 * measured once from complete endpoints unless rest_length_explicit is true;
 * disconnected measurement resolves to zero. Any live owner is accepted.
 * @return EntityResult containing the beam entity.
 */
EntityResult rohr_physics_soft_body_beam_add(SoftBodyBeamDescription description);
/** @brief Returns soft-body beam data. @param beam Beam entity. @return SoftBodyBeamResult. */
SoftBodyBeamResult rohr_physics_soft_body_beam_get(Entity beam);
/**
 * Atomically replaces a beam's independent collision configuration.
 * Thickness is stored unchanged. Enabled collision requires the engine minimum;
 * disabled collision permits zero. Node edits do not change these settings.
 */
EngineResult rohr_physics_soft_body_beam_collision_description_set(
    Entity beam, SoftBodyBeamCollisionDescription config);
/** Enables thick-segment collision when the stored width meets the engine minimum. */
EngineResult rohr_physics_soft_body_beam_collision_enable(Entity beam);
/** Disables thick-segment collision for a beam without discarding its thickness. */
EngineResult rohr_physics_soft_body_beam_collision_disable(Entity beam);
/** Sets an explicit beam collision thickness without endpoint-based clamping. */
EngineResult rohr_physics_soft_body_beam_collision_thickness_set(
    Entity beam, float thickness);
/** Sets the collision filters owned independently by a beam. */
EngineResult rohr_physics_soft_body_beam_collision_filter_description_set(Entity beam, CollisionFilterDescription description);
/** Return current soft body beam collision filter configuration. */
CollisionFilterDescriptionResult rohr_physics_soft_body_beam_collision_filter_description_get(Entity beam);
/**
 * @brief Creates an area from an outer loop minus its owned hole loops.
 * Copies geometry. Complete loops contain 3..64 distinct live nodes and close
 * implicitly. Nodes may have other owners. Self-crossing loops are accepted
 * when they enclose space; incomplete loops remain stored without a fill.
 * Every enclosed outer region is filled once, minus all enclosed hole regions,
 * independent of traversal direction. Holes may extend beyond the outer loop.
 * More than 16 holes returns ERROR_ENGINE_MAX_AREA_HOLES_EXCEEDED.
 * Invalid creation creates nothing. Areas add no mass or collision.
 * Descriptions supply visibility and optional color; the default getter gives
 * visible, inherited color. New areas append to the owner list order.
 * Node motion updates the fill, including collapse and crossings. Delete with
 * rohr_entity_delete. Deleting its owner deletes the area; deleting a referenced
 * node clears that slot and leaves the area alive. Nodes may have other owners.
 * @param description Copied owner and geometry. Any live owner is accepted;
 * ENTITY_INVALID means independent lifetime. Incomplete loops remain supported.
 */
EntityResult rohr_physics_soft_body_area_add(SoftBodyAreaDescription description);
/** Returns a copied geometry/style snapshot, without cache or borrowed storage. */
SoftBodyAreaResult rohr_physics_soft_body_area_get(Entity area);
/** Replaces all loops transactionally. Invalid geometry (including more than
 * 16 holes) preserves the previous description, style, entity and layer/order.
 * Full subtraction is valid; each authored loop must itself enclose space. */
EngineResult rohr_physics_soft_body_area_geometry_set(Entity area, SoftBodyAreaGeometry geometry);
/** Moves the area to a zero-based index in its owner's area list.
 * Later areas draw on top within the same layer. Out-of-range indices fail. */
EngineResult rohr_physics_soft_body_area_order_set(Entity area, uint32_t index);

/**
 * @brief Creates a joint borrowing two anchor handles.
 * @param description Lifetime owner, anchor endpoints, joint behavior, and automatic or explicit rest values.
 * @return EntityResult containing the joint entity, or an error.
 */
EntityResult rohr_physics_joint_add(JointDescription description);

/**
 * @brief Gets overlap information for two particle shapes.
 * @param shape_1 First shape.
 * @param shape_2 Second shape.
 * @return Geometric overlap information.
 */
OverlapInfo rohr_physics_particle_overlap_get(Shape shape_1, Shape shape_2);
/**
 * @brief Returns a valid standalone particle configuration with engine defaults.
 * @return ParticleDescription with a unit radius, default material and collision.
 */
ParticleDescription rohr_physics_particle_description_default_get(void);
/**
 * @brief Creates one translation-only particle entity.
 *
 * The particle radius and local origin drive both its circle geometry and
 * circular polygon hitbox. A rigid_vertices value of zero uses
 * ROHR_PARTICLE_RIGID_VERTICES_DEFAULT.
 *
 * @param config Reusable particle configuration and initial instance state.
 * @return EntityResult containing the created particle, or an error.
 */
EntityResult rohr_physics_particle_add(ParticleDescription config);
/** Atomically replace standalone particle configuration and ownership. Static
 * particles retain mass, velocity, acceleration, and gravity configuration.
 * Changed geometry updates both circle and active polygon, retaining hitbox IDs. */
EngineResult rohr_physics_particle_description_set(Entity entity, ParticleDescription description);
/** Get current configuration; non-standalone entities return component-missing. */
ParticleDescriptionResult rohr_physics_particle_description_get(Entity entity);
/** Set the circle origin and synchronize standalone particle polygon geometry. */
EngineResult rohr_physics_particle_origin_set(Entity entity, Position local_origin);
/** Set one coordinate, preserving the other in the same coordinate space. Preserves radius and synchronizes standalone particle geometry.
 * Validation and transform effects match the full-value setter. */
EngineResult rohr_physics_particle_origin_x_set(Entity entity, float x);
EngineResult rohr_physics_particle_origin_y_set(Entity entity, float y);
/** Get a particle circle's origin relative to its rigid-body origin. */
PositionResult rohr_physics_particle_origin_get(Entity entity);
/** Set the circle radius and synchronize standalone particle polygon geometry. */
EngineResult rohr_physics_particle_radius_set(Entity entity, float radius);
/** Get a particle circle's effective radius. */
ParticleRadiusResult rohr_physics_particle_radius_get(Entity entity);

/** Return whether two entities overlap during the current physics step. */
bool rohr_physics_overlap_check(Entity entity, Entity target);
/** Return current overlap geometry in the requested entity order. */
OverlapInfo rohr_physics_overlap_get(Entity entity, Entity target);
/** Return whether an overlap began during the current physics step. */
bool rohr_physics_overlap_entered_check(Entity entity, Entity target);
/** Return whether an overlap continued from the previous physics step. */
bool rohr_physics_overlap_stayed_check(Entity entity, Entity target);
/** Return whether an overlap ended during the current physics step. */
bool rohr_physics_overlap_exited_check(Entity entity, Entity target);
/** Return the number of current overlaps involving an entity. */
size_t rohr_physics_overlap_count_get(Entity entity);
/** Write up to capacity current overlaps and return the number written. */
size_t rohr_physics_overlaps_get(
    Entity entity,
    EntityInteraction *results,
    size_t capacity
);

/** Return whether two entities physically contacted during the current physics step. */
bool rohr_physics_contact_check(Entity entity, Entity target);
/** Return current contact geometry in the requested entity order. */
ContactInfo rohr_physics_contact_get(Entity entity, Entity target);
/** Return the sum of a contact's normal and friction impulses. */
Vec2D rohr_physics_contact_total_impulse_get(ContactInfo contact);
/** Return whether a physical contact began during the current physics step. */
bool rohr_physics_contact_entered_check(Entity entity, Entity target);
/** Return whether a physical contact continued from the previous physics step. */
bool rohr_physics_contact_stayed_check(Entity entity, Entity target);
/** Return whether a physical contact ended during the current physics step. */
bool rohr_physics_contact_exited_check(Entity entity, Entity target);
/** Return the number of current physical contacts involving an entity. */
size_t rohr_physics_contact_count_get(Entity entity);
/** Write up to capacity current contacts and return the number written. */
size_t rohr_physics_contacts_get(
    Entity entity,
    EntityContact *results,
    size_t capacity
);

/**
 * @brief Creates an engine color from a hexadecimal RRGGBBAA value.
 * @param hex_color_code RRGGBBAA hex color value.
 * @return Color created from hex_color_code.
 */
Color rohr_graphics_color_hex_create(uint32_t hex_color_code);

/**
 * @brief Starts the graphics system.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_graphics_start(void);

/**
 * @brief Stops the graphics service.
 */
void rohr_graphics_stop(void);

/**
 * @brief Polls graphics/window events.
 * @param event Destination for the SDL event.
 * @return true when an event was read, false otherwise.
 */
bool rohr_graphics_events_poll(SDL_Event *event);

/**
 * @brief Draws the frame background.
 * @param color Background color.
 */
void rohr_graphics_background_draw(Color color);

GraphicsLayerIdResult rohr_graphics_layer_create(LayerDescription description);
EngineResult rohr_graphics_layer_destroy(GraphicsLayerId layer);
EngineResult rohr_graphics_layer_set(GraphicsLayerId layer, int value);
GraphicsLayerValueResult rohr_graphics_layer_get(GraphicsLayerId layer);
EngineResult rohr_graphics_layer_name_set(const char *name, int value);
GraphicsLayerValueResult rohr_graphics_layer_name_get(const char *name);
GraphicsLayerIdResult rohr_graphics_layer_name_id_get(const char *name);
EngineResult rohr_graphics_layer_entity_set(Entity entity, int value);
EngineResult rohr_graphics_layer_entity_id_set(Entity entity, GraphicsLayerId layer);
EngineResult rohr_graphics_layer_entity_name_set(Entity entity, const char *name);
GraphicsLayerValueResult rohr_graphics_layer_entity_get(Entity entity);
GraphicsLayerIdResult rohr_graphics_layer_entity_id_get(Entity entity);
EngineResult rohr_graphics_layer_entity_clear(Entity entity);
EngineResult rohr_graphics_layer_sprite_set(Entity entity, int value);
EngineResult rohr_graphics_layer_sprite_id_set(Entity entity, GraphicsLayerId layer);
EngineResult rohr_graphics_layer_sprite_name_set(Entity entity, const char *name);
GraphicsLayerValueResult rohr_graphics_layer_sprite_get(Entity entity);
GraphicsLayerIdResult rohr_graphics_layer_sprite_id_get(Entity entity);
EngineResult rohr_graphics_layer_sprite_clear(Entity entity);
EngineResult rohr_graphics_layer_animation_set(Entity entity, int value);
EngineResult rohr_graphics_layer_animation_id_set(Entity entity, GraphicsLayerId layer);
EngineResult rohr_graphics_layer_animation_name_set(Entity entity, const char *name);
GraphicsLayerValueResult rohr_graphics_layer_animation_get(Entity entity);
GraphicsLayerIdResult rohr_graphics_layer_animation_id_get(Entity entity);
EngineResult rohr_graphics_layer_animation_clear(Entity entity);
EngineResult rohr_graphics_layer_ui_set(ViewportItemId item, int value);
EngineResult rohr_graphics_layer_ui_id_set(ViewportItemId item, GraphicsLayerId layer);
EngineResult rohr_graphics_layer_ui_name_set(ViewportItemId item, const char *name);
GraphicsLayerValueResult rohr_graphics_layer_ui_get(ViewportItemId item);
GraphicsLayerIdResult rohr_graphics_layer_ui_id_get(ViewportItemId item);
EngineResult rohr_graphics_layer_ui_clear(ViewportItemId item);
void rohr_graphics_layer_active_set(int value);
int rohr_graphics_layer_active_get(void);
EngineResult rohr_graphics_layer_active_id_set(GraphicsLayerId layer);
EngineResult rohr_graphics_layer_active_name_set(const char *name);

/**
 * @brief Draws a filled rectangle in logical screen coordinates.
 * @return true when the draw command was queued.
 */
bool rohr_graphics_screen_rect_draw(float x, float y, float width, float height, Color color);
bool rohr_graphics_screen_shape_filled_draw(Shape shape, Color color);
/** Returns the renderer output size in physical pixels. */
Scale rohr_graphics_render_output_size_get(void);
/** Changes the logical screen size while preserving aspect-correct presentation. */
bool rohr_graphics_logical_size_set(int width, int height);
/** Changes the logical aspect ratio while preserving the logical height. */
bool rohr_graphics_aspect_ratio_set(int width, int height);
/** Matches the logical aspect ratio to the render output when enabled. */
bool rohr_graphics_aspect_ratio_auto_set(bool enabled);
WindowPresentationDescription rohr_graphics_window_presentation_description_default_get(void);
/** Returns the active window mode and actual physical/logical dimensions. */
WindowPresentationDescription rohr_graphics_window_presentation_description_get(void);
EngineResult rohr_graphics_window_presentation_description_set(
    WindowPresentationDescription config);
/** Clips subsequent screen-space drawing until the clip is cleared. */
bool rohr_graphics_screen_clip_set(float x, float y, float width, float height);
/** Clears the active screen-space drawing clip. */
void rohr_graphics_screen_clip_clear(void);
/** Pushes a nested screen-space clip intersected with the active clip. */
bool rohr_graphics_screen_clip_push(float x, float y, float width, float height);
/** Restores the previous screen-space clip. */
void rohr_graphics_screen_clip_pop(void);

/** @brief Draws a centered rotated rectangle in logical screen space. */
bool rohr_graphics_screen_quad_draw(Position center, float width, float height, float angle, Color color);
/** Clockwise-positive radians alternative; other parameters retain their units. */
bool rohr_graphics_screen_quad_radians_draw(Position center, float width, float height, float angle, Color color);

/** Return a composed frame report. Recording failure still permits presentation.
 * Standalone order: cameras, commands, viewports, recording, presentation.
 * Frame pipelines must not be called recursively from camera callbacks.
 */
Report rohr_graphics_show(void);
/** Update camera motion and queue camera callback drawing. */
Report rohr_graphics_pipeline_cameras_update(void);
/** Consume pending submission diagnostics and execute queued commands. */
Report rohr_graphics_pipeline_commands_execute(void);
/** Compose screens and viewport UI after command execution. */
Report rohr_graphics_pipeline_viewports_draw(void);
/** Capture the composed frame; inactive recording returns NOT_RUN. */
Report rohr_graphics_pipeline_recording_update(void);
/** Apply frame limiting and present the composed frame. Software limiting spaces
 * presentation starts, including prior presentation time in the frame budget.
 * Late frames present immediately without accumulating catch-up work.
 */
Report rohr_graphics_pipeline_present(void);
/** VSync owns pacing when enabled; a failed change preserves the current mode. */
EngineResult rohr_graphics_vsync_set(bool enabled);
/** Software limit used with VSync off. Zero selects the 120 FPS fallback;
 * negative values are rejected. Changing the limit resets the pacing interval.
 */
EngineResult rohr_graphics_frame_limit_set(int frames_per_second);

/**
 * @brief Draws one entity hitbox.
 * @param entity Entity whose hitbox should be drawn.
 * @param fill_type Fill mode for drawing.
 */
void rohr_graphics_hit_box_draw(Entity entity, Fill fill_type);

/**
 * @brief Draws one entity hitbox with a caller supplied color.
 * @param entity Entity whose hitbox should be drawn.
 * @param fill_type Fill mode for drawing.
 * @param color Color to draw with.
 */
void rohr_graphics_hit_box_colored_draw(Entity entity, Fill fill_type, Color color);

/**
 * @brief Draws hitboxes for all renderable hitbox entities.
 */
void rohr_graphics_hit_boxes_draw(void);

/**
 * @brief Draws one joint using an engineering-style debug symbol.
 * @param joint Joint entity to draw.
 * @param color Symbol color.
 * @return true when the symbol's draw commands were queued.
 */
bool rohr_graphics_joint_draw(Entity joint, Color color);

/**
 * @brief Draws all live joints using engineering-style debug symbols.
 * @param color Symbol color.
 */
void rohr_graphics_joints_draw(Color color);

/**
 * @brief Draws a soft body's current surfaces, beams, and collision nodes.
 * @param soft_body Soft-body owner entity.
 * @param surface Fallback color for areas without a color override.
 * @param beam Beam color.
 * @param node Collision-node color.
 * @return true when the soft body's draw commands were queued.
 */
bool rohr_graphics_soft_body_draw(Entity soft_body, Color surface, Color beam, Color node);
/** Overrides an area's draw color; clear restores the draw call's surface color. */
EngineResult rohr_graphics_soft_body_area_color_set(Entity area, Color color);
/** Restores the draw call's surface color for this area. */
EngineResult rohr_graphics_soft_body_area_color_clear(Entity area);
/** Hides/shows the visual area without affecting nodes, beams or collision. */
EngineResult rohr_graphics_soft_body_area_visibility_set(Entity area, bool visible);

/** Sets a drawing-color override for one node belonging to a soft body. */
EngineResult rohr_graphics_soft_body_node_color_set(
    Entity soft_body, Entity node, Color color);
/** Sets a drawing-color override for the beam connecting two soft-body nodes. */
EngineResult rohr_graphics_soft_body_beam_color_set(
    Entity soft_body, Entity node_a, Entity node_b, Color color);
/**
 * @brief Loads or shares a texture and returns one caller-owned reference.
 * @param text_desc Texture description containing load settings.
 * @return TextureAssetResult containing the asset, or an error.
 */
TextureAssetResult rohr_graphics_texture_load(TextureDescription text_desc);
/** Zero defaults; a file is required to load. */
TextureDescription rohr_graphics_texture_description_default_get(void);
/** Inspect without retaining. Paths remain borrowed until this resource
 * loses its final owning reference or graphics shuts down.
 * Logical size comes from the supplied asset value. */
TextureDescriptionResult rohr_graphics_texture_description_get(TextureAsset asset);

/** @brief Adds one owning reference to a loaded texture asset. */
EngineResult rohr_graphics_texture_retain(TextureAsset asset);

/**
 * @brief Releases one owning reference and clears the asset value.
 * Copying a TextureAsset does not retain it; retain independently owned copies.
 */
EngineResult rohr_graphics_texture_release(TextureAsset *asset);

/** @brief Returns whether a texture asset names a live resource. */
bool rohr_graphics_texture_valid_check(TextureAsset asset);

/** Draws a loaded texture centered at a world position. */
void rohr_graphics_texture_draw(TextureAsset texture, Position position,
    Orientation orientation);
/** Clockwise-positive radians alternative; other parameters retain their units. */
void rohr_graphics_texture_radians_draw(TextureAsset texture, Position position,
    Orientation orientation);
void rohr_graphics_screen_texture_draw(TextureAsset texture, Position center,
    Scale size, Orientation orientation);
/** Clockwise-positive radians alternative; other parameters retain their units. */
void rohr_graphics_screen_texture_radians_draw(TextureAsset texture, Position center,
    Scale size, Orientation orientation);

/** @brief Loads or shares a font and returns one owning reference. */
FontAssetResult rohr_graphics_font_load(FontDescription descriptor);
/** Zero defaults select the engine-pinned built-in font. */
FontDescription rohr_graphics_font_description_default_get(void);
/** Inspect without retaining. Paths remain borrowed until this resource
 * loses its final owning reference or graphics shuts down.
 * Built-in inspection returns file NULL and point_size zero. */
FontDescriptionResult rohr_graphics_font_description_get(FontAsset asset);

/** @brief Returns the engine-pinned built-in font for this graphics lifetime. */
FontAsset rohr_graphics_font_default_get(void);

/** @brief Adds one custom-font owner; built-in retains are no-ops. */
EngineResult rohr_graphics_font_retain(FontAsset font);

/** @brief Releases one font owner and clears the asset value. */
EngineResult rohr_graphics_font_release(FontAsset *font);

/** @brief Returns whether a font asset names a live resource. */
bool rohr_graphics_font_valid_check(FontAsset font);

/** @brief Creates reusable caller-owned text. */
TextAssetResult rohr_graphics_text_create(TextDescription description);
/** Built-in font, empty string, opaque white, and no wrapping. */
TextDescription rohr_graphics_text_description_default_get(void);
/** Atomic full replacement; updates dimensions without changing the handle.
 * Failure preserves configuration, dependencies, and queued payloads. */
EngineResult rohr_graphics_text_description_set(TextAsset *text, TextDescription description);
/** Borrowed font and string; no retain. Valid until successful changed
 * configuration, public text destruction, or graphics shutdown. */
TextDescriptionResult rohr_graphics_text_description_get(TextAsset text);
bool rohr_graphics_text_value_set(TextAsset *text, const char *value);
/** @brief Sets text wrapping width; zero disables wrapping. */
bool rohr_graphics_text_wrap_width_set(TextAsset *text, int wrap_width);

/** @brief Destroys the unique text owner and clears its value. */
EngineResult rohr_graphics_text_destroy(TextAsset *text);

/** @brief Returns whether a text asset names a live public owner. */
bool rohr_graphics_text_valid_check(TextAsset text);

/** @brief Draws text in logical screen coordinates. */
bool rohr_graphics_text_draw(const TextAsset *text, Position position);
bool rohr_graphics_text_scaled_draw(const TextAsset *text, Position position,
    Scale scale);
bool rohr_graphics_screen_text_scaled_rotated_draw(const TextAsset *text,
    Position center, Scale scale, Orientation orientation);
/** Clockwise-positive radians alternative; other parameters retain their units. */
bool rohr_graphics_screen_text_scaled_rotated_radians_draw(const TextAsset *text,
    Position center, Scale scale, Orientation orientation);

/**
 * @brief Loads or shares an immutable animation resource.
 * The descriptor supplies image paths, stable IDs, and default timing.
 * Frame transforms are copied into each instance and edited after creation.
 * @param anim_desc Animation descriptor containing load settings.
 * @return AnimationAssetResult containing the asset, or an error.
 */
AnimationAssetResult rohr_graphics_animation_load(AnimationDescription anim_desc);
/** Zero defaults create an empty catalog; zero IDs normalize on load. */
AnimationDescription rohr_graphics_animation_description_default_get(void);
/** Inspect without retaining. Paths remain borrowed until this resource
 * loses its final owning reference or graphics shuts down.
 * Frame arrays are copied; paths are borrowed from retained frame textures. */
AnimationDescriptionResult rohr_graphics_animation_description_get(AnimationAsset asset);

/** Native pixel dimensions, independent of TextureAsset.size. */
TextureSizeResult rohr_graphics_texture_size_get(TextureAsset asset);
/** Scale one, facing right, visible, and following entity rotation. */
SpriteDescription rohr_graphics_sprite_description_default_get(void);
/** Replace all configuration atomically; the component must already exist. */
EngineResult rohr_graphics_sprite_description_set(Entity entity, SpriteDescription description);
/** Copied configuration with borrowed texture; no retain. */
SpriteDescriptionResult rohr_graphics_sprite_description_get(Entity entity);
/** Scale one with zero ID, offset, and rotation. */
AnimationFrameDescription rohr_graphics_animation_frame_description_default_get(void);
/** No catalog and zero timing. */
AnimationPlayerDescription rohr_graphics_animation_player_description_default_get(void);
/** Borrowed catalog and its timing; zero handle selects empty defaults. */
AnimationPlayerDescriptionResult rohr_graphics_animation_player_description_from_animation_get(AnimationAsset animation);
/** Preserves clocks for the same catalog; a changed catalog resets to frame zero
 * at the last observed update clocks. No resource retain or release. */
EngineResult rohr_graphics_animation_player_description_set(AnimationPlayer *player, AnimationPlayerDescription description);
/** Configuration only; the caller keeps the borrowed catalog alive. */
AnimationPlayerDescriptionResult rohr_graphics_animation_player_description_get(const AnimationPlayer *player);
/** Empty frames, scale one, facing right, visible, following entity rotation. */
AnimatedSpriteDescription rohr_graphics_animated_sprite_description_default_get(void);
/** Copies catalog frames and timing into independent instance configuration.
 * Images and catalog are borrowed until catalog release or graphics shutdown. */
AnimatedSpriteDescriptionResult rohr_graphics_animated_sprite_description_from_animation_get(AnimationAsset animation);
/** Copied value configuration. Images/catalog remain borrowed from the value's owners. */
AnimatedSpriteDescription rohr_graphics_animated_sprite_value_description_get(AnimatedSprite sprite);
/** For standalone unowned values only; validates atomically without retaining.
 * Preserve current frame by ID and clocks. Removed current ID or changed catalog
 * resets to frame zero at the last observed update clocks. */
EngineResult rohr_graphics_animated_sprite_value_description_set(AnimatedSprite *sprite, AnimatedSpriteDescription description);
/** Atomic component replacement, acquiring new references before releasing old.
 * Same preservation rules as value replacement; resets use engine clocks. */
EngineResult rohr_graphics_animated_sprite_description_set(Entity entity, AnimatedSpriteDescription description);
/** Copied frames and borrowed handles; valid until replacement/removal/deletion
 * or graphics shutdown. The getter never retains. */
AnimatedSpriteDescriptionResult rohr_graphics_animated_sprite_description_get(Entity entity);


/** Non-owning value builder; copies fields and resolves native texture size.
 * It does not validate or retain; add/set validates before publication. */
AnimationFrameDescription rohr_graphics_animation_frame_create(AnimationFrameDescription description);
/** Non-owning builders: copy values; keep image owners alive until component add.
 * Use only on unattached values, never directly on an installed component.
 * ID zero assigns an unused ID on add and preserves the current ID on set.
 * Replacing the current ID resets playback to the first frame. */
EngineResult rohr_graphics_animated_sprite_value_frame_add(AnimatedSprite *sprite, AnimationFrameDescription frame);
EngineResult rohr_graphics_animated_sprite_value_frame_description_set(AnimatedSprite *sprite, size_t index, AnimationFrameDescription frame);
/** Get a copied instance frame with a borrowed image. Do not release its texture. */
AnimationFrameDescriptionResult rohr_graphics_animated_sprite_frame_description_get(Entity entity, size_t index);
/** Instance mutation retains images and rolls back on error. Playback is
 * preserved unless the current frame ID changes, which resets to frame zero.
 * Signed scale flips content; zero collapses the image. All transforms must be finite. */
EngineResult rohr_graphics_animated_sprite_frame_description_set(Entity entity, size_t index, AnimationFrameDescription frame);
EngineResult rohr_graphics_animated_sprite_frame_add(Entity entity, AnimationFrameDescription frame);
EngineResult rohr_graphics_animated_sprite_frame_scale_set(Entity entity, size_t index, Scale scale);
EngineResult rohr_graphics_animated_sprite_frame_offset_set(Entity entity, size_t index, Position offset);
EngineResult rohr_graphics_animated_sprite_frame_rotation_set(Entity entity, size_t index, Orientation rotation);
EngineResult rohr_graphics_animated_sprite_frame_rotation_radians_set(Entity entity, size_t index, Orientation rotation);
TextureSizeResult rohr_graphics_animated_sprite_frame_scale_get(Entity entity, size_t index);
PositionResult rohr_graphics_animated_sprite_frame_offset_get(Entity entity, size_t index);
SpriteOrientationResult rohr_graphics_animated_sprite_frame_rotation_get(Entity entity, size_t index);
SpriteOrientationResult rohr_graphics_animated_sprite_frame_rotation_radians_get(Entity entity, size_t index);
/** @brief Adds one owning reference to a loaded animation asset. */
EngineResult rohr_graphics_animation_retain(AnimationAsset asset);

/**
 * @brief Releases one owning reference and clears the animation asset.
 * Copying an AnimationAsset does not retain it; retain independently owned
 * copies explicitly.
 */
EngineResult rohr_graphics_animation_release(AnimationAsset *asset);

/** @brief Returns whether an animation asset names a live resource. */
bool rohr_graphics_animation_valid_check(AnimationAsset asset);

/** @brief Returns immutable metadata for a loaded animation. */
AnimationInfoResult rohr_graphics_animation_info_get(AnimationAsset asset);

/** @brief Returns a reusable catalog frame with scale one and a borrowed texture. */
AnimationFrameDescriptionResult rohr_graphics_animation_frame_description_get(AnimationAsset asset,
    size_t frame_index);

/**
 * @brief Creates independent mutable playback state for an animation.
 * The player borrows the animation; its owner must keep the asset alive.
 */
AnimationPlayerResult rohr_graphics_animation_player_create(AnimationPlayerDescription description);

/** @brief Advances one animation player's frame state. */
void rohr_graphics_animation_player_update(AnimationPlayer *player,
    Tick current_tick, Time current_time);

/**
 * @brief Validates a description and creates a non-owning runtime value.
 * @param description Complete configuration; frames are explicit.
 * @return AnimatedSpriteResult; the caller keeps all borrowed resources alive.
 */
AnimatedSpriteResult rohr_graphics_animated_sprite_create(AnimatedSpriteDescription description);

/** Advance one animated sprite's frame state without attaching it to an entity. */
void rohr_graphics_animated_sprite_update(AnimatedSprite *sprite, Tick current_tick,
    Time current_time);

/**
 * @brief Adds an animated sprite to an entity.
 * @param entity Entity to modify.
 * @param description Complete configuration; compatible playback is preserved.
 * @return EngineResult describing success or failure.
 */
EngineResult rohr_graphics_animated_sprite_add(Entity entity, AnimatedSpriteDescription description);
EngineResult rohr_graphics_animated_sprite_frame_index_set(Entity entity,
    size_t frame_index);
AnimationFrameIndexResult rohr_graphics_animated_sprite_frame_index_get(
    Entity entity);
Sprite rohr_graphics_sprite_create(SpriteDescription description);
EngineResult rohr_graphics_sprite_add(Entity entity, SpriteDescription description);
EngineResult rohr_graphics_sprite_body_offset_set(Entity entity, Position offset);
PositionResult rohr_graphics_sprite_body_offset_get(Entity entity);
EngineResult rohr_graphics_sprite_orientation_offset_set(Entity entity,
    Orientation offset);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult rohr_graphics_sprite_orientation_offset_radians_set(Entity entity,
    Orientation offset);
SpriteOrientationResult rohr_graphics_sprite_orientation_offset_get(Entity entity);
/** Clockwise-positive radians alternative; other parameters retain their units. */
SpriteOrientationResult rohr_graphics_sprite_orientation_offset_radians_get(Entity entity);
EngineResult rohr_graphics_animated_sprite_body_offset_set(Entity entity,
    Position offset);
PositionResult rohr_graphics_animated_sprite_body_offset_get(Entity entity);
EngineResult rohr_graphics_animated_sprite_orientation_offset_set(Entity entity,
    Orientation offset);
/** Clockwise-positive radians alternative; other parameters retain their units. */
EngineResult rohr_graphics_animated_sprite_orientation_offset_radians_set(Entity entity,
    Orientation offset);
SpriteOrientationResult rohr_graphics_animated_sprite_orientation_offset_get(
    Entity entity);
/** Clockwise-positive radians alternative; other parameters retain their units. */
SpriteOrientationResult rohr_graphics_animated_sprite_orientation_offset_radians_get(
    Entity entity);
bool rohr_graphics_sprite_draw(Entity entity);
void rohr_graphics_sprites_draw(void);

/** Draw the animated sprite attached to one entity. */
bool rohr_graphics_animated_sprite_draw(Entity entity);

/**
 * @brief Draws all animated sprite components.
 */
void rohr_graphics_animated_sprites_draw(void);

/**
 * @brief Updates animated sprite frames.
 * @param current_tick Current engine tick.
 * @param current_time Current engine time.
 */
Report rohr_graphics_sprite_frames_update(Tick current_tick, Time current_time);

/**
 * @brief Scales textures attached to an entity.
 * @param entity Entity to modify.
 * @param scale Scale value.
 */
void rohr_graphics_textures_scale(Entity entity, Scale scale);

/**
 * @brief Translates the active camera in world space.
 * @param translation World-space translation to add.
 */
void rohr_graphics_camera_move(Vec2D translation);

/**
 * @brief Rotates the active camera clockwise.
 * @param degrees Clockwise rotation in degrees to add.
 */
void rohr_graphics_camera_rotate(Orientation degrees);
/** Clockwise-positive radians alternative; other parameters retain their units. */
void rohr_graphics_camera_radians_rotate(Orientation radians);

/** Attach the active camera using clockwise degree offsets. */
EngineResult rohr_graphics_camera_attach(CameraAttachmentDescription description);
/** Explicitly detach the active camera, preserving its resolved view. */
void rohr_graphics_camera_detach(void);
/** Whether the active camera can currently resolve its retained attachment. */
bool rohr_graphics_camera_attached_get(void);
/** Enabled, full-window dimensions, unit zoom, rendering allowed while paused. */
CameraDescription rohr_camera_description_default_get(void);
/** Create a camera from complete configuration; zero dimensions/zoom use defaults. */
CameraIdResult rohr_camera_create(CameraDescription description);
/** Destroy a non-active camera. */
EngineResult rohr_camera_destroy(CameraId camera);
EngineResult rohr_camera_active_set(CameraId camera);
CameraId rohr_camera_active_get(void);
/** Current resolved world view. Missing targets retain the last resolved view. */
CameraResult rohr_camera_get(CameraId camera);
/** Current configuration, including resolved world transform and borrowed callback context. */
CameraDescriptionResult rohr_camera_description_get(CameraId camera);
/** Atomic replacement; world edits adjust offsets without detaching.
 * Unchanged fields preserve progress; position/zoom edits replace only that motion. */
EngineResult rohr_camera_description_set(CameraId camera, CameraDescription description);
/** Follow both position and orientation; fill the required entity handle. */
CameraAttachmentDescription rohr_camera_attachment_description_default_get(void);
/** Replace an attachment atomically; unchanged configuration preserves motion. */
EngineResult rohr_camera_attachment_description_set(CameraId camera,
    CameraAttachmentDescription description);
/** Copy the retained attachment, including an unavailable target reference. */
CameraAttachmentDescriptionResult rohr_camera_attachment_description_get(CameraId camera);
/** Explicitly detach, preserving the last resolved world view. */
EngineResult rohr_camera_detach(CameraId camera);

EngineResult rohr_camera_render_callback_set(
    CameraId camera,
    CameraRenderCallback callback,
    void *context
);
EngineResult rohr_camera_enable_set(CameraId camera);
EngineResult rohr_camera_disable_set(CameraId camera);
EngineResult rohr_camera_pause_with_engine_set(CameraId camera);
EngineResult rohr_camera_render_when_paused_set(CameraId camera);
EngineResult rohr_camera_position_move(CameraId camera, Vec2D translation, Time duration);
EngineResult rohr_camera_position_set(CameraId camera, Position position, Time duration);
EngineResult rohr_camera_position_from_entity_set(CameraId camera, Entity entity, Time duration);
EngineResult rohr_camera_entity_attachment_set(CameraId camera, Entity entity);
EngineResult rohr_camera_moving_get(CameraId camera);
EngineResult rohr_camera_zoom_set(CameraId camera, float zoom, Time duration);
CameraZoomResult rohr_camera_zoom_get(CameraId camera);

LayerDescription rohr_graphics_layer_description_default_get(void);
EngineResult rohr_graphics_layer_description_set(GraphicsLayerId layer, LayerDescription description);
LayerDescriptionResult rohr_graphics_layer_description_get(GraphicsLayerId layer);
EngineResult rohr_screen_description_set(ScreenId screen, ScreenDescription description);
ScreenDescriptionResult rohr_screen_description_get(ScreenId screen);
EngineResult rohr_viewport_description_set(ViewportId viewport, ViewportDescription description);
ViewportDescriptionResult rohr_viewport_description_get(ViewportId viewport);
/** Mount exactly one existing screen or UI reference without transferring ownership.
 * Full item replacement preserves the handle, acquiring new dependencies before release.
 * Content replacement, hiding, or disabling cancels incompatible pointer interaction. */
ViewportItemIdResult rohr_viewport_item_add(ViewportId viewport, ViewportItemDescription description);
ViewportUiShapeDescription rohr_graphics_ui_shape_description_default_get(void);
ViewportUiTextDescription rohr_graphics_ui_text_description_default_get(void);
ViewportUiSliderDescription rohr_graphics_ui_slider_description_default_get(void);
ViewportDescription rohr_viewport_description_default_get(void);
ViewportItemDescription rohr_viewport_item_description_default_get(void);
/** Defaults use the active camera and window dimensions. Zero dimensions normalize;
 * negative dimensions fail without mutation. Camera IDs are borrowed: missing
 * cameras show a construction placeholder and warn during viewport composition.
 * Destroying a camera preserves screens; reassign their camera explicitly. */
ScreenDescription rohr_screen_description_default_get(void);
ScreenIdResult rohr_screen_create(ScreenDescription config);
EngineResult rohr_screen_destroy(ScreenId screen);
EngineResult rohr_screen_camera_set(ScreenId screen, CameraId camera);
ViewportIdResult rohr_viewport_create(ViewportDescription config);
ViewportIdResult rohr_viewport_by_name_get(const char *name);
ViewportNameResult rohr_viewport_name_get(ViewportId viewport);
EngineResult rohr_viewport_destroy(ViewportId viewport);
ViewportItemIdResult rohr_viewport_camera_add(ViewportId viewport,
    CameraId camera, ViewportItemDescription config);
ViewportItemIdResult rohr_viewport_screen_add(ViewportId viewport,
    ScreenId screen, ViewportItemDescription config);
GraphicsUiIdResult rohr_graphics_ui_shape_create(ViewportUiShapeDescription shape);
GraphicsUiIdResult rohr_graphics_ui_text_create(ViewportUiTextDescription text);
GraphicsUiIdResult rohr_graphics_ui_slider_create(ViewportUiSliderDescription slider);
EngineResult rohr_graphics_ui_shape_description_set(GraphicsUiId ui, ViewportUiShapeDescription shape);
ViewportUiShapeDescriptionResult rohr_graphics_ui_shape_description_get(GraphicsUiId ui);
EngineResult rohr_graphics_ui_text_description_set(GraphicsUiId ui, ViewportUiTextDescription text);
ViewportUiTextDescriptionResult rohr_graphics_ui_text_description_get(GraphicsUiId ui);
EngineResult rohr_graphics_ui_slider_description_set(GraphicsUiId ui,
    ViewportUiSliderDescription slider);
ViewportUiSliderDescriptionResult rohr_graphics_ui_slider_description_get(GraphicsUiId ui);
EngineResult rohr_graphics_ui_slider_value_set(GraphicsUiId ui, float value);
GraphicsUiSliderValueResult rohr_graphics_ui_slider_value_get(GraphicsUiId ui);
EngineResult rohr_graphics_ui_destroy(GraphicsUiId ui);
ViewportItemIdResult rohr_viewport_ui_add(ViewportId viewport,
    GraphicsUiId ui, ViewportItemDescription config);
ViewportItemIdResult rohr_viewport_ui_shape_add(ViewportId viewport,
    ViewportUiShapeDescription shape, ViewportItemDescription config);
ViewportItemIdResult rohr_viewport_ui_text_add(ViewportId viewport,
    ViewportUiTextDescription text, ViewportItemDescription config);
ViewportItemIdResult rohr_viewport_ui_slider_add(ViewportId viewport,
    ViewportUiSliderDescription slider, ViewportItemDescription config);
EngineResult rohr_viewport_item_remove(ViewportItemId item);
EngineResult rohr_viewport_item_description_set(ViewportItemId item, ViewportItemDescription config);
ViewportItemDescriptionResult rohr_viewport_item_description_get(ViewportItemId item);
ViewportIdResult rohr_viewport_item_viewport_get(ViewportItemId item);
bool rohr_viewport_ui_hovered_check(ViewportItemId item);
bool rohr_viewport_ui_pressed_check(ViewportItemId item);
bool rohr_viewport_ui_slider_changed_check(ViewportItemId item);
EngineResult rohr_viewport_item_drag_mode_set(ViewportItemId item,
    ViewportItemDragMode mode);
ViewportItemDragModeResult rohr_viewport_item_drag_mode_get(ViewportItemId item);
bool rohr_viewport_item_dragging_check(ViewportItemId item);
EngineResult rohr_viewport_camera_set(ViewportId viewport, CameraId camera);
EngineResult rohr_viewport_camera_clear(ViewportId viewport);
EngineResult rohr_viewport_enable_set(ViewportId viewport);
EngineResult rohr_viewport_disable_set(ViewportId viewport);

/**
 * @brief Converts a world position to screen coordinates.
 * @param pos World position.
 * @return Screen-space position.
 */
Position rohr_graphics_world_to_screen_get(Position pos);

/**
 * @brief Converts a screen position to world coordinates.
 * @param screen Screen-space position.
 * @return World-space position.
 */
Position rohr_graphics_screen_to_world_get(Position screen);

/**
 * @brief Returns the current mouse position in screen coordinates.
 * @return Mouse screen position.
 */
Position rohr_graphics_mouse_screen_position_get(void);

/** Enable or disable physics AABB-tree debug drawing. */
void rohr_graphics_aabb_tree_debug_set(bool enabled);
/** Return whether physics AABB-tree debug drawing is enabled. */
bool rohr_graphics_aabb_tree_debug_check(void);
/** Draw the current physics AABB-tree bounds when debug drawing is enabled. */
void rohr_graphics_aabb_tree_draw(void);
/** Enable or disable current-contact manifold debug drawing. */
void rohr_graphics_contacts_debug_set(bool enabled);
/** Return whether current-contact manifold debug drawing is enabled. */
bool rohr_graphics_contacts_debug_check(void);
/** Draw current contact manifold points and first-to-second normals when enabled. */
void rohr_graphics_contacts_draw(void);

/**
 * @brief Starts recording rendered frames to a video file.
 * @param output_path Path where the recording should be written.
 * @param fps Recording frame rate.
 * @return true when recording starts successfully, false otherwise.
 */
bool rohr_graphics_recording_start(const char *output_path, int fps);

/**
 * @brief Stops recording rendered frames.
 */
void rohr_graphics_recording_stop(void);

/**
 * @brief Draws one particle entity using its collision circle.
 * @param entity Particle entity to draw.
 * @param fill_type Whether to draw the circle filled or outlined.
 */
void rohr_graphics_particle_draw(Entity entity, Fill fill_type);

/**
 * @brief Draws active particle components.
 */
void rohr_graphics_particles_draw(void);

/**
 * @brief Draws local origin markers for entities.
 */
void rohr_graphics_local_origins_draw(void);

/**
 * @brief Creates normalized edge normals for a shape.
 * @param shape Shape to inspect.
 * @return List of normal vectors.
 */
Vec2DList rohr_math_normals_create(Shape shape);

/**
 * @brief Normalizes a vector.
 * @param vector Vector to normalize.
 * @return Normalized vector.
 */
Vec2D rohr_math_vector_normalize(Vec2D vector);

/**
 * @brief Normalizes all vectors in a list.
 * @param vectors Vector list to normalize.
 * @return Normalized vector list.
 */
Vec2DList rohr_math_vectors_normalize(Vec2DList vectors);

/**
 * @brief Calculates the dot product of two vectors.
 * @param vector_1 First vector.
 * @param vector_2 Second vector.
 * @return Dot product.
 */
float rohr_math_dot_product(Vec2D vector_1, Vec2D vector_2);

/**
 * @brief Creates a rectangular polygon shape.
 * @param width Rectangle width.
 * @param height Rectangle height.
 * @return Shape containing rectangle vertices.
 */
Shape rohr_math_square_create(float width, float height);

/**
 * @brief Creates a circle approximation shape.
 * @param radius Circle radius.
 * @param verticies Number of vertices used to approximate the circle.
 * @return Shape containing circle vertices.
 */
Shape rohr_math_circle_create(float radius, uint8_t verticies);

/**
 * @brief Projects a shape onto an axis.
 * @param shape Shape to project.
 * @param axis Axis to project onto.
 * @return Projection interval.
 */
Projection rohr_math_project_shape_on_axis(Shape shape, Axis axis);

/**
 * @brief Calculates the scalar 2D cross product.
 * @param a First vector.
 * @param b Second vector.
 * @return Cross product value.
 */
float rohr_math_cross_2d(Vec2D a, Vec2D b);

/**
 * @brief Calculates angular velocity crossed with a vector.
 * @param omega Angular velocity.
 * @param r Radius or offset vector.
 * @return Tangential velocity vector.
 */
Vec2D rohr_math_angular_velocity_cross_vec(float omega, Vec2D r);
/** Clockwise-positive radians alternative; other parameters retain their units. */
Vec2D rohr_math_angular_velocity_radians_cross_vec(float omega, Vec2D r);
/** Convert degrees to radians without sign changes or wrapping. */
float rohr_math_degrees_to_radians(float degrees);
/** Convert radians to degrees without sign changes or wrapping. */
float rohr_math_radians_to_degrees(float radians);

/**
 * @brief Projects a vector onto an axis.
 * @param v Vector to project.
 * @param axis Axis to project onto.
 * @return Projected vector.
 */
Vec2D rohr_math_project_onto_axis(Vec2D v, Axis axis);

/**
 * @brief Calculates axis magnitude.
 * @param axis Axis vector.
 * @return Magnitude.
 */
float rohr_math_axis_magnitude(Axis axis);

/**
 * @brief Calculates vector magnitude.
 * @param vector Vector to inspect.
 * @return Magnitude.
 */
float rohr_math_vector_magnitude(Vec2D vector);

/**
 * @brief Rotates a vector by an angle.
 * @param vector Vector to rotate.
 * @param angle Clockwise angle in degrees.
 * @return Rotated vector.
 */
Vec2D rohr_math_vector_rotate(Vec2D vector, float angle);
/** Clockwise-positive radians alternative; other parameters retain their units. */
Vec2D rohr_math_vector_radians_rotate(Vec2D vector, float angle);

/**
 * @brief Calculates a circle radius from its shape and centroid.
 * @param circle Circle shape.
 * @param centroid Circle centroid.
 * @return Circle radius.
 */
Vec1D rohr_math_circle_radius(Shape circle, Vec2D centroid);

/**
 * @brief Subtracts one vector from another.
 * @param vector_a Vector to subtract from.
 * @param vector_b Vector to subtract.
 * @return vector_a minus vector_b.
 */
Vec2D rohr_math_vector_subtract(Vec2D vector_a, Vec2D vector_b);

/**
 * @brief Calculates overlap depth between two circles.
 * @param centroid_1 First circle centroid.
 * @param radius_1 First circle radius.
 * @param centroid_2 Second circle centroid.
 * @param radius_2 Second circle radius.
 * @return Circle overlap depth.
 */
Vec1D rohr_math_circle_overlap_depth(Vec2D centroid_1, Vec1D radius_1, Vec2D centroid_2, Vec1D radius_2);

/**
 * @brief Calculates overlap between two projection intervals.
 * @param projection_1 First projection.
 * @param projection_2 Second projection.
 * @return Overlap depth.
 */
float rohr_math_projection_overlap(Projection projection_1, Projection projection_2);

/**
 * @brief Scales a shape uniformly.
 * @param shape Shape to scale.
 * @param scale Uniform scale value.
 * @return Scaled shape.
 */
Shape rohr_math_shape_scale(Shape shape, float scale);

/**
 * @brief Scales a shape along the y axis.
 * @param shape Shape to scale.
 * @param scale Y-axis scale value.
 * @return Scaled shape.
 */
Shape rohr_math_shape_y_scale(Shape shape, float scale);

/**
 * @brief Scales a shape along the x axis.
 * @param shape Shape to scale.
 * @param scale X-axis scale value.
 * @return Scaled shape.
 */
Shape rohr_math_shape_x_scale(Shape shape, float scale);

/**
 * @brief Calculates a polygon centroid.
 * @param shape Polygon shape.
 * @return Centroid position.
 */
Vec2D rohr_math_polygon_centroid(Shape shape);

/**
 * @brief Adds a vertex slot to a shape.
 * @param shape Shape to modify.
 * @return Shape with an additional vertex slot.
 */
Shape rohr_math_vertex_add(Shape shape);

/**
 * @brief Deletes the last vertex slot from a shape.
 * @param shape Shape to modify.
 * @return Shape with one fewer vertex slot.
 */
Shape rohr_math_vertex_delete(Shape shape);

/**
 * @brief Creates an axis-aligned bounding box for a world-space shape.
 * @param world_shape World-space shape.
 * @return Axis-aligned bounding box.
 */
AABB rohr_math_aabb_create(Shape world_shape);

/**
 * @brief Runs one physics-system update.
 * @param dt Simulation delta time in seconds.
 */
Report rohr_system_physics_update(double dt);

/**
 * @brief Delays execution for a number of seconds.
 * @param seconds Number of seconds to delay.
 */
void rohr_tools_delay(int seconds);

/**
 * @brief Writes a binary string representation of a value.
 * @param value Value to convert.
 * @param buffer Destination buffer.
 * @param size Size of buffer in bytes.
 */
void rohr_tools_binary_to_string(uint32_t value, char *buffer, size_t size);

/**
 * @brief Appends one string to another using explicit buffer sizes.
 * @param src Source string.
 * @param dst Destination string.
 * @param src_size Source buffer size.
 * @param dst_size Destination buffer size.
 */
void rohr_tools_append_string(char *src, char *dst, size_t src_size, size_t dst_size);

/**
 * @brief Counts characters in a string until a delimiter.
 * @param str String to inspect.
 * @param delimiter Delimiter that stops counting.
 * @return Number of characters before delimiter.
 */
uint32_t rohr_tools_sizeof_string(char *str, char delimiter);

/**
 * @brief Returns a random integer in a range.
 * @param min Minimum value.
 * @param max Maximum value.
 * @return Random integer between min and max.
 */
int rohr_tools_random_range(int min, int max);

/**
 * @brief Returns a random float in a range.
 * @param min Minimum value.
 * @param max Maximum value.
 * @return Random float between min and max.
 */
float rohr_tools_random_range_float(float min, float max);

/** @brief Replaces logical screen-space input without resetting widgets or events. */
void rohr_ui_input_set(UIInput input);
void rohr_ui_modal_set(UIRect bounds);
void rohr_ui_modal_controls_begin(void);
void rohr_ui_modal_controls_end(void);
/** @brief Applies reusable sizing components to UI bounds. */
UIRect rohr_ui_component_bounds_get(UIRect bounds, const TextAsset *const *texts,
    size_t text_count, UIComponentConfig config);
/** Queues keyboard and pointer events used by UI controls. */
void rohr_ui_event_add(const SDL_Event *event);
/** Queues a keyboard event for focused UI fields. */
void rohr_ui_field_event_add(const SDL_Event *event);
/** Clears the currently focused UI text or number field. */
void rohr_ui_field_focus_clear(void);
/** Draws and edits a caller-owned string or float field. */
UIFieldResult rohr_ui_field(const char *id, UIFieldBinding binding,
    TextAsset *display, UIRect bounds, const UIButtonStyle *style);
UIFieldResult rohr_ui_multiline_field(const char *id, UIFieldBinding binding,
    TextAsset *display, UIRect bounds, const UIButtonStyle *style);

/**
 * @brief Draws and updates one button identified by a stable string.
 * @param label Optional centered label; NULL or empty draws no label.
 */
UIButtonResult rohr_ui_button(
    const char *id,
    const TextAsset *label,
    UIRect bounds,
    const UIButtonStyle *style
);

/**
 * @brief Draws a dropdown and returns selection and hover-preview state.
 *
 * Set config.visible_row_limit to zero to display every option; limited lists scroll.
 * Set config.fit_window to fit the logical window by opening above or showing fewer scrollable rows.
 * Optional config.hint and config.pick add a heading and external selection; Escape cancels picking.
 */
UIDropdownResult rohr_ui_dropdown(const char *id, const TextAsset *const *options,
    size_t option_count, size_t selected_index, UIDropdownConfig config,
    UIRect bounds, const UIButtonStyle *style);
UIDropdownResult rohr_ui_dropdown_actions(const char *id,
    const TextAsset *const *options, size_t option_count, size_t selected_index,
    const TextAsset *action, size_t first_action_index, UIDropdownConfig config,
    UIRect bounds, const UIButtonStyle *style);
bool rohr_ui_key_pressed_check(SDL_Keycode key);
bool rohr_ui_primary_pressed_check(void);
/**
 * @brief Draws a menu button with every action visible and no scrollbar.
 *
 * The menu label is not repeated in its action list.
 */
UIDropdownResult rohr_ui_menu(const char *id, const TextAsset *label,
    const TextAsset *const *options, size_t option_count, UIRect bounds,
    const UIButtonStyle *style);
/** @brief Begins a clipped vertical scroll region for subsequent UI controls. */
UIScrollRegionResult rohr_ui_scroll_region_begin(const char *id, UIRect bounds,
    float content_height, float offset, float wheel_step);
/** @brief Ends the current UI scroll region. */
void rohr_ui_scroll_region_end(void);
/** @brief Adds a scoped vertical translation for subsequent UI controls. */
void rohr_ui_translation_y_push(float offset);
/** @brief Restores the vertical translation active before the last push. */
void rohr_ui_translation_y_pop(void);
/** @brief Updates pointer interaction without prescribing visuals. */
UIButtonResult rohr_ui_interaction(const char *id, UIRect bounds);
/** @brief Draws a filled rectangular UI primitive. */
void rohr_ui_surface(UIRect bounds, Color color);
/** @brief Draws a rectangular UI border primitive. */
void rohr_ui_border(UIRect bounds, float thickness, Color color);
/** @brief Draws centered clipped text content. */
void rohr_ui_content(const TextAsset *text, UIRect bounds);
/** @brief Draws an oriented UI rectangle primitive. */
void rohr_ui_quad(Position center, float width, float height, float angle, Color color);
/** Clockwise-positive radians alternative; other parameters retain their units. */
void rohr_ui_radians_quad(Position center, float width, float height, float angle, Color color);
/** @brief Begins and ends a clipped UI component region. */
bool rohr_ui_clip_begin(UIRect bounds);
void rohr_ui_clip_end(void);
/** @brief Moves UI keyboard focus in a screen-space direction. */
bool rohr_ui_navigation_move(UINavigationDirection direction);
/** @brief Activates the currently focused UI control. */
bool rohr_ui_navigation_activate(void);
/** @brief Returns the focused control's previous-frame screen bounds. */
bool rohr_ui_navigation_focus_bounds_get(UIRect *bounds);

/** @brief Draws reusable text centered inside bounds. */
void rohr_ui_label(const TextAsset *text, UIRect bounds);

/** @brief Draws a disabled button that cannot capture input. */
void rohr_ui_button_disabled(UIRect bounds, const UIButtonStyle *style);

/** Return current pointer consumption; read before rohr_ui_update(). */
bool rohr_ui_pointer_consumed_get(void);

/** Finalize overlays/focus and return one owned ROHR_UI_REPORT_ID, then clear
 * transient input and registration. Empty frames return COMPLETE. Timing covers
 * only this call, excluding earlier widget execution and application time. */
Report rohr_ui_update(void);

/** @brief Returns the default button colors. */
UIButtonStyle rohr_ui_button_style_default_get(void);

/** @brief Returns the default slider configuration and 0..1 range. */
UISliderConfig rohr_ui_slider_config_default_get(void);

/** @brief Draws and updates a caller-owned slider value. */
UISliderResult rohr_ui_slider(const char *id, float value, const UISliderConfig *config);

/** @brief Draws a slider with optional caller-owned label and value text. */
UISliderResult rohr_ui_slider_with_text(const char *id, float value,
    const UISliderConfig *config, const UISliderText *text);

#define rohr_viewport_create(...) graphics_viewport_create(__VA_ARGS__)
#define rohr_screen_create(...) graphics_screen_create(__VA_ARGS__)
#define rohr_graphics_ui_shape_create(...) graphics_ui_shape_create(__VA_ARGS__)
#define rohr_graphics_ui_text_create(...) graphics_ui_text_create(__VA_ARGS__)
#define rohr_graphics_ui_slider_create(...) graphics_ui_slider_create(__VA_ARGS__)
#define rohr_camera_create(...) graphics_camera_create(__VA_ARGS__)
#define rohr_graphics_texture_load(...) graphics_texture_load(__VA_ARGS__)
#define rohr_graphics_animation_load(...) graphics_animation_load(__VA_ARGS__)
#define rohr_graphics_font_load(...) graphics_font_load(__VA_ARGS__)
#define rohr_graphics_text_create(...) graphics_text_create(__VA_ARGS__)
#endif
