#include <gtest/gtest.h>
#include <pthread.h>

extern "C" {
#include "atomic.h"
}

/* Use a mutex+condvar instead of a spin loop so valgrind doesn't deadlock */
static pthread_mutex_t g_mu  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_cv  = PTHREAD_COND_INITIALIZER;
static atomic_t        g_flag  = 0;
static atomic_t        g_value = 0;

static void *writer_thread(void *) {
    ATOMIC_STORE(&g_value, 42);
    ATOMIC_MB_FULL;

    pthread_mutex_lock(&g_mu);
    ATOMIC_STORE(&g_flag, 1);
    pthread_cond_signal(&g_cv);
    pthread_mutex_unlock(&g_mu);
    return nullptr;
}

static void *reader_thread(void *) {
    pthread_mutex_lock(&g_mu);
    while (ATOMIC_LOAD_ACQ(&g_flag) == 0)
        pthread_cond_wait(&g_cv, &g_mu);
    pthread_mutex_unlock(&g_mu);

    ATOMIC_MB_FULL;
    EXPECT_EQ((int)ATOMIC_LOAD(&g_value), 42);
    return nullptr;
}

TEST(MemoryBarriers, WriterFlagVisibleAfterValue) {
    g_flag = g_value = 0;
    pthread_t w, r;
    ASSERT_EQ(pthread_create(&r, nullptr, reader_thread, nullptr), 0);
    ASSERT_EQ(pthread_create(&w, nullptr, writer_thread, nullptr), 0);
    pthread_join(w, nullptr);
    pthread_join(r, nullptr);
}
