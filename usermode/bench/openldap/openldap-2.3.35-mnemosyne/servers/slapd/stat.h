#ifndef _SLAPD_STATS_HAR182_H
#define _SLAPD_STATS_HAR182_H

#define TIME_SLOT_NUM       1000
#define TIME_SLOT_DURATION  1      /* seconds */

/* WARNING: This simple stats collector assumes it is being used by a 
 * single thread. 
 */

typedef struct {
	unsigned long long op_add_latency_sum[TIME_SLOT_NUM];
	unsigned long long op_add_count[TIME_SLOT_NUM];
} slapd_stats_t; 


extern slapd_stats_t slapd_stats;

void slapd_stats_init();
void slapd_stats_fini();
void slapd_stats_op_add(unsigned long long latency_us, struct timeval curtime);

#endif /* _SLAPD_STATS_HAR182_H */
