/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_workspace.h"
#include "editor_command.h"
#include "entity/object_internal.h"
#include "incomplete_joint_checks.h"
#include <string.h>

static void author(EditorProject *project) {
    editor_project_init(project);
    EditorObject *object = editor_project_object_add(project, (Position){0}); assert(object);
    snprintf(object->name, sizeof(object->name), "Drafts");
    EditorRigidBody *body = editor_project_rigid_body_add(project, object); assert(body);
    body->gravity_enabled = false; body->collision_enabled = false;
    EditorAnchor *anchor = editor_project_anchor_add(project, object, (Position){0}, body->id); assert(anchor);
    EditorAnchorId rigid = anchor->id;
    anchor = editor_project_anchor_add(project, object, (Position){10, 0}, 0); assert(anchor);
    EditorAnchorId world = anchor->id;
    EditorSoftBody *soft = editor_project_soft_body_add(project, object); assert(soft);
    EditorSoftNode *node = editor_project_soft_node_add(project, soft, (Position){5, 0}); assert(node);
    node->gravity_enabled = false; node->collision_enabled = false;
    anchor = editor_project_anchor_add(project, object, (Position){0}, 0); assert(anchor);
    anchor->attachment_kind = EDITOR_ANCHOR_ATTACHMENT_SOFT_NODE;
    anchor->attachment_soft_body = soft->id; anchor->attachment_soft_node = node->id;
    anchor->position_follows_body = true;
    EditorAnchorId node_anchor = anchor->id;
    const EditorJointKind kinds[] = {EDITOR_JOINT_REVOLUTE, EDITOR_JOINT_WELD, EDITOR_JOINT_SPRING};
    const EditorAnchorId a[] = {rigid, node_anchor, rigid}, b[] = {world, world, node_anchor};
    for(unsigned kind = 0; kind < 3; ++kind) for(unsigned endpoints = 0; endpoints < 4; ++endpoints) {
        EditorJoint *joint = editor_project_joint_add(project, object, kinds[kind]); assert(joint);
        snprintf(joint->name, sizeof(joint->name), "%s_%u", joint_names[kind], endpoints);
        joint->anchor_a = endpoints & 1 ? a[kind] : 0;
        joint->anchor_b = endpoints & 2 ? b[kind] : 0;
        joint->stiffness = 7; joint->damping = 2; joint->rest_length = 3; joint->rest_angle = 15;
        joint->rest_length_explicit = true; joint->rest_angle_explicit = true;
        joint->visuals.main = (ObjectVisualStyle){.specified = true, .fill_color = 0x123456ff,
            .border_color = 0x987654ff, .border_thickness = 2, .scale = {2, 3}, .offset = {4, 5}, .rotation = 10};
    }
}

static void invalid_checks(EditorProject *project) {
    EditorJoint *joint = &project->objects[0].joint_items[0];
    EditorJoint saved = *joint;
    for(unsigned bad = 0; bad < 6; ++bad) {
        *joint = saved;
        switch(bad) {
        case 0: joint->anchor_a = UINT32_MAX; break;
        case 1: joint->anchor_b = UINT32_MAX; break;
        case 2: joint->rest_length = -1; break;
        case 3: joint->stiffness = NAN; break;
        case 4: joint->damping = -1; break;
        case 5: joint->rest_angle = INFINITY; break;
        }
        ObjectDescriptionResult result = object_description_project_read(project, "draft_joints.json");
        assert(rohr_error_check(result));
    }
    *joint = saved;
}

int main(int argc, char **argv) {
    static EditorProject project, loaded; author(&project);
    assert(editor_project_save(&project, "draft_joints.json"));
    OK(editor_project_load(&loaded, "draft_joints.json"));
    assert(loaded.objects[0].joint_count == 12);
    for(unsigned i = 0; i < 12; ++i) {
        const EditorJoint *before = &project.objects[0].joint_items[i], *after = &loaded.objects[0].joint_items[i];
        assert(before->id == after->id && !strcmp(before->name, after->name));
        assert(before->anchor_a == after->anchor_a && before->anchor_b == after->anchor_b);
        assert(after->visuals.main.fill_color == 0x123456ff && after->visuals.main.offset.x == 4);
    }
    invalid_checks(&loaded);
    for(unsigned path = 0; path < 2; ++path) {
        OK(rohr_engine_start());
        ObjectDescriptionResult catalog = path ? rohr_object_load("draft_joints.json") :
            object_description_project_read(&loaded, "draft_joints.json"); OK(catalog);
        ObjectDescriptionResult draft = rohr_object_description_by_name_get(catalog.result.value, "pin_0"); OK(draft);
        assert(draft.result.value->endpoint_a == 0 && draft.result.value->endpoint_b == 0);
        assert(draft.result.value->visuals.main.fill_color == 0x123456ff &&
            draft.result.value->visuals.main.offset.x == 4);
        EntityResult instance = rohr_entity_object_by_name_add(catalog.result.value, "Drafts"); OK(instance);
        /* Runtime state persistence must retain incomplete bindings too. */
        EntityNameResult name = rohr_entity_name_get(instance.result.value); OK(name);
        OK(rohr_game_state_file_save("draft_joint_state.json"));
        OK(rohr_entity_delete(instance.result.value));
        OK(rohr_game_state_file_load("draft_joint_state.json"));
        instance = rohr_entity_by_name_get(name.result.value.value); OK(instance);
        draft_checks(draft_members(instance.result.value), instance.result.value);
        OK(rohr_entity_delete(instance.result.value));
        if(path) OK(rohr_object_destroy(catalog.result.value));
        else object_description_tree_destroy((ObjectDescription *)catalog.result.value);
        rohr_engine_stop();
    }
    /* The existing editor relationship command completes the same draft. */
    EditorObject *object = &loaded.objects[0]; EditorJoint *joint = &object->joint_items[0];
    EditorJointId id = joint->id;
    for(unsigned endpoint = 0; endpoint < 2; ++endpoint) {
        EditorCommand command = {.type = EDITOR_COMMAND_RELATIONSHIP_SET,
            .data.relationship_set = {EDITOR_RELATIONSHIP_JOINT_ANCHOR, object->id, 0, id,
                endpoint, object->anchors[endpoint].id}};
        OK(editor_command_execute(&loaded, &command));
    }
    assert(joint->id == id && joint->anchor_a && joint->anchor_b && object->joint_count == 12);
    if(argc == 2) {
        EditorWorkspace workspace = {0}; static EditorProject starter;
        OK(editor_workspace_create(&workspace, &starter, argv[1])); editor_project_destroy(&starter);
        assert(editor_workspace_save(&workspace, &project)); assert(editor_workspace_c_generate(&workspace, &project));
    }
    assert(remove("draft_joints.json") == 0); assert(remove("draft_joint_state.json") == 0);
    editor_project_destroy(&loaded); editor_project_destroy(&project);
    return 0;
}
