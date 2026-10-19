# Engine API Reference

This page is the GitHub-readable reference for the public Rohr Engine core C API.
The source of truth is [`include/rohr.h`](../include/rohr.h), which uses Doxygen comments for the generated HTML documentation.

Application code should include the public facade:

```c
#include "rohr.h"
```

Entity values are stable ids, not component table indexes. Use the public entity functions to validate ids and resolve indexes.

Angles and stored angular fields use clockwise degrees, with rates in degrees/second and degrees/second squared.
Zero heading is world up (+Y); zero orientation preserves authored geometry and artwork. Negative angles and multiple revolutions are retained.
Only explicitly named `_radians` scalar APIs use radians. Torque, angular impulse, and inertia retain their physical units.
See the [angle convention and format migration contract](angle_convention_contract.md) for coordinate-space rules and breaking changes.

## Contents

- <a href="#engine">Engine</a>
- <a href="#errors-and-results">Errors and Results</a>
- <a href="#console">Console</a>
- <a href="#entities">Entities</a>
- <a href="#physics">Physics</a>
- <a href="#graphics">Graphics</a>
- <a href="#math">Math</a>
- <a href="#systems">Systems</a>
- <a href="#input">Input</a>
- <a href="#tools">Tools</a>
- <a href="#other">Other</a>

## Engine

### `rohr_engine_start`

```c
EngineResult rohr_engine_start(void);
```

Starts the core engine service.

**Returns:** EngineResult containing true on success, or an engine error.

### `rohr_engine_stop`

```c
void rohr_engine_stop(void);
```

Stops the core engine service and releases its state.

### `rohr_engine_time_update`

```c
void rohr_engine_time_update(void);
```

Updates accumulated engine time from the platform clock.

### `rohr_engine_update`

```c
Report rohr_engine_update(void);
```

 Advance clock/ticks, then lifecycle cleanup; returns an owned ROHR_ENGINE_REPORT_ID.

Requires engine startup. Paused/zero-tick calls still run cleanup.

Does not update physics, graphics, audio or UI.

### `rohr_engine_tick_update`

```c
Tick rohr_engine_tick_update(void);
```

 Advance clock/ticks only and replace the latest tick delta (possibly zero).

### `rohr_engine_lifecycle_update`

```c
Report rohr_engine_lifecycle_update(void);
```

 Delete expired entities at current time/ticks; leave clock and delta unchanged.

Returns an owned ROHR_ENGINE_LIFECYCLE_REPORT_ID. Requires engine startup.

### `rohr_engine_tick_delta_get`

```c
Tick rohr_engine_tick_delta_get(void);
```

 Latest tick-update delta; reads do not consume it. Cleared on start/stop/reset.

### `rohr_engine_time_get`

```c
Time rohr_engine_time_get(void);
```

Returns the current engine time in seconds.

**Returns:** Current engine time.

### `rohr_engine_tick_get`

```c
Tick rohr_engine_tick_get(void);
```

Returns the current engine tick counter.

**Returns:** Current tick.

### `rohr_engine_pause`

```c
void rohr_engine_pause(void);
```

Pauses engine time-dependent updates.

### `rohr_engine_resume`

```c
void rohr_engine_resume(void);
```

Resumes engine time-dependent updates.

### `rohr_engine_time_per_tick_set`

```c
EngineResult rohr_engine_time_per_tick_set(Time time_per_tick);
```

 Sets the real-time duration required for one engine tick.

### `rohr_engine_time_per_tick_get`

```c
Time rohr_engine_time_per_tick_get(void);
```

 Returns the real-time duration required for one engine tick.

### `rohr_engine_event_poll`

```c
SDL_Event rohr_engine_event_poll(void);
```

Polls one SDL event.

**Returns:** SDL event value returned by the engine event poller.

### `rohr_engine_paused_get`

```c
bool rohr_engine_paused_get(void);
```

Checks whether the engine is paused.

**Returns:** true when paused, false when running.

### `rohr_engine_clock_reset`

```c
void rohr_engine_clock_reset(void);
```

Resets the engine clock baseline.

## Errors and Results

### `rohr_error_result_value`

```c
EngineResult rohr_error_result_value(bool value);
```

Creates a successful boolean engine result.

| Parameter | Description |
| --- | --- |
| `value` | Boolean value to store in the result. |

**Returns:** EngineResult containing value.

### `rohr_error_result_error`

```c
EngineResult rohr_error_result_error(EngineError error);
```

Creates a failed engine result.

| Parameter | Description |
| --- | --- |
| `error` | Error code to store in the result. |

**Returns:** EngineResult containing error.

### `rohr_error_check`

```c
#define rohr_error_check(ResultValue) error_check(ResultValue)
```

Checks whether a result contains an error.

| Parameter | Description |
| --- | --- |
| `ResultValue` | Result value to inspect. |

**Returns:** true when ResultValue contains an error, false otherwise.

### `rohr_error_code_message_get`

```c
const char *rohr_error_code_message_get(EngineError error);
```

Returns the static message associated with an engine error code.

| Parameter | Description |
| --- | --- |
| `error` | Error code to describe. |

**Returns:** Static string describing error.

### `rohr_error_message_get`

```c
#define rohr_error_message_get(ResultValue) \
```

Returns a result's error message with captured low-level detail.

ResultValue should be a result variable because this macro evaluates it

twice. Every generated Rohr result type supports this shared field layout.

| Parameter | Description |
| --- | --- |
| `ResultValue` | Generated Rohr result value to describe. |

**Returns:** Thread-local string; copy before the next error/message call on this thread.

## Console

### `rohr_console_logs_print`

```c
void rohr_console_logs_print(void);
```

Prints buffered console log messages.

### `rohr_console_start`

```c
void rohr_console_start(void);
```

Starts the engine console service.

### `rohr_console_stop`

```c
void rohr_console_stop(void);
```

Stops the engine console service.

### `rohr_console_read`

```c
bool rohr_console_read(ConsoleLogString *input);
```

Reads one console log string.

| Parameter | Description |
| --- | --- |
| `input` | Destination for the log string. |

**Returns:** true when a log string was read, false otherwise.

### `rohr_console_write`

```c
void rohr_console_write(LogSourceType source, const char *fmt, ...);
```

Writes a formatted message to the engine console.

| Parameter | Description |
| --- | --- |
| `source` | Source category for the log entry. |
| `fmt` | printf-style format string. |

### `rohr_console_active_get`

```c
bool rohr_console_active_get(void);
```

Checks whether the console is active.

**Returns:** true when active, false otherwise.

### `rohr_console_debug_write`

```c
void rohr_console_debug_write(LogSourceType source, const char *fmt, ...);
```

Writes a formatted debug message when debug logging is enabled.

| Parameter | Description |
| --- | --- |
| `source` | Source category for the log entry. |
| `fmt` | printf-style format string. |

### `rohr_console_debug_set`

```c
void rohr_console_debug_set(bool state);
```

Enables or disables debug console output.

| Parameter | Description |
| --- | --- |
| `state` | true to enable debug logging, false to disable it. |

## Entities

### `rohr_entity_info_get`

```c
EntityInfoResult rohr_entity_info_get(Entity entity, EntityInfoFlags flags);
```

Allocate a frozen current-state snapshot for the selected sections.

| Parameter | Description |
| --- | --- |
| `entity` | Live entity to inspect between updates. |
| `flags` | Nonzero combination of supported ROHR_ENTITY_INFO selectors. |

**Returns:** Owned EntityInfo or an error. Release every owning reference.

### `rohr_entity_info_retain`

```c
EntityInfoResult rohr_entity_info_retain(const EntityInfo *info);
```

Acquire another owner of the same immutable snapshot without reinspection.

| Parameter | Description |
| --- | --- |
| `info` | Live snapshot; plain C assignment does not retain ownership. |

**Returns:** Another owning EntityInfo, or an error without changing ownership.

### `rohr_entity_info_release`

```c
EngineResult rohr_entity_info_release(EntityInfo *info);
```

Release one snapshot owner and clear its value; zero is a no-op.

| Parameter | Description |
| --- | --- |
| `info` | Owning value. NULL and stale nonzero handles are errors. |

**Returns:** Success or an error. Final release frees all snapshot arrays.

### `rohr_entity_info_check`

```c
bool rohr_entity_info_check(const EntityInfo *info, EntityInfoFlags flags);
```

Check whether every selected section was captured successfully.

| Parameter | Description |
| --- | --- |
| `info` | Live immutable snapshot. |
| `flags` | Nonzero supported selectors; availability is not operation success. |

**Returns:** False for invalid input or any unavailable section.

### `rohr_entity_info_issues_count_get`

```c
EntityInfoIssueCountResult rohr_entity_info_issues_count_get( const EntityInfo *info, EntityInfoFlags flags);
```

Count captured issues matching the selected sections.

| Parameter | Description |
| --- | --- |
| `info` | Live immutable snapshot. |
| `flags` | Supported section selectors; uncaptured sections match no issues. |

**Returns:** Matching count, or an error for invalid input.

### `rohr_entity_info_issues_get`

```c
EntityInfoIssueReadResult rohr_entity_info_issues_get( const EntityInfo *info, EntityInfoFlags flags, EntityInfoIssue *destination, size_t capacity);
```

Copy a prefix of matching issues, including owned explanatory text.

| Parameter | Description |
| --- | --- |
| `info` | Live immutable snapshot. |
| `flags` | Nonzero supported section selectors. |
| `destination` | Caller array; NULL is permitted only with zero capacity. |
| `capacity` | Number of destination entries. |

**Returns:** Written and total counts; insufficient capacity is successful partial output.

### `rohr_entity_alive_check`

```c
bool rohr_entity_alive_check(Entity entity);
```

Checks whether an entity id currently refers to a live entity.

| Parameter | Description |
| --- | --- |
| `entity` | Stable entity id to inspect. |

**Returns:** true when the entity is alive, false otherwise.

### `rohr_entity_index_alive_check`

```c
bool rohr_entity_index_alive_check(EntityIndex index);
```

Checks whether an entity table index currently contains a live entity.

| Parameter | Description |
| --- | --- |
| `index` | Component table index to inspect. |

**Returns:** true when the index contains a live entity, false otherwise.

### `rohr_entity_alive_count_get`

```c
uint32_t rohr_entity_alive_count_get(void);
```

Returns the number of currently alive entities.

**Returns:** Number of alive entities.

### `rohr_entity_alive_at_get`

```c
EntityResult rohr_entity_alive_at_get(uint32_t position);
```

Returns the entity id stored at a dense alive-list position.

The position is not a component table index and can change when entities are

deleted.

| Parameter | Description |
| --- | --- |
| `position` | Dense alive-list position. |

**Returns:** EntityResult containing the entity id, or an error.

### `rohr_entity_index_get`

```c
EntityIndexResult rohr_entity_index_get(Entity entity);
```

Resolves a stable entity id to its current component table index.

| Parameter | Description |
| --- | --- |
| `entity` | Stable entity id to resolve. |

**Returns:** EntityIndexResult containing the index or an invalid-entity error.

### `rohr_entity_from_index_get`

```c
EntityResult rohr_entity_from_index_get(EntityIndex index);
```

Returns the stable entity id stored at a component table index.

| Parameter | Description |
| --- | --- |
| `index` | Component table index to inspect. |

**Returns:** EntityResult containing the entity id, or an error.

### `rohr_entity_add`

```c
EntityResult rohr_entity_add(void);
```

Creates a new entity.

Entity ids are stable handles and may not match component table indexes.

**Returns:** EntityResult containing the new entity id, or an error if the entity limit is reached.

### `rohr_entity_description_add`

```c
EntityResult rohr_entity_description_add(EntityDescription description);
```

 Create copied metadata and ownership; empty creation names are generated.

### `rohr_entity_description_set`

```c
EngineResult rohr_entity_description_set(Entity entity, EntityDescription description);
```

 Atomically replace name and owner; the name must be nonempty and unique.

### `rohr_entity_description_get`

```c
EntityDescriptionResult rohr_entity_description_get(Entity entity);
```

 Get a copied name and current independent owner.

### `rohr_entity_name_set`

```c
EngineResult rohr_entity_name_set(Entity entity, const char *name);
```

 Assigns a unique fixed-size name to an entity.

### `rohr_entity_by_name_get`

```c
EntityResult rohr_entity_by_name_get(const char *name);
```

 Finds a live entity by its state-file name.

### `rohr_entity_name_get`

```c
EntityNameResult rohr_entity_name_get(Entity entity);
```

 Returns a copy of an entity's fixed-size name component.

### `rohr_entity_delete`

```c
EngineResult rohr_entity_delete(Entity entity);
```

Deletes an entity and releases its slot for reuse.

| Parameter | Description |
| --- | --- |
| `entity` | Stable entity id to delete. |

**Returns:** EngineResult describing success or failure.

### `rohr_entity_components_add`

```c
EngineResult rohr_entity_components_add(Entity entity, RohrComponentMask mask);
```

Adds components to an entity.

| Parameter | Description |
| --- | --- |
| `entity` | Stable entity id to modify. |
| `mask` | Component mask to add. |

**Returns:** EngineResult describing success or failure.

### `rohr_entity_components_check`

```c
bool rohr_entity_components_check(Entity entity, RohrComponentMask components);
```

Checks whether an entity has all requested components.

| Parameter | Description |
| --- | --- |
| `entity` | Stable entity id to inspect. |
| `components` | Component mask to test. |

**Returns:** true when entity has every requested component, false otherwise.

### `rohr_entity_index_components_check`

```c
bool rohr_entity_index_components_check(EntityIndex index, RohrComponentMask components);
```

Checks whether an entity table index has all requested components.

| Parameter | Description |
| --- | --- |
| `index` | Component table index to inspect. |
| `components` | Component mask to test. |

**Returns:** true when index has every requested component, false otherwise.

### `rohr_entity_group_create`

```c
GroupIdResult rohr_entity_group_create(void);
```

Creates a reusable entity group.

**Returns:** GroupIdResult containing a group id, or an error.

### `rohr_entity_group_name_set`

```c
EngineResult rohr_entity_group_name_set(GroupId group, const char *name);
```

 Assigns a unique fixed-size name to a generic group.

### `rohr_entity_group_by_name_get`

```c
GroupIdResult rohr_entity_group_by_name_get(const char *name);
```

 Finds a live generic group by name.

### `rohr_entity_group_name_get`

```c
GroupNameResult rohr_entity_group_name_get(GroupId group);
```

 Returns a copy of a generic group's fixed-size name.

### `rohr_entity_group_destroy`

```c
EngineResult rohr_entity_group_destroy(GroupId group);
```

Destroys a generic entity group and clears member group references.

| Parameter | Description |
| --- | --- |
| `group` | Group id to destroy. |

**Returns:** EngineResult describing success or failure.

### `rohr_entity_object_members_get`

```c
GroupIdResult rohr_entity_object_members_get(Entity object);
```

 Get protected directly owned members.

### `rohr_entity_owner_get`

```c
EntityResult rohr_entity_owner_get(Entity entity);
```

 Get the immediate lifetime owner; ENTITY_INVALID means independent.

### `rohr_entity_owner_set`

```c
EngineResult rohr_entity_owner_set(Entity entity, Entity owner);
```

Transfer ownership or release with ENTITY_INVALID, preserving world state,

parenting and connections. Updates soft-body membership without converting

entity types. May allocate ownership storage; failure leaves ownership intact.

### `rohr_entity_group_add`

```c
EngineResult rohr_entity_group_add(GroupId group, Entity entity);
```

Adds an entity to a generic group.

| Parameter | Description |
| --- | --- |
| `group` | Group id to update. |
| `entity` | Entity id to add. |

**Returns:** EngineResult describing success or failure.

### `rohr_entity_group_remove`

```c
EngineResult rohr_entity_group_remove(GroupId group, Entity entity);
```

Removes an entity from a generic group.

| Parameter | Description |
| --- | --- |
| `group` | Group id to update. |
| `entity` | Entity id to remove. |

**Returns:** EngineResult describing success or failure.

### `rohr_entity_group_entity_check`

```c
bool rohr_entity_group_entity_check(GroupId group, Entity entity);
```

Checks whether an entity belongs to a group.

| Parameter | Description |
| --- | --- |
| `group` | Group id to inspect. |
| `entity` | Entity id to search for. |

**Returns:** true when entity belongs to the group.

### `rohr_entity_group_get`

```c
EntityGroupResult rohr_entity_group_get(GroupId group);
```

Returns an entity group.

| Parameter | Description |
| --- | --- |
| `group` | Group id to inspect. |

**Returns:** EntityGroupResult containing group data, or an error.

### `rohr_entity_groups_get`

```c
EntityGroupMembershipResult rohr_entity_groups_get(Entity entity);
```

Returns the groups assigned to an entity.

