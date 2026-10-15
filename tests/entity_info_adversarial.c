/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define F(name) ROHR_ENTITY_INFO_##name
#define OK(call) assert(!error_check(call))
static const Shape box = {.amount_of_vertices=4, .vertices={{-1,-1},{1,-1},{1,1},{-1,1}}};
#ifdef ROHR_TEST_ALLOCATION_WRAP
static long fail_after = -1;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
static bool fail(void) { if(fail_after < 0) return false; if(fail_after == 0) return true; --fail_after; return false; }
void *__wrap_malloc(size_t n) { return fail() ? NULL : __real_malloc(n); }
void *__wrap_calloc(size_t n, size_t z) { return fail() ? NULL : __real_calloc(n,z); }
void *__wrap_realloc(void *p, size_t n) { return fail() ? NULL : __real_realloc(p,n); }
#endif
static Entity make(void) { EntityResult r=entity_add(); OK(r); return r.result.value; }
static EntityIndex index_of(Entity e) { EntityIndex i; assert(entity_index_get(e,&i)); return i; }
static EntityInfo get(Entity e, EntityInfoFlags flags) { EntityInfoResult r=rohr_entity_info_get(e,flags); OK(r); return r.result.value; }
static size_t issues(EntityInfo *info, EntityInfoFlags flags) {
    EntityInfoIssueCountResult r=entity_info_issues_count_get(info,flags); OK(r); return r.result.value;
}
static void selector_composition(void) {
    Entity e=make(); EntityIndex i=index_of(e);
    /* Inject invalid stored data without changing the resource ownership graph. */
    OK(SpritePool_store_at(&sprites_pool,i,(Sprite){.scale={NAN,1}}));
    entity_mask[i] |= ROHR_SPRITE;
    EntityInfo alone=get(e,F(GRAPHICS_SPRITE_SCALE));
    EntityInfo combined=get(e,F(GRAPHICS_SPRITE_SCALE)|F(GRAPHICS_SPRITE));
    assert(issues(&alone,F(GRAPHICS_SPRITE_SCALE)) == 1);
    assert(issues(&combined,F(GRAPHICS_SPRITE_SCALE)) == 1);
    assert(isnan(alone.sprite_scale.x) && isnan(combined.sprite_scale.x));
    OK(entity_info_release(&alone)); OK(entity_info_release(&combined));
    entity_mask[i] &= ~ROHR_SPRITE; OK(SpritePool_release_at(&sprites_pool,i));
    OK(entity_delete(e));
}
static void independent_dependencies(void) {
    EntityResult body=physics_soft_body_create(); OK(body);
    EntityResult a=physics_soft_body_node_create(body.result.value,(Position){0,0},1,1); OK(a);
    EntityResult b=physics_soft_body_node_create(body.result.value,(Position){4,0},1,1); OK(b);
    EntityResult beam=physics_soft_body_beam_create(body.result.value,a.result.value,b.result.value,1,1); OK(beam);
    EntityIndex bi=index_of(b.result.value), ei=index_of(beam.result.value);
    Entity original=soft_body_beams[ei].node_a;
    soft_body_beams[ei].node_a=ENTITY_INVALID;
    uint8_t used=positions_pool.used[bi]; positions_pool.used[bi]=0;
    EntityInfo info=get(beam.result.value,F(PHYSICS_SOFT_BODY_BEAM));
    assert(entity_info_check(&info,F(PHYSICS_SOFT_BODY_BEAM)));
    EntityInfoIssue evidence[4];
    EntityInfoIssueReadResult r=entity_info_issues_get(&info,F(PHYSICS_SOFT_BODY_BEAM),evidence,4); OK(r);
    assert(r.result.value.total == 2);
    assert(evidence[0].reason == ROHR_ENTITY_INFO_REFERENCE_MISSING);
    assert(evidence[1].reason == ROHR_ENTITY_INFO_REFERENCE_DATA_MISSING);
    assert(evidence[1].reference.id == b.result.value);
    OK(entity_info_release(&info));
    positions_pool.used[bi]=used; soft_body_beams[ei].node_a=original;
    info=get(beam.result.value,F(PHYSICS_SOFT_BODY_BEAM)); assert(issues(&info,ROHR_ENTITY_INFO)==0);
    OK(entity_info_release(&info)); OK(entity_delete(body.result.value));
}
static void malformed_collection(void) {
    Entity e=make(); OK(physics_hitbox_add(e,box));
    EntityIndex i=index_of(e); HitboxVariantList saved=hitbox_variants_pool.objects[i];
    hitbox_variants_pool.objects[i].count=saved.capacity+1;
    EntityInfo info=get(e,F(PHYSICS_HITBOXES)|F(PHYSICS_HITBOX_COUNT));
    assert(!entity_info_check(&info,F(PHYSICS_HITBOXES)));
    assert(!entity_info_check(&info,F(PHYSICS_HITBOX_COUNT)));
    assert(info.hitboxes==NULL && info.hitbox_count==0);
    assert(issues(&info,F(PHYSICS_HITBOXES))==1 && issues(&info,F(PHYSICS_HITBOX_COUNT))==1);
    OK(entity_info_release(&info));
    hitbox_variants_pool.objects[i]=saved;
    hitbox_variants_pool.objects[i].values=NULL;
    info=get(e,F(PHYSICS_HITBOXES)|F(PHYSICS_HITBOX_COUNT));
    assert(info.available==0 && info.hitboxes==NULL && info.hitbox_count==0);
    OK(entity_info_release(&info)); hitbox_variants_pool.objects[i]=saved;
    hitbox_variants_pool.objects[i].active_index=saved.count;
    info=get(e,F(PHYSICS_HITBOX_LOCAL)|F(PHYSICS_HITBOX_ACTIVE));
    assert(info.available==0 && issues(&info,ROHR_ENTITY_INFO)==2);
    OK(entity_info_release(&info)); hitbox_variants_pool.objects[i]=saved;
    Shape prior=hit_boxes[i]; hit_boxes[i].amount_of_vertices=MAX_VERTICIES+1;
    info=get(e,F(PHYSICS_HITBOX_WORLD));
    assert(info.available==0 && issues(&info,ROHR_ENTITY_INFO)==1);
    OK(entity_info_release(&info)); hit_boxes[i]=prior;
    OK(entity_delete(e));
}
static void numeric_and_removal(void) {
    Entity e=make(); EntityIndex i=index_of(e);
    const float invalid_values[]={NAN,INFINITY,-INFINITY};
    for(size_t k=0;k<3;++k) {
        OK(VelocityPool_store_at(&velocities_pool,i,(Velocity){invalid_values[k],0}));
        EntityInfo info=get(e,F(PHYSICS_VELOCITY)|F(PHYSICS_ORIENTATION));
        assert(!entity_info_check(&info,F(PHYSICS_VELOCITY)));
        assert(info.velocity.x==0 && info.velocity.y==0);
        assert(issues(&info,F(PHYSICS_VELOCITY))==1);
        assert(isnan(invalid_values[k]) ? isnan(velocities[i].x) : velocities[i].x==invalid_values[k]);
        OK(entity_info_release(&info));
    }
    OK(physics_velocity_set(e,(Velocity){0}));
    EntityInfo zero=get(e,F(PHYSICS_VELOCITY)); assert(zero.available==zero.requested && issues(&zero,zero.requested)==0);
    OK(entity_info_release(&zero));
    Entity target=make(); OK(physics_force_set(e,(Force){0}));
    OK(entity_delete(target));
    OK(TargetPool_store_at(&targets_pool,i,target)); entity_mask[i] |= ROHR_TARGETABLE;
    EntityInfo stale=get(e,F(CORE_TARGET)|F(PHYSICS_FORCE));
    assert(stale.target==target && stale.force.x==0 && stale.available==stale.requested);
    assert(issues(&stale,F(CORE_TARGET))==1 && issues(&stale,F(PHYSICS_FORCE))==1);
    targets[i]=0;
    EntityInfo repaired=get(e,F(CORE_TARGET)|F(PHYSICS_FORCE));
    assert(issues(&repaired,ROHR_ENTITY_INFO)==0 && issues(&stale,ROHR_ENTITY_INFO)==2);
    OK(entity_info_release(&stale)); OK(entity_info_release(&repaired));
    OK(physics_hitbox_add(e,box)); EntityInfo before=get(e,F(PHYSICS_HITBOXES));
    OK(entity_components_delete(e,ROHR_HIT_BOX));
    EntityInfo after=get(e,F(PHYSICS_HITBOXES)|F(PHYSICS_HITBOX_LOCAL));
    assert(after.hitbox_count==0 && after.hitboxes==NULL);
    assert(entity_info_check(&after,F(PHYSICS_HITBOXES)));
    assert(!entity_info_check(&after,F(PHYSICS_HITBOX_LOCAL)));
    assert(before.hitbox_count==1 && before.hitboxes[0].shape.amount_of_vertices==4);
    OK(entity_info_release(&before)); OK(entity_info_release(&after)); OK(entity_delete(e));
}
static uint32_t random_value(uint32_t *state) { *state ^= *state<<13; *state ^= *state>>17; *state ^= *state<<5; return *state; }
typedef struct Owner { EntityInfo info; float expected_x; size_t issue_count; } Owner;
static void lifecycle_sequences(void) {
    const uint32_t seeds[]={1,0x12345678,0xdeadbeef,0x51f15e};
    for(size_t seed=0;seed<4;++seed) {
        printf("inspection seed=%08x\n",seeds[seed]); fflush(stdout);
        uint32_t rng=seeds[seed]; Owner slots[32]={0}; Entity e=make(); float x=0;
        OK(physics_velocity_set(e,(Velocity){x,0})); OK(physics_hitbox_add(e,box));
        for(size_t step=0;step<2000;++step) {
            size_t slot=random_value(&rng)%32, other=random_value(&rng)%32;
            unsigned action=random_value(&rng)%5;
            if(action==0 && !slots[slot].info.handle) {
                EntityInfoFlags flags=F(PHYSICS_VELOCITY)|F(PHYSICS_HITBOXES)|F(CORE_NAME);
                slots[slot].info=get(e,flags); slots[slot].expected_x=x;
                slots[slot].issue_count=issues(&slots[slot].info,ROHR_ENTITY_INFO);
            } else if(action==1 && !slots[slot].info.handle && slots[other].info.handle) {
                EntityInfoResult r=entity_info_retain(&slots[other].info); OK(r);
                slots[slot]=slots[other]; slots[slot].info=r.result.value;
            } else if(action==2 && slots[slot].info.handle) {
                EntityInfo alias=slots[slot].info; bool last=true;
                for(size_t j=0;j<32;++j) if(j!=slot && slots[j].info.handle==alias.handle) last=false;
                OK(rohr_entity_info_release(&slots[slot].info));
                if(last) { assert(!entity_info_check(&alias,F(PHYSICS_VELOCITY))); assert(error_check(entity_info_retain(&alias))); }
            } else if(action==3) { x=(float)step; OK(physics_velocity_set(e,(Velocity){x,0})); }
            if(step%200==199) {
                engine_stop(); OK(engine_start()); e=make(); x=-(float)step;
                OK(physics_velocity_set(e,(Velocity){x,0})); OK(physics_hitbox_add(e,box));
            }
            for(size_t j=0;j<32;++j) if(slots[j].info.handle) {
                assert(entity_info_check(&slots[j].info,F(PHYSICS_VELOCITY)|F(PHYSICS_HITBOXES)));
                assert(slots[j].info.velocity.x==slots[j].expected_x);
                assert(slots[j].info.hitbox_count==1 && slots[j].info.hitboxes[0].shape.amount_of_vertices==4);
                assert(issues(&slots[j].info,ROHR_ENTITY_INFO)==slots[j].issue_count);
            }
        }
        for(size_t j=0;j<32;++j) OK(entity_info_release(&slots[j].info));
        OK(entity_delete(e));
    }
}
static void issue_buffers_and_failures(void) {
    Entity e=make(); EntityInfo stable=get(e,ROHR_ENTITY_INFO);
    size_t count=issues(&stable,ROHR_ENTITY_INFO); assert(count>8);
    EntityInfoIssue *buffer=malloc((count+2)*sizeof(*buffer)); assert(buffer);
    for(size_t capacity=0;capacity<=count+1;++capacity) {
        memset(buffer,0x5a,(count+2)*sizeof(*buffer));
        EntityInfoIssue guard=buffer[capacity];
        EntityInfoIssueReadResult r=entity_info_issues_get(&stable,ROHR_ENTITY_INFO,buffer,capacity); OK(r);
        assert(r.result.value.total==count && r.result.value.written==(capacity<count?capacity:count));
        assert(memcmp(&guard,&buffer[capacity],sizeof(guard))==0);
        for(size_t j=0;j<r.result.value.written;++j) assert(memchr(buffer[j].detail,0,sizeof(buffer[j].detail)));
    }
    for(unsigned bit=0;bit<61;++bit) {
        EntityInfoFlags flag=UINT64_C(1)<<bit;
        EntityInfo single=get(e,flag);
        assert(issues(&single,flag)==issues(&stable,flag));
        assert(entity_info_check(&single,flag)==entity_info_check(&stable,flag));
        OK(entity_info_release(&single));
    }
#ifdef ROHR_TEST_ALLOCATION_WRAP
    bool success=false;
    for(long n=0;n<32;++n) {
        fail_after=n; EntityInfoResult r=entity_info_get(e,ROHR_ENTITY_INFO); fail_after=-1;
        assert(issues(&stable,ROHR_ENTITY_INFO)==count);
        if(r.kind==ERROR_RESULT_VALUE) { OK(entity_info_release(&r.result.value)); success=true; break; }
        assert(r.result.error==ERROR_MEMORY_POOL_ALLOCATION_FAILED);
    }
    assert(success);
    fail_after=0; EntityInfoResult retained=entity_info_retain(&stable); fail_after=-1; OK(retained);
    OK(entity_info_release(&retained.result.value));
#endif
    /* Random combinations must preserve each requested section's evidence. */
    uint32_t rng=0x7e57ca5e;
    static ReportResult before, after;
    before=report_get(ROHR_PHYSICS_REPORT); OK(before);
    for(size_t trial=0;trial<128;++trial) {
        EntityInfoFlags flags=(((uint64_t)random_value(&rng)<<32)|random_value(&rng)) & ROHR_ENTITY_INFO;
        if(!flags) flags=F(CORE_COMPONENTS);
        EntityInfo selected=get(e,flags);
        assert(selected.requested==flags);
        for(unsigned bit=0;bit<61;++bit) if(flags & (UINT64_C(1)<<bit)) {
            EntityInfoFlags flag=UINT64_C(1)<<bit;
            assert(issues(&selected,flag)==issues(&stable,flag));
            assert(entity_info_check(&selected,flag)==entity_info_check(&stable,flag));
        }
        OK(entity_info_release(&selected));
    }
    after=report_get(ROHR_PHYSICS_REPORT); OK(after);
    assert(memcmp(&before.result.value,&after.result.value,sizeof(Report))==0);
    free(buffer); OK(entity_info_release(&stable)); OK(entity_delete(e));
}
int main(int argc, char **argv) {
    const char *names[]={"selectors","dependencies","collections","numeric","lifecycle","buffers"};
    void (*cases[])(void)={selector_composition,independent_dependencies,malformed_collection,numeric_and_removal,lifecycle_sequences,issue_buffers_and_failures};
    for(size_t i=0;i<6;++i) if(argc==1 || strcmp(argv[1],names[i])==0) {
        OK(engine_start()); cases[i](); engine_stop();
    }
    return 0;
}
