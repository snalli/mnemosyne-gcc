#include "common.h"
#include "tc_mix.h"

ubench_functions_t ubenchs[] = {
	{ "tc_mtm_mix_latency",          tc_mtm_mix_init, tc_mix_fini, tc_mix_thread_init, tc_mix_thread_fini, tc_mix_latency_thread_main, tc_mix_latency_print_stats },
	{ "tc_mtm_mix_throughput",       tc_mtm_mix_init, tc_mix_fini, tc_mix_thread_init, tc_mix_thread_fini, tc_mix_throughput_thread_main, tc_mix_throughput_print_stats },
	{ "tc_mtm_mix_latency_think",    tc_mtm_mix_init, tc_mix_fini, tc_mix_thread_init, tc_mix_thread_fini, tc_mix_latency_think_thread_main, tc_mix_latency_print_stats },
	{ "tc_mtm_mix_throughput_think", tc_mtm_mix_init, tc_mix_fini, tc_mix_thread_init, tc_mix_thread_fini, tc_mix_throughput_think_thread_main, tc_mix_throughput_print_stats },
	{ "tc_mtm_mix_latency_etl",          tc_mtm_mix_init_etl, tc_mix_fini, tc_mix_thread_init, tc_mix_thread_fini, tc_mix_latency_etl_thread_main, tc_mix_latency_print_stats },
	{ "tc_mtm_mix_throughput_etl",       tc_mtm_mix_init_etl, tc_mix_fini, tc_mix_thread_init, tc_mix_thread_fini, tc_mix_throughput_etl_thread_main, tc_mix_throughput_print_stats },
	{ NULL, NULL, NULL, NULL, NULL, NULL, NULL},
};
