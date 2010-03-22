/**
 * \file index.h
 *
 * \brief The index that keeps mappings of SCM page frames to file offsets.
 *
 */

#ifndef _SCM_INDEX_H
#define _SCM_INDEX_H


int scmmap_index_create(int mappings_max_n);
int scmmap_index_destroy(void);
int scmmap_index_alloc_mapping(struct scmpage_mapping **mapping);
int scmmap_index_get_mapping(unsigned long index, struct scmpage_mapping **mapping);


/* Allow breaking the abstraction for better performance */
struct scmpage_mapping_index {
	struct scmpage_mapping **array;
	int                    mappings_max_n;
	int                    mappings_count;
};

extern struct scmpage_mapping_index  *scmpage_mapping_index;

#endif
