/*! \file vtable_macros.h
 *  \brief Platform-independent macros for generating vtable-related structures and functions
 *
 *  This file is compiled both as part of the project, and in intermediate build steps 
 *  (cross platform), therefore:
 *  1) This file should not include any header except for "itm.h"
 *  2) This file should not have any declarations or functions - only macros.
 */

#if (!defined (_VTABLE_MACROS_INCLUDED))
# define _VTABLE_MACROS_INCLUDED

# include "../inc/itm.h"

/*! Invoke the ACTION on each function in the vtable.
 *
 * We invoke the ACTION with three arguments, the function result, the
 * function name, and the function arguments. We need all three to be
 * able to generate the correct types for the entries in the struct.
 * In places where they're not all needed the macro being used can
 * just ignore arguments which it finds uninteresting.
 */

#  undef FOREACH_OUTER_FUNCTION
#  define FOREACH_OUTER_FUNCTION(ACTION, ...)                                            \
    ACTION (void,     abortTransaction,     (mtm_tx_t *td,                       \
                                             _ITM_abortReason reason,                    \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (void,     rollbackTransaction,  (mtm_tx_t *td,                       \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (void,     commitTransaction,    (mtm_tx_t *td,                       \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (uint32_t, tryCommitTransaction, (mtm_tx_t *td,                       \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (void,     commitTransactionToId,(mtm_tx_t *td,                       \
                                             const _ITM_transactionId tid,               \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \
    ACTION (uint32_t, beginTransaction,     (mtm_tx_t *td,                       \
                                             uint32 __properties,                        \
                                             const _ITM_srcLocation *loc), __VA_ARGS__)  \


# define FOREACH_VTABLE_ENTRY(ACTION,ARG)       \
    FOREACH_OUTER_FUNCTION(ACTION,ARG)          \
    _ITM_FOREACH_TRANSFER (ACTION,ARG)


/* *INDENT-OFF* */

/* Indent seems to be confused by the ## concatenation operator, so we 
 * turn it off for these macros...
 */

# define DEFINE_VTABLE_MEMBER(result, function, args, ARG)  \
    result (_ITM_CALL_CONVENTION * function##p) args;


#define _LIST_VTABLE_WRITE_MEMBERS(ACTION, result_type, encoding, W_pfx, WaR_pfx, WaW_pfx)  \
    W_pfx##encoding, WaR_pfx##encoding, WaW_pfx##encoding,

#define _LIST_VTABLE_READ_MEMBERS(ACTION, result_type, encoding, R_pfx, RaR_pfx, RaW_pfx, RfW_pfx)  \
    R_pfx##encoding, RaR_pfx##encoding, RaW_pfx##encoding, RfW_pfx##encoding,

#define _GEN_READ_BARRIERS_LIST(...) _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER(_LIST_VTABLE_READ_MEMBERS, dummy, __VA_ARGS__)
#define _GEN_WRITE_BARRIERS_LIST(...) _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER(_LIST_VTABLE_WRITE_MEMBERS, dummy, __VA_ARGS__)



#define VTABLE_FUNCTION(td,fn) ((td)->vtable->fn##p)
/* *INDENT-ON* */
#endif /* _VTABLE_MACROS_INCLUDED */
