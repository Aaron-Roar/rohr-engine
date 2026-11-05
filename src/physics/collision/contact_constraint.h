/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#ifndef CONTACT_CONSTRAINT_H
#define CONTACT_CONSTRAINT_H

#include <stdbool.h>
#include <stddef.h>

#include "entity_components.h"
#include "physics.h"
#include "physics/collision/contact_manifold.h"

typedef struct SystemSoftBoundaryQuery {
    Entity node_a;
    Entity node_b;
    EntityIndex beam_index;
    EntityIndex a;
    EntityIndex b;
    Shape shape;
    Position start;
    Position end;
    float radius;
    bool exclude_node_a;
    bool exclude_node_b;
    Entity rigid_entity;
    EntityIndex rigid;
    OverlapInfo overlap;
    Position contact_position;
    float t;
    bool responds;
    bool solving;
    bool solved;
    /* Fixed impact geometry and velocity target for this substep's solve.
     * Position correction may change the current overlap independently. */
    Axis velocity_normal;
    Vec2D rigid_offset;
    float velocity_t;
    float rebound_speed;
    float normal_impulse;
    float tangent_impulse;
    float position_fraction;
    ContactInfo contact;
    /* Response geometry cache, invalidated at each solver invocation. */
    Position geometry_rigid_position;
    Orientation geometry_rigid_orientation;
    bool geometry_valid;
    bool geometry_overlapping;
} SystemSoftBoundaryQuery;

typedef enum SystemContactConstraintType {
    SYSTEM_CONTACT_CONSTRAINT_RIGID_PAIR,
    SYSTEM_CONTACT_CONSTRAINT_SOFT_BOUNDARY,
    SYSTEM_CONTACT_CONSTRAINT_BEAM_PAIR
} SystemContactConstraintType;

typedef struct SystemBeamPair {
    Entity beams[2];
    EntityIndex indices[2], nodes[4];
    float weights[4], normal_impulse, tangent_impulse, rebound_speed;
    Axis normal;
    OverlapInfo overlap;
    ContactInfo contact;
    bool solved, responds;
    Position geometry_nodes[4], geometry_point;
    float geometry_weights[4];
    OverlapInfo geometry_overlap;
    bool geometry_valid, geometry_overlapping;
} SystemBeamPair;

typedef struct SystemContactConstraint {
    SystemContactConstraintType type;
    union {
        struct {
            Entity first;
            Entity second;
            EntityIndex first_index;
            EntityIndex second_index;
            OverlapInfo overlap;
            bool responds;
            bool solved;
            ContactInfo contact;
            ContactInfo manifold_contacts[CONTACT_MANIFOLD_MAX];
            /* Fixed normal velocity targets through this substep's iterations,
             * indexed alongside the corresponding manifold contact points. */
            float rebound_speeds[CONTACT_MANIFOLD_MAX][PHYSICS_CONTACT_POINT_MAX];
            uint8_t manifold_contact_count;
            /* Geometry-only cache, reset for each solver invocation. */
            ContactManifoldSet block_geometry;
            Position block_positions[2];
            Orientation block_orientations[2];
            bool block_geometry_valid;
        } rigid;
        SystemSoftBoundaryQuery soft;
        SystemBeamPair beam_pair;
    } value;
} SystemContactConstraint;

typedef struct ContactConstraintList {
    SystemContactConstraint *values;
    size_t count;
    size_t capacity;
    /* Owned lookup storage, containing list indices plus one. It changes neither
     * constraint order nor lifetime; clear empties it and destroy releases it. */
    size_t *rigid_pair_slots;
    size_t rigid_pair_capacity;
    size_t rigid_pair_count;
    /* Refresh only a rigid contact pass explicitly gathered by the caller. */
    bool rigid_gathered;
    /* Invalidated by geometry changes during the constraint iterations. */
    bool rigid_dirty;
    /* Set only by a solver that can prove further passes make no change. */
    bool solver_converged;
    /* Tree/pose reuse is confined to one synchronous solver invocation. */
    bool solving;
} ContactConstraintList;

EngineResult contact_constraint_list_init(
    ContactConstraintList *list,
    size_t initial_capacity
);
void contact_constraint_list_destroy(ContactConstraintList *list);
void contact_constraint_list_clear(ContactConstraintList *list);
bool contact_constraint_list_append(
    ContactConstraintList *list,
    SystemContactConstraint constraint
);
bool contact_constraint_list_rigid_append(ContactConstraintList *list,
    Entity first, Entity second, EntityIndex first_index, EntityIndex second_index,
    OverlapInfo overlap, bool responds);
bool contact_constraint_list_rigid_pair_check(
    const ContactConstraintList *list,
    Entity first,
    Entity second
);
SystemContactConstraint *contact_constraint_list_at(
    ContactConstraintList *list,
    size_t index
);

#endif
