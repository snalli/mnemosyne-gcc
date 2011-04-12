/**
 * \file dtablegen.h
 * \brief Preprocessor macros for generating dispatch table-related structures 
 * and functions
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 *
 */

#ifndef _DTABLEGEN_H_891AKL
#define _DTABLEGEN_H_891AKL

//#include <itm.h>
//#include "itm.h"
#include "mtm_i.h"


#undef FOREACH_ABI_FUNCTION
#define FOREACH_ABI_FUNCTION(ACTION, ...)                                                \
    ACTION (void,     abortTransaction,     (mtm_tx_t *td,                               \
                                             _ITM_abortReason reason,                    \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (void,     rollbackTransaction,  (mtm_tx_t *td,                               \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (void,     commitTransaction,    (mtm_tx_t *td,                               \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (bool,     tryCommitTransaction, (mtm_tx_t *td,                               \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (void,     commitTransactionToId,(mtm_tx_t *td,                               \
                                             const _ITM_transactionId tid,               \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (uint32_t, beginTransaction,     (mtm_tx_t *td,                               \
                                             uint32 __properties,                        \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \

#define FOREACH_DTABLE_ENTRY(ACTION,ARG)                           \
    FOREACH_ABI_FUNCTION(ACTION,ARG)                               \
    _ITM_FOREACH_TRANSFER (ACTION,ARG)


#define _DTABLE_MEMBER(result, function, args, ARG)                \
    result (_ITM_CALL_CONVENTION * function##p) args;


#define _GEN_READ_BARRIER(ACTION, result_type, encoding, ...)      \
    ACTION (result_type, R##encoding, dummy_args, __VA_ARGS__)     \
    ACTION (result_type, RaR##encoding, dummy_args, __VA_ARGS__)   \
    ACTION (result_type, RaW##encoding, dummy_args, __VA_ARGS__)   \
    ACTION (result_type, RfW##encoding, dummy_args, __VA_ARGS__) 

#define _GEN_WRITE_BARRIER(ACTION, result_type, encoding, ...)     \
    ACTION (result_type, W##encoding, dummy_args, __VA_ARGS__)     \
    ACTION (result_type, WaR##encoding, dummy_args, __VA_ARGS__)   \
    ACTION (result_type, WaW##encoding, dummy_args, __VA_ARGS__) 

#define FOREACH_READ_BARRIER(ACTION, ...)                          \
   _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER (_GEN_READ_BARRIER, ACTION, __VA_ARGS__)

#define FOREACH_WRITE_BARRIER(ACTION, ...)                         \
   _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER (_GEN_WRITE_BARRIER, ACTION, __VA_ARGS__)


#endif /* _DTABLEGEN_H_891AKL */
