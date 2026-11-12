/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "physics.h"
#include <float.h>
#include <math.h>

/* Components are expressed before the configured reference rotation. Keep the
 * existing magnitude sign, including at zero, so signed-source editing remains
 * consistent with magnitude_set. No reference geometry needs resolving here. */
static EngineResult force_vector_set(Entity source, ForceDescription description,
        double x, double y) {
    double magnitude = hypot(x, y);
    if(!isfinite(x) || !isfinite(y) || !isfinite(magnitude) || magnitude > FLT_MAX)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    bool negative = signbit(description.magnitude);
    description.magnitude = (float)(negative ? -magnitude : magnitude);
    if(magnitude != 0)
        description.orientation.offset = (float)(atan2(negative ? -x : x,
            negative ? -y : y) * (180.0 / PI_F));
    return physics_force_description_set(source, description);
}

EngineResult physics_force_set(Entity source, Force vector) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    return force_vector_set(source, result.result.value, vector.x, vector.y);
}

EngineResult physics_force_x_set(Entity source, float x) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription value = result.result.value;
    double radians = remainder((double)value.orientation.offset, 360.0) * (PI_F / 180.0);
    return force_vector_set(source, value, x, value.magnitude * cos(radians));
}

EngineResult physics_force_y_set(Entity source, float y) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription value = result.result.value;
    double radians = remainder((double)value.orientation.offset, 360.0) * (PI_F / 180.0);
    return force_vector_set(source, value, value.magnitude * sin(radians), y);
}

