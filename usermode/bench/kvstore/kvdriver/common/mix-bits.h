#ifndef _MIX_H_AAA121
#define _MIX_H_AAA121

#include "common.h"
#include "elemset.h"

typedef struct mix_stat_s {
	int      num_ins;
	int      num_del;
	int      num_get;
	uint64_t total_latency_cycles_ins;
	uint64_t total_latency_cycles_del;
	uint64_t total_latency_cycles_get;
	uint64_t work_transactions;
} mix_stat_t;


typedef struct mix_thread_state_s {
	void           *data;              /* backend specific data */
	unsigned int   tid;
	unsigned int   seed;
	char           buf[16384];
	elemset_t      elemset_ins;        /* the set of the keys inserted in the table */
	elemset_t      elemset_del;        /* the set of the keys deleted from the table */
	int            keys_range_len;
	int            keys_range_min;
	int            keys_range_max;
	mix_stat_t     stat;
} mix_thread_state_t;


typedef struct mix_global_state_s {
	void       *data;       /* backend specific data */
	mix_stat_t stat;
	int        (*op_ins)(void *, void *, int);
	int        (*op_del)(void *, void *, int);
	int        (*op_get)(void *, void *, int);
} mix_global_state_t;



static inline
int 
mix_init(mix_global_state_t **mix_global_state_ptr, 
         int (*op_ins)(void *, void *, int),
         int (*op_del)(void *, void *, int),
         int (*op_get)(void *, void *, int),
         void *data)
{
	mix_global_state_t     *mix_global_state;

	mix_global_state = (mix_global_state_t *) malloc(sizeof(mix_global_state_t));
	mix_global_state->data = data;
	mix_global_state->op_ins = op_ins; 
	mix_global_state->op_del = op_del; 
	mix_global_state->op_get = op_get; 
	ubench_desc.global_state = (void *) mix_global_state;
	*mix_global_state_ptr = mix_global_state;

	return 0;
}


static inline
int 
mix_thread_init(unsigned int tid,
                void *data,
                int create_elem(int, void **),
                mix_thread_state_t **thread_state_ptr
				)
{
	mix_thread_state_t     *thread_state;
	void                   *elem;
	int                    i;
	int                    j;
	int                    num_keys_per_thread;
	mix_stat_t             *stat;
	int                    keys_range_min;
	int                    keys_range_max;
	int                    keys_range_len;


	thread_state = (mix_thread_state_t*) malloc(sizeof(mix_thread_state_t));
	ubench_desc.thread_state[tid] = thread_state;
	thread_state->tid = tid;
	thread_state->data = data;
	*thread_state_ptr = thread_state;

	/* 
	 * We want to have a steady state experiment where deletes have the
	 * same rate as inserts, and the number of elements in the table is
	 * always num_keys.
	 *
	 * To achieve this, we have a range of keys [0, num_keys-1], and 
	 * only half of the keys are found in the hash table at anytime during
	 * the experiment's execution.
	 */

	keys_range_len = thread_state->keys_range_len = num_keys_per_thread = (num_keys) / num_threads;
	keys_range_min = thread_state->keys_range_min = tid*num_keys_per_thread;
	keys_range_max = thread_state->keys_range_max = (tid+1)*num_keys_per_thread-1;

	assert(elemset_init(&thread_state->elemset_ins, num_keys_per_thread, tid)==0);
	assert(elemset_init(&thread_state->elemset_del, num_keys_per_thread, tid)==0);

	for(j=keys_range_min; j<keys_range_max-(keys_range_len/2); j++) {
		create_elem(j, &elem);
		elemset_put(&thread_state->elemset_ins, elem);
	}

	for(j=keys_range_max-(keys_range_len/2); j<=keys_range_max; j++) {
		create_elem(j, &elem);
		elemset_put(&thread_state->elemset_del, elem);
	}

	stat = &(thread_state->stat);
	stat->work_transactions = 0;
	stat->num_ins = 0;
	stat->num_del = 0;
	stat->num_get = 0;
	stat->total_latency_cycles_ins = 0;
	stat->total_latency_cycles_del = 0;
	stat->total_latency_cycles_get = 0;

	return 0;
}


