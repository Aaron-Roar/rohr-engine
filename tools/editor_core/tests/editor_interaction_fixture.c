/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 * Manual acceptance fixture. Requires a new/empty destination directory.
 */
#include "editor_workspace.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static EditorViewportUiItem *panel(EditorProject *p,EditorLayoutViewport *v,
        const char *name,Position position,float width,float height,uint32_t color,int layer) {
    EditorViewportUiItem *ui=editor_viewport_ui_add(p,v,EDITOR_VIEWPORT_UI_SHAPE); assert(ui);
    snprintf(ui->name,sizeof(ui->name),"%s",name);
    ui->position=position; ui->layer=layer; ui->fill_color=color;
    ui->value.shape.fill_color=color; ui->value.shape.vertex_count=4;
    Shape shape=rohr_math_square_create(width,height);
    for(int i=0;i<4;++i) ui->value.shape.vertices[i]=shape.vertices[i];
    ui->value.shape.text.text[0]='\0';
    assert(editor_project_ui_definition_sync_from_item(p,ui->id)); return ui;
}
int main(int argc,char **argv) {
    if(argc!=4) { fprintf(stderr,"usage: %s <new-project-directory> <jetbrains-font-file> <ofl-license>\n",argv[0]); return 2; }
    static EditorProject project,loaded;
    EditorWorkspace workspace={0},reloaded={0};
    EditorResult result=editor_workspace_create(&workspace,&project,argv[1]);
    if(editor_result_check(result)) { fprintf(stderr,"%s\n",result.result.error.message); return 1; }
    EditorObject *starter=&project.objects[0];
    EditorLayoutViewport *v=&project.layout_viewports[0];
    snprintf(v->name,sizeof(v->name),"review_viewport");
    project.application_window_enabled=true;
    project.application_window=(WindowPresentationDescription){.window_width=1280,.window_height=720,
        .logical_width=1280,.logical_height=720,.aspect_ratio_auto=true};
    EditorViewportCameraItem *screen=&v->camera_items[0];
    snprintf(screen->name,sizeof(screen->name),"left_screen");
    screen->placement.rectangle=(ViewportRectangle){-40,40,800,600};
    screen->placement.orientation=8; screen->placement.layer=0;
    screen->render_size_explicit=true; screen->render_width=640; screen->render_height=360;
    screen=editor_viewport_camera_add(&project,v,starter->id,starter->cameras[0].id); assert(screen);
    snprintf(screen->name,sizeof(screen->name),"right_screen");
    screen->placement.rectangle=(ViewportRectangle){600,180,620,460};
    screen->placement.orientation=-12; screen->placement.layer=2;
    screen->render_size_explicit=true; screen->render_width=480; screen->render_height=320;
    panel(&project,v,"red_between_screens",(Position){500,360},700,60,0xe05040ff,1);
    panel(&project,v,"green_above_screens",(Position){720,360},150,50,0x60df80ff,3);
    EditorViewportUiItem *slider=editor_viewport_ui_add(&project,v,EDITOR_VIEWPORT_UI_SLIDER); assert(slider);
    snprintf(slider->name,sizeof(slider->name),"review_slider");
    slider->position=(Position){850,80}; slider->layer=4; slider->rotation=15;
    slider->value.slider.minimum=0; slider->value.slider.maximum=10; slider->value.slider.value=5;
    slider->value.slider.step=1; slider->value.slider.thumb_offset=5;
    assert(editor_project_ui_definition_sync_from_item(&project,slider->id));
    size_t length=0; void *bytes=SDL_LoadFile(argv[2],&length); assert(bytes);
    char path[2048]; snprintf(path,sizeof(path),"%s/assets/review_font.ttf",argv[1]);
    FILE *file=fopen(path,"wb"); assert(file);
    assert(fwrite(bytes,1,length,file)==length); assert(fclose(file)==0); SDL_free(bytes);
    bytes=SDL_LoadFile(argv[3],&length); assert(bytes);
    snprintf(path,sizeof(path),"%s/assets/review_font_ofl.txt",argv[1]);
    file=fopen(path,"wb"); assert(file);
    assert(fwrite(bytes,1,length,file)==length); assert(fclose(file)==0); SDL_free(bytes);
    snprintf(path,sizeof(path),"%s/assets/review_font_attribution.md",argv[1]);
    file=fopen(path,"wb"); assert(file);
    assert(fputs("JetBrains Mono Bold Italic, by JetBrains.\n"
        "Source: https://github.com/JetBrains/JetBrainsMono\n"
        "Redistributed under the SIL Open Font License 1.1; see review_font_ofl.txt.\n",file)>=0);
    assert(fclose(file)==0);
    EditorUiFont *font=editor_project_ui_font_add(&project,"assets/review_font.ttf"); assert(font); font->point_size=22;
    EditorViewportUiItem *text=editor_viewport_ui_add(&project,v,EDITOR_VIEWPORT_UI_TEXT); assert(text);
    snprintf(text->name,sizeof(text->name),"clipped_text"); text->position=(Position){160,620}; text->layer=4;
    text->value.text.font=font->id; text->value.text.wrap_width=180; text->value.text.rotation=10;
    snprintf(text->value.text.text,sizeof(text->value.text.text),"Clipped text review: wrap, rotate, resize");
    text->clip_enabled=true; text->clip_rectangle=(ViewportRectangle){60,580,200,80};
    assert(editor_project_ui_definition_sync_from_item(&project,text->id));
    EditorForce *force=editor_project_force_add(&project,starter); assert(force);
    snprintf(force->name,sizeof(force->name),"push_marker");
    force->description=(ObjectForceDescription){.target={OBJECT_SOURCE_BODY,starter->rigid_bodies[1].id,0},
        .magnitude=5,.location={.kind=ROHR_FORCE_ORIGIN_POSITION,
            .reference={OBJECT_SOURCE_BODY,starter->rigid_bodies[1].id,0}},.orientation={.offset=35}};
    EditorTorque *torque=editor_project_torque_add(&project,starter); assert(torque);
    snprintf(torque->name,sizeof(torque->name),"spin_marker");
    torque->description=(ObjectTorqueDescription){.target={OBJECT_SOURCE_BODY,starter->rigid_bodies[2].id,0},.magnitude=.1f};
    for(size_t i=0;i<starter->joint_count;++i) if(starter->joint_items[i].kind==EDITOR_JOINT_SPRING) {
        starter->joint_items[i].visuals.endpoint_a=(ObjectVisualStyle){.specified=true,
            .fill_color=0xff4040ff,.border_color=0xffffffff,.border_thickness=1,
            .scale={2,1},.offset={10,0},.rotation=20};
        starter->joint_items[i].visuals.endpoint_b=(ObjectVisualStyle){.specified=true,
            .fill_color=0x4090ffff,.border_color=0xffffffff,.border_thickness=2,
            .scale={1,2},.offset={-10,0},.rotation=-20};
    }
    if(starter->sprite_count) starter->sprites[0].size.x=-starter->sprites[0].size.x;
    EditorAnimatedSprite *blank=editor_project_animated_sprite_add(&project,starter); assert(blank);
    snprintf(blank->name,sizeof(blank->name),"empty_animation");
    project.simulation_viewport=v->id;
    assert(editor_workspace_save(&workspace,&project));
    editor_project_init(&loaded);
    result=editor_workspace_load(&reloaded,&loaded,argv[1]);
    if(editor_result_check(result)) { fprintf(stderr,"%s\n",result.result.error.message); return 1; }
    assert(loaded.layout_viewports[0].camera_item_count==2);
    assert(editor_workspace_c_generate(&reloaded,&loaded));
    editor_project_destroy(&loaded); editor_project_destroy(&project);
    printf("Review project ready: %s\n",argv[1]); return 0;
}
