/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_object_catalog.h"
#include "entity/object_internal.h"
#include <stdlib.h>
#include <string.h>

typedef struct CatalogEntry { const ObjectDescription *d; char *path; } CatalogEntry;
typedef struct Catalog { CatalogEntry entries[MAX_ENTITIES]; size_t count; const char *prefix; const EditorProject *project; } Catalog;
static const char *path_get(const Catalog *c, const ObjectDescription *d) {
    for(size_t i = 0; i < c->count; ++i) if(c->entries[i].d == d) return c->entries[i].path;
    return NULL;
}
static bool reserved(const char *name) {
    static const char *fields[] = {"_project", "_description", "viewports", "viewport_count", "id", "name", "kind", "categories", "parent", "children", "child_count",
        "position", "orientation", "velocity", "acceleration", "angular_velocity", "entity_settings", "mass", "radius", "friction", "restitution",
        "kinematic_driven", "static_body", "gravity_enabled", "collision_enabled", "rotation_locked", "visible", "center_of_mass_explicit",
        "center_of_mass", "particle_origin", "particle_vertices", "collision_filter", "geometry", "geometry_count",
        "active_geometry", "hitbox_bindings", "hitbox_binding_count", "hitbox_animation_driver", "color", "border_color", "color_overridden", "layer", "layer_name",
        "target", "endpoint_a", "endpoint_b", "joint_type", "rest_length", "stiffness", "damping", "rest_angle", "thickness",
        "area", "asset_path", "size", "scale", "frames", "frame_count", "ticks_per_frame", "time_per_frame", "starting_frame", "playback_mode",
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
void editor_catalog_input_binding_write(FILE *source,
        const InputBinding *binding) {
    fprintf(source, "        {.name = ");
    string_write(source, binding->name);
    fprintf(source, ", .source = %s, .input.",
        binding->source == INPUT_BINDING_KEY ? "INPUT_BINDING_KEY" :
        binding->source == INPUT_BINDING_MOUSE_BUTTON ?
            "INPUT_BINDING_MOUSE_BUTTON" :
        binding->source == INPUT_BINDING_MOUSE_MOTION ?
            "INPUT_BINDING_MOUSE_MOTION" : "INPUT_BINDING_MOUSE_WHEEL");
    if(binding->source == INPUT_BINDING_KEY)
        fprintf(source, "key = (SDL_Scancode)%u", (unsigned)binding->input.key);
    else if(binding->source == INPUT_BINDING_MOUSE_BUTTON)
        fprintf(source, "mouse_button = (InputMouseButton)%u",
            (unsigned)binding->input.mouse_button);
    else fprintf(source, "axis_component = %s",
        binding->input.axis_component == INPUT_AXIS_COMPONENT_X ?
            "INPUT_AXIS_COMPONENT_X" :
        binding->input.axis_component == INPUT_AXIS_COMPONENT_Y ?
            "INPUT_AXIS_COMPONENT_Y" : "INPUT_AXIS_COMPONENT_XY");
    fprintf(source,
        ", .modifiers = (SDL_Keymod)%u, .affects_x = %s, .affects_y = %s, "
        ".scale = {%#.9gf, %#.9gf}, "
        ".inverted_x = %s, .inverted_y = %s, "
        ".direction = {%#.9gf, %#.9gf}}",
        (unsigned)binding->modifiers,
        binding->affects_x ? "true" : "false",
        binding->affects_y ? "true" : "false",
        binding->scale.x, binding->scale.y,
        binding->inverted_x ? "true" : "false",
        binding->inverted_y ? "true" : "false",
        binding->direction.x, binding->direction.y);
}

static const char *viewport_item_name_get(const EditorLayoutViewport *view, size_t i) {
    return i < view->camera_item_count ? view->camera_items[i].name :
        view->ui_items[i - view->camera_item_count].name;
}
static void resource_types_write(FILE *f, const EditorProject *p) {
    for(size_t i = 0; i < p->input_controller_count; ++i) {
        const EditorInputController *c = &p->input_controllers[i];
        fprintf(f, "typedef struct ProjectControllerView_%zu {\n"
            "    union { ControllerDescription _description; struct { ROHR_CONTROLLER_DESCRIPTION_FIELDS; }; };\n", i);
        for(size_t j = 0; j < c->action_count; ++j) {
            char name[EDITOR_OBJECT_NAME_MAX];
            editor_project_property_name_format(name, sizeof(name), c->actions[j].name);
            fprintf(f, "    ControllerActionDescription %s;\n", name);
        }
        fprintf(f, "} ProjectControllerView_%zu;\n"
            "static inline ControllerDescription project_controller_view_%zu_recipe_get(\n"
            "        const ProjectControllerView_%zu *value) { return value->_description; }\n", i, i, i);
    }
    fputs("#undef ROHR_INTERNAL_CONTROLLER_VIEW_TYPES\n#define ROHR_INTERNAL_CONTROLLER_VIEW_TYPES(apply)", f);
    for(size_t i = 0; i < p->input_controller_count; ++i)
        fprintf(f, " \\\n    apply(ProjectControllerView_%zu, project_controller_view_%zu_recipe_get)", i, i);
    fputc('\n', f);
    for(size_t v = 0; v < p->layout_viewport_count; ++v) {
        const EditorLayoutViewport *view = &p->layout_viewports[v];
        fprintf(f, "typedef struct ProjectViewportView_%zu {\n"
            "    union { ViewportCreateDescription _description; struct { ROHR_VIEWPORT_CREATE_DESCRIPTION_FIELDS; }; };\n", v);
        for(size_t i = 0; i < view->camera_item_count + view->ui_item_count; ++i) {
            char name[EDITOR_OBJECT_NAME_MAX];
            editor_project_property_name_format(name, sizeof(name), viewport_item_name_get(view, i));
            fprintf(f, "    ViewportCreateItemDescription %s;\n", name);
        }
        fprintf(f, "} ProjectViewportView_%zu;\n"
            "static inline const ViewportCreateDescription *project_viewport_view_%zu_ref_get(\n"
            "        const ProjectViewportView_%zu *value) {\n"
            "    return &value->_description;\n}\n", v, v, v);
    }
    /* Register exact generated view types; creation borrows the recipe header. */
    fputs("#undef ROHR_INTERNAL_VIEWPORT_VIEW_TYPES\n#define ROHR_INTERNAL_VIEWPORT_VIEW_TYPES(apply)", f);
    for(size_t v = 0; v < p->layout_viewport_count; ++v)
        fprintf(f, " \\\n    apply(ProjectViewportView_%zu, project_viewport_view_%zu_ref_get)", v, v);
    fputc('\n', f);
}
static void type_write(FILE *f, const Catalog *c, const ObjectDescription *d) {
    for(size_t i = 0; i < d->child_count; ++i) type_write(f, c, d->children[i]);
    fprintf(f, "typedef struct %sDescriptionView_%llu {\n", c->prefix, (unsigned long long)d->id);
    if(c->project && !d->parent)
        fputs("    /* Initial project recipe; _description aliases its root for object-only access. */\n"
            "    union { ProjectDescription _project; struct { ObjectDescription _description; }; };\n", f);
    else
        fputs("    union { ObjectDescription _description; struct { ROHR_OBJECT_DESCRIPTION_FIELDS; }; };\n", f);
    for(size_t i = 0; i < d->child_count; ++i) {
        char name[EDITOR_OBJECT_NAME_MAX];
        editor_project_property_name_format(name, sizeof(name), d->children[i]->name);
        fprintf(f, "    %sDescriptionView_%llu %s;\n", c->prefix, (unsigned long long)d->children[i]->id, name);
    }
    if(c->project && !d->parent) {
        fputs("    const ViewportCreateDescription *const *viewports;\n    size_t viewport_count;\n", f);
        for(size_t i = 0; i < c->project->layout_viewport_count; ++i) {
        char name[EDITOR_OBJECT_NAME_MAX];
        editor_project_property_name_format(name, sizeof(name), c->project->layout_viewports[i].name);
        fprintf(f, "    ProjectViewportView_%zu %s;\n", i, name);
        }
        for(size_t i = 0; i < c->project->input_controller_count; ++i) {
            char name[EDITOR_OBJECT_NAME_MAX];
            editor_project_property_name_format(name, sizeof(name), c->project->input_controllers[i].name);
            fprintf(f, "    ProjectControllerView_%zu %s;\n", i, name);
        }
        for(size_t i = 0; i < c->project->audio_definition_count; ++i) {
            char name[EDITOR_OBJECT_NAME_MAX];
            editor_project_property_name_format(name, sizeof(name), c->project->audio_definitions[i].name);
            fprintf(f, "    %sDescription %s;\n", c->project->audio_definitions[i].music ? "Music" : "Sound", name);
        }
        for(size_t i = 0; i < c->project->graphics_layer_count; ++i) {
            char name[EDITOR_OBJECT_NAME_MAX];
            editor_project_property_name_format(name, sizeof(name), c->project->graphics_layers[i].name);
            fprintf(f, "    LayerDescription %s;\n", name);
        }
    }
    fprintf(f, "} %sDescriptionView_%llu;\n", c->prefix, (unsigned long long)d->id);
}
static void array_write(FILE *f, const Catalog *c, const ObjectDescription *d) {
    unsigned long long id = d->id;
    if(d->child_count) {
        fprintf(f, "static const ObjectDescription *const %s_children_%llu[] = {", c->prefix, id);
        for(size_t i = 0; i < d->child_count; ++i) fprintf(f, "%s&%s._description", i ? "," : "", path_get(c, d->children[i]));
        fputs("};\n", f);
    }
    if(d->geometry_count) {
        fprintf(f, "static const ObjectHitbox %s_geometry_%llu[] = {", c->prefix, id);
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
        fprintf(f, "static const HitboxAnimationBinding %s_bindings_%llu[] = {", c->prefix, id);
        for(size_t i = 0; i < d->hitbox_binding_count; ++i)
            fprintf(f, "%s{%u,%u,%u}", i ? "," : "", d->hitbox_bindings[i].animation_id,
                d->hitbox_bindings[i].frame_id, d->hitbox_bindings[i].hitbox_id);
        fputs("};\n", f);
    }
    if(d->frame_count) {
        fprintf(f, "static const ObjectFrame %s_frames_%llu[] = {", c->prefix, id);
        for(size_t i = 0; i < d->frame_count; ++i) {
            const ObjectFrame *v = &d->frames[i];
            fprintf(f, "%s{.id=%u,.path=", i ? "," : "", v->id); string_write(f, v->path);
            fprintf(f, ",.scale={%#.9gf,%#.9gf},.offset={%#.9gf,%#.9gf},.rotation=%#.9gf}",
                v->scale.x,v->scale.y,v->offset.x,v->offset.y,v->rotation);
        }
        fputs("};\n", f);
    }
    if(d->area) {
        fprintf(f, "static const ObjectAreaGeometry %s_area_%llu = {.hole_count=%u,", c->prefix, id, d->area->hole_count);
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
static bool viewport_arrays_write(FILE *f, const Catalog *c);
static void viewport_values_write(FILE *f, const EditorProject *project);
static void named_resources_write(FILE *f, const EditorProject *project);
static void value_write(FILE *f, const Catalog *c, const ObjectDescription *d) {
    unsigned long long id = d->id;
    bool root = c->project && !d->parent;
    fputs(root ? "{._project={.root={.name=" : "{._description={.name=", f);
    string_write(f, d->name);
#define U(field) fprintf(f, ",." #field "=UINT64_C(%llu)", (unsigned long long)d->field)
#define F(field) fprintf(f, ",." #field "=%#.9gf", (double)d->field)
#define V(field) fprintf(f, ",." #field "={%#.9gf,%#.9gf}", (double)d->field.x, (double)d->field.y)
#define C(field) fprintf(f, ",." #field "={%u,%u,%u,%u}", d->field.red,d->field.green,d->field.blue,d->field.alpha)
    U(id); U(kind); U(categories); U(child_count);
    if(d->parent) fprintf(f, ",.parent=&%s.%s", path_get(c,d->parent),
        c->project && !d->parent->parent ? "_project.root" : "_description");
    if(d->child_count) fprintf(f, ",.children=%s_children_%llu", c->prefix, id);
    V(position); F(orientation); V(velocity); V(acceleration); F(angular_velocity);
    F(mass); F(radius); F(friction); F(restitution);
    U(kinematic_driven); U(static_body); U(gravity_enabled); U(collision_enabled); U(rotation_locked); U(visible);
    U(center_of_mass_explicit); V(center_of_mass); V(particle_origin); U(particle_vertices);
    fprintf(f, ",.collision_filter={UINT64_C(%llu),UINT64_C(%llu)}",
        (unsigned long long)d->collision_filter.category, (unsigned long long)d->collision_filter.responds_to);
    U(geometry_count); U(active_geometry); U(hitbox_binding_count); U(hitbox_animation_driver);
    if(d->geometry_count) fprintf(f, ",.geometry=%s_geometry_%llu", c->prefix, id);
    if(d->hitbox_binding_count) fprintf(f, ",.hitbox_bindings=%s_bindings_%llu", c->prefix, id);
    if(d->visuals.main.specified || d->visuals.endpoint_a.specified || d->visuals.endpoint_b.specified) {
        fputs(",.visuals={",f); visual_style_write(f,"main",d->visuals.main);
        fputs(",",f); visual_style_write(f,"endpoint_a",d->visuals.endpoint_a);
        fputs(",",f); visual_style_write(f,"endpoint_b",d->visuals.endpoint_b); fputs("}",f);
    }
    C(color); C(border_color); U(color_overridden); fprintf(f, ",.layer=%d", d->layer);
    if(d->layer_name[0]) { fputs(",.layer_name=", f); string_write(f,d->layer_name); }
    U(target); U(endpoint_a); U(endpoint_b); U(joint_type);
    F(rest_length); F(stiffness); F(damping); F(rest_angle); F(thickness);
    if(d->area) fprintf(f, ",.area=&%s_area_%llu", c->prefix, id);
    fputs(",.asset_path=",f); string_write(f,d->asset_path);
    V(size); V(scale); U(frame_count);
    if(d->frame_count) fprintf(f, ",.frames=%s_frames_%llu", c->prefix, id);
    U(ticks_per_frame); fprintf(f, ",.time_per_frame=%.17g", d->time_per_frame);
    fprintf(f, ",.playback_mode=%s", d->playback_mode == ROHR_ANIMATION_PLAYBACK_ONCE ?
        "ROHR_ANIMATION_PLAYBACK_ONCE" : "ROHR_ANIMATION_PLAYBACK_LOOP");
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
    if(root) {
        const EditorProject *p = c->project;
        if(p->layout_viewport_count) fprintf(f, ",.viewports=project_viewport_recipes,.viewport_count=%zu", p->layout_viewport_count);
        if(p->input_controller_count) fprintf(f, ",.controllers=project_controller_recipes,.controller_count=%zu", p->input_controller_count);
        size_t sounds = 0, music = 0;
        for(size_t i = 0; i < p->audio_definition_count; ++i)
            if(p->audio_definitions[i].music) ++music; else ++sounds;
        if(sounds) fprintf(f, ",.sounds=project_sound_recipes,.sound_count=%zu", sounds);
        if(music) fprintf(f, ",.music=project_music_recipes,.music_count=%zu", music);
        fputs("}", f);
    }
    for(size_t i = 0; i < d->child_count; ++i) {
        char name[EDITOR_OBJECT_NAME_MAX]; editor_project_property_name_format(name,sizeof(name),d->children[i]->name);
        fprintf(f, ",.%s=",name); value_write(f,c,d->children[i]);
    }
    if(c->project && !d->parent) { viewport_values_write(f, c->project); named_resources_write(f, c->project); }
    fputs("}", f);
}
static void action_recipe_write(FILE *f, const EditorInputAction *a) {
    fputs("{.name=", f); string_write(f, a->name);
    fprintf(f, ",.type=%u,.button_mode=%u,.button_initial_state=%u,.bindings_default={.count=%zu",
        a->type, a->button_mode, a->button_initial_state, a->binding_count);
    if(a->binding_count) {
        fputs(",.values={", f);
        for(size_t b = 0; b < a->binding_count; ++b) {
            editor_catalog_input_binding_write(f, &a->bindings[b]); fputc(',', f);
        }
        fputc('}', f);
    }
    fputs("}}", f);
}
static void controller_recipe_write(FILE *f, const EditorInputController *c, size_t i) {
    fputs("{.controller={.name=", f); string_write(f, c->name);
    fprintf(f, ",.enabled=%u}", c->enabled);
    if(c->action_count) fprintf(f, ",.actions=project_actions_%zu,.action_count=%zu", i, c->action_count);
    fputc('}', f);
}
static void audio_recipe_write(FILE *f, const EditorAudioDefinition *a) {
    fputs("{.name=", f); string_write(f, a->name);
    fputs(",.path=", f); string_write(f, a->path);
    fprintf(f, ",.volume=%.9ef,.loop=%u,.playback_rate=%.9ef", a->volume, a->loop, a->playback_rate);
    if(!a->music) fprintf(f, ",.pan=%.9ef", a->pan);
    fputc('}', f);
}
static void layer_recipe_write(FILE *f, const EditorGraphicsLayer *layer) {
    fputs("{.name=", f); string_write(f, layer->name);
    fprintf(f, ",.value=%d}", layer->value);
}
static void named_resources_write(FILE *f, const EditorProject *p) {
    for(size_t i = 0; i < p->input_controller_count; ++i) {
        const EditorInputController *c = &p->input_controllers[i];
        char name[EDITOR_OBJECT_NAME_MAX]; editor_project_property_name_format(name, sizeof(name), c->name);
        fprintf(f, ",.%s={._description=", name); controller_recipe_write(f, c, i);
        for(size_t j = 0; j < c->action_count; ++j) {
            editor_project_property_name_format(name, sizeof(name), c->actions[j].name);
            fprintf(f, ",.%s=", name); action_recipe_write(f, &c->actions[j]);
        }
        fputc('}', f);
    }
    for(size_t i = 0; i < p->audio_definition_count; ++i) {
        char name[EDITOR_OBJECT_NAME_MAX]; editor_project_property_name_format(name, sizeof(name), p->audio_definitions[i].name);
        fprintf(f, ",.%s=", name); audio_recipe_write(f, &p->audio_definitions[i]);
    }
    for(size_t i = 0; i < p->graphics_layer_count; ++i) {
        char name[EDITOR_OBJECT_NAME_MAX]; editor_project_property_name_format(name, sizeof(name), p->graphics_layers[i].name);
        fprintf(f, ",.%s=", name); layer_recipe_write(f, &p->graphics_layers[i]);
    }
}
static void project_resources_write(FILE *f, const EditorProject *p) {
    for(size_t i = 0; i < p->input_controller_count; ++i) {
        const EditorInputController *c = &p->input_controllers[i];
        if(!c->action_count) continue;
        fprintf(f, "static const ControllerActionDescription project_actions_%zu[] = {\n", i);
        for(size_t j = 0; j < c->action_count; ++j) { action_recipe_write(f, &c->actions[j]); fputs(",\n", f); }
        fputs("};\n", f);
    }
    if(p->input_controller_count) {
        fputs("static const ProjectControllerDescription project_controller_recipes[] = {\n", f);
        for(size_t i = 0; i < p->input_controller_count; ++i) {
            controller_recipe_write(f, &p->input_controllers[i], i); fputs(",\n", f);
        }
        fputs("};\n", f);
    }
    for(int music = 0; music < 2; ++music) {
        size_t count = 0;
        for(size_t i = 0; i < p->audio_definition_count; ++i) {
            const EditorAudioDefinition *a = &p->audio_definitions[i];
            if(a->music != (bool)music) continue;
            if(!count++) fprintf(f, "static const Project%sDescription project_%s_recipes[] = {\n",
                music ? "Music" : "Sound", music ? "music" : "sound");
            fputs("{.name=", f); string_write(f, a->name);
            fputs(",.description=", f); audio_recipe_write(f, a); fputs("},\n", f);
        }
        if(count) fputs("};\n", f);
    }
}
/* Register exact generated types; never accept arbitrary initial-member casts.
 * Both catalogs may coexist, so each fills its own dispatch extension. */
static void creation_adapters_write(FILE *f, const Catalog *c, bool unified) {
    const char *prefix = unified ? "project" : "object";
    const char *hook = unified ? "PROJECT" : "OBJECT";
    for(size_t i = 0; i < c->count; ++i) {
        const ObjectDescription *d = c->entries[i].d;
        unsigned long long id = d->id;
        fprintf(f, "static inline const ObjectDescription *%s_description_view_%llu_ref_get(\n"
            "        const %sDescriptionView_%llu *value) { return &value->%s; }\n",
            prefix, id, c->prefix, id, unified && !d->parent ? "_project.root" : "_description");
    }
    fprintf(f, "#undef ROHR_INTERNAL_%s_VIEW_TYPES\n#define ROHR_INTERNAL_%s_VIEW_TYPES(apply)", hook, hook);
    for(size_t i = 0; i < c->count; ++i) {
        unsigned long long id = c->entries[i].d->id;
        fprintf(f, " \\\n    apply(%sDescriptionView_%llu, %s_description_view_%llu_ref_get)", c->prefix, id, prefix, id);
    }
    fputs("\n", f);
    if(unified) {
        unsigned long long id = c->entries[0].d->id;
        fprintf(f, "static inline const ProjectDescription *project_description_view_%llu_project_ref_get(\n"
            "        const ProjectDescriptionView_%llu *value) { return &value->_project; }\n"
            "#undef ROHR_INTERNAL_PROJECT_CREATE_TYPES\n#define ROHR_INTERNAL_PROJECT_CREATE_TYPES(apply) \\\n"
            "    apply(ProjectDescriptionView_%llu, project_description_view_%llu_project_ref_get)\n",
            id, id, id, id);
    }
}
static bool catalog_write(FILE *header, FILE *source, const EditorProject *project, bool unified) {
    if(!project) return false;
    ObjectDescriptionResult converted = object_description_project_read(project, "");
    if(error_check(converted)) return false;
    Catalog *catalog = calloc(1,sizeof(*catalog));
    if(catalog) { catalog->prefix = unified ? "Project" : "Object"; catalog->project = unified ? project : NULL; }
    bool ok = catalog && collect(catalog,converted.result.value,unified ? "project" : "objects",0);
    if(ok) {
        if(unified) resource_types_write(header, project);
        type_write(header,catalog,converted.result.value);
        creation_adapters_write(header,catalog,unified);
        fprintf(header,"extern const %sDescriptionView_%llu %s;\n\n",catalog->prefix,
            (unsigned long long)converted.result.value->id,unified ? "project" : "objects");
        for(size_t i = 0; i < catalog->count; ++i) array_write(source,catalog,catalog->entries[i].d);
        if(unified) { project_resources_write(source, project); ok = viewport_arrays_write(source,catalog); }
        fprintf(source,"const %sDescriptionView_%llu %s = ",catalog->prefix,
            (unsigned long long)converted.result.value->id,unified ? "project" : "objects");
        value_write(source,catalog,converted.result.value); fputs(";\n",source);
        ok = ok && !ferror(header) && !ferror(source);
    }
    if(catalog) for(size_t i = 0; i < catalog->count; ++i) free(catalog->entries[i].path);
    free(catalog); object_description_tree_destroy((ObjectDescription *)converted.result.value);
    return ok;
}
bool editor_object_catalog_write(FILE *header, FILE *source, const EditorProject *project) {
    return catalog_write(header, source, project, false);
}
bool editor_project_catalog_write(FILE *header, FILE *source, const EditorProject *project) {
    if(editor_result_check(editor_project_catalog_validate(project))) return false;
    fputs("/* Generated by Rohr Engine.\n"
        " * This generated file is not covered by Rohr Engine's LGPL-3.0-only license.\n"
        " * The project's developer may use, modify, and distribute it without restriction.\n"
        " */\n#ifndef ROHR_GENERATED_PROJECT_H\n#define ROHR_GENERATED_PROJECT_H\n"
        "#include \"project_objects.h\"\n", header);
    fputs("\n#include \"project.h\"\n", source);
    bool ok = catalog_write(header,source,project,true);
    fputs("#endif\n",header);
    return ok && !ferror(header) && !ferror(source);
}

static void color_write(FILE *f, uint32_t color) {
    fprintf(f,"{%u,%u,%u,%u}",color>>24,(color>>16)&255,(color>>8)&255,color&255);
}
static bool layer_write(FILE *f, const EditorProject *p, EditorGraphicsLayerId id) {
    if(!id) return true;
    for(size_t i=0;i<p->graphics_layer_count;++i) if(p->graphics_layers[i].id==id) {
        fputs(",.layer_name=",f); string_write(f,p->graphics_layers[i].name); return true;
    }
    return false;
}
static void placement_write(FILE *f, ViewportItemDescription v) {
    fprintf(f,".placement={.rectangle={%.9ef,%.9ef,%.9ef,%.9ef},.fit=%u,.orientation=%.9ef,"
        ".content_offset={%.9ef,%.9ef},.content_scale={%.9ef,%.9ef},.content_orientation=%.9ef,"
        ".layer=%d,.visible=%u,.drag_mode=%u,.clip_enabled=%u,.clip_rectangle={%.9ef,%.9ef,%.9ef,%.9ef}}",
        (double)v.rectangle.x,(double)v.rectangle.y,(double)v.rectangle.width,(double)v.rectangle.height,
        v.fit,(double)v.orientation,(double)v.content_offset.x,(double)v.content_offset.y,
        (double)v.content_scale.x,(double)v.content_scale.y,(double)v.content_orientation,
        v.layer,v.visible,v.drag_mode,v.clip_enabled,(double)v.clip_rectangle.x,(double)v.clip_rectangle.y,
        (double)v.clip_rectangle.width,(double)v.clip_rectangle.height);
}
static void text_layout_write(FILE *f, const EditorViewportUiText *t, bool standalone) {
    fprintf(f,"{.position={%.9ef,%.9ef},.offset={%.9ef,%.9ef},.scale={%.9ef,%.9ef},.orientation=%.9ef}",
        standalone ? (double)t->box_width*.5 : 0, standalone ? (double)t->box_height*.5 : 0,
        (double)t->offset.x,(double)t->offset.y,(double)t->width_scale*2,(double)t->height_scale*2,(double)t->rotation);
}
static bool ui_recipe_write(FILE *f, const EditorProject *p, const EditorViewportUiItem *item) {
    const EditorViewportUiDefinition *d=editor_project_ui_definition_get((EditorProject *)p,item->definition);
    if(!d) return false;
    fprintf(f,"{.name="); string_write(f,item->name);
    fprintf(f,",.kind=%s,",d->kind==EDITOR_VIEWPORT_UI_SHAPE ? "VIEWPORT_CREATE_SHAPE" :
        d->kind==EDITOR_VIEWPORT_UI_TEXT ? "VIEWPORT_CREATE_TEXT" : "VIEWPORT_CREATE_SLIDER");
    ViewportItemDescription placement=graphics_viewport_item_description_default_get();
    placement.fit=SCREEN_FIT_NONE;
    placement.rectangle=(ViewportRectangle){item->position.x,item->position.y,0,0};
    placement.content_scale=item->scale; placement.orientation=item->rotation;
    placement.layer=item->layer; placement.visible=!item->app_hidden; placement.drag_mode=item->drag_mode;
    placement.clip_enabled=item->clip_enabled; placement.clip_rectangle=item->clip_rectangle;
    placement_write(f,placement);
    if(!layer_write(f,p,item->graphics_layer)) return false;
    const EditorViewportUiText *t=d->kind==EDITOR_VIEWPORT_UI_SHAPE ? &d->value.shape.text :
        d->kind==EDITOR_VIEWPORT_UI_TEXT ? &d->value.text : NULL;
    if(t && t->text[0]) {
        fputs(",.text_value=",f); string_write(f,t->text);
        fputs(",.text_color=",f); color_write(f,t->color);
        fprintf(f,",.text_wrap_width=%d",t->wrap_width);
        if(t->font) {
            const EditorUiFont *font=NULL;
            for(size_t i=0;i<p->ui_font_count;++i) if(p->ui_fonts[i].id==t->font) font=&p->ui_fonts[i];
            if(!font) return false;
            fputs(",.font={.file=",f); string_write(f,font->path);
            fprintf(f,",.point_size=%.9ef}",(double)font->point_size);
        }
    }
    if(d->kind==EDITOR_VIEWPORT_UI_SHAPE) {
        const EditorViewportUiShape *s=&d->value.shape;
        fprintf(f,",.shape={.shape={.amount_of_vertices=%zu,.vertices={",s->vertex_count);
        for(size_t i=0;i<s->vertex_count;++i)
            fprintf(f,"%s{%.9ef,%.9ef}",i?",":"",(double)s->vertices[i].x,(double)s->vertices[i].y);
        fprintf(f,"}},.orientation=%.9ef,.button_enabled=%u,.border_enabled=%u,.border_type=%u,"
            ".border_thickness=%.9ef,.border_hash_spacing=%.9ef,.border_corner_radius=%.9ef",
            (double)s->rotation,s->button_enabled,d->border_enabled,d->border_type,
            (double)d->border_thickness,(double)d->border_hash_spacing,(double)d->border_corner_radius);
#define COLOR(field) fputs(",." #field "=",f); color_write(f,d->field)
        COLOR(border_color); COLOR(fill_color); COLOR(hover_border_color); COLOR(hover_fill_color);
        COLOR(click_border_color); COLOR(click_fill_color);
#undef COLOR
        fputs(",.text=",f); text_layout_write(f,t,false); fputc('}',f);
    } else if(d->kind==EDITOR_VIEWPORT_UI_TEXT) {
        fputs(",.text=",f); text_layout_write(f,t,true);
    } else {
        const EditorViewportUiSlider *s=&d->value.slider;
        fputs(",.slider={",f);
#define FLOAT(field) fprintf(f,"." #field "=%.9ef,",(double)s->field)
        FLOAT(minimum); FLOAT(maximum); FLOAT(value); FLOAT(step); FLOAT(length); FLOAT(track_thickness);
        FLOAT(thumb_width); FLOAT(thumb_height); FLOAT(thumb_radius); FLOAT(thumb_offset);
#undef FLOAT
        fprintf(f,".thumb_shape=%u,.enabled=%u",s->thumb_shape,s->enabled);
#define COLOR(field) fputs(",." #field "=",f); color_write(f,s->field)
        COLOR(track_color); COLOR(filled_track_color); COLOR(thumb_color); COLOR(hover_thumb_color); COLOR(pressed_thumb_color);
#undef COLOR
        fputc('}',f);
    }
    fputc('}',f); return true;
}
static bool viewport_item_write(FILE *f, const EditorProject *p, const EditorLayoutViewport *view, size_t i) {
    if(i >= view->camera_item_count) return ui_recipe_write(f, p, &view->ui_items[i-view->camera_item_count]);
    const EditorViewportCameraItem *item=&view->camera_items[i];
    const EditorObject *object=NULL; const EditorCamera *camera=NULL;
    for(size_t o=0;o<p->object_count;++o) if(p->objects[o].id==item->object) object=&p->objects[o];
    if(object) camera=editor_project_camera_get((EditorObject *)object,item->camera);
    if(!object || !camera) return false;
    char object_name[EDITOR_OBJECT_NAME_MAX], camera_name[EDITOR_OBJECT_NAME_MAX];
    editor_project_property_name_format(object_name,sizeof(object_name),object->name);
    editor_project_property_name_format(camera_name,sizeof(camera_name),camera->name);
    fputs("{.name=",f); string_write(f,item->name);
    fprintf(f,",.kind=VIEWPORT_CREATE_SCREEN,.camera_object=&project.%s._description,"
        ".camera_member=&project.%s.%s._description,.render_width=%d,.render_height=%d,.background_color=",
        object_name,object_name,camera_name,item->render_size_explicit ? item->render_width : (int)camera->dimensions.x,
        item->render_size_explicit ? item->render_height : (int)camera->dimensions.y);
    color_write(f,item->background_color); fputc(',',f);
    ViewportItemDescription placement=item->placement;
    placement.content_offset=item->content_offset; placement.content_scale=item->content_scale;
    placement.content_orientation=item->content_rotation; placement.visible=!item->app_hidden;
    placement_write(f,placement);
    if(!layer_write(f,p,item->graphics_layer)) return false;
    fputc('}',f);
    return !ferror(f);
}
static bool viewport_arrays_write(FILE *f, const Catalog *c) {
    const EditorProject *p=c->project;
    if(p->layout_viewport_count) {
        fputs("static const ViewportCreateDescription *const project_viewports[] = {", f);
        for(size_t i=0;i<p->layout_viewport_count;++i) {
            char name[EDITOR_OBJECT_NAME_MAX];
            editor_project_property_name_format(name, sizeof(name), p->layout_viewports[i].name);
            fprintf(f, "%s&project.%s._description", i ? "," : "", name);
        }
        fputs("};\nstatic const ProjectViewportDescription project_viewport_recipes[] = {\n", f);
        for(size_t i = 0; i < p->layout_viewport_count; ++i) {
            char name[EDITOR_OBJECT_NAME_MAX], authored[EDITOR_OBJECT_NAME_MAX];
            editor_project_property_name_format(name, sizeof(name), p->layout_viewports[i].name);
            /* Match the editor's persisted viewport naming before and after reload. */
            editor_project_object_name_format(authored, sizeof(authored), p->layout_viewports[i].name);
            fputs("{.name=", f); string_write(f, authored);
            fprintf(f, ",.description=&project.%s._description},\n", name);
        }
        fputs("};\n", f);
    }
    if(p->graphics_layer_count && p->layout_viewport_count) {
        fputs("static const LayerDescription project_layers[] = {\n",f);
        for(size_t i=0;i<p->graphics_layer_count;++i) {
            layer_recipe_write(f, &p->graphics_layers[i]); fputs(",\n", f);
        }
        fputs("};\n",f);
    }
    for(size_t v=0;v<p->layout_viewport_count;++v) {
        const EditorLayoutViewport *view=&p->layout_viewports[v];
        if(!view->camera_item_count && !view->ui_item_count) continue;
        fprintf(f,"static const ViewportCreateItemDescription project_viewport_%zu_items[] = {\n",v);
        for(size_t i=0;i<view->camera_item_count+view->ui_item_count;++i) {
            if(!viewport_item_write(f,p,view,i)) return false;
            fputs(",\n",f);
        }
        fputs("};\n",f);
    }
    return !ferror(f);
}
static void viewport_values_write(FILE *f, const EditorProject *p) {
    if(p->layout_viewport_count) fprintf(f, ",.viewports=project_viewports,.viewport_count=%zu", p->layout_viewport_count);
    for(size_t v=0;v<p->layout_viewport_count;++v) {
        const EditorLayoutViewport *view=&p->layout_viewports[v];
        char name[EDITOR_OBJECT_NAME_MAX]; editor_project_property_name_format(name,sizeof(name),view->name);
        fprintf(f,",.%s={._description={.viewport={.rectangle={%.9ef,%.9ef,%.9ef,%.9ef},.fit=%u,.enabled=%u,.background_color=",
            name,(double)view->config.rectangle.x,(double)view->config.rectangle.y,
            (double)view->config.rectangle.width,(double)view->config.rectangle.height,view->config.fit,!view->app_hidden);
        color_write(f,view->background_color);
        char authored[EDITOR_OBJECT_NAME_MAX];
        editor_project_object_name_format(authored, sizeof(authored), view->name);
        fputs(",.name=", f); string_write(f, authored); fputc('}',f);
        size_t count=view->camera_item_count+view->ui_item_count;
        if(count) fprintf(f,",.items=project_viewport_%zu_items,.item_count=%zu",v,count);
        if(p->graphics_layer_count) fprintf(f,",.layers=project_layers,.layer_count=%zu",p->graphics_layer_count);
        fputc('}',f);
        for(size_t i=0;i<count;++i) {
            char item[EDITOR_OBJECT_NAME_MAX];
            editor_project_property_name_format(item,sizeof(item),viewport_item_name_get(view,i));
            fprintf(f,",.%s=",item);
            (void)viewport_item_write(f,p,view,i); /* Arrays validated these same recipes first. */
        }
        fputc('}',f);
    }
}

static const char *project_resource_name_get(const EditorProject *p, size_t i) {
    if(i < p->object_count) return p->objects[i].name;
    i -= p->object_count;
    if(i < p->layout_viewport_count) return p->layout_viewports[i].name;
    i -= p->layout_viewport_count;
    if(i < p->input_controller_count) return p->input_controllers[i].name;
    i -= p->input_controller_count;
    if(i < p->audio_definition_count) return p->audio_definitions[i].name;
    return p->graphics_layers[i-p->audio_definition_count].name;
}
static bool viewport_field_reserved_check(const char *name) {
    const char *fields[] = {"_description", "template", "viewport", "items", "item_count",
        "layers", "layer_count", "objects", "object_count", "cameras", "camera_count"};
    for(size_t i=0;i<sizeof(fields)/sizeof(fields[0]);++i) if(!strcmp(name,fields[i])) return true;
    return false;
}
EditorResult editor_project_catalog_validate(const EditorProject *p) {
    if(!p) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,"Missing project catalog");
    size_t total=p->object_count+p->layout_viewport_count+p->input_controller_count+
        p->audio_definition_count+p->graphics_layer_count;
    for(size_t i=0;i<total;++i) {
        const char *original=project_resource_name_get(p,i);
        char name[EDITOR_OBJECT_NAME_MAX]; editor_project_property_name_format(name,sizeof(name),original);
        if(reserved(name)) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
            "Project item '%s' generates reserved description field '%s'",original,name);
        for(size_t j=0;j<i;++j) {
            char other[EDITOR_OBJECT_NAME_MAX];
            editor_project_property_name_format(other,sizeof(other),project_resource_name_get(p,j));
            if(!strcmp(name,other)) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "Project items '%s' and '%s' both generate field '%s'",project_resource_name_get(p,j),original,name);
        }
    }
    for(size_t c=0;c<p->input_controller_count;++c) {
        const EditorInputController *controller=&p->input_controllers[c];
        for(size_t i=0;i<controller->action_count;++i) {
            char name[EDITOR_OBJECT_NAME_MAX];
            editor_project_property_name_format(name,sizeof(name),controller->actions[i].name);
            if(!strcmp(name,"_description") || !strcmp(name,"controller") ||
                    !strcmp(name,"actions") || !strcmp(name,"action_count"))
                return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                    "Controller '%s' action '%s' generates reserved field '%s'",controller->name,controller->actions[i].name,name);
            for(size_t j=0;j<i;++j) {
                char prior[EDITOR_OBJECT_NAME_MAX];
                editor_project_property_name_format(prior,sizeof(prior),controller->actions[j].name);
                if(!strcmp(name,prior)) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                    "Controller '%s' actions '%s' and '%s' both generate field '%s'",
                    controller->name,controller->actions[j].name,controller->actions[i].name,name);
            }
        }
    }
    for(size_t v=0;v<p->layout_viewport_count;++v) {
        const EditorLayoutViewport *view=&p->layout_viewports[v];
        size_t count=view->camera_item_count+view->ui_item_count;
        if(count>MAX_VIEWPORT_ITEMS) return editor_result_error(EDITOR_ERROR_CAPACITY,
            "Viewport '%s' exceeds the runtime item capacity",view->name);
        for(size_t i=0;i<count;++i) {
            const char *name=viewport_item_name_get(view,i);
            char field[EDITOR_OBJECT_NAME_MAX]; editor_project_property_name_format(field,sizeof(field),name);
            if(viewport_field_reserved_check(field)) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "Viewport '%s' item '%s' generates reserved field '%s'",view->name,name,field);
            if(!*name || strlen(name)>=ENTITY_NAME_MAX) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                "Viewport '%s' has an empty or oversized item name",view->name);
            for(size_t j=0;j<i;++j) {
                const char *other=viewport_item_name_get(view,j);
                char prior[EDITOR_OBJECT_NAME_MAX]; editor_project_property_name_format(prior,sizeof(prior),other);
                if(!strcmp(field,prior)) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
                    "Viewport '%s' items '%s' and '%s' both generate field '%s'",view->name,other,name,field);
            }
        }
    }
    ObjectDescriptionResult converted = object_description_project_read(p, "");
    if(error_check(converted)) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "Cannot convert project descriptions");
    Catalog *catalog = calloc(1, sizeof(*catalog));
    bool valid = catalog && collect(catalog, converted.result.value, "project", 0);
    if(catalog) for(size_t i=0;i<catalog->count;++i) free(catalog->entries[i].path);
    free(catalog);
    object_description_tree_destroy((ObjectDescription *)converted.result.value);
    if(!valid) return editor_result_error(EDITOR_ERROR_INVALID_ARGUMENT,
        "Project descriptions contain invalid or conflicting generated member names");
    return editor_result_value(true);
}
