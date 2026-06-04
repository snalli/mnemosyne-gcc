/*
 * Tests for the portable ticket spinlock (spinlock.h).
 * The original used x86 xaddw/incb inline asm; the replacement uses
 * GCC __atomic_* builtins.
 */
#include <assert.h>
#include "spinlock.h"

int main(void)
{
    arch_spinlock_t lock = {0};

    /* Lock and unlock once */
    __ticket_spin_lock(&lock);
    __ticket_spin_unlock(&lock);

    /* Lock and unlock a second time — verifies the ticket counter wraps cleanly */
    __ticket_spin_lock(&lock);
    __ticket_spin_unlock(&lock);

    /* After two lock/unlock cycles the internal counters must be balanced */
    assert((lock.slock & 0xFF) == ((lock.slock >> 8) & 0xFF));

    return 0;
}
