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
#include <stdlib.h>
#include <string.h>

static double mesh_area(const AreaMesh *mesh) {
    double area=0;
    for(size_t i=0;i<mesh->count;i++) {
        const Position *p=mesh->triangles[i];
        area+=fabs((p[1].x-p[0].x)*(p[2].y-p[0].y)-(p[1].y-p[0].y)*(p[2].x-p[0].x))*.5;
    }
    return area;
}

static void mesh_on_beams_check(const EditorSoftBody *body, const AreaMesh *mesh) {
    for(size_t i = 0; i < mesh->edge_count; i++) {
        bool on_beam = false;
        for(size_t b = 0; b < body->beam_count; b++) {
            Position a = {0}, end = {0};
            for(size_t n = 0; n < body->node_count; n++) {
                if(body->nodes[n].id == body->beams[b].node_a) a = body->nodes[n].position;
                if(body->nodes[n].id == body->beams[b].node_b) end = body->nodes[n].position;
            }
            bool both = true;
            for(size_t k = 0; k < 2; k++) {
                Position p = mesh->edges[i][k];
                both &= fabs((end.x-a.x)*(p.y-a.y)-(end.y-a.y)*(p.x-a.x)) < .001;
                both &= (p.x-a.x)*(p.x-end.x)+(p.y-a.y)*(p.y-end.y) <= .001;
            }
            on_beam |= both;
        }
        assert(on_beam);
    }
}

static void self_crossing_overlap_check(void) {
    /* Five-point star: its center is enclosed by beams and must remain filled,
     * although an even-odd fill would incorrectly cut it out. */
    AreaSegment segments[5]; AreaBoundaryPoint boundary[5] = {0};
    Position points[5];
    for(size_t i = 0; i < 5; i++) {
        double angle = (double)i * 4 * 3.141592653589793 / 5;
        points[i] = (Position){(float)(100*cos(angle)), (float)(100*sin(angle))};
    }
    for(size_t i = 0; i < 5; i++) {
        segments[i] = (AreaSegment){i+1, i+1, (i+1)%5+1, points[i], points[(i+1)%5]};
        boundary[i] = (AreaBoundaryPoint){.nodes={i+1}, .edge=i+1};
    }
    AreaMesh mesh = {0};
    assert(area_boundary_mesh_create(segments, 5, boundary, 5, &mesh));
    assert(area_mesh_contains_check(&mesh, (Position){0,0}));
    assert(!area_mesh_contains_check(&mesh, (Position){200,0}));
    area_mesh_destroy(&mesh);
}

static void changing_crossings_coverage_check(void) {
    AreaSegment segments[8];
    Position points[6] = {{-100,-80},{0,-100},{100,-50},{80,80},{-20,100},{-100,30}};
    for(size_t i=0;i<6;i++) segments[i]=(AreaSegment){i+1,i+1,(i+1)%6+1,points[i],points[(i+1)%6]};
    segments[6]=(AreaSegment){7,1,4,points[0],points[3]};
    segments[7]=(AreaSegment){8,2,5,points[1],points[4]};
    AreaFaces definitions={0};
    assert(area_faces_create(segments,8,&definitions));
    unsigned random_state=1234;
    for(size_t pose=0;pose<50;pose++) {
        for(size_t i=0;i<6;i++) {
            random_state=random_state*1664525u+1013904223u;
            points[i].x=(float)(random_state%20001)/100-100;
            random_state=random_state*1664525u+1013904223u;
            points[i].y=(float)(random_state%20001)/100-100;
        }
        for(size_t i=0;i<8;i++) {
            segments[i].a=points[segments[i].node_a-1];
            segments[i].b=points[segments[i].node_b-1];
        }
        AreaMesh *meshes=calloc(definitions.count,sizeof(*meshes));
        assert(meshes && area_boundary_meshes_create(segments,8,definitions.items,definitions.count,meshes));
        AreaFaces current={0};
        assert(area_faces_create(segments,8,&current));
        for(size_t i=0;i<current.count;i++) {
            Position *boundary=malloc(current.items[i].count*sizeof(*boundary));
            assert(boundary);
            for(size_t k=0;k<current.items[i].count;k++)
                assert(area_boundary_position_get(segments,8,current.items[i].points[k],&boundary[k]));
            AreaMesh region={0};
            assert(area_mesh_create(boundary,current.items[i].count,&region));
            free(boundary);
            for(size_t t=0;t<region.count;t++) {
                const Position *p=region.triangles[t];
                double twice_area=fabs((p[1].x-p[0].x)*(p[2].y-p[0].y)-(p[1].y-p[0].y)*(p[2].x-p[0].x));
                /* Float-sized slivers can round their centroid onto an edge. */
                if(twice_area<.01) continue;
                Position sample={(p[0].x+p[1].x+p[2].x)/3,(p[0].y+p[1].y+p[2].y)/3};
                bool covered=false;
                for(size_t a=0;a<definitions.count;a++) covered |= area_mesh_contains_check(&meshes[a],sample);
                assert(covered);
            }
            area_mesh_destroy(&region);
        }
        area_faces_destroy(&current);
        for(size_t i=0;i<definitions.count;i++) area_mesh_destroy(&meshes[i]);
        free(meshes);
    }
    area_faces_destroy(&definitions);
}

