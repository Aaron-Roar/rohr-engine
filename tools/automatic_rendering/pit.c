/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 * Reuse the frozen physics fixture; replace only its presentation adapter.
 */
#include "rohr.h"
#include <stdlib.h>
static CameraIdResult measured_camera_create(CameraDescription description);
static Report measured_show(void);
#define rohr_camera_create measured_camera_create
#define rohr_graphics_show measured_show
#include "../../tests/particle_pit.c"
#undef rohr_camera_create
#undef rohr_graphics_show
static Pit *scene;
static bool configured;
static GraphicsUiId labels[3];
static void geometry_render(CameraId camera,void *context) {
    (void)camera; Pit *p=context;
    rohr_graphics_background_draw((Color){18,24,34,255});
    shape_draw(rohr_math_square_create(250,10),(Position){0,-5},(Color){95,105,120,255});
    shape_draw(rohr_math_square_create(10,180),(Position){-120,85},(Color){95,105,120,255});
    shape_draw(rohr_math_square_create(10,180),(Position){120,85},(Color){95,105,120,255});
    Shape circle=rohr_math_circle_create(2.5f,16);
    for(unsigned i=0;i<p->count;++i)
        shape_draw(circle,p->poses[i],i/40%2 ? (Color){242,198,90,255} : (Color){87,179,235,255});
}
static CameraIdResult measured_camera_create(CameraDescription description) {
    scene=description.render_context;
    description.render_callback=geometry_render;
    description.position=(Position){0,245.0/3};
    description.dimensions=(Vec2D){1280.0/3,240};
    return rohr_camera_create(description);
}
static bool configure(void) {
    bool automatic=getenv("ROHR_AUTOMATIC")!=NULL;
    for(unsigned i=0;i<scene->count+3;++i) {
        Entity entity=i<3 ? scene->walls[i] : scene->particles[i-3];
        Color color=i<3 ? (Color){95,105,120,255} : (i-3)/40%2 ? (Color){242,198,90,255} : (Color){87,179,235,255};
        if(!result_check(rohr_entity_appearance_set(entity,(ObjectAppearance){.kind=i<3 ? OBJECT_BODY : OBJECT_PARTICLE,
            .color=color,.enabled=true,.visible=true}))) return false;
    }
    ScreenDescriptionResult screen=rohr_screen_description_get(scene->screen);
    if(rohr_error_check(screen)) return false;
    screen.result.value.content_mode=automatic ? SCREEN_CONTENT_AUTOMATIC : SCREEN_CONTENT_CALLBACK;
    screen.result.value.background_color=(Color){18,24,34,255};
    if(!result_check(rohr_screen_description_set(scene->screen,screen.result.value))) return false;
    /* Both paths use identical persistent HUD text, outside camera geometry. */
    for(unsigned i=0;i<3;++i) {
        ViewportItemDescription item=rohr_viewport_item_description_default_get();
        item.layer=10; item.fit=SCREEN_FIT_NONE; item.rectangle=(ViewportRectangle){15,630+i*28,0,0};
        ViewportUiTextDescription text={.text=scene->text[i],.scale={1,1}};
        ViewportItemIdResult added=rohr_viewport_ui_text_add(scene->viewport,text,item);
        if(rohr_error_check(added)) return false;
        labels[i]=rohr_viewport_item_description_get(added.result.value).result.value.ui;
    }
    return true;
}
static Report measured_show(void) {
    if(!configured) { if(!configure()) abort(); configured=true; }
    for(unsigned i=0;i<3;++i) {
        ViewportUiTextDescription text={.text=scene->text[i],.scale={1,1},
            .position={scene->text[i].size.x*.5,scene->text[i].size.y*.5}};
        if(!result_check(rohr_graphics_ui_text_description_set(labels[i],text))) abort();
    }
    return rohr_graphics_show();
}
