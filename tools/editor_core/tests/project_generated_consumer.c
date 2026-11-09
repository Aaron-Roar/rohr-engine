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
    ViewportCreateDescription description = {.template=&project.play._description, .objects=&binding, .object_count=1};
    ViewportIdResult result = rohr_viewport_create(description); OK(result);
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
/* Authored creators return resources only, with the same ownership as native
 * inputs. This runs against generated headers and an installed SDK as well. */
static void resource_creation(void) {
    unsigned entities=rohr_entity_alive_count_get();
    CameraIdResult camera=rohr_camera_create(project.parity.camera); OK(camera);
    CameraAttachmentDescriptionResult attachment=rohr_camera_attachment_description_get(camera.result.value);
    assert(rohr_error_check(attachment) && attachment.result.error==ERROR_ENGINE_COMPONENT_MISSING);
    CameraDescriptionResult camera_description=rohr_camera_description_get(camera.result.value); OK(camera_description);
    CameraIdResult native_camera=graphics_camera_create(camera_description.result.value); OK(native_camera);
    assert(rohr_camera_get(native_camera.result.value).result.value.zoom==project.parity.camera.zoom);
    ScreenIdResult screen=rohr_screen_create(project.play.front); OK(screen);
    ScreenDescriptionResult screen_description=rohr_screen_description_get(screen.result.value); OK(screen_description);
    assert(!screen_description.result.value.camera && !screen_description.result.value.camera_entity);
    assert(screen_description.result.value.width==320 && screen_description.result.value.height==200);
    OK(rohr_screen_camera_set(screen.result.value,camera.result.value));
    assert(rohr_screen_description_get(screen.result.value).result.value.camera==camera.result.value);
    TextureAssetResult texture=rohr_graphics_texture_load(project.parity.flipped); OK(texture);
    AnimationAssetResult animation=rohr_graphics_animation_load(project.parity.moving); OK(animation);
    FontAssetResult font=rohr_graphics_font_load(project.play.label); OK(font);
    TextAssetResult text=rohr_graphics_text_create(project.play.label); OK(text);
    assert(!strcmp(rohr_graphics_text_description_get(text.result.value).result.value.value,"Wrapped text"));
    assert(rohr_graphics_font_description_get(font.result.value).result.value.point_size==24);
    for(unsigned cycle=0;cycle<1000;++cycle) {
        unsigned evaluations=0;
        CameraIdResult c=graphics_camera_create((&project.parity.camera)[evaluations++]); OK(c);
        assert(evaluations==1);
        evaluations=0;
        TextureAssetResult t=graphics_texture_load((&project.parity.flipped)[evaluations++]); OK(t);
        assert(evaluations==1 && t.result.value.handle==texture.result.value.handle);
        evaluations=0;
        AnimationAssetResult a=graphics_animation_load((&project.parity.moving)[evaluations++]); OK(a);
        assert(evaluations==1 && a.result.value.handle==animation.result.value.handle);
        evaluations=0;
        GraphicsUiIdResult label=rohr_graphics_ui_text_create((&project.play.label)[evaluations++]); OK(label);
        assert(evaluations==1);
        GraphicsUiIdResult button=rohr_graphics_ui_shape_create(project.overlay.button); OK(button);
        GraphicsUiIdResult slider=graphics_ui_slider_create(project.play.slider); OK(slider);
        TextAsset borrowed=rohr_graphics_ui_text_description_get(label.result.value).result.value.text;
        TextDescriptionResult content=rohr_graphics_text_description_get(borrowed); OK(content);
        assert(!strcmp(content.result.value.value,"Wrapped text") && content.result.value.wrap_width==70);
        assert(rohr_graphics_ui_shape_description_get(button.result.value).result.value.button_enabled);
        assert(rohr_graphics_ui_slider_value_get(slider.result.value).result.value==3);
        assert(rohr_entity_alive_count_get()==entities);
        OK(rohr_graphics_ui_destroy(label.result.value));
        assert(!rohr_graphics_text_valid_check(borrowed));
        OK(rohr_graphics_ui_destroy(button.result.value)); OK(rohr_graphics_ui_destroy(slider.result.value));
        OK(rohr_camera_destroy(c.result.value));
        OK(rohr_graphics_texture_release(&t.result.value)); OK(rohr_graphics_animation_release(&a.result.value));
        assert(rohr_graphics_texture_valid_check(texture.result.value));
        assert(rohr_graphics_animation_valid_check(animation.result.value));
        OK(rohr_graphics_text_description_get(text.result.value));
    }
    /* Wrong resource kinds fail rather than creating owners or interpreting an
     * unrelated union payload. Pointer/unrelated C types have compile probes. */
    assert(rohr_error_check(rohr_camera_create(project.parity.body)));
    assert(rohr_error_check(rohr_graphics_texture_load(project.parity.camera)));
    assert(rohr_error_check(rohr_graphics_animation_load(project.parity.flipped)));
    assert(rohr_error_check(rohr_screen_create(project.play.label)));
    assert(rohr_error_check(rohr_graphics_ui_text_create(project.play.front)));
    assert(rohr_error_check(rohr_graphics_ui_shape_create(project.play.slider)));
    assert(rohr_error_check(rohr_graphics_ui_slider_create(project.overlay.button)));
    assert(rohr_error_check(rohr_graphics_font_load(project.play.front)));
    assert(rohr_error_check(rohr_graphics_text_create(project.play.front)));
    OK(rohr_screen_destroy(screen.result.value)); OK(rohr_camera_destroy(camera.result.value));
    OK(rohr_camera_destroy(native_camera.result.value));
    OK(rohr_graphics_texture_release(&texture.result.value)); OK(rohr_graphics_animation_release(&animation.result.value));
    OK(rohr_graphics_text_destroy(&text.result.value)); OK(rohr_graphics_font_release(&font.result.value));
    assert(rohr_entity_alive_count_get()==entities);
}

