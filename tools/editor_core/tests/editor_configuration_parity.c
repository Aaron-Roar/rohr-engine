/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_command.h"
#include "editor_history.h"
#include "editor_workspace.h"
#include "editor_object_catalog.h"
#include "entity/object_internal.h"
#include "rohr.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define OK(value) assert((value).kind == ERROR_RESULT_VALUE)
static Entity member(Entity owner, const char *name) {
    EntityResult result = rohr_entity_object_member_by_name_get(owner, name);
    OK(result); return result.result.value;
}
static void settings_round_trip(void) {
    static EditorProject project, loaded;
    editor_project_init(&project); editor_project_init(&loaded);
    EditorObject *object = editor_project_object_add(&project, (Position){10, 20});
    assert(object); snprintf(object->name, sizeof(object->name), "fixture");
    EditorRigidBody *a = editor_project_rigid_body_add(&project, object);
    assert(a); snprintf(a->name, sizeof(a->name), "body");
    EditorRigidBodyId a_id = a->id;
    EditorSoftBody *soft = editor_project_soft_body_add(&project, object); assert(soft);
    snprintf(soft->name, sizeof(soft->name), "soft");
    EditorSoftNode *n1 = editor_project_soft_node_add(&project, soft, (Position){0});
    EditorSoftNode *n2 = editor_project_soft_node_add(&project, soft, (Position){3, 4});
    assert(n1 && n2); n1->static_body = true;
    EditorSoftBeam *beam = editor_project_soft_beam_add(&project, soft, n1->id, n2->id); assert(beam);
    snprintf(beam->name, sizeof(beam->name), "beam");
    beam->rest_length_explicit = true; beam->rest_length = 2.5f;
    EditorCamera *camera = editor_project_camera_add(&project, object);
    assert(camera); snprintf(camera->name, sizeof(camera->name), "camera");
    camera->position = (Position){7, 8};
    camera->attachment_kind = EDITOR_CAMERA_ATTACHMENT_RIGID_BODY;
    camera->attachment = a_id; camera->position_follow_disabled = true;
    camera->inherit_orientation = true; camera->pause_with_engine = true;
    a = editor_project_rigid_body_get(object, a_id);
    a->entity_settings = (ObjectEntitySettings){
        .lifetime = {.deadline = {.expiry_time = 25, .expiry_tick = 40,
            .time_enabled = true, .tick_enabled = true}},
        .angular_acceleration = 12, .angular_velocity_maximum = 30,
        .angular_velocity_maximum_enabled = true,
        .gameplay_target = {.kind = OBJECT_SOFT_BODY, .id = soft->id},
        .axis_lock_enabled = true, .axis_lock = {.axis = {1, 0}, .point_on_axis = {2, 3}}};
    assert(editor_project_save(&project, "configuration_parity.json"));
    OK(editor_project_load(&loaded, "configuration_parity.json"));
    EditorObject *copy = &loaded.objects[0];
    assert(copy->rigid_bodies[0].entity_settings.angular_acceleration == 12);
    assert(copy->soft_body_items[0].nodes[0].static_body);
    assert(copy->soft_body_items[0].beams[0].rest_length_explicit);
    assert(copy->cameras[0].position_follow_disabled && copy->cameras[0].pause_with_engine);
    ObjectDescriptionResult catalog = object_description_project_read(&loaded, "configuration_parity.json");
    OK(catalog);
    ObjectDescriptionResult found = object_description_name_get(catalog.result.value, copy->name);
    OK(found);
    const ObjectDescription *description = found.result.value;
    Time time = engine_time_get(); Tick tick = engine_tick_get();
    EntityResult first = entity_object_add(description); OK(first);
    EntityResult second = entity_object_add(description); OK(second);
    Entity body = member(first.result.value, "body"), other = member(second.result.value, "body");
    assert(body != other);
    EntityLifetimeDescription lifetime = entity_lifetime_description_get(body).result.value;
    assert(lifetime.expiry_time == time + 25 && lifetime.expiry_tick == tick + 40);
    EntityIndex index; assert(entity_index_get(body, &index));
    assert(angular_accelerations[index] == 12 && angular_velocity_maximums[index] == 30);
    assert(targets[index] == member(first.result.value, "soft"));
    AxisLockDescription axis = physics_axis_lock_description_get(body).result.value;
    assert(axis.point_on_axis.x == 12 && axis.point_on_axis.y == 23);
    Entity first_soft = member(first.result.value, "soft");
    assert(entity_components_check(member(first_soft, copy->soft_body_items[0].nodes[0].name), ROHR_STATIC));
    assert(physics_soft_body_beam_description_get(member(first_soft, "beam")).result.value.rest_length == 2.5f);
    CameraId camera_id = entity_object_camera_get(member(first.result.value, "camera")).result.value;
    CameraDescription camera_value = graphics_camera_description_get(camera_id).result.value;
    assert(camera_value.pause_with_engine && camera_value.position.x == 17 && camera_value.position.y == 28);
    EntityName first_name=entity_name_get(first.result.value).result.value;
    EntityName second_name=entity_name_get(second.result.value).result.value;
    OK(game_state_file_save("configuration_runtime.json"));
    OK(entity_delete(first.result.value)); OK(entity_delete(second.result.value));
    OK(game_state_file_load("configuration_runtime.json"));
    first=entity_by_name_get(first_name.value); second=entity_by_name_get(second_name.value); OK(first); OK(second);
    body=member(first.result.value,"body"); other=member(second.result.value,"body");
    assert(entity_index_get(body,&index));
    assert(angular_accelerations[index]==12 && angular_velocity_maximums[index]==30);
    assert(targets[index]==member(first.result.value,"soft"));
    assert(entity_lifetime_description_get(body).result.value.expiry_tick==tick+40);
    assert(physics_axis_lock_description_get(body).result.value.point_on_axis.x==12);
    first_soft=member(first.result.value,"soft");
    assert(entity_components_check(member(first_soft,copy->soft_body_items[0].nodes[0].name),ROHR_STATIC));
    assert(physics_soft_body_beam_description_get(member(first_soft,"beam")).result.value.rest_length==2.5f);
    camera_id=entity_object_camera_get(member(first.result.value,"camera")).result.value;
    camera_value=graphics_camera_description_get(camera_id).result.value;
    assert(camera_value.pause_with_engine && camera_value.position.x==17 && camera_value.position.y==28);
    CameraAttachmentDescription restored_attachment=graphics_camera_attachment_description_get(camera_id).result.value;
    assert(restored_attachment.entity==body && !restored_attachment.follow_position && restored_attachment.follow_orientation);
    OK(physics_angular_velocity_maximum_set(body,0));
    assert(physics_angular_velocity_maximum_get(other).result.value==30);
    OK(entity_delete(first.result.value));
    assert(entity_alive_check(other));
    OK(entity_delete(second.result.value));
    object_description_tree_destroy((ObjectDescription *)catalog.result.value);
    assert(remove("configuration_parity.json") == 0);
    assert(remove("configuration_runtime.json") == 0);
    editor_project_destroy(&loaded); editor_project_destroy(&project);
}

