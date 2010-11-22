/**
 * \file pwb_i.h
 *
 * \brief Private header file for write-back with encounter-time locking.
 *
 */

#ifndef _PWBNL_INTERNAL_HJA891_H
#define _PWBNL_INTERNAL_HJA891_H

#undef  DESIGN
#define DESIGN WRITE_BACK_ETL


#include "mode/pwb-common/pwb_i.h"

#include "mode/common/mask.h"
#include "mode/common/barrier.h"
#include "mode/pwbetl/barrier.h"
#include "mode/pwbetl/beginend.h"
#include "mode/pwbetl/memcpy.h"
#include "mode/pwbetl/memset.h"

#include "mode/common/memcpy.h"
#include "mode/common/memset.h"
#include "mode/common/rwset.h"

#include "cm.h"


#endif /* _PWBNL_INTERNAL_HJA891_H */
