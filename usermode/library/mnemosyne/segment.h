#ifndef _MNEMOSYNE_SPRING_H
#define _MNEMOSYNE_SPRING_H

#include <pthread.h>
#include <stdio.h>
#include <common/list.h>
#include <common/result.h>

struct mnemosyne_segment_list_s {
	pthread_mutex_t  mutex;
	struct list_head list;
};
typedef struct mnemosyne_segment_list_s mnemosyne_segment_list_t;

mnemosyne_segment_list_t segment_list;

/*!
 * The active list of segments mapped into this process.
 */
extern mnemosyne_segment_list_t segment_list;

/*!
 * The path under the segments directory identifying the primary segments table.
 * This table lists the layout of persistent segments in the process's address
 * space.
 */
extern char segment_table_path[256];

/*!
 * The directory relative to the user's home directory where persistent
 * segment mappings are kept between instances of the program.
 *
 * \todo Will this work for daemons running as users who don't have a home?
 */
#define SEGMENTS_DIR ".segments"

/*!
 * Reserves a block of the process's virtual address space for persistent data.
 * This data will be backed by some stable storage (ideally, storage-class
 * memory like PCM).
 */
void *mnemosyne_segment_create(void *start, size_t length, int prot, int flags);

#endif /* _MNEMOSYNE_SPRING_H */
