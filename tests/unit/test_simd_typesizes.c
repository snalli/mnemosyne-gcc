/*
 * Verifies that the portable SIMD type aliases (_ITM_TYPE_M64/128/256)
 * have the correct size and alignment on every target architecture.
 * On x86/x86_64 these are the native __m64/__m128/__m256 types;
 * on other architectures they are aligned byte-array structs.
 */
#include <assert.h>
#include <stddef.h>

/* Pull in the type definitions via the mcore internal header */
#include "mcore_i.h"

int main(void)
{
    assert(sizeof(_ITM_TYPE_M64)  == 8);
    assert(sizeof(_ITM_TYPE_M128) == 16);
    assert(sizeof(_ITM_TYPE_M256) == 32);

    assert(_Alignof(_ITM_TYPE_M64)  >= 8);
    assert(_Alignof(_ITM_TYPE_M128) >= 16);
    /* __m256 alignment is 32 only with -mavx; without it GCC uses 16.
     * Accept >= 16 so the test is valid on all x86 and non-x86 targets. */
    assert(_Alignof(_ITM_TYPE_M256) >= 16);

    return 0;
}
