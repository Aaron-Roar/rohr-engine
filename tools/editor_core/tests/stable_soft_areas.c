/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_project.h"
#include "editor_command.h"
#include "rohr.h"
#include "yyjson/yyjson.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static double mesh_area(const AreaMesh *mesh) {
    double area=0;
    for(size_t i=0;i<mesh->count;i++) {
        const Position *p=mesh->triangles[i];
        area+=fabs((p[1].x-p[0].x)*(p[2].y-p[0].y)-(p[1].y-p[0].y)*(p[2].x-p[0].x))*.5;
    }
    return area;
}
int main(void) {
    const Position square[4]={{-10,-10},{10,-10},{10,10},{-10,10}};
    Position bow[4]={{-10,-10},{10,10},{10,-10},{-10,10}};
    AreaMesh mesh={0};
    assert(area_mesh_create(bow,4,&mesh));
    assert(fabs(mesh_area(&mesh)-200)<.001);
    assert(area_polygon_contains_check(bow,4,(Position){-8,0}));
    assert(area_polygon_contains_check(bow,4,(Position){8,0}));
    assert(!area_polygon_contains_check(bow,4,(Position){0,8}));
    area_mesh_destroy(&mesh);
    EditorProject project,loaded;
    editor_project_init(&project); editor_project_init(&loaded);
    EditorObject *object=editor_project_object_add(&project,(Position){0});
    EditorSoftBody *body=editor_project_soft_body_add(&project,object);
    for(size_t i=0;i<4;i++) assert(editor_project_soft_node_add(&project,body,square[i]));
    for(size_t i=0;i<4;i++) assert(editor_project_soft_beam_add(&project,body,body->nodes[i].id,body->nodes[(i+1)%4].id));
    assert(body->area_count==1);
    EditorSoftAreaId original=body->areas[0].id;
    body->areas[0].color=0x12abcdff; body->areas[0].color_overridden=true;
    for(size_t i=0;i<4;i++) body->nodes[i].position=bow[i];
    assert(editor_project_soft_area_mesh_create(body,&body->areas[0],&mesh));
    assert(fabs(mesh_area(&mesh)-200)<.001); area_mesh_destroy(&mesh);
    assert(body->area_count==1 && body->areas[0].id==original);
    assert(editor_project_save(&project,"stable_soft_areas.json"));
    assert(!editor_result_check(editor_project_load(&loaded,"stable_soft_areas.json")));
    assert(loaded.objects[0].soft_body_items[0].area_count==1);
    assert(loaded.objects[0].soft_body_items[0].areas[0].id==original);
    /* Version 5 must preserve saved areas even when current geometry has two lobes. */
    yyjson_doc *json=yyjson_read_file("stable_soft_areas.json",0,NULL,NULL);
    assert(json);
    yyjson_mut_doc *legacy=yyjson_doc_mut_copy(json,NULL);
    yyjson_mut_val *root=yyjson_mut_doc_get_root(legacy);
    yyjson_mut_obj_put(root,yyjson_mut_str(legacy,"format_version"),yyjson_mut_uint(legacy,5));
    yyjson_mut_val *obj=yyjson_mut_arr_get(yyjson_mut_obj_get(root,"objects"),0);
    yyjson_mut_val *soft=yyjson_mut_arr_get(yyjson_mut_obj_get(obj,"soft_bodies"),0);
    yyjson_mut_val *area=yyjson_mut_arr_get(yyjson_mut_obj_get(soft,"areas"),0);
    yyjson_mut_obj_remove_key(area,"boundary");
    assert(yyjson_mut_write_file("stable_soft_areas_legacy.json",legacy,0,NULL,NULL));
    yyjson_mut_doc_free(legacy); yyjson_doc_free(json);
    assert(!editor_result_check(editor_project_load(&loaded,"stable_soft_areas_legacy.json")));
    assert(loaded.objects[0].soft_body_items[0].area_count==1);
    assert(loaded.objects[0].soft_body_items[0].areas[0].id==original);
    assert(remove("stable_soft_areas_legacy.json")==0);
    uint32_t node_reference=body->areas[0].boundary[0].nodes[0];
    body->areas[0].boundary[0].nodes[0]=999;
    assert(editor_project_save(&project,"stable_soft_areas.json"));
    EditorResult invalid=editor_project_load(&loaded,"stable_soft_areas.json");
    assert(editor_result_check(invalid) && strstr(invalid.result.error.message,"missing node 999"));
    assert(loaded.objects[0].soft_body_items[0].areas[0].id==original);
    body->areas[0].boundary[0].nodes[0]=node_reference;
    for(size_t i=0;i<4;i++) body->nodes[i].position=square[i];
    assert(editor_project_soft_beam_add(&project,body,body->nodes[0].id,body->nodes[2].id));
    assert(body->area_count==2);
    for(size_t i=0;i<2;i++) assert(body->areas[i].id!=original && body->areas[i].color==0x12abcdff);
    assert(editor_project_soft_beam_add(&project,body,body->nodes[1].id,body->nodes[3].id));
    assert(body->area_count==4);
    double sum=0;
    for(size_t i=0;i<4;i++) {
        assert(body->areas[i].node_count==3);
        assert(body->areas[i].color==0x12abcdff);
        assert(editor_project_soft_area_mesh_create(body,&body->areas[i],&mesh));
        sum+=mesh_area(&mesh); area_mesh_destroy(&mesh);
    }
    assert(fabs(sum-400)<.001);
    EditorSoftAreaId ids[4];
    for(size_t i=0;i<4;i++) ids[i]=body->areas[i].id;
    /* Deformation stops one crossing; stored interpolation remains well-defined. */
    body->nodes[2].position=(Position){-30,-40};
    assert(editor_project_save(&project,"stable_soft_areas.json"));
    assert(!editor_result_check(editor_project_load(&loaded,"stable_soft_areas.json")));
    EditorSoftBody *copy=&loaded.objects[0].soft_body_items[0];
    assert(copy->area_count==4);
    for(size_t i=0;i<4;i++) {
        assert(copy->areas[i].id==ids[i]);
        assert(editor_project_soft_area_mesh_create(copy,&copy->areas[i],&mesh));
        assert(isfinite(mesh_area(&mesh))); area_mesh_destroy(&mesh);
    }
    /* Runtime carries the same material boundary and independent stable handle. */
    assert(!rohr_error_check(rohr_engine_start()));
    EntityResult created=rohr_physics_soft_body_create(); assert(!rohr_error_check(created));
    Entity runtime=created.result.value,nodes[4],beams[6];
    for(size_t i=0;i<4;i++) {
        created=rohr_physics_soft_body_node_create(runtime,body->nodes[i].position,1,1);
        assert(!rohr_error_check(created)); nodes[i]=created.result.value;
    }
    for(size_t i=0;i<6;i++) {
        size_t a=0,b=0;
        while(body->nodes[a].id!=body->beams[i].node_a) a++;
        while(body->nodes[b].id!=body->beams[i].node_b) b++;
        created=rohr_physics_soft_body_beam_create(runtime,nodes[a],nodes[b],1,0);
        assert(!rohr_error_check(created)); beams[i]=created.result.value;
    }
    for(size_t i=0;i<4;i++) {
        AreaBoundaryPoint boundary[3]; memcpy(boundary,body->areas[i].boundary,sizeof(boundary));
        for(size_t k=0;k<3;k++) {
            for(size_t j=0;j<4;j++) if(boundary[k].nodes[j]) {
                size_t n=0; while(body->nodes[n].id!=boundary[k].nodes[j]) n++;
                boundary[k].nodes[j]=nodes[n];
            }
            for(size_t j=0;j<3;j++) {
                uint32_t *id=j==2?&boundary[k].edge:&boundary[k].beams[j];
                if(*id) { size_t n=0; while(body->beams[n].id!=*id) n++; *id=beams[n]; }
            }
        }
        created=rohr_physics_soft_body_area_create(runtime,boundary,3);
        assert(!rohr_error_check(created));
        AreaMesh editor={0};
        assert(editor_project_soft_area_mesh_create(body,&body->areas[i],&editor));
        assert(rohr_soft_body_area_mesh_create(created.result.value,&mesh));
        assert(fabs(mesh_area(&mesh)-mesh_area(&editor))<.001);
        area_mesh_destroy(&mesh); area_mesh_destroy(&editor);
    }
    SoftBodyResult runtime_body=rohr_physics_soft_body_get(runtime);
    assert(runtime_body.result.value.area_count==4);
    Entity old=runtime_body.result.value.areas[0];
    for(size_t i=0;i<4;i++) {
        assert(!rohr_error_check(rohr_physics_position_set(nodes[i],square[i])));
        assert(!rohr_error_check(rohr_graphics_soft_body_area_style_set(
            runtime_body.result.value.areas[i],(Color){1,2,3,255},true,true,true)));
        assert(!rohr_error_check(rohr_graphics_layer_entity_set(runtime_body.result.value.areas[i],17)));
    }
    assert(!rohr_error_check(rohr_physics_soft_body_areas_rebuild(runtime)));
    assert(!rohr_error_check(rohr_physics_soft_body_area_get(old)));
    assert(!rohr_error_check(rohr_entity_delete(beams[5])));
    assert(!rohr_error_check(rohr_physics_soft_body_areas_rebuild(runtime)));
    runtime_body=rohr_physics_soft_body_get(runtime);
    assert(runtime_body.result.value.area_count==2);
    for(size_t i=0;i<2;i++) {
        SoftBodyAreaResult value=rohr_physics_soft_body_area_get(runtime_body.result.value.areas[i]);
        assert(!rohr_error_check(value) && value.result.value.draw_color.red==1);
        assert(rohr_graphics_layer_entity_get(runtime_body.result.value.areas[i]).result.value==17);
    }
    assert(!rohr_error_check(rohr_entity_delete(runtime)));
    assert(rohr_error_check(rohr_physics_soft_body_area_get(old)));
    rohr_engine_stop();
    assert(remove("stable_soft_areas.json")==0);
    editor_project_destroy(&loaded); editor_project_destroy(&project);
    return 0;
}