static void lifetime_modes(void) {
    ObjectDescription d = {.id = 1, .name = "expiry", .kind = OBJECT_CONTAINER,
        .categories = ROHR_ENTITY_OBJECT_ALL, .visible = true};
    const ObjectDescription *description = &d;
    for(unsigned mode = 0; mode < 4; ++mode) {
        d.entity_settings.lifetime = (ObjectLifetimeDescription){
            .deadline = {.expiry_time = 1000, .expiry_tick = 1000,
                .time_enabled = true, .tick_enabled = true},
            .time_absolute = (mode & 1) != 0, .tick_absolute = (mode & 2) != 0};
        Time time = engine_time_get(); Tick tick = engine_tick_get();
        EntityResult spawned = entity_object_add(description); OK(spawned);
        EntityLifetimeDescription value = entity_lifetime_description_get(spawned.result.value).result.value;
        assert(value.expiry_time == 1000 + ((mode & 1) ? 0 : time));
        assert(value.expiry_tick == 1000 + ((mode & 2) ? 0 : tick));
        OK(entity_delete(spawned.result.value));
    }
    d.entity_settings.lifetime = (ObjectLifetimeDescription){
        .deadline = {.time_enabled = true}, .time_absolute = true};
    EntityResult due = entity_object_add(description); OK(due);
    (void)engine_lifecycle_update();
    assert(!entity_alive_check(due.result.value));
    d.entity_settings.lifetime = (ObjectLifetimeDescription){
        .deadline = {.tick_enabled = true, .expiry_tick = UINT64_MAX}};
    size_t count = entity_alive_count_get();
    assert(error_check(entity_object_add(description)));
    assert(entity_alive_count_get() == count);
}

static void setting_commands(void) {
    static EditorProject p; editor_project_init(&p);
    EditorObject *o = editor_project_object_add(&p, (Position){0}); assert(o);
    EditorRigidBody *body = editor_project_rigid_body_add(&p, o); assert(body);
    char *args[] = {"rohr-cli", "rigid-body", "set", "p.json", "1", "1", "lifetime-tick", "18446744073709551615"};
    const char *path; EditorCommand command;
    char *standard[] = {"rohr-cli", "--project", "p.json", "--object-id", "1",
        "--body-id", "1", "--property", "lifetime-time-enabled", "true"};
    OK(editor_command_cli_standard_parse(&p, 10, standard, &path, &command));
    OK(editor_command_execute(&p, &command));
    assert(body->entity_settings.lifetime.deadline.time_enabled);
    OK(editor_command_cli_parse(8, args, &path, &command));
    OK(editor_command_execute(&p, &command));
    assert(body->entity_settings.lifetime.deadline.expiry_tick == UINT64_MAX);
    char cli[512]; OK(editor_command_cli_write(&command, "p.json", cli, sizeof(cli)));
    assert(strstr(cli, "18446744073709551615"));
    args[6] = "gameplay-target"; args[7] = "object:1";
    OK(editor_command_cli_parse(8, args, &path, &command)); OK(editor_command_execute(&p, &command));
    assert(body->entity_settings.gameplay_target.id == o->id);
    args[7] = "body:999"; OK(editor_command_cli_parse(8, args, &path, &command));
    assert(editor_command_execute(&p, &command).kind == ERROR_RESULT_ERROR);
    assert(body->entity_settings.gameplay_target.id == o->id);
    /* Every shared field has a CLI parser, mutation, and exact JSON value. */
#define TEXT_BOOL "true"
#define TEXT_FLOAT "1.25"
#define TEXT_DOUBLE "1.2345678901234567"
#define TEXT_UINT64 "18446744073709551615"
#define TEXT_REFERENCE "object:1"
#define SET(symbol, name, label, type, member_name) \
    args[6] = name; args[7] = TEXT_##type; \
    OK(editor_command_cli_parse(8, args, &path, &command)); \
    OK(editor_command_execute(&p, &command)); \
    OK(editor_command_cli_standard_write(&p, &command, NULL, "p.json", cli, sizeof(cli)));
    EDITOR_ENTITY_SETTING_PROPERTIES(SET)
#undef SET
    assert(editor_project_save(&p, "settings_commands.json"));
    static EditorProject loaded; editor_project_init(&loaded);
    OK(editor_project_load(&loaded, "settings_commands.json"));
    ObjectEntitySettings expected = body->entity_settings, actual = loaded.objects[0].rigid_bodies[0].entity_settings;
#define EQUAL_BOOL(a, b) ((a) == (b))
#define EQUAL_FLOAT EQUAL_BOOL
#define EQUAL_DOUBLE EQUAL_BOOL
#define EQUAL_UINT64 EQUAL_BOOL
#define EQUAL_REFERENCE(a, b) ((a).kind == (b).kind && (a).id == (b).id && (a).soft_body == (b).soft_body)
#define CHECK(symbol, name, label, type, member_name) assert(EQUAL_##type(expected.member_name, actual.member_name));
    EDITOR_ENTITY_SETTING_PROPERTIES(CHECK)
#undef CHECK
#undef EQUAL_REFERENCE
#undef EQUAL_UINT64
#undef EQUAL_DOUBLE
#undef EQUAL_FLOAT
#undef EQUAL_BOOL
#undef TEXT_REFERENCE
#undef TEXT_UINT64
#undef TEXT_DOUBLE
#undef TEXT_FLOAT
#undef TEXT_BOOL
    assert(remove("settings_commands.json") == 0); editor_project_destroy(&loaded);
    editor_project_destroy(&p);
}

