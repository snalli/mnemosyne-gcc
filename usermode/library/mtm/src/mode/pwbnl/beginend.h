#ifndef _PWBNL_BEGINEND_JUI111_H
#define _PWBNL_BEGINEND_JUI111_H

extern void     _ITM_CALL_CONVENTION mtm_pwbnl_abortTransaction (mtm_tx_t *, _ITM_abortReason, const _ITM_srcLocation *);
extern void     _ITM_CALL_CONVENTION mtm_pwbnl_rollbackTransaction (mtm_tx_t *, const _ITM_srcLocation *);
extern void     _ITM_CALL_CONVENTION mtm_pwbnl_commitTransaction (mtm_tx_t *td, const _ITM_srcLocation *);
extern bool     _ITM_CALL_CONVENTION mtm_pwbnl_tryCommitTransaction (mtm_tx_t *, const _ITM_srcLocation *);
extern void     _ITM_CALL_CONVENTION mtm_pwbnl_commitTransactionToId (mtm_tx_t *, const _ITM_transactionId, const _ITM_srcLocation *);
extern uint32_t _ITM_CALL_CONVENTION mtm_pwbnl_beginTransaction (mtm_tx_t *, uint32, const _ITM_srcLocation *);

#endif /* _PWBNL_BEGINEND_JUI111_H */
