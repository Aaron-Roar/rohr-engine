/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "editor_diagnostics.h"
#include "editor_soft_area.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool same(EditorSelectionRef a, EditorSelectionRef b) {
    return a.kind == b.kind && a.object == b.object && a.parent == b.parent &&
        a.container == b.container && a.item == b.item;
}
bool editor_diagnostic_target_check(EditorSelectionRef target, EditorSelectionRef item,
        bool descendants) {
    if(same(target, item)) return true;
    if(!descendants) return false;
    if(item.kind == EDITOR_SELECTION_LAYOUT_VIEWPORT && !item.parent)
        return target.object == item.item && ((target.kind == EDITOR_SELECTION_LAYOUT_VIEWPORT && target.parent) ||
            target.kind == EDITOR_SELECTION_UI_TEXT || target.kind == EDITOR_SELECTION_UI_SHAPE || target.kind == EDITOR_SELECTION_UI_SLIDER);
    if(target.object != item.object) return false;
    if(item.kind == EDITOR_SELECTION_OBJECT && item.object)
        return target.kind != EDITOR_SELECTION_LAYOUT_VIEWPORT && target.kind != EDITOR_SELECTION_UI_TEXT &&
            target.kind != EDITOR_SELECTION_UI_SHAPE && target.kind != EDITOR_SELECTION_UI_SLIDER &&
            target.kind != EDITOR_SELECTION_UI_VERTEX && target.kind != EDITOR_SELECTION_UI_LINE &&
            target.kind != EDITOR_SELECTION_NONE;
    if(item.kind == EDITOR_SELECTION_SOFT_BODY)
        return target.parent == item.item && (target.kind == EDITOR_SELECTION_SOFT_NODE ||
            target.kind == EDITOR_SELECTION_SOFT_BEAM || target.kind == EDITOR_SELECTION_SOFT_AREA ||
            target.kind == EDITOR_SELECTION_SOFT_HOLE);
    if(item.kind == EDITOR_SELECTION_SOFT_AREA)
        return target.kind == EDITOR_SELECTION_SOFT_HOLE && target.parent == item.parent && target.container == item.item;
    if(item.kind == EDITOR_SELECTION_RIGID_BODY)
        return target.parent == item.item && (target.kind == EDITOR_SELECTION_HITBOX ||
            target.kind == EDITOR_SELECTION_VERTEX || target.kind == EDITOR_SELECTION_LINE);
    if(item.kind == EDITOR_SELECTION_HITBOX)
        return target.parent == item.parent && target.container == item.item &&
            (target.kind == EDITOR_SELECTION_VERTEX || target.kind == EDITOR_SELECTION_LINE);
    if(item.kind == EDITOR_SELECTION_ANIMATED_SPRITE)
        return target.kind == EDITOR_SELECTION_ANIMATION_FRAME && target.parent == item.item;
    return false;
}
const EditorDiagnostic *editor_diagnostics_item_get(const EditorDiagnostics *d,
        EditorSelectionRef item, bool descendants) {
    if(d) for(size_t i = 0; i < d->count; ++i)
        if(editor_diagnostic_target_check(d->items[i].target, item, descendants)) return &d->items[i];
    return NULL;
}
bool editor_diagnostics_field_check(const EditorDiagnostics *d, EditorSelectionRef item,
        const char *control) {
    if(!d || !control) return false;
    const char *tail = strrchr(control, '.'); tail = tail ? tail + 1 : control;
    for(size_t i = 0; i < d->count; ++i) {
        const EditorDiagnostic *error = &d->items[i];
        if(!same(error->target, item)) continue;
        const char *part = error->fields;
        while(*part) {
            const char *end = strchr(part, ','); size_t n = end ? (size_t)(end-part) : strlen(part);
            if((strlen(control) == n && !strncmp(control, part, n)) ||
                    (strlen(tail) == n && !strncmp(tail, part, n))) return true;
            if(!end) break;
            part = end + 1;
        }
    }
    return false;
}
bool editor_diagnostics_contains_check(const EditorDiagnostics *d, const EditorDiagnostic *error) {
    if(!d || !error) return false;
    for(size_t i = 0; i < d->count; ++i)
        if(same(d->items[i].target, error->target) && !strcmp(d->items[i].fields, error->fields) &&
                !strcmp(d->items[i].reason, error->reason)) return true;
    return false;
}
bool editor_diagnostics_section_check(const EditorDiagnostics *d, EditorSelectionRef item,
        const char *section) {
    if(!d || !section) return false;
    if(!strcmp(section, "editor.mode.properties")) return editor_diagnostics_item_get(d, item, true) != NULL;
    const char *tail = strrchr(section, '.'); tail = tail ? tail + 1 : section;
    const char *fields = NULL;
    if(!strncmp(section, "editor.entity_settings.section.", sizeof("editor.entity_settings.section.") - 1)) {
        static const char *groups[] = {"lifetime-time", "angular-maximum", "gameplay-target",
            "axis-angle,axis-point-x,axis-point-y", "transform-driver"};
        if(tail[0] >= '0' && tail[0] <= '4' && !tail[1]) fields = groups[tail[0]-'0'];
    } else if(!strcmp(tail, "transform") || !strcmp(tail, "placement")) fields = "x,y,rotation,rotation_lock";
    else if(!strcmp(tail, "physics")) fields = "mass,radius,rotation_lock";
    else if(!strcmp(tail, "material")) fields = "friction,restitution,stiffness,damping";
    else if(!strcmp(tail, "geometry")) fields = "radius";
    else if(!strcmp(tail, "parenting")) fields = "parent";
    else if(!strcmp(tail, "attachment")) fields = "body,soft_node";
    else if(!strcmp(tail, "asset")) fields = "path";
    else if(!strcmp(tail, "configuration")) fields = "kind";
    else if(!strcmp(tail, "connections")) fields = "anchor_a,anchor_b,node_a,node_b";
    else if(!strcmp(tail, "parameters")) fields = "rest_angle,rest_length,rest_locked,stiffness,damping";
    else if(!strcmp(tail, "view")) fields = "width,height,zoom";
    else if(!strcmp(tail, "source")) fields = "source,Target,Mode,Magnitude";
    else if(!strcmp(tail, "application") || !strcmp(tail, "direction")) fields = "Reference,Kind";
    else if(!strcmp(tail, "content")) fields = "font";
    if(fields) {
        while(*fields) {
            const char *end = strchr(fields, ','); size_t n = end ? (size_t)(end-fields) : strlen(fields);
            char field[64];
            if(n < sizeof(field)) {
                memcpy(field, fields, n); field[n] = 0;
                if(editor_diagnostics_field_check(d, item, field)) return true;
            }
            if(!end) break;
            fields = end+1;
        }
    }
    /* These accordions contain child rows rather than scalar controls. */
    if((item.kind == EDITOR_SELECTION_ANIMATED_SPRITE && !strcmp(tail, "frames")) ||
            (item.kind == EDITOR_SELECTION_RIGID_BODY && !strcmp(tail, "geometry")) ||
            (item.kind == EDITOR_SELECTION_SOFT_BODY && !strcmp(tail, "topology")))
        return editor_diagnostics_item_get(d, item, true) != NULL;
    if(item.kind == EDITOR_SELECTION_SOFT_BODY && !strcmp(tail, "physics")) {
        for(size_t i = 0; i < d->count; ++i) {
            const EditorSelectionRef child = d->items[i].target;
            if(child.kind == EDITOR_SELECTION_SOFT_NODE && editor_diagnostic_target_check(child, item, true) &&
                    (editor_diagnostics_field_check(d, child, "friction") ||
                     editor_diagnostics_field_check(d, child, "restitution"))) return true;
        }
    }
    return false;
}

