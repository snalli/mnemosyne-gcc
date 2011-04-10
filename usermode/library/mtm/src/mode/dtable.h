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
#include "mode/dtablegen.h"

typedef struct mtm_dtable_s mtm_dtable_t;

struct mtm_dtable_s
{
    FOREACH_DTABLE_ENTRY (_DTABLE_MEMBER, dummy) 
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