EngineResult physics_force_location_offset_set(Entity source, Vec2D value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    description.location.offset = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_location_offset_x_set(Entity source, float value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    description.location.offset.x = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_location_offset_y_set(Entity source, float value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    description.location.offset.y = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_location_reference_set(Entity source, ForcePositionReference value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    description.location.reference = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_location_reference_entity_set(Entity source, Entity value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    switch(description.location.reference.kind) {
    case ROHR_FORCE_COM_POSITION:
    case ROHR_FORCE_ORIGIN_POSITION:
        description.location.reference.entity = value;
        break;
    case ROHR_FORCE_BEAM_POSITION:
        description.location.reference.beam.entity = value;
        break;
    default:
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    }
    return physics_force_description_set(source, description);
}

EngineResult physics_force_location_reference_beam_anchor_set(Entity source, ForceBeamAnchor value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.location.reference.kind != ROHR_FORCE_BEAM_POSITION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.location.reference.beam.anchor = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_location_reference_beam_fraction_set(Entity source, float value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.location.reference.kind != ROHR_FORCE_BEAM_POSITION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.location.reference.beam.fraction = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_from_set(Entity source, ForceLocation value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.from = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_from_offset_set(Entity source, Vec2D value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.from.offset = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_from_offset_x_set(Entity source, float value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.from.offset.x = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_from_offset_y_set(Entity source, float value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.from.offset.y = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_from_reference_set(Entity source, ForcePositionReference value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.from.reference = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_from_reference_entity_set(Entity source, Entity value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    switch(description.orientation.reference.between.from.reference.kind) {
    case ROHR_FORCE_COM_POSITION:
    case ROHR_FORCE_ORIGIN_POSITION:
        description.orientation.reference.between.from.reference.entity = value;
        break;
    case ROHR_FORCE_BEAM_POSITION:
        description.orientation.reference.between.from.reference.beam.entity = value;
        break;
    default:
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    }
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_from_reference_beam_anchor_set(Entity source, ForceBeamAnchor value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(description.orientation.reference.between.from.reference.kind != ROHR_FORCE_BEAM_POSITION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.from.reference.beam.anchor = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_from_reference_beam_fraction_set(Entity source, float value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(description.orientation.reference.between.from.reference.kind != ROHR_FORCE_BEAM_POSITION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.from.reference.beam.fraction = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_to_set(Entity source, ForceLocation value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.to = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_to_offset_set(Entity source, Vec2D value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.to.offset = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_to_offset_x_set(Entity source, float value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.to.offset.x = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_to_offset_y_set(Entity source, float value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.to.offset.y = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_to_reference_set(Entity source, ForcePositionReference value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.to.reference = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_to_reference_entity_set(Entity source, Entity value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    switch(description.orientation.reference.between.to.reference.kind) {
    case ROHR_FORCE_COM_POSITION:
    case ROHR_FORCE_ORIGIN_POSITION:
        description.orientation.reference.between.to.reference.entity = value;
        break;
    case ROHR_FORCE_BEAM_POSITION:
        description.orientation.reference.between.to.reference.beam.entity = value;
        break;
    default:
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    }
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_to_reference_beam_anchor_set(Entity source, ForceBeamAnchor value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(description.orientation.reference.between.to.reference.kind != ROHR_FORCE_BEAM_POSITION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.to.reference.beam.anchor = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_between_to_reference_beam_fraction_set(Entity source, float value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(description.orientation.reference.between.to.reference.kind != ROHR_FORCE_BEAM_POSITION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.between.to.reference.beam.fraction = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_offset_set(Entity source, Orientation value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    description.orientation.offset = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_reference_set(Entity source, ForceOrientationReference value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    description.orientation.reference = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_reference_entity_set(Entity source, Entity value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BODY_ORIENTATION &&
            description.orientation.reference.kind != ROHR_FORCE_BEAM_ORIENTATION)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.reference.entity = value;
    return physics_force_description_set(source, description);
}

EngineResult physics_force_orientation_offset_radians_set(Entity source, Orientation value) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return error_result_error(result.result.error);
    ForceDescription description = result.result.value;
    double degrees = (double)value * (180.0 / PI_F);
    if(!isfinite(degrees) || fabs(degrees) > FLT_MAX)
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    description.orientation.offset = (Orientation)degrees;
    return physics_force_description_set(source, description);
}

ForceResult physics_force_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForceResult, result.result.error);
    ForceDescription value = result.result.value;
    double radians = remainder((double)value.orientation.offset, 360.0) * (PI_F / 180.0);
    Force vector = {value.magnitude * sin(radians), value.magnitude * cos(radians)};
    return ERROR_RESULT_MAKE_VALUE(ForceResult, vector);
}

ScalarResult physics_force_x_get(Entity source) {
    ForceResult result = physics_force_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, result.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, result.result.value.x);
}

ScalarResult physics_force_y_get(Entity source) {
    ForceResult result = physics_force_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, result.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, result.result.value.y);
}

PositionResult physics_force_location_offset_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(PositionResult, result.result.error);
    ForceDescription description = result.result.value;
    return ERROR_RESULT_MAKE_VALUE(PositionResult, description.location.offset);
}

ScalarResult physics_force_location_offset_x_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, result.result.error);
    ForceDescription description = result.result.value;
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, description.location.offset.x);
}

ScalarResult physics_force_location_offset_y_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, result.result.error);
    ForceDescription description = result.result.value;
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, description.location.offset.y);
}

ForcePositionReferenceResult physics_force_location_reference_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForcePositionReferenceResult, result.result.error);
    ForceDescription description = result.result.value;
    return ERROR_RESULT_MAKE_VALUE(ForcePositionReferenceResult, description.location.reference);
}

EntityResult physics_force_location_reference_entity_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    ForceDescription description = result.result.value;
    switch(description.location.reference.kind) {
    case ROHR_FORCE_COM_POSITION:
    case ROHR_FORCE_ORIGIN_POSITION:
        return ERROR_RESULT_MAKE_VALUE(EntityResult, description.location.reference.entity);
    case ROHR_FORCE_BEAM_POSITION:
        return ERROR_RESULT_MAKE_VALUE(EntityResult, description.location.reference.beam.entity);
    default:
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    }
}

ForceBeamAnchorResult physics_force_location_reference_beam_anchor_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForceBeamAnchorResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.location.reference.kind != ROHR_FORCE_BEAM_POSITION)
        return ERROR_RESULT_MAKE_ERROR(ForceBeamAnchorResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ForceBeamAnchorResult, description.location.reference.beam.anchor);
}

ScalarResult physics_force_location_reference_beam_fraction_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.location.reference.kind != ROHR_FORCE_BEAM_POSITION)
        return ERROR_RESULT_MAKE_ERROR(ScalarResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, description.location.reference.beam.fraction);
}

ForceLocationResult physics_force_orientation_between_from_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForceLocationResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(ForceLocationResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ForceLocationResult, description.orientation.reference.between.from);
}

PositionResult physics_force_orientation_between_from_offset_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(PositionResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(PositionResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(PositionResult, description.orientation.reference.between.from.offset);
}

ScalarResult physics_force_orientation_between_from_offset_x_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(ScalarResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, description.orientation.reference.between.from.offset.x);
}

ScalarResult physics_force_orientation_between_from_offset_y_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(ScalarResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, description.orientation.reference.between.from.offset.y);
}

ForcePositionReferenceResult physics_force_orientation_between_from_reference_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForcePositionReferenceResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(ForcePositionReferenceResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ForcePositionReferenceResult, description.orientation.reference.between.from.reference);
}

EntityResult physics_force_orientation_between_from_reference_entity_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    switch(description.orientation.reference.between.from.reference.kind) {
    case ROHR_FORCE_COM_POSITION:
    case ROHR_FORCE_ORIGIN_POSITION:
        return ERROR_RESULT_MAKE_VALUE(EntityResult, description.orientation.reference.between.from.reference.entity);
    case ROHR_FORCE_BEAM_POSITION:
        return ERROR_RESULT_MAKE_VALUE(EntityResult, description.orientation.reference.between.from.reference.beam.entity);
    default:
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    }
}

ForceBeamAnchorResult physics_force_orientation_between_from_reference_beam_anchor_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForceBeamAnchorResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(ForceBeamAnchorResult, ERROR_ENGINE_STATE_INVALID);
    if(description.orientation.reference.between.from.reference.kind != ROHR_FORCE_BEAM_POSITION)
        return ERROR_RESULT_MAKE_ERROR(ForceBeamAnchorResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ForceBeamAnchorResult, description.orientation.reference.between.from.reference.beam.anchor);
}

ScalarResult physics_force_orientation_between_from_reference_beam_fraction_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(ScalarResult, ERROR_ENGINE_STATE_INVALID);
    if(description.orientation.reference.between.from.reference.kind != ROHR_FORCE_BEAM_POSITION)
        return ERROR_RESULT_MAKE_ERROR(ScalarResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, description.orientation.reference.between.from.reference.beam.fraction);
}

ForceLocationResult physics_force_orientation_between_to_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForceLocationResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(ForceLocationResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ForceLocationResult, description.orientation.reference.between.to);
}

PositionResult physics_force_orientation_between_to_offset_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(PositionResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(PositionResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(PositionResult, description.orientation.reference.between.to.offset);
}

ScalarResult physics_force_orientation_between_to_offset_x_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(ScalarResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, description.orientation.reference.between.to.offset.x);
}

ScalarResult physics_force_orientation_between_to_offset_y_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(ScalarResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, description.orientation.reference.between.to.offset.y);
}

