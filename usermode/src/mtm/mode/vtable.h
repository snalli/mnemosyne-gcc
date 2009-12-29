/*! \file vtable.h
 *  \brief Enumeration of all the functions called through the "vtable", and its
 *  struct definition
 *
 *  This file contains the list of functions which are called through the vtable,
 *  wrapped in a macro so that it can easily be used to generate different 
 *  consistent sets of data.
 *
 */

#ifndef _VTABLE_H
#define _VTABLE_H

#include "mtm_i.h"
#include "mode/mode.h"
#include "mode/vtable_macros.h"

typedef struct mtm_vtable_s mtm_vtable_t;

struct mtm_vtable_s
{
    FOREACH_VTABLE_ENTRY (DEFINE_VTABLE_MEMBER, dummy) 
};



# define ACTION(mode) mtm_vtable_t * mtm_##mode;
typedef struct mtm_vtable_group_s
{
    FOREACH_MODE(ACTION)
} mtm_vtable_group_t;
# undef ACTION

//extern _ITM_PRIVATE mtm_vtable_group_t statsVtables;
extern _ITM_PRIVATE mtm_vtable_group_t perfVtables;
extern _ITM_PRIVATE mtm_vtable_group_t *defaultVtables;

#endif /* _VTABLE_H */
