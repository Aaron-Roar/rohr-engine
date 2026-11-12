/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "entity/object_visuals.h"
#include "runtime_visuals.h"
#include "graphics/animation_attachments.h"
#include "entity/object_internal.h"
#include "graphics/texture_assets.h"
#include "graphics/camera_state.h"
#include "graphics/layer_state.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

static bool layer_value_read(yyjson_val *value, int *out) {
    if(!yyjson_is_int(value) ||
            (yyjson_is_uint(value) && yyjson_get_uint(value) > INT_MAX) ||
            (yyjson_is_sint(value) && (yyjson_get_sint(value) < INT_MIN ||
                yyjson_get_sint(value) > INT_MAX))) return false;
    *out = (int)yyjson_get_sint(value);
    return true;
}

EngineResult state_layers_load(yyjson_val *root, GraphicsLayerId *created, size_t *created_count) {
    yyjson_val *layers = yyjson_obj_get(root, "graphics_layers"), *entry;
    size_t i, count;
    if(layers == NULL) return error_result_value(true);
    if(!yyjson_is_arr(layers)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
    yyjson_arr_foreach(layers, i, count, entry) {
        yyjson_val *name = yyjson_obj_get(entry, "name");
        int value;
        if(!yyjson_is_obj(entry) || !yyjson_is_str(name) || yyjson_get_len(name) == 0 ||
                yyjson_get_len(name) >= GRAPHICS_LAYER_NAME_MAX ||
                strlen(yyjson_get_str(name)) != yyjson_get_len(name) ||
                !layer_value_read(yyjson_obj_get(entry, "value"), &value))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        GraphicsLayerIdResult layer = graphics_layer_name_id_get(yyjson_get_str(name));
        if(!error_check(layer)) {
            LayerDescriptionResult existing = graphics_layer_description_get(layer.result.value);
            if(error_check(existing) || existing.result.value.value != value)
                return error_result_error(ERROR_ENGINE_DUPLICATE_GRAPHICS_LAYER_NAME);
            continue;
        }
        layer = graphics_layer_create((LayerDescription){.name = yyjson_get_str(name), .value = value});
        if(error_check(layer)) return error_result_error(layer.result.error);
        created[(*created_count)++] = layer.result.value;
    }
    return error_result_value(true);
}

bool state_layers_write(yyjson_mut_doc *doc, yyjson_mut_val *root) {
    LayerDescription layers[MAX_GRAPHICS_LAYERS];
    size_t count = 0;
    for(size_t slot = 0; slot < MAX_GRAPHICS_LAYERS; ++slot) {
        LayerDescription layer;
        if(!graphics_layer_registry_entry_get(slot, &layer)) continue;
        /* Stable output independent of registry slot allocation. */
        size_t at = count++;
        while(at > 0 && strcmp(layers[at - 1].name, layer.name) > 0) {
            layers[at] = layers[at - 1];
            --at;
        }
        layers[at] = layer;
    }
    yyjson_mut_val *array = yyjson_mut_arr(doc);
    if(!array || !yyjson_mut_obj_add_val(doc, root, "graphics_layers", array)) return false;
    for(size_t i = 0; i < count; ++i) {
        yyjson_mut_val *entry = yyjson_mut_obj(doc);
        if(!entry || !yyjson_mut_obj_add_strcpy(doc, entry, "name", layers[i].name) ||
                !yyjson_mut_obj_add_int(doc, entry, "value", layers[i].value) ||
                !yyjson_mut_arr_add_val(array, entry)) return false;
    }
    return true;
}

bool state_layers_template_copy(yyjson_mut_doc *doc, yyjson_mut_val *layers, yyjson_val *root) {
    yyjson_val *entry;
    size_t i, count;
    yyjson_arr_foreach(yyjson_obj_get(root, "graphics_layers"), i, count, entry) {
        const char *name = yyjson_get_str(yyjson_obj_get(entry, "name"));
        bool found = false;
        for(size_t j = 0; j < yyjson_mut_arr_size(layers); ++j) {
            yyjson_mut_val *existing = yyjson_mut_arr_get(layers, j);
            if(strcmp(name, yyjson_mut_get_str(yyjson_mut_obj_get(existing, "name"))) != 0) continue;
            if(yyjson_get_sint(yyjson_obj_get(entry, "value")) !=
                    yyjson_mut_get_sint(yyjson_mut_obj_get(existing, "value"))) return false;
            found = true;
            break;
        }
        if(!found) {
            yyjson_mut_val *copy = yyjson_val_mut_copy(doc, entry);
            if(!copy || !yyjson_mut_arr_add_val(layers, copy)) return false;
        }
    }
    return true;
}

static bool layer_binding_write(yyjson_mut_doc *doc, yyjson_mut_val *components,
        const char *key, GraphicsLayerIdResult id, GraphicsLayerValueResult value) {
    if(!error_check(id)) {
        LayerDescriptionResult layer = graphics_layer_description_get(id.result.value);
        return !error_check(layer) && yyjson_mut_obj_add_strcpy(doc, components, key, layer.result.value.name);
    }
    return error_check(value) || yyjson_mut_obj_add_int(doc, components, key, value.result.value);
}

static bool number(yyjson_val *v, const char *key, double *out) {
    yyjson_val *value = yyjson_obj_get(v,key);
    if(!yyjson_is_num(value) || !isfinite(yyjson_get_num(value))) return false;
    *out=yyjson_get_num(value); return true;
}
static bool scalar(yyjson_val *v,const char *key,float *out) {
    double n;
    if(!number(v,key,&n) || fabs(n)>FLT_MAX) return false;
    *out=(float)n; return true;
}
static bool boolean(yyjson_val *v,const char *key,bool *out) {
    yyjson_val *value=yyjson_obj_get(v,key);
    if(!yyjson_is_bool(value)) return false;
    *out=yyjson_get_bool(value); return true;
}
static bool integer(yyjson_val *v,const char *key,uint64_t *out) {
    yyjson_val *value=yyjson_obj_get(v,key);
    if(!yyjson_is_uint(value)) return false;
    *out=yyjson_get_uint(value); return true;
}
static bool vector_read(yyjson_val *v,const char *key,float *x,float *y) {
    yyjson_val *value=yyjson_obj_get(v,key);
    return scalar(value,"x",x) && scalar(value,"y",y);
}
static bool vector_write(yyjson_mut_doc *doc,yyjson_mut_val *v,const char *key,float x,float y) {
    yyjson_mut_val *value=yyjson_mut_obj(doc);
    return isfinite(x) && isfinite(y) && value && yyjson_mut_obj_add_real(doc,value,"x",x) &&
        yyjson_mut_obj_add_real(doc,value,"y",y) && yyjson_mut_obj_add_val(doc,v,key,value);
}
static bool texture_write(yyjson_mut_doc *doc,yyjson_mut_val *v,TextureAsset texture) {
    const char *path=graphics_texture_path_get(texture);
    char *directory=SDL_GetCurrentDirectory();
    if(directory && path) {
        for(char *p=directory;*p;++p) if(*p=='\\') *p='/';
        size_t length=strlen(directory);
        while(length && directory[length-1]=='/') --length;
        if(!strncmp(path,directory,length) && path[length]=='/') path+=length+1;
    }
    bool ok=path && yyjson_mut_obj_add_strcpy(doc,v,"path",path) &&
        vector_write(doc,v,"size",texture.size.x,texture.size.y);
    SDL_free(directory); return ok;
}
static TextureAssetResult texture_read(yyjson_val *v) {
    yyjson_val *path=yyjson_obj_get(v,"path"); Scale size;
    if(!yyjson_is_str(path) || yyjson_get_len(path)!=strlen(yyjson_get_str(path)) ||
            !vector_read(v,"size",&size.x,&size.y))
        return ERROR_RESULT_MAKE_ERROR(TextureAssetResult,ERROR_ENGINE_STATE_INVALID);
    return graphics_texture_load((TextureDescription){yyjson_get_str(path),size});
}
static bool sprite_write(yyjson_mut_doc *doc,yyjson_mut_val *v,Scale scale,Position offset,
        Orientation angle,Direction direction,bool follow,bool visible) {
    return vector_write(doc,v,"scale",scale.x,scale.y) && vector_write(doc,v,"offset",offset.x,offset.y) &&
        isfinite(angle) && yyjson_mut_obj_add_real(doc,v,"orientation",angle) &&
        yyjson_mut_obj_add_uint(doc,v,"direction",direction) && yyjson_mut_obj_add_bool(doc,v,"follow",follow) &&
        yyjson_mut_obj_add_bool(doc,v,"visible",visible);
}
static bool sprite_read(yyjson_val *v,Scale *scale,Position *offset,Orientation *angle,
        Direction *direction,bool *follow,bool *visible) {
    uint64_t d;
    if(!vector_read(v,"scale",&scale->x,&scale->y) || !vector_read(v,"offset",&offset->x,&offset->y) ||
            !scalar(v,"orientation",angle) || !integer(v,"direction",&d) || d>DIRECTION_RIGHT ||
            !boolean(v,"follow",follow) || !boolean(v,"visible",visible)) return false;
    *direction=(Direction)d; return true;
}
static bool color_write(yyjson_mut_doc *doc, yyjson_mut_val *v, const char *key, Color color) {
    yyjson_mut_val *array = yyjson_mut_arr(doc);
    return array && yyjson_mut_arr_add_uint(doc,array,color.red) && yyjson_mut_arr_add_uint(doc,array,color.green) &&
        yyjson_mut_arr_add_uint(doc,array,color.blue) && yyjson_mut_arr_add_uint(doc,array,color.alpha) && yyjson_mut_obj_add_val(doc,v,key,array);
}
static bool color_read(yyjson_val *v, const char *key, Color *color) {
    yyjson_val *array = yyjson_obj_get(v,key); uint8_t values[4];
    if(!yyjson_is_arr(array) || yyjson_arr_size(array)!=4) return false;
    for(size_t i=0;i<4;++i) {
        yyjson_val *item=yyjson_arr_get(array,i);
        if(!yyjson_is_uint(item) || yyjson_get_uint(item)>255) return false;
        values[i]=(uint8_t)yyjson_get_uint(item);
    }
    *color=(Color){values[0],values[1],values[2],values[3]}; return true;
}
bool state_visuals_write(yyjson_mut_doc *doc,yyjson_mut_val *components,Entity entity) {
    EntityIndex index;
    if(!entity_index_get(entity,&index)) return false;
#define WRITE(condition) do { if(!(condition)) return false; } while(0)
    ObjectInstanceOrigin origin;
    if(object_instance_origin_get(entity, &origin)) {
        yyjson_mut_val *v = yyjson_mut_obj(doc);
        WRITE(v && yyjson_mut_obj_add_uint(doc,v,"id",origin.id) &&
            yyjson_mut_obj_add_uint(doc,v,"catalog",origin.catalog) &&
            yyjson_mut_obj_add_strcpy(doc,v,"catalog_name",origin.catalog_name) &&
            yyjson_mut_obj_add_val(doc,components,"object_origin",v));
    }
    ObjectAppearance appearance;
    if(object_instance_appearance_get(entity,&appearance)) {
        yyjson_mut_val *v=yyjson_mut_obj(doc);
        WRITE(v && yyjson_mut_obj_add_uint(doc,v,"kind",appearance.kind) &&
            yyjson_mut_obj_add_bool(doc,v,"visible",appearance.visible) && yyjson_mut_obj_add_bool(doc,v,"enabled",appearance.enabled) &&
            color_write(doc,v,"color",appearance.color) && color_write(doc,v,"border",appearance.border_color) &&
            yyjson_mut_obj_add_val(doc,v,"visuals",object_visuals_json_write(doc,appearance.visuals)) &&
            yyjson_mut_obj_add_val(doc,components,"object_appearance",v));
    }
    if(sprites_pool.used[index]) {
        Sprite *s=&sprite_components[index]; yyjson_mut_val *v=yyjson_mut_obj(doc);
        WRITE(v && texture_write(doc,v,s->texture) && sprite_write(doc,v,s->scale,s->body_offset,
            s->orientation_offset,s->direction,s->follow_entity_rotation,s->visible) &&
            yyjson_mut_obj_add_val(doc,components,"sprite_state",v));
    }
    if(animated_sprites_pool.used[index]) {
        AnimatedSprite *s=&animated_sprites[index]; yyjson_mut_val *v=yyjson_mut_obj(doc), *frames=yyjson_mut_arr(doc);
        AnimationInfoResult info=graphics_animation_info_get(s->player.animation);
        WRITE(v && frames && s->frame_count<=MAX_ANIMATIONS_FRAMES &&
            sprite_write(doc,v,s->scale,s->body_offset,s->orientation_offset,s->direction,s->follow_entity_rotation,s->visible));
        WRITE(yyjson_mut_obj_add_uint(doc,v,"id",error_check(info) ? 0 : info.result.value.id) &&
            yyjson_mut_obj_add_uint(doc,v,"playback_mode",s->player.playback_mode) &&
            yyjson_mut_obj_add_bool(doc,v,"completed",s->player.completed) &&
            yyjson_mut_obj_add_uint(doc,v,"frame_index",s->player.frame_index) &&
            yyjson_mut_obj_add_uint(doc,v,"ticks_per_frame",s->player.ticks_per_frame) &&
            yyjson_mut_obj_add_real(doc,v,"time_per_frame",s->player.time_per_frame) &&
            yyjson_mut_obj_add_uint(doc,v,"elapsed_ticks",engine_tick_get()-s->player.last_update_tick) &&
            yyjson_mut_obj_add_real(doc,v,"elapsed_time",engine_time_get()-s->player.last_update_time));
        for(size_t i=0;i<s->frame_count;++i) {
            const AnimationFrameDescription *frame=&s->frames[i]; yyjson_mut_val *f=yyjson_mut_obj(doc);
            WRITE(f && texture_write(doc,f,frame->texture) && yyjson_mut_obj_add_uint(doc,f,"id",frame->id) &&
                vector_write(doc,f,"scale",frame->scale.x,frame->scale.y) && vector_write(doc,f,"offset",frame->offset.x,frame->offset.y) &&
                yyjson_mut_obj_add_real(doc,f,"rotation",frame->rotation) && yyjson_mut_arr_add_val(frames,f));
        }
        if(s->attachment_target) {
            EntityNameResult target = entity_name_get(s->attachment_target);
            WRITE(!error_check(target) && target.result.value.value[0] &&
                yyjson_mut_obj_add_strcpy(doc,v,"attachment_target",target.result.value.value));
        }
        WRITE(yyjson_mut_obj_add_val(doc,v,"frames",frames) && yyjson_mut_obj_add_val(doc,components,"animation_state",v));
    }
    CameraIdResult owned=entity_object_camera_get(entity);
    if(!error_check(owned)) {
        CameraStateSnapshot snapshot;
        WRITE(!error_check(graphics_camera_state_get(owned.result.value,&snapshot)));
        yyjson_mut_val *v=yyjson_mut_obj(doc); Camera *c=&snapshot.value;
        WRITE(v && vector_write(doc,v,"position",c->position.x,c->position.y) &&
            vector_write(doc,v,"dimensions",c->dimensions.x,c->dimensions.y) &&
            yyjson_mut_obj_add_real(doc,v,"orientation",c->orientation) && yyjson_mut_obj_add_real(doc,v,"zoom",c->zoom) &&
            yyjson_mut_obj_add_bool(doc,v,"enabled",snapshot.enabled) && yyjson_mut_obj_add_bool(doc,v,"paused",snapshot.pause_with_engine));
        if(snapshot.attached) {
            EntityNameResult target=entity_name_get(snapshot.attachment.entity);
            WRITE(!error_check(target) && yyjson_mut_obj_add_strcpy(doc,v,"target",target.result.value.value) &&
                vector_write(doc,v,"offset",snapshot.attachment.position_offset.x,snapshot.attachment.position_offset.y) &&
                yyjson_mut_obj_add_real(doc,v,"angle_offset",snapshot.attachment.orientation_offset) &&
                yyjson_mut_obj_add_bool(doc,v,"follow_position",snapshot.attachment.follow_position) &&
                yyjson_mut_obj_add_bool(doc,v,"follow_orientation",snapshot.attachment.follow_orientation));
        }
        WRITE(yyjson_mut_obj_add_val(doc,components,"object_camera",v));
    }
    WRITE(layer_binding_write(doc,components,"graphics_layer",
        graphics_layer_entity_id_get(entity),graphics_layer_entity_get(entity)));
    WRITE(layer_binding_write(doc,components,"sprite_layer",
        graphics_layer_sprite_id_get(entity),graphics_layer_sprite_get(entity)));
    WRITE(layer_binding_write(doc,components,"animation_layer",
        graphics_layer_animation_id_get(entity),graphics_layer_animation_get(entity)));
    return true;
#undef WRITE
}
static EngineResult animation_read(Entity entity,yyjson_val *v) {
    AnimatedSprite sprite={0}; AnimationDescription descriptor={0};
    yyjson_val *frames=yyjson_obj_get(v,"frames");
    uint64_t id,index,elapsed; double time_elapsed;
    if(!sprite_read(v,&sprite.scale,&sprite.body_offset,&sprite.orientation_offset,&sprite.direction,&sprite.follow_entity_rotation,&sprite.visible) ||
            !yyjson_is_arr(frames) || yyjson_arr_size(frames)>MAX_ANIMATIONS_FRAMES ||
            !integer(v,"id",&id) || id>UINT32_MAX || !integer(v,"frame_index",&index) ||
            index>SIZE_MAX || (yyjson_arr_size(frames) && index>=yyjson_arr_size(frames)) ||
            !integer(v,"ticks_per_frame",&descriptor.ticks_per_frame) || !number(v,"time_per_frame",&descriptor.time_per_frame) ||
            descriptor.time_per_frame<0 || !integer(v,"elapsed_ticks",&elapsed) || !number(v,"elapsed_time",&time_elapsed))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    yyjson_val *mode = yyjson_obj_get(v, "playback_mode");
    yyjson_val *completed = yyjson_obj_get(v, "completed");
    if((mode && (!yyjson_is_uint(mode) || yyjson_get_uint(mode) > ROHR_ANIMATION_PLAYBACK_ONCE)) ||
            (completed && !yyjson_is_bool(completed)))
        return error_result_error(ERROR_ENGINE_STATE_INVALID);
    AnimationPlaybackMode playback_mode = mode ? (AnimationPlaybackMode)yyjson_get_uint(mode) : ROHR_ANIMATION_PLAYBACK_LOOP;
    bool is_completed = completed && yyjson_get_bool(completed);
    descriptor.id=(AnimationId)id; descriptor.frame_count=(uint8_t)yyjson_arr_size(frames);
    sprite.frame_count=descriptor.frame_count;
    for(size_t i=0;i<sprite.frame_count;++i) {
        yyjson_val *f=yyjson_arr_get(frames,i),*path=yyjson_obj_get(f,"path");
        AnimationFrameDescription *frame=&sprite.frames[i];
        if(!yyjson_is_str(path) || strlen(yyjson_get_str(path))!=yyjson_get_len(path) ||
                !integer(f,"id",&id) || id>UINT32_MAX || !vector_read(f,"size",&frame->texture.size.x,&frame->texture.size.y) ||
                !vector_read(f,"scale",&frame->scale.x,&frame->scale.y) ||
                !vector_read(f,"offset",&frame->offset.x,&frame->offset.y) || !scalar(f,"rotation",&frame->rotation))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        frame->id=(AnimationFrameId)id; descriptor.frame_ids[i]=frame->id; descriptor.frame_files[i]=yyjson_get_str(path);
    }
    AnimationAssetResult loaded=sprite.frame_count ? graphics_animation_load(descriptor) :
        ERROR_RESULT_MAKE_VALUE(AnimationAssetResult, ((AnimationAsset){0}));
    if(error_check(loaded)) return error_result_error(loaded.result.error);
    sprite.player=graphics_animation_player_create(graphics_animation_player_description_from_animation_get(loaded.result.value).result.value).result.value;
    sprite.player.playback_mode=playback_mode;
    sprite.player.ticks_per_frame=descriptor.ticks_per_frame;
    sprite.player.time_per_frame=descriptor.time_per_frame;
    sprite.player.frame_index=(size_t)index;
    sprite.player.last_update_tick=engine_tick_get()-elapsed;
    sprite.player.last_update_time=engine_time_get()-time_elapsed;
    EngineResult result=error_result_value(true);
    for(size_t i=0;i<sprite.frame_count;++i) {
        AnimationFrameDescriptionResult frame=graphics_animation_frame_description_get(loaded.result.value,i);
        if(error_check(frame)) { result=error_result_error(frame.result.error); break; }
        sprite.frames[i].texture.handle=frame.result.value.texture.handle;
    }
    if(!error_check(result)) result=graphics_animated_sprite_add(entity, graphics_animated_sprite_value_description_get(sprite));
    if(!error_check(result)) {
        EntityIndex entity_index;
        if(entity_index_get(entity, &entity_index)) {
            /* Saved playback is restored explicitly, outside full configuration. */
            animated_sprites[entity_index].player.completed = is_completed;
            animated_sprites[entity_index].player.frame_index = sprite.frame_count ? (size_t)index : 0;
            animated_sprites[entity_index].player.last_update_tick = sprite.player.last_update_tick;
            animated_sprites[entity_index].player.last_update_time = sprite.player.last_update_time;
        }
    }
    (void)graphics_animation_release(&loaded.result.value);
    return result;
}
EngineResult state_visuals_load(Entity entity,yyjson_val *components) {
    yyjson_val *v=yyjson_obj_get(components,"object_origin");
    if(v) {
        ObjectInstanceOrigin origin = {0};
        yyjson_val *name = yyjson_obj_get(v,"catalog_name");
        if(!integer(v,"id",&origin.id) || !integer(v,"catalog",&origin.catalog) ||
                !yyjson_is_str(name) || yyjson_get_len(name) >= sizeof(origin.catalog_name) ||
                strlen(yyjson_get_str(name)) != yyjson_get_len(name))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        memcpy(origin.catalog_name, yyjson_get_str(name), yyjson_get_len(name) + 1);
        EngineResult result = object_instance_origin_set(entity,origin);
        if(error_check(result)) return result;
    }
    v=yyjson_obj_get(components,"object_appearance");
    if(v) {
        uint64_t kind; ObjectAppearance appearance;
        if(!integer(v,"kind",&kind) || kind>OBJECT_TORQUE || !boolean(v,"visible",&appearance.visible) ||
                !boolean(v,"enabled",&appearance.enabled) || !color_read(v,"color",&appearance.color) ||
                !color_read(v,"border",&appearance.border_color) ||
                !object_visuals_json_read(yyjson_obj_get(v,"visuals"),&appearance.visuals)) return error_result_error(ERROR_ENGINE_STATE_INVALID);
        appearance.kind=(ObjectKind)kind;
        EngineResult result=object_instance_appearance_set(entity,appearance);
        if(error_check(result)) return result;
    }
    v=yyjson_obj_get(components,"sprite_state");
    if(v) {
        Sprite s={0};
        if(!sprite_read(v,&s.scale,&s.body_offset,&s.orientation_offset,&s.direction,&s.follow_entity_rotation,&s.visible))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        TextureAssetResult loaded=texture_read(v);
        if(error_check(loaded)) return error_result_error(loaded.result.error);
        s.texture=loaded.result.value;
        EngineResult result=graphics_sprite_add(entity, s);
        (void)graphics_texture_release(&loaded.result.value);
        if(error_check(result)) return result;
    }
    v=yyjson_obj_get(components,"animation_state");
    if(v) {
        EngineResult result=animation_read(entity,v);
        if(error_check(result)) return result;
        yyjson_val *target = yyjson_obj_get(v,"attachment_target");
        if(target) {
            if(!yyjson_is_str(target) || !yyjson_get_len(target) ||
                    strlen(yyjson_get_str(target)) != yyjson_get_len(target))
                return error_result_error(ERROR_ENGINE_STATE_INVALID);
            EntityResult found = entity_by_name_get(yyjson_get_str(target));
            if(error_check(found)) return error_result_error(found.result.error);
            result = graphics_animated_sprite_attachment_restore(entity, found.result.value);
            if(error_check(result)) return result;
        }
    }
    v=yyjson_obj_get(components,"object_camera");
    if(v) {
        CameraDescription config = {0}; bool enabled,paused;
        if(!vector_read(v,"position",&config.position.x,&config.position.y) ||
                !vector_read(v,"dimensions",&config.dimensions.x,&config.dimensions.y) ||
                !scalar(v,"orientation",&config.orientation) || !scalar(v,"zoom",&config.zoom) ||
                config.dimensions.x<=0 || config.dimensions.y<=0 || config.zoom<=0 ||
                !boolean(v,"enabled",&enabled) || !boolean(v,"paused",&paused))
            return error_result_error(ERROR_ENGINE_STATE_INVALID);
        config.enabled = enabled; config.pause_with_engine = paused;
        CameraIdResult created=graphics_camera_create(config);
        if(error_check(created)) return error_result_error(created.result.error);
        EngineResult result=object_instance_camera_restore(entity,created.result.value);
        if(error_check(result)) { (void)graphics_camera_destroy(created.result.value); return result; }
        yyjson_val *target=yyjson_obj_get(v,"target");
        if(!error_check(result) && target) {
            Position offset; Orientation angle; bool follow_position,follow_orientation;
            if(!yyjson_is_str(target) || strlen(yyjson_get_str(target))!=yyjson_get_len(target) ||
                    !vector_read(v,"offset",&offset.x,&offset.y) || !scalar(v,"angle_offset",&angle) ||
                    !boolean(v,"follow_position",&follow_position) || !boolean(v,"follow_orientation",&follow_orientation))
                return error_result_error(ERROR_ENGINE_STATE_INVALID);
            EntityResult target_entity=entity_by_name_get(yyjson_get_str(target));
            if(error_check(target_entity)) return error_result_error(target_entity.result.error);
            result=graphics_camera_attachment_description_set(created.result.value, (CameraAttachmentDescription){.entity = target_entity.result.value, .position_offset = offset, .orientation_offset = angle, .follow_position = follow_position, .follow_orientation = follow_orientation});
        }
        if(error_check(result)) return result;
    }
    const char *layers[] = {"graphics_layer", "sprite_layer", "animation_layer"};
    for(size_t i=0;i<3;++i) {
        v=yyjson_obj_get(components,layers[i]);
        if(!v) continue;
        EngineResult result;
        int layer;
        if(yyjson_is_str(v) && strlen(yyjson_get_str(v)) == yyjson_get_len(v)) {
            const char *name=yyjson_get_str(v);
            result=i==0 ? graphics_layer_entity_name_set(entity,name) :
                i==1 ? graphics_layer_sprite_name_set(entity,name) : graphics_layer_animation_name_set(entity,name);
        } else if(layer_value_read(v,&layer)) {
            result=i==0 ? graphics_layer_entity_set(entity,layer) :
                i==1 ? graphics_layer_sprite_set(entity,layer) : graphics_layer_animation_set(entity,layer);
        } else return error_result_error(ERROR_ENGINE_STATE_INVALID);
        if(error_check(result)) return result;
    }
    return error_result_value(true);
}
