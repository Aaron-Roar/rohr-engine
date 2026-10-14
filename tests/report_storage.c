/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "rohr.h"
#include "core/report_internal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define OK(call) assert(!error_check(call))
static Report saved, combined, parent, child;

#ifdef ROHR_TEST_ALLOCATION_WRAP
static bool forbid_allocation;
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__wrap_malloc(size_t n) { assert(!forbid_allocation); return __real_malloc(n); }
void *__wrap_calloc(size_t n, size_t s) { assert(!forbid_allocation); return __real_calloc(n,s); }
void *__wrap_realloc(void *p, size_t n) { assert(!forbid_allocation); return __real_realloc(p,n); }
#endif

/* Compare semantic fields: separately composed values need not share padding. */
#define SAME(field) assert(a->field == b->field)
static void header_equal(const ReportHeader *a, const ReportHeader *b) {
    SAME(id); SAME(status); SAME(total_ms);
}
static void diagnostic_equal(const ReportDiagnostic *a, const ReportDiagnostic *b) {
    SAME(report_id); SAME(internal_stage); SAME(severity); SAME(code); SAME(recovery);
    SAME(source.kind); SAME(source.id); SAME(target.kind); SAME(target.id);
    SAME(occurrence_count); SAME(has_substep); SAME(first_substep); SAME(last_substep);
    SAME(detail_truncated); SAME(group_version);
    assert(strcmp(a->detail,b->detail)==0);
}
static void reports_equal(const Report *a, const Report *b) {
    header_equal(&a->header,&b->header);
    SAME(entry_count); SAME(statistic_count); SAME(diagnostic_count);
    SAME(dropped_entry_count); SAME(dropped_statistic_count);
    SAME(dropped_diagnostic_occurrences); SAME(counters_saturated); SAME(has_fatal);
    SAME(version); SAME(invocation_count);
    for(size_t i=0;i<ROHR_REPORT_ID;++i) SAME(included_versions[i]);
    for(size_t i=0;i<a->entry_count;++i) {
        header_equal(&a->entries[i].header,&b->entries[i].header);
        SAME(entries[i].invocation_count); SAME(entries[i].group_version);
        SAME(entries[i].group_root);
    }
    for(size_t i=0;i<a->statistic_count;++i) {
        SAME(statistics[i].report_id); SAME(statistics[i].metric_id);
        SAME(statistics[i].aggregation); SAME(statistics[i].value);
        SAME(statistics[i].group_version);
    }
    for(size_t i=0;i<a->diagnostic_count;++i)
        diagnostic_equal(&a->diagnostics[i],&b->diagnostics[i]);
    if(a->has_fatal) diagnostic_equal(&a->first_fatal,&b->first_fatal);
}
#undef SAME