| Parameter | Description |
| --- | --- |
| `entity` | Entity id to inspect. |

**Returns:** EntityGroupMembershipResult containing group ids, or an error.

### `rohr_entity_components_delete`

```c
EngineResult rohr_entity_components_delete(Entity entity, RohrComponentMask mask);
```

Removes components from an entity.

| Parameter | Description |
| --- | --- |
| `entity` | Stable entity id to modify. |
| `mask` | Component mask to remove. |

**Returns:** EngineResult describing success or failure.

### `rohr_entity_child_set`

```c
EngineResult rohr_entity_child_set(Entity parent, Entity child);
```

Adds a child relationship from parent to child.

| Parameter | Description |
| --- | --- |
| `parent` | Parent entity id. |
| `child` | Child entity id. |

**Returns:** EngineResult describing success or failure.

### `rohr_entity_parent_set`

```c
EngineResult rohr_entity_parent_set(Entity child, Entity parent);
```

Sets an entity parent relationship.

| Parameter | Description |
| --- | --- |
| `child` | Child entity id. |
| `parent` | Parent entity id. |

**Returns:** EngineResult describing success or failure.

### `rohr_entity_parent_remove`

```c
EngineResult rohr_entity_parent_remove(Entity child);
```

Removes the parent relationship from an entity.

| Parameter | Description |
| --- | --- |
| `child` | Child entity id. |

**Returns:** EngineResult describing success or failure.

### `rohr_entity_child_remove`

```c
EngineResult rohr_entity_child_remove(Entity parent, Entity child);
```

Removes a child relationship from a parent entity.

| Parameter | Description |
| --- | --- |
| `parent` | Parent entity id. |
| `child` | Child entity id. |

**Returns:** EngineResult describing success or failure.

### `rohr_entity_children_get`

```c
ChildrenResult rohr_entity_children_get(Entity entity);
```

Returns the children group assigned to an entity.

| Parameter | Description |
| --- | --- |
| `entity` | Stable entity id to inspect. |

**Returns:** ChildrenResult containing a group id, or an error.

### `rohr_entity_parent_get`

```c
ParentResult rohr_entity_parent_get(Entity entity);
```

Returns the parent assigned to an entity.

| Parameter | Description |
| --- | --- |
| `entity` | Stable entity id to inspect. |

**Returns:** ParentResult containing parent id, or an error.

### `rohr_entity_lifetime_description_set`

```c
EngineResult rohr_entity_lifetime_description_set(Entity entity, EntityLifetimeDescription description);
```

Replaces absolute lifetime deadlines; both disabled removes the configuration.

| Parameter | Description |
| --- | --- |
| `entity` | Stable entity id to modify. |
| `description` | Explicit flags allow enabled zero deadlines. Time must be finite and nonnegative. |

**Returns:** EngineResult describing success or failure.

### `rohr_entity_lifetime_description_get`

```c
EntityLifetimeDescriptionResult rohr_entity_lifetime_description_get(Entity entity);
```

 Get lifetime configuration, or disabled zeros when absent on a live entity.

### `rohr_entity_life_time_remove`

```c
EngineResult rohr_entity_life_time_remove(Entity entity);
```

Removes lifetime data from an entity.

| Parameter | Description |
| --- | --- |
| `entity` | Stable entity id to modify. |

**Returns:** EngineResult describing success or failure.

## Physics

### `rohr_physics_dt_per_tick_set`

```c
EngineResult rohr_physics_dt_per_tick_set(Time dt);
```

 Sets an explicit simulation delta per engine tick.

### `rohr_physics_dt_per_tick_get`

```c
Time rohr_physics_dt_per_tick_get(void);
```

 Returns the current physics delta per tick.

### `rohr_physics_engine_time_per_tick_use`

```c
void rohr_physics_engine_time_per_tick_use(void);
```

 Restores the engine time-per-tick default.

### `rohr_physics_solver_iterations_set`

```c
EngineResult rohr_physics_solver_iterations_set(uint32_t iterations);
```

 Sets constraint-solver iterations. Must be greater than zero.

### `rohr_physics_solver_iterations_get`

```c
uint32_t rohr_physics_solver_iterations_get(void);
```

 Returns constraint-solver iterations. Defaults to 8.

### `rohr_physics_substeps_set`

```c
EngineResult rohr_physics_substeps_set(uint32_t substeps);
```

 Sets integration and collision-detection substeps. Must be greater than zero.

### `rohr_physics_substeps_get`

```c
uint32_t rohr_physics_substeps_get(void);
```

 Returns physics substeps. Defaults to 1.

### `rohr_physics_gravity_set`

```c
EngineResult rohr_physics_gravity_set(Acceleration gravity);
```

 Sets the global acceleration applied to ROHR_GRAVITY entities.

### `rohr_physics_gravity_get`

```c
Acceleration rohr_physics_gravity_get(void);
```

 Returns the current global gravity acceleration.

### `rohr_physics_gravity_enable`

```c
EngineResult rohr_physics_gravity_enable(Entity entity);
```

 Enables engine gravity for an entity.

### `rohr_physics_gravity_disable`

```c
EngineResult rohr_physics_gravity_disable(Entity entity);
```

 Disables engine gravity for an entity.

### `rohr_physics_gravity_check`

```c
bool rohr_physics_gravity_check(Entity entity);
```

 Returns whether engine gravity is enabled for an entity.

### `rohr_physics_pipeline_step_begin`

```c
void rohr_physics_pipeline_step_begin(void);
```

 Begins a complete custom physics step and advances interaction state.

### `rohr_physics_pipeline_substep_begin`

```c
void rohr_physics_pipeline_substep_begin(void);
```

 Clears transient constraints before one custom substep.

### `rohr_physics_pipeline_accelerations_clear`

```c
void rohr_physics_pipeline_accelerations_clear(void);
```

 Clears force-derived acceleration from the previous substep.

### `rohr_physics_pipeline_gravity_apply`

```c
void rohr_physics_pipeline_gravity_apply(void);
```

 Applies global gravity to opted-in movable entities.

### `rohr_physics_pipeline_forces_apply`

```c
Report rohr_physics_pipeline_forces_apply(void);
```

 Applies spring-joint and soft-body-beam forces.

### `rohr_physics_pipeline_integrate`

```c
Report rohr_physics_pipeline_integrate(double dt);
```

 Integrates rigid-body state and reports world-boundary failures.

### `rohr_physics_pipeline_contacts_gather`

```c
Report rohr_physics_pipeline_contacts_gather(void);
```

 Detects contacts and gathers contact constraints.

### `rohr_physics_pipeline_joints_gather`

```c
Report rohr_physics_pipeline_joints_gather(void);
```

 Gathers active pin and weld joint constraints.

### `rohr_physics_pipeline_constraints_solve`

```c
Report rohr_physics_pipeline_constraints_solve(uint32_t iterations);
```

 Solves currently gathered contacts and joints.

### `rohr_physics_pipeline_substep`

```c
Report rohr_physics_pipeline_substep(double dt);
```

 Runs one standard physics substep.

### `rohr_physics_pipeline_update`

```c
Report rohr_physics_pipeline_update(double dt);
```

 Runs the standard plug-and-play physics pipeline.

### `rohr_physics_update`

```c
Report rohr_physics_update(Tick ticks);
```

 Advances physics and returns any pipeline failure.

### `rohr_physics_dt_update`

```c
Report rohr_physics_dt_update(Time dt);
```

 Advances physics once with an explicit exceptional delta.

### `rohr_physics_shape_world_translate`

```c
Shape rohr_physics_shape_world_translate(Shape shape, Position position, Orientation angle);
```

Translates a local shape into world space.

| Parameter | Description |
| --- | --- |
| `shape` | Shape with origin-relative local vertices; no recentering occurs. |
| `position` | World position of the local origin. |
| `angle` | World orientation in clockwise degrees. |

**Returns:** World-space shape.

### `rohr_physics_shape_world_radians_translate`

```c
Shape rohr_physics_shape_world_radians_translate(Shape shape, Position position, Orientation angle);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_physics_polygon_moment_of_inertia`

```c
float rohr_physics_polygon_moment_of_inertia(Shape shape, Mass mass_value);
```

Calculates polygon moment of inertia.

| Parameter | Description |
| --- | --- |
| `shape` | Polygon shape. |
| `mass_value` | Shape mass. |

**Returns:** Moment of inertia value.

### `rohr_physics_sat_overlap_get`

```c
OverlapInfo rohr_physics_sat_overlap_get(Shape shape_1, Shape shape_2);
```

Gets overlap information using the separating axis theorem.

| Parameter | Description |
| --- | --- |
| `shape_1` | First shape. |
| `shape_2` | Second shape. |

**Returns:** Geometric overlap information.

### `rohr_physics_circle_moment_of_inertia`

```c
Vec1D rohr_physics_circle_moment_of_inertia(Shape circle, Mass mass_value);
```

Calculates circle moment of inertia.

| Parameter | Description |
| --- | --- |
| `circle` | Circle shape. |
| `mass_value` | Circle mass. |

**Returns:** Moment of inertia value.

### `rohr_physics_entity_held_get`

```c
bool rohr_physics_entity_held_get(EntityIndex index);
```

Checks whether an entity index has ROHR_HOLD.

| Parameter | Description |
| --- | --- |
| `index` | Entity table index to inspect. |

**Returns:** true when index is live and held, false otherwise.

### `rohr_physics_acceleration_set`

```c
EngineResult rohr_physics_acceleration_set(Entity entity, Acceleration a);
```

Sets an entity acceleration component value.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `a` | Acceleration value. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_angular_acceleration_set`

```c
EngineResult rohr_physics_angular_acceleration_set( Entity entity, AngularAcceleration acceleration );
```

Sets an entity angular acceleration component value.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `acceleration` | Angular acceleration value. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_angular_acceleration_radians_set`

```c
EngineResult rohr_physics_angular_acceleration_radians_set( Entity entity, AngularAcceleration acceleration );
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_physics_acceleration_toward_position_set`

```c
EngineResult rohr_physics_acceleration_toward_position_set(Entity entity, float acceleration_magnitude, Position position);
```

Sets entity acceleration toward a world position.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `acceleration_magnitude` | Acceleration magnitude to apply along the direction to position. |
| `position` | Target world position. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_acceleration_toward_entity_set`

```c
EngineResult rohr_physics_acceleration_toward_entity_set(Entity entity, float acceleration_magnitude, Entity target);
```

Sets entity acceleration toward another entity's current world position.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `acceleration_magnitude` | Acceleration magnitude to apply along the direction to target. |
| `target` | Target entity. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_acceleration_away_from_position_set`

```c
EngineResult rohr_physics_acceleration_away_from_position_set(Entity entity, float acceleration_magnitude, Position position);
```

Sets entity acceleration away from a world position.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `acceleration_magnitude` | Acceleration magnitude to apply away from position. |
| `position` | Source world position. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_acceleration_away_from_entity_set`

```c
EngineResult rohr_physics_acceleration_away_from_entity_set(Entity entity, float acceleration_magnitude, Entity target);
```

Sets entity acceleration away from another entity's current world position.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `acceleration_magnitude` | Acceleration magnitude to apply away from target. |
| `target` | Source entity. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_group_acceleration_toward_entity_set`

```c
EngineResult rohr_physics_group_acceleration_toward_entity_set(GroupId group, float acceleration_magnitude, Entity target);
```

Sets acceleration toward an entity for every live entity in a group.

| Parameter | Description |
| --- | --- |
| `group` | Group id to update. |
| `acceleration_magnitude` | Acceleration magnitude to apply along the direction to target. |
| `target` | Target entity. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_group_acceleration_away_from_entity_set`

```c
EngineResult rohr_physics_group_acceleration_away_from_entity_set(GroupId group, float acceleration_magnitude, Entity target);
```

Sets acceleration away from an entity for every live entity in a group.

| Parameter | Description |
| --- | --- |
| `group` | Group id to update. |
| `acceleration_magnitude` | Acceleration magnitude to apply away from target. |
| `target` | Source entity. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_velocity_set`

```c
EngineResult rohr_physics_velocity_set(Entity entity, Velocity v);
```

Sets an entity velocity component value.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `v` | Velocity value. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_world_position_check`

```c
bool rohr_physics_world_position_check(Position position);
```

 Check that both coordinates are finite and inside the supported world.

### `rohr_physics_velocity_toward_position_set`

```c
EngineResult rohr_physics_velocity_toward_position_set(Entity entity, float speed, Position position);
```

Sets entity velocity toward a world position.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `speed` | Speed to apply along the direction to position. |
| `position` | Target world position. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_velocity_toward_entity_set`

```c
EngineResult rohr_physics_velocity_toward_entity_set(Entity entity, float speed, Entity target);
```

Sets entity velocity toward another entity's current world position.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `speed` | Speed to apply along the direction to target. |
| `target` | Target entity. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_velocity_away_from_position_set`

```c
EngineResult rohr_physics_velocity_away_from_position_set(Entity entity, float speed, Position position);
```

Sets entity velocity away from a world position.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `speed` | Speed to apply away from position. |
| `position` | Source world position. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_velocity_away_from_entity_set`

```c
EngineResult rohr_physics_velocity_away_from_entity_set(Entity entity, float speed, Entity target);
```

Sets entity velocity away from another entity's current world position.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `speed` | Speed to apply away from target. |
| `target` | Source entity. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_group_velocity_toward_entity_set`

```c
EngineResult rohr_physics_group_velocity_toward_entity_set(GroupId group, float speed, Entity target);
```

Sets velocity toward an entity for every live entity in a group.

| Parameter | Description |
| --- | --- |
| `group` | Group id to update. |
| `speed` | Speed to apply along the direction to target. |
| `target` | Target entity. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_group_velocity_away_from_entity_set`

```c
EngineResult rohr_physics_group_velocity_away_from_entity_set(GroupId group, float speed, Entity target);
```

Sets velocity away from an entity for every live entity in a group.

| Parameter | Description |
| --- | --- |
| `group` | Group id to update. |
| `speed` | Speed to apply away from target. |
| `target` | Source entity. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_entity_stop`

```c
EngineResult rohr_physics_entity_stop(Entity entity);
```

Sets an entity velocity to zero.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_group_entities_stop`

```c
EngineResult rohr_physics_group_entities_stop(GroupId group);
```

Sets velocity to zero for every live entity in a group.

| Parameter | Description |
| --- | --- |
| `group` | Group id to update. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_impulse_apply`

```c
EngineResult rohr_physics_impulse_apply(Entity entity, Vec2D impulse);
```

Applies an immediate linear impulse to an entity velocity.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `impulse` | Impulse vector. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_angular_impulse_apply`

```c
EngineResult rohr_physics_angular_impulse_apply(Entity entity, float impulse);
```

Adds an immediate angular impulse about the effective COM.

Adds degrees(impulse / derived inertia) to clockwise angular velocity without

changing motion type. Zero inverse inertia (including particles, static, held, kinematic,

massless, geometryless, and fully rotation-locked bodies) is a successful

no-op. Nonfinite input or resulting velocity is rejected without mutation.

| Parameter | Description |
| --- | --- |
| `entity` | Body to modify. |
| `impulse` | Signed angular impulse in mass * distance squared / second. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_position_set`

```c
EngineResult rohr_physics_position_set(Entity entity, Position p);
```

Sets the world origin while preserving local geometry and attachment offsets.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `p` | Position value. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_position_get`

```c
PositionResult rohr_physics_position_get(Entity entity);
```

 Returns an entity's world position.

### `rohr_physics_center_of_mass_local_position_set`

```c
EngineResult rohr_physics_center_of_mass_local_position_set(Entity entity, Position local_offset);
```

Selects explicit COM using a finite origin-relative offset.

Geometry is optional. Standalone particles reject overrides.

Soft bodies redistribute positive node masses at fixed total mass to the

requested COM, preserving their prior proportions as closely as possible.

Unreachable targets and numerical failures leave every node unchanged.

Failure preserves configuration; success preserves origin, geometry, and velocities.

### `rohr_physics_center_of_mass_local_position_get`

```c
PositionResult rohr_physics_center_of_mass_local_position_get(Entity entity);
```

Returns the effective COM in local origin axes.

Automatic COM requires valid active geometry; an explicit offset does not.

Standalone particles use their circle centroid.

Soft bodies derive COM from current node positions and masses; an empty

or zero-total-mass soft body has no available COM.

### `rohr_physics_center_of_mass_world_position_get`

```c
PositionResult rohr_physics_center_of_mass_world_position_get(Entity entity);
```

Returns origin plus rotated local COM.

Requires a transform and an available local COM.

### `rohr_physics_center_of_mass_automatic_set`

```c
EngineResult rohr_physics_center_of_mass_automatic_set(Entity entity);
```

 Discards explicit COM for ordinary bodies. Soft bodies: successful no-op.