static void add(EditorDiagnostics *d, EditorSelectionRef target, const char *fields, const char *reason) {
    if(d->count == d->capacity) {
        size_t capacity = d->capacity ? d->capacity * 2 : 32;
        if(capacity > SIZE_MAX / sizeof(*d->items)) { d->allocation_failed = true; return; }
        void *items = realloc(d->items, capacity * sizeof(*d->items));
        if(!items) { d->allocation_failed = true; return; }
        d->items = items; d->capacity = capacity;
    }
    EditorDiagnostic *item = &d->items[d->count++];
    *item = (EditorDiagnostic){.target = target};
    snprintf(item->fields, sizeof(item->fields), "%s", fields);
    snprintf(item->reason, sizeof(item->reason), "%s", reason);
}
void editor_diagnostics_destroy(EditorDiagnostics *d) {
    if(d) { free(d->items); *d = (EditorDiagnostics){0}; }
}
static EditorSelectionRef ref(EditorHierarchySelection kind, uint32_t object, uint32_t parent, uint32_t item) {
    return (EditorSelectionRef){.kind = kind, .object = object, .parent = parent, .item = item};
}
EditorSelectionRef editor_diagnostic_entity_get(EditorObject *object, ObjectEntityReference r) {
    if(!object || !r.id) return (EditorSelectionRef){0};
    EditorHierarchySelection kind = EDITOR_SELECTION_NONE;
    switch(r.kind) {
        case OBJECT_CONTAINER: kind = EDITOR_SELECTION_OBJECT; break;
        case OBJECT_BODY: case OBJECT_PARTICLE: {
            EditorRigidBody *body = editor_project_rigid_body_get(object, r.id);
            kind = body && body->standalone_particle ? EDITOR_SELECTION_PARTICLE : EDITOR_SELECTION_RIGID_BODY;
            break;
        }
        case OBJECT_SOFT_BODY: kind = EDITOR_SELECTION_SOFT_BODY; break;
        case OBJECT_NODE: kind = EDITOR_SELECTION_SOFT_NODE; break;
        case OBJECT_BEAM: kind = EDITOR_SELECTION_SOFT_BEAM; break;
        case OBJECT_ANCHOR: kind = EDITOR_SELECTION_ANCHOR; break;
        case OBJECT_JOINT: kind = EDITOR_SELECTION_JOINT; break;
        case OBJECT_CAMERA: kind = EDITOR_SELECTION_CAMERA; break;
        case OBJECT_SPRITE: kind = EDITOR_SELECTION_SPRITE; break;
        case OBJECT_ANIMATION: kind = EDITOR_SELECTION_ANIMATED_SPRITE; break;
        case OBJECT_FORCE: kind = EDITOR_SELECTION_FORCE; break;
        case OBJECT_TORQUE: kind = EDITOR_SELECTION_TORQUE; break;
        default: break;
    }
    return ref(kind, object->id, r.soft_body, r.id);
}
static void number(EditorDiagnostics *d, EditorSelectionRef r, const char *field,
        double value, double minimum, bool strict) {
    if(!isfinite(value) || value < minimum || (strict && value == minimum)) {
        char reason[128];
        snprintf(reason, sizeof(reason), "%s must be finite and %s %.6g", field,
            strict ? "greater than" : "at least", minimum);
        add(d, r, field, reason);
    }
}
static void position(EditorDiagnostics *d, EditorSelectionRef r, Position p) {
    if(!rohr_physics_world_position_check(p)) add(d, r, "x,y", "Position exceeds the supported world range");
}
static void settings(EditorDiagnostics *d, EditorObject *o, EditorSelectionRef r,
        const ObjectEntitySettings *s) {
    if(s->transform_driver.id && !editor_project_entity_settings_get(o, s->transform_driver))
        add(d, r, "transform-driver", "Transform driver no longer exists");
    if(s->gameplay_target.id && !editor_project_entity_settings_get(o, s->gameplay_target))
        add(d, r, "gameplay-target", "Gameplay target no longer exists");
    if(s->axis_lock_enabled && (!isfinite(s->axis_lock.axis.x) || !isfinite(s->axis_lock.axis.y) ||
            hypot(s->axis_lock.axis.x, s->axis_lock.axis.y) == 0))
        add(d, r, "axis-angle", "Axis direction must be finite and nonzero");
    if(s->axis_lock_enabled && !rohr_physics_world_position_check(s->axis_lock.point_on_axis))
        add(d, r, "axis-point-x,axis-point-y", "Axis shift exceeds the supported world range");
    number(d, r, "lifetime-time", s->lifetime.deadline.expiry_time, 0, false);
    number(d, r, "angular-maximum", s->angular_velocity_maximum, 0, false);
}
static void material(EditorDiagnostics *d, EditorSelectionRef r, float friction, float restitution) {
    number(d, r, "friction", friction, 0, false);
    number(d, r, "restitution", restitution, 0, false);
}
static bool asset_check(const EditorWorkspace *w, const char *path) {
    if(!w || !w->open) return true;
    char resolved[EDITOR_WORKSPACE_PATH_MAX + EDITOR_ASSET_PATH_MAX];
    SDL_PathInfo info;
    return path[0] && !editor_result_check(editor_workspace_asset_path_get(w, path, resolved, sizeof(resolved))) &&
        SDL_GetPathInfo(resolved, &info) && info.type == SDL_PATHTYPE_FILE;
}
static void asset(EditorDiagnostics *d, const EditorWorkspace *w, EditorSelectionRef r, const char *path) {
    if(!asset_check(w, path)) {
        char reason[256]; snprintf(reason, sizeof(reason), "Asset is missing or unavailable: %.200s", path);
        add(d, r, "path", reason);
    }
}
static void joint_related(EditorDiagnostics *d, EditorObject *o, const EditorAnchor *a, const char *reason) {
    if(!a) return;
    add(d, ref(EDITOR_SELECTION_ANCHOR, o->id, 0, a->id), "x,y,rotation,body,soft_node", reason);
    if(!a->position_follows_body) return;
    if(a->attachment_kind == EDITOR_ANCHOR_ATTACHMENT_SOFT_NODE)
        add(d, ref(EDITOR_SELECTION_SOFT_NODE, o->id, a->attachment_soft_body, a->attachment_soft_node),
            "x,y,axis-angle,axis-point-x,axis-point-y", reason);
    else if(a->rigid_body) {
        EditorSelectionRef r = editor_diagnostic_entity_get(o, (ObjectEntityReference){OBJECT_BODY, a->rigid_body, 0});
        add(d, r, "x,y,rotation,rotation_lock,axis-angle,axis-point-x,axis-point-y", reason);
    }
}
void editor_diagnostics_refresh(EditorDiagnostics *d, EditorProject *p, const EditorWorkspace *w) {
    if(!d) return;
    d->count = 0; d->allocation_failed = false;
    if(!p) return;
    for(size_t oi = 0; oi < p->object_count; ++oi) {
        EditorObject *o = &p->objects[oi];
        EditorSelectionRef r = ref(EDITOR_SELECTION_OBJECT, o->id, 0, o->id);
        settings(d, o, r, &o->entity_settings);
        for(size_t i = 0; i < o->rigid_body_count; ++i) {
            EditorRigidBody *b = &o->rigid_bodies[i];
            r = ref(b->standalone_particle ? EDITOR_SELECTION_PARTICLE : EDITOR_SELECTION_RIGID_BODY, o->id, 0, b->id);
            position(d, r, b->position); number(d, r, "mass", b->mass_value, 0, false);
            material(d, r, b->friction, b->restitution); settings(d, o, r, &b->entity_settings);
            if(b->particle) number(d, r, "radius", b->particle_radius, 0, true);
            if(b->parent && !editor_project_rigid_body_get(o, b->parent)) add(d, r, "parent", "Parent body no longer exists");
        }
        for(size_t i = 0; i < o->joint_count; ++i) {
            EditorJoint *j = &o->joint_items[i]; r = ref(EDITOR_SELECTION_JOINT, o->id, 0, j->id);
            settings(d, o, r, &j->entity_settings);
            number(d, r, "rest_length", j->rest_length, 0, false);
            number(d, r, "stiffness", j->stiffness, 0, false); number(d, r, "damping", j->damping, 0, false);
            EditorAnchor *a = editor_project_anchor_get(o, j->anchor_a), *b = editor_project_anchor_get(o, j->anchor_b);
            if(!a) add(d, r, "anchor_a", "Anchor A is unset or missing; joint does not respond");
            if(!b) add(d, r, "anchor_b", "Anchor B is unset or missing; joint does not respond");
            if(a && b) {
                EditorJointPlacementResult status = editor_project_joint_status_get(o, j);
                if(status.blocked) {
                    add(d, r, "anchor_a,anchor_b,kind,rest_angle,rest_length,rest_locked", status.reason);
                    char reason[256]; snprintf(reason, sizeof(reason), "Joint '%s' (%u): %.160s", j->name, j->id, status.reason);
                    joint_related(d, o, a, reason); joint_related(d, o, b, reason);
                }
            }
        }
        for(size_t i = 0; i < o->anchor_count; ++i) {
            EditorAnchor *a = &o->anchors[i]; r = ref(EDITOR_SELECTION_ANCHOR, o->id, 0, a->id);
            settings(d, o, r, &a->entity_settings); position(d, r, a->position);
            if(a->attachment_kind == EDITOR_ANCHOR_ATTACHMENT_RIGID_BODY && a->rigid_body &&
                    !editor_project_rigid_body_get(o, a->rigid_body)) add(d, r, "body", "Attached body no longer exists");
            if(a->attachment_kind == EDITOR_ANCHOR_ATTACHMENT_SOFT_NODE &&
                    !editor_source_reference_check(o, (ObjectSourceReference){OBJECT_SOURCE_NODE, a->attachment_soft_node, a->attachment_soft_body}))
                add(d, r, "soft_node", "Attached soft node no longer exists");
        }
        for(size_t bi = 0; bi < o->soft_body_count; ++bi) {
            EditorSoftBody *b = &o->soft_body_items[bi]; r = ref(EDITOR_SELECTION_SOFT_BODY, o->id, 0, b->id);
            settings(d, o, r, &b->entity_settings); position(d, r, b->position);
            for(size_t i = 0; i < b->node_count; ++i) {
                EditorSoftNode *n = &b->nodes[i]; r = ref(EDITOR_SELECTION_SOFT_NODE, o->id, b->id, n->id);
                settings(d, o, r, &n->entity_settings); position(d, r, n->position);
                number(d, r, "mass", n->node_mass, 0, true); number(d, r, "radius", n->radius, 0, true);
                material(d, r, n->friction, n->restitution);
            }
            for(size_t i = 0; i < b->beam_count; ++i) {
                EditorSoftBeam *v = &b->beams[i]; r = ref(EDITOR_SELECTION_SOFT_BEAM, o->id, b->id, v->id);
                settings(d, o, r, &v->entity_settings); material(d, r, v->friction, v->restitution);
                if(!editor_source_reference_check(o, (ObjectSourceReference){OBJECT_SOURCE_NODE, v->node_a, b->id}))
                    add(d, r, "node_a", "Node A is unset or missing");
                if(!editor_source_reference_check(o, (ObjectSourceReference){OBJECT_SOURCE_NODE, v->node_b, b->id}))
                    add(d, r, "node_b", "Node B is unset or missing");
                number(d, r, "stiffness", v->stiffness, 0, false); number(d, r, "damping", v->damping, 0, false);
            }
            for(size_t i = 0; i < b->area_count; ++i) {
                EditorSoftArea *a = &b->areas[i]; r = ref(EDITOR_SELECTION_SOFT_AREA, o->id, b->id, a->id);
                settings(d, o, r, &a->entity_settings);
                EditorSoftAreaFill fill = editor_soft_area_fill_get(b, a);
                if(!fill.complete) {
                    if(fill.incomplete_loop && fill.incomplete_loop <= a->hole_count) {
                        r.kind = EDITOR_SELECTION_SOFT_HOLE; r.container = a->id;
                        r.item = a->holes[fill.incomplete_loop-1].id;
                    }
                    add(d, r, "", "Incomplete or invalid loop; select at least three distinct nodes enclosing a region");
                }
            }
        }
        for(size_t i = 0; i < o->sprite_count; ++i) {
            EditorSprite *s = &o->sprites[i]; r = ref(EDITOR_SELECTION_SPRITE, o->id, 0, s->id);
            settings(d, o, r, &s->entity_settings); asset(d, w, r, s->path);
            if(s->rigid_body && !editor_project_rigid_body_get(o, s->rigid_body)) add(d, r, "body", "Attached body no longer exists");
        }
        for(size_t i = 0; i < o->animated_sprite_count; ++i) {
            EditorAnimatedSprite *s = &o->animated_sprite_items[i]; r = ref(EDITOR_SELECTION_ANIMATED_SPRITE, o->id, 0, s->id);
            settings(d, o, r, &s->entity_settings);
            for(size_t f = 0; f < s->frame_count; ++f)
                asset(d, w, ref(EDITOR_SELECTION_ANIMATION_FRAME, o->id, s->id, s->frames[f].id), s->frames[f].path);
        }
        for(size_t i = 0; i < o->force_count; ++i) {
            EditorForce *f = &o->force_items[i]; r = ref(EDITOR_SELECTION_FORCE, o->id, 0, f->id);
            settings(d, o, r, &f->entity_settings);
            if(!editor_force_description_check(o, f->description))
                add(d, r, "Target,Reference,Mode,Magnitude,Kind", "Force has an unavailable reference or invalid value");
        }
        for(size_t i = 0; i < o->torque_count; ++i) {
            EditorTorque *t = &o->torque_items[i]; r = ref(EDITOR_SELECTION_TORQUE, o->id, 0, t->id);
            settings(d, o, r, &t->entity_settings);
            if(!editor_torque_description_check(o, t->description))
                add(d, r, "Target,Mode,Magnitude", "Torque has an unavailable target or invalid value");
        }
        for(size_t i = 0; i < o->camera_count; ++i) {
            EditorCamera *c = &o->cameras[i]; r = ref(EDITOR_SELECTION_CAMERA, o->id, 0, c->id);
            settings(d, o, r, &c->entity_settings); number(d, r, "zoom", c->zoom, 0, true);
            number(d, r, "width", c->dimensions.x, 0, true); number(d, r, "height", c->dimensions.y, 0, true);
        }
    }
    for(size_t v = 0; v < p->layout_viewport_count; ++v) {
        EditorLayoutViewport *vp = &p->layout_viewports[v];
        for(size_t i = 0; i < vp->camera_item_count; ++i) {
            EditorViewportCameraItem *s = &vp->camera_items[i];
            EditorObject *o = editor_object_query_get(p, s->object);
            if(!o || !editor_project_camera_get(o, s->camera))
                add(d, ref(EDITOR_SELECTION_LAYOUT_VIEWPORT, vp->id, 1, s->id), "source", "Screen source object or camera is missing");
        }
        for(size_t i = 0; i < vp->ui_item_count; ++i) {
            EditorViewportUiItem *ui = &vp->ui_items[i];
            const EditorViewportUiText *text = ui->kind == EDITOR_VIEWPORT_UI_SHAPE ? &ui->value.shape.text : &ui->value.text;
            if(ui->kind == EDITOR_VIEWPORT_UI_SLIDER || !text->font) continue;
            const EditorUiFont *font = NULL;
            for(size_t f = 0; f < p->ui_font_count; ++f) if(p->ui_fonts[f].id == text->font) font = &p->ui_fonts[f];
            if(!font || !asset_check(w, font->path))
                add(d, ref(ui->kind == EDITOR_VIEWPORT_UI_SHAPE ? EDITOR_SELECTION_UI_SHAPE : EDITOR_SELECTION_UI_TEXT,
                    vp->id, 0, ui->id), "font", "Custom font is missing or unavailable");
        }
    }
}