static const ReportFlags selectors[ROHR_REPORT_ID + 1] = {
    [ROHR_PHYSICS_REPORT_ID] = ROHR_PHYSICS_REPORT,
    [ROHR_PHYSICS_SUBSTEP_REPORT_ID] = ROHR_PHYSICS_SUBSTEP_REPORT,
    [ROHR_PHYSICS_LOADS_REPORT_ID] = ROHR_PHYSICS_LOADS_REPORT,
    [ROHR_PHYSICS_INTEGRATION_REPORT_ID] = ROHR_PHYSICS_INTEGRATION_REPORT,
    [ROHR_PHYSICS_CONTACTS_REPORT_ID] = ROHR_PHYSICS_CONTACTS_REPORT,
    [ROHR_PHYSICS_JOINTS_REPORT_ID] = ROHR_PHYSICS_JOINTS_REPORT,
    [ROHR_PHYSICS_CONSTRAINTS_REPORT_ID] = ROHR_PHYSICS_CONSTRAINTS_REPORT,
    [ROHR_GRAPHICS_REPORT_ID] = ROHR_GRAPHICS_REPORT,
    [ROHR_AUDIO_REPORT_ID] = ROHR_AUDIO_REPORT,
    [ROHR_INPUT_REPORT_ID] = ROHR_INPUT_REPORT,
    [ROHR_UI_REPORT_ID] = ROHR_UI_REPORT,
    [ROHR_ENGINE_REPORT_ID] = ROHR_ENGINE_REPORT,
    [ROHR_GRAPHICS_CAMERAS_REPORT_ID] = ROHR_GRAPHICS_CAMERAS_REPORT,
    [ROHR_GRAPHICS_COMMANDS_REPORT_ID] = ROHR_GRAPHICS_COMMANDS_REPORT,
    [ROHR_GRAPHICS_VIEWPORTS_REPORT_ID] = ROHR_GRAPHICS_VIEWPORTS_REPORT,
    [ROHR_GRAPHICS_RECORDING_REPORT_ID] = ROHR_GRAPHICS_RECORDING_REPORT,
    [ROHR_GRAPHICS_PRESENTATION_REPORT_ID] = ROHR_GRAPHICS_PRESENTATION_REPORT,
    [ROHR_GRAPHICS_ANIMATIONS_REPORT_ID] = ROHR_GRAPHICS_ANIMATIONS_REPORT,
    [ROHR_AUDIO_COMMANDS_REPORT_ID] = ROHR_AUDIO_COMMANDS_REPORT,
    [ROHR_AUDIO_SOUNDS_REPORT_ID] = ROHR_AUDIO_SOUNDS_REPORT,
    [ROHR_AUDIO_MUSIC_REPORT_ID] = ROHR_AUDIO_MUSIC_REPORT,
    [ROHR_AUDIO_OUTPUT_REPORT_ID] = ROHR_AUDIO_OUTPUT_REPORT,
    [ROHR_ENGINE_LIFECYCLE_REPORT_ID] = ROHR_ENGINE_LIFECYCLE_REPORT,
    [ROHR_REPORT_ID] = ROHR_REPORT
};

