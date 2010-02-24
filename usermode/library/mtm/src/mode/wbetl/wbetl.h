/**
 * \file wbetl.h
 *
 * \brief WRITE_BACK_ETL: write-back with encounter-time locking
 *
 * WRITE_BACK_ETL: write-back with encounter-time locking acquires lock
 * when encountering write operations and buffers updates (they are
 * committed to main memory at commit time).
 *
 */
#ifndef _WBETL_H
#define _WBETL_H

#include "result.h"

m_result_t mtm_wbetl_create(mtm_tx_t *tx, mtm_mode_data_t **descp);
m_result_t mtm_wbetl_destroy(mtm_mode_data_t *desc);

#endif