### `rohr_physics_center_of_mass_automatic_check`

```c
BoolResult rohr_physics_center_of_mass_automatic_check(Entity entity);
```

 Returns the COM mode (always true for soft bodies), or an invalid-entity error.

### `rohr_physics_moment_of_inertia_get`

```c
MomentOfInertiaResult rohr_physics_moment_of_inertia_get(Entity entity);
```

Returns derived inertia about the effective COM.

Requires valid geometry and stored mass, including zero mass.

Static and locked bodies may report inertia without enabling angular response.

### `rohr_physics_mass_set`

```c
EngineResult rohr_physics_mass_set(Entity entity, Mass m);
```

 Sets finite non-negative ordinary mass. For soft bodies, proportionally

scales positive node masses to a positive total; failures preserve all nodes.

No ROHR_MASS component is added to the soft-body container.

### `rohr_physics_mass_get`

```c
MassResult rohr_physics_mass_get(Entity entity);
```

 Returns stored mass, or summed node mass for a soft body (zero when empty).

### `rohr_physics_force_add`

```c
EntityResult rohr_physics_force_add(ForceDescription description);
```

 Copies a description into one persistent source; delete its entity to stop future contributions.

### `rohr_physics_torque_add`

```c
EntityResult rohr_physics_torque_add(TorqueDescription description);
```

 Copies a description into one persistent source; delete its entity to stop future contributions.

### `rohr_physics_force_description_get`

```c
ForceDescriptionResult rohr_physics_force_description_get(Entity source);
```

 Returns configuration with the current generic owner; invalid sources return an error.

### `rohr_physics_torque_description_get`

```c
TorqueDescriptionResult rohr_physics_torque_description_get(Entity source);
```

 Returns configuration with the current generic owner; invalid sources return an error.

### `rohr_physics_force_description_set`

```c
EngineResult rohr_physics_force_description_set(Entity source, ForceDescription description);
```

 Atomically replaces configuration and lifetime owner; invalid input leaves both unchanged.

### `rohr_physics_torque_description_set`

```c
EngineResult rohr_physics_torque_description_set(Entity source, TorqueDescription description);
```

 Atomically replaces configuration and lifetime owner; invalid input leaves both unchanged.

### `rohr_physics_force_target_set`

```c
EngineResult rohr_physics_force_target_set(Entity source, Entity target);
```

 Replaces this field on an existing source using full-description validation.

### `rohr_physics_force_target_get`

```c
EntityResult rohr_physics_force_target_get(Entity source);
```

 Returns copied stored configuration; invalid handles or missing source components return an error.

### `rohr_physics_force_location_set`

```c
EngineResult rohr_physics_force_location_set(Entity source, ForceLocation location);
```

 Replaces this field on an existing source using full-description validation.

### `rohr_physics_force_location_get`

```c
ForceLocationResult rohr_physics_force_location_get(Entity source);
```

 Returns copied stored configuration; invalid handles or missing source components return an error.

### `rohr_physics_force_orientation_set`

```c
EngineResult rohr_physics_force_orientation_set(Entity source, ForceOrientation orientation);
```

 Replaces this field on an existing source using full-description validation.

### `rohr_physics_force_orientation_get`

```c
ForceOrientationResult rohr_physics_force_orientation_get(Entity source);
```

 Returns copied stored configuration; invalid handles or missing source components return an error.

### `rohr_physics_force_magnitude_set`

```c
EngineResult rohr_physics_force_magnitude_set(Entity source, float magnitude);
```

 Replaces this field on an existing source using full-description validation.

### `rohr_physics_force_magnitude_get`

```c
ForceMagnitudeResult rohr_physics_force_magnitude_get(Entity source);
```

 Returns copied stored configuration; invalid handles or missing source components return an error.

### `rohr_physics_torque_target_set`

```c
EngineResult rohr_physics_torque_target_set(Entity source, Entity target);
```

 Replaces this field on an existing source using full-description validation.

### `rohr_physics_torque_target_get`

```c
EntityResult rohr_physics_torque_target_get(Entity source);
```

 Returns copied stored configuration; invalid handles or missing source components return an error.

### `rohr_physics_torque_magnitude_set`

```c
EngineResult rohr_physics_torque_magnitude_set(Entity source, Torque magnitude);
```

 Replaces this field on an existing source using full-description validation.

### `rohr_physics_torque_magnitude_get`

```c
TorqueResult rohr_physics_torque_magnitude_get(Entity source);
```

 Returns copied stored configuration; invalid handles or missing source components return an error.

### `rohr_physics_force_apply`

```c
EngineResult rohr_physics_force_apply(ForceDescription description);
```

 Copies a source for engine tick T, expiring through lifecycle cleanup at T + 1.

Requires owner == ENTITY_INVALID; use add for a persistent owned source.

### `rohr_physics_torque_apply`

```c
EngineResult rohr_physics_torque_apply(TorqueDescription description);
```

 Copies a source for engine tick T, expiring through lifecycle cleanup at T + 1.

Requires owner == ENTITY_INVALID; use add for a persistent owned source.

### `rohr_physics_hitbox_set`

```c
EngineResult rohr_physics_hitbox_set(Entity entity, Shape hitbox);
```

Sets a simple polygon hitbox and prepares hidden convex decomposition when needed.

Degenerate and self-intersecting outlines are rejected. This does not enable

physical collision response.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `hitbox` | Local-space hitbox shape. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_hitbox_get`

```c
ShapeResult rohr_physics_hitbox_get(Entity entity);
```

 Returns the active local-space hitbox variant.

### `rohr_physics_hitbox_remove`

```c
EngineResult rohr_physics_hitbox_remove(Entity entity);
```

 Removes every hitbox variant and the ROHR_HIT_BOX component.

### `rohr_physics_hitbox_add`

```c
EngineResult rohr_physics_hitbox_add(Entity entity, Shape hitbox);
```

 Appends a local-space hitbox variant. The first variant becomes active.

### `rohr_physics_hitbox_at_get`

```c
ShapeResult rohr_physics_hitbox_at_get(Entity entity, size_t index);
```

 Returns a local-space hitbox variant by ordered index.

### `rohr_physics_hitbox_at_set`

```c
EngineResult rohr_physics_hitbox_at_set(Entity entity, size_t index, Shape hitbox);
```

 Replaces a local-space hitbox variant by ordered index.

### `rohr_physics_hitbox_at_remove`

```c
EngineResult rohr_physics_hitbox_at_remove(Entity entity, size_t index);
```

 Removes one local-space hitbox variant by ordered index.

### `rohr_physics_hitbox_count_get`

```c
HitboxIndexResult rohr_physics_hitbox_count_get(Entity entity);
```

 Returns the number of hitbox variants owned by an entity.

### `rohr_physics_hitbox_active_index_get`

```c
HitboxIndexResult rohr_physics_hitbox_active_index_get(Entity entity);
```

 Returns the active hitbox variant index.

### `rohr_physics_hitbox_active_index_set`

```c
EngineResult rohr_physics_hitbox_active_index_set(Entity entity, size_t index);
```

 Selects the hitbox variant used by the next physics tick.

### `rohr_physics_collision_filter_description_default_get`

```c
CollisionFilterDescription rohr_physics_collision_filter_description_default_get(void);
```

 Return the default collision filtering configuration.

### `rohr_physics_collision_filter_description_set`

```c
EngineResult rohr_physics_collision_filter_description_set(Entity entity, CollisionFilterDescription config);
```

 Replace an entity's collision category and whitelist, with or without geometry.

### `rohr_physics_collision_filter_description_get`

```c
CollisionFilterDescriptionResult rohr_physics_collision_filter_description_get(Entity entity);
```

 Return an entity's collision filtering configuration.

### `rohr_physics_collision_category_set`

```c
EngineResult rohr_physics_collision_category_set(Entity entity, RohrCollisionCategoryMask category);
```

 Set the collision categories represented by an entity.

### `rohr_physics_collision_with_set`

```c
EngineResult rohr_physics_collision_with_set(Entity entity, RohrCollisionCategoryMask categories);
```

 Set the collision category whitelist for an entity.

### `rohr_physics_collision_with_all_set`

```c
EngineResult rohr_physics_collision_with_all_set(Entity entity);
```

 Allow an entity to collide with every category.

### `rohr_physics_collision_with_none_set`

```c
EngineResult rohr_physics_collision_with_none_set(Entity entity);
```

 Prevent an entity from colliding with every category.

### `rohr_physics_collision_between_check`

```c
bool rohr_physics_collision_between_check(Entity entity_1, Entity entity_2);
```

 Return whether two entities mutually permit collision checks.

### `rohr_physics_orientation_set`

```c
EngineResult rohr_physics_orientation_set(Entity entity, Orientation angle);
```

Sets an entity orientation component value.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `angle` | Orientation in clockwise degrees. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_orientation_radians_set`

```c
EngineResult rohr_physics_orientation_radians_set(Entity entity, Orientation angle);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_physics_angular_velocity_set`

```c
EngineResult rohr_physics_angular_velocity_set(Entity entity, AngularVelocity v);
```

Sets clockwise angular velocity in degrees per second.

For a soft body, adjusts node velocities around its mass-weighted COM,

preserving COM translation and deformation motion. This is a one-time edit,

not a constraint. Node body modes are preserved; no container angular state

is created. Empty/degenerate bodies and invalid or overflowing values fail

without mutation.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `v` | Angular velocity value. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_angular_velocity_radians_set`

```c
EngineResult rohr_physics_angular_velocity_radians_set(Entity entity, AngularVelocity v);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_physics_angular_velocity_get`

```c
AngularVelocityResult rohr_physics_angular_velocity_get(Entity entity);
```

Returns an entity angular velocity.

For a soft body, derives clockwise angular momentum divided by inertia from

current node motion about COM. Empty or degenerate bodies return an error;

no single physical orientation is implied.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to inspect. |

**Returns:** AngularVelocityResult containing clockwise degrees per second, or an error.

### `rohr_physics_angular_velocity_radians_get`

```c
AngularVelocityResult rohr_physics_angular_velocity_radians_get(Entity entity);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_physics_angular_velocity_maximum_set`

```c
EngineResult rohr_physics_angular_velocity_maximum_set( Entity entity, AngularVelocity maximum );
```

 Sets the absolute angular-velocity limit applied before orientation integration.

### `rohr_physics_angular_velocity_maximum_radians_set`

```c
EngineResult rohr_physics_angular_velocity_maximum_radians_set( Entity entity, AngularVelocity maximum );
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_physics_angular_velocity_maximum_get`

```c
AngularVelocityResult rohr_physics_angular_velocity_maximum_get(Entity entity);
```

 Returns an entity's configured absolute angular-velocity limit.

### `rohr_physics_angular_velocity_maximum_radians_get`

```c
AngularVelocityResult rohr_physics_angular_velocity_maximum_radians_get(Entity entity);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_physics_global_hit_box_get`

```c
ShapeResult rohr_physics_global_hit_box_get(Entity entity);
```

Returns the active hitbox at the current origin and orientation, without waiting for a physics tick.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to inspect. |

**Returns:** ShapeResult containing the world-space hitbox, or an error.

### `rohr_physics_restitution_set`

```c
EngineResult rohr_physics_restitution_set(Entity entity, Restitution restitution);
```

Sets an entity restitution value.

For soft-body beams, stores an independent coefficient without enabling

collision. Beam values must be finite and in [0, 1]; errors preserve state.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `restitution` | Restitution value. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_dynamic_set`

```c
EngineResult rohr_physics_dynamic_set(Entity entity);
```

Marks an entity as dynamic for physics simulation.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_static_set`

```c
EngineResult rohr_physics_static_set(Entity entity);
```

Marks an entity as static for physics simulation.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_entity_hold`

```c
EngineResult rohr_physics_entity_hold(Entity entity);
```

Adds ROHR_HOLD so physics update stages preserve current values.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_entity_unhold`

```c
EngineResult rohr_physics_entity_unhold(Entity entity);
```

Removes ROHR_HOLD without changing ROHR_STATIC or ROHR_DYNAMIC state.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_group_entities_hold`

```c
EngineResult rohr_physics_group_entities_hold(GroupId group);
```

Adds ROHR_HOLD to every live entity in a group.

| Parameter | Description |
| --- | --- |
| `group` | Group id to update. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_group_entities_unhold`

```c
EngineResult rohr_physics_group_entities_unhold(GroupId group);
```

Removes ROHR_HOLD from every live entity in a group.

| Parameter | Description |
| --- | --- |
| `group` | Group id to update. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_angle_lock_set`

```c
EngineResult rohr_physics_angle_lock_set(Entity entity, Orientation min, Orientation max);
```

Locks an entity orientation between minimum and maximum angles.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `min` | Minimum orientation in clockwise degrees. |
| `max` | Maximum orientation in clockwise degrees. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_angle_lock_radians_set`

```c
EngineResult rohr_physics_angle_lock_radians_set(Entity entity, Orientation min, Orientation max);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_physics_axis_lock_description_set`

```c
EngineResult rohr_physics_axis_lock_description_set(Entity entity, AxisLockDescription description);
```

 Atomically replace the axis constraint; finite nonzero axis required.

Stores a normalized axis without changing current motion or ownership.

### `rohr_physics_axis_lock_description_get`

```c
AxisLockDescriptionResult rohr_physics_axis_lock_description_get(Entity entity);
```

 Get stored configuration; returns component-missing when no axis lock exists.

### `rohr_physics_axis_lock_remove`

```c
EngineResult rohr_physics_axis_lock_remove(Entity entity);
```

 Remove the axis lock without changing current motion. Idempotent for live entities.

### `rohr_physics_friction_set`

```c
EngineResult rohr_physics_friction_set(Entity entity, float friction);
```

Sets an entity friction value.

For soft-body beams, stores an independent coefficient without enabling

collision. Beam values must be finite and nonnegative; errors preserve state.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `friction` | Friction value. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_transform_lock_description_set`

```c
EngineResult rohr_physics_transform_lock_description_set(Entity driven, TransformLockDescription description);
```

 Atomically replace transform-following configuration using clockwise degrees.

Requires a live driver even when all flags are false. Does not capture the

current transform, move entities, or alter ownership. Use explicit conversion

helpers when supplying radians.

### `rohr_physics_transform_lock_description_get`

```c
TransformLockDescriptionResult rohr_physics_transform_lock_description_get(Entity driven);
```

 Get stored configuration, including its driver handle; component-missing

when absent. A deleted driver can remain in the stored configuration.

### `rohr_physics_transform_lock_remove`

```c
EngineResult rohr_physics_transform_lock_remove(Entity entity);
```

Removes an entity transform lock.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_transform_lock_current_transform_set`

```c
EngineResult rohr_physics_transform_lock_current_transform_set( Entity driven, Entity driver, bool lock_position, bool lock_orientation, bool inherit_velocity );
```

Locks one entity to another using their current transform offset.

| Parameter | Description |
| --- | --- |
| `driven` | Entity whose transform is driven. |
| `driver` | Entity used as the transform source. |
| `lock_position` | true to lock position. |
| `lock_orientation` | true to lock orientation. |
| `inherit_velocity` | true to inherit driver velocity. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_target_set`

```c
EngineResult rohr_physics_target_set(Entity entity, Entity target);
```

Sets the target used by a force or torque source entity.

