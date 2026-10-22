/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_workspace.h"
#include "editor_soft_area.h"
#include "entity/object_internal.h"
#include "construction_checks.h"

/* Author independently: never obtain these values from the runtime baseline. */
static void author(EditorProject *p) {
    editor_project_init(p);
    for(int i=1;i<7;++i) {
        char name[32]; snprintf(name,sizeof(name),"mask%d",i);
        assert(editor_project_collision_mask_add(p,name,NULL));
    }
    EditorObject *o=editor_project_object_add(p,(Position){20,30}); assert(o);
    snprintf(o->name,sizeof(o->name),"Construction");
    EditorRigidBody *body=editor_project_rigid_body_add(p,o); assert(body);
    snprintf(body->name,sizeof(body->name),"body");
    EditorRigidBodyId body_id=body->id;
    body->position=(Position){3,4}; body->rotation=-25; body->mass_value=5;
    body->initial_velocity=(Velocity){1,2}; body->initial_acceleration=(Acceleration){.2f,.3f};
    body->initial_angular_velocity=12; body->gravity_enabled=false;
    body->friction=.4f; body->restitution=.2f; body->collision_enabled=true;
    body->collision_category=4; body->responds_to=8;
    body->center_of_mass_explicit=true; body->center_of_mass_offset=(Position){1,-.5f};
    body->entity_settings=(ObjectEntitySettings){.angular_acceleration=2,
        .angular_velocity_maximum_enabled=true,.angular_velocity_maximum=50,
        .lifetime.deadline={.expiry_time=100,.expiry_tick=100000,.time_enabled=true,.tick_enabled=true}};
    const Position vertices[]={{3,2},{3,-2},{-3,-2},{-3,2}};
    assert(body->hitbox_count==1 && body->hitboxes[0].vertex_count==4);
    for(int i=0;i<4;++i) body->hitboxes[0].vertices[i].position=vertices[i];
    EditorRigidBody *particle=editor_project_rigid_body_add(p,o); assert(particle);
    snprintf(particle->name,sizeof(particle->name),"particle");
    particle->position=(Position){50,0}; particle->particle=true; particle->standalone_particle=true;
    particle->particle_auto_fit=false; particle->particle_radius=2; particle->particle_rigid_vertices=12;
    particle->particle_origin=(Position){1,0}; particle->initial_velocity=(Velocity){-1,0};
    particle->mass_value=2; particle->friction=.3f; particle->restitution=.6f;
    particle->gravity_enabled=false; particle->collision_enabled=true;
    particle->collision_category=8; particle->responds_to=4;
    EditorSoftBody *soft=editor_project_soft_body_add(p,o); assert(soft);
    snprintf(soft->name,sizeof(soft->name),"soft");
    soft->position=(Position){80,20}; soft->rotation=30;
    EditorSoftBodyId soft_id=soft->id;
    const Position points[]={{0,0},{20,0},{20,20},{0,20},{5,5},{10,5},{7,10}};
    EditorSoftNodeId nodes[7];
    for(int i=0;i<7;++i) {
        EditorSoftNode *n=editor_project_soft_node_add(p,soft,points[i]); assert(n);
        nodes[i]=n->id; snprintf(n->name,sizeof(n->name),"node%d",i);
        n->node_mass=(float)i+1; n->radius=.5f; n->initial_motion_inherited=false;
        n->initial_velocity=(Velocity){.2f,.1f}; n->static_body=i==0;
        n->friction=.25f; n->restitution=.1f; n->gravity_enabled=false;
        n->collision_enabled=true; n->collision_category=32; n->responds_to=0;
        n->color=0x0a141eff; n->color_overridden=true;
    }
    EditorSoftBeamId beam_id=0;
    for(int i=0;i<4;++i) {
        EditorSoftBeam *b=editor_project_soft_beam_add(p,soft,nodes[i],nodes[(i+1)%4]); assert(b);
        if(i==0) beam_id=b->id;
        snprintf(b->name,sizeof(b->name),"beam%d",i);
        b->stiffness=40; b->damping=2; b->rest_length=20; b->rest_length_explicit=i==0;
        b->friction=.2f; b->restitution=.3f; b->collision_enabled=i==1;
        b->collision_thickness=.2f; b->collision_category=64; b->responds_to=0;
        b->color=0x28323cff; b->color_overridden=true;
    }
    EditorSoftArea *area=editor_soft_area_add(soft); assert(area);
    snprintf(area->name,sizeof(area->name),"fill"); area->color=0x46505aff;
    area->outer.node_count=4; memcpy(area->outer.nodes,nodes,4*sizeof(nodes[0]));
    area->hole_count=1; area->holes[0].id=1; area->next_hole_id=2;
    snprintf(area->holes[0].name,sizeof(area->holes[0].name),"hole");
    area->holes[0].loop.node_count=3; memcpy(area->holes[0].loop.nodes,nodes+4,3*sizeof(nodes[0]));
    EditorSoftBody *other=editor_project_soft_body_add(p,o); assert(other);
    snprintf(other->name,sizeof(other->name),"other"); other->position=(Position){-50,20};
    EditorAnchor *a=editor_project_anchor_add(p,o,(Position){0},0); assert(a);
    EditorAnchorId aid=a->id;
    a->attachment_kind=EDITOR_ANCHOR_ATTACHMENT_SOFT_NODE;
    a->attachment_soft_body=soft_id; a->attachment_soft_node=nodes[0];
    a->position_follows_body=true; a->rotation_follows_body=true;
    EditorAnchor *b=editor_project_anchor_add(p,o,(Position){80,20},0); assert(b);
    EditorAnchorId bid=b->id; snprintf(b->name,sizeof(b->name),"world");
    const EditorJointKind kinds[]={EDITOR_JOINT_REVOLUTE,EDITOR_JOINT_WELD,EDITOR_JOINT_SPRING};
    for(int i=0;i<3;++i) {
        EditorJoint *j=editor_project_joint_add(p,o,kinds[i]); assert(j);
        snprintf(j->name,sizeof(j->name),"joint%d",i);
        j->anchor_a=aid; j->anchor_b=bid; j->rest_length=2; j->rest_angle=15;
        j->rest_length_explicit=true; j->rest_angle_explicit=true; j->stiffness=30; j->damping=2;
    }
    for(int location=0;location<4;++location) for(int angle=0;angle<4;++angle) {
        EditorForce *f=editor_project_force_add(p,o); assert(f);
        snprintf(f->name,sizeof(f->name),"force%d",location*4+angle);
        f->description=(ObjectForceDescription){.target={OBJECT_SOURCE_BODY,body_id,0},.magnitude=.25f,
            .location={.kind=location,.offset={1,2}},.orientation={.kind=angle,.offset=13}};
        if(location==ROHR_FORCE_COM_POSITION || location==ROHR_FORCE_ORIGIN_POSITION)
            f->description.location.reference=(ObjectSourceReference){OBJECT_SOURCE_BODY,body_id,0};
        f->graphics_layer.value = 100 + location*4 + angle;
        if(location==ROHR_FORCE_BEAM_POSITION) {
            f->description.location.reference=(ObjectSourceReference){OBJECT_SOURCE_BEAM,beam_id,soft_id};
            f->description.location.anchor=ROHR_FORCE_BEAM_CENTER; f->description.location.fraction=.3f;
        }
        if(angle==ROHR_FORCE_BODY_ORIENTATION)
            f->description.orientation.reference=(ObjectSourceReference){OBJECT_SOURCE_BODY,body_id,0};
        if(angle==ROHR_FORCE_BEAM_ORIENTATION)
            f->description.orientation.reference=(ObjectSourceReference){OBJECT_SOURCE_BEAM,beam_id,soft_id};
        if(angle==ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION) {
            f->description.orientation.from=f->description.location;
            f->description.orientation.to=(ObjectForceLocation){.offset={80,90}};
        }
    }
    EditorTorque *t=editor_project_torque_add(p,o); assert(t);
    snprintf(t->name,sizeof(t->name),"spin");
    t->graphics_layer.value = -55;
    t->description=(ObjectTorqueDescription){.target={OBJECT_SOURCE_BODY,body_id,0},.magnitude=-.5f};
    t=editor_project_torque_add(p,o); assert(t); snprintf(t->name,sizeof(t->name),"initial");
    t->description=(ObjectTorqueDescription){.mode=OBJECT_SOURCE_INITIAL};
}