static Report get(ReportId id) {
    ReportResult r = rohr_report_get(selectors[id]); OK(r);
    assert(report_valid_check(&r.result.value));
    assert(r.result.value.header.id == id);
    return r.result.value;
}
static Report value(ReportId id, uint64_t count, bool fatal) {
    Report r = {.header = {id, ROHR_REPORT_COMPLETE, 1}};
    OK(report_statistic_add(&r, (ReportStatistic){.report_id=id,
        .metric_id=ROHR_REPORT_PROCESSED_COUNT, .aggregation=ROHR_REPORT_AGGREGATION_SUM, .value=count}));
    ReportDiagnostic d = {.report_id=id, .severity=fatal ? ROHR_REPORT_SEVERITY_FATAL : ROHR_REPORT_SEVERITY_WARNING,
        .code=ERROR_ENGINE_STATE_INVALID, .occurrence_count=1};
    char detail[64]; snprintf(detail,sizeof(detail),"sample %llu",(unsigned long long)count);
    OK(report_diagnostic_detail_set(&d,detail)); OK(report_diagnostic_add(&r,d));
    return r;
}
static size_t roots(const Report *r, ReportId id) {
    size_t n=0;
    for(size_t i=0;i<r->entry_count;++i)
        if(r->entries[i].group_root && r->entries[i].header.id==id) ++n;
    return n;
}
static void initial(void) {
    const ReportFlags bad[]={0, ROHR_INPUT_REPORT, UINT64_C(1) << 63,
        UINT64_MAX, ROHR_PHYSICS_REPORT | ROHR_INPUT_REPORT,
        ROHR_REPORT | (UINT64_C(1) << 23), ROHR_REPORT | ROHR_INPUT_REPORT};
    for(size_t i=0;i<sizeof(bad)/sizeof(bad[0]);++i) {
        ReportResult r=report_get(bad[i]);
        assert(error_check(r) && r.result.error==ERROR_ENGINE_REPORT_INVALID);
        r=rohr_report_get(bad[i]);
        assert(error_check(r) && r.result.error==ERROR_ENGINE_REPORT_INVALID);
    }
    for(ReportId id=1;id<ROHR_REPORT_ID;++id) {
        if(id==ROHR_INPUT_REPORT_ID) continue;
        Report r=get(id); assert(r.version==0 && r.header.status==ROHR_REPORT_NOT_RUN);
        ReportResult direct=report_get(selectors[id]); OK(direct);
        assert(memcmp(&r,&direct.result.value,sizeof(r))==0);
    }
    ReportResult empty=report_get(ROHR_PHYSICS_REPORT | ROHR_AUDIO_REPORT); OK(empty);
    assert(empty.result.value.header.id==ROHR_REPORT_ID && empty.result.value.version==0);
    assert(empty.result.value.entry_count==0 && empty.result.value.header.status==ROHR_REPORT_NOT_RUN);
    combined=get(ROHR_REPORT_ID); assert(combined.version==0 && combined.entry_count==0);
    assert(combined.header.status==ROHR_REPORT_NOT_RUN);
}
static void composition(void) {
    parent=(Report){.header={ROHR_PHYSICS_REPORT_ID,ROHR_REPORT_COMPLETE,5}};
    for(unsigned i=1;i<=2;++i) {
        child=value(ROHR_PHYSICS_JOINTS_REPORT_ID,i,false);
        report_publish_internal(&child); OK(report_merge(&parent,&child));
    }
    assert(parent.entries[0].invocation_count==2 && parent.statistics[0].value==3);
    report_publish_internal(&parent); saved=parent;
    Report fetched=get(ROHR_PHYSICS_REPORT_ID); assert(memcmp(&fetched,&saved,sizeof(saved))==0);
    combined=get(ROHR_REPORT_ID);
    assert(roots(&combined,ROHR_PHYSICS_REPORT_ID)==1 && roots(&combined,ROHR_PHYSICS_JOINTS_REPORT_ID)==0);
    assert(combined.statistics[0].value==3 && combined.statistics[0].group_version==parent.version);
    child=value(ROHR_PHYSICS_JOINTS_REPORT_ID,7,false); report_publish_internal(&child);
    combined=get(ROHR_REPORT_ID);
    assert(roots(&combined,ROHR_PHYSICS_JOINTS_REPORT_ID)==1 && combined.statistic_count==2);
    assert(combined.statistics[1].value==7 && combined.statistics[1].group_version==child.version);
    assert(combined.diagnostic_count==2 && combined.diagnostics[0].occurrence_count==2);
    assert(combined.diagnostics[1].occurrence_count==1 && combined.header.total_ms==0);
    ReportResult direct=report_get(ROHR_REPORT); OK(direct);
    reports_equal(&combined,&direct.result.value);
    OK(report_text_write(&combined,"stored_report.txt"));
    OK(report_json_write(&combined,"stored_report.json",true));
    assert(remove("stored_report.txt")==0 && remove("stored_report.json")==0);
    child=(Report){.header={ROHR_PHYSICS_JOINTS_REPORT_ID,ROHR_REPORT_NOT_RUN,0}};
    report_publish_internal(&child); combined=get(ROHR_REPORT_ID);
    assert(roots(&combined,ROHR_PHYSICS_JOINTS_REPORT_ID)==1);
    for(size_t i=0;i<combined.entry_count;++i)
        if(combined.entries[i].group_root && combined.entries[i].header.id==ROHR_PHYSICS_JOINTS_REPORT_ID)
            assert(combined.entries[i].header.status==ROHR_REPORT_NOT_RUN && combined.entries[i].invocation_count==0);
    fetched=get(ROHR_PHYSICS_REPORT_ID); assert(memcmp(&fetched,&saved,sizeof(saved))==0);
}
static Report select_reports(ReportFlags flags) {
    ReportResult public = rohr_report_get(flags), direct = report_get(flags);
    OK(public); OK(direct);
    assert(report_valid_check(&public.result.value));
    reports_equal(&public.result.value,&direct.result.value);
    return public.result.value;
}

