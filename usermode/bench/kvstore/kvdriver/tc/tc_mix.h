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
