#ifndef _BDB_MIX_H_JA812A
#define _BDB_MIX_H_JA812A

int bdb_mix_init(void *);
int bdb_mix_fini(void *);
int bdb_mix_thread_init(unsigned int);
int bdb_mix_thread_fini(unsigned int);
void bdb_mix_latency_thread_main(unsigned int);
void bdb_mix_throughput_thread_main(unsigned int);
void bdb_mix_latency_print_stats(FILE *);
void bdb_mix_throughput_print_stats(FILE *);
int bdb_op_ins(void *t, void *elem, int lock);
int bdb_op_del(void *t, void *elem, int lock);
int bdb_op_get(void *t, void *elem, int lock);
#endif
