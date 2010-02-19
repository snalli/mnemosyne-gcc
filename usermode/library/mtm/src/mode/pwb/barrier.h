#ifndef _WBETL_BARRIER_H
#define _WBETL_BARRIER_H

#include "mode/common/barrier.h"
#include "mode/pwb/pwb_i.h"
#include "mode/common/rwset.h"


FOR_ALL_TYPES(DECLARE_READ_BARRIERS, pwb)
FOR_ALL_TYPES(DECLARE_WRITE_BARRIERS, pwb)

mtm_word_t mtm_pwb_load(mtm_tx_t *tx, volatile mtm_word_t *addr);


#endif
