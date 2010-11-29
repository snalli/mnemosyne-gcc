#include <db.h>
#include "common.h"
#include "bdb_mix.h"
#include "mtm_mix.h"

ubench_functions_t ubenchs[] = {
	{ "mtm_mix_latency",          mtm_mix_init, mtm_mix_fini, mtm_mix_thread_init, mtm_mix_thread_fini, mtm_mix_latency_thread_main, mtm_mix_latency_print_stats },
	{ "mtm_mix_throughput",       mtm_mix_init, mtm_mix_fini, mtm_mix_thread_init, mtm_mix_thread_fini, mtm_mix_throughput_thread_main, mtm_mix_throughput_print_stats },
	{ "mtm_mix_latency_think",    mtm_mix_init, mtm_mix_fini, mtm_mix_thread_init, mtm_mix_thread_fini, mtm_mix_latency_think_thread_main, mtm_mix_latency_print_stats },
	{ "mtm_mix_throughput_think", mtm_mix_init, mtm_mix_fini, mtm_mix_thread_init, mtm_mix_thread_fini, mtm_mix_throughput_think_thread_main, mtm_mix_throughput_print_stats },
	{ "mtm_mix_latency_nolock",   mtm_mix_init, mtm_mix_fini, mtm_mix_thread_init, mtm_mix_thread_fini, mtm_mix_latency_thread_main_nolock, mtm_mix_latency_print_stats },
	{ "mtm_mix_throughput_nolock",mtm_mix_init, mtm_mix_fini, mtm_mix_thread_init, mtm_mix_thread_fini, mtm_mix_throughput_thread_main_nolock, mtm_mix_throughput_print_stats },
	{ "bdb_mix_latency",          bdb_mix_init, bdb_mix_fini, bdb_mix_thread_init, bdb_mix_thread_fini, bdb_mix_latency_thread_main, bdb_mix_latency_print_stats },
	{ "bdb_mix_throughput",       bdb_mix_init, bdb_mix_fini, bdb_mix_thread_init, bdb_mix_thread_fini, bdb_mix_throughput_thread_main, bdb_mix_throughput_print_stats },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL},
};
