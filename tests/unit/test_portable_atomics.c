/*
 * Tests for the portable atomic operations in atomic.h.
 * These replace the old libatomic-ops (AO_*) dependency with
 * GCC __atomic_* builtins.
 */
#include <assert.h>
#include <stdint.h>
#include "atomic.h"

int main(void)
{
    atomic_t val = 10;

    /* CAS succeeds when expected matches */
    assert(ATOMIC_CAS_FULL(&val, 10, 20) == 1);
    assert(val == 20);

    /* CAS fails when expected does not match */
    assert(ATOMIC_CAS_FULL(&val, 10, 99) == 0);
    assert(val == 20);

    /* fetch-and-increment returns old value */
    atomic_t old = ATOMIC_FETCH_INC_FULL(&val);
    assert(old == 20);
    assert(val == 21);

    /* fetch-and-decrement returns old value */
    old = ATOMIC_FETCH_DEC_FULL(&val);
    assert(old == 21);
    assert(val == 20);

    /* fetch-and-add */
    old = ATOMIC_FETCH_ADD_FULL(&val, 5);
    assert(old == 20);
    assert(val == 25);

    /* load / store */
    ATOMIC_STORE(&val, 42);
    assert(ATOMIC_LOAD(&val) == 42);

    return 0;
}
