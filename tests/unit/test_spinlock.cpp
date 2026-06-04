#include <gtest/gtest.h>
#include <pthread.h>

extern "C" {
#include "spinlock.h"
}

TEST(Spinlock, LockUnlockOnce) {
    arch_spinlock_t lock = {0};
    __ticket_spin_lock(&lock);
    __ticket_spin_unlock(&lock);
}

TEST(Spinlock, LockUnlockTwice) {
    arch_spinlock_t lock = {0};
    __ticket_spin_lock(&lock);
    __ticket_spin_unlock(&lock);
    __ticket_spin_lock(&lock);
    __ticket_spin_unlock(&lock);
}

TEST(Spinlock, CountersBalancedAfterRelease) {
    arch_spinlock_t lock = {0};
    __ticket_spin_lock(&lock);
    __ticket_spin_unlock(&lock);
    EXPECT_EQ(lock.slock & 0xFF, (lock.slock >> 8) & 0xFF);
}

/* Concurrent mutual-exclusion test */
static arch_spinlock_t g_lock = {0};
static volatile long   g_counter = 0;

static void *increment_worker(void *arg) {
    int iters = *(int *)arg;
    for (int i = 0; i < iters; i++) {
        __ticket_spin_lock(&g_lock);
        g_counter++;
        __ticket_spin_unlock(&g_lock);
    }
    return nullptr;
}

TEST(Spinlock, ConcurrentMutualExclusion) {
    const int NTHREADS = 4, ITERS = 10000;
    g_counter = 0;
    g_lock = {0};

    pthread_t threads[8];
    int iters = ITERS;
    for (int i = 0; i < NTHREADS; i++)
        ASSERT_EQ(pthread_create(&threads[i], nullptr, increment_worker, &iters), 0);
    for (int i = 0; i < NTHREADS; i++)
        pthread_join(threads[i], nullptr);

    EXPECT_EQ(g_counter, (long)NTHREADS * ITERS);
}
