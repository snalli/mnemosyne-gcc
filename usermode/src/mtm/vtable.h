/*! \file vtable.h
 *  \brief Enumeration of all the functions called through the "vtable", and its struct definition
 *
 *  This file contains the list of functions which are called through the vtable, wrapped in a 
 *  macro so that it can easily be used to generate different consistent sets of data.
 */

#if (!defined (_VTABLE_INCLUDED))
# define _VTABLE_INCLUDED

# include "mtm_i.h"
# include "vtable_macros.h"

/*! The vtable containing pointers to all of the functions which take a transaction descriptor 
 *  as their first argument.
 */
typedef struct txninterface_s txninterface_t;
struct txninterface_s
{
    FOREACH_VTABLE_ENTRY (DEFINE_VTABLE_MEMBER, dummy) 
};

# define FOREACH_MODE(ACTION)   \
    ACTION(mtm_wbetl)           

# define ACTION(mode) txninterface_t * mode;
typedef struct txninterfacegroup_s
{
    FOREACH_MODE(ACTION)
} txninterfacegroup_t;
# undef ACTION

//extern _ITM_PRIVATE txninterfacegroup_t statsVtables;
extern _ITM_PRIVATE txninterfacegroup_t perfVtables;
extern _ITM_PRIVATE txninterfacegroup_t *defaultVtables;

#endif
