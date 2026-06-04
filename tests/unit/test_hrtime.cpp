#include <gtest/gtest.h>

extern "C" {
#include "hrtime.h"
}

TEST(HrTime, IsPositive) {
    EXPECT_GT(hrtime_cycles(), (hrtime_t)0);
}

TEST(HrTime, IsMonotonic) {
    hrtime_t t1 = hrtime_cycles();
    hrtime_t t2 = hrtime_cycles();
    EXPECT_GE(t2, t1);
}

TEST(HrTime, MacrosAreIdentity) {
    EXPECT_EQ(HRTIME_NS2CYCLE(1000), 1000ULL);
    EXPECT_EQ(HRTIME_CYCLE2NS(1000), 1000ULL);
}

TEST(HrTime, BarrierDoesNotCrash) {
    hrtime_barrier();
}