static void selection(void) {
    /* Re-publish a parent containing the exact current child. */
    child=value(ROHR_PHYSICS_JOINTS_REPORT_ID,11,false);
    report_publish_internal(&child);
    parent=(Report){.header={ROHR_PHYSICS_REPORT_ID,ROHR_REPORT_COMPLETE,5}};
    OK(report_merge(&parent,&child)); report_publish_internal(&parent);
    Report ui=value(ROHR_UI_REPORT_ID,19,false); report_publish_internal(&ui);
    Report r=select_reports(ROHR_PHYSICS_JOINTS_REPORT | ROHR_UI_REPORT);
    assert(r.header.id==ROHR_REPORT_ID && r.entry_count==2 && r.statistic_count==2);
    assert(roots(&r,ROHR_PHYSICS_JOINTS_REPORT_ID)==1);
    assert(roots(&r,ROHR_PHYSICS_REPORT_ID)==0); /* Unselected parent cannot suppress. */
    assert(r.statistics[0].value==11 && r.statistics[1].value==19);
    Report copy=r;
    r=select_reports(ROHR_PHYSICS_REPORT | ROHR_PHYSICS_JOINTS_REPORT);
    assert(roots(&r,ROHR_PHYSICS_REPORT_ID)==1 && roots(&r,ROHR_PHYSICS_JOINTS_REPORT_ID)==0);
    assert(r.statistic_count==1 && r.statistics[0].value==11);
    r=select_reports(ROHR_PHYSICS_REPORT | ROHR_PHYSICS_REPORT);
    assert(memcmp(&r,&parent,sizeof(r))==0); /* Repeated flags still select one snapshot. */
    child=value(ROHR_PHYSICS_JOINTS_REPORT_ID,23,false); report_publish_internal(&child);
    r=select_reports(ROHR_PHYSICS_REPORT | ROHR_PHYSICS_JOINTS_REPORT);
    assert(roots(&r,ROHR_PHYSICS_REPORT_ID)==1 && roots(&r,ROHR_PHYSICS_JOINTS_REPORT_ID)==1);
    assert(r.statistic_count==2 && r.statistics[0].value==11 && r.statistics[1].value==23);
    assert(copy.statistics[0].value==11 && copy.statistics[1].value==19);
    r=select_reports(ROHR_UI_REPORT | ROHR_AUDIO_REPORT); /* Unpublished selections omitted. */
    assert(r.header.id==ROHR_REPORT_ID && r.entry_count==1 && roots(&r,ROHR_UI_REPORT_ID)==1);
    ui=(Report){.header={ROHR_UI_REPORT_ID,ROHR_REPORT_NOT_RUN,0}};
    report_publish_internal(&ui);
    r=select_reports(ROHR_UI_REPORT | ROHR_AUDIO_REPORT);
    assert(r.header.status==ROHR_REPORT_NOT_RUN && r.entry_count==1);
    assert(r.entries[0].invocation_count==0 && r.entries[0].group_version==ui.version);
    ReportFlags all=0;
    for(ReportId id=1;id<ROHR_REPORT_ID;++id)
        if(id!=ROHR_INPUT_REPORT_ID) all |= selectors[id];
    assert(all==ROHR_REPORT);
    r=select_reports(all); Report all_copy=get(ROHR_REPORT_ID);
    reports_equal(&r,&all_copy);
    r=select_reports(ROHR_REPORT | ROHR_UI_REPORT);
    reports_equal(&r,&all_copy);
}

