/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_PARTICLE_SOLVER_H
#define ROHR_PARTICLE_SOLVER_H

#include "physics/collision/contact_constraint.h"

/* Owned scratch belongs to the contact list. Preparation is the only allocation
 * boundary; no cache survives a solve or permits stale manual-stage state. */
bool physics_particle_solver_prepare_check(ContactConstraintList *list,
    uint32_t iterations, Report *report);
bool physics_particle_solver_active_check(const ContactConstraintList *list);
bool physics_particle_solver_pass_check(const SystemContactConstraint *constraint,
    float *position_fraction);
void physics_particle_solver_solve(ContactConstraintList *list,
    float position_fraction, Report *report);
void physics_particle_solver_finish(ContactConstraintList *list);
void physics_particle_solver_destroy(ContactConstraintList *list);

#endif
