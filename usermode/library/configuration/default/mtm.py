########################################################################
#                                                                    
#       PERSISTENT MEMORY TRANSACTIONS BUILD-TIME CONFIGURATION              
#                                                                    
########################################################################



########################################################################
# ALLOW_ABORTS: Allows transaction aborts. When disabled and 
#   combined with no-isolation, the TM system does not need to perform 
#   version management for volatile data.
########################################################################

ALLOW_ABORTS = False

########################################################################
# SYNC_TRUNCATION: Synchronously flushes the write set out of the HW
#   cache and truncates the persistent log. 
########################################################################

SYNC_TRUNCATION = True

########################################################################
# TMLOG_TYPE: Determines the type of the persistent log used. 
# 
# TMLOG_TYPE_BASE: a simple log that synchronously updates the tail on
#   each transaction commit/abort to properly keep track of the log 
#   limits.
#   
# TMLOG_TYPE_TORNBIT: a log that reserves a torn bit per 64-bit word to 
#   detect writes that did not make it to persistent storage. Does not
#   require updating the tail on each transaction commit/abort.
########################################################################

TMLOG_TYPE = 'TMLOG_TYPE_BASE'


########################################################################
# FLUSH_CACHELINE_ONCE: When asynchronously truncating the log, the log 
#   manager flushes each cacheline of the write set only once by keeping 
#   track flushed cachelines. This adds some bookkeeping overhead which 
#   for some workloads might be worse than simply flushing a cacheline 
#   multiple times.
########################################################################

FLUSH_CACHELINE_ONCE = False

########################################################################
########################################################################
########################################################################



########################################################################
#                                                                    
#                           GENERIC FLAGS                           
#                                                                    
########################################################################


########################################################################
# Maintain detailed internal statistics.  Statistics are stored in
# thread locals and do not add much overhead, so do not expect much gain
# from disabling them.
########################################################################

INTERNAL_STATS = True

########################################################################
# Roll over clock when it reaches its maximum value.  Clock rollover can
# be safely disabled on 64 bits to save a few cycles, but it is
# necessary on 32 bits if the application executes more than 2^28
# (write-through) or 2^31 (write-back) transactions.
########################################################################

ROLLOVER_CLOCK = True

########################################################################
# Ensure that the global clock does not share the same cache line than
# some other variable of the program.  This should be normally enabled.
########################################################################

CLOCK_IN_CACHE_LINE = True

########################################################################
# Prevent duplicate entries in read/write sets when accessing the same
# address multiple times.  Enabling this option may reduce performance
# so leave it disabled unless transactions repeatedly read or write the
# same address.
########################################################################

NO_DUPLICATES_IN_RW_SETS = True

########################################################################
# Yield the processor when waiting for a contended lock to be released.
# This only applies to the CM_WAIT and CM_PRIORITY contention managers.
########################################################################

WAIT_YIELD = True

########################################################################
# Use an epoch-based memory allocator and garbage collector to ensure
# that accesses to the dynamic memory allocated by a transaction from
# another transaction are valid.  There is a slight overhead from
# enabling this feature.
########################################################################

EPOCH_GC = False

########################################################################
# Keep track of conflicts between transactions and notifies the
# application (using a callback), passing the identity of the two
# conflicting transaction and the associated threads.  This feature
# requires EPOCH_GC.
########################################################################

CONFLICT_TRACKING = False

########################################################################
# Allow transactions to read the previous version of locked memory
# locations, as in the original LSA algorithm (see [DISC-06]).  This is
# achieved by peeking into the write set of the transaction that owns
# the lock.  There is a small overhead with non-contended workloads but
# it may significantly reduce the abort rate, especially with
# transactions that read much data.  This feature only works with the
# WRITE_BACK_ETL design and requires EPOCH_GC.
########################################################################

READ_LOCKED_DATA = False

########################################################################
# Tweak the hash function that maps addresses to locks so that
# consecutive addresses do not map to consecutive locks.  This can avoid
# cache line invalidations for application that perform sequential
# memory accesses.  The last byte of the lock index is swapped with the
# previous byte.
########################################################################

LOCK_IDX_SWAP = True

