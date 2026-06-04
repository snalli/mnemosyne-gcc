/*
 * Tests for the portable high-resolution timer (hrtime.h).
 * The original used x86 rdtsc; the replacement uses clock_gettime.
 */
#include <assert.h>
#include "hrtime.h"

int main(void)
{
    hrtime_t t1 = hrtime_cycles();
    assert(t1 > 0);

    hrtime_t t2 = hrtime_cycles();
    assert(t2 >= t1); /* monotonically non-decreasing */

    /* Cycle<->ns conversion macros must be identity with the portable impl */
    assert(HRTIME_NS2CYCLE(1000) == 1000);
    assert(HRTIME_CYCLE2NS(1000) == 1000);

    /* Barrier must compile and run without faulting */
    hrtime_barrier();

    return 0;
}