| Parameter | Description |
| --- | --- |
| `entity` | Force or torque source entity to modify. |
| `target` | Live entity that receives the force or torque. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_joint_description_set`

```c
EngineResult rohr_physics_joint_description_set(Entity entity, JointDescription description);
```

 Atomically replace the complete configuration and lifetime owner.

Invalid anchors disconnect endpoints; stale nonzero handles are rejected.

Automatic rest values measure the current pose once, or zero when disconnected.

Failure leaves the entity, configuration, and ownership unchanged.

### `rohr_physics_joint_description_get`

```c
JointDescriptionResult rohr_physics_joint_description_get(Entity entity);
```

 Return the current owner and resolved rest values with explicit flags true.

Get-modify-set preserves the resting pose even after the bodies have moved.

### `rohr_physics_soft_body_description_default_get`

```c
SoftBodyDescription rohr_physics_soft_body_description_default_get(void);
```

 Full replacements are atomic; ownership never follows connections.

### `rohr_physics_soft_body_description_set`

```c
EngineResult rohr_physics_soft_body_description_set(Entity entity, SoftBodyDescription description);
```

 Atomically replace soft body configuration.

### `rohr_physics_soft_body_description_get`

```c
SoftBodyDescriptionResult rohr_physics_soft_body_description_get(Entity entity);
```

 Return current soft body configuration.

### `rohr_physics_soft_body_node_description_default_get`

```c
SoftBodyNodeDescription rohr_physics_soft_body_node_description_default_get(void);
```

 Full node descriptions use world positions; local creation remains explicit.

### `rohr_physics_soft_body_node_description_set`

```c
EngineResult rohr_physics_soft_body_node_description_set(Entity entity, SoftBodyNodeDescription description);
```

 Atomically replace soft body node configuration.

### `rohr_physics_soft_body_node_description_get`

```c
SoftBodyNodeDescriptionResult rohr_physics_soft_body_node_description_get(Entity entity);
```

 Return current soft body node configuration.

### `rohr_physics_soft_body_beam_description_default_get`

```c
SoftBodyBeamDescription rohr_physics_soft_body_beam_description_default_get(void);
```

 Return defaults for soft body beam configuration.

### `rohr_physics_soft_body_beam_description_set`

```c
EngineResult rohr_physics_soft_body_beam_description_set(Entity entity, SoftBodyBeamDescription description);
```

 Atomically replace soft body beam configuration.

### `rohr_physics_soft_body_beam_description_get`

```c
SoftBodyBeamDescriptionResult rohr_physics_soft_body_beam_description_get(Entity entity);
```

 Resolved rest length is returned with rest_length_explicit true.

### `rohr_physics_soft_body_beam_collision_description_default_get`

```c
SoftBodyBeamCollisionDescription rohr_physics_soft_body_beam_collision_description_default_get(void);
```

 Return defaults for soft body beam collision configuration.

### `rohr_physics_soft_body_beam_collision_description_get`

```c
SoftBodyBeamCollisionDescriptionResult rohr_physics_soft_body_beam_collision_description_get(Entity entity);
```

 Return current soft body beam collision configuration.

### `rohr_physics_soft_body_area_description_default_get`

```c
SoftBodyAreaDescription rohr_physics_soft_body_area_description_default_get(void);
```

 Return defaults for soft body area configuration.

### `rohr_physics_soft_body_area_description_set`

```c
EngineResult rohr_physics_soft_body_area_description_set(Entity entity, SoftBodyAreaDescription description);
```

 Atomically replace soft body area configuration.

### `rohr_physics_soft_body_area_description_get`

```c
SoftBodyAreaDescriptionResult rohr_physics_soft_body_area_description_get(Entity entity);
```

 Return current soft body area configuration.

### `rohr_physics_joint_anchor_description_set`

```c
EngineResult rohr_physics_joint_anchor_description_set(JointAnchorId anchor, JointAnchorDescription description);
```

 Reattach the same anchor handle and all its joint references atomically.

Keep joint owners and rest values; offsets are local to the new entity.

### `rohr_physics_joint_anchor_description_get`

```c
JointAnchorDescriptionResult rohr_physics_joint_anchor_description_get(JointAnchorId anchor);
```

 Return current joint anchor configuration.

### `rohr_physics_joint_anchor_add`

```c
JointAnchorIdResult rohr_physics_joint_anchor_add(JointAnchorDescription description);
```

Creates an anchor owned by an entity.

| Parameter | Description |
| --- | --- |
| `description` | Attached entity and offset from its stable local origin. |

**Returns:** JointAnchorIdResult containing the stable anchor handle, or an error.

### `rohr_physics_joint_anchors_get`

```c
JointAnchorListResult rohr_physics_joint_anchors_get(Entity entity);
```

Returns the anchors owned by an entity.

| Parameter | Description |
| --- | --- |
| `entity` | Entity whose anchors should be listed. |

**Returns:** JointAnchorListResult containing the anchor handles, or an error.

### `rohr_physics_joint_anchor_local_position_get`

```c
JointAnchorPositionResult rohr_physics_joint_anchor_local_position_get(JointAnchorId anchor);
```

Returns an anchor offset relative to its owner's stable local origin.

| Parameter | Description |
| --- | --- |
| `anchor` | Anchor to inspect. |

**Returns:** JointAnchorPositionResult containing the origin-relative local offset, or an error.

### `rohr_physics_joint_anchor_world_position_get`

```c
JointAnchorPositionResult rohr_physics_joint_anchor_world_position_get(JointAnchorId anchor);
```

Returns the current world position of an anchor.

| Parameter | Description |
| --- | --- |
| `anchor` | Anchor to resolve. |

**Returns:** JointAnchorPositionResult containing its world position, or an error.

### `rohr_physics_joint_anchor_local_position_set`

```c
EngineResult rohr_physics_joint_anchor_local_position_set(JointAnchorId anchor, Vec2D local_offset);
```

Sets an anchor offset relative to its owner's stable local origin.

| Parameter | Description |
| --- | --- |
| `anchor` | Anchor to modify. |
| `local_offset` | New origin-relative local offset. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_joint_anchor_remove`

```c
EngineResult rohr_physics_joint_anchor_remove(JointAnchorId anchor);
```

Removes an anchor and disconnects joint endpoints that reference it.

| Parameter | Description |
| --- | --- |
| `anchor` | Anchor to remove. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_soft_body_add`

```c
EntityResult rohr_physics_soft_body_add(SoftBodyDescription description);
```

Creates an empty soft body and its origin transform.

The returned owner entity is also the body's origin and may be passed to

rohr_physics_position_set and rohr_physics_orientation_set. Changing that

transform translates or rotates owned nodes without changing their

topology. SoftBody.origin exposes the same entity for attachment APIs.

| Parameter | Description |
| --- | --- |
| `description` | Copied owner and initial world pose; zero initializes an independent empty body at the origin. Owner may be any live entity. |

**Returns:** EntityResult containing the owner/origin entity.

### `rohr_physics_soft_body_get`

```c
SoftBodyResult rohr_physics_soft_body_get(Entity soft_body);
```

 @brief Returns soft-body topology. @param soft_body Soft-body owner. @return SoftBodyResult.

### `rohr_physics_soft_body_node_add`

```c
EntityResult rohr_physics_soft_body_node_add(SoftBodyNodeDescription description);
```

Creates a lightweight point-mass node.

| Parameter | Description |
| --- | --- |
| `description` | Copied owner, world position, positive mass and radius. An invalid owner means independent lifetime; any live owner is accepted. Omitted or zero mass/radius is invalid. Omitted flags/material/filter are zero; use the default description getter for standard node collision and material. |

**Returns:** EntityResult containing the node entity.

### `rohr_physics_soft_body_node_local_add`

```c
EntityResult rohr_physics_soft_body_node_local_add(SoftBodyNodeDescription description);
```

 Creates a node from an owner-local position. Requires a live soft-body owner;

uses its origin position and orientation exactly once.

### `rohr_physics_soft_body_node_get`

```c
SoftBodyNodeResult rohr_physics_soft_body_node_get(Entity node);
```

 @brief Returns soft-body node data. @param node Node entity. @return SoftBodyNodeResult.

### `rohr_physics_soft_body_node_local_position_get`

```c
PositionResult rohr_physics_soft_body_node_local_position_get(Entity node);
```

 Returns a node position relative to its soft-body origin.

### `rohr_physics_soft_body_node_local_position_set`

```c
EngineResult rohr_physics_soft_body_node_local_position_set( Entity node, Position local_position);
```

 Sets a node position relative to its soft-body origin.

### `rohr_physics_soft_body_node_collision_filter_description_set`

```c
EngineResult rohr_physics_soft_body_node_collision_filter_description_set(Entity node, CollisionFilterDescription description);
```

Sets node-versus-rigid collision filtering.

| Parameter | Description |
| --- | --- |
| `node` | Node entity. |
| `description` | Categories represented and accepted by the node. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_soft_body_node_collision_filter_description_get`

```c
CollisionFilterDescriptionResult rohr_physics_soft_body_node_collision_filter_description_get(Entity node);
```

 Return current soft body node collision filter configuration.

### `rohr_physics_soft_body_node_impulse_apply`

```c
EngineResult rohr_physics_soft_body_node_impulse_apply(Entity node, Vec2D impulse);
```

Applies an immediate impulse to one soft-body node.

| Parameter | Description |
| --- | --- |
| `node` | Soft-body node entity. |
| `impulse` | Impulse to apply. |

**Returns:** EngineResult describing success or failure.

### `rohr_physics_soft_body_node_to_anchor_pin_add`

```c
SoftBodyNodeAnchorPinResult rohr_physics_soft_body_node_to_anchor_pin_add(SoftBodyNodeAnchorPinDescription description);
```

Pins a soft-body node to an existing anchor.

| Parameter | Description |
| --- | --- |
| `description` | Joint owner, soft-body node, and existing target anchor. |

**Returns:** SoftBodyNodeAnchorPinResult containing the joint and node-owned anchor. Delete the joint and remove node_anchor separately to release the attachment. Removing an anchor disconnects referencing joints; it does not delete them.

### `rohr_physics_soft_body_beam_add`

```c
EntityResult rohr_physics_soft_body_beam_add(SoftBodyBeamDescription description);
```

Creates an elastic beam between two nodes.

Collision defaults off; thickness and filters are explicit and never derived

from endpoints. Use the default getter for fixed material/filter/width defaults.

| Parameter | Description |
| --- | --- |
| `description` | Copied owner, endpoints, spring, material, collision and color. Endpoints may have different owners or be ENTITY_INVALID. Rest length is measured once from complete endpoints unless rest_length_explicit is true; disconnected measurement resolves to zero. Any live owner is accepted. |

**Returns:** EntityResult containing the beam entity.

### `rohr_physics_soft_body_beam_get`

```c
SoftBodyBeamResult rohr_physics_soft_body_beam_get(Entity beam);
```

 @brief Returns soft-body beam data. @param beam Beam entity. @return SoftBodyBeamResult.

### `rohr_physics_soft_body_beam_collision_description_set`

```c
EngineResult rohr_physics_soft_body_beam_collision_description_set( Entity beam, SoftBodyBeamCollisionDescription config);
```

Atomically replaces a beam's independent collision configuration.

Thickness is stored unchanged. Enabled collision requires the engine minimum;

disabled collision permits zero. Node edits do not change these settings.

### `rohr_physics_soft_body_beam_collision_enable`

```c
EngineResult rohr_physics_soft_body_beam_collision_enable(Entity beam);
```

 Enables thick-segment collision when the stored width meets the engine minimum.

### `rohr_physics_soft_body_beam_collision_disable`

```c
EngineResult rohr_physics_soft_body_beam_collision_disable(Entity beam);
```

 Disables thick-segment collision for a beam without discarding its thickness.

### `rohr_physics_soft_body_beam_collision_thickness_set`

```c
EngineResult rohr_physics_soft_body_beam_collision_thickness_set( Entity beam, float thickness);
```

 Sets an explicit beam collision thickness without endpoint-based clamping.

### `rohr_physics_soft_body_beam_collision_filter_description_set`

```c
EngineResult rohr_physics_soft_body_beam_collision_filter_description_set(Entity beam, CollisionFilterDescription description);
```

 Sets the collision filters owned independently by a beam.

### `rohr_physics_soft_body_beam_collision_filter_description_get`

```c
CollisionFilterDescriptionResult rohr_physics_soft_body_beam_collision_filter_description_get(Entity beam);
```

 Return current soft body beam collision filter configuration.

### `rohr_physics_soft_body_area_add`

```c
EntityResult rohr_physics_soft_body_area_add(SoftBodyAreaDescription description);
```

Creates an area from an outer loop minus its owned hole loops.

Copies geometry. Complete loops contain 3..64 distinct live nodes and close

implicitly. Nodes may have other owners. Self-crossing loops are accepted

when they enclose space; incomplete loops remain stored without a fill.

Every enclosed outer region is filled once, minus all enclosed hole regions,

independent of traversal direction. Holes may extend beyond the outer loop.

More than 16 holes returns ERROR_ENGINE_MAX_AREA_HOLES_EXCEEDED.

Invalid creation creates nothing. Areas add no mass or collision.

Descriptions supply visibility and optional color; the default getter gives

visible, inherited color. New areas append to the owner list order.

Node motion updates the fill, including collapse and crossings. Delete with

rohr_entity_delete. Deleting its owner deletes the area; deleting a referenced

node clears that slot and leaves the area alive. Nodes may have other owners.

| Parameter | Description |
| --- | --- |
| `description` | Copied owner and geometry. Any live owner is accepted; ENTITY_INVALID means independent lifetime. Incomplete loops remain supported. |

### `rohr_physics_soft_body_area_get`

```c
SoftBodyAreaResult rohr_physics_soft_body_area_get(Entity area);
```

 Returns a copied geometry/style snapshot, without cache or borrowed storage.

### `rohr_physics_soft_body_area_geometry_set`

```c
EngineResult rohr_physics_soft_body_area_geometry_set(Entity area, SoftBodyAreaGeometry geometry);
```

 Replaces all loops transactionally. Invalid geometry (including more than

16 holes) preserves the previous description, style, entity and layer/order.

Full subtraction is valid; each authored loop must itself enclose space.

### `rohr_physics_soft_body_area_order_set`

```c
EngineResult rohr_physics_soft_body_area_order_set(Entity area, uint32_t index);
```

 Moves the area to a zero-based index in its owner's area list.

Later areas draw on top within the same layer. Out-of-range indices fail.

### `rohr_physics_joint_add`

```c
EntityResult rohr_physics_joint_add(JointDescription description);
```

Creates a joint borrowing two anchor handles.

| Parameter | Description |
| --- | --- |
| `description` | Lifetime owner, anchor endpoints, joint behavior, and automatic or explicit rest values. |

**Returns:** EntityResult containing the joint entity, or an error.

### `rohr_physics_particle_overlap_get`

```c
OverlapInfo rohr_physics_particle_overlap_get(Shape shape_1, Shape shape_2);
```

Gets overlap information for two particle shapes.

| Parameter | Description |
| --- | --- |
| `shape_1` | First shape. |
| `shape_2` | Second shape. |

**Returns:** Geometric overlap information.

### `rohr_physics_particle_description_default_get`

```c
ParticleDescription rohr_physics_particle_description_default_get(void);
```

Returns a valid standalone particle configuration with engine defaults.

**Returns:** ParticleDescription with a unit radius, default material and collision.

### `rohr_physics_particle_add`

```c
EntityResult rohr_physics_particle_add(ParticleDescription config);
```

Creates one translation-only particle entity.

The particle radius and local origin drive both its circle geometry and

circular polygon hitbox. A rigid_vertices value of zero uses

ROHR_PARTICLE_RIGID_VERTICES_DEFAULT.

| Parameter | Description |
| --- | --- |
| `config` | Reusable particle configuration and initial instance state. |

**Returns:** EntityResult containing the created particle, or an error.

### `rohr_physics_particle_description_set`

```c
EngineResult rohr_physics_particle_description_set(Entity entity, ParticleDescription description);
```

 Atomically replace standalone particle configuration and ownership. Static

particles retain mass, velocity, acceleration, and gravity configuration.

Changed geometry updates both circle and active polygon, retaining hitbox IDs.

### `rohr_physics_particle_description_get`

```c
ParticleDescriptionResult rohr_physics_particle_description_get(Entity entity);
```

 Get current configuration; non-standalone entities return component-missing.

### `rohr_physics_particle_origin_set`

```c
EngineResult rohr_physics_particle_origin_set(Entity entity, Position local_origin);
```

 Set the circle origin and synchronize standalone particle polygon geometry.

### `rohr_physics_particle_origin_get`

```c
PositionResult rohr_physics_particle_origin_get(Entity entity);
```

 Get a particle circle's origin relative to its rigid-body origin.

### `rohr_physics_particle_radius_set`

```c
EngineResult rohr_physics_particle_radius_set(Entity entity, float radius);
```

 Set the circle radius and synchronize standalone particle polygon geometry.

### `rohr_physics_particle_radius_get`

```c
ParticleRadiusResult rohr_physics_particle_radius_get(Entity entity);
```

 Get a particle circle's effective radius.

### `rohr_physics_overlap_check`

```c
bool rohr_physics_overlap_check(Entity entity, Entity target);
```

 Return whether two entities overlap during the current physics step.

### `rohr_physics_overlap_get`

```c
OverlapInfo rohr_physics_overlap_get(Entity entity, Entity target);
```

 Return current overlap geometry in the requested entity order.

### `rohr_physics_overlap_entered_check`

```c
bool rohr_physics_overlap_entered_check(Entity entity, Entity target);
```

 Return whether an overlap began during the current physics step.

### `rohr_physics_overlap_stayed_check`

```c
bool rohr_physics_overlap_stayed_check(Entity entity, Entity target);
```

 Return whether an overlap continued from the previous physics step.

### `rohr_physics_overlap_exited_check`

```c
bool rohr_physics_overlap_exited_check(Entity entity, Entity target);
```

 Return whether an overlap ended during the current physics step.

### `rohr_physics_overlap_count_get`

```c
size_t rohr_physics_overlap_count_get(Entity entity);
```

 Return the number of current overlaps involving an entity.

