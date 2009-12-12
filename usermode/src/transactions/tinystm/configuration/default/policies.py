########################################################################
# Three different designs can be chosen from, which differ in when locks
# are acquired (encounter-time vs. commit-time), and when main memory is
# updated (write-through vs. write-back).
#
# WRITE_BACK_ETL: write-back with encounter-time locking acquires lock
#   when encountering write operations and buffers updates (they are
#   committed to main memory at commit time).
#
# WRITE_BACK_CTL: write-back with commit-time locking delays acquisition
#   of lock until commit time and buffers updates.
#
# WRITE_THROUGH: write-through (encounter-time locking) directly updates
#   memory and keeps an undo log for possible rollback.
#
# Refer to [PPoPP-08] for more details.
########################################################################

DESIGN = 'WRITE_BACK_ETL'


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
# CM_DELAY: like CM_SUICIDE but wait for a random delay before
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

CONFLICT_MANAGER = 'CM_SUICIDE'


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


