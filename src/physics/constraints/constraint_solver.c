/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "constraint_solver.h"
#include "physics/physics_operation.h"
#include "physics/physics_step_internal.h"

void constraint_solver_run(
    ContactConstraintList *contacts,
    uint32_t iterations,
    ConstraintContactsSolveFn contacts_solve,
    ConstraintJointsSolveFn joints_solve,
    ConstraintContactsFinalizeFn contacts_finalize,
    void *context
) {
    float position_fraction;

    if(contacts == NULL || iterations == 0 || contacts_solve == NULL) return;
    contacts->solving = true;
    if(contacts == &physics_step_contact_constraints) {
        physics_step_hitbox_cache_begin();
        physics_soft_body_gather_cache_reset();
        physics_beam_pairs_gather_cache_reset();
    }
    contacts->rigid_dirty |= contacts->rigid_gathered;
    bool soft_pass = contacts == &physics_step_contact_constraints && soft_body_beams_pool.count > 0;
    if(contacts == &physics_step_contact_constraints)
        for(size_t i = 0; i < contacts->count; ++i)
            soft_pass |= contacts->values[i].type != SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR;
    for(size_t i = 0; i < contacts->count; ++i)
        if(contacts->values[i].type == SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR)
            contacts->values[i].value.rigid.block_geometry_valid = false;
        else if(contacts->values[i].type == SYSTEM_CONTACT_CONSTRAINT_SOFT_BOUNDARY)
            contacts->values[i].value.soft.geometry_valid = false;
        else if(contacts->values[i].type == SYSTEM_CONTACT_CONSTRAINT_BEAM_PAIR)
            contacts->values[i].value.beam_pair.geometry_valid = false;
    for(uint32_t iteration = 0; iteration < iterations; iteration += 1) {
        contacts->solver_converged = false;
        position_fraction = soft_pass ? 1.0f : 1.0f / (float)(iterations - iteration);
        if(soft_pass) physics_motion_tracking_begin();
        contacts_solve(contacts, position_fraction, context);
        if(joints_solve != NULL) joints_solve(context);
        if(soft_pass) contacts->solver_converged = physics_motion_tracking_end_check();
        /* A preceding pass must have applied transform locks before contact
         * convergence can prove the contact-only system is unchanged. */
        if(iteration > 0 && contacts->solver_converged) break;
    }
    if(contacts_finalize != NULL) contacts_finalize(contacts, context);
    contacts->solving = false;
    if(contacts == &physics_step_contact_constraints) physics_step_hitbox_cache_end();
}