### `rohr_physics_overlaps_get`

```c
size_t rohr_physics_overlaps_get( Entity entity, EntityInteraction *results, size_t capacity );
```

 Write up to capacity current overlaps and return the number written.

### `rohr_physics_contact_check`

```c
bool rohr_physics_contact_check(Entity entity, Entity target);
```

 Return whether two entities physically contacted during the current physics step.

### `rohr_physics_contact_get`

```c
ContactInfo rohr_physics_contact_get(Entity entity, Entity target);
```

 Return current contact geometry in the requested entity order.

### `rohr_physics_contact_total_impulse_get`

```c
Vec2D rohr_physics_contact_total_impulse_get(ContactInfo contact);
```

 Return the sum of a contact's normal and friction impulses.

### `rohr_physics_contact_entered_check`

```c
bool rohr_physics_contact_entered_check(Entity entity, Entity target);
```

 Return whether a physical contact began during the current physics step.

### `rohr_physics_contact_stayed_check`

```c
bool rohr_physics_contact_stayed_check(Entity entity, Entity target);
```

 Return whether a physical contact continued from the previous physics step.

### `rohr_physics_contact_exited_check`

```c
bool rohr_physics_contact_exited_check(Entity entity, Entity target);
```

 Return whether a physical contact ended during the current physics step.

### `rohr_physics_contact_count_get`

```c
size_t rohr_physics_contact_count_get(Entity entity);
```

 Return the number of current physical contacts involving an entity.

### `rohr_physics_contacts_get`

```c
size_t rohr_physics_contacts_get( Entity entity, EntityContact *results, size_t capacity );
```

 Write up to capacity current contacts and return the number written.

## Graphics

### `rohr_graphics_color_hex_create`

```c
Color rohr_graphics_color_hex_create(uint32_t hex_color_code);
```

Creates an engine color from a hexadecimal RRGGBBAA value.

| Parameter | Description |
| --- | --- |
| `hex_color_code` | RRGGBBAA hex color value. |

**Returns:** Color created from hex_color_code.

### `rohr_graphics_start`

```c
EngineResult rohr_graphics_start(void);
```

Starts the graphics system.

**Returns:** EngineResult describing success or failure.

### `rohr_graphics_stop`

```c
void rohr_graphics_stop(void);
```

Stops the graphics service.

### `rohr_graphics_events_poll`

```c
bool rohr_graphics_events_poll(SDL_Event *event);
```

Polls graphics/window events.

| Parameter | Description |
| --- | --- |
| `event` | Destination for the SDL event. |

**Returns:** true when an event was read, false otherwise.

### `rohr_graphics_background_draw`

```c
void rohr_graphics_background_draw(Color color);
```

Draws the frame background.

| Parameter | Description |
| --- | --- |
| `color` | Background color. |

### `rohr_graphics_screen_rect_draw`

```c
bool rohr_graphics_screen_rect_draw(float x, float y, float width, float height, Color color);
```

Draws a filled rectangle in logical screen coordinates.

**Returns:** true when the draw command was queued.

### `rohr_graphics_render_output_size_get`

```c
Scale rohr_graphics_render_output_size_get(void);
```

 Returns the renderer output size in physical pixels.

### `rohr_graphics_logical_size_set`

```c
bool rohr_graphics_logical_size_set(int width, int height);
```

 Changes the logical screen size while preserving aspect-correct presentation.

### `rohr_graphics_aspect_ratio_set`

```c
bool rohr_graphics_aspect_ratio_set(int width, int height);
```

 Changes the logical aspect ratio while preserving the logical height.

### `rohr_graphics_aspect_ratio_auto_set`

```c
bool rohr_graphics_aspect_ratio_auto_set(bool enabled);
```

 Matches the logical aspect ratio to the render output when enabled.

### `rohr_graphics_window_presentation_get`

```c
GraphicsWindowPresentationConfig rohr_graphics_window_presentation_get(void);
```

 Returns the active window mode and actual physical/logical dimensions.

### `rohr_graphics_screen_clip_set`

```c
bool rohr_graphics_screen_clip_set(float x, float y, float width, float height);
```

 Clips subsequent screen-space drawing until the clip is cleared.

### `rohr_graphics_screen_clip_clear`

```c
void rohr_graphics_screen_clip_clear(void);
```

 Clears the active screen-space drawing clip.

### `rohr_graphics_screen_clip_push`

```c
bool rohr_graphics_screen_clip_push(float x, float y, float width, float height);
```

 Pushes a nested screen-space clip intersected with the active clip.

### `rohr_graphics_screen_clip_pop`

```c
void rohr_graphics_screen_clip_pop(void);
```

 Restores the previous screen-space clip.

### `rohr_graphics_screen_quad_draw`

```c
bool rohr_graphics_screen_quad_draw(Position center, float width, float height, float angle, Color color);
```

 @brief Draws a centered rotated rectangle in logical screen space.

### `rohr_graphics_screen_quad_radians_draw`

```c
bool rohr_graphics_screen_quad_radians_draw(Position center, float width, float height, float angle, Color color);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_graphics_show`

```c
Report rohr_graphics_show(void);
```

 Return a composed frame report. Recording failure still permits presentation.

Standalone order: cameras, commands, viewports, recording, presentation.

Frame pipelines must not be called recursively from camera callbacks.

### `rohr_graphics_pipeline_cameras_update`

```c
Report rohr_graphics_pipeline_cameras_update(void);
```

 Update camera motion and queue camera callback drawing.

### `rohr_graphics_pipeline_commands_execute`

```c
Report rohr_graphics_pipeline_commands_execute(void);
```

 Consume pending submission diagnostics and execute queued commands.

### `rohr_graphics_pipeline_viewports_draw`

```c
Report rohr_graphics_pipeline_viewports_draw(void);
```

 Compose screens and viewport UI after command execution.

### `rohr_graphics_pipeline_recording_update`

```c
Report rohr_graphics_pipeline_recording_update(void);
```

 Capture the composed frame; inactive recording returns NOT_RUN.

### `rohr_graphics_pipeline_present`

```c
Report rohr_graphics_pipeline_present(void);
```

 Apply frame limiting and present the composed frame.

### `rohr_graphics_vsync_set`

```c
EngineResult rohr_graphics_vsync_set(bool enabled);
```

 Enables or disables synchronization with the display refresh rate.

### `rohr_graphics_frame_limit_set`

```c
EngineResult rohr_graphics_frame_limit_set(int frames_per_second);
```

 Sets the non-VSync frame limit; zero uses the 120 FPS fallback.

### `rohr_graphics_hit_box_draw`

```c
void rohr_graphics_hit_box_draw(Entity entity, Fill fill_type);
```

Draws one entity hitbox.

| Parameter | Description |
| --- | --- |
| `entity` | Entity whose hitbox should be drawn. |
| `fill_type` | Fill mode for drawing. |

### `rohr_graphics_hit_box_colored_draw`

```c
void rohr_graphics_hit_box_colored_draw(Entity entity, Fill fill_type, Color color);
```

Draws one entity hitbox with a caller supplied color.

| Parameter | Description |
| --- | --- |
| `entity` | Entity whose hitbox should be drawn. |
| `fill_type` | Fill mode for drawing. |
| `color` | Color to draw with. |

### `rohr_graphics_hit_boxes_draw`

```c
void rohr_graphics_hit_boxes_draw(void);
```

Draws hitboxes for all renderable hitbox entities.

### `rohr_graphics_joint_draw`

```c
bool rohr_graphics_joint_draw(Entity joint, Color color);
```

Draws one joint using an engineering-style debug symbol.

| Parameter | Description |
| --- | --- |
| `joint` | Joint entity to draw. |
| `color` | Symbol color. |

**Returns:** true when the symbol's draw commands were queued.

### `rohr_graphics_joints_draw`

```c
void rohr_graphics_joints_draw(Color color);
```

Draws all live joints using engineering-style debug symbols.

| Parameter | Description |
| --- | --- |
| `color` | Symbol color. |

### `rohr_graphics_soft_body_draw`

```c
bool rohr_graphics_soft_body_draw(Entity soft_body, Color surface, Color beam, Color node);
```

Draws a soft body's current surfaces, beams, and collision nodes.

| Parameter | Description |
| --- | --- |
| `soft_body` | Soft-body owner entity. |
| `surface` | Fallback color for areas without a color override. |
| `beam` | Beam color. |
| `node` | Collision-node color. |

**Returns:** true when the soft body's draw commands were queued.

### `rohr_graphics_soft_body_area_color_set`

```c
EngineResult rohr_graphics_soft_body_area_color_set(Entity area, Color color);
```

 Overrides an area's draw color; clear restores the draw call's surface color.

### `rohr_graphics_soft_body_area_color_clear`

```c
EngineResult rohr_graphics_soft_body_area_color_clear(Entity area);
```

 Restores the draw call's surface color for this area.

### `rohr_graphics_soft_body_area_visibility_set`

```c
EngineResult rohr_graphics_soft_body_area_visibility_set(Entity area, bool visible);
```

 Hides/shows the visual area without affecting nodes, beams or collision.

### `rohr_graphics_soft_body_node_color_set`

```c
EngineResult rohr_graphics_soft_body_node_color_set( Entity soft_body, Entity node, Color color);
```

 Sets a drawing-color override for one node belonging to a soft body.

### `rohr_graphics_soft_body_beam_color_set`

```c
EngineResult rohr_graphics_soft_body_beam_color_set( Entity soft_body, Entity node_a, Entity node_b, Color color);
```

 Sets a drawing-color override for the beam connecting two soft-body nodes.

### `rohr_graphics_texture_load`

```c
TextureAssetResult rohr_graphics_texture_load(TextureDescription text_desc);
```

Loads or shares a texture and returns one caller-owned reference.

| Parameter | Description |
| --- | --- |
| `text_desc` | Texture description containing load settings. |

**Returns:** TextureAssetResult containing the asset, or an error.

### `rohr_graphics_texture_description_default_get`

```c
TextureDescription rohr_graphics_texture_description_default_get(void);
```

 Zero defaults; a file is required to load.

### `rohr_graphics_texture_description_get`

```c
TextureDescriptionResult rohr_graphics_texture_description_get(TextureAsset asset);
```

 Inspect without retaining. Paths remain borrowed until this resource

loses its final owning reference or graphics shuts down.

Logical size comes from the supplied asset value.

### `rohr_graphics_texture_retain`

```c
EngineResult rohr_graphics_texture_retain(TextureAsset asset);
```

 @brief Adds one owning reference to a loaded texture asset.

### `rohr_graphics_texture_release`

```c
EngineResult rohr_graphics_texture_release(TextureAsset *asset);
```

Releases one owning reference and clears the asset value.

Copying a TextureAsset does not retain it; retain independently owned copies.

### `rohr_graphics_texture_valid_check`

```c
bool rohr_graphics_texture_valid_check(TextureAsset asset);
```

 @brief Returns whether a texture asset names a live resource.

### `rohr_graphics_texture_draw`

```c
void rohr_graphics_texture_draw(TextureAsset texture, Position position, Orientation orientation);
```

 Draws a loaded texture centered at a world position.

### `rohr_graphics_texture_radians_draw`

```c
void rohr_graphics_texture_radians_draw(TextureAsset texture, Position position, Orientation orientation);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_graphics_screen_texture_radians_draw`

```c
void rohr_graphics_screen_texture_radians_draw(TextureAsset texture, Position center, Scale size, Orientation orientation);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_graphics_font_load`

```c
FontAssetResult rohr_graphics_font_load(FontDescription descriptor);
```

 @brief Loads or shares a font and returns one owning reference.

### `rohr_graphics_font_description_default_get`

```c
FontDescription rohr_graphics_font_description_default_get(void);
```

 Zero defaults select the engine-pinned built-in font.

### `rohr_graphics_font_description_get`

```c
FontDescriptionResult rohr_graphics_font_description_get(FontAsset asset);
```

 Inspect without retaining. Paths remain borrowed until this resource

loses its final owning reference or graphics shuts down.

Built-in inspection returns file NULL and point_size zero.

### `rohr_graphics_font_default_get`

```c
FontAsset rohr_graphics_font_default_get(void);
```

 @brief Returns the engine-pinned built-in font for this graphics lifetime.

### `rohr_graphics_font_retain`

```c
EngineResult rohr_graphics_font_retain(FontAsset font);
```

 @brief Adds one custom-font owner; built-in retains are no-ops.

### `rohr_graphics_font_release`

```c
EngineResult rohr_graphics_font_release(FontAsset *font);
```

 @brief Releases one font owner and clears the asset value.

### `rohr_graphics_font_valid_check`

```c
bool rohr_graphics_font_valid_check(FontAsset font);
```

 @brief Returns whether a font asset names a live resource.

### `rohr_graphics_text_create`

```c
TextAssetResult rohr_graphics_text_create(TextDescription description);
```

 @brief Creates reusable caller-owned text.

### `rohr_graphics_text_description_default_get`

```c
TextDescription rohr_graphics_text_description_default_get(void);
```

 Built-in font, empty string, opaque white, and no wrapping.

### `rohr_graphics_text_description_set`

```c
EngineResult rohr_graphics_text_description_set(TextAsset *text, TextDescription description);
```

 Atomic full replacement; updates dimensions without changing the handle.

Failure preserves configuration, dependencies, and queued payloads.

### `rohr_graphics_text_description_get`

```c
TextDescriptionResult rohr_graphics_text_description_get(TextAsset text);
```

 Borrowed font and string; no retain. Valid until successful changed

configuration, public text destruction, or graphics shutdown.

### `rohr_graphics_text_wrap_width_set`

```c
bool rohr_graphics_text_wrap_width_set(TextAsset *text, int wrap_width);
```

 @brief Sets text wrapping width; zero disables wrapping.

### `rohr_graphics_text_destroy`

```c
EngineResult rohr_graphics_text_destroy(TextAsset *text);
```

 @brief Destroys the unique text owner and clears its value.

### `rohr_graphics_text_valid_check`

```c
bool rohr_graphics_text_valid_check(TextAsset text);
```

 @brief Returns whether a text asset names a live public owner.

### `rohr_graphics_text_draw`

```c
bool rohr_graphics_text_draw(const TextAsset *text, Position position);
```

 @brief Draws text in logical screen coordinates.

### `rohr_graphics_screen_text_scaled_rotated_radians_draw`

```c
bool rohr_graphics_screen_text_scaled_rotated_radians_draw(const TextAsset *text, Position center, Scale scale, Orientation orientation);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_graphics_animation_load`

```c
AnimationAssetResult rohr_graphics_animation_load(AnimationDescription anim_desc);
```

Loads or shares an immutable animation resource.

The descriptor supplies image paths, stable IDs, and default timing.

Frame transforms are copied into each instance and edited after creation.

| Parameter | Description |
| --- | --- |
| `anim_desc` | Animation descriptor containing load settings. |

**Returns:** AnimationAssetResult containing the asset, or an error.

### `rohr_graphics_animation_description_default_get`

```c
AnimationDescription rohr_graphics_animation_description_default_get(void);
```

 Zero defaults create an empty catalog; zero IDs normalize on load.

### `rohr_graphics_animation_description_get`

```c
AnimationDescriptionResult rohr_graphics_animation_description_get(AnimationAsset asset);
```

 Inspect without retaining. Paths remain borrowed until this resource

loses its final owning reference or graphics shuts down.

Frame arrays are copied; paths are borrowed from retained frame textures.

### `rohr_graphics_texture_size_get`

```c
TextureSizeResult rohr_graphics_texture_size_get(TextureAsset asset);
```

 Native pixel dimensions, independent of TextureAsset.size.

### `rohr_graphics_sprite_description_default_get`

```c
SpriteDescription rohr_graphics_sprite_description_default_get(void);
```

 Scale one, facing right, visible, and following entity rotation.

### `rohr_graphics_sprite_description_set`

```c
EngineResult rohr_graphics_sprite_description_set(Entity entity, SpriteDescription description);
```

 Replace all configuration atomically; the component must already exist.

### `rohr_graphics_sprite_description_get`

```c
SpriteDescriptionResult rohr_graphics_sprite_description_get(Entity entity);
```

 Copied configuration with borrowed texture; no retain.

### `rohr_graphics_animation_frame_description_default_get`

```c
AnimationFrameDescription rohr_graphics_animation_frame_description_default_get(void);
```

 Scale one with zero ID, offset, and rotation.

### `rohr_graphics_animation_player_description_default_get`

```c
AnimationPlayerDescription rohr_graphics_animation_player_description_default_get(void);
```

 No catalog and zero timing.

### `rohr_graphics_animation_player_description_from_animation_get`