static void history_execute(EditorHistory *history, EditorProject *project, EditorCommand command) {
    editor_history_command_begin(history, project, &command);
    EditorCommandResult result = editor_command_execute(project, &command);
    editor_history_command_finish(history, &command, &result);
    OK(result);
}

static void reference_lifecycle(void) {
    static EditorProject p; editor_project_init(&p);
    EditorObject *o = editor_project_object_add(&p, (Position){0}); assert(o);
    EditorObjectId object_id = o->id;
    EditorRigidBody *body = editor_project_rigid_body_add(&p, o); assert(body);
    EditorSoftBody *soft = editor_project_soft_body_add(&p, o); assert(soft);
    EditorSoftNode *node = editor_project_soft_node_add(&p, soft, (Position){0}); assert(node);
    ObjectEntityReference target = {OBJECT_NODE, node->id, soft->id};
    body->entity_settings.gameplay_target = target;
    body->entity_settings.transform_driver = target;
    body->entity_settings.transform_position = true;
    EditorHistory history; assert(editor_history_init(&history, &p));
    EditorCommand remove = {.type = EDITOR_COMMAND_ITEM_REMOVE, .data.item_remove = {
        .kind = EDITOR_ITEM_SOFT_NODE, .object = o->id, .parent = soft->id, .item = node->id}};
    history_execute(&history, &p, remove);
    assert(!p.objects[0].rigid_bodies[0].entity_settings.gameplay_target.id);
    assert(!p.objects[0].rigid_bodies[0].entity_settings.transform_driver.id);
    assert(editor_history_undo(&history));
    o = &p.objects[0]; body = &o->rigid_bodies[0];
    assert(body->entity_settings.gameplay_target.id == target.id &&
        body->entity_settings.transform_driver.id == target.id);
    body->entity_settings.transform_driver = (ObjectEntityReference){0};
    EditorSprite *sprite = editor_project_sprite_add(&p, o, "sprite", "unused.png"); assert(sprite);
    target = (ObjectEntityReference){OBJECT_SPRITE, sprite->id, 0};
    body->entity_settings.gameplay_target = target;
    remove = (EditorCommand){.type = EDITOR_COMMAND_SPRITE_REMOVE, .data.sprite_remove = {o->id, sprite->id}};
    history_execute(&history, &p, remove);
    assert(!p.objects[0].rigid_bodies[0].entity_settings.gameplay_target.id);
    assert(editor_history_undo(&history));
    assert(p.objects[0].rigid_bodies[0].entity_settings.gameplay_target.id == target.id);
    p.objects[0].rigid_bodies[0].entity_settings.gameplay_target = (ObjectEntityReference){OBJECT_CONTAINER, object_id, 0};
    EditorObject *copy = editor_project_object_duplicate(&p, object_id); assert(copy);
    assert(copy->rigid_bodies[0].entity_settings.gameplay_target.id == copy->id);
    assert(p.objects[0].rigid_bodies[0].entity_settings.gameplay_target.id == object_id);
    assert(!editor_project_entity_settings_get(copy, (ObjectEntityReference){OBJECT_BODY, copy->rigid_bodies[0].id, 123}));
    editor_history_destroy(&history); editor_project_destroy(&p);
}

static void presentation_round_trip(void) {
    static EditorProject p, loaded, cloned;
    editor_project_init(&p); editor_project_init(&loaded);
    EditorObject *object = editor_project_object_add(&p, (Position){0}); assert(object);
    EditorCamera *camera = editor_project_camera_add(&p, object); assert(camera);
    EditorLayoutViewport *viewport = editor_project_layout_viewport_add(&p); assert(viewport);
    EditorViewportCameraItem *screen = editor_viewport_camera_add(&p, viewport, object->id, camera->id);
    assert(screen && !screen->render_size_explicit);
    screen->render_size_explicit = true; screen->render_width = 320; screen->render_height = 200;
    EditorUiFont *font = editor_project_ui_font_add(&p, "assets/test.ttf");
    assert(font && font->point_size == 12);
    EditorViewportUiDefinition *text = editor_project_ui_definition_add(&p, EDITOR_VIEWPORT_UI_TEXT);
    assert(text);
    text->value.text.font = font->id;
    text->value.text.wrap_width = 123; text->value.text.rotation = -45;
    EditorAudioDefinition *sound = editor_project_audio_add(&p, false, "impact", "assets/impact.wav");
    assert(sound && sound->volume == 0 && sound->playback_rate == 1);
    uint32_t sound_id = sound->id;
    assert(editor_project_audio_add(&p, true, "impact", "assets/music.ogg"));
    assert(!editor_project_audio_add(&p, false, "impact", "duplicate.wav"));
    EditorHistory history; assert(editor_history_init(&history, &p));
    EditorCommand command = {.type = EDITOR_COMMAND_PROPERTY_SET, .data.property_set = {
        .kind = EDITOR_ITEM_UI_FONT, .item = font->id, .property = EDITOR_PROPERTY_FONT_POINT_SIZE,
        .value_kind = EDITOR_PROPERTY_VALUE_FLOAT, .value.number = 24}};
    history_execute(&history, &p, command);
    assert(p.ui_fonts[0].point_size == 24);
    assert(editor_history_undo(&history) && p.ui_fonts[0].point_size == 12);
    assert(editor_history_redo(&history) && p.ui_fonts[0].point_size == 24);
    command = (EditorCommand){.type = EDITOR_COMMAND_AUDIO_SET,
        .data.audio = *editor_project_audio_get(&p, sound_id)};
    command.data.audio.volume = .75f; command.data.audio.pan = -.5f;
    command.data.audio.loop = true; command.data.audio.playback_rate = 1.5f;
    history_execute(&history, &p, command);
    assert(editor_history_undo(&history));
    assert(editor_project_audio_get(&p, sound_id)->volume == 0);
    assert(editor_history_redo(&history));
    assert(editor_project_audio_get(&p, sound_id)->volume == .75f);
    command.data.audio.volume = -1;
    assert(editor_command_execute(&p, &command).kind == ERROR_RESULT_ERROR);
    assert(editor_project_audio_get(&p, sound_id)->volume == .75f);
    command = (EditorCommand){.type = EDITOR_COMMAND_APPLICATION_WINDOW_SET,
        .data.application_window = {true, {.mode = GRAPHICS_WINDOW_MODE_BORDERLESS_FULLSCREEN,
            .window_width = 900, .window_height = 600, .logical_width = 450, .logical_height = 300}}};
    history_execute(&history, &p, command);
    assert(editor_history_undo(&history) && !p.application_window_enabled);
    assert(editor_history_redo(&history) && p.application_window.window_width == 900);
    size_t undo_count = history.undo_count;
    assert(editor_history_transaction_begin(&history));
    command.data.application_window.description.window_width = 1000;
    history_execute(&history, &p, command);
    command.data.application_window.description.window_width = 1100;
    history_execute(&history, &p, command);
    assert(editor_history_transaction_end(&history));
    assert(history.undo_count == undo_count + 1);
    assert(editor_history_undo(&history) && p.application_window.window_width == 900);
    assert(editor_project_save(&p, "presentation_parity.json"));
    OK(editor_project_load(&loaded, "presentation_parity.json"));
    assert(loaded.application_window_enabled && loaded.application_window.logical_width == 450);
    assert(loaded.audio_definition_count == 2 && loaded.audio_definitions[0].pan == -.5f);
    assert(loaded.ui_fonts[0].point_size == 24);
    assert(loaded.ui_definitions[0].value.text.wrap_width == 123);
    assert(loaded.ui_definitions[0].value.text.rotation == -45);
    assert(loaded.layout_viewports[0].camera_items[0].render_width == 320);
    assert(editor_project_clone(&cloned, &loaded));
    assert(cloned.audio_definitions != loaded.audio_definitions);
    assert(editor_project_audio_remove(&cloned, sound_id));
    assert(loaded.audio_definition_count == 2);
    assert(remove("presentation_parity.json") == 0);
    editor_history_destroy(&history);
    editor_project_destroy(&cloned); editor_project_destroy(&loaded); editor_project_destroy(&p);
}

