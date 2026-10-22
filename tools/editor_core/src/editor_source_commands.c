/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_command.h"
#include "editor_sources_json.h"
#include "entity/object_visuals.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <float.h>

static EditorCommandResult failure(const char *message) {
    EditorCommandResult r = {.kind=ERROR_RESULT_ERROR};
    r.result.error.code = EDITOR_ERROR_INVALID_ARGUMENT;
    snprintf(r.result.error.message,sizeof(r.result.error.message),"%s",message);
    return r;
}
static size_t source_find(const EditorObject *o, bool torque, uint32_t id) {
    size_t count=torque?o->torque_count:o->force_count;
    for(size_t i=0;i<count;++i) if((torque?o->torque_items[i].id:o->force_items[i].id)==id) return i;
    return SIZE_MAX;
}
EditorCommandResult editor_source_command_execute(EditorProject *p, const EditorSourceCommand *c) {
    EditorObject *o=editor_object_query_get(p,c->object);
    if(!o) return failure("Source object was not found");
    size_t i=source_find(o,c->torque,c->id);
    if(c->operation==EDITOR_SOURCE_LIST) return (EditorCommandResult){.kind=ERROR_RESULT_VALUE};
    if(c->operation!=EDITOR_SOURCE_ADD && i==SIZE_MAX) return failure("Source was not found in this object");
    if(c->operation==EDITOR_SOURCE_GET) return (EditorCommandResult){.kind=ERROR_RESULT_VALUE};
    if(c->operation==EDITOR_SOURCE_REMOVE) {
        if(!editor_project_source_remove(o,c->torque,c->id)) return failure("Source removal failed");
        return (EditorCommandResult){.kind=ERROR_RESULT_VALUE};
    }
    if(c->operation==EDITOR_SOURCE_MARKER_SET) {
        if(!c->torque || !isfinite(c->marker_offset.x) || !isfinite(c->marker_offset.y)) return failure("Invalid torque marker offset");
        o->torque_items[i].marker_offset=c->marker_offset;
        return (EditorCommandResult){.kind=ERROR_RESULT_VALUE};
    }
    if(c->operation==EDITOR_SOURCE_SET) {
        bool ok=c->torque?editor_project_torque_set(o,c->id,c->torque_description):editor_project_force_set(o,c->id,c->force);
        return ok?(EditorCommandResult){.kind=ERROR_RESULT_VALUE}:failure("Invalid source description or reference outside its object");
    }
    if(c->operation!=EDITOR_SOURCE_ADD && c->operation!=EDITOR_SOURCE_DUPLICATE && c->operation!=EDITOR_SOURCE_RENAME)
        return failure("Unknown source operation");
    char name[EDITOR_OBJECT_NAME_MAX]={0};
    if(!memchr(c->name,0,sizeof(c->name))) return failure("Source name is too long");
    if(c->name[0]) editor_project_property_name_format(name,sizeof(name),c->name);
    else if(c->operation==EDITOR_SOURCE_RENAME) return failure("Source name is required");
    else snprintf(name,sizeof(name),"%s_%u",c->torque?"torque":"force",c->torque?p->next_torque_id:p->next_force_id);
    if(!name[0]) return failure("Source name is invalid");
    size_t count=c->torque?o->torque_count:o->force_count;
    for(size_t n=0;n<count;++n)
        if(!(c->operation==EDITOR_SOURCE_RENAME && n==i) && !strcmp(name,c->torque?o->torque_items[n].name:o->force_items[n].name))
            return failure("Source name already exists in this object");
    bool app_hidden=c->operation==EDITOR_SOURCE_DUPLICATE &&
        (c->torque?o->torque_items[i].app_hidden:o->force_items[i].app_hidden);
    ObjectVisuals visuals=c->operation==EDITOR_SOURCE_DUPLICATE?
        (c->torque?o->torque_items[i].visuals:o->force_items[i].visuals):(ObjectVisuals){0};
    EditorGraphicsLayerBinding graphics_layer=c->operation==EDITOR_SOURCE_DUPLICATE?
        (c->torque?o->torque_items[i].graphics_layer:o->force_items[i].graphics_layer):(EditorGraphicsLayerBinding){0};
    uint32_t id=c->id;
    if(c->torque) {
        bool visible=c->operation==EDITOR_SOURCE_DUPLICATE?o->torque_items[i].visible:true;
        Position marker=c->operation==EDITOR_SOURCE_DUPLICATE?o->torque_items[i].marker_offset:(Position){0};
        ObjectTorqueDescription value=c->operation==EDITOR_SOURCE_DUPLICATE?o->torque_items[i].description:(ObjectTorqueDescription){0};
        EditorTorque *t=c->operation==EDITOR_SOURCE_RENAME?&o->torque_items[i]:editor_project_torque_add(p,o);
        if(!t) return failure("Source capacity or allocation failed");
        if(c->operation!=EDITOR_SOURCE_RENAME) { t->description=value; t->visible=visible; t->marker_offset=marker; t->app_hidden=app_hidden; t->visuals=visuals; t->graphics_layer=graphics_layer; }
        strcpy(t->name,name); id=t->id;
    } else {
        bool visible=c->operation==EDITOR_SOURCE_DUPLICATE?o->force_items[i].visible:true;
        ObjectForceDescription value=c->operation==EDITOR_SOURCE_DUPLICATE?o->force_items[i].description:(ObjectForceDescription){0};
        EditorForce *f=c->operation==EDITOR_SOURCE_RENAME?&o->force_items[i]:editor_project_force_add(p,o);
        if(!f) return failure("Source capacity or allocation failed");
        if(c->operation!=EDITOR_SOURCE_RENAME) { f->description=value; f->visible=visible; f->app_hidden=app_hidden; f->visuals=visuals; f->graphics_layer=graphics_layer; }
        strcpy(f->name,name); id=f->id;
    }
    EditorCommandResult result={.kind=ERROR_RESULT_VALUE,.created={.valid=true,
        .kind=c->torque?EDITOR_ITEM_TORQUE:EDITOR_ITEM_FORCE,.object=o->id,.item=id}};
    strcpy(result.created.name,name); return result;
}
static bool uint_parse(const char *s,uint32_t *v) {
    if(!s || !*s || *s=='-') return false;
    char *end; errno=0; unsigned long long n=strtoull(s,&end,10);
    if(errno || *end || !n || n>UINT32_MAX) return false;
    *v=(uint32_t)n; return true;
}
static bool operation(const char *s) {
    return !strcmp(s,"add") || !strcmp(s,"set") || (!strcmp(s,"remove") || !strcmp(s,"delete")) || !strcmp(s,"rename") ||
        !strcmp(s,"duplicate") || !strcmp(s,"get") || !strcmp(s,"list");
}
static EditorResult invalid(const char *message) {
    return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,"%s",message);
}
EditorResult editor_source_cli_parse(const EditorProject *p,int count,char **args,const char **path,EditorCommand *command,bool *handled) {
    *handled=false;
    for(int n=1;n<count;++n) if(!strcmp(args[n],"--force") || !strcmp(args[n],"--force-id") || !strcmp(args[n],"--torque") || !strcmp(args[n],"--torque-id")) *handled=true;
    if(!*handled) return editor_result_value(true);
    EditorSourceCommand c={0}; const char *object_name=NULL,*source_name=NULL,*property="description",*op=NULL,*value=NULL;
    bool object_set=false,source_set=false,property_set=false,path_set=false;
    for(int n=1;n<count;++n) {
        const char *a=args[n];
        if(!strcmp(a,"--project")) {
            if(path_set || n+1==count) return invalid("Specify one project path");
            *path=args[++n];path_set=true;
        } else if(!strcmp(a,"--object") || !strcmp(a,"--object-id")) {
            if(object_set || n+1==count) return invalid("Specify one object selector");
            object_set=true;
            if(!strcmp(a,"--object-id")) { if(!uint_parse(args[++n],&c.object)) return invalid("Invalid object ID"); }
            else object_name=args[++n];
        } else if(!strcmp(a,"--force") || !strcmp(a,"--force-id") || !strcmp(a,"--torque") || !strcmp(a,"--torque-id")) {
            if(source_set) return invalid("Specify one source selector");
            source_set=true;c.torque=strstr(a,"torque")!=NULL;
            if(strstr(a,"-id")) { if(n+1==count || !uint_parse(args[++n],&c.id)) return invalid("Invalid source ID"); }
            else if(n+1<count && args[n+1][0]!='-' && !operation(args[n+1])) source_name=args[++n];
        } else if(!strcmp(a,"--property")) {
            if(property_set || n+1==count) return invalid("Specify one source property");
            property=args[++n];property_set=true;
            if(n+1<count && !operation(args[n+1])) { op="set"; value=args[++n]; }
        } else if(operation(a) && !op) {
            op=a;
        } else if(op && !value) value=a;
        else return invalid("Unexpected source argument");
    }
    if(!path_set || !object_set || !op) return invalid("Source commands require --project, --object or --object-id, and an operation");
    if(object_name) for(size_t i=0;i<p->object_count;++i) if(!strcmp(p->objects[i].name,object_name)) {
        if(c.object) return invalid("Ambiguous object name");
        c.object=p->objects[i].id;
    }
    const EditorObject *o=editor_object_query_const_get(p,c.object);
    if(!o) return invalid("Object was not found");
    if(source_name) for(size_t i=0;i<(c.torque?o->torque_count:o->force_count);++i)
        if(!strcmp(source_name,c.torque?o->torque_items[i].name:o->force_items[i].name)) {
            if(c.id) return invalid("Ambiguous source name");
            c.id=c.torque?o->torque_items[i].id:o->force_items[i].id;
        }
    c.operation=!strcmp(op,"add")?EDITOR_SOURCE_ADD:!strcmp(op,"set")?EDITOR_SOURCE_SET:
        (!strcmp(op,"remove") || !strcmp(op,"delete"))?EDITOR_SOURCE_REMOVE:!strcmp(op,"rename")?EDITOR_SOURCE_RENAME:
        !strcmp(op,"duplicate")?EDITOR_SOURCE_DUPLICATE:!strcmp(op,"get")?EDITOR_SOURCE_GET:EDITOR_SOURCE_LIST;
    size_t i=source_find(o,c.torque,c.id);
    if(c.operation!=EDITOR_SOURCE_ADD && c.operation!=EDITOR_SOURCE_LIST && i==SIZE_MAX) return invalid("Source was not found");
    if(c.operation==EDITOR_SOURCE_ADD && source_name) {
        if(value) return invalid("Specify the new source name once");
        value=source_name;
    }
    if(c.operation==EDITOR_SOURCE_ADD && c.id) return invalid("An existing source cannot be added again");
    if(c.operation==EDITOR_SOURCE_LIST && (c.id || source_name)) return invalid("list selects a source kind, not an individual source");
    if(c.operation==EDITOR_SOURCE_ADD || c.operation==EDITOR_SOURCE_RENAME || c.operation==EDITOR_SOURCE_DUPLICATE) {
        if(property_set || (value && strlen(value)>=sizeof(c.name))) return invalid("Invalid source name or property");
        if(value) strcpy(c.name,value);
    } else if(c.operation==EDITOR_SOURCE_SET) {
        EditorPropertyKind visual_property; EditorPropertyValueKind visual_kind;
        if(editor_visual_property_parse(property,&visual_property,&visual_kind)) {
            char object_id[24],item_id[24]; snprintf(object_id,sizeof(object_id),"%u",c.object); snprintf(item_id,sizeof(item_id),"%u",c.id);
            if(!value) return invalid("Visual property requires a value");
            char *raw[]={"rohr-cli",c.torque?"torque":"force","set",(char *)*path,object_id,item_id,(char *)property,(char *)value};
            return editor_command_cli_parse(8,raw,path,command);
        }
        if(!strcmp(property,"visible_in_app")) {
            if(!value || (strcmp(value,"true") && strcmp(value,"false"))) return invalid("visible_in_app requires true or false");
            *command=(EditorCommand){.type=EDITOR_COMMAND_APP_VISIBILITY,
                .data.app_visibility={c.torque?EDITOR_ITEM_TORQUE:EDITOR_ITEM_FORCE,c.object,0,c.id,!strcmp(value,"true")}};
            return editor_result_value(true);
        }
        if(!strcmp(property,"visible")) {
            if(!value || (strcmp(value,"true") && strcmp(value,"false"))) return invalid("visible requires true or false");
            *command=(EditorCommand){.type=EDITOR_COMMAND_VISIBILITY,.data.visibility={c.torque?EDITOR_VISIBILITY_TORQUE:EDITOR_VISIBILITY_FORCE,c.object,0,c.id,!strcmp(value,"true")}};
            return editor_result_value(true);
        }
        if(c.torque && !strcmp(property,"marker_offset")) {
            yyjson_doc *d=value?yyjson_read(value,strlen(value),0):NULL;
            yyjson_val *v=d?yyjson_doc_get_root(d):NULL;
            yyjson_val *x=yyjson_obj_get(v,"x"),*y=yyjson_obj_get(v,"y");
            bool ok=yyjson_is_obj(v) && yyjson_obj_size(v)==2 && yyjson_is_num(x) && yyjson_is_num(y) &&
                isfinite(yyjson_get_num(x)) && isfinite(yyjson_get_num(y)) && fabs(yyjson_get_num(x))<=FLT_MAX && fabs(yyjson_get_num(y))<=FLT_MAX;
            c.marker_offset=(Position){(float)yyjson_get_num(x),(float)yyjson_get_num(y)};
            yyjson_doc_free(d);
            if(!ok) return invalid("marker_offset requires finite x and y");
            c.operation=EDITOR_SOURCE_MARKER_SET;
            *command=(EditorCommand){.type=EDITOR_COMMAND_SOURCE,.data.source=c};
            return editor_result_value(true);
        }
        if(!value) return invalid("set requires a value (JSON for descriptions and references)");
        yyjson_mut_doc *d=yyjson_mut_doc_new(NULL);
        if(!d) return invalid("Allocation failed");
        yyjson_mut_val *root=c.torque?editor_torque_json_write(d,o->torque_items[i].description):editor_force_json_write(d,o->force_items[i].description);
        yyjson_mut_doc_set_root(d,root);
        bool scalar_mode=!strcmp(property,"mode");
        yyjson_doc *parsed=scalar_mode?NULL:yyjson_read(value,strlen(value),0);
        bool ok=root!=NULL;
        if(!strcmp(property,"description")) {
            ok=ok && parsed!=NULL;
            if(ok) yyjson_mut_doc_set_root(d,yyjson_val_mut_copy(d,yyjson_doc_get_root(parsed)));
        } else {
            bool allowed=scalar_mode || !strcmp(property,"magnitude") || !strcmp(property,"target") ||
                (!c.torque && (!strcmp(property,"location") || !strcmp(property,"orientation")));
            yyjson_mut_val *replacement=scalar_mode?yyjson_mut_strcpy(d,value):parsed?yyjson_val_mut_copy(d,yyjson_doc_get_root(parsed)):NULL;
            ok=ok && allowed && replacement && yyjson_mut_obj_put(root,yyjson_mut_strcpy(d,property),replacement);
        }
        yyjson_doc *copy=ok?yyjson_mut_doc_imut_copy(d,NULL):NULL;
        ok=copy && (c.torque?editor_torque_json_read(yyjson_doc_get_root(copy),&c.torque_description):editor_force_json_read(yyjson_doc_get_root(copy),&c.force));
        yyjson_doc_free(copy);yyjson_doc_free(parsed);yyjson_mut_doc_free(d);
        if(!ok) return invalid("Malformed source property or description");
        if(!(c.torque?editor_torque_description_check(o,c.torque_description):editor_force_description_check(o,c.force))) return invalid("Invalid source reference or value");
    } else if(value || property_set) return invalid("Unexpected source value or property");
    *command=(EditorCommand){.type=EDITOR_COMMAND_SOURCE,.data.source=c};
    return editor_result_value(true);
}
char *editor_source_query_json(const EditorProject *p,const EditorSourceCommand *c) {
    const EditorObject *o=editor_object_query_const_get(p,c->object); if(!o) return NULL;
    yyjson_mut_doc *d=yyjson_mut_doc_new(NULL);if(!d) return NULL;
    yyjson_mut_val *arr=yyjson_mut_arr(d);yyjson_mut_doc_set_root(d,arr);
    bool ok=arr!=NULL; size_t found=0;
    for(size_t i=0;i<(c->torque?o->torque_count:o->force_count);++i) {
        uint32_t id=c->torque?o->torque_items[i].id:o->force_items[i].id;
        if(c->operation!=EDITOR_SOURCE_LIST && c->id!=id) continue;
        yyjson_mut_val *v=c->torque?editor_torque_json_write(d,o->torque_items[i].description):editor_force_json_write(d,o->force_items[i].description);
        if(v) editor_json_graphics_layer_binding_write(d,v,c->torque?o->torque_items[i].graphics_layer:o->force_items[i].graphics_layer);
        if(v) ok=ok && yyjson_mut_obj_add_val(d,v,"visuals",object_visuals_json_write(d,c->torque?o->torque_items[i].visuals:o->force_items[i].visuals));
        if(v) ok=ok && yyjson_mut_obj_add_bool(d,v,"visible_in_app",
            !(c->torque?o->torque_items[i].app_hidden:o->force_items[i].app_hidden));
        if(v) ok=ok && yyjson_mut_obj_add_bool(d,v,"visible",c->torque?o->torque_items[i].visible:o->force_items[i].visible);
        if(c->torque && v) ok=ok && yyjson_mut_obj_add_real(d,v,"marker_offset_x",o->torque_items[i].marker_offset.x) &&
            yyjson_mut_obj_add_real(d,v,"marker_offset_y",o->torque_items[i].marker_offset.y);
        ok=ok && v && yyjson_mut_obj_add_uint(d,v,"id",id) && yyjson_mut_obj_add_strcpy(d,v,"name",c->torque?o->torque_items[i].name:o->force_items[i].name) && yyjson_mut_arr_add_val(arr,v);++found;
    }
    char *result=ok && (found || c->operation==EDITOR_SOURCE_LIST)?yyjson_mut_write(d,YYJSON_WRITE_PRETTY,NULL):NULL;
    yyjson_mut_doc_free(d);return result;
}
static bool quote(char *out,size_t cap,size_t *used,const char *s) {
    if(*used+1>=cap) return false;
    out[(*used)++]='\'';
    for(;*s;++s) {
        const char *value=*s=='\''?"'\\''":NULL;
        size_t size=value?4:1;
        if(*used+size+2>=cap) return false;
        if(value) memcpy(out+*used,value,size);else out[*used]=*s;
        *used+=size;
    }
    out[(*used)++]='\'';out[*used]=0;return true;
}
EditorResult editor_source_cli_write(const EditorProject *p,const EditorCommand *command,const char *path,char *out,size_t cap) {
    if(command->type==EDITOR_COMMAND_VISIBILITY && (command->data.visibility.kind==EDITOR_VISIBILITY_FORCE || command->data.visibility.kind==EDITOR_VISIBILITY_TORQUE)) {
        if(!out || !cap || !path) return invalid("Invalid CLI output");
        int n=snprintf(out,cap,"rohr-cli --project ");
        if(n<0 || (size_t)n>=cap) return invalid("CLI output too small");
        size_t used=(size_t)n;
        if(!quote(out,cap,&used,path)) return invalid("CLI output too small");
        n=snprintf(out+used,cap-used," --object-id %u --%s-id %u --property visible set %s",
            command->data.visibility.object,command->data.visibility.kind==EDITOR_VISIBILITY_TORQUE?"torque":"force",command->data.visibility.item,command->data.visibility.visible?"true":"false");
        return n>=0 && (size_t)n<cap-used?editor_result_value(true):invalid("CLI output too small");
    }
    if(command->type == EDITOR_COMMAND_OBJECT_DUPLICATE) {
        if(!out || !cap || !path) return invalid("Invalid CLI output");
        int n=snprintf(out,cap,"rohr-cli --project ");
        if(n<0 || (size_t)n>=cap) return invalid("CLI output too small");
        size_t used=(size_t)n;
        if(!quote(out,cap,&used,path)) return invalid("CLI output too small");
        n=snprintf(out+used,cap-used," --object-id %u duplicate",command->data.object_remove.object);
        return n>=0 && (size_t)n<cap-used?editor_result_value(true):invalid("CLI output too small");
    }
    (void)p; const EditorSourceCommand *c=&command->data.source;
    if(!out || !cap || !path) return invalid("Invalid CLI output");
    const char *ops[]={"add","set","remove","rename","duplicate","get","list","set"};
    if(c->operation<EDITOR_SOURCE_ADD || c->operation>EDITOR_SOURCE_MARKER_SET) return invalid("Invalid source operation");
    size_t used=0;int n=snprintf(out,cap,"rohr-cli --project ");if(n<0 || (size_t)n>=cap) return invalid("CLI output too small");used=n;
    if(!quote(out,cap,&used,path)) return invalid("CLI output too small");
    bool id=c->operation!=EDITOR_SOURCE_ADD && c->operation!=EDITOR_SOURCE_LIST;
    n=snprintf(out+used,cap-used," --object-id %u --%s%s",c->object,c->torque?"torque":"force",id?"-id":"");
    if(n<0 || (size_t)n>=cap-used) return invalid("CLI output too small");
    used+=n;
    if(id) { n=snprintf(out+used,cap-used," %u",c->id);if(n<0 || (size_t)n>=cap-used) return invalid("CLI output too small");
    used+=n; }
    if(c->operation==EDITOR_SOURCE_MARKER_SET) {
        n=snprintf(out+used,cap-used," --property marker_offset set '{\"x\":%.9g,\"y\":%.9g}'",c->marker_offset.x,c->marker_offset.y);
        return n>=0 && (size_t)n<cap-used?editor_result_value(true):invalid("CLI output too small");
    }
    n=snprintf(out+used,cap-used," %s",ops[c->operation]);if(n<0 || (size_t)n>=cap-used) return invalid("CLI output too small");
    used+=n;
    char *json=NULL; const char *value=NULL;
    if(c->operation==EDITOR_SOURCE_SET) {
        yyjson_mut_doc *d=yyjson_mut_doc_new(NULL);if(!d) return invalid("Allocation failed");
        yyjson_mut_doc_set_root(d,c->torque?editor_torque_json_write(d,c->torque_description):editor_force_json_write(d,c->force));
        json=yyjson_mut_write(d,0,NULL);yyjson_mut_doc_free(d);if(!json) return invalid("Invalid source description");value=json;
    } else if(c->name[0]) value=c->name;
    bool ok=true;
    if(value) { if(used+1>=cap) ok=false;else { out[used++]=' ';ok=quote(out,cap,&used,value); } }
    free(json);return ok?editor_result_value(true):invalid("CLI output too small");
}