```c
AnimationPlayerDescriptionResult rohr_graphics_animation_player_description_from_animation_get(AnimationAsset animation);
```

 Borrowed catalog and its timing; zero handle selects empty defaults.

### `rohr_graphics_animation_player_description_set`

```c
EngineResult rohr_graphics_animation_player_description_set(AnimationPlayer *player, AnimationPlayerDescription description);
```

 Preserves clocks for the same catalog; a changed catalog resets to frame zero

at the last observed update clocks. No resource retain or release.

### `rohr_graphics_animation_player_description_get`

```c
AnimationPlayerDescriptionResult rohr_graphics_animation_player_description_get(const AnimationPlayer *player);
```

 Configuration only; the caller keeps the borrowed catalog alive.

### `rohr_graphics_animated_sprite_description_default_get`

```c
AnimatedSpriteDescription rohr_graphics_animated_sprite_description_default_get(void);
```

 Empty frames, scale one, facing right, visible, following entity rotation.

### `rohr_graphics_animated_sprite_description_from_animation_get`

```c
AnimatedSpriteDescriptionResult rohr_graphics_animated_sprite_description_from_animation_get(AnimationAsset animation);
```

 Copies catalog frames and timing into independent instance configuration.

Images and catalog are borrowed until catalog release or graphics shutdown.

### `rohr_graphics_animated_sprite_value_description_get`

```c
AnimatedSpriteDescription rohr_graphics_animated_sprite_value_description_get(AnimatedSprite sprite);
```

 Copied value configuration. Images/catalog remain borrowed from the value's owners.

### `rohr_graphics_animated_sprite_value_description_set`

```c
EngineResult rohr_graphics_animated_sprite_value_description_set(AnimatedSprite *sprite, AnimatedSpriteDescription description);
```

 For standalone unowned values only; validates atomically without retaining.

Preserve current frame by ID and clocks. Removed current ID or changed catalog

resets to frame zero at the last observed update clocks.

### `rohr_graphics_animated_sprite_description_set`

```c
EngineResult rohr_graphics_animated_sprite_description_set(Entity entity, AnimatedSpriteDescription description);
```

 Atomic component replacement, acquiring new references before releasing old.

Same preservation rules as value replacement; resets use engine clocks.

### `rohr_graphics_animated_sprite_description_get`

```c
AnimatedSpriteDescriptionResult rohr_graphics_animated_sprite_description_get(Entity entity);
```

 Copied frames and borrowed handles; valid until replacement/removal/deletion

or graphics shutdown. The getter never retains.

### `rohr_graphics_animation_frame_create`

```c
AnimationFrameDescription rohr_graphics_animation_frame_create(AnimationFrameDescription description);
```

 Non-owning value builder; copies fields and resolves native texture size.

It does not validate or retain; add/set validates before publication.

### `rohr_graphics_animated_sprite_value_frame_add`

```c
EngineResult rohr_graphics_animated_sprite_value_frame_add(AnimatedSprite *sprite, AnimationFrameDescription frame);
```

 Non-owning builders: copy values; keep image owners alive until component add.

Use only on unattached values, never directly on an installed component.

ID zero assigns an unused ID on add and preserves the current ID on set.

Replacing the current ID resets playback to the first frame.

### `rohr_graphics_animated_sprite_frame_description_get`

```c
AnimationFrameDescriptionResult rohr_graphics_animated_sprite_frame_description_get(Entity entity, size_t index);
```

 Get a copied instance frame with a borrowed image. Do not release its texture.

### `rohr_graphics_animated_sprite_frame_description_set`

```c
EngineResult rohr_graphics_animated_sprite_frame_description_set(Entity entity, size_t index, AnimationFrameDescription frame);
```

 Instance mutation retains images and rolls back on error. Playback is

preserved unless the current frame ID changes, which resets to frame zero.

Signed scale flips content; zero collapses the image. All transforms must be finite.

### `rohr_graphics_animation_retain`

```c
EngineResult rohr_graphics_animation_retain(AnimationAsset asset);
```

 @brief Adds one owning reference to a loaded animation asset.

### `rohr_graphics_animation_release`

```c
EngineResult rohr_graphics_animation_release(AnimationAsset *asset);
```

Releases one owning reference and clears the animation asset.

Copying an AnimationAsset does not retain it; retain independently owned

copies explicitly.

### `rohr_graphics_animation_valid_check`

```c
bool rohr_graphics_animation_valid_check(AnimationAsset asset);
```

 @brief Returns whether an animation asset names a live resource.

### `rohr_graphics_animation_info_get`

```c
AnimationInfoResult rohr_graphics_animation_info_get(AnimationAsset asset);
```

 @brief Returns immutable metadata for a loaded animation.

### `rohr_graphics_animation_frame_description_get`

```c
AnimationFrameDescriptionResult rohr_graphics_animation_frame_description_get(AnimationAsset asset, size_t frame_index);
```

 @brief Returns a reusable catalog frame with scale one and a borrowed texture.

### `rohr_graphics_animation_player_create`

```c
AnimationPlayerResult rohr_graphics_animation_player_create(AnimationPlayerDescription description);
```

Creates independent mutable playback state for an animation.

The player borrows the animation; its owner must keep the asset alive.

### `rohr_graphics_animation_player_update`

```c
void rohr_graphics_animation_player_update(AnimationPlayer *player, Tick current_tick, Time current_time);
```

 @brief Advances one animation player's frame state.

### `rohr_graphics_animated_sprite_create`

```c
AnimatedSpriteResult rohr_graphics_animated_sprite_create(AnimatedSpriteDescription description);
```

Validates a description and creates a non-owning runtime value.

| Parameter | Description |
| --- | --- |
| `description` | Complete configuration; frames are explicit. |

**Returns:** AnimatedSpriteResult; the caller keeps all borrowed resources alive.

### `rohr_graphics_animated_sprite_update`

```c
void rohr_graphics_animated_sprite_update(AnimatedSprite *sprite, Tick current_tick, Time current_time);
```

 Advance one animated sprite's frame state without attaching it to an entity.

### `rohr_graphics_animated_sprite_add`

```c
EngineResult rohr_graphics_animated_sprite_add(Entity entity, AnimatedSpriteDescription description);
```

Adds an animated sprite to an entity.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `description` | Complete configuration; compatible playback is preserved. |

**Returns:** EngineResult describing success or failure.

### `rohr_graphics_sprite_orientation_offset_radians_set`

```c
EngineResult rohr_graphics_sprite_orientation_offset_radians_set(Entity entity, Orientation offset);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_graphics_sprite_orientation_offset_radians_get`

```c
SpriteOrientationResult rohr_graphics_sprite_orientation_offset_radians_get(Entity entity);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_graphics_animated_sprite_orientation_offset_radians_set`

```c
EngineResult rohr_graphics_animated_sprite_orientation_offset_radians_set(Entity entity, Orientation offset);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_graphics_animated_sprite_orientation_offset_radians_get`

```c
SpriteOrientationResult rohr_graphics_animated_sprite_orientation_offset_radians_get( Entity entity);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_graphics_animated_sprite_draw`

```c
bool rohr_graphics_animated_sprite_draw(Entity entity);
```

 Draw the animated sprite attached to one entity.

### `rohr_graphics_animated_sprites_draw`

```c
void rohr_graphics_animated_sprites_draw(void);
```

Draws all animated sprite components.

### `rohr_graphics_sprite_frames_update`

```c
Report rohr_graphics_sprite_frames_update(Tick current_tick, Time current_time);
```

Updates animated sprite frames.

| Parameter | Description |
| --- | --- |
| `current_tick` | Current engine tick. |
| `current_time` | Current engine time. |

### `rohr_graphics_textures_scale`

```c
void rohr_graphics_textures_scale(Entity entity, Scale scale);
```

Scales textures attached to an entity.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to modify. |
| `scale` | Scale value. |

### `rohr_graphics_camera_move`

```c
void rohr_graphics_camera_move(Vec2D translation);
```

Translates the active camera in world space.

| Parameter | Description |
| --- | --- |
| `translation` | World-space translation to add. |

### `rohr_graphics_camera_rotate`

```c
void rohr_graphics_camera_rotate(Orientation degrees);
```

Rotates the active camera clockwise.

| Parameter | Description |
| --- | --- |
| `degrees` | Clockwise rotation in degrees to add. |

### `rohr_graphics_camera_radians_rotate`

```c
void rohr_graphics_camera_radians_rotate(Orientation radians);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_graphics_camera_attach`

```c
EngineResult rohr_graphics_camera_attach( Entity entity, Vec2D position_offset, Orientation orientation_offset );
```

Attaches the camera to an entity's position and orientation.

The position offset is in the entity's local space and rotates with the

entity. The orientation offset is added to the entity's orientation.

| Parameter | Description |
| --- | --- |
| `entity` | Entity transform to follow. |
| `position_offset` | Local-space position offset. |
| `orientation_offset` | orientation offset in clockwise degrees. |

**Returns:** EngineResult describing success or a missing transform.

### `rohr_graphics_camera_radians_attach`

```c
EngineResult rohr_graphics_camera_radians_attach( Entity entity, Vec2D position_offset, Orientation orientation_offset );
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_graphics_camera_with_options_attach`

```c
EngineResult rohr_graphics_camera_with_options_attach( Entity entity, Vec2D position_offset, Orientation orientation_offset, bool follow_position, bool follow_orientation );
```

Attaches a camera with independent transform inheritance.

When orientation following is disabled, position_offset is world-space.

When position following is disabled, position_offset is the fixed camera

position. When orientation following is disabled, orientation_offset is the

fixed camera orientation.

| Parameter | Description |
| --- | --- |
| `entity` | Entity to associate with the camera. |
| `position_offset` | Relative offset or fixed world position. |
| `orientation_offset` | Relative or fixed clockwise orientation in degrees. |
| `follow_position` | Whether to inherit entity position. |
| `follow_orientation` | Whether to inherit entity orientation. |

**Returns:** EngineResult describing success or a missing required transform.

### `rohr_graphics_camera_with_options_radians_attach`

```c
EngineResult rohr_graphics_camera_with_options_radians_attach( Entity entity, Vec2D position_offset, Orientation orientation_offset, bool follow_position, bool follow_orientation );
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_graphics_camera_detach`

```c
void rohr_graphics_camera_detach(void);
```

Detaches the camera and preserves its current world transform.

### `rohr_graphics_camera_attached_get`

```c
bool rohr_graphics_camera_attached_get(void);
```

Reports whether the camera is attached to a live entity transform.

**Returns:** true when attached to a valid entity transform.

### `rohr_graphics_camera_attachment_get`

```c
CameraAttachmentResult rohr_graphics_camera_attachment_get(void);
```

Returns the active camera attachment description.

**Returns:** CameraAttachmentResult containing the attachment or a missing-component error.

### `rohr_graphics_world_to_screen_get`

```c
Position rohr_graphics_world_to_screen_get(Position pos);
```

Converts a world position to screen coordinates.

| Parameter | Description |
| --- | --- |
| `pos` | World position. |

**Returns:** Screen-space position.

### `rohr_graphics_screen_to_world_get`

```c
Position rohr_graphics_screen_to_world_get(Position screen);
```

Converts a screen position to world coordinates.

| Parameter | Description |
| --- | --- |
| `screen` | Screen-space position. |

**Returns:** World-space position.

### `rohr_graphics_mouse_screen_position_get`

```c
Position rohr_graphics_mouse_screen_position_get(void);
```

Returns the current mouse position in screen coordinates.

**Returns:** Mouse screen position.

### `rohr_graphics_aabb_tree_debug_set`

```c
void rohr_graphics_aabb_tree_debug_set(bool enabled);
```

 Enable or disable physics AABB-tree debug drawing.

### `rohr_graphics_aabb_tree_debug_check`

```c
bool rohr_graphics_aabb_tree_debug_check(void);
```

 Return whether physics AABB-tree debug drawing is enabled.

### `rohr_graphics_aabb_tree_draw`

```c
void rohr_graphics_aabb_tree_draw(void);
```

 Draw the current physics AABB-tree bounds when debug drawing is enabled.

### `rohr_graphics_contacts_debug_set`

```c
void rohr_graphics_contacts_debug_set(bool enabled);
```

 Enable or disable current-contact manifold debug drawing.

### `rohr_graphics_contacts_debug_check`

```c
bool rohr_graphics_contacts_debug_check(void);
```

 Return whether current-contact manifold debug drawing is enabled.

### `rohr_graphics_contacts_draw`

```c
void rohr_graphics_contacts_draw(void);
```

 Draw current contact manifold points and first-to-second normals when enabled.

### `rohr_graphics_recording_start`

```c
bool rohr_graphics_recording_start(const char *output_path, int fps);
```

Starts recording rendered frames to a video file.

| Parameter | Description |
| --- | --- |
| `output_path` | Path where the recording should be written. |
| `fps` | Recording frame rate. |

**Returns:** true when recording starts successfully, false otherwise.

### `rohr_graphics_recording_stop`

```c
void rohr_graphics_recording_stop(void);
```

Stops recording rendered frames.

### `rohr_graphics_particle_draw`

```c
void rohr_graphics_particle_draw(Entity entity, Fill fill_type);
```

Draws one particle entity using its collision circle.

| Parameter | Description |
| --- | --- |
| `entity` | Particle entity to draw. |
| `fill_type` | Whether to draw the circle filled or outlined. |

### `rohr_graphics_particles_draw`

```c
void rohr_graphics_particles_draw(void);
```

Draws active particle components.

### `rohr_graphics_local_origins_draw`

```c
void rohr_graphics_local_origins_draw(void);
```

Draws local origin markers for entities.

## Math

### `rohr_math_normals_create`

```c
Vec2DList rohr_math_normals_create(Shape shape);
```

Creates normalized edge normals for a shape.

| Parameter | Description |
| --- | --- |
| `shape` | Shape to inspect. |

**Returns:** List of normal vectors.

### `rohr_math_vector_normalize`

```c
Vec2D rohr_math_vector_normalize(Vec2D vector);
```

Normalizes a vector.

| Parameter | Description |
| --- | --- |
| `vector` | Vector to normalize. |

**Returns:** Normalized vector.

### `rohr_math_vectors_normalize`

```c
Vec2DList rohr_math_vectors_normalize(Vec2DList vectors);
```

Normalizes all vectors in a list.

| Parameter | Description |
| --- | --- |
| `vectors` | Vector list to normalize. |

**Returns:** Normalized vector list.

### `rohr_math_dot_product`

```c
float rohr_math_dot_product(Vec2D vector_1, Vec2D vector_2);
```

Calculates the dot product of two vectors.

| Parameter | Description |
| --- | --- |
| `vector_1` | First vector. |
| `vector_2` | Second vector. |

**Returns:** Dot product.

### `rohr_math_square_create`

```c
Shape rohr_math_square_create(float width, float height);
```

Creates a rectangular polygon shape.

| Parameter | Description |
| --- | --- |
| `width` | Rectangle width. |
| `height` | Rectangle height. |

**Returns:** Shape containing rectangle vertices.

### `rohr_math_circle_create`

```c
Shape rohr_math_circle_create(float radius, uint8_t verticies);
```

Creates a circle approximation shape.

| Parameter | Description |
| --- | --- |
| `radius` | Circle radius. |
| `verticies` | Number of vertices used to approximate the circle. |

**Returns:** Shape containing circle vertices.

### `rohr_math_project_shape_on_axis`

```c
Projection rohr_math_project_shape_on_axis(Shape shape, Axis axis);
```

Projects a shape onto an axis.

| Parameter | Description |
| --- | --- |
| `shape` | Shape to project. |
| `axis` | Axis to project onto. |

**Returns:** Projection interval.

### `rohr_math_cross_2d`

```c
float rohr_math_cross_2d(Vec2D a, Vec2D b);
```

Calculates the scalar 2D cross product.

| Parameter | Description |
| --- | --- |
| `a` | First vector. |
| `b` | Second vector. |

**Returns:** Cross product value.

### `rohr_math_angular_velocity_cross_vec`

```c
Vec2D rohr_math_angular_velocity_cross_vec(float omega, Vec2D r);
```

Calculates angular velocity crossed with a vector.

| Parameter | Description |
| --- | --- |
| `omega` | Angular velocity. |
| `r` | Radius or offset vector. |

**Returns:** Tangential velocity vector.

### `rohr_math_angular_velocity_radians_cross_vec`

```c
Vec2D rohr_math_angular_velocity_radians_cross_vec(float omega, Vec2D r);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_math_degrees_to_radians`

```c
float rohr_math_degrees_to_radians(float degrees);
```

 Convert degrees to radians without sign changes or wrapping.

### `rohr_math_radians_to_degrees`

```c
float rohr_math_radians_to_degrees(float radians);
```

 Convert radians to degrees without sign changes or wrapping.

### `rohr_math_project_onto_axis`

