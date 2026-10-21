/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Compiled against the installed SDK and the configuration parity fixture.
 */
#include "generated/project_objects.h"
#include "generated/project_viewports.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define OK(call) assert(!rohr_error_check(call))

int main(void) {
    OK(rohr_directory_working_set(rohr_directory_base_get()));
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    ProjectObjects objects_in_scene = {0};
    Time start_time = rohr_engine_time_get(); Tick start_tick = rohr_engine_tick_get();
    EngineResult created = project_objects_create_all(&objects_in_scene);
    if(rohr_error_check(created)) fprintf(stderr, "Project creation: %s\n", rohr_error_message_get(created));
    OK(created);
    Parity *instance = &objects_in_scene.parity;
    EntityLifetimeDescription lifetime = rohr_entity_lifetime_description_get(instance->body).result.value;
    assert(lifetime.time_enabled && lifetime.expiry_time == start_time + 100);
    assert(lifetime.tick_enabled && lifetime.expiry_tick == start_tick + 1000);
    assert(rohr_physics_angular_velocity_maximum_get(instance->body).result.value == 3);
    EntityInfoResult info = rohr_entity_info_get(instance->body,
        ROHR_ENTITY_INFO_CORE_TARGET | ROHR_ENTITY_INFO_PHYSICS_ANGULAR_ACCELERATION); OK(info);
    assert(info.result.value.target == instance->soft && info.result.value.angular_acceleration == 2);
    rohr_entity_info_release(&info.result.value);
    assert(rohr_physics_axis_lock_description_get(instance->body).result.value.point_on_axis.y == 23);
    assert(rohr_physics_soft_body_node_description_get(instance->fixed_node).result.value.static_body);
    assert(rohr_physics_soft_body_beam_description_get(instance->beam).result.value.rest_length == 2.5f);
    SpriteDescription sprite = rohr_graphics_sprite_description_get(instance->sprite_flipped).result.value;
    assert(sprite.texture.size.x * sprite.scale.x == -20 && sprite.texture.size.y * sprite.scale.y == -10);
    assert(sprite.direction == DIRECTION_RIGHT);
    assert(rohr_physics_position_get(instance->sprite_flipped).result.value.x == 14);
    info = rohr_entity_info_get(instance->sprite_flipped, ROHR_ENTITY_INFO_PHYSICS_ORIENTATION); OK(info);
    assert(info.result.value.orientation == 23); rohr_entity_info_release(&info.result.value);
    CameraDescription camera = rohr_camera_description_get(instance->camera_camera).result.value;
    assert(camera.position.x == 17 && camera.position.y == 28 && camera.pause_with_engine);
    CameraAttachmentDescription attachment = rohr_camera_attachment_description_get(instance->camera_camera).result.value;
    assert(!attachment.follow_position && attachment.follow_orientation);
    WindowPresentationDescription window = rohr_graphics_window_presentation_description_get();
    assert(window.window_width == 900 && window.window_height == 600);
    assert(window.logical_width == 450 && window.logical_height == 300 && window.aspect_ratio_auto);
    assert(project_audio.sounds.impact.volume == .75f && project_audio.sounds.impact.pan == -.5f);
    assert(project_audio.sounds.impact.loop && project_audio.sounds.impact.playback_rate == 1.5f);
    assert(!strcmp(project_audio.music.theme.path, "assets/theme.ogg") && project_audio.music.theme.volume == 0);
    ProjectViewports views = {0}; OK(project_viewports_create(&views, &objects_in_scene));
    assert(views.screen_count == 2 && views.text_count == 2 && views.ui_element_count == 2);
    ScreenDescription first = rohr_screen_description_get(views.screens[0]).result.value;
    ScreenDescription second = rohr_screen_description_get(views.screens[1]).result.value;
    assert(first.width == 320 && first.height == 200 && second.width == 480 && second.height == 360);
    TextDescription text = rohr_graphics_text_description_get(views.texts[0]).result.value;
    assert(text.wrap_width == 70);
    assert(rohr_graphics_font_description_get(text.font).result.value.point_size == 24);
    assert(rohr_graphics_ui_text_description_get(views.ui_elements[0]).result.value.orientation == 17);
    /* Legacy generated convenience creation uses the same configuration and
     * treats its position as the requested object origin. */
    Parity independent = {0}; OK(parity_create(&independent, (Position){100, 200}));
    Position position = rohr_physics_position_get(independent.body).result.value;
    assert(position.x == 102 && position.y == 203);
    info = rohr_entity_info_get(independent.body, ROHR_ENTITY_INFO_CORE_TARGET); OK(info);
    assert(info.result.value.target == independent.soft);
    rohr_entity_info_release(&info.result.value);
    assert(independent.body != instance->body);
    OK(rohr_physics_angular_velocity_maximum_set(independent.body,0));
    SoftBodyBeamDescription beam=rohr_physics_soft_body_beam_description_get(independent.beam).result.value;
    beam.rest_length=9; beam.rest_length_explicit=true;
    OK(rohr_physics_soft_body_beam_description_set(independent.beam,beam));
    OK(rohr_physics_position_set(independent.body,(Position){300,400}));
    assert(rohr_physics_angular_velocity_maximum_get(instance->body).result.value==3);
    assert(rohr_physics_soft_body_beam_description_get(instance->beam).result.value.rest_length==2.5f);
    assert(rohr_physics_position_get(instance->body).result.value.x==12);
    parity_destroy(&independent);
    assert(rohr_entity_alive_check(instance->body));
    info=rohr_entity_info_get(instance->body,ROHR_ENTITY_INFO_CORE_TARGET); OK(info);
    assert(info.result.value.target==instance->soft); rohr_entity_info_release(&info.result.value);
    project_viewports_destroy(&views); project_objects_destroy_all(&objects_in_scene);
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
