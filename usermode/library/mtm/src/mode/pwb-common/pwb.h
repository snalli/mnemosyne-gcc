/**
 * \file pwbnl.h
 *
 * \brief write-back with no locking
 *
 */

#ifndef _PWBNL_H
#define _PWBNL_H

m_result_t mtm_pwb_create(mtm_tx_t *tx, mtm_mode_data_t **descp);
m_result_t mtm_pwb_destroy(mtm_mode_data_t *desc);

#endif /* _PWBNL_H */