```c
Vec2D rohr_math_project_onto_axis(Vec2D v, Axis axis);
```

Projects a vector onto an axis.

| Parameter | Description |
| --- | --- |
| `v` | Vector to project. |
| `axis` | Axis to project onto. |

**Returns:** Projected vector.

### `rohr_math_axis_magnitude`

```c
float rohr_math_axis_magnitude(Axis axis);
```

Calculates axis magnitude.

| Parameter | Description |
| --- | --- |
| `axis` | Axis vector. |

**Returns:** Magnitude.

### `rohr_math_vector_magnitude`

```c
float rohr_math_vector_magnitude(Vec2D vector);
```

Calculates vector magnitude.

| Parameter | Description |
| --- | --- |
| `vector` | Vector to inspect. |

**Returns:** Magnitude.

### `rohr_math_vector_rotate`

```c
Vec2D rohr_math_vector_rotate(Vec2D vector, float angle);
```

Rotates a vector by an angle.

| Parameter | Description |
| --- | --- |
| `vector` | Vector to rotate. |
| `angle` | Clockwise angle in degrees. |

**Returns:** Rotated vector.

### `rohr_math_vector_radians_rotate`

```c
Vec2D rohr_math_vector_radians_rotate(Vec2D vector, float angle);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_math_circle_radius`

```c
Vec1D rohr_math_circle_radius(Shape circle, Vec2D centroid);
```

Calculates a circle radius from its shape and centroid.

| Parameter | Description |
| --- | --- |
| `circle` | Circle shape. |
| `centroid` | Circle centroid. |

**Returns:** Circle radius.

### `rohr_math_vector_subtract`

```c
Vec2D rohr_math_vector_subtract(Vec2D vector_a, Vec2D vector_b);
```

Subtracts one vector from another.

| Parameter | Description |
| --- | --- |
| `vector_a` | Vector to subtract from. |
| `vector_b` | Vector to subtract. |

**Returns:** vector_a minus vector_b.

### `rohr_math_circle_overlap_depth`

```c
Vec1D rohr_math_circle_overlap_depth(Vec2D centroid_1, Vec1D radius_1, Vec2D centroid_2, Vec1D radius_2);
```

Calculates overlap depth between two circles.

| Parameter | Description |
| --- | --- |
| `centroid_1` | First circle centroid. |
| `radius_1` | First circle radius. |
| `centroid_2` | Second circle centroid. |
| `radius_2` | Second circle radius. |

**Returns:** Circle overlap depth.

### `rohr_math_projection_overlap`

```c
float rohr_math_projection_overlap(Projection projection_1, Projection projection_2);
```

Calculates overlap between two projection intervals.

| Parameter | Description |
| --- | --- |
| `projection_1` | First projection. |
| `projection_2` | Second projection. |

**Returns:** Overlap depth.

### `rohr_math_shape_scale`

```c
Shape rohr_math_shape_scale(Shape shape, float scale);
```

Scales a shape uniformly.

| Parameter | Description |
| --- | --- |
| `shape` | Shape to scale. |
| `scale` | Uniform scale value. |

**Returns:** Scaled shape.

### `rohr_math_shape_y_scale`

```c
Shape rohr_math_shape_y_scale(Shape shape, float scale);
```

Scales a shape along the y axis.

| Parameter | Description |
| --- | --- |
| `shape` | Shape to scale. |
| `scale` | Y-axis scale value. |

**Returns:** Scaled shape.

### `rohr_math_shape_x_scale`

```c
Shape rohr_math_shape_x_scale(Shape shape, float scale);
```

Scales a shape along the x axis.

| Parameter | Description |
| --- | --- |
| `shape` | Shape to scale. |
| `scale` | X-axis scale value. |

**Returns:** Scaled shape.

### `rohr_math_polygon_centroid`

```c
Vec2D rohr_math_polygon_centroid(Shape shape);
```

Calculates a polygon centroid.

| Parameter | Description |
| --- | --- |
| `shape` | Polygon shape. |

**Returns:** Centroid position.

### `rohr_math_vertex_add`

```c
Shape rohr_math_vertex_add(Shape shape);
```

Adds a vertex slot to a shape.

| Parameter | Description |
| --- | --- |
| `shape` | Shape to modify. |

**Returns:** Shape with an additional vertex slot.

### `rohr_math_vertex_delete`

```c
Shape rohr_math_vertex_delete(Shape shape);
```

Deletes the last vertex slot from a shape.

| Parameter | Description |
| --- | --- |
| `shape` | Shape to modify. |

**Returns:** Shape with one fewer vertex slot.

### `rohr_math_aabb_create`

```c
AABB rohr_math_aabb_create(Shape world_shape);
```

Creates an axis-aligned bounding box for a world-space shape.

| Parameter | Description |
| --- | --- |
| `world_shape` | World-space shape. |

**Returns:** Axis-aligned bounding box.

## Systems

### `rohr_system_physics_update`

```c
Report rohr_system_physics_update(double dt);
```

Runs one physics-system update.

| Parameter | Description |
| --- | --- |
| `dt` | Simulation delta time in seconds. |

## Input

### `rohr_input_frame_begin`

```c
void rohr_input_frame_begin(void);
```

 Clear transient input values before polling events for a new frame.

### `rohr_input_key_down_check`

```c
bool rohr_input_key_down_check(SDL_Scancode key);
```

 Return whether a physical key is currently held.

### `rohr_input_key_pressed_check`

```c
bool rohr_input_key_pressed_check(SDL_Scancode key);
```

 Return whether a physical key entered the held state this frame.

### `rohr_input_key_released_check`

```c
bool rohr_input_key_released_check(SDL_Scancode key);
```

 Return whether a physical key left the held state this frame.

### `rohr_input_modifiers_get`

```c
SDL_Keymod rohr_input_modifiers_get(void);
```

 Return the modifier state captured from the latest keyboard event.

### `rohr_input_mouse_button_down_check`

```c
bool rohr_input_mouse_button_down_check(InputMouseButton button);
```

 Return whether a mouse button is currently held.

### `rohr_input_mouse_button_pressed_check`

```c
bool rohr_input_mouse_button_pressed_check(InputMouseButton button);
```

 Return whether a mouse button entered the held state this frame.

### `rohr_input_mouse_button_released_check`

```c
bool rohr_input_mouse_button_released_check(InputMouseButton button);
```

 Return whether a mouse button left the held state this frame.

### `rohr_input_mouse_position_get`

```c
Vec2D rohr_input_mouse_position_get(void);
```

 Return the latest absolute pointer position in window coordinates.

### `rohr_input_mouse_delta_get`

```c
Vec2D rohr_input_mouse_delta_get(void);
```

 Return accumulated pointer movement since rohr_input_frame_begin.

### `rohr_input_mouse_wheel_get`

```c
Vec2D rohr_input_mouse_wheel_get(void);
```

 Return accumulated normalized wheel movement for the current frame.

### `rohr_input_mouse_relative_mode_set`

```c
EngineResult rohr_input_mouse_relative_mode_set(bool enabled);
```

 Enable or disable relative pointer mode on the focused input window.

### `rohr_input_mouse_relative_mode_check`

```c
bool rohr_input_mouse_relative_mode_check(void);
```

 Return whether the focused input window uses relative pointer mode.

### `rohr_input_text_start`

```c
EngineResult rohr_input_text_start(void);
```

 Begin SDL UTF-8 and IME text input on the focused keyboard window.

### `rohr_input_text_stop`

```c
EngineResult rohr_input_text_stop(void);
```

 Stop SDL text input and clear its active composition and candidates.

### `rohr_input_text_area_set`

```c
EngineResult rohr_input_text_area_set(SDL_Rect area, int cursor);
```

 Set the on-screen text field and cursor location used to place an IME.

### `rohr_input_text_state_get`

```c
InputTextState rohr_input_text_state_get(void);
```

 Return a copied snapshot of committed text, composition, and IME candidates.

### `rohr_input_binding_valid_check`

```c
bool rohr_input_binding_valid_check(InputActionType type, const InputBinding *binding);
```

 Return whether a tagged physical binding is valid for an action type.

### `rohr_input_binding_key_create`

```c
InputBinding rohr_input_binding_key_create(SDL_Keycode key);
```

 Create a key binding from an SDL keycode, including required Shift state.

### `rohr_input_binding_scancode_create`

```c
InputBinding rohr_input_binding_scancode_create(SDL_Scancode key);
```

 Create a physical-layout key binding directly from an SDL scancode.

### `rohr_input_controller_create`

```c
InputControllerIdResult rohr_input_controller_create(const char *name);
```

 Create an enabled, engine-owned logical controller with a unique name.

### `rohr_input_controller_destroy`

```c
EngineResult rohr_input_controller_destroy(InputControllerId controller);
```

 Destroy a logical controller and all actions owned by it.

### `rohr_input_controller_by_name_get`

```c
InputControllerIdResult rohr_input_controller_by_name_get(const char *name);
```

 Find a logical controller by its unique name.

### `rohr_input_controller_enabled_set`

```c
EngineResult rohr_input_controller_enabled_set(InputControllerId controller, bool enabled);
```

 Enable or disable a controller without changing its action state.

### `rohr_input_controller_enabled_check`

```c
bool rohr_input_controller_enabled_check(InputControllerId controller);
```

 Return whether a valid logical controller is enabled.

### `rohr_input_action_create`

```c
InputActionIdResult rohr_input_action_create(InputControllerId controller, const char *name, InputActionType type);
```

 Create an engine-owned typed action with a controller-local unique name.

### `rohr_input_action_destroy`

```c
EngineResult rohr_input_action_destroy(InputActionId action);
```

 Destroy an action while leaving its owning controller intact.

### `rohr_input_action_by_name_get`

```c
InputActionIdResult rohr_input_action_by_name_get(InputControllerId controller, const char *name);
```

 Find an action by its controller and controller-local name.

### `rohr_input_action_type_get`

```c
InputActionTypeResult rohr_input_action_type_get(InputActionId action);
```

 Return the logical value type declared for an action.

### `rohr_input_action_button_mode_set`

```c
EngineResult rohr_input_action_button_mode_set(InputActionId action, InputButtonMode mode);
```

 Replace a Button action's momentary or persistent behavior.

### `rohr_input_action_button_mode_get`

```c
InputButtonModeResult rohr_input_action_button_mode_get(InputActionId action);
```

 Return a Button action's configured behavior.

### `rohr_input_action_button_initial_state_set`

```c
EngineResult rohr_input_action_button_initial_state_set(InputActionId action, bool state);
```

 Set the state restored by a persistent Button reset.

### `rohr_input_action_button_initial_state_check`

```c
bool rohr_input_action_button_initial_state_check(InputActionId action);
```

 Return a Button action's authored initial state.

### `rohr_input_action_button_state_set`

```c
EngineResult rohr_input_action_button_state_set(InputActionId action, bool state);
```

 Explicitly replace a persistent Button action's logical state.

### `rohr_input_action_button_state_reset`

```c
EngineResult rohr_input_action_button_state_reset(InputActionId action);
```

 Restore a persistent Button action to its authored initial state.

### `rohr_input_action_bindings_default_set`

```c
EngineResult rohr_input_action_bindings_default_set(InputActionId action, const InputBinding *bindings, size_t count);
```

 Replace copied defaults; non-empty names must be unique within the list.

### `rohr_input_action_bindings_default_get`

```c
InputBindingListResult rohr_input_action_bindings_default_get(InputActionId action);
```

 Return a copy of an action's default bindings.

### `rohr_input_action_bindings_override_set`

```c
EngineResult rohr_input_action_bindings_override_set(InputActionId action, const InputBinding *bindings, size_t count);
```

 Replace copied overrides; non-empty names must be unique within the list.

### `rohr_input_action_bindings_override_get`

```c
InputBindingListResult rohr_input_action_bindings_override_get(InputActionId action);
```

 Return a copy of an action's runtime override bindings.

### `rohr_input_action_bindings_effective_get`

```c
InputBindingListResult rohr_input_action_bindings_effective_get(InputActionId action);
```

 Return a copy of the active override bindings, or defaults when none exist.

### `rohr_input_action_bindings_override_clear`

```c
EngineResult rohr_input_action_bindings_override_clear(InputActionId action);
```

 Clear runtime overrides so the action uses its defaults again.

### `rohr_input_action_bindings_override_check`

```c
bool rohr_input_action_bindings_override_check(InputActionId action);
```

 Return whether runtime override bindings are active for an action.

### `rohr_input_action_button_down_check`

```c
bool rohr_input_action_button_down_check(InputActionId action);
```

 Return whether a Button action is currently held.

### `rohr_input_action_button_pressed_check`

```c
bool rohr_input_action_button_pressed_check(InputActionId action);
```

 Return whether a Button action entered the held state this frame.

### `rohr_input_action_button_released_check`

```c
bool rohr_input_action_button_released_check(InputActionId action);
```

 Return whether a Button action left the held state this frame.

### `rohr_input_action_axis_1d_get`

```c
InputAxis1DResult rohr_input_action_axis_1d_get(InputActionId action);
```

 Return a combined and clamped Axis 1D action value.

### `rohr_input_action_axis_2d_get`

```c
InputAxis2DResult rohr_input_action_axis_2d_get(InputActionId action);
```

 Return a combined and unit-length-clamped Axis 2D action value.

## Tools

### `rohr_tools_delay`

```c
void rohr_tools_delay(int seconds);
```

Delays execution for a number of seconds.

| Parameter | Description |
| --- | --- |
| `seconds` | Number of seconds to delay. |

### `rohr_tools_binary_to_string`

```c
void rohr_tools_binary_to_string(uint32_t value, char *buffer, size_t size);
```

Writes a binary string representation of a value.

| Parameter | Description |
| --- | --- |
| `value` | Value to convert. |
| `buffer` | Destination buffer. |
| `size` | Size of buffer in bytes. |

### `rohr_tools_append_string`

```c
void rohr_tools_append_string(char *src, char *dst, size_t src_size, size_t dst_size);
```

Appends one string to another using explicit buffer sizes.

| Parameter | Description |
| --- | --- |
| `src` | Source string. |
| `dst` | Destination string. |
| `src_size` | Source buffer size. |
| `dst_size` | Destination buffer size. |

### `rohr_tools_sizeof_string`

```c
uint32_t rohr_tools_sizeof_string(char *str, char delimiter);
```

Counts characters in a string until a delimiter.

| Parameter | Description |
| --- | --- |
| `str` | String to inspect. |
| `delimiter` | Delimiter that stops counting. |

**Returns:** Number of characters before delimiter.

### `rohr_tools_random_range`

```c
int rohr_tools_random_range(int min, int max);
```

Returns a random integer in a range.

| Parameter | Description |
| --- | --- |
| `min` | Minimum value. |
| `max` | Maximum value. |

**Returns:** Random integer between min and max.

### `rohr_tools_random_range_float`

```c
float rohr_tools_random_range_float(float min, float max);
```

Returns a random float in a range.

| Parameter | Description |
| --- | --- |
| `min` | Minimum value. |
| `max` | Maximum value. |

**Returns:** Random float between min and max.

## Other

### `rohr_report_create`

```c
Report rohr_report_create(void);
```

 Returns an empty caller-owned report builder, independent of engine startup.

### `rohr_report_get`

```c
ReportResult rohr_report_get(ReportFlags flags);
```

 Return an owned snapshot or compose selectors joined with |. Zero and

unsupported bits return an error; ROHR_REPORT selects all supported reports.

### `rohr_report_valid_check`

```c
bool rohr_report_valid_check(const Report *report);
```

 Checks populated report fields, custom IDs, and reserved-fatal consistency.

### `rohr_report_header_set`

```c
EngineResult rohr_report_header_set(Report *report, ReportHeader header);
```

 Sets report identity, completion status, and measured time; failure is sticky.

### `rohr_report_entry_add`

```c
EngineResult rohr_report_entry_add(Report *report, ReportEntry entry);
```

 Adds/aggregates a child summary without changing the parent's elapsed time.

### `rohr_report_statistic_add`

```c
EngineResult rohr_report_statistic_add(Report *report, ReportStatistic statistic);
```

 Adds a statistic using its validated sum/max/latest aggregation rule.

### `rohr_report_diagnostic_detail_set`

```c
EngineResult rohr_report_diagnostic_detail_set(ReportDiagnostic *diagnostic, const char *detail);
```

 Copies optional detail text; NULL clears, oversized text sets detail_truncated.

### `rohr_report_diagnostic_add`

```c
EngineResult rohr_report_diagnostic_add(Report *report, ReportDiagnostic diagnostic);
```

 Adds an issue, aggregating repeats and preserving the first fatal when full.

### `rohr_report_merge`

```c
EngineResult rohr_report_merge(Report *destination, const Report *source);
```

