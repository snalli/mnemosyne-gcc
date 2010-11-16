#ifndef _MTM_MIX_H_JA812A
#define _MTM_MIX_H_JA812A

int mtm_mix_init(void *);
int mtm_mix_fini(void *);
int mtm_mix_thread_init(unsigned int);
int mtm_mix_thread_fini(unsigned int);
void mtm_mix_latency_thread_main(unsigned int);
void mtm_mix_throughput_thread_main(unsigned int);
void mtm_mix_latency_print_stats(FILE *);
void mtm_mix_throughput_print_stats(FILE *);
void mtm_mix_latency_think_thread_main(unsigned int);
void mtm_mix_throughput_think_thread_main(unsigned int);
#endif
