#include <gtest/gtest.h>
#include <pthread.h>

extern "C" {
#include "atomic.h"
}

static atomic_t g_flag  = 0;
static atomic_t g_value = 0;

static void *writer_thread(void *) {
    ATOMIC_STORE(&g_value, 42);
    ATOMIC_MB_FULL;
    ATOMIC_STORE(&g_flag, 1);
    return nullptr;
}

static void *reader_thread(void *) {
    while (ATOMIC_LOAD_ACQ(&g_flag) == 0)
        ;
    ATOMIC_MB_FULL;
    /* Value must be visible after the flag */
    EXPECT_EQ((int)ATOMIC_LOAD(&g_value), 42);
    return nullptr;
}

TEST(MemoryBarriers, WriterFlagVisibleAfterValue) {
    g_flag = g_value = 0;
    pthread_t w, r;
    ASSERT_EQ(pthread_create(&w, nullptr, writer_thread, nullptr), 0);
    ASSERT_EQ(pthread_create(&r, nullptr, reader_thread, nullptr), 0);
    pthread_join(w, nullptr);
    pthread_join(r, nullptr);
}
