/*
    Copyright (C) 2011 Computer Sciences Department, 
    University of Wisconsin -- Madison

    ----------------------------------------------------------------------

    This file is part of Mnemosyne: Lightweight Persistent Memory, 
    originally developed at the University of Wisconsin -- Madison.

    Mnemosyne was originally developed primarily by Haris Volos
    with contributions from Andres Jaan Tack.

    ----------------------------------------------------------------------

    Mnemosyne is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, version 2
    of the License.
 
    Mnemosyne is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA.

### END HEADER ###
*/

#ifndef _PWBETL_BEGINEND_JUI111_H
#define _PWBETL_BEGINEND_JUI111_H

extern void     _ITM_CALL_CONVENTION mtm_pwbetl_abortTransaction (mtm_tx_t *, _ITM_abortReason, const _ITM_srcLocation *);
extern void     _ITM_CALL_CONVENTION mtm_pwbetl_rollbackTransaction (mtm_tx_t *, const _ITM_srcLocation *);
extern void     _ITM_CALL_CONVENTION mtm_pwbetl_commitTransaction (mtm_tx_t *td, const _ITM_srcLocation *);
extern bool     _ITM_CALL_CONVENTION mtm_pwbetl_tryCommitTransaction (mtm_tx_t *, const _ITM_srcLocation *);
extern void     _ITM_CALL_CONVENTION mtm_pwbetl_commitTransactionToId (mtm_tx_t *, const _ITM_transactionId, const _ITM_srcLocation *);
extern uint32_t _ITM_CALL_CONVENTION mtm_pwbetl_beginTransaction (mtm_tx_t *, uint32, const _ITM_srcLocation *);

#endif /* _PWBETL_BEGINEND_JUI111_H */
