/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "car_controls.h"

static const Torque drive_torque = 11111900000.0f;
/* Retain the force magnitude used in the accepted 60 Hz comparison. */
static const float jump_force = 2162880.0f;

static bool external_body_check(Entity root, Entity target) {
    if(!rohr_entity_components_check(target, ROHR_HIT_BOX) ||
            rohr_entity_components_check(target, ROHR_SOFT_BODY_NODE) ||
            rohr_entity_components_check(target, ROHR_SOFT_BODY_BEAM) ||
            rohr_entity_components_check(target, ROHR_SOFT_BODY)) return false;
    for(Entity member=target; member;) {
        if(member==root) return false;
        EntityResult owner=rohr_entity_owner_get(member);
        if(rohr_error_check(owner)) return false;
        member=owner.result.value;
    }
    if(rohr_entity_components_check(target, ROHR_PARTICLE)) {
        EntityInfoResult info=rohr_entity_info_get(target,ROHR_ENTITY_INFO_PHYSICS_PARTICLE);
        if(rohr_error_check(info)) return false;
        bool rigid=!info.result.value.particle.standalone;
        (void)rohr_entity_info_release(&info.result.value);
        return rigid;
    }
    return true;
}
static bool supported_check(Entity root, Entity member) {
    /* Example controls run serially between physics updates. */
    static EntityContact contacts[MAX_ENTITIES];
    size_t count=rohr_physics_contacts_get(member,contacts,MAX_ENTITIES);
    for(size_t i=0;i<count;++i)
        if(external_body_check(root,contacts[i].target)) return true;
    return false;
}
static bool wheel_supported_check(Entity root, Entity disk, Entity tire) {
    if(supported_check(root,disk)) return true;
    SoftBodyResult body=rohr_physics_soft_body_get(tire);
    if(rohr_error_check(body)) return false;
    for(uint32_t i=0;i<body.result.value.node_count;++i)
        if(supported_check(root,body.result.value.nodes[i])) return true;
    for(uint32_t i=0;i<body.result.value.beam_count;++i)
        if(supported_check(root,body.result.value.beams[i])) return true;
    return false;
}
bool car_grounded_check(const Car *car) {
    return car && wheel_supported_check(car->source_instance,car->rear_disk,car->rear_tire) &&
        wheel_supported_check(car->source_instance,car->front_disk,car->front_tire);
}
void car_jump_request(CarControls *controls, const Car *car, bool pressed) {
    if(controls && pressed) controls->jump_pending=car_grounded_check(car);
}
EngineResult car_controls_update(CarControls *controls, const Car *car, float drive_axis) {
    if(!controls || !car) return rohr_error_result_error(ERROR_ENGINE_STATE_INVALID);
    EngineResult result=rohr_physics_torque_magnitude_set(car->torque_rear_drive,drive_axis*drive_torque);
    if(rohr_error_check(result)) return result;
    bool jump=controls->jump_pending;controls->jump_pending=false;
    if(jump && car_grounded_check(car)) {
        ForceDescription description={.target=car->chassis,
            .location={.reference={.kind=ROHR_FORCE_COM_POSITION,.entity=car->chassis}},
            .magnitude=jump_force};
        if(!controls->scripted) return rohr_physics_force_apply(description);
        /* Scripted physics does not advance the engine clock that expires
         * one-shot sources. Keep this force across substeps, then delete it. */
        if(controls->scripted_jump_force)
            return rohr_error_result_error(ERROR_ENGINE_STATE_INVALID);
        EntityResult source=rohr_physics_force_add(description);
        if(rohr_error_check(source)) return rohr_error_result_error(source.result.error);
        controls->scripted_jump_force=source.result.value;
    }
    return result;
}
EngineResult car_controls_step_end(CarControls *controls) {
    if(!controls) return rohr_error_result_error(ERROR_ENGINE_STATE_INVALID);
    if(controls->scripted_jump_force) {
        EngineResult result=rohr_entity_delete(controls->scripted_jump_force);
        if(rohr_error_check(result)) return result;
        controls->scripted_jump_force=ENTITY_INVALID;
    }
    return rohr_error_result_value(true);
}