static void authored_placement(void) {
    static EditorProject p; editor_project_init(&p);
    EditorObject *o = editor_project_object_add(&p, (Position){0}); assert(o);
    snprintf(o->name, sizeof(o->name), "placement");
    EditorRigidBody *a = editor_project_rigid_body_add(&p, o); assert(a);
    EditorRigidBodyId a_id = a->id;
    EditorRigidBody *b = editor_project_rigid_body_add(&p, o); assert(b);
    EditorRigidBodyId b_id = b->id;
    a = editor_project_rigid_body_get(o, a_id);
    a->position = (Position){2, 3}; b->position = (Position){20, 30};
    a->initial_velocity = (Velocity){5, 6};
    snprintf(a->name, sizeof(a->name), "first");
    snprintf(b->name, sizeof(b->name), "second");
    EditorAnchor *aa = editor_project_anchor_add(&p, o, (Position){1, 0}, a_id); assert(aa);
    EditorAnchorId aa_id = aa->id;
    EditorAnchor *bb = editor_project_anchor_add(&p, o, (Position){0}, b_id); assert(bb);
    EditorAnchorId bb_id = bb->id;
    EditorJoint *joint = editor_project_joint_add(&p, o, EDITOR_JOINT_REVOLUTE); assert(joint);
    snprintf(joint->name, sizeof(joint->name), "connection");
    assert(editor_project_joint_anchor_set(o, joint, 0, aa_id));
    assert(editor_project_joint_anchor_set(o, joint, 1, bb_id));
    assert(a->position.x == 2 && b->position.x == 20); /* Selecting never places. */
    ObjectDescriptionResult catalog = object_description_project_read(&p, "placement.json"); OK(catalog);
    ObjectDescriptionResult found = object_description_name_get(catalog.result.value, o->name); OK(found);
    EntityResult spawned = entity_object_add(found.result.value); OK(spawned);
    Entity runtime_a = member(spawned.result.value, "first");
    Entity runtime_b = member(spawned.result.value, "second");
    Entity connection = member(spawned.result.value, "connection");
    OK(rohr_physics_joint_place(connection)); /* Includes owned anchor helpers. */
    EditorHistory history; assert(editor_history_init(&history, &p));
    EditorCommand command = {.type = EDITOR_COMMAND_JOINT_PLACE, .data.joint_place = {o->id, joint->id}};
    history_execute(&history, &p, command);
    a = editor_project_rigid_body_get(o, a_id);
    b = editor_project_rigid_body_get(o, b_id);
    Position placed = physics_position_get(runtime_a).result.value;
    assert(fabsf(a->position.x - placed.x) < .0001f && fabsf(a->position.y - placed.y) < .0001f);
    assert(a->position.x == 19 && a->position.y == 30 && a->initial_velocity.x == 5);
    assert(physics_position_get(runtime_b).result.value.x == 20 && b->position.x == 20);
    assert(editor_history_undo(&history));
    o = &p.objects[0]; a = editor_project_rigid_body_get(o, a_id); joint = &o->joint_items[0];
    assert(a->position.x == 2);
    a->static_body = true;
    assert(editor_project_joint_place(o, joint)); /* Static is simulation-only in the editor. */
    assert(a->position.x == 19 && a->position.y == 30 && a->static_body);
    a->position = (Position){2,3};
    a->static_body = false;
    a->entity_settings.axis_lock_enabled = true;
    a->entity_settings.axis_lock = (AxisLockDescription){.axis = {1, 0}, .point_on_axis = {0, 3}};
    assert(!editor_project_joint_place(o, joint));
    a->entity_settings.axis_lock_enabled = false;
    assert(editor_project_joint_kind_set(o, joint, EDITOR_JOINT_SPRING));
    joint->rest_length = 5; joint->rest_length_explicit = true;
    assert(editor_project_joint_anchor_set(o, joint, 0, aa_id) && joint->rest_length == 5);
    assert(editor_project_joint_place(o, joint));
    Position anchor = {a->position.x + 1, a->position.y};
    assert(fabsf(hypotf(anchor.x - 20, anchor.y - 30) - 5) < .0001f);
    assert(editor_project_joint_place(o, joint));
    OK(entity_delete(spawned.result.value));
    object_description_tree_destroy((ObjectDescription *)catalog.result.value);
    editor_history_destroy(&history); editor_project_destroy(&p);
}

