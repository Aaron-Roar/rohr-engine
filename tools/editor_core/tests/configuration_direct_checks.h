/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#ifndef ROHR_CONFIGURATION_DIRECT_CHECKS_H
#define ROHR_CONFIGURATION_DIRECT_CHECKS_H
/* These literal recipes are deliberately independent of generated descriptions. */
#define SAME(field) assert(a.field==b.field)
#define SAME_XY(field) do { SAME(field.x); SAME(field.y); } while(0)
#define SAME_COLOR(field) do { SAME(field.red); SAME(field.green); SAME(field.blue); SAME(field.alpha); } while(0)
static void item_compare(ViewportItemId direct,ViewportItemId generated) {
    ViewportItemDescription a=rohr_viewport_item_description_get(direct).result.value;
    ViewportItemDescription b=rohr_viewport_item_description_get(generated).result.value;
    SAME_XY(rectangle); SAME(rectangle.width); SAME(rectangle.height); SAME(fit);
    SAME(orientation); SAME_XY(content_offset); SAME_XY(content_scale); SAME(content_orientation);
    SAME(layer); SAME(visible); SAME(drag_mode); SAME(clip_enabled);
    SAME_XY(clip_rectangle); SAME(clip_rectangle.width); SAME(clip_rectangle.height);
}
static void direct_composition(const Parity *instance,const ProjectViewports *generated) {
    EntityResult target=rohr_entity_add(); OK(target);
    OK(rohr_physics_position_set(target.result.value,(Position){12,23}));
    OK(rohr_physics_orientation_set(target.result.value,0));
    CameraIdResult camera=rohr_camera_create((CameraDescription){.position={17,28},
        .dimensions={640,360},.zoom=1,.enabled=true,.pause_with_engine=true}); OK(camera);
    OK(rohr_camera_attachment_description_set(camera.result.value,(CameraAttachmentDescription){
        .entity=target.result.value,.position_offset={17,28},.follow_orientation=true}));
    {
        CameraDescription a=rohr_camera_description_get(camera.result.value).result.value;
        CameraDescription b=rohr_camera_description_get(instance->camera_camera).result.value;
        SAME_XY(position); SAME_XY(dimensions); SAME(zoom); SAME(orientation); SAME(enabled); SAME(pause_with_engine);
    }
    {
        CameraAttachmentDescription a=rohr_camera_attachment_description_get(camera.result.value).result.value;
        CameraAttachmentDescription b=rohr_camera_attachment_description_get(instance->camera_camera).result.value;
        assert(a.entity==target.result.value && b.entity==instance->body);
        SAME_XY(position_offset); SAME(orientation_offset); SAME(follow_position); SAME(follow_orientation);
    }
    ViewportIdResult viewport=rohr_viewport_create((ViewportDescription){.rectangle={0,0,450,300},
        .fit=SCREEN_FIT_CONTAIN,.background_color={0,0,0,255},.enabled=true}); OK(viewport);
    {
        ViewportDescription a=rohr_viewport_description_get(viewport.result.value).result.value;
        ViewportDescription b=rohr_viewport_description_get(generated->viewports[0]).result.value;
        SAME_XY(rectangle); SAME(rectangle.width); SAME(rectangle.height); SAME(fit); SAME(enabled); SAME_COLOR(background_color);
    }
    ScreenId screens[2];
    for(int i=0;i<2;++i) {
        ScreenIdResult screen=rohr_screen_create((ScreenDescription){camera.result.value,i?480:320,i?360:200}); OK(screen);
        screens[i]=screen.result.value;
        ScreenDescription a=rohr_screen_description_get(screens[i]).result.value;
        ScreenDescription b=rohr_screen_description_get(generated->screens[i]).result.value;
        SAME(width); SAME(height); assert(a.camera==camera.result.value && b.camera==instance->camera_camera);
        OK(rohr_viewport_screen_add(viewport.result.value,screens[i],(ViewportItemDescription){
            .rectangle={i?200:-20,i?50:10,250,200},.fit=SCREEN_FIT_CONTAIN,
            .orientation=i?-18:12,.content_scale={1,1},.layer=i?2:0,.visible=true}));
    }
    FontAssetResult font=rohr_graphics_font_load((FontDescription){.file="assets/parity_font.ttf",.point_size=24}); OK(font);
    TextAssetResult text=rohr_graphics_text_create((TextDescription){.font=font.result.value,
        .value="Wrapped text",.wrap_width=70,.color={255,255,255,255}}); OK(text);
    OK(rohr_graphics_font_release(&font.result.value)); /* Text retains its dependency. */
    GraphicsUiIdResult ui=rohr_graphics_ui_text_create((ViewportUiTextDescription){.text=text.result.value,
        .position={80,14},.scale={2,2},.orientation=17}); OK(ui);
    for(int i=0;i<2;++i) {
        ViewportItemIdResult item=rohr_viewport_ui_add(viewport.result.value,ui.result.value,(ViewportItemDescription){
            .rectangle={20,20,0,0},.content_scale={1,1},.layer=i?3:1,.visible=true,
            .clip_enabled=true,.clip_rectangle={10,15,100,80}}); OK(item);
        item_compare(item.result.value,generated->ui_items[i]);
        ViewportUiTextDescription a=rohr_graphics_ui_text_description_get(ui.result.value).result.value;
        ViewportUiTextDescription b=rohr_graphics_ui_text_description_get(generated->ui_elements[i]).result.value;
        SAME_XY(position); SAME_XY(offset); SAME_XY(scale); SAME(orientation);
        TextDescription at=rohr_graphics_text_description_get(a.text).result.value;
        TextDescription bt=rohr_graphics_text_description_get(b.text).result.value;
        assert(!strcmp(at.value,bt.value) && at.wrap_width==bt.wrap_width);
        assert(rohr_graphics_font_description_get(at.font).result.value.point_size==
            rohr_graphics_font_description_get(bt.font).result.value.point_size);
        SAME_XY(text.size);
        if(i==0) {
            ViewportItemDescription hidden=rohr_viewport_item_description_get(item.result.value).result.value;
            hidden.visible=false; OK(rohr_viewport_item_description_set(item.result.value,hidden));
            assert(rohr_viewport_item_description_get(generated->ui_items[i]).result.value.visible);
        }
    }
    GraphicsUiIdResult slider=rohr_graphics_ui_slider_create((ViewportUiSliderDescription){
        .minimum=-2,.maximum=6,.value=3,.step=.5f,.length=180,.track_thickness=6,
        .thumb_width=12,.thumb_height=24,.thumb_radius=9,.thumb_offset=4,
        .track_color={57,64,82,255},.filled_track_color={110,158,214,255},
        .thumb_color={216,230,255,255},.hover_thumb_color={255,255,255,255},
        .pressed_thumb_color={175,200,240,255},.enabled=true}); OK(slider);
    {
        ViewportUiSliderDescription a=rohr_graphics_ui_slider_description_get(slider.result.value).result.value;
        ViewportUiSliderDescription b=rohr_graphics_ui_slider_description_get(generated->ui_elements[2]).result.value;
        SAME(minimum); SAME(maximum); SAME(value); SAME(step); SAME(length); SAME(track_thickness);
        SAME(thumb_width); SAME(thumb_height); SAME(thumb_radius); SAME(thumb_offset); SAME(thumb_shape); SAME(enabled);
        SAME_COLOR(track_color); SAME_COLOR(filled_track_color); SAME_COLOR(thumb_color);
        SAME_COLOR(hover_thumb_color); SAME_COLOR(pressed_thumb_color);
    }
    ViewportItemIdResult slider_item=rohr_viewport_ui_add(viewport.result.value,slider.result.value,
        (ViewportItemDescription){.rectangle={20,20,0,0},.content_scale={1,1},.orientation=35,.layer=4,.visible=true}); OK(slider_item);
    item_compare(slider_item.result.value,generated->ui_items[2]);
    OK(rohr_graphics_ui_slider_value_set(slider.result.value,5));
    assert(rohr_graphics_ui_slider_value_get(generated->ui_elements[2]).result.value==3);
    OK(rohr_graphics_text_destroy(&text.result.value));
    assert(rohr_error_check(rohr_graphics_ui_destroy(ui.result.value))); /* Still mounted. */
    OK(rohr_viewport_destroy(viewport.result.value));
    OK(rohr_graphics_ui_destroy(ui.result.value));
    OK(rohr_graphics_ui_destroy(slider.result.value));
    for(int i=0;i<2;++i) OK(rohr_screen_destroy(screens[i]));
    OK(rohr_camera_destroy(camera.result.value)); OK(rohr_entity_delete(target.result.value));
    assert(rohr_graphics_text_description_get(generated->texts[0]).kind==ERROR_RESULT_VALUE);
}
static void direct_assets(const Parity *instance) {
    EntityResult entity=rohr_entity_add(); OK(entity);
    TextureAssetResult texture=rohr_graphics_texture_load((TextureDescription){.file="assets/flip.png",.size={-20,-10}}); OK(texture);
    OK(rohr_graphics_sprite_add(entity.result.value,(SpriteDescription){.texture=texture.result.value,
        .direction=DIRECTION_RIGHT,.scale={1,1},.follow_entity_rotation=true,.visible=true}));
    OK(rohr_graphics_texture_release(&texture.result.value));
    {
        SpriteDescription a=rohr_graphics_sprite_description_get(entity.result.value).result.value;
        SpriteDescription b=rohr_graphics_sprite_description_get(instance->sprite_flipped).result.value;
        SAME_XY(texture.size); SAME_XY(scale); SAME_XY(body_offset); SAME(orientation_offset);
        SAME(direction); SAME(follow_entity_rotation); SAME(visible);
    }
    AnimationAssetResult animation=rohr_graphics_animation_load((AnimationDescription){
        .frame_count=2,.frame_files={"assets/flip.png","assets/flip.png"},.ticks_per_frame=3,.time_per_frame=.25}); OK(animation);
    AnimatedSpriteDescriptionResult description=rohr_graphics_animated_sprite_description_from_animation_get(animation.result.value); OK(description);
    description.result.value.scale=(Scale){2,3}; description.result.value.direction=DIRECTION_LEFT;
    description.result.value.frames[0].scale=(Scale){4,6}; description.result.value.frames[0].offset=(Position){1,2};
    description.result.value.frames[0].rotation=27;
    description.result.value.frames[1].scale=(Scale){8,10}; description.result.value.frames[1].offset=(Position){-3,4};
    description.result.value.frames[1].rotation=-19;
    EntityResult animated=rohr_entity_add(); OK(animated);
    OK(rohr_graphics_animated_sprite_add(animated.result.value,description.result.value));
    OK(rohr_graphics_animation_release(&animation.result.value));
    EntityResult member=rohr_entity_object_member_by_name_get(instance->source_instance,"moving"); OK(member);
    {
        AnimatedSpriteDescription a=rohr_graphics_animated_sprite_description_get(animated.result.value).result.value;
        AnimatedSpriteDescription b=rohr_graphics_animated_sprite_description_get(member.result.value).result.value;
        SAME(frame_count); SAME(player.ticks_per_frame); SAME(player.time_per_frame);
        SAME_XY(scale); SAME(direction); SAME(visible); SAME(follow_entity_rotation);
        for(size_t i=0;i<a.frame_count;++i) {
            SAME_XY(frames[i].texture.size); SAME_XY(frames[i].scale); SAME_XY(frames[i].offset); SAME(frames[i].rotation);
        }
    }
    for(Tick tick=3;tick<=6;tick+=3) {
        assert(rohr_graphics_sprite_frames_update(tick,0).header.status!=ROHR_REPORT_FAILED);
        assert(rohr_graphics_animated_sprite_frame_index_get(animated.result.value).result.value==
            rohr_graphics_animated_sprite_frame_index_get(member.result.value).result.value);
        assert(rohr_graphics_animated_sprite_frame_index_get(animated.result.value).result.value==(tick==3?1u:0u));
    }
    OK(rohr_graphics_animated_sprite_frame_offset_set(animated.result.value,0,(Position){99,99}));
    assert(rohr_graphics_animated_sprite_frame_offset_get(member.result.value,0).result.value.x==1);
    EntityResult blank=rohr_entity_add(); OK(blank);
    OK(rohr_graphics_animated_sprite_add(blank.result.value,(AnimatedSpriteDescription){0}));
    member=rohr_entity_object_member_by_name_get(instance->source_instance,"blank"); OK(member);
    assert(rohr_graphics_animated_sprite_description_get(blank.result.value).result.value.frame_count==
        rohr_graphics_animated_sprite_description_get(member.result.value).result.value.frame_count);
    OK(rohr_entity_delete(blank.result.value)); OK(rohr_entity_delete(animated.result.value)); OK(rohr_entity_delete(entity.result.value));
    assert(rohr_graphics_sprite_description_get(instance->sprite_flipped).kind==ERROR_RESULT_VALUE);
}
#undef SAME_COLOR
#undef SAME_XY
#undef SAME
#endif
