/**
 * \file dtable.h
 * \brief Definition of the dispatch table
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 *
 * This file contains the definition of the dispatch table, which is used to 
 * support dynamic switching between execution modes as described in 
 * section 4.1 of the paper:
 * 
 * Design and Implementation of Transactional Constructs for C/C++, OOPLSLA 2008
 *
 * The dispatch table contains only the ABI functions that take as first 
 * argument a transaction descriptor. The rest of the ABI is implemented 
 * in intelabi.c
 *
 */

#ifndef _DISPATCH_TABLE_H_119AKL
#define _DISPATCH_TABLE_H_119AKL

#include "mtm_i.h"
#include "mode/mode.h"


#undef FOREACH_ABI_FUNCTION
#define FOREACH_ABI_FUNCTION(ACTION, ...)                                                \
    ACTION (void,     abortTransaction,     (                               \
                                             _ITM_abortReason reason,                    \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (void,     rollbackTransaction,  (                               \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (void,     commitTransaction,    (                               \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (bool,     tryCommitTransaction, (                               \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (void,     commitTransactionToId,(                               \
                                             const _ITM_transactionId tid,               \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (uint32_t, beginTransaction,     (                               \
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



typedef struct mtm_dtable_s mtm_dtable_t;

struct mtm_dtable_s
{
    // FOREACH_DTABLE_ENTRY (_DTABLE_MEMBER, dummy) 
};


# define ACTION(mode) mtm_dtable_t * mtm_##mode;
typedef struct mtm_dtable_group_s
{
    FOREACH_MODE(ACTION)
} mtm_dtable_group_t;
# undef ACTION

extern mtm_dtable_group_t normal_dtable_group;
extern mtm_dtable_group_t *default_dtable_group;

#endif /* _DISPATCH_TABLE_H_119AKL */