static void placement_matrix(void) {
    for(int kind = EDITOR_JOINT_REVOLUTE; kind <= EDITOR_JOINT_SPRING; ++kind) {
        for(unsigned scenario = 0; scenario < 9; ++scenario) {
            static EditorProject p; editor_project_init(&p);
            EditorObject *o = editor_project_object_add(&p, (Position){10, 20}); assert(o);
            EditorRigidBody *a = editor_project_rigid_body_add(&p, o); assert(a);
            EditorRigidBodyId ai = a->id;
            EditorRigidBody *b = editor_project_rigid_body_add(&p, o); assert(b);
            EditorRigidBodyId bi = b->id;
            EditorRigidBody *c = editor_project_rigid_body_add(&p, o); assert(c);
            a = editor_project_rigid_body_get(o, ai); b = editor_project_rigid_body_get(o, bi);
            a->position = (Position){2, 3}; b->position = (Position){20, 30};
            c->position = a->position;
            b->rotation = 60; a->rotation_locked = scenario == 6;
            EditorAnchor *aa = editor_project_anchor_add(&p, o, (Position){0}, scenario == 2 ? 0 : ai); assert(aa);
            EditorAnchorId aa_id = aa->id;
            EditorAnchor *bb = editor_project_anchor_add(&p, o, (Position){0}, scenario == 3 ? 0 : bi); assert(bb);
            EditorAnchorId bb_id = bb->id;
            if(scenario == 3) bb->position = b->position;
            EditorAnchor *cc = editor_project_anchor_add(&p, o, (Position){0}, c->id); assert(cc);
            EditorAnchorId cc_id = cc->id;
            EditorJoint *joint = editor_project_joint_add(&p, o, (EditorJointKind)kind); assert(joint);
            assert(editor_project_joint_anchor_set(o, joint, 0, scenario == 1 ? bb_id : aa_id));
            assert(editor_project_joint_anchor_set(o, joint, 1, scenario == 1 ? aa_id : bb_id));
            joint->rest_length = 5; joint->rest_length_explicit = true;
            joint->rest_angle = 20; joint->rest_angle_explicit = true;
            if(scenario == 4 || scenario == 5) {
                EditorJoint *edge = editor_project_joint_add(&p, o, EDITOR_JOINT_REVOLUTE); assert(edge);
                assert(editor_project_joint_anchor_set(o, edge, 0, aa_id));
                assert(editor_project_joint_anchor_set(o, edge, 1, cc_id));
                if(scenario == 5) {
                    edge = editor_project_joint_add(&p, o, EDITOR_JOINT_REVOLUTE); assert(edge);
                    assert(editor_project_joint_anchor_set(o, edge, 0, cc_id));
                    assert(editor_project_joint_anchor_set(o, edge, 1, bb_id));
                }
            }
            if(scenario == 7) {
                EditorCamera *camera = editor_project_camera_add(&p, o); assert(camera);
                camera->entity_settings.transform_driver = (ObjectEntityReference){OBJECT_BODY, ai, 0};
                camera->entity_settings.transform_position = true;
            }
            if(scenario == 8) {
                a->entity_settings.axis_lock_enabled = true;
                a->entity_settings.axis_lock = (AxisLockDescription){.axis = {1, 0}, .point_on_axis = {0, 3}};
            }
            joint = &o->joint_items[0];
            ObjectDescriptionResult catalog = object_description_project_read(&p, "placement.json"); OK(catalog);
            ObjectDescriptionResult description = object_description_name_get(catalog.result.value, o->name); OK(description);
            EntityResult spawned = entity_object_add(description.result.value); OK(spawned);
            bool runtime_ok = !error_check(physics_joint_place(member(spawned.result.value, joint->name)));
            bool authored_ok = editor_project_joint_place(o, joint);
            assert(runtime_ok == authored_ok);
            assert(authored_ok == (scenario == 0 || scenario == 1 || scenario == 3 ||
                scenario == 4 || (scenario == 6 && kind != EDITOR_JOINT_WELD)));
            for(size_t i = 0; i < o->rigid_body_count; ++i) {
                EditorRigidBody *body = &o->rigid_bodies[i];
                Entity entity = member(spawned.result.value, body->name);
                Position position = physics_position_get(entity).result.value;
                assert(fabsf(position.x - body->position.x - o->position.x) < .0001f);
                assert(fabsf(position.y - body->position.y - o->position.y) < .0001f);
                EntityIndex index; assert(entity_index_get(entity, &index));
                assert(fabsf(orientations[index] - body->rotation) < .0001f);
            }
            OK(entity_delete(spawned.result.value));
            object_description_tree_destroy((ObjectDescription *)catalog.result.value);
            editor_project_destroy(&p);
        }
    }
}