static void explicit_hole_check(void) {
    Position points[8]={{-10,-10},{10,-10},{10,10},{-10,10},
        {-2,-2},{2,-2},{2,2},{-2,2}};
    AreaSegment segments[8];
    for(size_t i=0;i<8;i++) {
        size_t next=i/4*4+(i+1)%4;
        segments[i]=(AreaSegment){i+1,i+1,next+1,points[i],points[next]};
    }
    AreaFaces faces={0}; AreaMesh mesh={0};
    assert(area_faces_create(segments,8,&faces) && faces.count==2);
    size_t outer=faces.items[0].count>4?0:1;
    /* No inner-area definition is needed to preserve an explicit hole. */
    assert(area_boundary_meshes_create(segments,8,&faces.items[outer],1,&mesh));
    assert(area_mesh_contains_check(&mesh,(Position){5,0}));
    assert(!area_mesh_contains_check(&mesh,(Position){0,0}));
    assert(fabs(mesh_area(&mesh)-384)<.001);
    area_mesh_destroy(&mesh); area_faces_destroy(&faces);
}

static Position motion_point_get(Position p, Position offset, double angle, double scale) {
    double x = p.x + .11*sin(angle)*p.y, y = p.y;
    return (Position){(float)(offset.x+scale*(cos(angle)*x-sin(angle)*y)),
        (float)(offset.y+scale*(sin(angle)*x+cos(angle)*y))};
}

static void continuous_motion_ownership_check(void) {
    const Position square[4] = {{-70,-70},{70,-70},{70,70},{-70,70}};
    const Position samples[4] = {{0,-45},{45,0},{0,45},{-45,0}};
    AreaSegment segments[6];
    for(size_t i = 0; i < 4; i++)
        segments[i] = (AreaSegment){i+1,i+1,(i+1)%4+1,square[i],square[(i+1)%4]};
    segments[4] = (AreaSegment){5,1,3,square[0],square[2]};
    segments[5] = (AreaSegment){6,2,4,square[1],square[3]};
    AreaFaces definitions = {0};
    assert(area_faces_create(segments,6,&definitions) && definitions.count==4);
    unsigned owners[4] = {0};
    for(size_t frame = 0; frame < 7000; frame++) {
        Position offset = {(float)frame*.0137f,(float)frame*1.0173f};
        double angle = 0, scale = 1;
        if(frame >= 5000) {
            const double scales[] = {.02,1,50,1};
            size_t phase = (frame-5000)/500;
            scale = scales[phase];
            angle = ((frame-5000)%500)*2*3.141592653589793/500;
            offset = (Position){(phase%2 ? -10000 : 10000)+(float)frame*.0031f,
                (phase%2 ? 10000 : -10000)+(float)frame*.013f};
        }
        for(size_t i = 0; i < 6; i++) {
            segments[i].a = motion_point_get(square[segments[i].node_a-1],offset,angle,scale);
            segments[i].b = motion_point_get(square[segments[i].node_b-1],offset,angle,scale);
        }
        AreaMesh meshes[4] = {0}, individual[4] = {0};
        assert(area_boundary_meshes_create(segments,6,definitions.items,4,meshes));
        for(size_t i = 0; i < 4; i++)
            assert(area_boundary_mesh_create(segments,6,definitions.items[i].points,
                definitions.items[i].count,&individual[i]));
        for(size_t i = 0; i < 4; i++) {
            Position sample = motion_point_get(samples[i],offset,angle,scale);
            unsigned combined = 0, raw = 0;
            for(size_t j = 0; j < 4; j++) {
                if(area_mesh_contains_check(&meshes[j],sample)) combined |= 1u<<j;
                if(area_mesh_contains_check(&individual[j],sample)) raw |= 1u<<j;
            }
            if(frame == 0) owners[i] = combined;
            /* One unchanged owner, not merely at least one colored surface. */
            assert(owners[i] && !(owners[i] & (owners[i]-1)));
            if(combined != owners[i] || raw != owners[i]) {
                fprintf(stderr,"motion ownership changed: frame %zu region %zu expected %x combined %x raw %x\n",
                    frame,i,owners[i],combined,raw);
                assert(false);
            }
        }
        for(size_t i = 0; i < 4; i++) {
            area_mesh_destroy(&meshes[i]); area_mesh_destroy(&individual[i]);
        }
    }
    area_faces_destroy(&definitions);
}