Combines a child report and its descendants without allocation or simulation.

Preserves source values and destination identity/time. First fatal and latest

metrics follow merge order. Malformed inputs, self-merge, or incompatible

metric rules fail without mutation. Capacity exhaustion succeeds with visible

loss counts; the reserved first fatal is not dropped. Merge each child once.

### `rohr_report_console_write`

```c
EngineResult rohr_report_console_write(const Report *report);
```

 Write a validated report through the console; no engine startup required.

### `rohr_report_json_write`

```c
EngineResult rohr_report_json_write(const Report *report, const char *path, bool pretty);
```

 Replace a file with versioned JSON; pretty changes whitespace only.

Uses temporary allocation. Invalid reports/paths fail before opening the file.

An I/O failure can leave a partial file. The source report is never modified.

### `rohr_report_text_write`

```c
EngineResult rohr_report_text_write(const Report *report, const char *path);
```

 Replace a file with readable report text, without console prefixes.

Does not create directories. An I/O failure can leave a partial file.

### `rohr_audio_start`

```c
EngineResult rohr_audio_start(void);
```

Start optional callback-driven audio. Runtime controls enqueue requests;

call rohr_audio_update to submit them. Concurrent submissions are serialized

in enqueue order; getters observe applied state, not pending requests.

Creation, destruction, startup and shutdown remain synchronous. Callers

synchronize their own handle variables; see docs/audio.md for lifetime rules.

### `rohr_audio_stop`

```c
Report rohr_audio_stop(void);
```

 Stop synchronously and return final evidence without consuming it.

The next update returns the same report once. Restart discards that copy.

### `rohr_audio_update`

```c
Report rohr_audio_update(void);
```

 Submit pending controls and collect completed work; never waits for playback.

### `rohr_audio_started_check`

```c
bool rohr_audio_started_check(void);
```

 Return whether the audio service is currently started.

### `rohr_audio_volume_set`

```c
EngineResult rohr_audio_volume_set(float volume);
```

 Set the shared mixer volume, clamped to 0..1.

### `rohr_audio_volume_get`

```c
float rohr_audio_volume_get(void);
```

 Return the shared mixer volume.

### `rohr_audio_sound_config_default_get`

```c
SoundConfig rohr_audio_sound_config_default_get(void);
```

 Return defaults for one WAV sound playback instance.

### `rohr_audio_sound_create`

```c
SoundResult rohr_audio_sound_create(SoundConfig config);
```

 Load a WAV and create one independently controlled playback instance.

### `rohr_audio_sound_destroy`

```c
EngineResult rohr_audio_sound_destroy(Sound *sound);
```

 Stop and release one sound owner, clearing it on success.

### `rohr_audio_sound_valid_check`

```c
bool rohr_audio_sound_valid_check(Sound sound);
```

 Return whether a sound player handle is currently valid.

### `rohr_audio_sound_play`

```c
EngineResult rohr_audio_sound_play(Sound sound);
```

 Restart one sound from its beginning.

### `rohr_audio_sound_stop`

```c
EngineResult rohr_audio_sound_stop(Sound sound);
```

 Stop one sound and reset it to its beginning.

### `rohr_audio_sound_volume_set`

```c
EngineResult rohr_audio_sound_volume_set(Sound sound, float volume);
```

 Set one sound's volume, clamped to 0..1.

### `rohr_audio_sound_volume_get`

```c
AudioValueResult rohr_audio_sound_volume_get(Sound sound);
```

 Return one sound's stored volume, or an error for an invalid handle.

### `rohr_audio_sound_pan_set`

```c
EngineResult rohr_audio_sound_pan_set(Sound sound, float pan);
```

 Set one sound's stereo pan, clamped to -1..1.

### `rohr_audio_sound_pan_get`

```c
AudioValueResult rohr_audio_sound_pan_get(Sound sound);
```

 Return one sound's stored stereo pan.

### `rohr_audio_sound_loop_set`

```c
EngineResult rohr_audio_sound_loop_set(Sound sound, bool loop);
```

 Enable or disable restarting the sound when it reaches its end.

### `rohr_audio_sound_loop_check`

```c
bool rohr_audio_sound_loop_check(Sound sound);
```

 Return whether one sound is configured to loop.

### `rohr_audio_sound_playback_rate_set`

```c
EngineResult rohr_audio_sound_playback_rate_set( Sound sound, float playback_rate );
```

Set one sound's speed and pitch ratio.

Zero freezes playback and negative values are normalized to zero.

### `rohr_audio_sound_playback_rate_get`

```c
AudioValueResult rohr_audio_sound_playback_rate_get(Sound sound);
```

 Return one sound's normalized speed and pitch ratio.

### `rohr_audio_sound_playing_check`

```c
bool rohr_audio_sound_playing_check(Sound sound);
```

 Return true only while the sound is actively advancing.

### `rohr_audio_music_config_default_get`

```c
MusicConfig rohr_audio_music_config_default_get(void);
```

 Return defaults for one streamed Ogg Vorbis track.

### `rohr_audio_music_create`

```c
MusicResult rohr_audio_music_create(MusicConfig config);
```

 Open an Ogg Vorbis file for incremental decoding during playback.

### `rohr_audio_music_destroy`

```c
EngineResult rohr_audio_music_destroy(Music *music);
```

 Stop and release one music owner, clearing it on success.

### `rohr_audio_music_valid_check`

```c
bool rohr_audio_music_valid_check(Music music);
```

 Return whether a streamed music handle is currently valid.

### `rohr_audio_music_play`

```c
EngineResult rohr_audio_music_play(Music music);
```

 Start one track from the beginning, replacing any active track.

### `rohr_audio_music_pause`

```c
EngineResult rohr_audio_music_pause(Music music);
```

 Pause the active track without changing its playback position.

### `rohr_audio_music_resume`

```c
EngineResult rohr_audio_music_resume(Music music);
```

 Resume an explicitly paused active track.

### `rohr_audio_music_stop`

```c
EngineResult rohr_audio_music_stop(Music music);
```

 Stop one track and reset it to the beginning.

### `rohr_audio_music_volume_set`

```c
EngineResult rohr_audio_music_volume_set(Music music, float volume);
```

 Set one music track's volume, clamped to 0..1.

### `rohr_audio_music_volume_get`

```c
AudioValueResult rohr_audio_music_volume_get(Music music);
```

 Return one music track's stored volume.

### `rohr_audio_music_loop_set`

```c
EngineResult rohr_audio_music_loop_set(Music music, bool loop);
```

 Enable or disable restarting the track when it reaches its end.

### `rohr_audio_music_loop_check`

```c
bool rohr_audio_music_loop_check(Music music);
```

 Return whether one music track is configured to loop.

### `rohr_audio_music_playback_rate_set`

```c
EngineResult rohr_audio_music_playback_rate_set( Music music, float playback_rate );
```

 Set speed and pitch; zero freezes and negative values normalize to zero.

### `rohr_audio_music_playback_rate_get`

```c
AudioValueResult rohr_audio_music_playback_rate_get(Music music);
```

 Return one music track's normalized speed and pitch ratio.

### `rohr_audio_music_playing_check`

```c
bool rohr_audio_music_playing_check(Music music);
```

 Return true only while the active track is advancing.

### `rohr_audio_music_paused_check`

```c
bool rohr_audio_music_paused_check(Music music);
```

 Return whether the active track was explicitly paused.

### `rohr_directory_base_get`

```c
const char *rohr_directory_base_get(void);
```

Returns the platform application base directory.

The returned path is SDL-owned and must not be freed.

### `rohr_directory_working_set`

```c
EngineResult rohr_directory_working_set(const char *directory);
```

Sets the process-wide working directory to a non-empty path.

Returns an error without changing the directory when the path is invalid or

the platform rejects it.

### `rohr_game_state_file_load`

```c
EngineResult rohr_game_state_file_load(const char *path);
```

 Loads and merges one JSON game-state file.

### `rohr_game_state_files_load`

```c
EngineResult rohr_game_state_files_load(const char *const *paths, size_t path_count);
```

 Loads multiple JSON files with cross-file name resolution.

### `rohr_ui_button_by_name_get`

```c
UIButtonDefinitionResult rohr_ui_button_by_name_get(const char *name);
```

 @brief Finds a UI button definition loaded from JSON.

### `rohr_ui_font_by_name_get`

```c
UIFontDefinitionResult rohr_ui_font_by_name_get(const char *name);
```

 @brief Finds a UI font definition loaded from JSON.

### `rohr_ui_label_by_name_get`

```c
UILabelDefinitionResult rohr_ui_label_by_name_get(const char *name);
```

 @brief Finds a standalone UI label definition loaded from JSON.

### `rohr_ui_slider_by_name_get`

```c
UISliderDefinitionResult rohr_ui_slider_by_name_get(const char *name);
```

 @brief Finds a UI slider definition loaded from JSON.

### `rohr_game_state_file_save`

```c
EngineResult rohr_game_state_file_save(const char *path);
```

 Saves all live entities using their unique names to a JSON game-state file.

### `rohr_game_state_template_file_save`

```c
EngineResult rohr_game_state_template_file_save(const char *path);
```

 Saves retained authored definitions without expanding prototypes.

### `rohr_camera_config_default_get`

```c
CameraConfig rohr_camera_config_default_get(void);
```

 Returns defaults for a full-screen camera with unit zoom.

### `rohr_camera_create`

```c
CameraIdResult rohr_camera_create(CameraConfig config);
```

 Creates an engine-owned camera.

### `rohr_camera_destroy`

```c
EngineResult rohr_camera_destroy(CameraId camera);
```

 Destroys a non-active camera.

### `rohr_camera_active_set`

```c
EngineResult rohr_camera_active_set(CameraId camera);
```

 Selects the camera used by transforms and subsequent drawing.

### `rohr_camera_active_get`

```c
CameraId rohr_camera_active_get(void);
```

 Returns the active camera handle.

### `rohr_camera_get`

```c
CameraResult rohr_camera_get(CameraId camera);
```

 Returns an engine-owned camera value.

### `rohr_camera_set`

```c
EngineResult rohr_camera_set(CameraId camera, Camera value);
```

 Replaces a camera value and detaches it.

### `rohr_camera_attach`

```c
EngineResult rohr_camera_attach( CameraId camera, Entity entity, Vec2D position_offset, Orientation orientation_offset, bool follow_position, bool follow_orientation );
```

 Attaches a camera to an entity transform.

### `rohr_camera_radians_attach`

```c
EngineResult rohr_camera_radians_attach( CameraId camera, Entity entity, Vec2D position_offset, Orientation orientation_offset, bool follow_position, bool follow_orientation );
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_camera_detach`

```c
EngineResult rohr_camera_detach(CameraId camera);
```

 Detaches a camera while preserving its resolved transform.

### `rohr_ui_input_set`

```c
void rohr_ui_input_set(UIInput input);
```

 @brief Replaces logical screen-space input without resetting widgets or events.

### `rohr_ui_component_bounds_get`

```c
UIRect rohr_ui_component_bounds_get(UIRect bounds, const TextAsset *const *texts, size_t text_count, UIComponentConfig config);
```

 @brief Applies reusable sizing components to UI bounds.

### `rohr_ui_event_add`

```c
void rohr_ui_event_add(const SDL_Event *event);
```

 Queues keyboard and pointer events used by UI controls.

### `rohr_ui_field_event_add`

```c
void rohr_ui_field_event_add(const SDL_Event *event);
```

 Queues a keyboard event for focused UI fields.

### `rohr_ui_field_focus_clear`

```c
void rohr_ui_field_focus_clear(void);
```

 Clears the currently focused UI text or number field.

### `rohr_ui_field`

```c
UIFieldResult rohr_ui_field(const char *id, UIFieldBinding binding, TextAsset *display, UIRect bounds, const UIButtonStyle *style);
```

 Draws and edits a caller-owned string or float field.

### `rohr_ui_button`

```c
UIButtonResult rohr_ui_button( const char *id, const TextAsset *label, UIRect bounds, const UIButtonStyle *style );
```

Draws and updates one button identified by a stable string.

| Parameter | Description |
| --- | --- |
| `label` | Optional centered label; NULL or empty draws no label. |

### `rohr_ui_dropdown`

```c
UIDropdownResult rohr_ui_dropdown(const char *id, const TextAsset *const *options, size_t option_count, size_t selected_index, UIDropdownConfig config, UIRect bounds, const UIButtonStyle *style);
```

Draws a dropdown and returns selection and hover-preview state.

Set config.visible_row_limit to zero to display every option; limited lists scroll.

### `rohr_ui_menu`

```c
UIDropdownResult rohr_ui_menu(const char *id, const TextAsset *label, const TextAsset *const *options, size_t option_count, UIRect bounds, const UIButtonStyle *style);
```

Draws a menu button with every action visible and no scrollbar.

The menu label is not repeated in its action list.

### `rohr_ui_scroll_region_begin`

```c
UIScrollRegionResult rohr_ui_scroll_region_begin(const char *id, UIRect bounds, float content_height, float offset, float wheel_step);
```

 @brief Begins a clipped vertical scroll region for subsequent UI controls.

### `rohr_ui_scroll_region_end`

```c
void rohr_ui_scroll_region_end(void);
```

 @brief Ends the current UI scroll region.

### `rohr_ui_translation_y_push`

```c
void rohr_ui_translation_y_push(float offset);
```

 @brief Adds a scoped vertical translation for subsequent UI controls.

### `rohr_ui_translation_y_pop`

```c
void rohr_ui_translation_y_pop(void);
```

 @brief Restores the vertical translation active before the last push.

### `rohr_ui_interaction`

```c
UIButtonResult rohr_ui_interaction(const char *id, UIRect bounds);
```

 @brief Updates pointer interaction without prescribing visuals.

### `rohr_ui_surface`

```c
void rohr_ui_surface(UIRect bounds, Color color);
```

 @brief Draws a filled rectangular UI primitive.

### `rohr_ui_border`

```c
void rohr_ui_border(UIRect bounds, float thickness, Color color);
```

 @brief Draws a rectangular UI border primitive.

### `rohr_ui_content`

```c
void rohr_ui_content(const TextAsset *text, UIRect bounds);
```

 @brief Draws centered clipped text content.

### `rohr_ui_quad`

```c
void rohr_ui_quad(Position center, float width, float height, float angle, Color color);
```

 @brief Draws an oriented UI rectangle primitive.

### `rohr_ui_radians_quad`

```c
void rohr_ui_radians_quad(Position center, float width, float height, float angle, Color color);
```

 Clockwise-positive radians alternative; other parameters retain their units.

### `rohr_ui_clip_begin`

```c
bool rohr_ui_clip_begin(UIRect bounds);
```

 @brief Begins and ends a clipped UI component region.

### `rohr_ui_navigation_move`

```c
bool rohr_ui_navigation_move(UINavigationDirection direction);
```

 @brief Moves UI keyboard focus in a screen-space direction.

### `rohr_ui_navigation_activate`

```c
bool rohr_ui_navigation_activate(void);
```

 @brief Activates the currently focused UI control.

### `rohr_ui_navigation_focus_bounds_get`

```c
bool rohr_ui_navigation_focus_bounds_get(UIRect *bounds);
```

 @brief Returns the focused control's previous-frame screen bounds.

### `rohr_ui_label`

```c
void rohr_ui_label(const TextAsset *text, UIRect bounds);
```

 @brief Draws reusable text centered inside bounds.

### `rohr_ui_button_disabled`

```c
void rohr_ui_button_disabled(UIRect bounds, const UIButtonStyle *style);
```

 @brief Draws a disabled button that cannot capture input.

### `rohr_ui_pointer_consumed_get`

```c
bool rohr_ui_pointer_consumed_get(void);
```

 Return current pointer consumption; read before rohr_ui_update().

### `rohr_ui_update`

```c
Report rohr_ui_update(void);
```

 Finalize overlays/focus and return one owned ROHR_UI_REPORT_ID, then clear

transient input and registration. Empty frames return COMPLETE. Timing covers

only this call, excluding earlier widget execution and application time.

### `rohr_ui_button_style_default_get`

```c
UIButtonStyle rohr_ui_button_style_default_get(void);
```

 @brief Returns the default button colors.

### `rohr_ui_slider_config_default_get`

```c
UISliderConfig rohr_ui_slider_config_default_get(void);
```

 @brief Returns the default slider configuration and 0..1 range.

### `rohr_ui_slider`

```c
UISliderResult rohr_ui_slider(const char *id, float value, const UISliderConfig *config);
```

 @brief Draws and updates a caller-owned slider value.

### `rohr_ui_slider_with_text`

```c
UISliderResult rohr_ui_slider_with_text(const char *id, float value, const UISliderConfig *config, const UISliderText *text);
```

 @brief Draws a slider with optional caller-owned label and value text.
