/*
 * prime — minimal mnemosyne priming binary.
 *
 * Links against libmnemosyne.so so the library constructor runs on startup,
 * which creates and initialises all persistent segment files
 * (segment table, log pool, PERSISTENT section backing store).
 *
 * No threads, no PTx, no pmalloc — exits immediately after init.
 * Used by CI to prime /dev/shm/psegments before integration tests run.
 */
#include <mnemosyne.h>

__attribute__((section("PERSISTENT"))) static int _prime_flag = 0;

int main(void)
{
    (void)_prime_flag;
    return 0;
}
