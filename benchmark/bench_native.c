#define _POSIX_C_SOURCE 200809L
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "hkd_kernel.h"

static double now_s(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}
static uint64_t xs = UINT64_C(0x123456789abcdef0);
static uint64_t rng(void) { xs ^= xs << 7; xs ^= xs >> 9; xs ^= xs << 8; return xs; }

int main(int argc, char **argv) {
    size_t n = argc > 1 ? strtoull(argv[1], 0, 10) : 4000000;
    size_t updates = argc > 2 ? strtoull(argv[2], 0, 10) : 200;
    uint64_t *a = malloc(n * sizeof(*a));
    uint64_t *b = malloc(n * sizeof(*b));
    size_t *idx = malloc(updates * sizeof(*idx));
    uint64_t *val = malloc(updates * sizeof(*val));
    if (!a || !b || !idx || !val) return 2;

    for (size_t i=0;i<n;i++) a[i] = (rng() & 0xffffu);
    for (size_t i=0;i<n;i++) b[i] = a[i];
    for (size_t j=0;j<updates;j++) { idx[j] = rng()%n; val[j] = rng() & 0xffffu; }

    hkd_u64_state s;
    if (hkd_state_init(&s, a, n)) return 3;

    volatile uint64_t baseline_sink = 0, hkd_sink = 0;
    double t0 = now_s();
    for (size_t j=0;j<updates;j++) {
        b[idx[j]] = val[j];
        baseline_sink = hkd_sum_u64(b, n);
    }
    double t1 = now_s();

    double t2 = now_s();
    for (size_t j=0;j<updates;j++) {
        if (hkd_state_update(&s, idx[j], val[j])) return 4;
        hkd_sink = hkd_state_total(&s);
    }
    double t3 = now_s();

    uint64_t verify = hkd_sum_u64(a, n);
    int exact = (verify == hkd_sink) && (baseline_sink == hkd_sink);
    double base = t1-t0, hot=t3-t2;
    printf("HKD_KERNEL_NATIVE_BENCH\n");
    printf("n=%zu updates=%zu\n", n, updates);
    printf("baseline_full_recompute_s=%.9f\n", base);
    printf("hkd_incremental_update_s=%.9f\n", hot);
    printf("speedup_x=%.2f\n", base/hot);
    printf("baseline_ns_per_update=%.1f\n", base*1e9/updates);
    printf("hkd_ns_per_update=%.1f\n", hot*1e9/updates);
    printf("exact_state_match=%s\n", exact ? "True" : "False");
    printf("final_total=%" PRIu64 "\n", (uint64_t)hkd_sink);
    free(a); free(b); free(idx); free(val);
    return exact ? 0 : 5;
}
