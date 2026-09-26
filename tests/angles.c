/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "test_png.h"
#include <float.h>
#include <math.h>
#include <stdio.h>

#define CHECK(c) do { if(!(c)) { fprintf(stderr, "angles line %d: %s\n", __LINE__, #c); return false; } } while(0)
#define OK(c) CHECK(!rohr_error_check(c))
static bool near(float a, float b) { return fabsf(a-b) < 0.001f; }
static bool point(Vec2D a, Vec2D b) { return near(a.x,b.x) && near(a.y,b.y); }

static void rotation_scene_draw(CameraId camera, void *context) {
    (void)camera; (void)context;
    graphics_background_draw((Color){0,0,0,255});
    (void)graphics_screen_quad_draw((Position){200,200},80,10,45,(Color){255,0,0,255});
    (void)rohr_graphics_screen_quad_radians_draw((Position){400,200},80,10,PI_F/4,(Color){255,0,0,255});
    (void)graphics_screen_quad_draw((Position){320,20},20,20,0,(Color){255,0,0,255});
}

static bool red_pixel_check(int x, int y, bool expected) {
    int count=0;
    SDL_Window **windows=SDL_GetWindows(&count);
    CHECK(windows != NULL && count > 0);
    SDL_Renderer *renderer=SDL_GetRenderer(windows[0]);
    SDL_free(windows);
    SDL_Surface *pixels=SDL_RenderReadPixels(renderer,NULL);
    CHECK(pixels != NULL);
    Uint8 r=0,g=0,b=0,a=0;
    bool read=SDL_ReadSurfacePixel(pixels,x,y,&r,&g,&b,&a);
    SDL_DestroySurface(pixels);
    CHECK(read);
    CHECK((r > 200 && g < 30 && b < 30) == expected);
    return true;
}

static bool math_check(void) {
    const float angles[] = {0,90,180,270,-450,810};
    const Vec2D expected[] = {{0,1},{1,0},{0,-1},{-1,0},{-1,0},{1,0}};
    Shape shape = {.amount_of_vertices=3,.vertices={{0,1},{0,0},{1,0}}};
    for(size_t n=0;n<6;n+=1) {
        float a=angles[n], r=math_degrees_to_radians(a);
        CHECK(near(math_radians_to_degrees(r),a));
        CHECK(near(rohr_math_radians_to_degrees(rohr_math_degrees_to_radians(a)),a));
        CHECK(point(math_vector_rotate((Vec2D){0,1},a),expected[n]));
        CHECK(point(rohr_math_vector_rotate((Vec2D){0,1},a),expected[n]));
        CHECK(point(math_vector_radians_rotate((Vec2D){0,1},r),expected[n]));
        CHECK(point(rohr_math_vector_radians_rotate((Vec2D){0,1},r),expected[n]));
        CHECK(point(math_vector_rotate(math_vector_rotate((Vec2D){3,7},a),-a),(Vec2D){3,7}));
        CHECK(point(math_vector_rotate((Vec2D){3,7},a+30),
            math_vector_rotate(math_vector_rotate((Vec2D){3,7},a),30)));
        CHECK(point(physics_shape_world_translate(shape,(Position){0},a).vertices[0],expected[n]));
        CHECK(point(rohr_physics_shape_world_translate(shape,(Position){0},a).vertices[0],expected[n]));
        CHECK(point(physics_shape_world_radians_translate(shape,(Position){0},r).vertices[0],expected[n]));
        CHECK(point(rohr_physics_shape_world_radians_translate(shape,(Position){0},r).vertices[0],expected[n]));
    }
    CHECK(near(math_cross_2d((Vec2D){1,0},(Vec2D){0,1}),1));
    CHECK(point(math_angular_velocity_cross_vec(180,(Vec2D){0,2}),(Vec2D){2*PI_F,0}));
    CHECK(point(rohr_math_angular_velocity_cross_vec(180,(Vec2D){0,2}),(Vec2D){2*PI_F,0}));
    CHECK(point(math_angular_velocity_radians_cross_vec(PI_F,(Vec2D){0,2}),(Vec2D){2*PI_F,0}));
    CHECK(point(rohr_math_angular_velocity_radians_cross_vec(PI_F,(Vec2D){0,2}),(Vec2D){2*PI_F,0}));
    return true;
}