int main(int argc,char **argv) {
    static EditorProject project; author(&project);
    const char *path="construction_parity.json";
    assert(editor_project_save(&project,path));
    ConstructionSnapshot baseline,motion,actual;
    REQUIRE(rohr_engine_start());
    ConstructionScene direct=construction_direct((Position){0});
    construction_capture(&direct,&baseline,false); construction_simulate(direct,&motion);
    construction_lifecycle(direct); rohr_engine_stop();
    for(int source=0;source<2;++source) {
        REQUIRE(rohr_engine_start());
        ObjectDescriptionResult catalog=source?rohr_object_load(path):object_description_project_read(&project,path);
        if(rohr_error_check(catalog)) fprintf(stderr,"catalog %d: %s\n",source,rohr_error_message_get(catalog));
        REQUIRE(catalog);
        ObjectDescriptionResult found=rohr_object_description_by_name_get(catalog.result.value,"Construction"); REQUIRE(found);
        ConstructionScene loaded=construction_members(construction_take(rohr_entity_object_add(found.result.value)));
        construction_capture(&loaded,&actual,false); construction_compare(&baseline,&actual,"authored configuration",false);
        construction_simulate(loaded,&actual); construction_compare(&motion,&actual,"authored simulation",true);
        construction_lifecycle(loaded);
        if(source) REQUIRE(rohr_object_destroy(catalog.result.value));
        else object_description_tree_destroy((ObjectDescription *)catalog.result.value);
        rohr_engine_stop();
    }
    if(argc==2) {
        EditorWorkspace workspace={0}; static EditorProject starter;
        REQUIRE(editor_workspace_create(&workspace,&starter,argv[1])); editor_project_destroy(&starter);
        assert(editor_workspace_save(&workspace,&project)); assert(editor_workspace_c_generate(&workspace,&project));
    }
    assert(remove(path)==0); editor_project_destroy(&project); return 0;
}