static void simulated_motion_ownership_check(void) {
    const Position square[4] = {{-70,-70},{70,-70},{70,70},{-70,70}};
    const Position samples[4] = {{0,-45},{45,0},{0,45},{-45,0}};
    const Color colors[4] = {{255,0,0,255},{0,255,0,255},{0,0,255,255},{255,255,0,255}};
    EntityResult created = rohr_physics_soft_body_create();
    assert(!rohr_error_check(created));
    Entity body = created.result.value, nodes[4];
    for(size_t i = 0; i < 4; i++) {
        created = rohr_physics_soft_body_node_create(body,square[i],1,1);
        assert(!rohr_error_check(created)); nodes[i] = created.result.value;
        assert(!rohr_error_check(rohr_physics_gravity_disable(nodes[i])));
        assert(!rohr_error_check(rohr_physics_velocity_set(nodes[i],(Velocity){1.37f,101.73f})));
    }
    const size_t ends[6][2] = {{0,1},{1,2},{2,3},{3,0},{0,2},{1,3}};
    for(size_t i = 0; i < 6; i++) {
        created = rohr_physics_soft_body_beam_create(body,nodes[ends[i][0]],nodes[ends[i][1]],100,1);
        assert(!rohr_error_check(created));
        assert(!rohr_error_check(rohr_physics_soft_body_beam_collision_disable(created.result.value)));
    }
    assert(!rohr_error_check(rohr_physics_soft_body_areas_rebuild(body)));
    SoftBodyResult stored = rohr_physics_soft_body_get(body);
    assert(!rohr_error_check(stored) && stored.result.value.area_count==4);
    for(size_t i = 0; i < 4; i++) assert(!rohr_error_check(rohr_graphics_soft_body_area_style_set(
        stored.result.value.areas[i],colors[i],true,true,true)));
    unsigned owners[4] = {0};
    for(size_t frame = 0; frame < 1024; frame++) {
        if(frame) assert(!rohr_error_check(rohr_system_physics_update(.01)));
        Position center = {0};
        for(size_t i = 0; i < 4; i++) {
            PositionResult position = rohr_physics_position_get(nodes[i]);
            assert(!rohr_error_check(position));
            center.x += position.result.value.x*.25f; center.y += position.result.value.y*.25f;
        }
        AreaMesh meshes[4] = {0};
        assert(physics_soft_body_area_meshes_create(body,meshes,4));
        for(size_t i = 0; i < 4; i++) {
            Position point = {samples[i].x+center.x,samples[i].y+center.y};
            unsigned owner = 0;
            for(size_t j = 0; j < 4; j++) if(area_mesh_contains_check(&meshes[j],point)) owner |= 1u<<j;
            if(frame == 0) owners[i] = owner;
            assert(owners[i] && !(owners[i] & (owners[i]-1)) && owner==owners[i]);
            SoftBodyAreaResult area = rohr_physics_soft_body_area_get(stored.result.value.areas[i]);
            assert(!rohr_error_check(area) && area.result.value.draw_color_overridden);
            assert(memcmp(&area.result.value.draw_color,&colors[i],sizeof(Color))==0);
        }
        for(size_t i = 0; i < 4; i++) area_mesh_destroy(&meshes[i]);
    }
    PositionResult moved = rohr_physics_position_get(nodes[0]);
    assert(!rohr_error_check(moved) && moved.result.value.y>square[0].y+500);
    assert(!rohr_error_check(rohr_entity_delete(body)));
}

