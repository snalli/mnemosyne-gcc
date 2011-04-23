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

#include <db.h>
#include "common.h"
#include "bdb_mix.h"
#include "mtm_mix.h"

ubench_functions_t ubenchs[] = {
	{ "mtm_mix_latency",          mtm_mix_init, mtm_mix_fini, mtm_mix_thread_init, mtm_mix_thread_fini, mtm_mix_latency_thread_main, mtm_mix_latency_print_stats },
	{ "mtm_mix_throughput",       mtm_mix_init, mtm_mix_fini, mtm_mix_thread_init, mtm_mix_thread_fini, mtm_mix_throughput_thread_main, mtm_mix_throughput_print_stats },
	{ "mtm_mix_latency_think",    mtm_mix_init, mtm_mix_fini, mtm_mix_thread_init, mtm_mix_thread_fini, mtm_mix_latency_think_thread_main, mtm_mix_latency_print_stats },
	{ "mtm_mix_throughput_think", mtm_mix_init, mtm_mix_fini, mtm_mix_thread_init, mtm_mix_thread_fini, mtm_mix_throughput_think_thread_main, mtm_mix_throughput_print_stats },
	{ "mtm_mix_latency_etl",   mtm_mix_init, mtm_mix_fini, mtm_mix_thread_init, mtm_mix_thread_fini, mtm_mix_latency_thread_main_etl, mtm_mix_latency_print_stats },
	{ "mtm_mix_throughput_etl",mtm_mix_init, mtm_mix_fini, mtm_mix_thread_init, mtm_mix_thread_fini, mtm_mix_throughput_thread_main_etl, mtm_mix_throughput_print_stats },
	{ "bdb_mix_latency",          bdb_mix_init, bdb_mix_fini, bdb_mix_thread_init, bdb_mix_thread_fini, bdb_mix_latency_thread_main, bdb_mix_latency_print_stats },
	{ "bdb_mix_throughput",       bdb_mix_init, bdb_mix_fini, bdb_mix_thread_init, bdb_mix_thread_fini, bdb_mix_throughput_thread_main, bdb_mix_throughput_print_stats },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL},
};
