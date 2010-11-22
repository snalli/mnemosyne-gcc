/**
 * \file pwb.h
 *
 * \brief WRITE_BACK_ETL: write-back with encounter-time locking
 *
 * WRITE_BACK_ETL: write-back with encounter-time locking acquires lock
 * when encountering write operations and buffers updates (they are
 * committed to main memory at commit time).
 *
 */

#ifndef _PWB_H
#define _PWB_H

m_result_t mtm_pwb_create(mtm_tx_t *tx, mtm_mode_data_t **descp);
m_result_t mtm_pwb_destroy(mtm_mode_data_t *desc);

#endif
