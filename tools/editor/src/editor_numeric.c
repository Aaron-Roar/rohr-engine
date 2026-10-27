/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only */
#include "editor_numeric.h"
#include "editor_command.h"
#include "editor_array.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define N(K,T,M,G,V) {#M,G,EDITOR_ITEM_##K,EDITOR_NUMERIC_##V,offsetof(T,M)}
#define F(K,T,M,G) N(K,T,M,G,FLOAT)
#define XY(K,T,M,G) F(K,T,M.x,G), F(K,T,M.y,G)
#define A(K,T,M) {#M ".alpha","appearance",EDITOR_ITEM_##K,EDITOR_NUMERIC_ALPHA,offsetof(T,M)}
#define MOTION(K,T) XY(K,T,initial_velocity,"motion"), XY(K,T,initial_acceleration,"motion")
#define LAYER(K,T) N(K,T,graphics_layer.value,"appearance",INT)
#define SETTINGS(K,T) \
    N(K,T,entity_settings.lifetime.deadline.expiry_time,"lifetime",DOUBLE), \
    N(K,T,entity_settings.lifetime.deadline.expiry_tick,"lifetime",UINT64), \
    F(K,T,entity_settings.angular_acceleration,"angular"), \
    F(K,T,entity_settings.angular_velocity_maximum,"angular"), \
    {"axis-angle","axis",EDITOR_ITEM_##K,EDITOR_NUMERIC_AXIS_ANGLE,offsetof(T,entity_settings.axis_lock.axis)}, \
    XY(K,T,entity_settings.axis_lock.point_on_axis,"axis"), \
    XY(K,T,entity_settings.transform_offset,"driver"), F(K,T,entity_settings.transform_angle,"driver")
#define VISUAL(K,T,P) \
    A(K,T,visuals.P.fill_color), A(K,T,visuals.P.border_color), \
    F(K,T,visuals.P.border_thickness,"appearance"), XY(K,T,visuals.P.scale,"appearance"), \
    XY(K,T,visuals.P.offset,"appearance"), F(K,T,visuals.P.rotation,"appearance")
#define TEXT(K,T,P) XY(K,T,P.offset,"text"), F(K,T,P.box_width,"text"), F(K,T,P.box_height,"text"), \
    F(K,T,P.width_scale,"text"), F(K,T,P.height_scale,"text"), N(K,T,P.wrap_width,"text",INT), F(K,T,P.rotation,"text"), A(K,T,P.color)
#define SLIDER(K,T,P) F(K,T,P.minimum,"slider"),F(K,T,P.maximum,"slider"),F(K,T,P.value,"slider"), \
    F(K,T,P.step,"slider"),F(K,T,P.length,"slider"),F(K,T,P.track_thickness,"slider"), \
    F(K,T,P.thumb_width,"thumb"),F(K,T,P.thumb_height,"thumb"),F(K,T,P.thumb_radius,"thumb"),F(K,T,P.thumb_offset,"thumb"), \
    A(K,T,P.track_color),A(K,T,P.filled_track_color),A(K,T,P.thumb_color),A(K,T,P.hover_thumb_color),A(K,T,P.pressed_thumb_color)
#define UI(K,T) F(K,T,border_thickness,"appearance"),F(K,T,border_hash_spacing,"appearance"), \
    F(K,T,border_corner_radius,"appearance"),A(K,T,border_color),A(K,T,fill_color),A(K,T,hover_border_color), \
    A(K,T,hover_fill_color),A(K,T,click_border_color),A(K,T,click_fill_color), \
    A(K,T,value.shape.outline_color),A(K,T,value.shape.fill_color),TEXT(K,T,value.text),TEXT(K,T,value.shape.text), \
    F(K,T,value.shape.rotation,"transform"),SLIDER(K,T,value.slider)
static const EditorNumericProperty properties[] = {
    XY(OBJECT,EditorObject,position,"position"), XY(OBJECT,EditorObject,overview_position,"overview"), SETTINGS(OBJECT,EditorObject),
    XY(RIGID_BODY,EditorRigidBody,position,"position"),F(RIGID_BODY,EditorRigidBody,rotation,"rotation"),
    MOTION(RIGID_BODY,EditorRigidBody),F(RIGID_BODY,EditorRigidBody,initial_angular_velocity,"motion"),
    F(RIGID_BODY,EditorRigidBody,mass_value,"physics"),F(RIGID_BODY,EditorRigidBody,friction,"physics"),F(RIGID_BODY,EditorRigidBody,restitution,"physics"),
    XY(RIGID_BODY,EditorRigidBody,center_of_mass_offset,"center_of_mass"),F(RIGID_BODY,EditorRigidBody,particle_radius,"geometry"),
    N(RIGID_BODY,EditorRigidBody,particle_rigid_vertices,"geometry",UINT),XY(RIGID_BODY,EditorRigidBody,particle_origin,"origin"),
    LAYER(RIGID_BODY,EditorRigidBody),SETTINGS(RIGID_BODY,EditorRigidBody),
    A(RIGID_BODY,EditorRigidBody,border_color),A(RIGID_BODY,EditorRigidBody,surface_color),
    A(RIGID_BODY,EditorRigidBody,particle_ring_color),A(RIGID_BODY,EditorRigidBody,particle_fill_color),
    XY(VERTEX,EditorVertex,position,"position"),
    {"length","geometry",EDITOR_ITEM_LINE,EDITOR_NUMERIC_LENGTH,0},
    {"mass","physics",EDITOR_ITEM_SOFT_BODY,EDITOR_NUMERIC_MASS,0},
    {"center_of_mass.x","center_of_mass",EDITOR_ITEM_SOFT_BODY,EDITOR_NUMERIC_COM_X,0},
    {"center_of_mass.y","center_of_mass",EDITOR_ITEM_SOFT_BODY,EDITOR_NUMERIC_COM_Y,0},
    F(UI_VERTEX,Position,x,"position"),F(UI_VERTEX,Position,y,"position"),
    {"length","geometry",EDITOR_ITEM_UI_LINE,EDITOR_NUMERIC_UI_LENGTH,0},
    XY(ANCHOR,EditorAnchor,position,"position"),F(ANCHOR,EditorAnchor,rotation,"rotation"),LAYER(ANCHOR,EditorAnchor),SETTINGS(ANCHOR,EditorAnchor),
    F(JOINT,EditorJoint,rest_length,"parameters"),F(JOINT,EditorJoint,rest_angle,"parameters"),F(JOINT,EditorJoint,stiffness,"parameters"),
    F(JOINT,EditorJoint,damping,"parameters"),F(JOINT,EditorJoint,visual_size,"configuration"),LAYER(JOINT,EditorJoint),SETTINGS(JOINT,EditorJoint),
    VISUAL(JOINT,EditorJoint,main),VISUAL(JOINT,EditorJoint,endpoint_a),VISUAL(JOINT,EditorJoint,endpoint_b),
    XY(SOFT_BODY,EditorSoftBody,position,"position"),F(SOFT_BODY,EditorSoftBody,rotation,"rotation"),MOTION(SOFT_BODY,EditorSoftBody),
    F(SOFT_BODY,EditorSoftBody,initial_angular_velocity,"motion"),F(SOFT_BODY,EditorSoftBody,node_friction_default,"physics"),
    F(SOFT_BODY,EditorSoftBody,node_restitution_default,"physics"),LAYER(SOFT_BODY,EditorSoftBody),SETTINGS(SOFT_BODY,EditorSoftBody),
    XY(SOFT_NODE,EditorSoftNode,position,"position"),MOTION(SOFT_NODE,EditorSoftNode),F(SOFT_NODE,EditorSoftNode,node_mass,"physics"),
    F(SOFT_NODE,EditorSoftNode,radius,"geometry"),F(SOFT_NODE,EditorSoftNode,friction,"physics"),F(SOFT_NODE,EditorSoftNode,restitution,"physics"),
    LAYER(SOFT_NODE,EditorSoftNode),SETTINGS(SOFT_NODE,EditorSoftNode),
    A(SOFT_NODE,EditorSoftNode,color),A(SOFT_BODY,EditorSoftBody,node_color),A(SOFT_BODY,EditorSoftBody,beam_color),
    F(SOFT_BEAM,EditorSoftBeam,friction,"physics"),F(SOFT_BEAM,EditorSoftBeam,restitution,"physics"),
    F(SOFT_BEAM,EditorSoftBeam,stiffness,"physics"),F(SOFT_BEAM,EditorSoftBeam,damping,"physics"),F(SOFT_BEAM,EditorSoftBeam,rest_length,"physics"),
    F(SOFT_BEAM,EditorSoftBeam,collision_thickness,"physics"),LAYER(SOFT_BEAM,EditorSoftBeam),SETTINGS(SOFT_BEAM,EditorSoftBeam),
    LAYER(SOFT_AREA,EditorSoftArea),SETTINGS(SOFT_AREA,EditorSoftArea),
    A(SOFT_AREA,EditorSoftArea,color),A(SOFT_BEAM,EditorSoftBeam,color),
    XY(SPRITE,EditorSprite,position,"position"),F(SPRITE,EditorSprite,rotation,"rotation"),XY(SPRITE,EditorSprite,size,"geometry"),LAYER(SPRITE,EditorSprite),SETTINGS(SPRITE,EditorSprite),
    XY(ANIMATED_SPRITE,EditorAnimatedSprite,editor_position,"position"),F(ANIMATED_SPRITE,EditorAnimatedSprite,editor_rotation,"rotation"),
    XY(ANIMATED_SPRITE,EditorAnimatedSprite,scale,"geometry"),N(ANIMATED_SPRITE,EditorAnimatedSprite,ticks_per_frame,"playback",UINT64),
    N(ANIMATED_SPRITE,EditorAnimatedSprite,time_per_frame,"playback",DOUBLE),N(ANIMATED_SPRITE,EditorAnimatedSprite,starting_frame,"playback",UINT),
    LAYER(ANIMATED_SPRITE,EditorAnimatedSprite),SETTINGS(ANIMATED_SPRITE,EditorAnimatedSprite),
    XY(ANIMATION_FRAME,EditorAnimationFrame,scale,"geometry"),XY(ANIMATION_FRAME,EditorAnimationFrame,offset,"position"),F(ANIMATION_FRAME,EditorAnimationFrame,rotation,"rotation"),
    XY(CAMERA,EditorCamera,position,"position"),F(CAMERA,EditorCamera,rotation,"rotation"),XY(CAMERA,EditorCamera,dimensions,"geometry"),
    F(CAMERA,EditorCamera,zoom,"view"),LAYER(CAMERA,EditorCamera),SETTINGS(CAMERA,EditorCamera),
    F(FORCE,EditorForce,description.magnitude,"force"),XY(FORCE,EditorForce,description.location.offset,"location"),
    F(FORCE,EditorForce,description.location.fraction,"location"),F(FORCE,EditorForce,description.orientation.offset,"orientation"),
    XY(FORCE,EditorForce,description.orientation.from.offset,"from"),F(FORCE,EditorForce,description.orientation.from.fraction,"from"),
    XY(FORCE,EditorForce,description.orientation.to.offset,"to"),F(FORCE,EditorForce,description.orientation.to.fraction,"to"),
    LAYER(FORCE,EditorForce),SETTINGS(FORCE,EditorForce),VISUAL(FORCE,EditorForce,main),
    F(TORQUE,EditorTorque,description.magnitude,"torque"),XY(TORQUE,EditorTorque,marker_offset,"position"),
    LAYER(TORQUE,EditorTorque),SETTINGS(TORQUE,EditorTorque),VISUAL(TORQUE,EditorTorque,main),
    N(PROJECT,EditorProject,engine_time_per_tick,"physics",DOUBLE),N(PROJECT,EditorProject,physics_dt_per_tick,"physics",DOUBLE),
    N(PROJECT,EditorProject,physics_substeps,"physics",UINT),XY(PROJECT,EditorProject,physics_gravity,"physics"),N(PROJECT,EditorProject,physics_solver_iterations,"physics",UINT),
    N(PROJECT,EditorProject,application_window.window_width,"window",INT),N(PROJECT,EditorProject,application_window.window_height,"window",INT),
    N(PROJECT,EditorProject,application_window.logical_width,"window",INT),N(PROJECT,EditorProject,application_window.logical_height,"window",INT),
    F(AUDIO,EditorAudioDefinition,volume,"audio"),F(AUDIO,EditorAudioDefinition,pan,"audio"),F(AUDIO,EditorAudioDefinition,playback_rate,"audio"),
    N(GRAPHICS_LAYER,EditorGraphicsLayer,value,"appearance",INT),F(UI_FONT,EditorUiFont,point_size,"font"),
    F(LAYOUT_VIEWPORT,EditorLayoutViewport,config.rectangle.x,"position"),F(LAYOUT_VIEWPORT,EditorLayoutViewport,config.rectangle.y,"position"),
    F(LAYOUT_VIEWPORT,EditorLayoutViewport,config.rectangle.width,"geometry"),F(LAYOUT_VIEWPORT,EditorLayoutViewport,config.rectangle.height,"geometry"),
    XY(LAYOUT_VIEWPORT,EditorLayoutViewport,overview_position,"overview"),
    A(LAYOUT_VIEWPORT,EditorLayoutViewport,background_color),
    F(VIEWPORT_CAMERA,EditorViewportCameraItem,placement.rectangle.x,"position"),F(VIEWPORT_CAMERA,EditorViewportCameraItem,placement.rectangle.y,"position"),
    F(VIEWPORT_CAMERA,EditorViewportCameraItem,placement.rectangle.width,"geometry"),F(VIEWPORT_CAMERA,EditorViewportCameraItem,placement.rectangle.height,"geometry"),
    F(VIEWPORT_CAMERA,EditorViewportCameraItem,placement.orientation,"rotation"),N(VIEWPORT_CAMERA,EditorViewportCameraItem,placement.layer,"appearance",INT),
    XY(VIEWPORT_CAMERA,EditorViewportCameraItem,content_offset,"content"),XY(VIEWPORT_CAMERA,EditorViewportCameraItem,content_scale,"content"),
    F(VIEWPORT_CAMERA,EditorViewportCameraItem,content_rotation,"content"),N(VIEWPORT_CAMERA,EditorViewportCameraItem,render_width,"render",INT),N(VIEWPORT_CAMERA,EditorViewportCameraItem,render_height,"render",INT),
    XY(VIEWPORT_UI,EditorViewportUiItem,position,"position"),XY(VIEWPORT_UI,EditorViewportUiItem,scale,"geometry"),
    F(VIEWPORT_UI,EditorViewportUiItem,rotation,"rotation"),N(VIEWPORT_UI,EditorViewportUiItem,layer,"appearance",INT),
    UI(UI_DEFINITION,EditorViewportUiDefinition),
    XY(INPUT_BINDING,InputBinding,scale,"scale"),XY(INPUT_BINDING,InputBinding,direction,"direction"),
    N(INPUT_BINDING,InputBinding,input.key,"input",INT),N(INPUT_BINDING,InputBinding,modifiers,"input",UINT16),
};
#undef N
#undef F
#undef XY

bool editor_numeric_target_check(EditorNumericTarget a, EditorNumericTarget b) {
    return a.kind==b.kind && a.object==b.object && a.parent==b.parent && a.container==b.container && a.item==b.item && a.index==b.index;
}
const EditorNumericProperty *editor_numeric_properties_get(size_t *count) {
    if(count) *count=sizeof(properties)/sizeof(*properties);
    return properties;
}
const EditorNumericProperty *editor_numeric_property_get(EditorNumericTarget target, const char *key) {
    if(!key) return NULL;
    for(size_t i=0;i<sizeof(properties)/sizeof(*properties);++i)
        if(properties[i].kind==target.kind && !strcmp(properties[i].key,key)) return &properties[i];
    return NULL;
}
const char *editor_numeric_compatible_key_get(EditorNumericTarget target,const char *key) {
    if(!key || key[0]=='@' || editor_numeric_property_get(target,key)) return key;
    if(!strcmp(key,"mass_value") || !strcmp(key,"node_mass") || !strcmp(key,"mass"))
        return target.kind==EDITOR_ITEM_SOFT_NODE?"node_mass":target.kind==EDITOR_ITEM_SOFT_BODY?"mass":"mass_value";
    if(!strcmp(key,"friction") || !strcmp(key,"node_friction_default")) return target.kind==EDITOR_ITEM_SOFT_BODY?"node_friction_default":"friction";
    if(!strcmp(key,"restitution") || !strcmp(key,"node_restitution_default")) return target.kind==EDITOR_ITEM_SOFT_BODY?"node_restitution_default":"restitution";
    if(!strcmp(key,"position.x") || !strcmp(key,"editor_position.x")) return target.kind==EDITOR_ITEM_ANIMATED_SPRITE?"editor_position.x":"position.x";
    if(!strcmp(key,"position.y") || !strcmp(key,"editor_position.y")) return target.kind==EDITOR_ITEM_ANIMATED_SPRITE?"editor_position.y":"position.y";
    if(!strcmp(key,"rotation") || !strcmp(key,"editor_rotation")) return target.kind==EDITOR_ITEM_ANIMATED_SPRITE?"editor_rotation":"rotation";
    return key;
}
void *editor_numeric_target_get(EditorProject *project, EditorNumericTarget t) {
    if(!project) return NULL;
    if(t.kind==EDITOR_ITEM_PROJECT) return project;
    if(t.kind==EDITOR_ITEM_UI_VERTEX || t.kind==EDITOR_ITEM_UI_LINE) {
        for(size_t d=0;d<project->ui_definition_count;++d) {
            EditorViewportUiDefinition *definition=&project->ui_definitions[d];
            if(definition->id==t.parent && definition->kind==EDITOR_VIEWPORT_UI_SHAPE && t.item && t.item<=definition->value.shape.vertex_count)
                return t.kind==EDITOR_ITEM_UI_VERTEX?(void *)&definition->value.shape.vertices[t.item-1]:(void *)&definition->value.shape;
        }
        return NULL;
    }
    if(t.kind==EDITOR_ITEM_INPUT_BINDING) {
        for(size_t c=0;c<project->input_controller_count;++c) {
            EditorInputController *controller=&project->input_controllers[c];
            if(controller->id!=t.object) continue;
            for(size_t a=0;a<controller->action_count;++a) if(controller->actions[a].id==t.parent) {
                EditorInputAction *action=&controller->actions[a];
                for(size_t b=0;b<action->binding_count;++b) if(action->binding_ids[b]==t.item) return &action->bindings[b];
            }
        }
        return NULL;
    }
#define FIND(A,C,ID) for(size_t i=0;i<(C);++i) if((A)[i].id==(ID)) return &(A)[i]
    switch(t.kind) {
        case EDITOR_ITEM_AUDIO: FIND(project->audio_definitions,project->audio_definition_count,t.item);return NULL;
        case EDITOR_ITEM_GRAPHICS_LAYER: FIND(project->graphics_layers,project->graphics_layer_count,t.item);return NULL;
        case EDITOR_ITEM_UI_FONT: FIND(project->ui_fonts,project->ui_font_count,t.item);return NULL;
        case EDITOR_ITEM_UI_DEFINITION: FIND(project->ui_definitions,project->ui_definition_count,t.item);return NULL;
        default:break;
    }
    if(t.kind==EDITOR_ITEM_LAYOUT_VIEWPORT || t.kind==EDITOR_ITEM_VIEWPORT_CAMERA || t.kind==EDITOR_ITEM_VIEWPORT_UI) {
        for(size_t v=0;v<project->layout_viewport_count;++v) {
            EditorLayoutViewport *viewport=&project->layout_viewports[v];
            if(viewport->id!=(t.kind==EDITOR_ITEM_LAYOUT_VIEWPORT?t.item:t.parent)) continue;
            if(t.kind==EDITOR_ITEM_LAYOUT_VIEWPORT) return viewport;
            if(t.kind==EDITOR_ITEM_VIEWPORT_CAMERA) {FIND(viewport->camera_items,viewport->camera_item_count,t.item);}
            if(t.kind==EDITOR_ITEM_VIEWPORT_UI) {FIND(viewport->ui_items,viewport->ui_item_count,t.item);}
        }
        return NULL;
    }
    EditorObject *o=NULL;
    for(size_t i=0;i<project->object_count;++i) if(project->objects[i].id==t.object) o=&project->objects[i];
    if(!o) return NULL;
    switch(t.kind) {
        case EDITOR_ITEM_OBJECT:return o;
        case EDITOR_ITEM_RIGID_BODY:FIND(o->rigid_bodies,o->rigid_body_count,t.item);break;
        case EDITOR_ITEM_ANCHOR:FIND(o->anchors,o->anchor_count,t.item);break;
        case EDITOR_ITEM_JOINT:FIND(o->joint_items,o->joint_count,t.item);break;
        case EDITOR_ITEM_SOFT_BODY:FIND(o->soft_body_items,o->soft_body_count,t.item);break;
        case EDITOR_ITEM_CAMERA:FIND(o->cameras,o->camera_count,t.item);break;
        case EDITOR_ITEM_SPRITE:FIND(o->sprites,o->sprite_count,t.item);break;
        case EDITOR_ITEM_ANIMATED_SPRITE:FIND(o->animated_sprite_items,o->animated_sprite_count,t.item);break;
        case EDITOR_ITEM_FORCE:FIND(o->force_items,o->force_count,t.item);break;
        case EDITOR_ITEM_TORQUE:FIND(o->torque_items,o->torque_count,t.item);break;
        case EDITOR_ITEM_ANIMATION_FRAME:
            for(size_t a=0;a<o->animated_sprite_count;++a) if(o->animated_sprite_items[a].id==t.parent) {
                FIND(o->animated_sprite_items[a].frames,o->animated_sprite_items[a].frame_count,t.item);
            } break;
        case EDITOR_ITEM_VERTEX:case EDITOR_ITEM_LINE:
            for(size_t b=0;b<o->rigid_body_count;++b) if(o->rigid_bodies[b].id==t.parent)
                for(size_t h=0;h<o->rigid_bodies[b].hitbox_count;++h) {
                    EditorHitbox *hitbox=&o->rigid_bodies[b].hitboxes[h];
                    if(hitbox->id==t.container) {
                        if(t.kind==EDITOR_ITEM_LINE) {for(size_t v=0;v<hitbox->vertex_count;++v) if(hitbox->vertices[v].id==t.item) return hitbox;}
                        else {FIND(hitbox->vertices,hitbox->vertex_count,t.item);}
                    }
                } break;
        case EDITOR_ITEM_SOFT_NODE:case EDITOR_ITEM_SOFT_BEAM:case EDITOR_ITEM_SOFT_AREA:
            for(size_t b=0;b<o->soft_body_count;++b) if(o->soft_body_items[b].id==t.parent) {
                EditorSoftBody *body=&o->soft_body_items[b];
                if(t.kind==EDITOR_ITEM_SOFT_NODE) {FIND(body->nodes,body->node_count,t.item);}
                if(t.kind==EDITOR_ITEM_SOFT_BEAM) {FIND(body->beams,body->beam_count,t.item);}
                if(t.kind==EDITOR_ITEM_SOFT_AREA) {FIND(body->areas,body->area_count,t.item);}
            } break;
        default:break;
    }
#undef FIND
    return NULL;
}
bool editor_numeric_value_get(EditorProject *project, EditorNumericTarget target, const char *key, EditorNumericValue *value) {
    const EditorNumericProperty *p=editor_numeric_property_get(target,key);
    unsigned char *base=editor_numeric_target_get(project,target);
    if(!p || !base || !value) return false;
    if(target.kind==EDITOR_ITEM_UI_DEFINITION) {
        EditorViewportUiDefinition *definition=(void *)base;
        if((strstr(key,"value.text.") && definition->kind!=EDITOR_VIEWPORT_UI_TEXT) ||
            (strstr(key,"value.shape.") && definition->kind!=EDITOR_VIEWPORT_UI_SHAPE) ||
            (strstr(key,"value.slider.") && definition->kind!=EDITOR_VIEWPORT_UI_SLIDER)) return false;
    }
    *value=(EditorNumericValue){0};base+=p->offset;
    switch(p->type) {
        case EDITOR_NUMERIC_ALPHA: {uint32_t color;memcpy(&color,base,sizeof(color));value->number=(float)(color&255)*100/255;break;}
        case EDITOR_NUMERIC_FLOAT:memcpy(&value->number,base,sizeof(float));break;
        case EDITOR_NUMERIC_DOUBLE:memcpy(&value->real,base,sizeof(double));break;
        case EDITOR_NUMERIC_UINT:memcpy(&value->integer,base,sizeof(uint32_t));break;
        case EDITOR_NUMERIC_UINT16: {uint16_t v;memcpy(&v,base,sizeof(v));value->integer=v;break;}
        case EDITOR_NUMERIC_INT:memcpy(&value->signed_integer,base,sizeof(int));break;
        case EDITOR_NUMERIC_UINT64:memcpy(&value->wide,base,sizeof(uint64_t));break;
        case EDITOR_NUMERIC_AXIS_ANGLE: {
            Axis axis;memcpy(&axis,base,sizeof(axis));value->real=atan2(axis.x,axis.y);break;
        }
        case EDITOR_NUMERIC_LENGTH: {
            EditorHitbox *hitbox=(void *)base;
            for(size_t i=0;i<hitbox->vertex_count;++i) if(hitbox->vertices[i].id==target.item) {
                Position a=hitbox->vertices[i].position,b=hitbox->vertices[(i+1)%hitbox->vertex_count].position;
                value->real=hypot((double)b.x-a.x,(double)b.y-a.y);break;
            } break;
        }
        case EDITOR_NUMERIC_UI_LENGTH: {
            EditorViewportUiShape *shape=(void *)base;
            Position a=shape->vertices[target.item-1],b=shape->vertices[target.item%shape->vertex_count];
            value->real=hypot((double)b.x-a.x,(double)b.y-a.y);break;
        }
        case EDITOR_NUMERIC_MASS:case EDITOR_NUMERIC_COM_X:case EDITOR_NUMERIC_COM_Y: {
            EditorSoftBody *body=(void *)base;double mass=0,x=0,y=0;
            for(size_t i=0;i<body->node_count;++i) {mass+=body->nodes[i].node_mass;x+=body->nodes[i].position.x*body->nodes[i].node_mass;y+=body->nodes[i].position.y*body->nodes[i].node_mass;}
            value->real=p->type==EDITOR_NUMERIC_MASS?mass:mass>0?(p->type==EDITOR_NUMERIC_COM_X?x:y)/mass:0;break;
        }
    }
    return true;
}
bool editor_numeric_value_equal_check(const EditorNumericProperty *p, EditorNumericValue a, EditorNumericValue b) {
    if(!p) return false;
    switch(p->type) {
        case EDITOR_NUMERIC_FLOAT:case EDITOR_NUMERIC_ALPHA:return a.number==b.number;
        case EDITOR_NUMERIC_DOUBLE:case EDITOR_NUMERIC_AXIS_ANGLE:case EDITOR_NUMERIC_LENGTH:case EDITOR_NUMERIC_UI_LENGTH:
        case EDITOR_NUMERIC_MASS:case EDITOR_NUMERIC_COM_X:case EDITOR_NUMERIC_COM_Y:return a.real==b.real;
        case EDITOR_NUMERIC_INT:return a.signed_integer==b.signed_integer;
        case EDITOR_NUMERIC_UINT:case EDITOR_NUMERIC_UINT16:return a.integer==b.integer;
        case EDITOR_NUMERIC_UINT64:return a.wide==b.wide;
    }
    return false;
}
bool editor_numeric_value_restore(EditorProject *project, EditorNumericTarget target, const char *key, EditorNumericValue value) {
    const EditorNumericProperty *p=editor_numeric_property_get(target,key);
    unsigned char *base=editor_numeric_target_get(project,target);
    if(!p || !base) return false;
    base+=p->offset;
    switch(p->type) {
        case EDITOR_NUMERIC_ALPHA: {uint32_t color;memcpy(&color,base,sizeof(color));color=(color&UINT32_C(0xffffff00))|(uint32_t)lroundf(value.number*255/100);memcpy(base,&color,sizeof(color));break;}
        case EDITOR_NUMERIC_FLOAT:memcpy(base,&value.number,sizeof(float));break;
        case EDITOR_NUMERIC_DOUBLE:memcpy(base,&value.real,sizeof(double));break;
        case EDITOR_NUMERIC_UINT:memcpy(base,&value.integer,sizeof(uint32_t));break;
        case EDITOR_NUMERIC_UINT16: {uint16_t v=(uint16_t)value.integer;memcpy(base,&v,sizeof(v));break;}
        case EDITOR_NUMERIC_INT:memcpy(base,&value.signed_integer,sizeof(int));break;
        case EDITOR_NUMERIC_UINT64:memcpy(base,&value.wide,sizeof(uint64_t));break;
        case EDITOR_NUMERIC_AXIS_ANGLE: {
            Axis axis={(float)sin(value.real),(float)cos(value.real)};memcpy(base,&axis,sizeof(axis));break;
        }
        case EDITOR_NUMERIC_LENGTH:case EDITOR_NUMERIC_UI_LENGTH:case EDITOR_NUMERIC_MASS:case EDITOR_NUMERIC_COM_X:case EDITOR_NUMERIC_COM_Y:return false;
    }
    return true;
}
bool editor_numeric_lock_check(const EditorProject *project, EditorNumericTarget target, const char *key) {
    if(!project || !key) return false;
    for(size_t i=0;i<project->numeric_lock_count;++i)
        if(editor_numeric_target_check(project->numeric_locks[i].target,target) && !strcmp(project->numeric_locks[i].property,key)) return true;
    return false;
}
bool editor_numeric_lock_set(EditorProject *project, EditorNumericTarget target, const char *key, bool locked) {
    EditorNumericValue value;
    if(target.index) return false;
    switch(target.kind) {
        case EDITOR_ITEM_PROJECT:
            if(target.object || target.parent || target.container || target.item) return false;break;
        case EDITOR_ITEM_OBJECT:
            if(!target.object || target.item!=target.object || target.parent || target.container) return false;break;
        case EDITOR_ITEM_UI_VERTEX:case EDITOR_ITEM_UI_LINE:case EDITOR_ITEM_VIEWPORT_UI:case EDITOR_ITEM_VIEWPORT_CAMERA:
            if(target.object || !target.parent || target.container || !target.item) return false;break;
        case EDITOR_ITEM_LAYOUT_VIEWPORT:case EDITOR_ITEM_UI_DEFINITION:case EDITOR_ITEM_UI_FONT:
        case EDITOR_ITEM_GRAPHICS_LAYER:case EDITOR_ITEM_AUDIO:
            if(target.object || target.parent || target.container || !target.item) return false;break;
        case EDITOR_ITEM_VERTEX:case EDITOR_ITEM_LINE:
            if(!target.object || !target.parent || !target.container || !target.item) return false;break;
        case EDITOR_ITEM_SOFT_NODE:case EDITOR_ITEM_SOFT_BEAM:case EDITOR_ITEM_SOFT_AREA:
        case EDITOR_ITEM_ANIMATION_FRAME:case EDITOR_ITEM_INPUT_BINDING:
            if(!target.object || !target.parent || target.container || !target.item) return false;break;
        default:
            if(!target.object || target.parent || target.container || !target.item) return false;break;
    }
    if(!editor_numeric_value_get(project,target,key,&value)) return false;
    for(size_t i=0;i<project->numeric_lock_count;++i) {
        EditorNumericLock *lock=&project->numeric_locks[i];
        if(!editor_numeric_target_check(lock->target,target) || strcmp(lock->property,key)) continue;
        if(!locked) {
            memmove(lock,lock+1,(project->numeric_lock_count-i-1)*sizeof(*lock));
            --project->numeric_lock_count;
        }
        return true;
    }
    if(!locked) return true;
    if(!EDITOR_ARRAY_RESERVE(project->numeric_locks,project->numeric_lock_capacity,project->numeric_lock_count+1)) return false;
    EditorNumericLock *lock=&project->numeric_locks[project->numeric_lock_count++];
    *lock=(EditorNumericLock){.target=target};snprintf(lock->property,sizeof(lock->property),"%s",key);
    return true;
}
bool editor_numeric_locks_copy(EditorProject *destination, const EditorProject *source) {
    if(!destination || !source) return false;
    if(!EDITOR_ARRAY_RESERVE(destination->numeric_locks,destination->numeric_lock_capacity,source->numeric_lock_count)) return false;
    if(source->numeric_lock_count) memcpy(destination->numeric_locks,source->numeric_locks,source->numeric_lock_count*sizeof(*source->numeric_locks));
    destination->numeric_lock_count=source->numeric_lock_count;return true;
}
void editor_numeric_locks_prune(EditorProject *project) {
    if(!project) return;
    size_t count=0;
    for(size_t i=0;i<project->numeric_lock_count;++i) {
        EditorNumericValue value;
        if(editor_numeric_value_get(project,project->numeric_locks[i].target,project->numeric_locks[i].property,&value))
            project->numeric_locks[count++]=project->numeric_locks[i];
    }
    project->numeric_lock_count=count;
}
void editor_numeric_ui_vertices_remap(EditorProject *project,uint32_t definition,size_t old_count,const bool *removed,size_t insertion) {
    if(!project || !old_count) return;
    size_t count=0;
    for(size_t i=0;i<project->numeric_lock_count;++i) {
        EditorNumericLock lock=project->numeric_locks[i];
        if(lock.target.parent==definition && (lock.target.kind==EDITOR_ITEM_UI_VERTEX || lock.target.kind==EDITOR_ITEM_UI_LINE)) {
            size_t index=lock.target.item-1;
            if(index>=old_count || (removed && (removed[index] || (lock.target.kind==EDITOR_ITEM_UI_LINE && removed[(index+1)%old_count])))) continue;
            if(!removed && lock.target.kind==EDITOR_ITEM_UI_LINE && (index+1)%old_count==insertion%old_count) continue;
            size_t shift=0;
            if(removed) {for(size_t j=0;j<index;++j) shift+=removed[j];lock.target.item-=(uint32_t)shift;}
            else if(index>=insertion) ++lock.target.item;
        }
        project->numeric_locks[count++]=lock;
    }
    project->numeric_lock_count=count;
    ++project->numeric_history_revision;
}
static const char *kind_names[]={"object","body","hitbox","joint","anchor","soft-body","node","beam","vertex","line","camera","viewport","screen","input-controller","input-action","input-binding","sprite","animation","frame","area","hole","force","torque","ui","font","ui-definition","project","audio","layer","ui-vertex","ui-line"};
const char *editor_numeric_kind_name_get(uint32_t kind) {return kind<sizeof(kind_names)/sizeof(*kind_names)?kind_names[kind]:NULL;}
bool editor_numeric_kind_parse(const char *name,uint32_t *kind) {
    if(!name || !kind) return false;
    for(size_t i=0;i<sizeof(kind_names)/sizeof(*kind_names);++i) if(!strcmp(name,kind_names[i])) {*kind=(uint32_t)i;return true;}
    return false;
}
bool editor_numeric_section_check(const EditorNumericProperty *property,const char *section) {
    if(!property || !section) return false;
    if(!strcmp(section,"all")) return true;
    if(!strncmp(section,"visuals.",8)) return !strncmp(property->key,section,strlen(section));
    if(!strcmp(section,"input-x")) return !strcmp(property->key,"scale.x") || !strcmp(property->key,"direction.x");
    if(!strcmp(section,"input-y")) return !strcmp(property->key,"scale.y") || !strcmp(property->key,"direction.y");
    if(!strcmp(section,"source")) return property->kind==EDITOR_ITEM_VIEWPORT_CAMERA?
        !strcmp(property->section,"render"):!strcmp(property->key,"description.magnitude");
    if(!strcmp(section,"application")) return !strcmp(property->section,"location");
    if(!strcmp(section,"direction")) return !strcmp(property->section,"orientation") || !strcmp(property->section,"from") || !strcmp(property->section,"to");
    if(!strcmp(section,"marker")) return property->kind==EDITOR_ITEM_TORQUE && !strcmp(property->section,"position");
    if(!strcmp(section,"physics") && property->kind==EDITOR_ITEM_RIGID_BODY)
        return !strcmp(property->key,"mass_value") || !strcmp(property->section,"center_of_mass");
    if(!strcmp(section,"physics") && property->kind==EDITOR_ITEM_SOFT_NODE)
        return !strcmp(property->key,"node_mass") || !strcmp(property->key,"radius");
    if(!strcmp(section,"transform") || !strcmp(section,"placement")) return !strcmp(property->section,"position") || !strcmp(property->section,"rotation") || !strcmp(property->section,"transform") ||
        (!strcmp(property->section,"geometry") && (property->kind==EDITOR_ITEM_LAYOUT_VIEWPORT || property->kind==EDITOR_ITEM_VIEWPORT_CAMERA || property->kind==EDITOR_ITEM_VIEWPORT_UI ||
            property->kind==EDITOR_ITEM_CAMERA || property->kind==EDITOR_ITEM_SPRITE || property->kind==EDITOR_ITEM_ANIMATED_SPRITE || property->kind==EDITOR_ITEM_ANIMATION_FRAME));
    if(!strcmp(section,"slider-content")) return !strcmp(property->section,"slider") &&
        (strstr(property->key,"minimum") || strstr(property->key,"maximum") || strstr(property->key,".value") || strstr(property->key,".step"));
    if(!strcmp(section,"slider-appearance")) return !strcmp(property->section,"appearance") || !strcmp(property->section,"thumb") ||
        strstr(property->key,".length") || strstr(property->key,"track_thickness");
    if(!strcmp(section,"text-content")) return !strcmp(property->section,"text") || strstr(property->key,".color.alpha") || property->kind==EDITOR_ITEM_UI_FONT;
    if(!strcmp(section,"material")) return strstr(property->key,"friction") || strstr(property->key,"restitution");
    if(!strcmp(section,"initial_motion")) return !strcmp(property->section,"motion");
    if(!strcmp(section,"timing")) return strstr(property->key,"time_per_tick") || strstr(property->key,"dt_per_tick") || strstr(property->key,"substeps");
    if(!strcmp(section,"world")) return strstr(property->key,"physics_gravity")!=NULL;
    if(!strcmp(section,"solver")) return strstr(property->key,"solver_iterations")!=NULL;
    return !strcmp(property->section,section);
}
bool editor_numeric_guard_begin(EditorNumericGuard *guard,EditorProject *project) {
    if(!guard || !project) return false;
    guard->count=0;guard->revision=project->numeric_history_revision;
    if(!EDITOR_ARRAY_RESERVE(guard->entries,guard->capacity,project->numeric_lock_count)) return false;
    for(size_t i=0;i<project->numeric_lock_count;++i) {
        EditorNumericGuardEntry entry={.lock=project->numeric_locks[i]};
        if(entry.lock.target.kind==EDITOR_ITEM_UI_LINE) {
            EditorViewportUiShape *shape=editor_numeric_target_get(project,entry.lock.target);
            if(shape) {entry.ui_line=true;entry.endpoints[0]=shape->vertices[entry.lock.target.item-1];entry.endpoints[1]=shape->vertices[entry.lock.target.item%shape->vertex_count];}
        }
        if(entry.lock.target.kind==EDITOR_ITEM_UI_DEFINITION &&
                (!strcmp(entry.lock.property,"value.slider.minimum") || !strcmp(entry.lock.property,"value.slider.maximum") ||
                 !strcmp(entry.lock.property,"value.slider.value"))) {
            EditorViewportUiDefinition *definition=editor_numeric_target_get(project,entry.lock.target);
            if(definition && definition->kind==EDITOR_VIEWPORT_UI_SLIDER) {
                entry.slider_range=true;entry.minimum=definition->value.slider.minimum;
                entry.maximum=definition->value.slider.maximum;entry.slider_value=definition->value.slider.value;
            }
        }
        if(editor_numeric_value_get(project,entry.lock.target,entry.lock.property,&entry.value)) guard->entries[guard->count++]=entry;
    }
    return true;
}
size_t editor_numeric_guard_finish(EditorNumericGuard *guard,EditorProject *project,char *reason,size_t capacity) {
    if(reason && capacity) reason[0]='\0';
    if(!guard || !project || guard->revision!=project->numeric_history_revision) return 0;
    size_t blocked=0;
    for(size_t i=0;i<guard->count;++i) {
        EditorNumericGuardEntry *entry=&guard->entries[i];EditorNumericValue now;
        if(!editor_numeric_lock_check(project,entry->lock.target,entry->lock.property) ||
            !editor_numeric_value_get(project,entry->lock.target,entry->lock.property,&now) ||
            editor_numeric_value_equal_check(editor_numeric_property_get(entry->lock.target,entry->lock.property),entry->value,now)) continue;
        (void)editor_numeric_value_restore(project,entry->lock.target,entry->lock.property,entry->value);++blocked;
        if(entry->ui_line) {
            EditorViewportUiShape *shape=editor_numeric_target_get(project,entry->lock.target);
            shape->vertices[entry->lock.target.item-1]=entry->endpoints[0];shape->vertices[entry->lock.target.item%shape->vertex_count]=entry->endpoints[1];
        }
        if(entry->slider_range) {
            EditorViewportUiDefinition *definition=editor_numeric_target_get(project,entry->lock.target);
            definition->value.slider.minimum=entry->minimum;
            definition->value.slider.maximum=entry->maximum;
            definition->value.slider.value=entry->slider_value;
        }
        if(reason && capacity) {
            size_t used=strlen(reason);
            if(used<capacity-1) snprintf(reason+used,capacity-used,"%s%s %u: %s locked",used?"; ":"",
                editor_numeric_kind_name_get(entry->lock.target.kind),entry->lock.target.item,entry->lock.property);
        }
    }
    return blocked;
}
void editor_numeric_guard_destroy(EditorNumericGuard *guard) {
    if(guard) {free(guard->entries);*guard=(EditorNumericGuard){0};}
}