static bool physics_check(void) {
    EntityResult made=entity_add(), follower=entity_add();
    OK(made); OK(follower);
    Entity e=made.result.value, f=follower.result.value;
    EntityIndex i=rohr_entity_index_get(e).result.value;
    OK(physics_orientation_radians_set(e,PI_F/2));
    CHECK(near(orientations[i],90));
    OK(rohr_physics_orientation_radians_set(e,-2.5f*PI_F));
    CHECK(near(orientations[i],-450));
    OK(rohr_physics_orientation_set(e,810)); CHECK(orientations[i]==810);
    OK(physics_angular_velocity_radians_set(e,PI_F));
    CHECK(near(rohr_physics_angular_velocity_get(e).result.value,180));
    CHECK(near(physics_angular_velocity_radians_get(e).result.value,PI_F));
    OK(rohr_physics_angular_velocity_radians_set(e,-PI_F));
    CHECK(near(physics_angular_velocity_get(e).result.value,-180));
    CHECK(near(rohr_physics_angular_velocity_radians_get(e).result.value,-PI_F));
    OK(physics_angular_velocity_maximum_radians_set(e,2*PI_F));
    CHECK(near(rohr_physics_angular_velocity_maximum_get(e).result.value,360));
    CHECK(near(physics_angular_velocity_maximum_radians_get(e).result.value,2*PI_F));
    OK(rohr_physics_angular_velocity_maximum_radians_set(e,PI_F));
    CHECK(near(rohr_physics_angular_velocity_maximum_radians_get(e).result.value,PI_F));
    OK(physics_angular_acceleration_radians_set(e,PI_F));
    CHECK(near(angular_accelerations[i],180));
    OK(rohr_physics_angular_acceleration_radians_set(e,-PI_F));
    CHECK(near(angular_accelerations[i],-180));
    OK(physics_angle_lock_radians_set(e,-2.5f*PI_F,4.5f*PI_F));
    CHECK(near(angle_locks[i].min,-450) && near(angle_locks[i].max,810));
    OK(rohr_physics_angle_lock_radians_set(e,-PI_F,PI_F));
    CHECK(near(angle_locks[i].min,-180));
    OK(physics_transform_lock_radians_set(f,e,(Vec2D){0},PI_F/2,true,true,true));
    EntityIndex fi=rohr_entity_index_get(f).result.value;
    CHECK(near(transform_locks[fi].local_angle,90));
    OK(rohr_physics_transform_lock_radians_set(f,e,(Vec2D){0},-PI_F,true,true,true));
    CHECK(near(transform_locks[fi].local_angle,-180));
    CHECK(rohr_error_check(physics_orientation_radians_set(e,FLT_MAX)));
    CHECK(orientations[i]==810);
    CHECK(rohr_error_check(rohr_physics_angular_velocity_radians_set(e,INFINITY)));
    CHECK(near(angular_velocities[i],-180));
    CHECK(rohr_error_check(physics_angular_velocity_radians_get(ENTITY_INVALID)));
    CHECK(rohr_error_check(rohr_physics_angular_velocity_maximum_radians_get(ENTITY_INVALID)));
    OK(entity_delete(f)); OK(entity_delete(e));

    made=entity_add(); OK(made); e=made.result.value;
    i=rohr_entity_index_get(e).result.value;
    OK(physics_hitbox_set(e,math_square_create(2,2)));
    OK(physics_mass_set(e,3)); /* I = 2 */
    OK(physics_dynamic_set(e));
    OK(physics_orientation_set(e,0));
    OK(physics_angular_velocity_set(e,0));
    OK(physics_angular_acceleration_set(e,0));
    OK(physics_angular_impulse_apply(e,PI_F));
    CHECK(near(angular_velocities[i],90));
    OK(system_physics_update(1));
    CHECK(near(orientations[i],90));
    CHECK(point(math_vector_rotate((Vec2D){0,1},orientations[i]),(Vec2D){1,0}));
    OK(entity_delete(e));

    /* Positive soft-body torque rotates the top node toward the right. */
    EntityResult soft=physics_soft_body_create(); OK(soft);
    EntityResult top=physics_soft_body_node_create(soft.result.value,(Position){0,2},1,0.1f);
    EntityResult bottom=physics_soft_body_node_create(soft.result.value,(Position){0,-2},1,0.1f);
    OK(top); OK(bottom);
    OK(physics_soft_body_torque_apply(soft.result.value,8));
    OK(system_physics_update(0.1));
    CHECK(velocities[rohr_entity_index_get(top.result.value).result.value].x > 0);
    CHECK(velocities[rohr_entity_index_get(bottom.result.value).result.value].x < 0);
    OK(entity_delete(soft.result.value));
    return true;
}

