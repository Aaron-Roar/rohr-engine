/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#if !defined(_WIN32)
#define _XOPEN_SOURCE 700
#endif
#include "editor_collision_migration.h"
#include "editor_project.h"
#include "entity_components.h"
#include "yyjson/yyjson.h"
#include <SDL3/SDL.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Bound the quadratic relationship audit and all input allocations. This is a
 * tooling limit, not a claim about the minimum Boolean matrix factorization. */
#define MIGRATION_FILES_MAX 128
#define MIGRATION_PROFILES_MAX 2048
#define MIGRATION_REFERENCES_MAX 65536
#define MIGRATION_FILE_BYTES_MAX (64u * 1024u * 1024u)
#define MIGRATION_PATH_MAX 4096

typedef struct MigrationProfile {
    uint64_t category, filter, new_category, new_filter;
} MigrationProfile;
typedef struct MigrationFile {
    char path[MIGRATION_PATH_MAX];
    char backup[MIGRATION_PATH_MAX], pending[MIGRATION_PATH_MAX], rollback[MIGRATION_PATH_MAX];
    void *original;
    size_t original_size;
    yyjson_mut_doc *json;
    char *output;
    size_t output_size;
    bool project, backup_created, pending_created, moved, installed;
} MigrationFile;
typedef struct MigrationReference {
    size_t file;
    yyjson_mut_val *value;
    const char *category_key, *filter_key;
    uint64_t category, filter;
} MigrationReference;
struct EditorCollisionMigration {
    MigrationFile *files;
    size_t file_count;
    MigrationProfile profiles[MIGRATION_PROFILES_MAX];
    size_t profile_count;
    MigrationReference *references;
    size_t reference_count;
    char names[64][EDITOR_OBJECT_NAME_MAX];
    char old_names[64][EDITOR_OBJECT_NAME_MAX];
    size_t name_count, old_name_count;
    bool reassigned;
    char *report;
    char report_path[MIGRATION_PATH_MAX];
};

