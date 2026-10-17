/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_command.h"
#include "editor_sources_json.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

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
    uint32_t id=c->id;
    if(c->torque) {
        ObjectTorqueDescription value=c->operation==EDITOR_SOURCE_DUPLICATE?o->torque_items[i].description:(ObjectTorqueDescription){0};
        EditorTorque *t=c->operation==EDITOR_SOURCE_RENAME?&o->torque_items[i]:editor_project_torque_add(p,o);
        if(!t) return failure("Source capacity or allocation failed");
        if(c->operation!=EDITOR_SOURCE_RENAME) t->description=value;
        strcpy(t->name,name); id=t->id;
    } else {
        ObjectForceDescription value=c->operation==EDITOR_SOURCE_DUPLICATE?o->force_items[i].description:(ObjectForceDescription){0};
        EditorForce *f=c->operation==EDITOR_SOURCE_RENAME?&o->force_items[i]:editor_project_force_add(p,o);
        if(!f) return failure("Source capacity or allocation failed");
        if(c->operation!=EDITOR_SOURCE_RENAME) f->description=value;
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
    const char *ops[]={"add","set","remove","rename","duplicate","get","list"};
    if(c->operation<EDITOR_SOURCE_ADD || c->operation>EDITOR_SOURCE_LIST) return invalid("Invalid source operation");
    size_t used=0;int n=snprintf(out,cap,"rohr-cli --project ");if(n<0 || (size_t)n>=cap) return invalid("CLI output too small");used=n;
    if(!quote(out,cap,&used,path)) return invalid("CLI output too small");
    bool id=c->operation!=EDITOR_SOURCE_ADD && c->operation!=EDITOR_SOURCE_LIST;
    n=snprintf(out+used,cap-used," --object-id %u --%s%s",c->object,c->torque?"torque":"force",id?"-id":"");
    if(n<0 || (size_t)n>=cap-used) return invalid("CLI output too small");
    used+=n;
    if(id) { n=snprintf(out+used,cap-used," %u",c->id);if(n<0 || (size_t)n>=cap-used) return invalid("CLI output too small");
    used+=n; }
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
