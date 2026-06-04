/*
 * Integration test: persistent malloc/free.
 * Ported from tests/test/malloc/ (UnitTest++ → GTest).
 *
 * Requires: libmnemosyne.so, /dev/shm/psegments, mnemosyne.ini
 */
#include <gtest/gtest.h>

extern "C" {
#include <mnemosyne.h>
#include <mtm.h>
#include <pmalloc.h>
}
#include "tm_def.h"

/* Persistent pointer lives in the PERSISTENT section */
__attribute__((section("PERSISTENT"))) static void *ptr = nullptr;

TEST(PersistentMalloc, SmallAllocationSucceeds) {
    PTx { ptr = pmalloc(16); }
    EXPECT_NE(ptr, nullptr);
}

TEST(PersistentMalloc, SmallFreeSucceeds) {
    if (!ptr) { PTx { ptr = pmalloc(16); } }
    PTx { pfree(ptr); ptr = nullptr; }
    EXPECT_EQ(ptr, nullptr);
}

TEST(PersistentMalloc, LargeAllocationSucceeds) {
    PTx { ptr = pmalloc(1024); }
    EXPECT_NE(ptr, nullptr);
}

TEST(PersistentMalloc, LargeFreeSucceeds) {
    if (!ptr) { PTx { ptr = pmalloc(1024); } }
    PTx { pfree(ptr); ptr = nullptr; }
    EXPECT_EQ(ptr, nullptr);
}

TEST(PersistentMalloc, MultipleAllocations) {
    void *ptrs[8] = {};
    for (int i = 0; i < 8; i++) {
        PTx { ptrs[i] = pmalloc(64 * (i + 1)); }
        EXPECT_NE(ptrs[i], nullptr);
    }
    for (int i = 0; i < 8; i++) {
        PTx { pfree(ptrs[i]); }
    }
}