static EditorResult invalid(const char *message) {
    return editor_result_error(EDITOR_ERROR_SCHEMA_INVALID, "%s", message);
}
static bool put(yyjson_mut_doc *doc, yyjson_mut_val *obj, const char *key, yyjson_mut_val *value) {
    return value && yyjson_mut_obj_put(obj, yyjson_mut_str(doc, key), value);
}
static bool clean_json(yyjson_mut_val *value, unsigned depth) {
    if(depth > 64) return false;
    if(yyjson_mut_is_obj(value)) {
        size_t i, n;
        yyjson_mut_val *key, *child;
        yyjson_mut_obj_foreach(value, i, n, key, child) {
            if(strlen(yyjson_mut_get_str(key)) != yyjson_mut_get_len(key)) return false;
            yyjson_mut_obj_iter iter = yyjson_mut_obj_iter_with(value);
            for(size_t j = 0; j < i; ++j) {
                yyjson_mut_val *other = yyjson_mut_obj_iter_next(&iter);
                if(!strcmp(yyjson_mut_get_str(key), yyjson_mut_get_str(other))) return false;
            }
            if(!clean_json(child, depth + 1)) return false;
        }
    } else if(yyjson_mut_is_arr(value)) {
        size_t i, n;
        yyjson_mut_val *child;
        yyjson_mut_arr_foreach(value, i, n, child)
            if(!clean_json(child, depth + 1)) return false;
    }
    return true;
}
static EditorResult reference_add(EditorCollisionMigration *plan, size_t file,
        yyjson_mut_val *value, bool project, bool defaults) {
    const char *category_key = project ? "collision_category" : "category";
    const char *filter_key = project ? "collision_with" : "collides_with";
    yyjson_mut_val *category = yyjson_mut_obj_get(value, category_key);
    yyjson_mut_val *filter = yyjson_mut_obj_get(value, filter_key);
    defaults = defaults && (!project || !yyjson_mut_obj_get(value, "collision_enabled"));
    if(!yyjson_mut_is_obj(value) || yyjson_mut_obj_get(value, "responds_to") ||
            ((!defaults || category || filter) &&
             (!yyjson_mut_is_uint(category) || !yyjson_mut_is_uint(filter))))
        return invalid("Expected a legacy unsigned collision category/filter pair (no responds_to)");
    uint64_t c = category ? yyjson_mut_get_uint(category) : UINT64_C(1);
    uint64_t f = filter ? yyjson_mut_get_uint(filter) : (project ? UINT64_C(1) : UINT64_MAX);
    if(project) {
        uint64_t valid = plan->old_name_count == 64 ? UINT64_MAX :
            (UINT64_C(1) << plan->old_name_count) - 1;
        if((c | f) & ~valid) return invalid("Project collision mask refers to an unnamed category bit");
        yyjson_mut_val *enabled = yyjson_mut_obj_get(value, "collision_enabled");
        if((enabled && !yyjson_mut_is_bool(enabled)) || (!enabled && (category || filter)))
            return invalid("Invalid project collision_enabled/filter combination");
        if(!enabled && !yyjson_mut_obj_add_bool(plan->files[file].json, value, "collision_enabled", true))
            return invalid("Cannot allocate collision defaults");
    }
    size_t p = 0;
    while(p < plan->profile_count && (plan->profiles[p].category != c || plan->profiles[p].filter != f)) ++p;
    if(p == plan->profile_count) {
        if(p == MIGRATION_PROFILES_MAX)
            return editor_result_error(EDITOR_ERROR_CAPACITY, "Migration supports at most %u distinct filter profiles", MIGRATION_PROFILES_MAX);
        plan->profiles[plan->profile_count++] = (MigrationProfile){.category = c, .filter = f};
    }
    if(plan->reference_count == MIGRATION_REFERENCES_MAX)
        return editor_result_error(EDITOR_ERROR_CAPACITY, "Too many collision records for migration");
    MigrationReference *refs = realloc(plan->references, (plan->reference_count + 1) * sizeof(*refs));
    if(!refs) return invalid("Cannot allocate collision migration records");
    plan->references = refs;
    refs[plan->reference_count++] = (MigrationReference){file, value, category_key, filter_key, c, f};
    return editor_result_value(true);
}
static EditorResult project_collect(EditorCollisionMigration *plan, size_t file) {
    yyjson_mut_val *root = yyjson_mut_doc_get_root(plan->files[file].json);
    yyjson_mut_val *names = yyjson_mut_obj_get(root, "collision_masks");
    size_t count = yyjson_mut_arr_size(names);
    if(!yyjson_mut_is_arr(names) || !count || count > 64)
        return invalid("Project collision_masks must contain 1 to 64 names");
    if(plan->old_name_count && plan->old_name_count != count)
        return invalid("Jointly migrated projects must share the same legacy named-bit registry");
    for(size_t i = 0; i < count; ++i) {
        yyjson_mut_val *name = yyjson_mut_arr_get(names, i);
        if(!yyjson_mut_is_str(name) || !yyjson_mut_get_len(name) ||
                yyjson_mut_get_len(name) >= EDITOR_OBJECT_NAME_MAX ||
                strlen(yyjson_mut_get_str(name)) != yyjson_mut_get_len(name))
            return invalid("Invalid collision category name");
        if(plan->old_name_count && strcmp(plan->old_names[i], yyjson_mut_get_str(name)))
            return invalid("Jointly migrated projects must share the same legacy named-bit registry");
        for(size_t j = 0; j < i; ++j)
            if(!strcmp(plan->old_names[j], yyjson_mut_get_str(name))) return invalid("Duplicate collision category name");
        SDL_strlcpy(plan->old_names[i], yyjson_mut_get_str(name), sizeof(plan->old_names[i]));
    }
    plan->old_name_count = count;
    yyjson_mut_val *objects = yyjson_mut_obj_get(root, "objects");
    if(!yyjson_mut_is_arr(objects)) return invalid("Missing project objects array");
    for(size_t o = 0; o < yyjson_mut_arr_size(objects); ++o) {
        yyjson_mut_val *obj = yyjson_mut_arr_get(objects, o);
        yyjson_mut_val *bodies = yyjson_mut_obj_get(obj, "rigid_bodies");
        yyjson_mut_val *soft = yyjson_mut_obj_get(obj, "soft_bodies");
        if(!yyjson_mut_is_arr(bodies) || (soft && !yyjson_mut_is_arr(soft)))
            return invalid("Invalid project body arrays");
        for(size_t b = 0; b < yyjson_mut_arr_size(bodies); ++b) {
            EditorResult r = reference_add(plan, file, yyjson_mut_arr_get(bodies, b), true, true);
            if(editor_result_check(r)) return r;
        }
        for(size_t b = 0; b < yyjson_mut_arr_size(soft); ++b) {
            const char *keys[] = {"nodes", "beams"};
            for(size_t k = 0; k < 2; ++k) {
                yyjson_mut_val *items = yyjson_mut_obj_get(yyjson_mut_arr_get(soft, b), keys[k]);
                if(!yyjson_mut_is_arr(items)) return invalid("Invalid soft-body node/beam array");
                for(size_t j = 0; j < yyjson_mut_arr_size(items); ++j) {
                    EditorResult r = reference_add(plan, file, yyjson_mut_arr_get(items, j), true, k == 0);
                    if(editor_result_check(r)) return r;
                }
            }
        }
    }
    return editor_result_value(true);
}
static EditorResult runtime_collect(EditorCollisionMigration *plan, size_t file) {
    yyjson_mut_doc *doc = plan->files[file].json;
    yyjson_mut_val *entities = yyjson_mut_obj_get(yyjson_mut_doc_get_root(doc), "entities");
    if(!yyjson_mut_is_arr(entities)) return invalid("Missing runtime entities array");
    for(size_t i = 0; i < yyjson_mut_arr_size(entities); ++i) {
        yyjson_mut_val *components = yyjson_mut_obj_get(yyjson_mut_arr_get(entities, i), "components");
        if(!yyjson_mut_is_obj(components)) return invalid("Invalid runtime components object");
        yyjson_mut_val *beam = yyjson_mut_obj_get(components, "soft_body_beam");
        yyjson_mut_val *filter = yyjson_mut_obj_get(components, "collision_filter");
        bool implicit = !beam && !filter;
        /* Include implicit defaults on all entities: filters may be used after
         * collision is enabled or geometry is attached later. Beams use their
         * topology filter, not the generic component. */
        if(implicit) {
            filter = yyjson_mut_obj(doc);
            if(!filter || !put(doc, components, "collision_filter", filter)) return invalid("Cannot allocate default filter");
        }
        if(filter) {
            EditorResult r = reference_add(plan, file, filter, false, implicit);
            if(editor_result_check(r)) return r;
            yyjson_mut_val *mask = yyjson_mut_obj_get(components, "mask");
            if(mask && (!yyjson_mut_is_uint(mask) || !yyjson_mut_set_uint(mask,
                    yyjson_mut_get_uint(mask) | ROHR_COLLISION_FILTER)))
                return invalid("Invalid runtime component mask");
        }
        if(beam) {
            EditorResult r = reference_add(plan, file, beam, false, false);
            if(editor_result_check(r)) return r;
        }
    }
    return editor_result_value(true);
}
static int profile_compare(const void *a, const void *b) {
    const MigrationProfile *x = a, *y = b;
    if(x->category != y->category) return x->category < y->category ? -1 : 1;
    return x->filter == y->filter ? 0 : x->filter < y->filter ? -1 : 1;
}
static bool allowed(const MigrationProfile *a, const MigrationProfile *b) {
    return (a->filter & b->category) && (b->filter & a->category);
}
static bool mapping_verified(const EditorCollisionMigration *plan) {
    for(size_t i = 0; i < plan->profile_count; ++i)
        for(size_t j = 0; j < plan->profile_count; ++j) {
            const MigrationProfile *a = &plan->profiles[i], *b = &plan->profiles[j];
            bool old = allowed(a, b);
            if(((a->new_filter & b->new_category) != 0) != old ||
                    ((b->new_filter & a->new_category) != 0) != old) return false;
        }
    return true;
}
static EditorResult channels_build(EditorCollisionMigration *plan) {
    /* A channel (c,f) means category contains c AND filter contains f.
     * Responding to the reverse channel (f,c) is exactly the old mutual rule.
     * This supports many distinct neighborhoods without needing a bit per row.
     * Channels with identical carrier sets share one bit. */
    uint64_t carriers[64][MIGRATION_PROFILES_MAX / 64] = {{0}};
    size_t words = (plan->profile_count + 63) / 64, count = 0;
    memset(plan->names, 0, sizeof(plan->names));
    for(size_t i = 0; i < plan->profile_count; ++i)
        plan->profiles[i].new_category = plan->profiles[i].new_filter = 0;
    for(unsigned c = 0; c < 64; ++c) for(unsigned f = 0; f < 64; ++f) {
        uint64_t signature[MIGRATION_PROFILES_MAX / 64] = {0};
        bool forward = false, reverse = false;
        for(size_t i = 0; i < plan->profile_count; ++i) {
            const MigrationProfile *p = &plan->profiles[i];
            if((p->category & (UINT64_C(1) << c)) && (p->filter & (UINT64_C(1) << f))) {
                signature[i / 64] |= UINT64_C(1) << (i % 64); forward = true;
            }
            reverse |= (p->category & (UINT64_C(1) << f)) && (p->filter & (UINT64_C(1) << c));
        }
        if(!forward || !reverse) continue;
        size_t bit = 0;
        while(bit < count && memcmp(signature, carriers[bit], words * sizeof(*signature))) ++bit;
        if(bit == count) {
            if(count == 64) return editor_result_error(EDITOR_ERROR_CAPACITY,
                "Cannot encode migration within 64 bits using category retention, neighborhood grouping or legacy bit-pair channels; no files changed");
            memcpy(carriers[count], signature, words * sizeof(*signature));
            snprintf(plan->names[count], EDITOR_OBJECT_NAME_MAX, "migrated_bit_%u_%.*s", (unsigned)count,
                EDITOR_OBJECT_NAME_MAX - 32, c < plan->old_name_count ? plan->old_names[c] : "category");
            ++count;
        }
        for(size_t i = 0; i < plan->profile_count; ++i) {
            MigrationProfile *p = &plan->profiles[i];
            if(signature[i / 64] & (UINT64_C(1) << (i % 64))) p->new_category |= UINT64_C(1) << bit;
            if((p->category & (UINT64_C(1) << f)) && (p->filter & (UINT64_C(1) << c)))
                p->new_filter |= UINT64_C(1) << bit;
        }
    }
    plan->reassigned = true;
    plan->name_count = count;
    return mapping_verified(plan) ? editor_result_value(true) : invalid("Bit-pair collision equivalence audit failed");
}
static EditorResult mapping_build(EditorCollisionMigration *plan) {
    qsort(plan->profiles, plan->profile_count, sizeof(plan->profiles[0]), profile_compare);
    /* First retain all categories. A response bit is safe only when every
     * profile carrying it was mutually accepted. This often needs no split. */
    uint64_t used = 0;
    for(size_t i = 0; i < plan->profile_count; ++i) used |= plan->profiles[i].category;
    for(size_t i = 0; i < plan->profile_count; ++i) {
        MigrationProfile *p = &plan->profiles[i];
        p->new_category = p->category;
        p->new_filter = used;
        for(size_t j = 0; j < plan->profile_count; ++j)
            if(!allowed(p, &plan->profiles[j])) p->new_filter &= ~plan->profiles[j].category;
    }
    memcpy(plan->names, plan->old_names, sizeof(plan->names));
    plan->name_count = plan->old_name_count;
    if(mapping_verified(plan)) {
        for(size_t bit = 0; bit < 64; ++bit) if(used & (UINT64_C(1) << bit)) {
            while(plan->name_count <= bit) {
                unsigned suffix = 0;
                bool duplicate;
                do {
                    snprintf(plan->names[plan->name_count], EDITOR_OBJECT_NAME_MAX,
                        "migrated_bit_%zu_%u", plan->name_count, suffix++);
                    duplicate = false;
                    for(size_t n = 0; n < plan->name_count; ++n)
                        if(!strcmp(plan->names[n], plan->names[plan->name_count])) duplicate = true;
                } while(duplicate);
                ++plan->name_count;
            }
        }
        return editor_result_value(true);
    }
    /* Equal adjacency rows may share one bit even with different old masks.
     * The fallback is deterministic, bounded, and does not solve the general
     * minimum biclique-cover problem. Never silently approximate relationships. */
    size_t words = (plan->profile_count + 63) / 64;
    uint64_t *rows = calloc(plan->profile_count * words, sizeof(*rows));
    if(!rows) return invalid("Cannot allocate collision relationship audit");
    for(size_t i = 0; i < plan->profile_count; ++i)
        for(size_t j = 0; j < plan->profile_count; ++j)
            if(allowed(&plan->profiles[i], &plan->profiles[j])) rows[i * words + j / 64] |= UINT64_C(1) << (j % 64);
    size_t representatives[64], classes = 0, groups[MIGRATION_PROFILES_MAX];
    for(size_t i = 0; i < plan->profile_count; ++i) {
        size_t g = 0;
        while(g < classes && memcmp(rows + i * words, rows + representatives[g] * words, words * sizeof(*rows))) ++g;
        if(g == classes) {
            if(classes == 64) {
                free(rows);
                return channels_build(plan);
            }
            representatives[classes++] = i;
        }
        groups[i] = g;
    }
    free(rows);
    plan->reassigned = true;
    uint64_t allocated = 0, bits[64] = {0};
    /* Prefer an original single bit for a class before allocating free bits. */
    for(size_t g = 0; g < classes; ++g) {
        uint64_t c = plan->profiles[representatives[g]].category;
        if(c && !(c & (c - 1)) && !(allocated & c)) bits[g] = c, allocated |= c;
    }
    for(size_t g = 0; g < classes; ++g) if(!bits[g]) {
        for(unsigned b = 0; b < 64; ++b) if(!(allocated & (UINT64_C(1) << b))) {
            bits[g] = UINT64_C(1) << b;
            allocated |= bits[g];
            break;
        }
    }
    memset(plan->names, 0, sizeof(plan->names));
    plan->name_count = 0;
    for(size_t g = 0; g < classes; ++g) {
        unsigned bit = 0;
        while(bits[g] != (UINT64_C(1) << bit)) ++bit;
        const MigrationProfile *p = &plan->profiles[representatives[g]];
        unsigned old = 0;
        while(old < 64 && !(p->category & (UINT64_C(1) << old))) ++old;
        const char *base = old < plan->old_name_count ? plan->old_names[old] : "category";
        snprintf(plan->names[bit], EDITOR_OBJECT_NAME_MAX, "migrated_bit_%u_%.*s", bit, EDITOR_OBJECT_NAME_MAX - 32, base);
        if(plan->name_count <= bit) plan->name_count = bit + 1;
    }
    /* Keep unused legacy names where possible; otherwise the mapping report
     * retains their meaning as a union of the new profile bits. */
    if(plan->name_count < plan->old_name_count) plan->name_count = plan->old_name_count;
    for(size_t b = 0; b < plan->name_count; ++b) if(!plan->names[b][0]) {
        if(b < plan->old_name_count) snprintf(plan->names[b], EDITOR_OBJECT_NAME_MAX,
            "migrated_bit_%u_%.*s", (unsigned)b, EDITOR_OBJECT_NAME_MAX - 32, plan->old_names[b]);
        else snprintf(plan->names[b], EDITOR_OBJECT_NAME_MAX, "unused_bit_%zu", b);
    }
    for(size_t i = 0; i < plan->profile_count; ++i) {
        plan->profiles[i].new_category = bits[groups[i]];
        plan->profiles[i].new_filter = 0;
        for(size_t j = 0; j < plan->profile_count; ++j)
            if(allowed(&plan->profiles[i], &plan->profiles[j])) plan->profiles[i].new_filter |= bits[groups[j]];
    }
    return mapping_verified(plan) ? editor_result_value(true) : invalid("Collision equivalence audit failed");
}

