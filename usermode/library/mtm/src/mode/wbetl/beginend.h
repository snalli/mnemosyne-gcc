#ifndef _WBETL_BEGINEND_H
#define _WBETL_BEGINEND_H

extern void _ITM_CALL_CONVENTION mtm_wbetl_abortTransaction (mtm_tx_t *, _ITM_abortReason, const _ITM_srcLocation *);
extern void _ITM_CALL_CONVENTION mtm_wbetl_rollbackTransaction (mtm_tx_t *, const _ITM_srcLocation *);
extern void _ITM_CALL_CONVENTION mtm_wbetl_commitTransaction (mtm_tx_t *td, const _ITM_srcLocation *);
extern uint32_t _ITM_CALL_CONVENTION mtm_wbetl_tryCommitTransaction (mtm_tx_t *, const _ITM_srcLocation *);
extern void _ITM_CALL_CONVENTION mtm_wbetl_commitTransactionToId (mtm_tx_t *, const _ITM_transactionId, const _ITM_srcLocation *);
extern uint32_t _ITM_CALL_CONVENTION mtm_wbetl_beginTransaction (mtm_tx_t *, uint32, const _ITM_srcLocation *);

#endif
