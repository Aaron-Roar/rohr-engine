/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#include "constraint_solver.h"

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
    contacts->rigid_dirty |= contacts->rigid_gathered;
    for(size_t i = 0; i < contacts->count; ++i)
        if(contacts->values[i].type == SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR)
            contacts->values[i].value.rigid.block_geometry_valid = false;
    for(uint32_t iteration = 0; iteration < iterations; iteration += 1) {
        contacts->solver_converged = false;
        position_fraction = 1.0f / (float)(iterations - iteration);
        contacts_solve(contacts, position_fraction, context);
        if(joints_solve != NULL) joints_solve(context);
        /* A preceding pass must have applied transform locks before contact
         * convergence can prove the contact-only system is unchanged. */
        if(iteration > 0 && contacts->solver_converged) break;
    }
    if(contacts_finalize != NULL) contacts_finalize(contacts, context);
}