static EditorResult documents_update(EditorCollisionMigration *plan) {
    for(size_t r = 0; r < plan->reference_count; ++r) {
        MigrationReference *ref = &plan->references[r];
        MigrationProfile key = {.category = ref->category, .filter = ref->filter};
        MigrationProfile *p = bsearch(&key, plan->profiles, plan->profile_count, sizeof(*p), profile_compare);
        yyjson_mut_doc *doc = plan->files[ref->file].json;
        if(!p || !put(doc, ref->value, ref->category_key, yyjson_mut_uint(doc, p->new_category)) ||
                !put(doc, ref->value, "responds_to", yyjson_mut_uint(doc, p->new_filter)))
            return invalid("Cannot allocate migrated masks");
        yyjson_mut_obj_remove_key(ref->value, ref->filter_key);
    }
    for(size_t i = 0; i < plan->file_count; ++i) {
        MigrationFile *f = &plan->files[i];
        yyjson_mut_val *root = yyjson_mut_doc_get_root(f->json);
        if(!put(f->json, root, f->project ? "format_version" : "version", yyjson_mut_uint(f->json, f->project ? 12 : 10)))
            return invalid("Cannot allocate migrated version");
        if(f->project) {
            yyjson_mut_val *names = yyjson_mut_arr(f->json);
            if(!names) return invalid("Cannot allocate migrated names");
            for(size_t n = 0; n < plan->name_count; ++n)
                if(!yyjson_mut_arr_add_strcpy(f->json, names, plan->names[n])) return invalid("Cannot allocate category name");
            if(!put(f->json, root, "collision_masks", names)) return invalid("Cannot allocate category registry");
        }
        f->output = yyjson_mut_write(f->json, YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_UNICODE, &f->output_size);
        if(!f->output) return invalid("Cannot serialize migrated file");
    }
    return editor_result_value(true);
}

