#include <gtest/gtest.h>

extern "C" {
#include "atomic.h"
}

TEST(Atomics, CasSucceedsWhenExpectedMatches) {
    atomic_t val = 10;
    EXPECT_EQ(ATOMIC_CAS_FULL(&val, 10, 20), 1);
    EXPECT_EQ((int)val, 20);
}

TEST(Atomics, CasFailsWhenExpectedMismatches) {
    atomic_t val = 20;
    EXPECT_EQ(ATOMIC_CAS_FULL(&val, 10, 99), 0);
    EXPECT_EQ((int)val, 20);
}

TEST(Atomics, FetchIncrementReturnsOldValue) {
    atomic_t val = 20;
    EXPECT_EQ((int)ATOMIC_FETCH_INC_FULL(&val), 20);
    EXPECT_EQ((int)val, 21);
}

TEST(Atomics, FetchDecrementReturnsOldValue) {
    atomic_t val = 21;
    EXPECT_EQ((int)ATOMIC_FETCH_DEC_FULL(&val), 21);
    EXPECT_EQ((int)val, 20);
}

TEST(Atomics, FetchAddReturnsOldValue) {
    atomic_t val = 20;
    EXPECT_EQ((int)ATOMIC_FETCH_ADD_FULL(&val, 5), 20);
    EXPECT_EQ((int)val, 25);
}

TEST(Atomics, LoadStoreRoundtrip) {
    atomic_t val = 0;
    ATOMIC_STORE(&val, 42);
    EXPECT_EQ((int)ATOMIC_LOAD(&val), 42);
}