static inline
void 
mix_thread_main(unsigned int tid, int measure_latency, int think, int lock)
{
	mix_global_state_t        *global_state = (mix_global_state_t *) ubench_desc.global_state;
	mix_thread_state_t        *thread_state = ubench_desc.thread_state[tid];
 	unsigned int              iterations_per_chunk = ubench_desc.iterations_per_chunk;
	unsigned int*             seedp = &thread_state->seed;
	int                       i;
	int                       j;
	int                       op;
	int                       id;
	void                      *del_elem;
	void                      *ins_elem;
	void                      *get_elem;
	int                       found;
	int                       keys_range_min;
	int                       keys_range_max;
	int                       keys_range_len;
	int                       work_transactions = 0;
	uint64_t                  start;
	uint64_t                  stop; 
	struct timeval            work_start_time;
	struct timeval            work_stop_time;
	uint64_t                  work_time;   /* in usec */
	uint64_t                  think_time;  /* in usec */
	mix_stat_t                *stat = &thread_state->stat;
	int                       (*op_ins)(void *, void *, int);
	int                       (*op_del)(void *, void *, int);
	int                       (*op_get)(void *, void *, int);
	int                       block = 512;

	op_ins = global_state->op_ins;
	op_del = global_state->op_del;
	op_get = global_state->op_get;
	keys_range_min = thread_state->keys_range_min;
	keys_range_max = thread_state->keys_range_max;
	keys_range_len = thread_state->keys_range_len;

	for (i=0; i<iterations_per_chunk; i+=block) {
		if (think) {
			gettimeofday(&work_start_time, NULL);
		}
		for (j=0; j<block; j++) {
			op = random_operation(seedp, percent_write);
			switch(op) {
				case OP_HASH_WRITE:
					elemset_del_random(&thread_state->elemset_ins, &del_elem);
					elemset_del_random(&thread_state->elemset_del, &ins_elem);
					elemset_put(&thread_state->elemset_ins, ins_elem);
					elemset_put(&thread_state->elemset_del, del_elem);
					if (measure_latency) {
						start = gethrtime();
					}
					assert(op_del(thread_state, del_elem, lock) == 0);
					if (measure_latency) {
						asm_cpuid();
						stop = gethrtime();
						stat->total_latency_cycles_del += stop-start;
					}
					stat->num_del++;
					
					if (measure_latency) {
						start = gethrtime();
					}
					assert(op_ins(thread_state, ins_elem, lock) == 0);
					if (measure_latency) {
						asm_cpuid();
						stop = gethrtime();
						stat->total_latency_cycles_ins += stop-start;
					}
					stat->num_ins++;
					work_transactions += 2;
					break;
				case OP_HASH_READ:
					elemset_get_random(&thread_state->elemset_ins, &get_elem);
					if (measure_latency) {
						start = gethrtime();
					}
					assert(op_get(thread_state, get_elem, lock) == 0);
					if (measure_latency) {
						asm_cpuid();
						stop = gethrtime();
						stat->total_latency_cycles_get += stop-start;
					}
					stat->num_get++;
					work_transactions++;
					break;
			}
		}	
		if (think) {
			gettimeofday(&work_stop_time, NULL);
			work_time = 1000000 * (work_stop_time.tv_sec - work_start_time.tv_sec) +
			                       work_stop_time.tv_usec - work_start_time.tv_usec;
			think_time = ((100 - work_percent) * work_time) / work_percent;
			m_logtrunc_signal();
			usleep(think_time);
		}
	}
	stat->work_transactions += work_transactions;
}


static inline
void 
mix_print_stats(FILE *fout, int report_latency) 
{
	int                     i;
	uint64_t                total_work_transactions = 0;
	uint64_t                total_runtime = 0;
	uint64_t                avg_runtime;
	uint64_t                avg_latency_ins;
	uint64_t                avg_latency_get;
	uint64_t                avg_latency_del;
	double                  throughput;
	double                  throughput_ins;
	double                  throughput_del;
	double                  throughput_get;
	char                    prefix[] = "bench.stat";
	char                    separator[] = "rj,40";
	uint64_t                hashtable_footprint;
	unsigned int            hashtable_num_entries;

	mix_global_state_t      *mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	mix_stat_t              *thread_stat;
	mix_stat_t              total_stat;

	total_stat.num_ins = 0;
	total_stat.num_del = 0;
	total_stat.num_get = 0;
	total_stat.total_latency_cycles_ins = 0;
	total_stat.total_latency_cycles_del = 0;
	total_stat.total_latency_cycles_get = 0;
	for (i=0; i<num_threads; i++) {
		thread_stat = &(((mix_thread_state_t *) ubench_desc.thread_state[i])->stat);
		total_work_transactions += thread_stat->work_transactions;
		total_runtime += ubench_desc.thread_runtime[i];
		total_stat.num_ins += thread_stat->num_ins;
		total_stat.num_del += thread_stat->num_del;
		total_stat.num_get += thread_stat->num_get;
		total_stat.total_latency_cycles_ins += thread_stat->total_latency_cycles_ins;
		total_stat.total_latency_cycles_del += thread_stat->total_latency_cycles_del;
		total_stat.total_latency_cycles_get += thread_stat->total_latency_cycles_get;
	}
	avg_runtime = total_runtime/num_threads;
	throughput = ((double) total_work_transactions) / ((double) avg_runtime);
	throughput_ins = ((double) total_stat.num_ins) / ((double) avg_runtime);
	throughput_del = ((double) total_stat.num_del) / ((double) avg_runtime);
	throughput_get = ((double) total_stat.num_get) / ((double) avg_runtime);
	avg_latency_ins = CYCLE2NS(mydiv(total_stat.total_latency_cycles_ins, total_stat.num_ins));
	avg_latency_del = CYCLE2NS(mydiv(total_stat.total_latency_cycles_del, total_stat.num_del));
	avg_latency_get = CYCLE2NS(mydiv(total_stat.total_latency_cycles_get, total_stat.num_get));

	fprintf(fout, "STATISTICS\n");
	fprintf(fout, "%s\n", EQUALSIGNS(60));
	PRINT_NAMEVAL_U64(prefix, "runtime", avg_runtime/1000, "ms", separator);
	PRINT_NAMEVAL_U64(prefix, "total_work_transactions", total_work_transactions, "ops", separator);
	PRINT_NAMEVAL_DOUBLE(prefix, "throughput", throughput * 1000 * 1000, "ops/s", separator);
	PRINT_NAMEVAL_DOUBLE(prefix, "throughput_ins", throughput_ins * 1000 * 1000, "ops/s", separator);
	PRINT_NAMEVAL_DOUBLE(prefix, "throughput_del", throughput_del * 1000 * 1000, "ops/s", separator);
	PRINT_NAMEVAL_DOUBLE(prefix, "throughput_get", throughput_get * 1000 * 1000, "ops/s", separator);
	if (report_latency) {
		PRINT_NAMEVAL_U64(prefix, "avg_latency_ins", avg_latency_ins, "ns", separator);
		PRINT_NAMEVAL_U64(prefix, "avg_latency_del", avg_latency_del, "ns", separator);
		PRINT_NAMEVAL_U64(prefix, "avg_latency_get", avg_latency_get, "ns", separator);
	}
}
#endif /* _MIX_H_AAA121 */
