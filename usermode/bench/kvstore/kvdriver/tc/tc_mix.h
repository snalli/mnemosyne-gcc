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

#ifndef _TC_MIX_H_JA812A
#define _TC_MIX_H_JA812A

#include <stdio.h>

int tc_base_mix_init(void *);
int tc_mtm_mix_init(void *);
int tc_mtm_mix_init_etl(void *);
int tc_mix_fini(void *);
int tc_mix_thread_init(unsigned int);
int tc_mix_thread_fini(unsigned int);
void tc_mix_latency_thread_main(unsigned int);
void tc_mix_throughput_thread_main(unsigned int);
void tc_mix_latency_think_thread_main(unsigned int);
void tc_mix_throughput_think_thread_main(unsigned int);
void tc_mix_latency_etl_thread_main(unsigned int);
void tc_mix_throughput_etl_thread_main(unsigned int);
void tc_mix_latency_print_stats(FILE *);
void tc_mix_throughput_print_stats(FILE *);
int  tc_base_op_ins(void *t, void *elem, int lock);
int  tc_base_op_del(void *t, void *elem, int lock);
int  tc_base_op_get(void *t, void *elem, int lock);
int  tc_mtm_op_ins(void *t, void *elem, int lock);
int  tc_mtm_op_del(void *t, void *elem, int lock);
int  tc_mtm_op_get(void *t, void *elem, int lock);

#endif
