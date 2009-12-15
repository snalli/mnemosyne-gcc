#ifndef _WBETL_BEGINEND_H
#define _WBETL_BEGINEND_H

extern void _ITM_CALL_CONVENTION mtm_wbetl_abortTransaction (mtm_thread_t *, _ITM_abortReason, const _ITM_srcLocation *);
extern void _ITM_CALL_CONVENTION mtm_wbetl_rollbackTransaction (mtm_thread_t *, const _ITM_srcLocation *);
extern void _ITM_CALL_CONVENTION mtm_wbetl_commitTransaction (mtm_thread_t *td, const _ITM_srcLocation *);
extern uint32_t _ITM_CALL_CONVENTION mtm_wbetl_tryCommitTransaction (mtm_thread_t *, const _ITM_srcLocation *);
extern void _ITM_CALL_CONVENTION mtm_wbetl_commitTransactionToId (mtm_thread_t *, const _ITM_transactionId, const _ITM_srcLocation *);
extern uint32_t _ITM_CALL_CONVENTION mtm_wbetl_beginTransaction (mtm_thread_t *, uint32, const _ITM_srcLocation *);

#endif
