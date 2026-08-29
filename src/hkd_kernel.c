#include "hkd_kernel.h"

uint64_t hkd_sum_u64(const uint64_t *data, size_t n) {
    uint64_t acc = 0;
    for (size_t i = 0; i < n; ++i) acc += data[i];
    return acc;
}

int hkd_state_init(hkd_u64_state *s, uint64_t *data, size_t n) {
    if (!s || (!data && n)) return -1;
    s->data = data;
    s->n = n;
    s->total = hkd_sum_u64(data, n);
    return 0;
}

int hkd_state_update(hkd_u64_state *s, size_t index, uint64_t value) {
    if (!s || index >= s->n) return -1;
    uint64_t old = s->data[index];
    s->data[index] = value;
    s->total += value - old;
    return 0;
}

uint64_t hkd_state_total(const hkd_u64_state *s) {
    return s ? s->total : 0;
}

uint64_t hkd_weighted_sum_u64(const uint64_t *weights, const uint64_t *values, size_t n) {
    uint64_t acc = 0;
    for (size_t i = 0; i < n; ++i) acc += weights[i] * values[i];
    return acc;
}

int hkd_weighted_state_init(hkd_weighted_u64_state *s, uint64_t *weights, uint64_t *values, size_t n) {
    if (!s || ((!weights || !values) && n)) return -1;
    s->weights = weights;
    s->values = values;
    s->n = n;
    s->total = hkd_weighted_sum_u64(weights, values, n);
    return 0;
}

int hkd_weighted_state_update_value(hkd_weighted_u64_state *s, size_t index, uint64_t value) {
    if (!s || index >= s->n) return -1;
    uint64_t old = s->values[index];
    s->values[index] = value;
    s->total += s->weights[index] * (value - old);
    return 0;
}

uint64_t hkd_weighted_state_total(const hkd_weighted_u64_state *s) {
    return s ? s->total : 0;
}
