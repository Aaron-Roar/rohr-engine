/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_collision_migration.h"
#include "editor_project.h"
#include "editor_workspace.h"
#include "rohr.h"
#include "yyjson/yyjson.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OK(call) assert(!editor_result_check(call))
#define ENGINE_OK(call) assert(!rohr_error_check(call))
static unsigned rename_failure, rename_count;
#ifdef ROHR_TEST_RENAME_WRAP
extern bool __real_SDL_RenamePath(const char *, const char *);
bool __wrap_SDL_RenamePath(const char *a, const char *b) {
    if(rename_failure && ++rename_count == rename_failure) return false;
    return __real_SDL_RenamePath(a, b);
}
#endif
static void write_text(const char *path, const char *text) {
    FILE *f = fopen(path, "wb"); assert(f);
    assert(fwrite(text, 1, strlen(text), f) == strlen(text)); assert(fclose(f) == 0);
}
static void cleanup(const char *path) {
    const char *suffixes[] = {"", ".collision_original", ".collision_pending", ".collision_rollback", ".collision_migration.json"};
    for(size_t i = 0; i < sizeof(suffixes) / sizeof(*suffixes); ++i) {
        char p[1024]; snprintf(p, sizeof(p), "%s%s", path, suffixes[i]); (void)SDL_RemovePath(p);
    }
}
static char *read_text(const char *path) { char *s = SDL_LoadFile(path, NULL); assert(s); return s; }
static void unchanged_check(const char *path, const char *before) {
    char *after = read_text(path); assert(!strcmp(after, before)); SDL_free(after);
}
static void legacy_keys(yyjson_mut_doc *doc, yyjson_mut_val *value, bool project) {
    if(yyjson_mut_is_obj(value)) {
        yyjson_mut_val *filter = yyjson_mut_obj_remove_key(value, "responds_to");
        if(filter) assert(yyjson_mut_obj_add_val(doc, value, project ? "collision_with" : "collides_with", filter));
        size_t i, n; yyjson_mut_val *k, *v;
        yyjson_mut_obj_foreach(value, i, n, k, v) { (void)k; legacy_keys(doc, v, project); }
    } else if(yyjson_mut_is_arr(value)) {
        size_t i, n; yyjson_mut_val *v;
        yyjson_mut_arr_foreach(value, i, n, v) legacy_keys(doc, v, project);
    }
}
static void make_legacy(const char *path, bool project) {
    yyjson_doc *input = yyjson_read_file(path, 0, NULL, NULL); assert(input);
    yyjson_mut_doc *doc = yyjson_doc_mut_copy(input, NULL); assert(doc); yyjson_doc_free(input);
    yyjson_mut_val *root = yyjson_mut_doc_get_root(doc);
    assert(yyjson_mut_set_uint(yyjson_mut_obj_get(root, project ? "format_version" : "version"), project ? 11 : 9));
    legacy_keys(doc, root, project);
    assert(yyjson_mut_write_file(path, doc, YYJSON_WRITE_PRETTY, NULL, NULL)); yyjson_mut_doc_free(doc);
}
static EditorCollisionMigration *prepare(const char *const *paths, size_t count) {
    EditorCollisionMigration *plan = NULL;
    EditorResult r = editor_collision_migration_prepare(paths, count, &plan);
    if(editor_result_check(r)) editor_result_stderr_print(r);
    assert(!editor_result_check(r)); assert(plan); return plan;
}
static void migration_report_assert(EditorCollisionMigration *plan, bool remapped) {
    const char *text = editor_collision_migration_report_get(plan);
    yyjson_doc *doc = yyjson_read(text, strlen(text), 0); assert(doc);
    yyjson_val *root = yyjson_doc_get_root(doc), *profiles = yyjson_obj_get(root, "profiles");
    assert(yyjson_get_bool(yyjson_obj_get(root, "pair_equivalence_verified")));
    assert(yyjson_get_bool(yyjson_obj_get(root, "categories_reassigned")) == remapped);
    for(size_t i = 0; i < yyjson_arr_size(profiles); ++i) for(size_t j = 0; j < yyjson_arr_size(profiles); ++j) {
        yyjson_val *a = yyjson_arr_get(profiles, i), *b = yyjson_arr_get(profiles, j);
        bool old = (yyjson_get_uint(yyjson_obj_get(a, "old_filter")) & yyjson_get_uint(yyjson_obj_get(b, "old_category"))) &&
            (yyjson_get_uint(yyjson_obj_get(b, "old_filter")) & yyjson_get_uint(yyjson_obj_get(a, "old_category")));
        bool current = (yyjson_get_uint(yyjson_obj_get(a, "responds_to")) & yyjson_get_uint(yyjson_obj_get(b, "category"))) != 0;
        assert(old == current);
    }
    yyjson_doc_free(doc);
}
static void state_write(const char *path, const uint64_t *categories, const uint64_t *filters, size_t count) {
    FILE *f = fopen(path, "wb"); assert(f);
    fputs("{\"version\":9,\"entities\":[", f);
    for(size_t i = 0; i < count; ++i) fprintf(f,
        "%s{\"name\":\"body%zu\",\"components\":{\"collision_filter\":{\"category\":%" PRIu64 ",\"collides_with\":%" PRIu64 "}}}",
        i ? "," : "", i, categories[i], filters[i]);
    fputs("]}", f); assert(!fclose(f));
}
static void runtime_cases(void) {
    const char *paths[] = {"migration_state_a.json", "migration_state_b.json"};
    cleanup(paths[0]); cleanup(paths[1]);
    uint64_t categories[] = {1, 1, 2, 2}, filters[] = {2, 0, 1, 0};
    state_write(paths[0], categories, filters, 4);
    write_text(paths[1], "{\"version\":9,\"entities\":[{\"name\":\"implicit\",\"components\":{}}]}");
    char *before_a = read_text(paths[0]), *before_b = read_text(paths[1]);
    EditorCollisionMigration *p = prepare(paths, 2);
    migration_report_assert(p, true);
    unchanged_check(paths[0], before_a); unchanged_check(paths[1], before_b);
    EditorCollisionMigration *again = prepare(paths, 2);
    assert(!strcmp(editor_collision_migration_report_get(p), editor_collision_migration_report_get(again)));
    editor_collision_migration_destroy(again);
    write_text(paths[1], "changed");
    assert(editor_result_check(editor_collision_migration_apply(p)));
    unchanged_check(paths[0], before_a);
    write_text(paths[1], before_b);
    write_text("migration_state_b.json.collision_original", "occupied");
    assert(editor_result_check(editor_collision_migration_apply(p)));
    unchanged_check(paths[0], before_a); unchanged_check("migration_state_b.json.collision_original", "occupied");
    assert(SDL_RemovePath("migration_state_b.json.collision_original"));
#ifdef ROHR_TEST_RENAME_WRAP
    /* Fail after file one is installed, then restore the entire batch. */
    rename_failure = 4; rename_count = 0;
    assert(editor_result_check(editor_collision_migration_apply(p)));
    rename_failure = 0;
    unchanged_check(paths[0], before_a); unchanged_check(paths[1], before_b);
#else
    (void)rename_failure; (void)rename_count;
#endif
    OK(editor_collision_migration_apply(p));
    unchanged_check("migration_state_a.json.collision_original", before_a);
    unchanged_check("migration_state_b.json.collision_original", before_b);
    ENGINE_OK(rohr_engine_start());
    ENGINE_OK(rohr_game_state_files_load(paths, 2));
    Entity entities[5];
    for(size_t i = 0; i < 5; ++i) {
        char name[32]; snprintf(name, sizeof(name), "body%zu", i);
        EntityResult e = rohr_entity_by_name_get(i == 4 ? "implicit" : name); ENGINE_OK(e); entities[i] = e.result.value;
    }
    uint64_t cats[] = {1,1,2,2,1}, masks[] = {2,0,1,0,UINT64_MAX};
    for(size_t i = 0; i < 5; ++i) for(size_t j = 0; j < 5; ++j) if(i != j) {
        bool old = (cats[i] & masks[j]) && (cats[j] & masks[i]);
        assert(rohr_physics_collision_response_to_check(entities[i], entities[j]) == old);
    }
    ENGINE_OK(rohr_game_state_file_save("migration_roundtrip.json"));
    rohr_engine_stop(); ENGINE_OK(rohr_engine_start());
    ENGINE_OK(rohr_game_state_file_load("migration_roundtrip.json"));
    rohr_engine_stop();
    editor_collision_migration_destroy(p);
    SDL_free(before_a); SDL_free(before_b);
    cleanup(paths[0]); cleanup(paths[1]); cleanup("migration_roundtrip.json");
}
static void project_case(void) {
    const char *path = "migration_project.json"; cleanup(path);
    EditorProject *project = calloc(1, sizeof(*project)); assert(project); editor_project_init(project);
    assert(editor_project_collision_mask_add(project, "second", NULL));
    EditorObject *object = editor_project_object_add(project, (Position){0}); assert(object);
    snprintf(object->name, sizeof(object->name), "Migration");
    uint64_t cats[] = {1,1,2}, masks[] = {2,0,1};
    for(size_t i = 0; i < 3; ++i) {
        EditorRigidBody *body = editor_project_rigid_body_add(project, object); assert(body);
        snprintf(body->name, sizeof(body->name), "body%zu", i);
        body->collision_category = cats[i]; body->responds_to = masks[i];
        if(i == 1) body->particle = body->standalone_particle = true;
    }
    EditorSoftBody *soft = editor_project_soft_body_add(project, object); assert(soft);
    EditorSoftNode *node = editor_project_soft_node_add(project, soft, (Position){0}); assert(node);
    EditorSoftNodeId a = node->id;
    node->collision_category = 1; node->responds_to = 2;
    node = editor_project_soft_node_add(project, soft, (Position){20,0}); assert(node);
    EditorSoftNodeId b = node->id;
    node->collision_category = 2; node->responds_to = 1;
    EditorSoftBeam *beam = editor_project_soft_beam_add(project, soft, a, b); assert(beam);
    beam->collision_category = 1; beam->responds_to = 0; beam->collision_enabled = false;
    assert(editor_project_save(project, path)); editor_project_destroy(project);
    make_legacy(path, true);
    char *before = read_text(path);
    EditorCollisionMigration *plan = prepare(&path, 1); migration_report_assert(plan, true);
    OK(editor_collision_migration_apply(plan)); editor_collision_migration_destroy(plan);
    unchanged_check("migration_project.json.collision_original", before); SDL_free(before);
    OK(editor_project_load(project, path));
    assert(project->objects[0].rigid_body_count == 3);
    EditorRigidBody *bodies = project->objects[0].rigid_bodies;
    for(size_t i = 0; i < 3; ++i) for(size_t j = 0; j < 3; ++j)
        assert(((bodies[i].responds_to & bodies[j].collision_category) != 0) ==
            ((cats[i] & masks[j]) != 0 && (cats[j] & masks[i]) != 0));
    assert(!project->objects[0].soft_body_items[0].beams[0].collision_enabled);
    assert(editor_project_save(project, "migration_project_roundtrip.json"));
    editor_project_destroy(project);
    OK(editor_project_load(project, "migration_project_roundtrip.json"));
    editor_project_destroy(project); free(project);
    /* Keep migrated project for the CLI/generated consumer integration case. */
    cleanup("migration_project_roundtrip.json");
}
static void limits_and_errors(void) {
    const char *path = "migration_limits.json"; cleanup(path);
    const char *bad[] = {
        "{\"version\":8,\"entities\":[]}", "{\"version\":10,\"entities\":[]}",
        "{\"version\":9,\"version\":9,\"entities\":[]}",
        "{\"version\":9,\"entities\":[{\"components\":{\"collision_filter\":{}}}]}",
        "{\"format_version\":11,\"collision_masks\":[\"default\"],\"objects\":[{\"rigid_bodies\":[{\"collision_enabled\":true}]}]}",
        "{\"version\":9,\"entities\":[{\"components\":{\"collision_filter\":{\"category\":-1,\"collides_with\":1}}}]}",
        "{\"version\":9,\"entities\":[{\"components\":{\"collision_filter\":{\"category\":1,\"responds_to\":1}}}]}"
    };
    for(size_t i = 0; i < sizeof(bad) / sizeof(*bad); ++i) {
        write_text(path, bad[i]); EditorCollisionMigration *p = NULL;
        assert(editor_result_check(editor_collision_migration_prepare(&path, 1, &p))); assert(!p);
        unchanged_check(path, bad[i]);
    }
    uint64_t categories[130], masks[130];
    /* All 64 bits, including the reserved high bit, retain exact numeric values. */
    for(unsigned i = 0; i < 64; ++i) categories[i] = masks[i] = UINT64_C(1) << i;
    state_write(path, categories, masks, 64);
    EditorCollisionMigration *p = prepare(&path, 1); migration_report_assert(p, false); editor_collision_migration_destroy(p);
    /* Distinct masks can share adjacency rows: 130 profiles need only one bit. */
    for(unsigned i = 0; i < 130; ++i) categories[i] = 1, masks[i] = (uint64_t)i << 1;
    state_write(path, categories, masks, 130);
    p = prepare(&path, 1); migration_report_assert(p, false); editor_collision_migration_destroy(p);
    /* 130 neighborhoods over seven old bits fit into at most 49 bit-pair
     * channels, despite failing both category retention and row grouping. */
    for(unsigned i = 0; i < 130; ++i) {
        categories[i] = UINT64_C(1) << (i % 7);
        masks[i] = (i * 71u + 23u) & 127u;
    }
    state_write(path, categories, masks, 130);
    p = prepare(&path, 1); migration_report_assert(p, true); editor_collision_migration_destroy(p);
    /* 33 isolated pairs have 66 singleton response neighborhoods, excluding
     * self-profile contacts. Every responding target needs a distinct bit:
     * sharing one introduces a forbidden pair. Nine legacy bits express this
     * via reversed single-bit category/filter pairs, but 64 directional bits
     * cannot preserve both response directions and all exclusions. */
    size_t count = 0;
    for(unsigned a = 0; a < 9 && count < 66; ++a)
        for(unsigned b = a + 1; b < 9 && count < 66; ++b) {
            categories[count] = UINT64_C(1) << a; masks[count++] = UINT64_C(1) << b;
            categories[count] = UINT64_C(1) << b; masks[count++] = UINT64_C(1) << a;
        }
    assert(count == 66);
    state_write(path, categories, masks, 64);
    p = prepare(&path, 1); migration_report_assert(p, true); editor_collision_migration_destroy(p);
    state_write(path, categories, masks, count);
    char *before = read_text(path);
    EditorResult r = editor_collision_migration_prepare(&path, 1, &p);
    assert(editor_result_check(r) && r.result.error.code == EDITOR_ERROR_CAPACITY && !p);
    unchanged_check(path, before); SDL_free(before);
    const char *duplicate[] = {path, path};
    assert(editor_result_check(editor_collision_migration_prepare(duplicate, 2, &p)));
    cleanup(path);
}
static void snapshot_case(void) {
    const char *path = "migration_snapshot.json"; cleanup(path);
    ENGINE_OK(rohr_engine_start());
    EntityResult root = rohr_physics_soft_body_add((SoftBodyDescription){0}); ENGINE_OK(root);
    ENGINE_OK(rohr_entity_name_set(root.result.value, "soft"));
    SoftBodyNodeDescription node = rohr_physics_soft_body_node_description_default_get();
    node.owner = root.result.value;
    node.collision_filter = (CollisionFilterDescription){1,2};
    EntityResult a = rohr_physics_soft_body_node_add(node); ENGINE_OK(a);
    ENGINE_OK(rohr_entity_name_set(a.result.value, "a"));
    node.position.x = 20; node.collision_filter = (CollisionFilterDescription){2,1};
    EntityResult b = rohr_physics_soft_body_node_add(node); ENGINE_OK(b);
    ENGINE_OK(rohr_entity_name_set(b.result.value, "b"));
    SoftBodyBeamDescription beam = rohr_physics_soft_body_beam_description_default_get();
    beam.owner = root.result.value; beam.node_a = a.result.value; beam.node_b = b.result.value;
    beam.collision.filter = (CollisionFilterDescription){1,0};
    EntityResult edge = rohr_physics_soft_body_beam_add(beam); ENGINE_OK(edge);
    ENGINE_OK(rohr_entity_name_set(edge.result.value, "edge"));
    ENGINE_OK(rohr_game_state_file_save(path)); rohr_engine_stop();
    make_legacy(path, false);
    EditorCollisionMigration *p = prepare(&path, 1); migration_report_assert(p, true);
    OK(editor_collision_migration_apply(p)); editor_collision_migration_destroy(p);
    ENGINE_OK(rohr_engine_start()); ENGINE_OK(rohr_game_state_file_load(path));
    EntityResult ar = rohr_entity_by_name_get("a"), br = rohr_entity_by_name_get("b"), er = rohr_entity_by_name_get("edge");
    ENGINE_OK(ar); ENGINE_OK(br); ENGINE_OK(er);
    assert(rohr_physics_collision_response_to_check(ar.result.value, br.result.value));
    assert(rohr_physics_collision_response_to_check(br.result.value, ar.result.value));
    assert(!rohr_physics_collision_response_between_check(ar.result.value, er.result.value));
    assert(!rohr_physics_collision_response_between_check(br.result.value, er.result.value));
    ENGINE_OK(rohr_game_state_file_save("migration_snapshot_roundtrip.json"));
    rohr_engine_stop(); ENGINE_OK(rohr_engine_start());
    ENGINE_OK(rohr_game_state_file_load("migration_snapshot_roundtrip.json")); rohr_engine_stop();
    cleanup(path); cleanup("migration_snapshot_roundtrip.json");
}
int main(int argc, char **argv) {
    runtime_cases(); project_case(); limits_and_errors();
    snapshot_case();
    if(argc == 2) {
        EditorProject *starter = calloc(1, sizeof(*starter)); assert(starter);
        /* Start the CLI integration from the actual old masks, not a second
         * migration of already-remapped values. */
        EditorWorkspace workspace = {0};
        OK(editor_workspace_create(&workspace, starter, argv[1]));
        editor_project_destroy(starter); free(starter);
        char *legacy = read_text("migration_project.json.collision_original");
        char path[1024]; OK(editor_workspace_path_get(&workspace, EDITOR_PATH_STATE, path, sizeof(path)));
        write_text(path, legacy); SDL_free(legacy);
    }
    cleanup("migration_project.json");
    puts("Collision migration: project/runtime parity, exact masks, deterministic preview, round trips and rollback passed");
    return 0;
}
