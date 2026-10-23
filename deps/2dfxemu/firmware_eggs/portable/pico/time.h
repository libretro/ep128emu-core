#ifndef TWODFX_EGG_PORTABLE_TIME_H
#define TWODFX_EGG_PORTABLE_TIME_H
#include <stdint.h>
#include <time.h>
/* Diagnostic-only wall time: not a simulation of MCU render duration. */
static inline uint32_t time_us_32(void) {
    return (uint32_t)((uint64_t)clock()*1000000u/CLOCKS_PER_SEC);
}
#endif
