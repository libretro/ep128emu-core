#ifndef TWODFX_EGG_PORTABLE_RAND_H
#define TWODFX_EGG_PORTABLE_RAND_H
#include <stdint.h>
/* Portable deterministic fallback, never hardware TRNG. */
static inline uint32_t get_rand_32(void) {
    static uint32_t state = 0x9e3779b9u;
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return state;
}
#endif
