#include <mtm_i.h>

#ifdef TLS
__thread mtm_tx_t* _mtm_thread_tx;
#else /* ! TLS */
pthread_key_t _mtm_thread_tx;
#endif /* ! TLS */

volatile mtm_word_t locks[LOCK_ARRAY_SIZE];

#ifdef CLOCK_IN_CACHE_LINE
/* At least twice a cache line (512 bytes to be on the safe side) */
volatile mtm_word_t gclock[1024 / sizeof(mtm_word_t)];
#else /* ! CLOCK_IN_CACHE_LINE */
volatile mtm_word_t gclock;
#endif /* ! CLOCK_IN_CACHE_LINE */


int vr_threshold;
int cm_threshold;

uint64_t mtm_spin_count_var = 1000;

mtm_rwlock_t mtm_serial_lock;
