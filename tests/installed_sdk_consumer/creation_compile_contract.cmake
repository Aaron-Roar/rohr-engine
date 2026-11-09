# Copyright 2026 Aaron Rohrer
# SPDX-License-Identifier: LGPL-3.0-only
# Compile against installed headers, with both generated catalogs visible.
# Check a valid translation unit first so missing includes cannot make every
# negative case appear to pass. Static libraries also support cross-compilation.
include(CheckCSourceCompiles)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_REQUIRED_LIBRARIES Rohr::Engine)
set(CMAKE_REQUIRED_INCLUDES "${object_fixture}")
function(creation_compile_case name expected expression)
    unset(creation_${name} CACHE)
    check_c_source_compiles("#include \"project.h\"
        int main(void) {
            ObjectDescription object = {0};
            const ObjectDescription immutable = {0};
            ProjectDescription root = {0};
            struct Unrelated { ObjectDescription first; } unrelated = {0};
            ${expression}; return 0;
        }" creation_${name})
    if(expected AND NOT creation_${name})
        message(FATAL_ERROR "Supported creation input rejected: ${name}")
    elseif(NOT expected AND creation_${name})
        message(FATAL_ERROR "Unsupported creation input accepted: ${name}")
    endif()
endfunction()
creation_compile_case(supported TRUE "
    rohr_entity_create(object); entity_create(immutable);
    rohr_object_create(object); object_create(immutable);
    rohr_project_create(root); project_create(project);
    rohr_object_create(project); rohr_object_create(root);
    rohr_object_create(objects.mass_demo); rohr_object_create(project.mass_demo);
    rohr_entity_create(project.mass_demo.automatic_body);
    rohr_entity_create(objects.mass_demo.automatic_body);
    rohr_project_create(*(ProjectDescriptionView_1 *)0);
    rohr_object_create(*(ObjectDescriptionView_1 *)0);
    rohr_entity_create_with_options(object, entity_create_options_default_get());
    rohr_object_create_with_options(project.mass_demo, object_create_options_default_get());
    rohr_entity_create((ObjectDescription){.id=1, .kind=OBJECT_PARTICLE});
    rohr_entity_create_with_options(((ObjectDescription){.id=1, .kind=OBJECT_PARTICLE}), entity_create_options_default_get());
    rohr_project_create((ProjectDescription){.root={.id=1, .kind=OBJECT_CONTAINER}})")
foreach(creator entity object project)
    creation_compile_case(${creator}_pointer FALSE "rohr_${creator}_create(&object)")
    creation_compile_case(${creator}_generated_pointer FALSE "rohr_${creator}_create(&project)")
    creation_compile_case(${creator}_null FALSE "rohr_${creator}_create(NULL)")
    creation_compile_case(${creator}_integer FALSE "int n=0; rohr_${creator}_create(n)")
    creation_compile_case(${creator}_unrelated FALSE "rohr_${creator}_create(unrelated)")
endforeach()
creation_compile_case(project_object FALSE "rohr_project_create(object)")
creation_compile_case(project_child FALSE "rohr_project_create(project.mass_demo)")
creation_compile_case(project_runtime_pointer FALSE "rohr_project_create(&root)")
creation_compile_case(entity_options_pointer FALSE "rohr_entity_create_with_options(&object, entity_create_options_default_get())")
creation_compile_case(object_options_unrelated FALSE "rohr_object_create_with_options(unrelated, object_create_options_default_get())")
set(CMAKE_REQUIRED_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/../../examples/player_controller/src/generated")
creation_compile_case(controller_supported TRUE "
    InputControllerDescription native = {.name=\"native\", .enabled=true};
    const ControllerDescription complete = {.controller={.name=\"complete\"}};
    rohr_controller_create(native); controller_create(complete);
    rohr_controller_create(project.player); controller_create(project.player._description);
    rohr_controller_create((InputControllerDescription){.name=\"literal\", .enabled=true});
    rohr_controller_create(*(ProjectControllerView_0 *)0);
    rohr_viewport_create(project.game_viewport);
    graphics_viewport_create(project.game_viewport);
    ViewportCreateItemDescription screen=project.game_viewport.main_screen;
    ControllerActionDescription jump=project.player.jump; (void)screen; (void)jump")
creation_compile_case(controller_pointer FALSE "InputControllerDescription c={0}; rohr_controller_create(&c)")
creation_compile_case(controller_generated_pointer FALSE "rohr_controller_create(&project.player)")
creation_compile_case(controller_action FALSE "rohr_controller_create(project.player.jump)")
creation_compile_case(controller_wrong_recipe FALSE "rohr_controller_create(project.player_world)")
creation_compile_case(entity_controller FALSE "rohr_entity_create(project.player)")
creation_compile_case(controller_unrelated FALSE "rohr_controller_create(unrelated)")
creation_compile_case(controller_null FALSE "rohr_controller_create(NULL)")
# Native temporary values and canonical authored lvalues use one creator.
creation_compile_case(graphics_supported TRUE "
    CameraDescription camera=rohr_camera_description_default_get();
    rohr_camera_create(camera); graphics_camera_create(project.player_world);
    rohr_camera_create(rohr_camera_description_default_get());
    rohr_camera_create((CameraDescription){.zoom=1});
    rohr_graphics_texture_load(project.player_world);
    graphics_animation_load(objects.player_world);
    rohr_graphics_texture_load(rohr_graphics_texture_description_default_get());
    rohr_graphics_animation_load(rohr_graphics_animation_description_default_get());
    rohr_viewport_create(rohr_viewport_description_default_get());
    rohr_viewport_create((ViewportCreateDescription){0});
    rohr_screen_create(project.game_viewport.main_screen);
    graphics_screen_create(graphics_screen_description_default_get());
    rohr_graphics_ui_shape_create(project.game_viewport.main_screen);
    rohr_graphics_ui_text_create(project.game_viewport.main_screen);
    rohr_graphics_ui_slider_create(project.game_viewport.main_screen);
    graphics_ui_shape_create(graphics_ui_shape_description_default_get());
    graphics_ui_text_create(graphics_ui_text_description_default_get());
    graphics_ui_slider_create(graphics_ui_slider_description_default_get());
    rohr_graphics_font_load(project.game_viewport.main_screen);
    rohr_graphics_text_create(project.game_viewport.main_screen);
    graphics_font_load(graphics_font_description_default_get());
    graphics_text_create(graphics_text_description_default_get())")
creation_compile_case(viewport_create_pointer FALSE "ViewportDescription d={0}; rohr_viewport_create(&d)")
creation_compile_case(viewport_create_recipe_pointer FALSE "rohr_viewport_create(&project.game_viewport)")
creation_compile_case(viewport_create_unrelated FALSE "rohr_viewport_create(unrelated)")
creation_compile_case(viewport_create_wrong_family FALSE "rohr_viewport_create(project.player)")
creation_compile_case(camera_create_pointer FALSE "CameraDescription d={0}; rohr_camera_create(&d)")
creation_compile_case(camera_create_recipe_pointer FALSE "rohr_camera_create(&project.player_world)")
creation_compile_case(camera_create_unrelated FALSE "rohr_camera_create(unrelated)")
creation_compile_case(camera_create_wrong_family FALSE "rohr_camera_create(project.player)")
creation_compile_case(screen_create_pointer FALSE "ScreenDescription d={0}; rohr_screen_create(&d)")
creation_compile_case(screen_create_recipe_pointer FALSE "rohr_screen_create(&project.game_viewport.main_screen)")
creation_compile_case(screen_create_unrelated FALSE "rohr_screen_create(unrelated)")
creation_compile_case(screen_create_wrong_family FALSE "rohr_screen_create(project.player)")
creation_compile_case(graphics_texture_load_pointer FALSE "TextureDescription d={0}; rohr_graphics_texture_load(&d)")
creation_compile_case(graphics_texture_load_recipe_pointer FALSE "rohr_graphics_texture_load(&project.player_world)")
creation_compile_case(graphics_texture_load_unrelated FALSE "rohr_graphics_texture_load(unrelated)")
creation_compile_case(graphics_texture_load_wrong_family FALSE "rohr_graphics_texture_load(project.player)")
creation_compile_case(graphics_animation_load_pointer FALSE "AnimationDescription d={0}; rohr_graphics_animation_load(&d)")
creation_compile_case(graphics_animation_load_recipe_pointer FALSE "rohr_graphics_animation_load(&project.player_world)")
creation_compile_case(graphics_animation_load_unrelated FALSE "rohr_graphics_animation_load(unrelated)")
creation_compile_case(graphics_animation_load_wrong_family FALSE "rohr_graphics_animation_load(project.player)")
creation_compile_case(graphics_font_load_pointer FALSE "FontDescription d={0}; rohr_graphics_font_load(&d)")
creation_compile_case(graphics_font_load_recipe_pointer FALSE "rohr_graphics_font_load(&project.game_viewport.main_screen)")
creation_compile_case(graphics_font_load_unrelated FALSE "rohr_graphics_font_load(unrelated)")
creation_compile_case(graphics_font_load_wrong_family FALSE "rohr_graphics_font_load(project.player)")
creation_compile_case(graphics_text_create_pointer FALSE "TextDescription d={0}; rohr_graphics_text_create(&d)")
creation_compile_case(graphics_text_create_recipe_pointer FALSE "rohr_graphics_text_create(&project.game_viewport.main_screen)")
creation_compile_case(graphics_text_create_unrelated FALSE "rohr_graphics_text_create(unrelated)")
creation_compile_case(graphics_text_create_wrong_family FALSE "rohr_graphics_text_create(project.player)")
creation_compile_case(graphics_ui_shape_create_pointer FALSE "ViewportUiShapeDescription d={0}; rohr_graphics_ui_shape_create(&d)")
creation_compile_case(graphics_ui_shape_create_recipe_pointer FALSE "rohr_graphics_ui_shape_create(&project.game_viewport.main_screen)")
creation_compile_case(graphics_ui_shape_create_unrelated FALSE "rohr_graphics_ui_shape_create(unrelated)")
creation_compile_case(graphics_ui_shape_create_wrong_family FALSE "rohr_graphics_ui_shape_create(project.player)")
creation_compile_case(graphics_ui_text_create_pointer FALSE "ViewportUiTextDescription d={0}; rohr_graphics_ui_text_create(&d)")
creation_compile_case(graphics_ui_text_create_recipe_pointer FALSE "rohr_graphics_ui_text_create(&project.game_viewport.main_screen)")
creation_compile_case(graphics_ui_text_create_unrelated FALSE "rohr_graphics_ui_text_create(unrelated)")
creation_compile_case(graphics_ui_text_create_wrong_family FALSE "rohr_graphics_ui_text_create(project.player)")
creation_compile_case(graphics_ui_slider_create_pointer FALSE "ViewportUiSliderDescription d={0}; rohr_graphics_ui_slider_create(&d)")
creation_compile_case(graphics_ui_slider_create_recipe_pointer FALSE "rohr_graphics_ui_slider_create(&project.game_viewport.main_screen)")
creation_compile_case(graphics_ui_slider_create_unrelated FALSE "rohr_graphics_ui_slider_create(unrelated)")
creation_compile_case(graphics_ui_slider_create_wrong_family FALSE "rohr_graphics_ui_slider_create(project.player)")
unset(CMAKE_REQUIRED_LIBRARIES)
unset(CMAKE_REQUIRED_INCLUDES)
unset(CMAKE_TRY_COMPILE_TARGET_TYPE)