static EditorResult report_build(EditorCollisionMigration *plan) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    if(!doc) return invalid("Cannot allocate migration report");
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_val *profiles = yyjson_mut_arr(doc), *files = yyjson_mut_arr(doc), *names = yyjson_mut_arr(doc);
    yyjson_mut_val *registry = yyjson_mut_arr(doc);
    bool ok = root && profiles && files && names && registry &&
        yyjson_mut_obj_add_bool(doc, root, "pair_equivalence_verified", true) &&
        yyjson_mut_obj_add_bool(doc, root, "categories_reassigned", plan->reassigned) &&
        yyjson_mut_obj_add_uint(doc, root, "ordered_profile_pairs_verified", plan->profile_count * plan->profile_count) &&
        yyjson_mut_obj_add_str(doc, root, "scope", "Only supplied files and profiles. Include all interacting files in one migration. Unseen C-created entities and later mask edits require manual review.") &&
        yyjson_mut_obj_add_str(doc, root, "c_code", "C files are unchanged. Replace legacy API names and numeric category/filter constants using each profile mapping; a split category cannot be substituted globally without its filter context. Regenerate editor-owned C after applying.") &&
        yyjson_mut_obj_add_str(doc, root, "recovery", "Close the editor/application before apply. Originals are preserved beside each input as .collision_original. If interrupted during replacement, restore every listed original before retrying; do not mix versions.") &&
        yyjson_mut_obj_add_val(doc, root, "files", files) &&
        yyjson_mut_obj_add_val(doc, root, "profiles", profiles) &&
        yyjson_mut_obj_add_val(doc, root, "named_categories", names) &&
        yyjson_mut_obj_add_val(doc, root, "new_category_registry", registry);
    for(size_t b = 0; ok && b < plan->name_count; ++b) {
        yyjson_mut_val *v = yyjson_mut_obj(doc);
        ok = v && yyjson_mut_obj_add_strcpy(doc, v, "name", plan->names[b]) &&
            yyjson_mut_obj_add_uint(doc, v, "bit", UINT64_C(1) << b) && yyjson_mut_arr_append(registry, v);
    }
    for(size_t i = 0; ok && i < plan->file_count; ++i) {
        MigrationFile *f = &plan->files[i];
        yyjson_mut_val *v = yyjson_mut_obj(doc);
        ok = v && yyjson_mut_obj_add_strcpy(doc, v, "path", f->path) &&
            yyjson_mut_obj_add_strcpy(doc, v, "original", f->backup) &&
            yyjson_mut_obj_add_uint(doc, v, "from_version", f->project ? 11 : 9) &&
            yyjson_mut_obj_add_uint(doc, v, "to_version", f->project ? 12 : 10) && yyjson_mut_arr_append(files, v);
    }
    for(size_t i = 0; ok && i < plan->profile_count; ++i) {
        MigrationProfile *p = &plan->profiles[i];
        yyjson_mut_val *v = yyjson_mut_obj(doc);
        ok = v && yyjson_mut_obj_add_uint(doc, v, "old_category", p->category) &&
            yyjson_mut_obj_add_uint(doc, v, "old_filter", p->filter) &&
            yyjson_mut_obj_add_uint(doc, v, "category", p->new_category) &&
            yyjson_mut_obj_add_uint(doc, v, "responds_to", p->new_filter) && yyjson_mut_arr_append(profiles, v);
    }
    for(size_t b = 0; ok && b < plan->old_name_count; ++b) {
        uint64_t mapped = 0;
        for(size_t i = 0; i < plan->profile_count; ++i)
            if(plan->profiles[i].category & (UINT64_C(1) << b)) mapped |= plan->profiles[i].new_category;
        yyjson_mut_val *v = yyjson_mut_obj(doc);
        ok = v && yyjson_mut_obj_add_strcpy(doc, v, "old_name", plan->old_names[b]) &&
            yyjson_mut_obj_add_uint(doc, v, "old_bit", UINT64_C(1) << b) &&
            yyjson_mut_obj_add_uint(doc, v, "migrated_membership_union", mapped) && yyjson_mut_arr_append(names, v);
    }
    if(ok) plan->report = yyjson_mut_write(doc, YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_UNICODE, NULL);
    yyjson_mut_doc_free(doc);
    return plan->report ? editor_result_value(true) : invalid("Cannot serialize migration report");
}

