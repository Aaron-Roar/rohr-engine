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
        "position", "orientation", "velocity", "acceleration", "angular_velocity", "entity_settings", "mass", "radius", "friction", "restitution",
        "static_body", "gravity_enabled", "collision_enabled", "rotation_locked", "visible", "center_of_mass_explicit",
        "center_of_mass", "particle_origin", "particle_vertices", "collision_filter", "geometry", "geometry_count",
        "active_geometry", "hitbox_bindings", "hitbox_binding_count", "color", "border_color", "color_overridden", "layer",
        "target", "endpoint_a", "endpoint_b", "joint_type", "rest_length", "stiffness", "damping", "rest_angle", "thickness",
        "area", "asset_path", "size", "scale", "frames", "frame_count", "ticks_per_frame", "time_per_frame", "starting_frame",
        "direction", "playing", "follow_position", "follow_orientation", "pause_with_engine", "zoom", "force", "torque", "visuals", "source_instance"};
    for(size_t i = 0; i < sizeof(fields)/sizeof(fields[0]); ++i) if(!strcmp(name, fields[i])) return true;
    return false;
}
static void visual_style_write(FILE *f,const char *name,ObjectVisualStyle v) {
    if(!v.specified) v=(ObjectVisualStyle){0};
    fprintf(f,".%s={.specified=%u,.fill_color=UINT32_C(%u),.border_color=UINT32_C(%u),"
        ".border_thickness=%.9ef,.scale={%.9ef,%.9ef},.offset={%.9ef,%.9ef},.rotation=%.9ef}",
        name,v.specified,v.fill_color,v.border_color,(double)v.border_thickness,
        (double)v.scale.x,(double)v.scale.y,(double)v.offset.x,(double)v.offset.y,(double)v.rotation);
}
static void entity_settings_write(FILE *f, ObjectEntitySettings s) {
    fprintf(f, ",.entity_settings={.lifetime={.deadline={"
        ".expiry_time=%.17g,.expiry_tick=UINT64_C(%llu),.time_enabled=%u,.tick_enabled=%u},"
        ".time_absolute=%u,.tick_absolute=%u},"
        ".angular_acceleration=%.9ef,.angular_velocity_maximum=%.9ef,.angular_velocity_maximum_enabled=%u,"
        ".gameplay_target={.kind=%u,.id=UINT64_C(%llu)},"
        ".axis_lock_enabled=%u,.axis_lock_world_fixed=%u,"
        ".axis_lock={.axis={%.9ef,%.9ef},.point_on_axis={%.9ef,%.9ef}},"
        ".transform_driver={.kind=%u,.id=UINT64_C(%llu)},"
        ".transform_offset={%.9ef,%.9ef},.transform_angle=%.9ef,"
        ".transform_position=%u,.transform_orientation=%u,.transform_inherit_velocity=%u}",
        s.lifetime.deadline.expiry_time, (unsigned long long)s.lifetime.deadline.expiry_tick,
        s.lifetime.deadline.time_enabled, s.lifetime.deadline.tick_enabled,
        s.lifetime.time_absolute, s.lifetime.tick_absolute,
        (double)s.angular_acceleration, (double)s.angular_velocity_maximum,
        s.angular_velocity_maximum_enabled, s.gameplay_target.kind,
        (unsigned long long)s.gameplay_target.id, s.axis_lock_enabled, s.axis_lock_world_fixed,
        (double)s.axis_lock.axis.x, (double)s.axis_lock.axis.y,
        (double)s.axis_lock.point_on_axis.x, (double)s.axis_lock.point_on_axis.y,
        s.transform_driver.kind, (unsigned long long)s.transform_driver.id,
        (double)s.transform_offset.x, (double)s.transform_offset.y, (double)s.transform_angle,
        s.transform_position, s.transform_orientation, s.transform_inherit_velocity);
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
static void source_reference_write(FILE *f, ObjectSourceReference r) {
    fprintf(f, "{.kind=%u,.id=UINT64_C(%llu),.soft_body=UINT64_C(%llu)}",
        (unsigned)r.kind, (unsigned long long)r.id, (unsigned long long)r.soft_body);
}
static void source_location_write(FILE *f, ObjectForceLocation v) {
    fprintf(f, "{.kind=%u,.offset={%#.9gf,%#.9gf}", (unsigned)v.kind, v.offset.x, v.offset.y);
    if(v.kind != ROHR_FORCE_WORLD_POSITION) {
        fputs(",.reference=", f); source_reference_write(f, v.reference);
    }
    if(v.kind == ROHR_FORCE_BEAM_POSITION)
        fprintf(f, ",.anchor=%u,.fraction=%#.9gf", (unsigned)v.anchor, v.fraction);
    fputc('}', f);
}
static void source_write(FILE *f, const ObjectDescription *d) {
    if(d->kind != OBJECT_FORCE && d->kind != OBJECT_TORQUE) return;
    bool force = d->kind == OBJECT_FORCE;
    fprintf(f, ",.%s={.mode=%u,.magnitude=%#.9gf,.target=", force ? "force" : "torque",
        (unsigned)(force ? d->force.mode : d->torque.mode), force ? d->force.magnitude : d->torque.magnitude);
    source_reference_write(f, force ? d->force.target : d->torque.target);
    if(force) {
        fputs(",.location=", f); source_location_write(f, d->force.location);
        ObjectForceOrientation o = d->force.orientation;
        fprintf(f, ",.orientation={.kind=%u,.offset=%#.9gf", (unsigned)o.kind, o.offset);
        if(o.kind == ROHR_FORCE_BETWEEN_LOCATIONS_ORIENTATION) {
            fputs(",.from=", f); source_location_write(f, o.from);
            fputs(",.to=", f); source_location_write(f, o.to);
        } else if(o.kind != ROHR_FORCE_WORLD_ORIENTATION) {
            fputs(",.reference=", f); source_reference_write(f, o.reference);
        }
        fputc('}', f);
    }
    fputc('}', f);
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
    if(d->visuals.main.specified || d->visuals.endpoint_a.specified || d->visuals.endpoint_b.specified) {
        fputs(",.visuals={",f); visual_style_write(f,"main",d->visuals.main);
        fputs(",",f); visual_style_write(f,"endpoint_a",d->visuals.endpoint_a);
        fputs(",",f); visual_style_write(f,"endpoint_b",d->visuals.endpoint_b); fputs("}",f);
    }
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
    U(pause_with_engine);
    entity_settings_write(f, d->entity_settings);
    source_write(f, d);
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