static void soft_anchor_equivalence(void) {
    static EditorProject p; editor_project_init(&p);
    EditorObject *o = editor_project_object_add(&p, (Position){10, 20}); assert(o);
    EditorSoftBody *soft = editor_project_soft_body_add(&p, o); assert(soft);
    soft->position = (Position){4, 5}; soft->rotation = 30;
    EditorSoftNode *a = editor_project_soft_node_add(&p, soft, (Position){0}); assert(a);
    EditorSoftNodeId ai = a->id;
    EditorSoftNode *b = editor_project_soft_node_add(&p, soft, (Position){3, 4}); assert(b);
    a = &soft->nodes[0];
    EditorSoftBeam *beam = editor_project_soft_beam_add(&p, soft, ai, b->id); assert(beam);
    EditorCommand set = {.type = EDITOR_COMMAND_PROPERTY_SET, .data.property_set = {
        .kind = EDITOR_ITEM_SOFT_BEAM, .object = o->id, .parent = soft->id, .item = beam->id,
        .property = EDITOR_PROPERTY_REST_LENGTH_EXPLICIT, .value_kind = EDITOR_PROPERTY_VALUE_BOOL,
        .value.boolean = true}};
    OK(editor_command_execute(&p, &set));
    assert(beam->rest_length_explicit && beam->rest_length == 5);
    b->position.x = 8;
    assert(beam->rest_length == 5);
    set.data.property_set.value.boolean = false; OK(editor_command_execute(&p, &set));
    EditorAnchor *anchor = editor_project_anchor_add(&p, o, (Position){0}, 0); assert(anchor);
    anchor->attachment_kind = EDITOR_ANCHOR_ATTACHMENT_SOFT_NODE;
    anchor->attachment_soft_body = soft->id; anchor->attachment_soft_node = ai;
    anchor->position_follows_body = true;
    EditorAnchorId anchor_id = anchor->id;
    EditorAnchor *world = editor_project_anchor_add(&p, o, (Position){30, 40}, 0); assert(world);
    EditorJoint *joint = editor_project_joint_add(&p, o, EDITOR_JOINT_REVOLUTE); assert(joint);
    assert(editor_project_joint_anchor_set(o, joint, 0, anchor_id));
    assert(editor_project_joint_anchor_set(o, joint, 1, world->id));
    ObjectDescriptionResult catalog = object_description_project_read(&p, "soft_pin.json"); OK(catalog);
    ObjectDescriptionResult description = object_description_name_get(catalog.result.value, o->name); OK(description);
    EntityResult spawned = entity_object_add(description.result.value); OK(spawned);
    Entity runtime_soft = member(spawned.result.value, soft->name);
    Entity runtime_a = member(runtime_soft, a->name);
    Entity runtime_joint = member(spawned.result.value, joint->name);
    JointDescription authored = physics_joint_description_get(runtime_joint).result.value;
    SoftBodyNodeAnchorPinResult direct = physics_soft_body_node_to_anchor_pin_add(
        (SoftBodyNodeAnchorPinDescription){.owner = spawned.result.value,
            .node = runtime_a, .anchor = authored.anchor_b}); OK(direct);
    JointDescription equivalent = physics_joint_description_get(direct.result.value.joint).result.value;
    assert(equivalent.owner == authored.owner && equivalent.type == authored.type);
    JointAnchorDescription first = physics_joint_anchor_description_get(authored.anchor_a).result.value;
    JointAnchorDescription second = physics_joint_anchor_description_get(equivalent.anchor_a).result.value;
    assert(first.entity == second.entity && first.local_offset.x == second.local_offset.x &&
        first.local_offset.y == second.local_offset.y);
    OK(entity_delete(direct.result.value.joint));
    OK(physics_joint_anchor_remove(direct.result.value.node_anchor));
    OK(physics_joint_place(runtime_joint));
    assert(editor_project_joint_place(o, joint));
    Position origin = physics_position_get(runtime_soft).result.value;
    assert(fabsf(origin.x - o->position.x - soft->position.x) < .0001f);
    assert(fabsf(origin.y - o->position.y - soft->position.y) < .0001f);
    assert(soft->rotation == 30 && b->position.x == 8 && b->position.y == 4);
    authored.type = JOINT_WELD; authored.rest_angle = -45; authored.rest_angle_explicit = true;
    OK(physics_joint_description_set(runtime_joint, authored));
    joint->kind = EDITOR_JOINT_WELD; joint->rest_angle = -45; joint->rest_angle_explicit = true;
    OK(physics_joint_place(runtime_joint)); assert(editor_project_joint_place(o, joint));
    EntityIndex index; assert(entity_index_get(runtime_soft, &index));
    assert(orientations[index] == soft->rotation && soft->rotation == 45);
    assert(b->position.x == 8 && b->position.y == 4);
    OK(entity_delete(spawned.result.value));
    object_description_tree_destroy((ObjectDescription *)catalog.result.value);
    editor_project_destroy(&p);
}

