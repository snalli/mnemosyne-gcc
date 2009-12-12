ROLLOVER_CLOCK = True
CLOCK_IN_CACHE_LINE = True
NO_DUPLICATES_IN_RW_SETS = True
WAIT_YIELD = True
USE_BLOOM_FILTER = True
EPOCH_GC = True
CONFLICT_TRACKING = True

########################################################################
# Allow transactions to read the previous version of locked memory
# locations, as in the original LSA algorithm (see [DISC-06]).  This is
# achieved by peeking into the write set of the transaction that owns
# the lock.  There is a small overhead with non-contended workloads but
# it may significantly reduce the abort rate, especially with
# transactions that read much data.  This feature only works with the
# WRITE_BACK_ETL design and requires EPOCH_GC.
########################################################################

READ_LOCKED_DATA = True

########################################################################
# Tweak the hash function that maps addresses to locks so that
# consecutive addresses do not map to consecutive locks.  This can avoid
# cache line invalidations for application that perform sequential
# memory accesses.  The last byte of the lock index is swapped with the
# previous byte.
########################################################################

LOCK_IDX_SWAP = True
