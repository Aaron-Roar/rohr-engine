/* Copyright 2026 Aaron Rohrer
 * SPDX-License-Identifier: LGPL-3.0-only
 */
#include "soft_body_stability.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
static volatile sig_atomic_t interrupted;
static void interrupt_test(int signum){(void)signum;interrupted=1;}
static int compare(const void *a,const void *b){double x=*(const double*)a,y=*(const double*)b;return (x>y)-(x<y);}
static bool run(unsigned kind,bool ramp,unsigned hz,bool delayed,bool performance) {
    if(!sb_ok(rohr_engine_start()))return false;
    SbFixture f;if(!sb_create(&f,kind,ramp)){rohr_engine_stop();return false;}
    for(unsigned i=0;i<8;++i){f.previous[i]=positions[f.ni[i]];f.previous_valid[i]=true;}
    physics_substep_observer_set(sb_observe,&f);
    double integration=0,gather=0,solve=0;
    double t=0,end=kind==5?10:30,samples[4000],stall=0;unsigned count=0;bool short_done=false,long_done=false;
    double short_time=kind==5?4.01:8.01,long_time=kind==5?8.01:12.01;
    while(t<end-1e-9&&!f.stop_reason&&!interrupted) {
        if(!sb_events(&f,t)){f.valid=false;f.stop_reason="fixture_event_failed";break;}
        double dt=fmin(1.0/hz,end-t);bool delay=false;
        /* Loads are created before the delayed update, at exact event boundaries. */
        if(delayed&&!short_done&&t>=short_time-1e-9){dt=.1;short_done=true;delay=true;}
        else if(delayed&&!long_done&&t>=long_time-1e-9){dt=1;long_done=true;delay=true;}
        if(!delay){double boundaries[]={8,12,delayed&&!short_done?short_time:end,delayed&&!long_done?long_time:end};
            for(unsigned i=0;i<4;++i)if(boundaries[i]>t+1e-9)dt=fmin(dt,boundaries[i]-t);}
        uint64_t before=f.substeps,reported=0;
        double observer=f.observer_cpu;clock_t begin=clock();Report r=rohr_system_physics_update(dt);
        double cpu=1000.0*(clock()-begin)/CLOCKS_PER_SEC-(f.observer_cpu-observer);
        for(size_t e=0;e<r.entry_count;++e) {
            ReportEntry entry=r.entries[e];
            if(entry.header.id==ROHR_PHYSICS_INTEGRATION_REPORT_ID)integration+=entry.header.total_ms;
            if(entry.header.id==ROHR_PHYSICS_CONTACTS_REPORT_ID)gather+=entry.header.total_ms;
            if(entry.header.id==ROHR_PHYSICS_CONSTRAINTS_REPORT_ID)solve+=entry.header.total_ms;
        }
        t+=dt;if(delay)stall=fmax(stall,cpu);else if(count<4000)samples[count++]=cpu;
        for(size_t i=0;i<r.statistic_count;++i)
            if(r.statistics[i].report_id==ROHR_PHYSICS_REPORT_ID&&
                r.statistics[i].metric_id==ROHR_REPORT_SUBSTEPS_COMPLETED_COUNT)reported=r.statistics[i].value;
        if(r.diagnostic_count)f.valid=false;
        if(r.has_fatal||r.header.status==ROHR_REPORT_FAILED)f.stop_reason="engine_failure";
        else if(fabs(f.time-t)>1e-7||reported!=f.substeps-before||!reported)f.stop_reason="time_or_substep_mismatch";
    }
    physics_substep_observer_set(NULL,NULL);
    if(interrupted)f.stop_reason="interrupted";
    bool complete=!f.stop_reason&&fabs(f.time-end)<=1e-7;
    if(!complete)f.valid=false;
    qsort(samples,count,sizeof(double),compare);double p95=count?samples[(count-1)*95/100]:0;
    if(kind==5&&!f.self_contacts)f.valid=false;
    if(kind>=2&&kind<=4&&!f.particle_contacts)f.valid=false;
    if((kind==1||kind==4)&&!f.load_contacts)f.valid=false;
    if(performance&&((hz==60&&p95>2)||stall>50))f.valid=false;
    printf("%s kind=%u ramp=%d hz=%u delayed=%d time=%.9g steps=%llu depth=%.9g pair=%u/%u energy_error=%.9g self=%llu speed=%.9g final_speed=%.9g p95_ms=%.6f stall_ms=%.6f\n",f.valid?"PASS":"FAIL",kind,ramp,hz,delayed,f.time,(unsigned long long)f.substeps,f.depth,f.depth_a,f.depth_b,f.energy_error,(unsigned long long)f.self_contacts,f.peak_speed,f.final_speed,p95,stall);
    printf("contacts particle=%llu load=%llu tunneled=%d\n",(unsigned long long)f.particle_contacts,(unsigned long long)f.load_contacts,f.tunneled);
    printf("profile_ms integration=%.3f gather=%.3f solve=%.3f\n",integration,gather,solve);
    printf("completion condition=time target=%.9g reached=%d reason=%s max_cpu_ms=%.6f\n",
        end,complete,f.stop_reason?f.stop_reason:complete?"endpoint_reached":"endpoint_not_reached",
        fmax(stall,count?samples[count-1]:0));
    fflush(stdout);rohr_engine_stop();return f.valid;
}
int main(int argc,char **argv) {
    signal(SIGINT,interrupt_test);signal(SIGTERM,interrupt_test);bool valid=true,performance=false,quick=false,only_delayed=false;int single=-1;
    for(int i=1;i<argc;++i){if(!strcmp(argv[i],"--performance"))performance=true;else if(!strcmp(argv[i],"--quick"))quick=true;else if(!strcmp(argv[i],"--delayed"))only_delayed=true;else single=atoi(argv[i]);}
    const unsigned rates[]={30,60,120,60};
    for(unsigned k=0;k<7&&!interrupted;++k)if(single<0||(unsigned)single==k)
        for(unsigned ramp=0;ramp<(k<5?2u:1u);++ramp)for(unsigned r=0;r<4&&!interrupted;++r)if((!quick||r==1)&&(!only_delayed||r==3))
            valid=run(k,ramp,rates[r],r==3,performance)&&valid;
    return valid&&!interrupted?0:1;
}