static void generation_fixture(const char *directory, const char *font_path) {
    static EditorProject p; EditorWorkspace workspace = {0};
    OK(editor_workspace_create(&workspace, &p, directory));
    editor_project_destroy(&p); editor_project_init(&p);
    p.engine_time_per_tick=1.0/73.0;
    p.physics_timestep_override=true;
    p.physics_dt_per_tick=1.0/137.0;
    p.application_window_enabled = true;
    p.application_window = (WindowPresentationDescription){.window_width = 900, .window_height = 600,
        .logical_width = 450, .logical_height = 300, .aspect_ratio_auto = true};
    EditorObject *o = editor_project_object_add(&p, (Position){10, 20}); assert(o);
    snprintf(o->name, sizeof(o->name), "Parity");
    EditorGraphicsLayer *layer = editor_project_graphics_layer_add(&p, "world", 5); assert(layer);
    EditorGraphicsLayerId world = layer->id;
    layer = editor_project_graphics_layer_add(&p, "effects", 5); assert(layer);
    EditorGraphicsLayerId effects = layer->id;
    layer = editor_project_graphics_layer_add(&p, "ui_only", 8); assert(layer);
    EditorGraphicsLayerId ui_layer = layer->id;
    assert(editor_project_graphics_layer_add(&p, "quoted \"layer\"\\", 8));
    EditorRigidBody *body = editor_project_rigid_body_add(&p, o); assert(body);
    snprintf(body->name, sizeof(body->name), "body");
    body->graphics_layer.layer = world;
    body->position = (Position){2, 3};
    body->entity_settings = (ObjectEntitySettings){
        .lifetime = {.deadline = {.time_enabled = true, .expiry_time = 100, .tick_enabled = true, .expiry_tick = 1000}},
        .angular_acceleration = 2, .angular_velocity_maximum_enabled = true, .angular_velocity_maximum = 3,
        .axis_lock_enabled = true, .axis_lock = {.axis = {1, 0}, .point_on_axis = {0, 3}}};
    EditorSoftBody *soft = editor_project_soft_body_add(&p, o); assert(soft);
    snprintf(soft->name, sizeof(soft->name), "soft");
    body->entity_settings.gameplay_target = (ObjectEntityReference){OBJECT_SOFT_BODY, soft->id, 0};
    EditorSoftNode *a = editor_project_soft_node_add(&p, soft, (Position){0});
    EditorSoftNode *b = editor_project_soft_node_add(&p, soft, (Position){3, 4});
    assert(a && b); snprintf(a->name, sizeof(a->name), "fixed_node"); a->static_body = true;
    snprintf(b->name, sizeof(b->name), "node");
    EditorSoftBeam *beam = editor_project_soft_beam_add(&p, soft, a->id, b->id); assert(beam);
    snprintf(beam->name, sizeof(beam->name), "beam");
    beam->rest_length_explicit = true; beam->rest_length = 2.5f;
    EditorCamera *camera = editor_project_camera_add(&p, o); assert(camera);
    snprintf(camera->name, sizeof(camera->name), "camera");
    camera->attachment_kind = EDITOR_CAMERA_ATTACHMENT_RIGID_BODY;
    camera->attachment = body->id; camera->position = (Position){7, 8};
    camera->position_follow_disabled = true; camera->inherit_orientation = true; camera->pause_with_engine = true;
    EditorLayoutViewport *viewport = editor_project_layout_viewport_add(&p); assert(viewport);
    snprintf(viewport->name, sizeof(viewport->name), "play");
    viewport->config.rectangle = (ViewportRectangle){0, 0, 450, 300};
    EditorViewportCameraItem *screen = editor_viewport_camera_add(&p, viewport, o->id, camera->id); assert(screen);
    screen->render_size_explicit = true; screen->render_width = 320; screen->render_height = 200;
    screen->placement.rectangle = (ViewportRectangle){-20, 10, 250, 200}; screen->placement.orientation = 12;
    screen->placement.clip_enabled = true;
    screen->placement.clip_rectangle = (ViewportRectangle){5,10,210,160};
    screen->background_color = 0x123456ff;
    snprintf(screen->name, sizeof(screen->name), "front");
    screen = editor_viewport_camera_add(&p, viewport, o->id, camera->id); assert(screen);
    screen->render_size_explicit = true; screen->render_width = 480; screen->render_height = 360;
    screen->placement.rectangle = (ViewportRectangle){200, 50, 250, 200};
    screen->placement.orientation = -18; screen->placement.layer = 2;
    snprintf(screen->name, sizeof(screen->name), "rear");
    size_t size = 0; void *bytes = SDL_LoadFile(font_path, &size); assert(bytes);
    char destination[2048]; snprintf(destination, sizeof(destination), "%s/assets/parity_font.ttf", directory);
    FILE *font_file = fopen(destination, "wb"); assert(font_file);
    assert(fwrite(bytes, 1, size, font_file) == size); assert(fclose(font_file) == 0); SDL_free(bytes);
    SDL_Surface *image = SDL_CreateSurface(2, 1, SDL_PIXELFORMAT_RGBA8888); assert(image);
    ((Uint32 *)image->pixels)[0] = 0xff0000ff; ((Uint32 *)image->pixels)[1] = 0x0000ffff;
    snprintf(destination, sizeof(destination), "%s/assets/flip.png", directory);
    assert(SDL_SavePNG(image, destination)); SDL_DestroySurface(image);
    EditorSprite *sprite = editor_project_sprite_add(&p, o, "flipped", "assets/flip.png"); assert(sprite);
    sprite->graphics_layer.layer = effects;
    sprite->position = (Position){4, 5}; sprite->rotation = 23;
    EditorCommand size_command = {.type = EDITOR_COMMAND_SPRITE_SIZE_SET,
        .data.sprite_size_set = {o->id, sprite->id, {-20, -10}}};
    OK(editor_command_execute(&p, &size_command));
    size_command.data.sprite_size_set.size.x = NAN;
    assert(editor_command_execute(&p, &size_command).kind == ERROR_RESULT_ERROR);
    assert(sprite->size.x == -20);
    EditorAnimatedSprite *animation=editor_project_animated_sprite_add(&p,o); assert(animation);
    snprintf(animation->name,sizeof(animation->name),"moving");
    animation->graphics_layer.value = 5;
    animation->ticks_per_frame=3; animation->time_per_frame=.25; animation->playing=true;
    animation->scale=(Scale){2,3}; animation->direction=DIRECTION_LEFT;
    assert(editor_project_animation_frame_add(&p,animation,"first","assets/flip.png",(Scale){4,6}));
    assert(editor_project_animation_frame_add(&p,animation,"second","assets/flip.png",(Scale){8,10}));
    animation->frames[0].offset=(Position){1,2}; animation->frames[0].rotation=27;
    animation->frames[1].offset=(Position){-3,4}; animation->frames[1].rotation=-19;
    animation=editor_project_animated_sprite_add(&p,o); assert(animation);
    snprintf(animation->name,sizeof(animation->name),"blank");
    EditorUiFont *font = editor_project_ui_font_add(&p, "assets/parity_font.ttf"); assert(font); font->point_size = 24;
    for(size_t i = 0; i < 2; ++i) {
        EditorViewportUiItem *ui = editor_viewport_ui_add(&p, viewport, EDITOR_VIEWPORT_UI_TEXT); assert(ui);
        snprintf(ui->name, sizeof(ui->name), "%s", i ? "status" : "label");
        ui->value.text.font = font->id; ui->value.text.wrap_width = 70; ui->value.text.rotation = 17;
        snprintf(ui->value.text.text, sizeof(ui->value.text.text), "Wrapped text");
        ui->layer = i ? 3 : 1;
        ui->clip_enabled=true; ui->clip_rectangle=(ViewportRectangle){10,15,100,80};
        assert(editor_project_ui_definition_sync_from_item(&p, ui->id));
    }
    EditorViewportUiItem *slider=editor_viewport_ui_add(&p,viewport,EDITOR_VIEWPORT_UI_SLIDER); assert(slider);
    snprintf(slider->name, sizeof(slider->name), "slider");
    slider->value.slider.minimum=-2; slider->value.slider.maximum=6; slider->value.slider.value=3;
    slider->value.slider.step=.5f; slider->value.slider.thumb_offset=4;
    slider->rotation=35; slider->layer=4;
    assert(editor_project_ui_definition_sync_from_item(&p,slider->id));
    viewport = editor_project_layout_viewport_add(&p); assert(viewport);
    snprintf(viewport->name, sizeof(viewport->name), "overlay");
    EditorViewportUiItem *button = editor_viewport_ui_add(&p, viewport, EDITOR_VIEWPORT_UI_SHAPE); assert(button);
    snprintf(button->name, sizeof(button->name), "button");
    button->graphics_layer = ui_layer;
    button->value.shape.rotation = 21;
    button->value.shape.button_enabled = true;
    button->value.shape.text.font = font->id;
    snprintf(button->value.shape.text.text, sizeof(button->value.shape.text.text), "Action");
    assert(editor_project_ui_definition_sync_from_item(&p,button->id));
    viewport = editor_project_layout_viewport_add(&p); assert(viewport);
    snprintf(viewport->name, sizeof(viewport->name), "empty_view");
    viewport->app_hidden = true;
    EditorAudioDefinition *sound = editor_project_audio_add(&p, false, "impact", "assets/impact.wav"); assert(sound);
    sound->volume = .75f; sound->pan = -.5f; sound->loop = true; sound->playback_rate = 1.5f;
    assert(editor_project_audio_add(&p, true, "theme", "assets/theme.ogg"));
    EditorInputController *input = editor_project_input_controller_add(&p, "player"); assert(input);
    EditorInputAction *action = editor_project_input_action_add(&p, input->id, "toggle", INPUT_ACTION_BUTTON);
    assert(action); action->button_mode = INPUT_BUTTON_PERSISTENT; action->button_initial_state = true;
    assert(editor_project_input_binding_add(&p, input->id, action->id,
        input_binding_scancode_create(SDL_SCANCODE_SPACE)));
    assert(editor_project_input_action_add(&p, input->id, "alternate", INPUT_ACTION_AXIS_1D));
    input = editor_project_input_controller_add(&p, "disabled"); assert(input); input->enabled = false;
    assert(editor_workspace_save(&workspace, &p));
    static EditorProject loaded; editor_project_init(&loaded);
    EditorWorkspace loaded_workspace = {0};
    OK(editor_workspace_load(&loaded_workspace, &loaded, directory));
    assert(loaded.engine_time_per_tick==p.engine_time_per_tick);
    assert(loaded.physics_dt_per_tick==p.physics_dt_per_tick && loaded.physics_timestep_override);
    assert(loaded.objects[0].sprites[0].size.x == -20 && loaded.objects[0].sprites[0].size.y == -10);
    assert(editor_workspace_c_generate(&workspace, &p));
    /* Regenerate from the saved project, comparing every generated artifact
     * and a developer-edited entry point byte for byte. */
    snprintf(destination,sizeof(destination),"%s/src/main.c",directory);
    FILE *main_file=fopen(destination,"wb"); assert(main_file);
    assert(fputs("/* developer-owned main */\nint main(void) { return 42; }\n",main_file)>=0);
    assert(fclose(main_file)==0);
    const char *files[]={"main.c","generated/project.h","generated/scene/project.h","generated/project_objects.c","generated/project_objects.h",
        "generated/project_scene.c","generated/project_scene.cmake",
        "generated/scene/project_objects.c","generated/scene/project_objects.h"};
    void *contents[sizeof(files)/sizeof(files[0])];
    size_t lengths[sizeof(files)/sizeof(files[0])];
    for(size_t i=0;i<sizeof(files)/sizeof(files[0]);++i) {
        snprintf(destination,sizeof(destination),"%s/src/%s",directory,files[i]);
        contents[i]=SDL_LoadFile(destination,&lengths[i]); assert(contents[i]);
    }
    for(unsigned pass=0;pass<2;++pass) {
        assert(editor_workspace_c_generate(&loaded_workspace,&loaded));
        for(size_t i=0;i<sizeof(files)/sizeof(files[0]);++i) {
            snprintf(destination,sizeof(destination),"%s/src/%s",directory,files[i]);
            size_t length; void *content=SDL_LoadFile(destination,&length); assert(content);
            if(length!=lengths[i] || memcmp(content,contents[i],length)!=0) {
                fprintf(stderr,"Generation changed %s after reload (pass %u)\n",files[i],pass);
                size_t different=0;
                while(different<length && different<lengths[i] &&
                    ((char *)content)[different]==((char *)contents[i])[different]) ++different;
                fprintf(stderr,"At byte %zu, before: %.180s\nafter: %.180s\n",different,
                    (char *)contents[i]+different,(char *)content+different);
                assert(false);
            }
            SDL_free(content);
        }
    }
    /* Name conflicts must fail before replacing any generated files. */
    struct { char *name; const char *invalid; const char *field; } conflicts[] = {
        {loaded.layout_viewports[0].name, "Parity", "parity"},
        {loaded.input_controllers[0].name, "Parity", "parity"},
        {loaded.audio_definitions[0].name, "player", "player"},
        {loaded.graphics_layers[0].name, "impact", "impact"},
        {loaded.layout_viewports[0].name, "world", "world"},
        {loaded.input_controllers[0].name, "viewport_count", "viewport_count"},
        {loaded.input_controllers[0].actions[0].name, "actions", "actions"},
        {loaded.input_controllers[0].actions[1].name, "Toggle", "toggle"},
        {loaded.layout_viewports[0].ui_items[0].name, "Front", "front"},
        {loaded.layout_viewports[0].ui_items[0].name, "camera_count", "camera_count"},
    };
    for(size_t c=0;c<sizeof(conflicts)/sizeof(conflicts[0]);++c) {
        char saved[EDITOR_OBJECT_NAME_MAX]; strcpy(saved,conflicts[c].name);
        strcpy(conflicts[c].name,conflicts[c].invalid);
        EditorResult invalid=editor_project_catalog_validate(&loaded);
        assert(editor_result_check(invalid));
        assert(strstr(invalid.result.error.message,conflicts[c].field));
        assert(!editor_workspace_c_generate(&loaded_workspace, &loaded));
        for(size_t i=0;i<sizeof(files)/sizeof(files[0]);++i) {
            snprintf(destination,sizeof(destination),"%s/src/%s",directory,files[i]);
            size_t length; void *content=SDL_LoadFile(destination,&length); assert(content);
            assert(length==lengths[i] && !memcmp(content,contents[i],length));
            SDL_free(content);
        }
        strcpy(conflicts[c].name,saved);
    }
    for(size_t i=0;i<sizeof(files)/sizeof(files[0]);++i) SDL_free(contents[i]);
    editor_project_destroy(&loaded);
    editor_project_destroy(&p);
}

int main(int argc, char **argv) {
    OK(engine_start());
    OK(engine_time_per_tick_set(0.001));
    SDL_Delay(5);
    engine_time_update(); (void)engine_tick_update();
    assert(engine_time_get() > 0 && engine_tick_get() > 0);
    settings_round_trip(); lifetime_modes(); setting_commands(); reference_lifecycle(); presentation_round_trip(); authored_placement(); placement_matrix(); soft_anchor_equivalence();
    if(argc == 3) generation_fixture(argv[1], argv[2]);
    engine_stop();
    return 0;
}
