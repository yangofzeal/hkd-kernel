#ifndef HKD_KERNEL_H
#define HKD_KERNEL_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t *data;
    size_t n;
    uint64_t total;
} hkd_u64_state;

uint64_t hkd_sum_u64(const uint64_t *data, size_t n);
int hkd_state_init(hkd_u64_state *s, uint64_t *data, size_t n);
int hkd_state_update(hkd_u64_state *s, size_t index, uint64_t value);
uint64_t hkd_state_total(const hkd_u64_state *s);

typedef struct {
    uint64_t *weights;
    uint64_t *values;
    size_t n;
    uint64_t total;
} hkd_weighted_u64_state;

uint64_t hkd_weighted_sum_u64(const uint64_t *weights, const uint64_t *values, size_t n);
int hkd_weighted_state_init(hkd_weighted_u64_state *s, uint64_t *weights, uint64_t *values, size_t n);
int hkd_weighted_state_update_value(hkd_weighted_u64_state *s, size_t index, uint64_t value);
uint64_t hkd_weighted_state_total(const hkd_weighted_u64_state *s);

#ifdef __cplusplus
}
#endif
#endif