int main(void) {
    self_crossing_overlap_check();
    changing_crossings_coverage_check();
    explicit_hole_check();
    continuous_motion_ownership_check();
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
    for(size_t i=0;i<4;i++) {
        ids[i]=body->areas[i].id;
        body->areas[i].color = 0x112233ff + (uint32_t)i * 0x20100000;
    }
    /* Live intersections move along both beams, rather than retaining their
     * authored fractions. Every resulting exterior segment stays on a beam. */
    body->nodes[2].position = (Position){30,10};
    sum = 0;
    for(size_t i=0;i<4;i++) {
        for(size_t k=0;k<body->areas[i].node_count;k++) if(body->areas[i].boundary[k].beams[0]) {
            Position p;
            assert(editor_project_soft_area_position_get(body,&body->areas[i],k,&p));
            assert(fabs(p.x-10.0/3)<.001 && fabs(p.y+10.0/3)<.001);
        }
        assert(editor_project_soft_area_mesh_create(body,&body->areas[i],&mesh));
        mesh_on_beams_check(body,&mesh);
        sum += mesh_area(&mesh);
        area_mesh_destroy(&mesh);
    }
    assert(fabs(sum-600)<.001);
    /* Crossing disappears: current faces can belong to multiple definitions,
     * but no definition, color, or ID is destroyed. */
    body->nodes[2].position = (Position){-5,-5};
    size_t covering = 0;
    for(size_t i=0;i<4;i++) {
        assert(editor_project_soft_area_mesh_create(body,&body->areas[i],&mesh));
        mesh_on_beams_check(body,&mesh);
        covering += area_mesh_contains_check(&mesh,(Position){-2,-8});
        area_mesh_destroy(&mesh);
    }
    assert(covering == 2);
    /* Returning to the original pose restores four disjoint original colors. */
    for(size_t i=0;i<4;i++) body->nodes[i].position=square[i];
    sum = 0;
    for(size_t i=0;i<4;i++) {
        assert(body->areas[i].id==ids[i]);
        assert(body->areas[i].color==0x112233ff+(uint32_t)i*0x20100000);
        assert(editor_project_soft_area_mesh_create(body,&body->areas[i],&mesh));
        sum += mesh_area(&mesh); area_mesh_destroy(&mesh);
    }
    assert(fabs(sum-400)<.001);
    /* Save and load while crossings are absent; no pose-dependent rewriting. */
    body->nodes[2].position=(Position){-30,-40};
    assert(editor_project_save(&project,"stable_soft_areas.json"));
    assert(!editor_result_check(editor_project_load(&loaded,"stable_soft_areas.json")));
    EditorSoftBody *copy=&loaded.objects[0].soft_body_items[0];
    assert(copy->area_count==4);
    for(size_t i=0;i<4;i++) {
        assert(copy->areas[i].id==ids[i]);
        assert(copy->areas[i].color==body->areas[i].color && copy->areas[i].color_overridden);
        assert(editor_project_soft_area_mesh_create(copy,&copy->areas[i],&mesh));
        mesh_on_beams_check(copy,&mesh);
        assert(isfinite(mesh_area(&mesh))); area_mesh_destroy(&mesh);
    }
    /* Runtime resolves the same beam boundaries and independent stable handle. */
    assert(!rohr_error_check(rohr_engine_start()));
    /* Deleting the moving body also exercises remapping into recycled IDs. */
    simulated_motion_ownership_check();
    EntityResult created=rohr_physics_soft_body_create(); assert(!rohr_error_check(created));
    Entity runtime=created.result.value,nodes[4],beams[6],areas[4];
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
        areas[i]=created.result.value;
    }
    for(size_t i=0;i<4;i++) {
        AreaMesh editor={0};
        assert(editor_project_soft_area_mesh_create(body,&body->areas[i],&editor));
        assert(rohr_soft_body_area_mesh_create(areas[i],&mesh));
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
    for(size_t i=0;i<4;i++)
        assert(!rohr_error_check(rohr_physics_soft_body_area_get(areas[i])));
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