static void capacity(void) {
    child=(Report){.header={ROHR_GRAPHICS_COMMANDS_REPORT_ID,ROHR_REPORT_COMPLETE,0}};
    for(unsigned i=0;i<ROHR_REPORT_DIAGNOSTIC_CAPACITY;++i) {
        ReportDiagnostic d={.report_id=child.header.id,.severity=ROHR_REPORT_SEVERITY_WARNING,
            .code=ERROR_ENGINE_STATE_INVALID,.source={ROHR_REPORT_OBJECT_CUSTOM,i},.occurrence_count=1};
        OK(report_diagnostic_add(&child,d));
    }
    ReportDiagnostic fatal={.report_id=child.header.id,.severity=ROHR_REPORT_SEVERITY_FATAL,
        .code=ERROR_ENGINE_STATE_INVALID,.occurrence_count=1};
    OK(report_diagnostic_detail_set(&fatal,"reserved original")); OK(report_diagnostic_add(&child,fatal));
    report_publish_internal(&child); combined=get(ROHR_REPORT_ID);
    assert(combined.has_fatal && strcmp(combined.first_fatal.detail,"reserved original")==0);
    assert(combined.dropped_diagnostic_occurrences>0);
    Report later=value(ROHR_GRAPHICS_PRESENTATION_REPORT_ID,9,true); report_publish_internal(&later);
    combined=get(ROHR_REPORT_ID); assert(strcmp(combined.first_fatal.detail,"reserved original")==0);
    assert(combined.first_fatal.group_version==child.version);
    Report selected=select_reports(ROHR_GRAPHICS_COMMANDS_REPORT | ROHR_GRAPHICS_PRESENTATION_REPORT);
    assert(selected.has_fatal && strcmp(selected.first_fatal.detail,"reserved original")==0);
    assert(selected.dropped_diagnostic_occurrences>0);
    selected=select_reports(ROHR_UI_REPORT | ROHR_ENGINE_REPORT);
    assert(!selected.has_fatal && selected.dropped_diagnostic_occurrences==0);
    parent=(Report){.header={ROHR_GRAPHICS_REPORT_ID,ROHR_REPORT_COMPLETE,0}};
    /* Fill ordinary entries before merging a child: coverage must not be lost. */
    for(unsigned i=0;i<ROHR_REPORT_ENTRY_CAPACITY;++i)
        OK(report_entry_add(&parent,(ReportEntry){.header={(ReportId)(ROHR_REPORT_CUSTOM_ID_BEGIN+i),ROHR_REPORT_COMPLETE,0},.invocation_count=1}));
    OK(report_merge(&parent,&child)); report_publish_internal(&parent);
    combined=get(ROHR_REPORT_ID); assert(roots(&combined,ROHR_GRAPHICS_COMMANDS_REPORT_ID)==0);
    assert(combined.dropped_entry_count>0 && combined.has_fatal);
}
static void producers(void) {
    OK(engine_start()); engine_pause(); OK(physics_substeps_set(2));
    Report r=engine_update(), copy=get(ROHR_ENGINE_REPORT_ID);
    assert(memcmp(&r,&copy,sizeof(r))==0);
    Report life=get(ROHR_ENGINE_LIFECYCLE_REPORT_ID);
    assert(r.included_versions[ROHR_ENGINE_LIFECYCLE_REPORT_ID]==life.version);
    uint64_t before=get(ROHR_REPORT_ID).version;
    r=physics_dt_update(.01); copy=get(ROHR_PHYSICS_REPORT_ID);
    assert(memcmp(&r,&copy,sizeof(r))==0 && r.version==before+13);
    Report substep=get(ROHR_PHYSICS_SUBSTEP_REPORT_ID);
    assert(substep.invocation_count==1 && substep.entries[0].invocation_count==1);
    r=physics_dt_update(NAN); copy=get(ROHR_PHYSICS_REPORT_ID);
    assert(memcmp(&r,&copy,sizeof(r))==0 && r.has_fatal);
    assert(get(ROHR_PHYSICS_SUBSTEP_REPORT_ID).version==substep.version);
    r=physics_update(0); assert(r.header.status==ROHR_REPORT_NOT_RUN && get(ROHR_PHYSICS_REPORT_ID).version==r.version);
    saved=r; engine_clock_reset(); engine_stop(); OK(engine_start());
    copy=get(ROHR_PHYSICS_REPORT_ID); assert(memcmp(&copy,&saved,sizeof(saved))==0);
    OK(graphics_start());
    r=graphics_show(); copy=get(ROHR_GRAPHICS_REPORT_ID); assert(memcmp(&r,&copy,sizeof(r))==0);
    r=graphics_sprite_frames_update(0,0); assert(get(ROHR_GRAPHICS_ANIMATIONS_REPORT_ID).version==r.version);
    uint64_t ui_before=get(ROHR_UI_REPORT_ID).version;
    ui_translation_y_pop(); assert(get(ROHR_UI_REPORT_ID).version==ui_before);
    r=ui_update(); copy=get(ROHR_UI_REPORT_ID);
    assert(memcmp(&r,&copy,sizeof(r))==0);
    OK(audio_start()); r=audio_update(); copy=get(ROHR_AUDIO_REPORT_ID);
    assert(memcmp(&r,&copy,sizeof(r))==0);
    Report sounds=get(ROHR_AUDIO_SOUNDS_REPORT_ID);
    assert(r.included_versions[ROHR_AUDIO_SOUNDS_REPORT_ID]==sounds.version);
    r=audio_stop(); copy=get(ROHR_AUDIO_REPORT_ID); assert(memcmp(&r,&copy,sizeof(r))==0);
    copy=audio_update(); assert(memcmp(&r,&copy,sizeof(r))==0);
    assert(get(ROHR_AUDIO_REPORT_ID).version==r.version);
    OK(audio_start()); assert(get(ROHR_AUDIO_REPORT_ID).version==r.version);
    (void)audio_stop(); graphics_stop(); engine_stop();
}
static SDL_AtomicInt done;
static int SDLCALL writer(void *unused) {
    (void)unused;
    for(unsigned i=1;i<=300;++i) {
        Report r=value(ROHR_UI_REPORT_ID,i,false); report_publish_internal(&r);
    }
    SDL_SetAtomicInt(&done,1); return 0;
}
static int SDLCALL reader(void *unused) {
    (void)unused;
    uint64_t previous=0;
    do {
        Report r=get(ROHR_UI_REPORT_ID);
        assert(r.version>=previous); previous=r.version;
        unsigned long long n=0; assert(sscanf(r.diagnostics[0].detail,"sample %llu",&n)==1);
        assert(r.statistics[0].value==n);
        Report all=get(ROHR_REPORT_ID); assert(all.version>=r.version);
        ReportResult read=report_get(ROHR_UI_REPORT | ROHR_ENGINE_REPORT); OK(read);
        Report subset=read.result.value;
        for(size_t i=0;i<subset.statistic_count;++i) {
            if(subset.statistics[i].report_id!=ROHR_UI_REPORT_ID) continue;
            bool found=false;
            for(size_t j=0;j<subset.diagnostic_count;++j) {
                if(subset.diagnostics[j].report_id!=ROHR_UI_REPORT_ID) continue;
                assert(sscanf(subset.diagnostics[j].detail,"sample %llu",&n)==1);
                assert(subset.statistics[i].value==n); found=true;
            }
            assert(found);
        }
    } while(!SDL_GetAtomicInt(&done));
    return 0;
}
int main(void) {
    initial(); composition(); selection(); capacity(); producers();
    child=value(ROHR_UI_REPORT_ID,0,false); report_publish_internal(&child);
#ifdef ROHR_TEST_ALLOCATION_WRAP
    forbid_allocation=true;
#endif
    for(unsigned i=0;i<10;++i) { child.version=0; report_publish_internal(&child); combined=select_reports(ROHR_UI_REPORT | ROHR_ENGINE_REPORT); }
#ifdef ROHR_TEST_ALLOCATION_WRAP
    forbid_allocation=false;
#endif
    SDL_Thread *a=SDL_CreateThread(reader,"report-reader-a",NULL);
    SDL_Thread *b=SDL_CreateThread(reader,"report-reader-b",NULL);
    SDL_Thread *w=SDL_CreateThread(writer,"report-writer",NULL);
    assert(a && b && w); SDL_WaitThread(w,NULL); SDL_WaitThread(a,NULL); SDL_WaitThread(b,NULL);
    Uint64 start=SDL_GetTicksNS();
    for(unsigned i=0;i<100;++i) combined=get(ROHR_REPORT_ID);
    printf("report storage: Report=%zu bytes, cache=%zu bytes, 100 combined reads=%.3f ms\n",
        sizeof(Report),sizeof(Report)*ROHR_REPORT_ID,(double)(SDL_GetTicksNS()-start)/1000000.0);
    return 0;
}
