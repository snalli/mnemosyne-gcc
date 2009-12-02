#include <stdio.h>
#include <sys/time.h>
#include "stat.h"

slapd_stats_t slapd_stats;

static struct timeval  stats_init_time;

void slapd_stats_init()
{
	int i;

	for (i=0; i<TIME_SLOT_NUM; i++) {
		slapd_stats.op_add_latency_sum[i] = 0;
		slapd_stats.op_add_count[i] = 0;
	}
	gettimeofday(&stats_init_time, NULL);
}

static int time2slot(struct timeval mytime) 
{
	int time_diff;

	time_diff = mytime.tv_sec - stats_init_time.tv_sec;
	return time_diff;
}

void slapd_stats_op_add(unsigned long long latency_us, struct timeval curtime)
{
	int curslot;
	
	curslot = time2slot(curtime);
	if (curslot > TIME_SLOT_NUM) {
		return;
	}
	
	slapd_stats.op_add_latency_sum[curslot] += latency_us;
	slapd_stats.op_add_count[curslot] += 1;
}

static void stats_print(FILE *fout) {
	unsigned long long tmp;
	int                i;

	for (i=0; i<TIME_SLOT_NUM; i++) {
		if (slapd_stats.op_add_count[i]) {
			tmp = slapd_stats.op_add_latency_sum[i]/slapd_stats.op_add_count[i];
			fprintf(fout, "slot %d: op_add_avg_latency = %llu (us)\n", i, tmp);

		}
	}
}


void slapd_stats_fini() {
	FILE *fout;
	fout = fopen("slapd.stats", "w");
	stats_print(fout);
	fclose(fout);
}