void editor_collision_migration_destroy(EditorCollisionMigration *plan) {
    if(!plan) return;
    for(size_t i = 0; i < plan->file_count; ++i) {
        SDL_free(plan->files[i].original);
        free(plan->files[i].output);
        yyjson_mut_doc_free(plan->files[i].json);
    }
    free(plan->files);
    free(plan->references);
    free(plan->report);
    free(plan);
}
const char *editor_collision_migration_report_get(const EditorCollisionMigration *plan) {
    return plan ? plan->report : NULL;
}
EditorResult editor_collision_migration_prepare(const char *const *paths,
        size_t count, EditorCollisionMigration **out) {
    if(out) *out = NULL;
    if(!out || !paths || !count || count > MIGRATION_FILES_MAX)
        return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT, "Supply 1 to %u legacy JSON files", MIGRATION_FILES_MAX);
    EditorCollisionMigration *plan = calloc(1, sizeof(*plan));
    if(!plan) return invalid("Cannot allocate migration plan");
    plan->files = calloc(count, sizeof(*plan->files));
    EditorResult result = invalid("Cannot allocate migration inputs");
    if(!plan->files) goto fail;
    plan->file_count = count;
    for(size_t i = 0; i < count; ++i) {
        MigrationFile *f = &plan->files[i];
        if(!paths[i]) { result = invalid("Null migration path"); goto fail; }
#ifdef _WIN32
        char *absolute = _fullpath(NULL, paths[i], 0);
#else
        char *absolute = realpath(paths[i], NULL);
#endif
        if(!absolute) { result = editor_result_error(EDITOR_ERROR_FILE_IO, "Cannot resolve migration input: %s", paths[i]); goto fail; }
        bool fits = SDL_strlcpy(f->path, absolute, sizeof(f->path)) < sizeof(f->path);
        free(absolute);
        if(!fits || snprintf(f->backup, sizeof(f->backup), "%s.collision_original", f->path) >= (int)sizeof(f->backup) ||
                snprintf(f->pending, sizeof(f->pending), "%s.collision_pending", f->path) >= (int)sizeof(f->pending) ||
                snprintf(f->rollback, sizeof(f->rollback), "%s.collision_rollback", f->path) >= (int)sizeof(f->rollback)) {
            result = invalid("Migration path too long"); goto fail;
        }
        for(size_t j = 0; j < i; ++j) if(!strcmp(f->path, plan->files[j].path)) {
            result = invalid("Duplicate migration input path"); goto fail;
        }
        SDL_PathInfo info;
        if(!SDL_GetPathInfo(f->path, &info) || info.type != SDL_PATHTYPE_FILE || info.size > MIGRATION_FILE_BYTES_MAX ||
                !(f->original = SDL_LoadFile(f->path, &f->original_size)) || f->original_size > MIGRATION_FILE_BYTES_MAX) {
            result = editor_result_error(EDITOR_ERROR_FILE_IO, "Cannot read migration input (maximum 64 MiB): %s", f->path); goto fail;
        }
        yyjson_doc *doc = yyjson_read(f->original, f->original_size, 0);
        if(!doc) { result = invalid("Invalid migration JSON"); goto fail; }
        f->json = yyjson_doc_mut_copy(doc, NULL);
        yyjson_doc_free(doc);
        if(!f->json || !clean_json(yyjson_mut_doc_get_root(f->json), 0)) {
            result = invalid("Invalid migration JSON: duplicate keys, excessive nesting or allocation failure"); goto fail;
        }
        yyjson_mut_val *root = yyjson_mut_doc_get_root(f->json);
        yyjson_mut_val *pv = yyjson_mut_obj_get(root, "format_version"), *rv = yyjson_mut_obj_get(root, "version");
        f->project = pv != NULL;
        if((pv && rv) || !yyjson_mut_is_uint(pv ? pv : rv) || yyjson_mut_get_uint(pv ? pv : rv) != (pv ? 11u : 9u)) {
            result = editor_result_error(EDITOR_ERROR_SCHEMA_VERSION, "Migration only supports project 11 and runtime 9: %s", f->path); goto fail;
        }
    }
    /* Read projects first so the named registry also covers runtime inputs. */
    for(unsigned pass = 0; pass < 2; ++pass) for(size_t i = 0; i < count; ++i) {
        if(plan->files[i].project != (pass == 0)) continue;
        result = pass == 0 ? project_collect(plan, i) : runtime_collect(plan, i);
        if(editor_result_check(result)) goto fail;
    }
    result = mapping_build(plan); if(editor_result_check(result)) goto fail;
    result = documents_update(plan); if(editor_result_check(result)) goto fail;
    result = report_build(plan); if(editor_result_check(result)) goto fail;
    if(snprintf(plan->report_path, sizeof(plan->report_path), "%s.collision_migration.json", plan->files[0].path) >= (int)sizeof(plan->report_path)) {
        result = invalid("Migration report path too long"); goto fail;
    }
    *out = plan;
    return editor_result_value(true);
