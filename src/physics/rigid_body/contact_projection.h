/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 * Internal numeric template, deliberately included with different
 * PROJECTION_REAL / PROJECTION_NAME / PROJECTION_SQRT definitions.
 */
/* Project zero onto A*x >= b. Cholesky or reorthogonalized QR provides the
 * triangular systems for minimum change and nonnegative normal impulses.
 * Degenerate/inconsistent sets return to the caller's fallback path. */
static bool PROJECTION_NAME(ContactBlock *block, bool position, bool sticking,
    bool seed, long double *change, long double *impulses) {
    PROJECTION_REAL q[BLOCK_ROW_MAX][BLOCK_DOF_MAX];
    PROJECTION_REAL l[BLOCK_ROW_MAX][BLOCK_ROW_MAX];
    PROJECTION_REAL y[BLOCK_ROW_MAX], lambda[BLOCK_ROW_MAX];
    PROJECTION_REAL projected[BLOCK_DOF_MAX];
    unsigned active[BLOCK_ROW_MAX], count = 0;
    unsigned factorized = 0;
    bool sliding[BLOCK_ROW_MAX];
    for(unsigned i = 0; i < block->row_count; ++i) sliding[i] = block->rows[i].sliding;
    uint64_t seen[64][2];
    unsigned dofs = 3 * block->body_count;
#ifdef PROJECTION_GRAM
    PROJECTION_REAL gram[BLOCK_ROW_MAX][BLOCK_ROW_MAX];
    for(unsigned i = 0; i < block->row_count; ++i) {
        memset(q[i], 0, dofs * sizeof(q[i][0]));
        for(unsigned side = 0; side < 2; ++side)
            for(unsigned axis = 0; axis < 3; ++axis) {
                unsigned k = 3 * block->rows[i].slots[side] + axis;
                q[i][k] = block->rows[i].a[k];
            }
    }
    for(unsigned i = 0; i < block->row_count; ++i)
        for(unsigned j = 0; j <= i; ++j) {
            PROJECTION_REAL dot = 0;
            for(unsigned side = 0; side < 2; ++side)
                for(unsigned axis = 0; axis < 3; ++axis) {
                    unsigned k = 3 * block->rows[i].slots[side] + axis;
                    dot += q[i][k] * q[j][k];
                }
            gram[i][j] = gram[j][i] = dot;
        }
#endif
    for(unsigned i = 0; i < block->row_count; ++i)
        if(block->rows[i].tangent ? (sticking && !block->rows[i].sliding) :
                (seed && block->rows[i].active_hint)) active[count++] = i;
    memset(projected, 0, dofs * sizeof(*projected));
    memset(impulses, 0, block->row_count * sizeof(*impulses));
    for(unsigned pass = 0; pass < 4 * BLOCK_ROW_MAX * BLOCK_ROW_MAX; ++pass) {
        uint64_t mask[2] = {0};
        for(unsigned i = 0; i < count; ++i) mask[active[i] / 64] |= UINT64_C(1) << (active[i] % 64);
        for(unsigned i = 0; i < pass && i < 64; ++i)
            if(seen[i][0] == mask[0] && seen[i][1] == mask[1]) return false;
        seen[pass % 64][0] = mask[0]; seen[pass % 64][1] = mask[1];
        if(count) {
            /* Adding a contact preserves the existing orthogonal basis.
             * Rebuild only the suffix invalidated by an active-row removal. */
            for(unsigned i = factorized; i < count; ++i) {
                memset(l[i], 0, sizeof(l[i]));
#ifdef PROJECTION_GRAM
                for(unsigned j = 0; j <= i; ++j) {
                    PROJECTION_REAL value = gram[active[i]][active[j]];
                    for(unsigned k = 0; k < j; ++k) value -= l[i][k] * l[j][k];
                    if(i == j) {
                        if(!isfinite(value) || value < 1e-12) return false;
                        l[i][i] = PROJECTION_SQRT(value);
                    } else l[i][j] = value / l[j][j];
                }
#else
                const BlockRow *row = &block->rows[active[i]];
                for(unsigned k = 0; k < dofs; ++k) q[i][k] = row->a[k];
                for(unsigned repeat = 0; repeat < 2; ++repeat) {
                    for(unsigned j = 0; j < i; ++j) {
                        PROJECTION_REAL dot = 0;
                        for(unsigned k = 0; k < dofs; ++k) dot += q[i][k] * q[j][k];
                        l[i][j] += dot;
                        for(unsigned k = 0; k < dofs; ++k) q[i][k] -= dot * q[j][k];
                    }
                }
                PROJECTION_REAL length = 0;
                for(unsigned k = 0; k < dofs; ++k) length += q[i][k] * q[i][k];
                length = PROJECTION_SQRT(length);
                if(!isfinite(length) || length < 1e-12) return false;
                l[i][i] = length;
                for(unsigned k = 0; k < dofs; ++k) q[i][k] /= length;
#endif
            }
            factorized = count;
            for(unsigned i = 0; i < count; ++i) {
                const BlockRow *row = &block->rows[active[i]];
                y[i] = position ? row->position_rhs : row->velocity_rhs;
                for(unsigned j = 0; j < i; ++j) y[i] -= l[i][j] * y[j];
                y[i] /= l[i][i];
            }
            int remove = -1;
            PROJECTION_REAL lowest = -1e-12;
            for(unsigned reverse = count; reverse > 0; --reverse) {
                unsigned i = reverse - 1;
                lambda[i] = y[i];
                for(unsigned j = i + 1; j < count; ++j)
                    lambda[i] -= l[j][i] * lambda[j];
                lambda[i] /= l[i][i];
                if(!block->rows[active[i]].tangent && lambda[i] < lowest) {
                    lowest = lambda[i]; remove = (int)i;
                }
            }
            if(remove >= 0) {
                for(unsigned i = (unsigned)remove; i + 1 < count; ++i)
                    active[i] = active[i + 1];
                --count;
                factorized = (unsigned)remove;
                memset(projected, 0, dofs * sizeof(*projected));
                continue;
            }
            memset(projected, 0, dofs * sizeof(*projected));
            for(unsigned i = 0; i < count; ++i)
#ifdef PROJECTION_GRAM
                for(unsigned side = 0; side < 2; ++side)
                    for(unsigned axis = 0; axis < 3; ++axis) {
                        unsigned k = 3 * block->rows[active[i]].slots[side] + axis;
                        projected[k] += q[active[i]][k] * lambda[i];
                    }
#else
                for(unsigned k = 0; k < dofs; ++k)
                    projected[k] += q[i][k] * y[i];
#endif
        }
        int add = -1;
        PROJECTION_REAL largest = 1e-12;
        for(unsigned i = 0; i < block->row_count; ++i) {
            if(block->rows[i].tangent) continue;
            if(mask[i / 64] & (UINT64_C(1) << (i % 64))) continue;
            const BlockRow *row = &block->rows[i];
            PROJECTION_REAL violation = position ? row->position_rhs : row->velocity_rhs;
#ifdef PROJECTION_GRAM
            for(unsigned side = 0; side < 2; ++side)
                for(unsigned axis = 0; axis < 3; ++axis) {
                    unsigned k = 3 * row->slots[side] + axis;
                    violation -= q[i][k] * projected[k];
                }
#else
            for(unsigned k = 0; k < dofs; ++k)
                violation -= (PROJECTION_REAL)row->a[k] * projected[k];
#endif
            if(violation > largest) { largest = violation; add = (int)i; }
        }
        if(add < 0) {
            /* Remove only tangents outside their own Coulomb bound. Preserve
             * the factorized prefix as those contacts switch to sliding. */
            bool remove[BLOCK_ROW_MAX] = {0};
            unsigned removed = 0;
            for(unsigned i = 0; i < count; ++i) {
                const BlockRow *row = &block->rows[active[i]];
                if(!row->tangent) continue;
                long double normal = 0;
                for(unsigned j = 0; j < count; ++j) {
                    const BlockRow *other = &block->rows[active[j]];
                    if(!other->tangent && other->pair == row->pair && other->manifold == row->manifold)
                        normal += lambda[j] / other->scale;
                }
                if(fabsl(lambda[i] / row->scale) > row->friction * normal + 1e-12) {
                    remove[i] = true;
                    ++removed;
                    sliding[active[i]] = true;
                }
            }
            if(removed) {
                unsigned next = 0;
                for(unsigned i = 0; i < count; ++i) {
                    if(remove[i]) {
                        if(factorized > next) factorized = next;
                    } else active[next++] = active[i];
                }
                count = next;
                memset(projected, 0, dofs * sizeof(*projected));
                continue;
            }
            for(unsigned i = 0; i < count; ++i) {
                impulses[active[i]] = lambda[i] / block->rows[active[i]].scale;
                if(!isfinite(impulses[active[i]])) return false;
            }
            for(unsigned k = 0; k < dofs; ++k) {
                if(!isfinite(projected[k])) return false;
                change[k] = projected[k];
            }
            for(unsigned i = 0; i < block->row_count; ++i) block->rows[i].sliding = sliding[i];
            return true;
        }
        if(count == BLOCK_ROW_MAX) return false;
        active[count++] = (unsigned)add;
    }
    return false;
}