########################################################################
# Several contention management strategies are available:
#
# CM_SUICIDE: immediately abort the transaction that detects the
#   conflict.
#
# CM_DELAY: like CM_SUICIDE but wait until the contended lock that
#   caused the abort (if any) has been released before restarting the
#   transaction.  The intuition is that the transaction will likely try
#   again to acquire the same lock and might fail once more if it has
#   not been released.  In addition, this increases the chances that the
#   transaction can succeed with no interruption upon retry, which
#   improves execution time on the processor.
#
# CM_BACKOFF: like CM_SUICIDE but wait for a random delay before
#   restarting the transaction.  The delay duration is chosen uniformly
#   at random from a range whose size increases exponentially with every
#   restart.
#
# CM_PRIORITY: cooperative priority-based contention manager that avoids
#   livelocks.  It only works with the ETL-based design (WRITE_BACK_ETL
#   or WRITE_THROUGH).  The principle is to give preference to
#   transactions that have already aborted many times.  Therefore, a
#   priority is associated to each transaction and it increases with the
#   number of retries.
# 
#   A transaction that tries to acquire a lock can "reserve" it if it is
#   currently owned by another transaction with lower priority.  If the
#   latter is blocked waiting for another lock, it will detect that the
#   former is waiting for the lock and will abort.  As with CM_DELAY,
#   before retrying after failing to acquire some lock, we wait until
#   the lock we were waiting for is released.
#
#   If a transaction fails because of a read-write conflict (detected
#   upon validation at commit time), we do not increase the priority.
#   It such a failure occurs sufficiently enough (e.g., three times in a
#   row, can be parametrized), we switch to visible reads.
#
#   When using visible reads, each read is implemented as a write and we
#   do not allow multiple readers.  The reasoning is that (1) visible
#   reads are expected to be used rarely, (2) supporting multiple
#   readers is complex and has non-negligible overhead, especially if
#   fairness must be guaranteed, e.g., to avoid writer starvation, and
#   (3) having a single reader makes lock upgrade trivial.
#
#   To implement cooperative contention management, we associate a
#   priority to each transaction.  The priority is used to avoid
#   deadlocks and to decide which transaction can proceed or must abort
#   upon conflict.  Priorities can vary between 0 and MAX_PRIORITY.  By
#   default we use 3 bits, i.e., MAX_PRIORITY=7, and we use the number
#   of retries of a transaction to specify its priority.  The priority
#   of a transaction is encoded in the locks (when the lock bit is set).
#   If the number of concurrent transactions is higher than
#   MAX_PRIORITY+1, the properties of the CM (bound on the number of
#   retries) might not hold.
#
#   The priority contention manager can be activated only after a
#   configurable number of retries.  Until then, CM_SUICIDE is used.
########################################################################

CM = 'CM_SUICIDE'

########################################################################
# RW_SET_SIZE: initial size of the read and write sets. These sets will
#   grow dynamically when they become full.
########################################################################

RW_SET_SIZE = 32768

########################################################################
# LOCK_ARRAY_LOG_SIZE (default=20): number of bits used for indexes in
#   the lock array.  The size of the array will be 2 to the power of
#   LOCK_ARRAY_LOG_SIZE.
########################################################################

LOCK_ARRAY_LOG_SIZE = 20

########################################################################
# LOCK_SHIFT_EXTRA (default=2): additional shifts to apply to the
#   address when determining its index in the lock array.  This controls
#   how many consecutive memory words will be covered by the same lock
#   (2 to the power of LOCK_SHIFT_EXTRA).  Higher values will increase
#   false sharing but reduce the number of CASes necessary to acquire
#   locks and may avoid cache line invalidations on some workloads.  As
#   shown in [PPoPP-08], a value of 2 seems to offer best performance on
#   many benchmarks.
########################################################################

LOCK_SHIFT_EXTRA = 2

########################################################################
# PRIVATE_LOCK_ARRAY_LOG_SIZE (default=20): number of bits used for indexes 
#   in the private pseudo-lock array.  The size of the array will be 2 to 
#   the power of  PRIVATE_LOCK_ARRAY_LOG_SIZE.
########################################################################

PRIVATE_LOCK_ARRAY_LOG_SIZE = 8


########################################################################
# MIN_BACKOFF (default=0x04UL) and MAX_BACKOFF (default=0x80000000UL):
#   minimum and maximum values of the exponential backoff delay.  This
#   parameter is only used with the CM_BACKOFF contention manager.
########################################################################

MIN_BACKOFF = 0x04
MAX_BACKOFF = 0x80000000

########################################################################
# VR_THRESHOLD_DEFAULT (default=3): number of aborts due to failed
#   validation before switching to visible reads.  A value of 0
#   indicates no limit.  This parameter is only used with the
#   CM_PRIORITY contention manager.  It can also be set using an
#   environment variable of the same name.
########################################################################

VR_THRESHOLD_DEFAULT = 3

########################################################################
# CM_THRESHOLD_DEFAUL: number of executions of the transaction with a 
#   CM_SUICIDE contention management strategy before switching to 
#   CM_PRIORITY.  This parameter is only used with the CM_PRIORITY 
#   contention manager.  It can also be set using an environment 
#   variable of the same name.
########################################################################

CM_THRESHOLD_DEFAULT = 0
