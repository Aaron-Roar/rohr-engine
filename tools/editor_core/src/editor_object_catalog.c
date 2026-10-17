/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_object_catalog.h"
#include "entity/object_internal.h"
#include <stdlib.h>
#include <string.h>

typedef struct CatalogEntry { const ObjectDescription *d; char *path; } CatalogEntry;
typedef struct Catalog { CatalogEntry entries[MAX_ENTITIES]; size_t count; } Catalog;
static const char *path_get(const Catalog *c, const ObjectDescription *d) {
    for(size_t i = 0; i < c->count; ++i) if(c->entries[i].d == d) return c->entries[i].path;
    return NULL;
}
static bool reserved(const char *name) {
    static const char *fields[] = {"_description", "id", "name", "kind", "categories", "parent", "children", "child_count",
        "position", "orientation", "velocity", "acceleration", "angular_velocity", "mass", "radius", "friction", "restitution",
        "static_body", "gravity_enabled", "collision_enabled", "rotation_locked", "visible", "center_of_mass_explicit",
        "center_of_mass", "particle_origin", "particle_vertices", "collision_filter", "geometry", "geometry_count",
        "active_geometry", "hitbox_bindings", "hitbox_binding_count", "color", "border_color", "color_overridden", "layer",
        "target", "endpoint_a", "endpoint_b", "joint_type", "rest_length", "stiffness", "damping", "rest_angle", "thickness",
        "area", "asset_path", "size", "scale", "frames", "frame_count", "ticks_per_frame", "time_per_frame", "starting_frame",
        "direction", "playing", "follow_position", "follow_orientation", "zoom", "force", "torque"};
    for(size_t i = 0; i < sizeof(fields)/sizeof(fields[0]); ++i) if(!strcmp(name, fields[i])) return true;
    return false;
}
static bool collect(Catalog *c, const ObjectDescription *d, const char *path, unsigned depth) {
    if(c->count >= MAX_ENTITIES || depth > OBJECT_DEPTH_MAX) return false;
    char *copy = object_string_copy(path);
    if(!copy) return false;
    c->entries[c->count++] = (CatalogEntry){d, copy};
    for(size_t i = 0; i < d->child_count; ++i) {
        char name[EDITOR_OBJECT_NAME_MAX];
        editor_project_property_name_format(name, sizeof(name), d->children[i]->name);
        if(reserved(name)) {
            fprintf(stderr, "Object catalog: '%s.%s' conflicts with a description property\n", path, name);
            return false;
        }
        for(size_t j = 0; j < i; ++j) {
            char prior[EDITOR_OBJECT_NAME_MAX];
            editor_project_property_name_format(prior, sizeof(prior), d->children[j]->name);
            if(!strcmp(name, prior)) {
                fprintf(stderr, "Object catalog: children of '%s' both generate identifier '%s'\n", path, name);
                return false;
            }
        }
        size_t length = strlen(path) + strlen(name) + 2;
        char *child = malloc(length);
        if(!child) return false;
        snprintf(child, length, "%s.%s", path, name);
        bool ok = collect(c, d->children[i], child, depth + 1);
        free(child);
        if(!ok) return false;
    }
    return true;
}
static void string_write(FILE *f, const char *s) {
    if(!s) { fputs("NULL", f); return; }
    fputc('"', f);
    for(const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        if(*p == '"' || *p == '\\') fprintf(f, "\\%c", *p);
        else if(*p < 32 || *p > 126) fprintf(f, "\\%03o", *p);
        else fputc(*p, f);
    }
    fputc('"', f);
}
static void type_write(FILE *f, const ObjectDescription *d) {
    for(size_t i = 0; i < d->child_count; ++i) type_write(f, d->children[i]);
    fprintf(f, "typedef struct ObjectDescriptionView_%llu {\n"
        "    union { ObjectDescription _description; struct { ROHR_OBJECT_DESCRIPTION_FIELDS; }; };\n",
        (unsigned long long)d->id);
    for(size_t i = 0; i < d->child_count; ++i) {
        char name[EDITOR_OBJECT_NAME_MAX];
        editor_project_property_name_format(name, sizeof(name), d->children[i]->name);
        fprintf(f, "    ObjectDescriptionView_%llu %s;\n", (unsigned long long)d->children[i]->id, name);
    }
    fprintf(f, "} ObjectDescriptionView_%llu;\n", (unsigned long long)d->id);
}
static void array_write(FILE *f, const Catalog *c, const ObjectDescription *d) {
    unsigned long long id = d->id;
    if(d->child_count) {
        fprintf(f, "static const ObjectDescription *const object_children_%llu[] = {", id);
        for(size_t i = 0; i < d->child_count; ++i) fprintf(f, "%s&%s._description", i ? "," : "", path_get(c, d->children[i]));
        fputs("};\n", f);
    }
    if(d->geometry_count) {
        fprintf(f, "static const ObjectHitbox object_geometry_%llu[] = {", id);
        for(size_t i = 0; i < d->geometry_count; ++i) {
            const ObjectHitbox *h = &d->geometry[i];
            fprintf(f, "%s{.id=%u,.shape={.amount_of_vertices=%u,.vertices={", i ? "," : "", h->id, h->shape.amount_of_vertices);
            for(uint32_t j = 0; j < h->shape.amount_of_vertices; ++j)
                fprintf(f, "%s{%#.9gf,%#.9gf}", j ? "," : "", h->shape.vertices[j].x, h->shape.vertices[j].y);
            fputs("}}}", f);
        }
        fputs("};\n", f);
    }
    if(d->hitbox_binding_count) {
        fprintf(f, "static const HitboxAnimationBinding object_bindings_%llu[] = {", id);
        for(size_t i = 0; i < d->hitbox_binding_count; ++i)
            fprintf(f, "%s{%u,%u,%u}", i ? "," : "", d->hitbox_bindings[i].animation_id,
                d->hitbox_bindings[i].frame_id, d->hitbox_bindings[i].hitbox_id);
        fputs("};\n", f);
    }
    if(d->frame_count) {
        fprintf(f, "static const ObjectFrame object_frames_%llu[] = {", id);
        for(size_t i = 0; i < d->frame_count; ++i) {
            const ObjectFrame *v = &d->frames[i];
            fprintf(f, "%s{.id=%u,.path=", i ? "," : "", v->id); string_write(f, v->path);
            fprintf(f, ",.scale={%#.9gf,%#.9gf},.offset={%#.9gf,%#.9gf},.rotation=%#.9gf}",
                v->scale.x,v->scale.y,v->offset.x,v->offset.y,v->rotation);
        }
        fputs("};\n", f);
    }
    if(d->area) {
        fprintf(f, "static const ObjectAreaGeometry object_area_%llu = {.hole_count=%u,", id, d->area->hole_count);
        for(uint32_t l = 0; l <= d->area->hole_count; ++l) {
            const ObjectAreaLoop *loop = l ? &d->area->holes[l-1] : &d->area->outer;
            if(!l) fputs(".outer=", f);
            else fprintf(f, ".holes[%u]=", l-1);
            fprintf(f, "{.node_count=%u,.nodes={", loop->node_count);
            if(!loop->node_count) fputs("0", f);
            for(uint32_t j = 0; j < loop->node_count; ++j)
                fprintf(f, "%sUINT64_C(%llu)", j ? "," : "", (unsigned long long)loop->nodes[j]);
            fputs("}},", f);
        }
        fputs("};\n", f);
    }
}
static void value_write(FILE *f, const Catalog *c, const ObjectDescription *d) {
    unsigned long long id = d->id;
    fputs("{._description={.name=", f); string_write(f, d->name);
#define U(field) fprintf(f, ",." #field "=UINT64_C(%llu)", (unsigned long long)d->field)
#define F(field) fprintf(f, ",." #field "=%#.9gf", (double)d->field)
#define V(field) fprintf(f, ",." #field "={%#.9gf,%#.9gf}", (double)d->field.x, (double)d->field.y)
#define C(field) fprintf(f, ",." #field "={%u,%u,%u,%u}", d->field.red,d->field.green,d->field.blue,d->field.alpha)
    U(id); U(kind); U(categories); U(child_count);
    if(d->parent) fprintf(f, ",.parent=&%s._description", path_get(c,d->parent));
    if(d->child_count) fprintf(f, ",.children=object_children_%llu", id);
    V(position); F(orientation); V(velocity); V(acceleration); F(angular_velocity);
    F(mass); F(radius); F(friction); F(restitution);
    U(static_body); U(gravity_enabled); U(collision_enabled); U(rotation_locked); U(visible);
    U(center_of_mass_explicit); V(center_of_mass); V(particle_origin); U(particle_vertices);
    fprintf(f, ",.collision_filter={UINT64_C(%llu),UINT64_C(%llu)}",
        (unsigned long long)d->collision_filter.category, (unsigned long long)d->collision_filter.collides_with);
    U(geometry_count); U(active_geometry); U(hitbox_binding_count);
    if(d->geometry_count) fprintf(f, ",.geometry=object_geometry_%llu", id);
    if(d->hitbox_binding_count) fprintf(f, ",.hitbox_bindings=object_bindings_%llu", id);
    C(color); C(border_color); U(color_overridden); fprintf(f, ",.layer=%d", d->layer);
    U(target); U(endpoint_a); U(endpoint_b); U(joint_type);
    F(rest_length); F(stiffness); F(damping); F(rest_angle); F(thickness);
    if(d->area) fprintf(f, ",.area=&object_area_%llu", id);
    fputs(",.asset_path=",f); string_write(f,d->asset_path);
    V(size); V(scale); U(frame_count);
    if(d->frame_count) fprintf(f, ",.frames=object_frames_%llu", id);
    U(ticks_per_frame); fprintf(f, ",.time_per_frame=%.17g", d->time_per_frame);
    U(starting_frame); fprintf(f, ",.direction=%d", (int)d->direction);
    U(playing); U(follow_position); U(follow_orientation); F(zoom);
#undef U
#undef F
#undef V
#undef C
    fputs("}", f);
    for(size_t i = 0; i < d->child_count; ++i) {
        char name[EDITOR_OBJECT_NAME_MAX]; editor_project_property_name_format(name,sizeof(name),d->children[i]->name);
        fprintf(f, ",.%s=",name); value_write(f,c,d->children[i]);
    }
    fputs("}", f);
}
bool editor_object_catalog_write(FILE *header, FILE *source, const EditorProject *project) {
    if(!project) return false;
    for(size_t i = 0; i < project->object_count; ++i)
        if(project->objects[i].force_count || project->objects[i].torque_count) return false;
    ObjectDescriptionResult converted = object_description_project_read(project, "");
    if(error_check(converted)) return false;
    Catalog *catalog = calloc(1,sizeof(*catalog));
    bool ok = catalog && collect(catalog,converted.result.value,"objects",0);
    if(ok) {
        type_write(header,converted.result.value);
        fprintf(header,"extern const ObjectDescriptionView_%llu objects;\n\n",(unsigned long long)converted.result.value->id);
        for(size_t i = 0; i < catalog->count; ++i) array_write(source,catalog,catalog->entries[i].d);
        fprintf(source,"const ObjectDescriptionView_%llu objects = ",(unsigned long long)converted.result.value->id);
        value_write(source,catalog,converted.result.value); fputs(";\n",source);
        ok = !ferror(header) && !ferror(source);
    }
    if(catalog) for(size_t i = 0; i < catalog->count; ++i) free(catalog->entries[i].path);
    free(catalog); object_description_tree_destroy((ObjectDescription *)converted.result.value);
    return ok;
}
