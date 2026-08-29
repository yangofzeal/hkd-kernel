#define _POSIX_C_SOURCE 200809L
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "hkd_kernel.h"

#if defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
static uint64_t reference_sum(const uint64_t *a, size_t n) {
    uint64_t s = 0;
    for (size_t i = 0; i < n; ++i) s += a[i];
    return s;
}

static double now_s(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

int main(int argc, char **argv) {
    size_t n = argc > 1 ? strtoull(argv[1], 0, 10) : 8000000;
    size_t reps = argc > 2 ? strtoull(argv[2], 0, 10) : 40;
    uint64_t *a = malloc(n * sizeof(*a));
    if (!a) return 2;
    for (size_t i=0;i<n;i++) a[i]=(uint64_t)((i*11400714819323198485ull)>>48);

    volatile uint64_t r0=0, r1=0;
    double t0=now_s();
    for (size_t j=0;j<reps;j++) r0 ^= reference_sum(a,n);
    double t1=now_s();
    double t2=now_s();
    for (size_t j=0;j<reps;j++) r1 ^= hkd_sum_u64(a,n);
    double t3=now_s();
    uint64_t check0=reference_sum(a,n), check1=hkd_sum_u64(a,n);
    int exact=(check0==check1) && (r0==r1);
    double base=t1-t0, hkd=t3-t2;
    printf("HKD_DENSE_CONTROL_BENCH\n");
    printf("n=%zu reps=%zu\n",n,reps);
    printf("reference_dense_s=%.9f\n",base);
    printf("hkd_dense_s=%.9f\n",hkd);
    printf("speedup_x=%.4f\n",base/hkd);
    printf("exact_state_match=%s\n",exact?"True":"False");
    printf("checksum=%" PRIu64 "\n",check1);
    free(a);
    return exact?0:5;
}