int main(void) {
    OK(rohr_directory_working_set(rohr_directory_base_get()));
    OK(rohr_engine_start()); OK(rohr_graphics_start());
    resource_creation();
    assert(project._project.root.children[0] == &project.parity._description);
    assert(project.parity.body.parent == &project.parity._description);
    assert(project.play.items[0].camera_member == &project.parity.camera._description);
    assert(project.play.front.camera_member == project.play.items[0].camera_member);
    assert(project.play.front.placement.orientation == project.play.items[0].placement.orientation);
    assert(!strcmp(project.play.label.text_value,"Wrapped text"));
    assert(project.play.slider.slider.minimum == -2 && project.play.slider.slider.value == 3);
    assert(project.overlay.button.shape.orientation == 21);
    assert(!strcmp(project.world.name,"world") && project.world.value == 5);
    assert(!strcmp(project.effects.name,"effects") && project.effects.value == 5);
    assert(project.ui_only.value == 8 && project.quoted_layer.value == 8);
    assert(!strcmp(project.parity.body.layer_name,"world"));
    assert(!strcmp(project.parity.flipped.layer_name,"effects"));
    ViewportIdResult unbound = rohr_viewport_create(project.play); OK(unbound);
    assert(rohr_screen_description_get(item(unbound.result.value,"front").screen).result.value.camera==CAMERA_INVALID);
    assert(rohr_screen_description_get(item(unbound.result.value,"rear").screen).result.value.camera==CAMERA_INVALID);
    Report empty_frame = rohr_graphics_viewport_show(unbound.result.value);
    assert(empty_frame.header.status==ROHR_REPORT_COMPLETE && empty_frame.diagnostic_count==0);
    OK(rohr_viewport_destroy(unbound.result.value));
    ViewportIdResult overlay = rohr_viewport_create(project.overlay); OK(overlay);
    OK(rohr_graphics_layer_name_id_get("quoted \"layer\"\\"));
    ViewportItemDescription button = item(overlay.result.value,"button");
    ViewportUiShapeDescription shape = rohr_graphics_ui_shape_description_get(button.ui).result.value;
    assert(shape.orientation == 21 && shape.button_enabled && shape.shape.amount_of_vertices == 4);
    assert(!strcmp(rohr_graphics_text_description_get(shape.text.text).result.value.value,"Action"));
    assert(button.layer_id == rohr_graphics_layer_name_id_get("ui_only").result.value);
    ViewportIdResult empty = rohr_viewport_create(project.empty_view); OK(empty);
    assert(!rohr_viewport_description_get(empty.result.value).result.value.enabled);
    OK(rohr_viewport_destroy(empty.result.value)); OK(rohr_viewport_destroy(overlay.result.value));
    EntityResult created = rohr_object_create(project.parity); OK(created);
    Entity first = created.result.value; ViewportId first_view = view_create(first);
    verify(first, first_view);
    uint32_t alive = rohr_entity_alive_count_get();
    for(unsigned i=0;i<1000;++i) {
        created = rohr_object_create(project.parity); OK(created);
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
    EntityResult visual = rohr_entity_create(project.parity.flipped); OK(visual);
    OK(rohr_viewport_destroy(first_view)); OK(rohr_entity_delete(first));
    assert(rohr_entity_alive_check(visual.result.value));
    OK(rohr_entity_delete(visual.result.value));
    assert(rohr_entity_alive_count_get() == 0);
    /* The same named views now expose a full project recipe, without duplicating
     * object/camera descriptions or retaining generated mutable runtime state. */
    assert(&project._project.root == &project._description);
    assert(project._project.viewports[0].description == &project.play._description);
    assert(project._project.controller_count == 2);
    assert(project._project.sound_count == 1 && project._project.music_count == 1);
    OK(rohr_audio_start());
    /* Named recipes work without whole-project creation or generated helpers. */
    assert(project.player.action_count == 2 && project.disabled.action_count == 0);
    assert(project.player.toggle.type == project.player.actions[0].type);
    assert(project.player.toggle.button_initial_state && project.player.toggle.bindings_default.count == 1);
    assert(project.player.alternate.type == INPUT_ACTION_AXIS_1D);
    for(unsigned cycle=0;cycle<1000;++cycle) {
        unsigned evaluated=0;
        InputControllerIdResult controller=rohr_controller_create((&project.player)[evaluated++]); OK(controller);
        assert(evaluated==1);
        InputActionIdResult toggle=rohr_input_action_by_name_get(controller.result.value,project.player.toggle.name); OK(toggle);
        assert(rohr_input_action_button_down_check(toggle.result.value));
        OK(rohr_input_action_button_state_set(toggle.result.value,false));
        assert(project.player.toggle.button_initial_state);
        OK(rohr_input_controller_destroy(controller.result.value));
        assert(rohr_error_check(rohr_input_action_description_get(toggle.result.value)));
    }
    InputControllerIdResult disabled=controller_create(project.disabled); OK(disabled);
    assert(!rohr_input_controller_enabled_check(disabled.result.value));
    OK(rohr_input_controller_destroy(disabled.result.value));
    SoundResult standalone_sound=rohr_audio_sound_create(project.impact); OK(standalone_sound);
    MusicResult standalone_music=rohr_audio_music_create(project.theme); OK(standalone_music);
    assert(!rohr_audio_sound_playing_check(standalone_sound.result.value));
    assert(!rohr_audio_music_playing_check(standalone_music.result.value));
    SoundDescriptionResult sound_description=rohr_audio_sound_description_get(standalone_sound.result.value); OK(sound_description);
    assert(sound_description.result.value.pan == project.impact.pan);
    OK(rohr_audio_sound_destroy(&standalone_sound.result.value)); OK(rohr_audio_music_destroy(&standalone_music.result.value));
    InputControllerIdResult independent=rohr_controller_create(project.player); OK(independent);
    InputActionIdResult independent_toggle=rohr_input_action_by_name_get(independent.result.value,project.player.toggle.name); OK(independent_toggle);
    OK(rohr_input_action_button_state_set(independent_toggle.result.value,false));
    ProjectIdResult p1 = rohr_project_create(project); OK(p1);
    ProjectIdResult p2 = rohr_project_create(project); OK(p2);
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
        assert(rohr_input_action_button_down_check(action.result.value));
        assert(!rohr_input_action_button_down_check(independent_toggle.result.value));
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
    OK(rohr_input_controller_description_get(independent.result.value));
    assert(!rohr_input_action_button_down_check(independent_toggle.result.value));
    OK(rohr_input_controller_destroy(independent.result.value));
    assert(rohr_entity_alive_count_get() == 0);
    assert(rohr_audio_stop().header.status != ROHR_REPORT_FAILED);
    rohr_graphics_stop(); rohr_engine_stop();
    return 0;
}
