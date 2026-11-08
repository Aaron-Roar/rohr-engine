/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "generated/project.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define OK(call) do { if(rohr_error_check(call)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #call); assert(false); } } while(0)

static Entity member(Entity root, const char *name) {
    EntityResult result = rohr_object_member_get(root, name); OK(result);
    return result.result.value;
}
static ViewportItemDescription item(ViewportId viewport, const char *name) {
    ViewportItemIdResult id = rohr_viewport_item_by_name_get(viewport, name); OK(id);
    ViewportItemDescriptionResult description = rohr_viewport_item_description_get(id.result.value); OK(description);
    return description.result.value;
}
static ViewportId view_create(Entity instance) {
    ViewportObjectBinding binding = {&project.parity._description, instance};
    ViewportCreateDescription description = {.template=&project.play, .objects=&binding, .object_count=1};
    ViewportIdResult result = rohr_viewport_create(&description); OK(result);
    return result.result.value;
}
static void verify(Entity instance, ViewportId view) {
    Entity body = member(instance, "body");
    Entity soft = member(instance, "soft");
    EntityInfoResult info = rohr_entity_info_get(body, ROHR_ENTITY_INFO_CORE_TARGET); OK(info);
    assert(info.result.value.target == soft); rohr_entity_info_release(&info.result.value);
    assert(rohr_physics_position_get(body).result.value.x == 12);
    assert(rohr_physics_soft_body_node_description_get(member(soft,"fixed_node")).result.value.static_body);
    assert(rohr_physics_soft_body_beam_description_get(member(soft,"beam")).result.value.rest_length == 2.5f);
    CameraId camera = rohr_entity_object_camera_get(member(instance,"camera")).result.value;
    assert(rohr_camera_attachment_description_get(camera).result.value.entity == body);
    ViewportItemDescription front = item(view,"front"), rear = item(view,"rear");
    ScreenDescription screen = rohr_screen_description_get(front.screen).result.value;
    assert(screen.camera == camera && screen.width == 320 && screen.height == 200);
    assert(screen.content_mode == SCREEN_CONTENT_AUTOMATIC);
    assert(front.rectangle.x == -20 && front.orientation == 12);
    assert(front.clip_enabled && front.clip_rectangle.width == 210);
    assert(screen.background_color.red == 0x12 && screen.background_color.green == 0x34);
    screen = rohr_screen_description_get(rear.screen).result.value;
    assert(screen.camera == camera && screen.width == 480 && screen.height == 360);
    assert(rear.rectangle.x == 200 && rear.orientation == -18 && rear.layer == 2);
    ViewportItemDescription label = item(view,"label");
    assert(label.layer == 1 && label.clip_enabled && label.clip_rectangle.width == 100);
    ViewportUiTextDescription text = rohr_graphics_ui_text_description_get(label.ui).result.value;
    assert(text.position.x == 80 && text.position.y == 14 && text.orientation == 17);
    TextDescription content = rohr_graphics_text_description_get(text.text).result.value;
    assert(!strcmp(content.value,"Wrapped text") && content.wrap_width == 70);
    assert(rohr_graphics_font_description_get(content.font).result.value.point_size == 24);
    ViewportUiSliderDescription slider = rohr_graphics_ui_slider_description_get(item(view,"slider").ui).result.value;
    assert(slider.minimum == -2 && slider.maximum == 6 && slider.value == 3 && slider.thumb_offset == 4);
    assert(slider.step == .5f && item(view,"slider").orientation == 35);
    GraphicsLayerId world = rohr_graphics_layer_name_id_get("world").result.value;
    GraphicsLayerId effects = rohr_graphics_layer_name_id_get("effects").result.value;
    Entity sprite = member(instance,"flipped"), animation = member(instance,"moving");
    assert(world && effects && world != effects);
    assert(rohr_graphics_layer_entity_id_get(body).result.value == world);
    assert(rohr_graphics_layer_sprite_id_get(sprite).result.value == effects);
    assert(rohr_error_check(rohr_graphics_layer_animation_id_get(animation)));
    OK(rohr_graphics_layer_set(world, 7));
    assert(rohr_graphics_layer_entity_get(body).result.value == 7);
    assert(rohr_graphics_layer_sprite_get(sprite).result.value == 5);
    assert(rohr_graphics_layer_animation_get(animation).result.value == 5);
    OK(rohr_graphics_layer_set(world, 5));
}
int main(void) {
    OK(rohr_directory_working_set(rohr_directory_base_get()));
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    assert(project._project.root.children[0] == &project.parity._description);
    assert(project.parity.body.parent == &project.parity._description);
    assert(project.play.items[0].camera_member == &project.parity.camera._description);
    assert(!strcmp(project.parity.body.layer_name,"world"));
    assert(!strcmp(project.parity.flipped.layer_name,"effects"));
    ViewportIdResult unbound = rohr_viewport_create(&project.play); OK(unbound);
    assert(rohr_screen_description_get(item(unbound.result.value,"front").screen).result.value.camera==CAMERA_INVALID);
    assert(rohr_screen_description_get(item(unbound.result.value,"rear").screen).result.value.camera==CAMERA_INVALID);
    Report empty_frame = rohr_graphics_viewport_show(unbound.result.value);
    assert(empty_frame.header.status==ROHR_REPORT_COMPLETE && empty_frame.diagnostic_count==0);
    OK(rohr_viewport_destroy(unbound.result.value));
    ViewportIdResult overlay = rohr_viewport_create(&project.overlay); OK(overlay);
    OK(rohr_graphics_layer_name_id_get("quoted \"layer\"\\"));
    ViewportItemDescription button = item(overlay.result.value,"button");
    ViewportUiShapeDescription shape = rohr_graphics_ui_shape_description_get(button.ui).result.value;
    assert(shape.orientation == 21 && shape.button_enabled && shape.shape.amount_of_vertices == 4);
    assert(!strcmp(rohr_graphics_text_description_get(shape.text.text).result.value.value,"Action"));
    assert(button.layer_id == rohr_graphics_layer_name_id_get("ui_only").result.value);
    ViewportIdResult empty = rohr_viewport_create(&project.empty_view); OK(empty);
    assert(!rohr_viewport_description_get(empty.result.value).result.value.enabled);
    OK(rohr_viewport_destroy(empty.result.value)); OK(rohr_viewport_destroy(overlay.result.value));
    EntityResult created = rohr_object_create(&project.parity); OK(created);
    Entity first = created.result.value; ViewportId first_view = view_create(first);
    verify(first, first_view);
    uint32_t alive = rohr_entity_alive_count_get();
    for(unsigned i=0;i<1000;++i) {
        created = rohr_object_create(&project.parity); OK(created);
        Entity second = created.result.value; ViewportId second_view = view_create(second);
        verify(second, second_view);
        assert(member(first,"body") != member(second,"body"));
        OK(rohr_physics_position_set(member(second,"body"),(Position){123,456}));
        assert(rohr_physics_position_get(member(first,"body")).result.value.x == 12);
        OK(rohr_graphics_ui_slider_value_set(item(second_view,"slider").ui,-1));
        assert(rohr_graphics_ui_slider_value_get(item(first_view,"slider").ui).result.value == 3);
        if(i==0) {
            ViewportId views[] = {first_view, second_view};
            Report report = rohr_graphics_viewports_show(views,2);
            assert(report.header.status != ROHR_REPORT_FAILED);
        }
        ScreenId screen = item(second_view,"front").screen;
        OK(rohr_viewport_destroy(second_view));
        assert(rohr_error_check(rohr_screen_description_get(screen)));
        OK(rohr_entity_delete(second));
        assert(rohr_entity_alive_count_get() == alive);
    }
    /* An independent visual needs no object instance. */
    EntityResult visual = rohr_entity_create(&project.parity.flipped); OK(visual);
    OK(rohr_viewport_destroy(first_view)); OK(rohr_entity_delete(first));
    assert(rohr_entity_alive_check(visual.result.value));
    OK(rohr_entity_delete(visual.result.value));
    assert(rohr_entity_alive_count_get() == 0);
    /* The same named views now expose a full project recipe, without duplicating
     * object/camera descriptions or retaining generated mutable runtime state. */
    assert(&project._project.root == &project._description);
    assert(project._project.viewports[0].description == &project.play);
    assert(project._project.controller_count == 2);
    assert(project._project.sound_count == 1 && project._project.music_count == 1);
    OK(rohr_audio_start());
    ProjectIdResult p1 = rohr_project_create(&project); OK(p1);
    ProjectIdResult p2 = rohr_project_create(&project); OK(p2);
    ProjectId ids[] = {p1.result.value, p2.result.value};
    Entity instances[2];
    InputControllerId inputs[2];
    for(size_t i = 0; i < 2; ++i) {
        EntityResult root = rohr_project_root_get(ids[i]); OK(root);
        instances[i] = member(root.result.value, "Parity");
        ViewportIdResult view = rohr_project_viewport_by_name_get(ids[i], "Play"); OK(view);
        verify(instances[i], view.result.value);
        ViewportListResult views = rohr_project_viewports_get(ids[i]); OK(views);
        assert(views.result.value.count == 3);
        InputControllerIdResult controller = rohr_project_controller_by_name_get(ids[i], "player"); OK(controller);
        inputs[i] = controller.result.value;
        InputActionIdResult action = rohr_input_action_by_name_get(inputs[i], "toggle"); OK(action);
        InputActionDescriptionResult description = rohr_input_action_description_get(action.result.value); OK(description);
        assert(description.result.value.button_mode == INPUT_BUTTON_PERSISTENT);
        assert(description.result.value.button_initial_state);
        assert(description.result.value.bindings_default.count == 1);
        assert(description.result.value.bindings_default.values[0].input.key == SDL_SCANCODE_SPACE);
        controller = rohr_project_controller_by_name_get(ids[i], "disabled"); OK(controller);
        InputControllerDescriptionResult disabled = rohr_input_controller_description_get(controller.result.value); OK(disabled);
        assert(!disabled.result.value.enabled);
        SoundResult sound = rohr_project_sound_by_name_get(ids[i], "impact"); OK(sound);
        SoundDescriptionResult audio = rohr_audio_sound_description_get(sound.result.value); OK(audio);
        assert(audio.result.value.volume == .75f && audio.result.value.pan == -.5f);
        assert(audio.result.value.loop && audio.result.value.playback_rate == 1.5f);
        assert(!rohr_audio_sound_playing_check(sound.result.value));
        MusicResult music = rohr_project_music_by_name_get(ids[i], "theme"); OK(music);
        assert(!rohr_audio_music_playing_check(music.result.value));
    }
    assert(instances[0] != instances[1] && inputs[0] != inputs[1]);
    OK(rohr_physics_position_set(member(instances[0], "body"), (Position){333, 444}));
    assert(rohr_physics_position_get(member(instances[1], "body")).result.value.x == 12);
    OK(rohr_project_destroy(ids[0]));
    assert(rohr_entity_alive_check(instances[1]));
    OK(rohr_input_controller_description_get(inputs[1]));
    OK(rohr_project_destroy(ids[1]));
    assert(rohr_entity_alive_count_get() == 0);
    assert(rohr_audio_stop().header.status != ROHR_REPORT_FAILED);
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
