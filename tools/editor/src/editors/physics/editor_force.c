/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_force.h"
#include "editor_force_geometry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Choice indices are shared only inside this panel. */
static const char *choices[]={"Persistent","Initial (one tick)","World","COM","Origin","Beam",
    "World","Body","Beam","Between locations","Start","Center","End","[X]","[ ]","None"};
bool editor_force_editor_create(EditorForceEditor *e, FontAsset *font) {
    if(!e || !font) return false;
    *e=(EditorForceEditor){.font=font,.content_height=500};
    e->references=calloc(EDITOR_FORCE_REFERENCE_MAX,sizeof(*e->references));
    e->options=calloc(EDITOR_FORCE_REFERENCE_MAX,sizeof(*e->options));
    e->indices=calloc(EDITOR_FORCE_REFERENCE_MAX,sizeof(*e->indices));
    if(!e->references || !e->options || !e->indices) goto fail;
    for(size_t i=0;i<16;++i) if(!editor_mode_text_create(font,choices[i],&e->choices[i])) goto fail;
    for(size_t i=0;i<EDITOR_FORCE_FIELD_MAX;++i)
        if(!editor_mode_text_create(font,"",&e->fields[i])) goto fail;
    if(!editor_mode_text_create(font,"Delete Force",&e->delete_label) ||
       !editor_mode_accordion_section_create(&e->source_section,font,"Source",true) ||
       !editor_mode_accordion_section_create(&e->location_section,font,"Application",true) ||
       !editor_mode_accordion_section_create(&e->orientation_section,font,"Direction",true)) goto fail;
    return true;
fail: editor_force_editor_destroy(e); return false;
}
void editor_force_editor_destroy(EditorForceEditor *e) {
    if(!e) return;
    for(size_t i=0;i<16;++i) rohr_graphics_text_destroy(&e->choices[i]);
    for(size_t i=0;i<EDITOR_FORCE_FIELD_MAX;++i) {
        rohr_graphics_text_destroy(&e->labels[i]); rohr_graphics_text_destroy(&e->fields[i]);
    }
    if(e->references) for(size_t i=0;i<EDITOR_FORCE_REFERENCE_MAX;++i) rohr_graphics_text_destroy(&e->references[i].text);
    free(e->references); free(e->options); free(e->indices);
    rohr_graphics_text_destroy(&e->name); rohr_graphics_text_destroy(&e->delete_label);
    editor_mode_accordion_section_destroy(&e->source_section);
    editor_mode_accordion_section_destroy(&e->location_section);
    editor_mode_accordion_section_destroy(&e->orientation_section);
    *e=(EditorForceEditor){0};
}
static bool reference_add(EditorForceEditor *e,ObjectSourceReference r,const char *prefix,const char *name) {
    if(e->reference_count>=EDITOR_FORCE_REFERENCE_MAX) return false;
    EditorForceReferenceLabel *v=&e->references[e->reference_count++];
    v->reference=r;
    char text[sizeof(v->name)]; snprintf(text,sizeof(text),"%s%s",prefix,name);
    return editor_mode_named_text_sync(e->font,text,&v->text,v->name,sizeof(v->name));
}
static bool references_sync(EditorForceEditor *e,const EditorObject *o) {
    e->reference_count=0;
    if(!reference_add(e,(ObjectSourceReference){0},"","None")) return false;
    for(size_t i=0;i<o->rigid_body_count;++i) {
        const EditorRigidBody *b=&o->rigid_bodies[i];
        if(!reference_add(e,(ObjectSourceReference){.kind=b->standalone_particle?OBJECT_SOURCE_PARTICLE:OBJECT_SOURCE_BODY,.id=b->id},b->standalone_particle?"Particle: ":"Body: ",b->name)) return false;
    }
    for(size_t i=0;i<o->soft_body_count;++i) {
        const EditorSoftBody *s=&o->soft_body_items[i]; char prefix[EDITOR_OBJECT_NAME_MAX+4];
        if(!reference_add(e,(ObjectSourceReference){.kind=OBJECT_SOURCE_SOFT_BODY,.id=s->id},"Soft body: ",s->name)) return false;
        snprintf(prefix,sizeof(prefix),"%s / ",s->name);
        for(size_t n=0;n<s->node_count;++n) if(!reference_add(e,(ObjectSourceReference){OBJECT_SOURCE_NODE,s->nodes[n].id,s->id},prefix,s->nodes[n].name)) return false;
        for(size_t n=0;n<s->beam_count;++n) if(!reference_add(e,(ObjectSourceReference){OBJECT_SOURCE_BEAM,s->beams[n].id,s->id},prefix,s->beams[n].name)) return false;
    }
    return true;
}
static UIRect row(EditorForceEditor *e,const EditorModeContext *c,float *y,const char *label,char *id) {
    size_t n=e->row++;
    snprintf(id,64,"editor.force.%s.%s",e->scope,label);
    if(n<EDITOR_FORCE_FIELD_MAX && editor_mode_named_text_sync(e->font,label,&e->labels[n],e->label_cache[n],sizeof(e->label_cache[n])))
        rohr_ui_label(&e->labels[n],(UIRect){c->x+10,*y,110,28});
    UIRect r={c->x+122,*y,c->width-132,28}; *y+=34; return r;
}
static bool number(EditorForceEditor *e,const EditorModeContext *c,float *y,const char *label,float *value) {
    char id[64]; size_t n=e->row; UIRect r=row(e,c,y,label,id);
    if(n>=EDITOR_FORCE_FIELD_MAX) return false;
    UIFieldResult result=editor_mode_field(id,(UIFieldBinding){.kind=UI_FIELD_FLOAT,.number=value},&e->fields[n],r,NULL);
    e->editing|=result.active; return result.changed;
}
static bool choice(EditorForceEditor *e,const EditorModeContext *c,float *y,const char *label,size_t first,size_t count,int *value) {
    const TextAsset *options[16]; for(size_t i=0;i<count;++i) options[i]=&e->choices[first+i];
    char id[64]; UIRect r=row(e,c,y,label,id);
    UIDropdownResult result=editor_mode_dropdown(id,options,count,(size_t)*value,r,NULL);
    if(result.changed) *value=(int)result.selected_index;
    return result.changed;
}
/* mask selects valid reference kinds; None is permitted only for the target. */
static bool reference(EditorForceEditor *e,const EditorModeContext *c,float *y,const char *label,unsigned mask,ObjectSourceReference *ref) {
    size_t count=0,selected=0;
    for(size_t i=0;i<e->reference_count;++i) {
        ObjectSourceReference r=e->references[i].reference;
        if(!(mask & (1u<<r.kind))) continue;
        if(r.kind==ref->kind && r.id==ref->id && r.soft_body==ref->soft_body) selected=count;
        e->indices[count]=i; e->options[count++]=&e->references[i].text;
    }
    char id[64]; UIRect bounds=row(e,c,y,label,id);
    if(!count) { rohr_ui_label(&e->choices[15],bounds); return false; }
    UIDropdownResult result=editor_mode_dropdown(id,e->options,count,selected,bounds,NULL);
    if(result.changed) *ref=e->references[e->indices[result.selected_index]].reference;
    return result.changed;
}
static bool first_reference(EditorForceEditor *e,unsigned mask,ObjectSourceReference *r) {
    for(size_t i=0;i<e->reference_count;++i) if(mask&(1u<<e->references[i].reference.kind)) {
        *r=e->references[i].reference; return true;
    }
    return false;
}
#define REFERENCE_ENTITIES ((1u<<OBJECT_SOURCE_BODY)|(1u<<OBJECT_SOURCE_PARTICLE)|(1u<<OBJECT_SOURCE_SOFT_BODY)|(1u<<OBJECT_SOURCE_NODE)|(1u<<OBJECT_SOURCE_BEAM))
static size_t location_rows(ObjectForceLocation l) { return 3+(l.kind!=ROHR_FORCE_WORLD_POSITION)+(l.kind==ROHR_FORCE_BEAM_POSITION?2:0); }
static bool location(EditorForceEditor *e,const EditorModeContext *c,float *y,ObjectForceLocation *l) {
    bool changed=false; int kind=l->kind;
    if(choice(e,c,y,"Reference",2,4,&kind)) {
        ObjectForceLocation next={.kind=kind};
        unsigned mask=kind==ROHR_FORCE_BEAM_POSITION?1u<<OBJECT_SOURCE_BEAM:REFERENCE_ENTITIES;
        if(kind==ROHR_FORCE_WORLD_POSITION || first_reference(e,mask,&next.reference)) { *l=next; changed=true; }
    }
    if(l->kind!=ROHR_FORCE_WORLD_POSITION) changed|=reference(e,c,y,"Relative to",l->kind==ROHR_FORCE_BEAM_POSITION?1u<<OBJECT_SOURCE_BEAM:REFERENCE_ENTITIES,&l->reference);
    if(l->kind==ROHR_FORCE_BEAM_POSITION) {
        int anchor=l->anchor; changed|=choice(e,c,y,"Anchor",10,3,&anchor); l->anchor=anchor;
        changed|=number(e,c,y,"Fraction",&l->fraction);
    }
    changed|=number(e,c,y,l->kind==ROHR_FORCE_BEAM_POSITION?"Along":"X offset",&l->offset.x);
    changed|=number(e,c,y,l->kind==ROHR_FORCE_BEAM_POSITION?"Left offset":"Y offset",&l->offset.y);
    return changed;
}
static EditorModeAccordionLayoutResult section(EditorModeAccordionLayoutCursor *cursor,EditorModeAccordionSection *s,const char *id,size_t count) {
    float rows[EDITOR_FORCE_FIELD_MAX]; for(size_t i=0;i<count;++i) rows[i]=28;
    return editor_mode_accordion_layout_section(cursor,s,id,rows,count,6);
}
bool editor_force_editor_draw(EditorForceEditor *e,const EditorModeContext *c) {
    if(!e || !c || !c->project || !c->viewport) return false;
    EditorObject *o=editor_project_selected_get(c->project);
    EditorForce *f=editor_project_force_get(o,c->viewport->selected_force);
    if(!f || !references_sync(e,o)) return false;
    e->row=0; e->scope="common"; e->editing=false; float y=42; char id[64],name[EDITOR_OBJECT_NAME_MAX];
    snprintf(name,sizeof(name),"%s",f->name);
    if(!editor_mode_named_text_sync(e->font,name,&e->name,e->name_cache,sizeof(e->name_cache))) return false;
    UIRect bounds=row(e,c,&y,"Name",id);
    UIFieldResult named=editor_mode_name_field("editor.force.name",name,sizeof(name),&e->name,bounds);
    e->editing|=named.active;
    if(named.changed) {
        EditorCommand command={.type=EDITOR_COMMAND_SOURCE,.data.source={.object=o->id,.id=f->id,.operation=EDITOR_SOURCE_RENAME}};
        snprintf(command.data.source.name,sizeof(command.data.source.name),"%s",name);
        (void)editor_command_execute(c->project,&command);
    }
    int visible=f->visible?0:1;
    if(choice(e,c,&y,"Editor Visibility",13,2,&visible)) {
        EditorCommand command={.type=EDITOR_COMMAND_VISIBILITY,.data.visibility={EDITOR_VISIBILITY_FORCE,o->id,0,f->id,visible==0}};
        (void)editor_command_execute(c->project,&command);
    }
    ObjectForceDescription d=f->description; bool changed=false;
    EditorModeAccordionLayoutCursor cursor=editor_mode_accordion_layout_cursor_get(c->x,c->width,y+6);
    EditorModeAccordionLayoutResult s=section(&cursor,&e->source_section,"editor.force.source",3);
    if(s.expanded) {
        e->scope="source"; y=s.content_y; int mode=d.mode; changed|=choice(e,c,&y,"Mode",0,2,&mode); d.mode=mode;
        changed|=reference(e,c,&y,"Target",REFERENCE_ENTITIES|(1u<<OBJECT_SOURCE_BEAM)|1u,&d.target);
        changed|=number(e,c,&y,"Magnitude",&d.magnitude);
    }
    s=section(&cursor,&e->location_section,"editor.force.application",location_rows(d.location));
    if(s.expanded) { e->scope="application"; y=s.content_y; changed|=location(e,c,&y,&d.location); }
    size_t rows=2+(d.orientation.kind==ROHR_FORCE_WORLD_ORIENTATION?0:d.orientation.kind==ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION?2+location_rows(d.orientation.from)+location_rows(d.orientation.to):1);
    s=section(&cursor,&e->orientation_section,"editor.force.direction",rows);
    if(s.expanded) {
        e->scope="direction"; y=s.content_y; int kind=d.orientation.kind;
        if(choice(e,c,&y,"Reference",6,4,&kind)) {
            ObjectForceOrientation next={.kind=kind};
            unsigned mask=kind==ROHR_FORCE_BODY_ORIENTATION?REFERENCE_ENTITIES:1u<<OBJECT_SOURCE_BEAM;
            if(kind==ROHR_FORCE_WORLD_ORIENTATION || kind==ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION || first_reference(e,mask,&next.reference)) { d.orientation=next; changed=true; }
        }
        if(d.orientation.kind==ROHR_FORCE_BODY_ORIENTATION || d.orientation.kind==ROHR_FORCE_BEAM_ORIENTATION)
            changed|=reference(e,c,&y,"Relative to",(d.orientation.kind==ROHR_FORCE_BODY_ORIENTATION?REFERENCE_ENTITIES:1u<<OBJECT_SOURCE_BEAM),&d.orientation.reference);
        if(d.orientation.kind==ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION) {
            e->scope="from"; (void)row(e,c,&y,"From location",id); changed|=location(e,c,&y,&d.orientation.from);
            e->scope="to"; (void)row(e,c,&y,"To location",id); changed|=location(e,c,&y,&d.orientation.to);
        }
        e->scope="direction"; changed|=number(e,c,&y,"Rotation (deg)",&d.orientation.offset);
    }
    if(changed) {
        EditorCommand command={.type=EDITOR_COMMAND_SOURCE,.data.source={.object=o->id,.id=f->id,.operation=EDITOR_SOURCE_SET,.force=d}};
        (void)editor_command_execute(c->project,&command);
    }
    Position p; float angle;
    y=cursor.y;
    if(!editor_force_location_get(o,d.location,&p) || !editor_force_orientation_get(o,d.orientation,&angle))
        (void)row(e,c,&y,"Unresolved reference",id);
    e->content_height=y+60;
    return e->editing;
}
