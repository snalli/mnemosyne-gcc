/**
 * \file wt.h
 *
 * \brief WRITE_THROUGH: write-through (encounter-time locking)
 *
 * WRITE_THROUGH: write-through (encounter-time locking) directly updates
 * memory and keeps an undo log for possible rollback.
 *
 */

#ifndef _WT_H
#define _WT_H

m_result_t mtm_wt_create(mtm_tx_t *tx, mtm_mode_data_t **descp);
m_result_t mtm_wt_destroy(mtm_mode_data_t *desc);

#endif
