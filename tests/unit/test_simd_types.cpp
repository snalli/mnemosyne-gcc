#include <gtest/gtest.h>

extern "C" {
#include "mcore_i.h"
}

TEST(SimdTypes, M64Size)  { EXPECT_EQ(sizeof(_ITM_TYPE_M64),  8U); }
TEST(SimdTypes, M128Size) { EXPECT_EQ(sizeof(_ITM_TYPE_M128), 16U); }
TEST(SimdTypes, M256Size) { EXPECT_EQ(sizeof(_ITM_TYPE_M256), 32U); }

TEST(SimdTypes, Alignments) {
    EXPECT_GE(alignof(_ITM_TYPE_M64),  8U);
    EXPECT_GE(alignof(_ITM_TYPE_M128), 16U);
    EXPECT_GE(alignof(_ITM_TYPE_M256), 16U); /* 16 min; 32 with -mavx */
}
