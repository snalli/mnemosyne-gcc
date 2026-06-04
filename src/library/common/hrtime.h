/*
    Copyright (C) 2011 Computer Sciences Department,
    University of Wisconsin -- Madison

    This file is part of Mnemosyne: Lightweight Persistent Memory.

    Portable high-resolution timer using POSIX clock_gettime().
    Original x86 rdtsc implementation replaced for architecture portability.
*/

#ifndef _HRTIME_H_121AJ1
#define _HRTIME_H_121AJ1

#include <time.h>
#include <stdint.h>

typedef unsigned long long hrtime_t;

/* Return current time in nanoseconds */
static inline hrtime_t hrtime_cycles(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (hrtime_t)ts.tv_sec * 1000000000ULL + (hrtime_t)ts.tv_nsec;
}

/* Full memory barrier — replaces the old cpuid serialisation */
static inline void hrtime_barrier(void)
{
    __sync_synchronize();
}

/* These macros existed in the original for cycle<->ns conversion.
 * With clock_gettime returning ns directly, CYCLE==NS. */
#ifndef _HRTIME_CPUFREQ
# define _HRTIME_CPUFREQ 1  /* unused, kept for source compatibility */
#endif
#define HRTIME_NS2CYCLE(__ns)     (__ns)
#define HRTIME_CYCLE2NS(__cycles) (__cycles)

#endif /* _HRTIME_H_121AJ1 */