ForcePositionReferenceResult physics_force_orientation_between_to_reference_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForcePositionReferenceResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(ForcePositionReferenceResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ForcePositionReferenceResult, description.orientation.reference.between.to.reference);
}

EntityResult physics_force_orientation_between_to_reference_entity_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    switch(description.orientation.reference.between.to.reference.kind) {
    case ROHR_FORCE_COM_POSITION:
    case ROHR_FORCE_ORIGIN_POSITION:
        return ERROR_RESULT_MAKE_VALUE(EntityResult, description.orientation.reference.between.to.reference.entity);
    case ROHR_FORCE_BEAM_POSITION:
        return ERROR_RESULT_MAKE_VALUE(EntityResult, description.orientation.reference.between.to.reference.beam.entity);
    default:
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    }
}

ForceBeamAnchorResult physics_force_orientation_between_to_reference_beam_anchor_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForceBeamAnchorResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(ForceBeamAnchorResult, ERROR_ENGINE_STATE_INVALID);
    if(description.orientation.reference.between.to.reference.kind != ROHR_FORCE_BEAM_POSITION)
        return ERROR_RESULT_MAKE_ERROR(ForceBeamAnchorResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ForceBeamAnchorResult, description.orientation.reference.between.to.reference.beam.anchor);
}

ScalarResult physics_force_orientation_between_to_reference_beam_fraction_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(ScalarResult, ERROR_ENGINE_STATE_INVALID);
    if(description.orientation.reference.between.to.reference.kind != ROHR_FORCE_BEAM_POSITION)
        return ERROR_RESULT_MAKE_ERROR(ScalarResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, description.orientation.reference.between.to.reference.beam.fraction);
}

ScalarResult physics_force_orientation_offset_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, result.result.error);
    ForceDescription description = result.result.value;
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, description.orientation.offset);
}

ForceOrientationReferenceResult physics_force_orientation_reference_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ForceOrientationReferenceResult, result.result.error);
    ForceDescription description = result.result.value;
    return ERROR_RESULT_MAKE_VALUE(ForceOrientationReferenceResult, description.orientation.reference);
}

EntityResult physics_force_orientation_reference_entity_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(EntityResult, result.result.error);
    ForceDescription description = result.result.value;
    if(description.orientation.reference.kind != ROHR_FORCE_BODY_ORIENTATION &&
            description.orientation.reference.kind != ROHR_FORCE_BEAM_ORIENTATION)
        return ERROR_RESULT_MAKE_ERROR(EntityResult, ERROR_ENGINE_STATE_INVALID);
    return ERROR_RESULT_MAKE_VALUE(EntityResult, description.orientation.reference.entity);
}

ScalarResult physics_force_orientation_offset_radians_get(Entity source) {
    ForceDescriptionResult result = physics_force_description_get(source);
    if(error_check(result)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, result.result.error);
    ForceDescription description = result.result.value;
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, description.orientation.offset * (PI_F / 180.0));
}

/* Scalar shorthand; target, ownership and lifetime follow the magnitude API. */
ScalarResult physics_torque_get(Entity source) {
    TorqueResult current = physics_torque_magnitude_get(source);
    if(error_check(current)) return ERROR_RESULT_MAKE_ERROR(ScalarResult, current.result.error);
    return ERROR_RESULT_MAKE_VALUE(ScalarResult, current.result.value);
}
EngineResult physics_torque_set(Entity source, Torque value) { return physics_torque_magnitude_set(source, value); }