fail:
    editor_collision_migration_destroy(plan);
    return result;
}

static bool exists(const char *path) { SDL_PathInfo info; return SDL_GetPathInfo(path, &info); }
static bool write_exclusive(const char *path, const void *data, size_t size) {
    FILE *file = fopen(path, "wbx");
    if(!file) return false;
    bool ok = fwrite(data, 1, size, file) == size;
    ok &= fclose(file) == 0;
    if(!ok) (void)SDL_RemovePath(path);
    return ok;
}
static bool unchanged(const MigrationFile *f) {
    size_t size = 0;
    void *bytes = SDL_LoadFile(f->path, &size);
    bool ok = bytes && size == f->original_size && !memcmp(bytes, f->original, size);
    SDL_free(bytes);
    return ok;
}
EditorResult editor_collision_migration_apply(EditorCollisionMigration *plan) {
    if(!plan || !plan->report) return invalid("No prepared migration plan");
    if(exists(plan->report_path)) return editor_result_error(EDITOR_ERROR_FILE_IO, "Migration report already exists: %s", plan->report_path);
    for(size_t i = 0; i < plan->file_count; ++i) {
        MigrationFile *f = &plan->files[i];
        if(!unchanged(f) || exists(f->backup) || exists(f->pending) || exists(f->rollback))
            return editor_result_error(EDITOR_ERROR_FILE_IO, "Input changed or recovery sidecar exists: %s", f->path);
    }
    bool report_created = false;
    const char *failed_path = plan->report_path;
    for(size_t i = 0; i < plan->file_count; ++i) {
        MigrationFile *f = &plan->files[i];
        failed_path = f->path;
        if(!(f->backup_created = write_exclusive(f->backup, f->original, f->original_size)) ||
                !(f->pending_created = write_exclusive(f->pending, f->output, f->output_size))) goto rollback;
    }
    failed_path = plan->report_path;
    if(!(report_created = write_exclusive(plan->report_path, plan->report, strlen(plan->report)))) goto rollback;
    for(size_t i = 0; i < plan->file_count; ++i) {
        MigrationFile *f = &plan->files[i];
        failed_path = f->path;
        if(!unchanged(f) || exists(f->rollback) || !SDL_RenamePath(f->path, f->rollback)) goto rollback;
        f->moved = true;
        if(!SDL_RenamePath(f->pending, f->path)) goto rollback;
        f->pending_created = false;
        f->installed = true;
    }
    for(size_t i = 0; i < plan->file_count; ++i) {
        if(!SDL_RemovePath(plan->files[i].rollback))
            return editor_result_error(EDITOR_ERROR_FILE_IO, "Migration applied; redundant rollback copy could not be removed: %s", plan->files[i].rollback);
        plan->files[i].moved = false;
    }
    return editor_result_value(true);
rollback:;
    bool restored = true;
    for(size_t i = plan->file_count; i-- > 0;) {
        MigrationFile *f = &plan->files[i];
        if(f->moved) {
            bool ok = SDL_RenamePath(f->rollback, f->path);
            restored &= ok;
            if(ok) f->moved = f->installed = false;
        }
        if(f->pending_created) { (void)SDL_RemovePath(f->pending); f->pending_created = false; }
    }
    /* Backups and report remain if restoration itself failed. */
    if(restored) {
        for(size_t i = 0; i < plan->file_count; ++i) if(plan->files[i].backup_created) {
            (void)SDL_RemovePath(plan->files[i].backup); plan->files[i].backup_created = false;
        }
        if(report_created) (void)SDL_RemovePath(plan->report_path);
    }
    return editor_result_error(EDITOR_ERROR_FILE_IO, restored ?
        "Migration failed at %s; input files restored" :
        "Migration failed at %s; rollback incomplete: restore ALL inputs from retained .collision_original files", failed_path);
}