static bool graphics_check(void) {
    OK(rohr_graphics_start());
    CameraId camera=rohr_camera_active_get();
    Camera c=rohr_camera_get(camera).result.value;
    c.orientation=90;
    OK(rohr_camera_set(camera,c));
    Position center=rohr_graphics_world_to_screen_get(c.position);
    Position up=rohr_graphics_world_to_screen_get((Position){c.position.x,c.position.y+10});
    CHECK(up.x < center.x && near(up.y,center.y));
    CHECK(point(rohr_graphics_screen_to_world_get(up),(Position){c.position.x,c.position.y+10}));
    graphics_camera_radians_rotate(PI_F/2);
    CHECK(near(rohr_camera_get(camera).result.value.orientation,180));
    rohr_graphics_camera_radians_rotate(PI_F/2);
    CHECK(near(rohr_camera_get(camera).result.value.orientation,270));
    EntityResult made=entity_add(); OK(made); Entity e=made.result.value;
    OK(physics_position_set(e,(Position){0}));
    OK(physics_orientation_set(e,90));
    OK(graphics_camera_radians_attach(e,(Vec2D){0,2},PI_F/2));
    CHECK(point(rohr_camera_get(camera).result.value.position,(Position){2,0}));
    CHECK(near(rohr_camera_get(camera).result.value.orientation,180));
    OK(rohr_graphics_camera_radians_attach(e,(Vec2D){0},PI_F));
    CHECK(near(rohr_camera_get(camera).result.value.orientation,270));
    OK(graphics_camera_with_options_radians_attach(e,(Vec2D){0},PI_F,true,false));
    CHECK(near(rohr_camera_get(camera).result.value.orientation,180));
    OK(rohr_graphics_camera_with_options_radians_attach(e,(Vec2D){0},PI_F/2,true,true));
    CHECK(near(rohr_camera_get(camera).result.value.orientation,180));
    OK(graphics_camera_attachment_radians_set(camera,e,(Vec2D){0},PI_F,true,true));
    CHECK(near(rohr_camera_get(camera).result.value.orientation,270));
    OK(rohr_camera_radians_attach(camera,e,(Vec2D){0},-PI_F,true,true));
    CHECK(near(rohr_camera_get(camera).result.value.orientation,-90));
    CHECK(rohr_error_check(rohr_camera_radians_attach(camera,e,(Vec2D){0},FLT_MAX,true,true)));
    CHECK(near(rohr_camera_get(camera).result.value.orientation,-90));
    graphics_camera_detach();

    CHECK(SDL_SaveFile("angles_asset.png",test_png,sizeof(test_png)));
    TextureAssetResult texture=graphics_texture_load((TextureDescriptor){"angles_asset.png",{20,10}});
    OK(texture);
    OK(graphics_sprite_add(e,graphics_sprite_create(texture.result.value,(Scale){1,1})));
    OK(graphics_sprite_orientation_offset_radians_set(e,PI_F));
    CHECK(near(rohr_graphics_sprite_orientation_offset_get(e).result.value,180));
    CHECK(near(graphics_sprite_orientation_offset_radians_get(e).result.value,PI_F));
    OK(rohr_graphics_sprite_orientation_offset_radians_set(e,PI_F/2));
    CHECK(near(rohr_graphics_sprite_orientation_offset_radians_get(e).result.value,PI_F/2));
    AnimationAssetResult animation=graphics_animation_load((AnimationDescriptor){
        .id=1,.amount_of_descriptors=1,.texture_descriptors={{"angles_asset.png",{20,10}}}});
    OK(animation);
    OK(graphics_animated_sprite_add(e,graphics_animated_sprite_create(animation.result.value,(Scale){1,1})));
    OK(graphics_animated_sprite_orientation_offset_radians_set(e,PI_F));
    CHECK(near(rohr_graphics_animated_sprite_orientation_offset_get(e).result.value,180));
    CHECK(near(graphics_animated_sprite_orientation_offset_radians_get(e).result.value,PI_F));
    OK(rohr_graphics_animated_sprite_orientation_offset_radians_set(e,-PI_F/2));
    CHECK(near(rohr_graphics_animated_sprite_orientation_offset_radians_get(e).result.value,-PI_F/2));
    FontAsset font=graphics_font_default_get();
    TextAssetResult text=graphics_text_create(&font,"angle",(Color){255,255,255,255}); OK(text);
    CHECK(graphics_screen_quad_radians_draw((Position){50,50},20,10,PI_F/4,(Color){255,0,0,255}));
    CHECK(rohr_graphics_screen_quad_radians_draw((Position){100,50},20,10,PI_F/4,(Color){255,0,0,255}));
    CHECK(graphics_screen_text_scaled_rotated_radians_draw(&text.result.value,(Position){50,100},(Scale){1,1},PI_F/4));
    CHECK(rohr_graphics_screen_text_scaled_rotated_radians_draw(&text.result.value,(Position){100,100},(Scale){1,1},PI_F/4));
    graphics_texture_radians_draw(texture.result.value,(Position){0},PI_F/4);
    rohr_graphics_texture_radians_draw(texture.result.value,(Position){0},PI_F/4);
    graphics_screen_texture_radians_draw(texture.result.value,(Position){50,150},(Scale){20,10},PI_F/4);
    rohr_graphics_screen_texture_radians_draw(texture.result.value,(Position){100,150},(Scale){20,10},PI_F/4);
    ui_radians_quad((Position){50,200},20,10,PI_F/4,(Color){255,0,0,255});
    rohr_ui_radians_quad((Position){100,200},20,10,PI_F/4,(Color){255,0,0,255});
    /* A clockwise 45-degree slider runs down-right in screen coordinates. */
    UISliderConfig slider=ui_slider_config_default_get();
    slider.center=(Position){200,200}; slider.length=100; slider.angle=45;
    ui_frame_begin((UIInput){.pointer={225,225},.primary_button=MOUSE_BUTTON_STATE_PRESSED});
    UISliderResult hit=ui_slider("clockwise",0,&slider);
    CHECK(hit.hovered && hit.value > 0.8f);
    ui_frame_end();
    graphics_show();
    ViewportIdResult viewport=rohr_viewport_create((ViewportConfig){
        .rectangle={0,0,1100,480},.background_color={0,0,0,255}});
    OK(viewport);
    ScreenIdResult screen=rohr_screen_create((ScreenConfig){.camera=camera,.width=640,.height=480});
    ScreenIdResult second=rohr_screen_create((ScreenConfig){.camera=camera,.width=640,.height=480});
    OK(screen); OK(second);
    ViewportItemIdResult screen_item=rohr_viewport_screen_add(viewport.result.value,screen.result.value,
        (ViewportItemConfig){.rectangle={0,0,660,480},.visible=true,.layer=0});
    ViewportItemIdResult second_item=rohr_viewport_screen_add(viewport.result.value,second.result.value,
        (ViewportItemConfig){.rectangle={600,0,640,480},.orientation=90,.visible=true,.layer=2});
    OK(screen_item); OK(second_item);
    OK(rohr_camera_render_callback_set(camera,rotation_scene_draw,NULL));
    OK(rohr_viewport_enable_set(viewport.result.value));
    graphics_show();
    CHECK(red_pixel_check(230,220,true) && red_pixel_check(230,180,false));
    CHECK(red_pixel_check(430,220,true) && red_pixel_check(430,180,false));
    CHECK(red_pixel_check(940,140,true) && red_pixel_check(940,100,false));
    CHECK(red_pixel_check(1140,240,false));
    ViewportItemConfig second_config=rohr_viewport_item_get(second_item.result.value).result.value;
    second_config.orientation=30; second_config.content_orientation=60;
    OK(rohr_viewport_item_set(second_item.result.value,second_config));
    graphics_show(); CHECK(red_pixel_check(940,140,true));
    second_config.orientation=90; second_config.content_orientation=0;
    second_config.rectangle.x=520;
    OK(rohr_viewport_item_set(second_item.result.value,second_config));
    graphics_show(); CHECK(red_pixel_check(1060,240,true));
    second_config.rectangle.x=600;
    OK(rohr_viewport_item_set(second_item.result.value,second_config));
    GraphicsUiIdResult shape=graphics_ui_shape_create((ViewportUiShapeConfig){
        .shape=math_square_create(10,10),.position={30,0},.border_enabled=true,.fill_color={255,0,0,255}});
    OK(shape);
    ViewportItemIdResult item=rohr_viewport_ui_add(viewport.result.value,shape.result.value,
        (ViewportItemConfig){.rectangle={200,200,0,0},.content_scale={1,1},.orientation=90,.visible=true,.layer=1});
    OK(item); OK(rohr_viewport_enable_set(viewport.result.value));
    graphics_show();
    CHECK(red_pixel_check(200,230,true) && red_pixel_check(200,170,false));
    ViewportItemIdResult cover=rohr_viewport_ui_add(viewport.result.value,shape.result.value,
        (ViewportItemConfig){.rectangle={960,150,0,0},.content_scale={1,1},.orientation=90,.visible=true,.layer=1});
    OK(cover);
    /* UI composition follows screen composition; layers order UI siblings. */
    GraphicsUiIdResult occluder=graphics_ui_shape_create((ViewportUiShapeConfig){
        .shape=math_square_create(10,10),.position={30,0},.border_enabled=true,.fill_color={0,0,0,255}});
    OK(occluder);
    ViewportItemIdResult occluder_item=rohr_viewport_ui_add(viewport.result.value,occluder.result.value,
        (ViewportItemConfig){.rectangle={960,150,0,0},.content_scale={1,1},.orientation=90,.visible=true,.layer=2});
    OK(occluder_item);
    graphics_show(); CHECK(red_pixel_check(960,180,false));
    ViewportItemConfig cover_config=rohr_viewport_item_get(cover.result.value).result.value;
    cover_config.layer=3;
    OK(rohr_viewport_item_set(cover.result.value,cover_config));
    graphics_show(); CHECK(red_pixel_check(960,180,true));
    cover_config.visible=false;
    OK(rohr_viewport_item_set(cover.result.value,cover_config));
    graphics_show(); CHECK(red_pixel_check(960,180,false));
    cover_config.visible=true; second_config.visible=false;
    OK(rohr_viewport_item_set(cover.result.value,cover_config));
    OK(rohr_viewport_item_set(second_item.result.value,second_config));
    graphics_show();
    CHECK(red_pixel_check(960,180,true) && red_pixel_check(940,140,false));
    OK(rohr_viewport_destroy(viewport.result.value));
    OK(rohr_screen_destroy(screen.result.value));
    OK(rohr_screen_destroy(second.result.value));
    OK(rohr_camera_render_callback_set(camera,NULL,NULL));
    OK(rohr_graphics_ui_destroy(shape.result.value));
    OK(rohr_graphics_ui_destroy(occluder.result.value));
    OK(entity_delete(e));
    OK(graphics_animation_release(&animation.result.value));
    OK(graphics_texture_release(&texture.result.value));
    OK(graphics_text_destroy(&text.result.value));
    graphics_stop();
    CHECK(SDL_RemovePath("angles_asset.png"));
    return true;
}

int main(void) {
    if(rohr_error_check(rohr_engine_start())) return 1;
    bool passed=math_check() && physics_check() && graphics_check();
    rohr_engine_stop();
    return passed ? 0 : 1;
}
