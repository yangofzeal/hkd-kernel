#define _POSIX_C_SOURCE 200809L
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "hkd_kernel.h"
static double now_s(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return(double)t.tv_sec+(double)t.tv_nsec*1e-9;}
static uint64_t xs=UINT64_C(0x9e3779b97f4a7c15);
static uint64_t rng(void){xs^=xs<<7;xs^=xs>>9;xs^=xs<<8;return xs;}
int main(int argc,char**argv){
    size_t n=argc>1?strtoull(argv[1],0,10):2000000;
    size_t updates=argc>2?strtoull(argv[2],0,10):300;
    uint64_t *qty=malloc(n*sizeof(*qty)),*p_hkd=malloc(n*sizeof(*p_hkd)),*p_ref=malloc(n*sizeof(*p_ref));
    size_t *idx=malloc(updates*sizeof(*idx)); uint64_t *val=malloc(updates*sizeof(*val));
    if(!qty||!p_hkd||!p_ref||!idx||!val)return 2;
    for(size_t i=0;i<n;i++){qty[i]=1+(rng()%100);p_hkd[i]=1000+(rng()%100000);p_ref[i]=p_hkd[i];}
    for(size_t j=0;j<updates;j++){idx[j]=rng()%n;val[j]=1000+(rng()%100000);}
    hkd_weighted_u64_state s;if(hkd_weighted_state_init(&s,qty,p_hkd,n))return 3;
    volatile uint64_t ref_sink=0,hkd_sink=0;
    double t0=now_s();
    for(size_t j=0;j<updates;j++){p_ref[idx[j]]=val[j];ref_sink=hkd_weighted_sum_u64(qty,p_ref,n);} double t1=now_s();
    double t2=now_s();
    for(size_t j=0;j<updates;j++){if(hkd_weighted_state_update_value(&s,idx[j],val[j]))return 4;hkd_sink=hkd_weighted_state_total(&s);} double t3=now_s();
    uint64_t verify=hkd_weighted_sum_u64(qty,p_hkd,n);
    int exact=(verify==hkd_sink)&&(ref_sink==hkd_sink);
    double base=t1-t0,hot=t3-t2;
    printf("HKD_APPLICATION_PORTFOLIO_BENCH\n");
    printf("instruments=%zu price_updates=%zu\n",n,updates);
    printf("baseline_full_revalue_s=%.9f\n",base);
    printf("hkd_incremental_revalue_s=%.9f\n",hot);
    printf("speedup_x=%.2f\n",base/hot);
    printf("baseline_ns_per_update=%.1f\n",base*1e9/updates);
    printf("hkd_ns_per_update=%.1f\n",hot*1e9/updates);
    printf("exact_state_match=%s\n",exact?"True":"False");
    printf("final_exposure=%" PRIu64 "\n",(uint64_t)hkd_sink);
    free(qty);free(p_hkd);free(p_ref);free(idx);free(val);return exact?0:5;
}