static EditorHierarchySelection item_kind(EditorItemKind kind) {
    switch(kind) {
        case EDITOR_ITEM_OBJECT: return EDITOR_SELECTION_OBJECT;
        case EDITOR_ITEM_RIGID_BODY: return EDITOR_SELECTION_RIGID_BODY;
        case EDITOR_ITEM_HITBOX: return EDITOR_SELECTION_HITBOX;
        case EDITOR_ITEM_JOINT: return EDITOR_SELECTION_JOINT;
        case EDITOR_ITEM_ANCHOR: return EDITOR_SELECTION_ANCHOR;
        case EDITOR_ITEM_SOFT_BODY: return EDITOR_SELECTION_SOFT_BODY;
        case EDITOR_ITEM_SOFT_NODE: return EDITOR_SELECTION_SOFT_NODE;
        case EDITOR_ITEM_SOFT_BEAM: return EDITOR_SELECTION_SOFT_BEAM;
        case EDITOR_ITEM_CAMERA: return EDITOR_SELECTION_CAMERA;
        case EDITOR_ITEM_SPRITE: return EDITOR_SELECTION_SPRITE;
        case EDITOR_ITEM_ANIMATED_SPRITE: return EDITOR_SELECTION_ANIMATED_SPRITE;
        case EDITOR_ITEM_ANIMATION_FRAME: return EDITOR_SELECTION_ANIMATION_FRAME;
        case EDITOR_ITEM_FORCE: return EDITOR_SELECTION_FORCE;
        case EDITOR_ITEM_TORQUE: return EDITOR_SELECTION_TORQUE;
        case EDITOR_ITEM_SOFT_AREA: return EDITOR_SELECTION_SOFT_AREA;
        case EDITOR_ITEM_SOFT_HOLE: return EDITOR_SELECTION_SOFT_HOLE;
        default: return EDITOR_SELECTION_NONE;
    }
}
EditorSelectionRef editor_diagnostic_command_target_get(const EditorCommand *c) {
    if(!c) return (EditorSelectionRef){0};
    switch(c->type) {
        case EDITOR_COMMAND_PROPERTY_SET:
            return ref(item_kind(c->data.property_set.kind), c->data.property_set.object,
                c->data.property_set.parent, c->data.property_set.item);
        case EDITOR_COMMAND_ITEM_RENAME:
            return ref(item_kind(c->data.item_rename.kind), c->data.item_rename.object,
                c->data.item_rename.parent, c->data.item_rename.item);
        case EDITOR_COMMAND_RELATIONSHIP_SET: {
            const EditorRelationshipSetCommand *r = &c->data.relationship_set;
            return ref(r->kind == EDITOR_RELATIONSHIP_JOINT_ANCHOR ? EDITOR_SELECTION_JOINT :
                r->kind == EDITOR_RELATIONSHIP_SOFT_BEAM_NODE ? EDITOR_SELECTION_SOFT_BEAM :
                r->kind == EDITOR_RELATIONSHIP_ENTITY_PARENT ? EDITOR_SELECTION_RIGID_BODY : EDITOR_SELECTION_ANCHOR,
                r->object, r->parent, r->item);
        }
        case EDITOR_COMMAND_SOURCE: return ref(c->data.source.torque ? EDITOR_SELECTION_TORQUE : EDITOR_SELECTION_FORCE,
            c->data.source.object, 0, c->data.source.id);
        case EDITOR_COMMAND_JOINT_PLACE: return ref(EDITOR_SELECTION_JOINT, c->data.joint_place.object, 0, c->data.joint_place.joint);
        case EDITOR_COMMAND_RIGID_BODY_TRANSFORM: return ref(EDITOR_SELECTION_RIGID_BODY, c->data.rigid_body_transform.object, 0, c->data.rigid_body_transform.body);
        case EDITOR_COMMAND_ANCHOR_TRANSFORM: return ref(EDITOR_SELECTION_ANCHOR, c->data.anchor_transform.object, 0, c->data.anchor_transform.anchor);
        case EDITOR_COMMAND_SOFT_BODY_TRANSFORM: return ref(EDITOR_SELECTION_SOFT_BODY, c->data.soft_body_transform.object, 0, c->data.soft_body_transform.body);
        case EDITOR_COMMAND_SOFT_NODE_POSITION: return ref(EDITOR_SELECTION_SOFT_NODE, c->data.soft_node_position.object, c->data.soft_node_position.body, c->data.soft_node_position.node);
        case EDITOR_COMMAND_CAMERA_TRANSFORM: return ref(EDITOR_SELECTION_CAMERA, c->data.camera_transform.object, 0, c->data.camera_transform.camera);
        default: return (EditorSelectionRef){0};
    }
}
