#include <linux/slab.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/delay.h>
#include "pcm.h"

										  
typedef uint64_t cacheline_bitmask_t;
typedef struct cacheline_s cacheline_t;
typedef struct cacheline_crash_cdf_s cacheline_crash_cdf_t;


/* 
 * The CDF that there will be a partial crash after we write the N first bytes
 * of a cacheline given there is a crash. SUM(cdf[0..8]) must be 1000.
 * cdf[I] means: crash after writing (I+1)*sizeof(word) bytes
 *
 * Example: For a cache line of size 64 bytes and word size of 8 bytes: 
 * cdf[0] --> crash after writing 0 bytes
 * cdf[1] --> crash after writing 8 bytes
 * ...
 * cdf[7] --> crash after writing 56 bytes
 * cdf[8] --> crash after writing 64 bytes (no partial crash)
 */

struct cacheline_crash_cdf_s {
	int word[CACHELINE_SIZE/sizeof(pcm_word_t)+1];
};

struct cacheline_s {
	UINTPTR_T           block_addr;
	cacheline_bitmask_t bitmask;
	uint8_t             bytes[CACHELINE_SIZE];
	struct list_head    chain;
};

struct pcm_global_s {
	struct kmem_cache *storeset_cache;
	struct kmem_cache *cacheline_cache;
};

struct pcm_s {
	spinlock_t        lock;
	struct kmem_cache *storeset_cache;
	struct kmem_cache *cacheline_cache;
	uint32_t          count;
	struct list_head  pcm_storeset_list;
	volatile uint8_t  mode;
};


static struct pcm_global_s pcm_global;

/* Dynamic configuration */

/* CDF for a cacheline partial crash. */

#define NO_PARTIAL_CRASH {{0,0,0,0,0,0,0,0,1000000}}

cacheline_crash_cdf_t cacheline_crash_cdf = NO_PARTIAL_CRASH;


/* Likelihood a store will block wait. */

unsigned int pcm_likelihood_store_blockwaits = 1000;  

/* Likelihood a cacheline has been evicted. */

unsigned int pcm_likelihood_evicted_cacheline = 10000;  


void
cacheline_tbl_init(cacheline_tbl_t *tbl)
{
	int i;

	tbl->size = PAGE_MAX_SIZE/CACHELINE_SIZE;
	tbl->count = 0;
	for (i=0; i<tbl->size; i++) {
		INIT_LIST_HEAD(&(tbl->chain[i]));
	}
}


void
cacheline_tbl_print(cacheline_tbl_t *tbl)
{
	int         hi;
	cacheline_t *cacheline;
	int         header_printed = 0;

	for (hi = 0; hi < tbl->size; hi++) {
		list_for_each_entry(cacheline, &tbl->chain[hi], chain) {
			if (cacheline) {
				if (header_printed == 0) {
					printk(KERN_INFO "cacheline_tbl_print\n");
					header_printed = 1;
				}
				printk(KERN_INFO "\ttbl[%d].block_addr = %llX\n", hi, cacheline->block_addr);
			}
		}
	}
}


void
cacheline_tbl_delall(pcm_storeset_t *set, cacheline_tbl_t *tbl)
{
	int         hi;
	cacheline_t *cacheline;
	cacheline_t *cacheline_tmp;

	for (hi = 0; hi < tbl->size; hi++) {
		list_for_each_entry_safe(cacheline, cacheline_tmp, &tbl->chain[hi], chain) {
			list_del(&cacheline->chain);
			kmem_cache_free(set->cacheline_cache, cacheline);
		}
	}
	tbl->count = 0;
}


cacheline_t *
cacheline_get(pcm_storeset_t *set, cacheline_tbl_t *tbl, UINTPTR_T block_addr)
{
	int         hi;
	cacheline_t *cacheline;
	cacheline_t *cacheline_tmp;

	hi = (block_addr >> (CACHELINE_SIZE_LOG+1)) % tbl->size;

#ifdef DEBUG
	printk(KERN_INFO "cacheline_get: %x\n", block_addr);
#endif
	if (set->pcm->mode == MODE_CRASH || set->pcm->mode == MODE_POSTCRASH) {
		printk(KERN_EMERG "cacheline_get: block_addr=%llX tbl=%p tbl->size=%d hi=%d\n", block_addr, tbl, tbl->size, hi);
	}

	list_for_each_entry_safe(cacheline, cacheline_tmp, &tbl->chain[hi], chain) {
		//printk(KERN_INFO "cacheline_get: for_each cacheline->block_addr=%p\n", cacheline->block_addr);
		if (set->pcm->mode == MODE_CRASH || set->pcm->mode == MODE_POSTCRASH) {
			printk(KERN_EMERG "cacheline_get: for_each cacheline=%p\n", cacheline);
			printk(KERN_EMERG "cacheline_get: for_each cacheline->block_addr=%llX\n", cacheline->block_addr);
		}
		if (cacheline && cacheline->block_addr == block_addr) {
#ifdef DEBUG
			printk(KERN_INFO "cacheline_get: found %x\n", block_addr);
#endif
			return cacheline;
		}
	}

	return NULL;
}


cacheline_t *
cacheline_add(pcm_storeset_t *set, cacheline_tbl_t *tbl, UINTPTR_T block_addr)
{
	int hi;
	cacheline_t *cacheline;

	hi = (block_addr >> (CACHELINE_SIZE_LOG+1)) % tbl->size;

	list_for_each_entry(cacheline, &tbl->chain[hi], chain) {
		if (cacheline && cacheline->block_addr == block_addr) {
			return cacheline;
		}
	}

	if (!set->cacheline_cache) {
		return NULL;
	}
	if (!(cacheline = (cacheline_t *) kmem_cache_alloc(set->cacheline_cache, GFP_KERNEL))) {
		printk(KERN_INFO "cacheline_add: Could not allocate cacheline.\n");
		return NULL;
	}

	cacheline->block_addr = block_addr;
	cacheline->bitmask = 0x0;
	list_add(&cacheline->chain, &tbl->chain[hi]);
	tbl->count++;
#ifdef DEBUG
	printk(KERN_INFO "cacheline_add: added %x\n", block_addr);
#endif
	return cacheline;
}



void
cacheline_del(pcm_storeset_t *set, cacheline_tbl_t *tbl, UINTPTR_T block_addr)
{
	int              hi;
	cacheline_t      *cacheline;

	hi = (block_addr >> (CACHELINE_SIZE_LOG+1)) % tbl->size;

#ifdef DEBUG
	printk(KERN_INFO "cacheline_del: del %x\n", block_addr);
#endif
	list_for_each_entry(cacheline, &tbl->chain[hi], chain) {
		if (cacheline && cacheline->block_addr == block_addr) {
			list_del(&cacheline->chain);
			kmem_cache_free(set->cacheline_cache, cacheline);
			tbl->count--;
			return;
		}
	}
}


int 
pcm_global_init(void) 
{
	printk(KERN_INFO "pcm_global_init\n");

	pcm_global.storeset_cache = kmem_cache_create("pcm_storeset_cache", 
	                                              sizeof(pcm_storeset_t), 0,
	                                              SLAB_HWCACHE_ALIGN, NULL);
	if (!pcm_global.storeset_cache) {
		printk(KERN_INFO "pcm_global_init: Could not create pcm_storeset_cache.\n");
		return -ENOMEM;
	}

	pcm_global.cacheline_cache = kmem_cache_create("pcm_cacheline_cache", 
	                                               sizeof(cacheline_t), 0,
	                                               SLAB_HWCACHE_ALIGN, NULL);
	if (!pcm_global.cacheline_cache) {
		printk(KERN_INFO "pcm_global_init: Could not create pcm_cacheline_cache.\n");
		kmem_cache_destroy(pcm_global.storeset_cache);
		return -ENOMEM;
	}

	return 0;
}


void
pcm_global_fini(void)
{
	printk(KERN_INFO "pcm_global_fini\n");

	if (pcm_global.storeset_cache) {
		kmem_cache_destroy(pcm_global.storeset_cache);
	}
	if (pcm_global.cacheline_cache) {
		kmem_cache_destroy(pcm_global.cacheline_cache);
	}
}


int
pcm_init(pcm_t **pcmp)
{
	pcm_t *pcm;
	printk(KERN_INFO "pcm_init:\n");

	if (!pcm_global.storeset_cache) {
		return -ENOMEM;
	}

	if (!pcm_global.cacheline_cache) {
		return -ENOMEM;
	}

	if (!(pcm = (pcm_t *) kmalloc(sizeof(pcm_t), GFP_KERNEL))) {
		return -ENOMEM;
	}

	pcm->storeset_cache = pcm_global.storeset_cache;
	pcm->cacheline_cache = pcm_global.cacheline_cache;
	pcm->mode = MODE_NORMAL;
	INIT_LIST_HEAD(&pcm->pcm_storeset_list);
	spin_lock_init(&pcm->lock);
	printk(KERN_INFO "pcm_init: DONE: pcm=%p\n", pcm);

	*pcmp = pcm;
	return 0;                 
}


void
pcm_fini(pcm_t *pcm)
{
	printk("pcm_fini: %p\n", pcm);

	if (!pcm) {
		return;
	}

	kfree(pcm);
}


int
pcm_storeset_create(pcm_t *pcm, pcm_storeset_t **setp)
{
	pcm_storeset_t *set;

#ifdef PCM_EMULATE_CRASH
	if (pcm->mode == MODE_CRASH || pcm->mode == MODE_POSTCRASH) {
		*setp = NULL;
		return -1;
	}
#endif

	if (!pcm->storeset_cache) {
		*setp = NULL;
		return -ENOMEM;
	}
	if (!(set = (pcm_storeset_t *) kmem_cache_alloc(pcm->storeset_cache, GFP_KERNEL))) {
		printk(KERN_INFO "pcm_storeset_create: Could not allocate storeset\n");
		*setp = NULL;
		return -ENOMEM;
	}
	*setp = set;
	set->pcm = pcm;
	memset(set->wcbuf_hashtbl, 0, WCBUF_HASHTBL_SIZE);
	set->wcbuf_hashtbl_count = 0;
	set->seqstream_len = 0;
	/* Initialize reentrant random generator */
	set->rand_seed = 0;
	rand_int(&set->rand_seed);
#ifdef PCM_EMULATE_CRASH
	set->in_crash_emulation_code = 0;
	set->cacheline_cache = pcm->cacheline_cache;
	cacheline_tbl_init(&set->oldval_tbl);
	while (!spin_trylock(&pcm->lock)) {
		if (pcm->mode == MODE_CRASH || pcm->mode == MODE_POSTCRASH) {
			kmem_cache_free(pcm->storeset_cache, set);
			*setp = NULL;
			return -1;
		}
	}
	asm_mfence();
	if (pcm->mode == MODE_CRASH || pcm->mode == MODE_POSTCRASH) {
		spin_unlock(&pcm->lock);
		kmem_cache_free(pcm->storeset_cache, set);
		*setp = NULL;
		return -1;
	}
	list_add(&set->pcm_storeset_list, &pcm->pcm_storeset_list);
	pcm->count++;
	spin_unlock(&pcm->lock);
#endif
	return 0;
}


static 
void
pcm_storeset_destroy_internal(pcm_storeset_t *set, int have_lock, int ignore_crash_checks)
{
	pcm_t *pcm;

	if (!set) {
		return;
	}
	pcm = set->pcm;

#ifdef PCM_EMULATE_CRASH
	if (!ignore_crash_checks) {
		asm_mfence();
		if (pcm->mode == MODE_CRASH || pcm->mode == MODE_POSTCRASH) {
			return;
		}
	}

	if (!have_lock) {
		spin_lock(&pcm->lock);
	}
	if (!ignore_crash_checks) {
		if (pcm->mode == MODE_CRASH || pcm->mode == MODE_POSTCRASH) {
			if (!have_lock) {
				spin_unlock(&pcm->lock);
			}
			return;
		}
	}
	list_del(&set->pcm_storeset_list);
	pcm->count--;
	if (!have_lock) {
		spin_unlock(&pcm->lock);
	}

	/* Clean up any allocated cache lines that have not been flushed.*/
	cacheline_tbl_delall(set, &set->oldval_tbl);
#endif

	if (pcm->storeset_cache) {
		kmem_cache_free(pcm->storeset_cache, set);
	}

}


void
pcm_storeset_destroy(pcm_storeset_t *set)
{
	pcm_storeset_destroy_internal(set, 0, 0);
}


static inline
void
crash_save_oldvalue(pcm_storeset_t *set, volatile pcm_word_t *addr)
{
	UINTPTR_T           byte_addr;
	UINTPTR_T           block_byte_addr;
	UINTPTR_T           index_byte_addr;
	int                 i;
	cacheline_t         *cacheline;
	cacheline_bitmask_t bitmask;
	uint8_t             byte_oldvalue;

	for (i=0; i<sizeof(pcm_word_t); i++) {
		byte_addr = (UINTPTR_T) addr + i;
		block_byte_addr = (UINTPTR_T) BLOCK_ADDR(byte_addr);
		index_byte_addr = (UINTPTR_T) INDEX_ADDR(byte_addr);
		if (!(cacheline = cacheline_add(set, &set->oldval_tbl, block_byte_addr))) {
			return;
		}
		bitmask = 1ULL << index_byte_addr;
		if (!(cacheline->bitmask & bitmask)) {
			byte_oldvalue = *((uint8_t *) byte_addr);
			cacheline->bytes[index_byte_addr] = byte_oldvalue;
			cacheline->bitmask |= bitmask;
		}
	}

}


static inline
void
crash_restore_unflushed_values(pcm_storeset_t *set)
{
	UINTPTR_T           byte_addr;
	UINTPTR_T           block_byte_addr;
	int                 i;
	int                 j;
	cacheline_t         *cacheline;
	cacheline_bitmask_t bitmask;

	for (i = 0; i < set->oldval_tbl.size; i++) {
		list_for_each_entry(cacheline, &(set->oldval_tbl.chain[i]), chain) {
			block_byte_addr = cacheline->block_addr;
			for (j=0; j<CACHELINE_SIZE; j++) {
				byte_addr = block_byte_addr + j;
				bitmask = 1ULL << j;
				if (cacheline->bitmask & bitmask) {
					*((uint8_t*)byte_addr) = cacheline->bytes[j];
				}
			}
		}
	}
}


/*
 * Flush a cacheline by removing the line from the list of outstanding stores.
 */
void
crash_flush_cacheline(pcm_storeset_t *set, volatile pcm_word_t *addr, int allow_partial_crash)
{
	UINTPTR_T           byte_addr;
	UINTPTR_T           block_byte_addr;
	int                 random_number;
	int                 i;
	int                 sum;
	int                 successfully_flushed_words_num=0;
	cacheline_t         *cacheline;
	cacheline_bitmask_t bitmask;

	byte_addr = (UINTPTR_T) addr;
	block_byte_addr = (UINTPTR_T) BLOCK_ADDR(addr);

//#if DEBUG
#if 0
	printk(KERN_INFO "crash_flush_cacheline: set=%p, byte_addr=%x\n", set, byte_addr);
#endif
flush_cacheline:
	cacheline = cacheline_get(set, &set->oldval_tbl, block_byte_addr);
	if (!cacheline) {
		/* Cache line already flushed. */
		return;
	}
	
	/* If there is an outstanding failure then the currently flushed line
	 * can be treaded as not flushed, partially flushed, or completely
	 * flushed based on some distribution.
	 * Remove from the outstanding stores just the part of the cache line
	 * that has been successfully flushed.
	 */
	if (set->pcm->mode && allow_partial_crash) {
		random_number = rand_int(&set->rand_seed) % TOTAL_OUTCOMES_NUM;
		for (i=0, sum=0; i<CACHELINE_SIZE/sizeof(pcm_word_t)+1; i++) {
			sum += cacheline_crash_cdf.word[i];
			if (random_number <= sum) {
				successfully_flushed_words_num = i;
				break;
			}
		}
	} else {
		successfully_flushed_words_num = CACHELINE_SIZE/sizeof(pcm_word_t);
	}	
	if (successfully_flushed_words_num == CACHELINE_SIZE/sizeof(pcm_word_t)) {
		bitmask = 0x0;
	} else {
		bitmask = (((uint64_t) -1)
		           << (successfully_flushed_words_num * sizeof(pcm_word_t))) & ((uint64_t) -1);
	}	
	cacheline->bitmask = cacheline->bitmask & bitmask;
return;
	if (cacheline->bitmask == 0x0) {
		cacheline_del(set, &set->oldval_tbl, block_byte_addr);
	}

	/* The word could fall in two cachelines. */
	byte_addr += sizeof(pcm_word_t)-1;
	block_byte_addr = (UINTPTR_T) BLOCK_ADDR(byte_addr);
	goto flush_cacheline;
}


/*
 * Either flush all cachelines, or randomly flush some based on a given
 * probability distribution.
 */
static inline
void
crash_flush_cachelines(pcm_storeset_t *set, int all, int likelihood_flush_cacheline)
{
	int         i;
	int         count = 0;
	int         random_number;
	int         do_flush;
	int         more_work_left;
	cacheline_t *cacheline;
	cacheline_t *ca[128];

more_work:
	more_work_left = 0;
	for (i = 0; i < set->oldval_tbl.size; i++) {
		list_for_each_entry(cacheline, &(set->oldval_tbl.chain[i]), chain) {
			if (!all) {
				random_number = rand_int(&set->rand_seed) % TOTAL_OUTCOMES_NUM;
				if (random_number < likelihood_flush_cacheline) {
					do_flush = 1;
				} else {
					do_flush = 0;
				}
			}
			if (all || (!all && do_flush)) {
				if (count >= 128) {
					more_work_left = 1;
					break;
				}
				ca[count++] = cacheline;
			}	
		}
	}

	for(i = 0; i < count; i++) {
		crash_flush_cacheline(set, (pcm_word_t *) ca[i]->block_addr, 0);
	}
	if (more_work_left) {
		goto more_work;
	}	
}


int
pcm_check_crash(pcm_t *pcm) 
{
#ifdef PCM_EMULATE_CRASH		
	asm_mfence();
	if (pcm->mode == MODE_CRASH || pcm->mode == MODE_POSTCRASH) {
		return 1;
	} else {
		return 0;
	}
#endif
	return 0;
}


void 
pcm_trigger_crash(pcm_t *pcm)
{
	pcm_storeset_t  *set;
	pcm_storeset_t  *set_tmp;

	printk(KERN_INFO "pcm_trigger_crash: pcm = %p, pcm->mode=%d\n", pcm, pcm->mode);
	printk(KERN_EMERG "pcm_trigger_crash: pcm = %p, pcm->mode=%d\n", pcm, pcm->mode);
	spin_lock(&pcm->lock);
	if (pcm->mode == MODE_CRASH || pcm->mode == MODE_POSTCRASH) {
		/* Someone else has already triggered a crash */
		spin_unlock(&pcm->lock);
		return;
	}
	pcm->mode = MODE_CRASH;

	/* Make sure there is no concurrent action on a store set. */
	list_for_each_entry(set, &pcm->pcm_storeset_list, pcm_storeset_list) {
		while (set->in_crash_emulation_code);
		
	}

	printk(KERN_EMERG "pcm_trigger_crash: pcm=%p mode=%d\n", pcm, pcm->mode);

	/* We are at a safe point; Proceed with crash. 
	 * To emulate evictions that might had happened, first randomly flush some 
	 * cache lines based on a probability distribution, and then restore
	 * old values of any outstanding stores.
	 */
	list_for_each_entry_safe(set, set_tmp, &pcm->pcm_storeset_list, pcm_storeset_list) {
		printk(KERN_EMERG "pcm_trigger_crash: set=%p set->pcm=%p mode=%d\n", set, set->pcm, pcm->mode);
		crash_flush_cachelines(set, 0, pcm_likelihood_evicted_cacheline);
		//crash_restore_unflushed_values(set);
	}

	pcm->mode = MODE_POSTCRASH;
	spin_unlock(&pcm->lock);
	return;

	/* now destroy all storesets */
	list_for_each_entry_safe(set, set_tmp, &pcm->pcm_storeset_list, pcm_storeset_list) {
		pcm_storeset_destroy_internal(set, 1, 1);
	}


	pcm->mode = MODE_POSTCRASH;
	spin_unlock(&pcm->lock);
}

void 
pcm_trigger_crash_reset(pcm_t *pcm) 
{
	spin_lock(&pcm->lock);
	pcm->mode = MODE_NORMAL;
	spin_unlock(&pcm->lock);
}

/*
 * WRITE BACK CACHE MODE
 */


void
pcm_wb_store_emulate_crash(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val)
{
	if (set->pcm->mode == MODE_CRASH || set->pcm->mode == MODE_POSTCRASH) {
		return;
	}
	set->in_crash_emulation_code = 1;
	crash_save_oldvalue(set, addr);
	set->in_crash_emulation_code = 0;
}


/*
 * Flush the cacheline containing address addr.
 */
void
pcm_wb_flush_emulate_crash(pcm_storeset_t *set, volatile pcm_word_t *addr)
{
	if (set->pcm->mode == MODE_CRASH || set->pcm->mode == MODE_POSTCRASH) {
		return;
	}
	set->in_crash_emulation_code = 1;
	crash_flush_cacheline(set, addr, 1);
	set->in_crash_emulation_code = 0;
}


/*
 * NON-TEMPORAL STREAM MODE
 *
 */

/* 
 * We emulate the write-combining buffers using a hash table that never contains 
 * more than WRITE_COMBINING_BUFFERS_NUM keys.
 */

static inline
void
stream_flush_buffers(pcm_storeset_t *set)
{
	crash_flush_cachelines(set, 1, 0);
}


void
pcm_nt_store_emulate_crash(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val)
{
	unsigned int active_buffers_count;
	UINTPTR_T    byte_addr;
	UINTPTR_T    block_byte_addr1;
	UINTPTR_T    block_byte_addr2;
	cacheline_t  *cacheline;
	int          buffers_needed = 0;

	pcm_check_crash(set->pcm);
	set->in_crash_emulation_code = 1;
	byte_addr = (UINTPTR_T) addr;

	/* Check if a buffer already holds the cacheline containing addr.
	 * If not then find out how many empty buffers we need. 
	 */
	block_byte_addr1 = (UINTPTR_T) BLOCK_ADDR(byte_addr);
	block_byte_addr2 = (UINTPTR_T) BLOCK_ADDR(byte_addr+sizeof(pcm_word_t)-1);
	if (!(cacheline = cacheline_get(set, &set->oldval_tbl, block_byte_addr1))) { 
		buffers_needed++;
	}
	if (block_byte_addr1 != block_byte_addr2 /* Word spans more than one cacheline. */
		&& (!(cacheline = cacheline_get(set, &set->oldval_tbl, block_byte_addr2)))) 
	{
		buffers_needed++;
	}

	/* Check if there is space. If not then flush buffers */
	active_buffers_count = set->oldval_tbl.count;
	if (active_buffers_count + buffers_needed > WRITE_COMBINING_BUFFERS_NUM) {
		stream_flush_buffers(set);
	}
	crash_save_oldvalue(set, addr);
	set->in_crash_emulation_code = 0;
}


void
pcm_nt_flush_emulate_crash(pcm_storeset_t *set)
{
	pcm_check_crash(set->pcm);
	set->in_crash_emulation_code = 1;
	stream_flush_buffers(set);
	set->in_crash_emulation_code = 0;
}

static
void *
pcm_memcpy_internal(pcm_storeset_t *set, void *dst, const void *src, size_t n, int use_stream_writes)
{
	volatile uint8_t *saddr=((volatile uint8_t *) src);
	volatile uint8_t *daddr=dst;
	uint8_t          offset;
 	pcm_word_t       val;
	size_t           size = n;
	
	if (size == 0) {
		return dst;
	}

	PCM_SEQSTREAM_INIT(set);

	/* First write the new data. */
	while(size > sizeof(pcm_word_t)) {
		val = *((pcm_word_t *) saddr);
		if (use_stream_writes == 0) {
			PCM_WB_STORE(set, (volatile pcm_word_t *) daddr, val);
		} else {
			PCM_SEQSTREAM_STORE(set, (volatile pcm_word_t *) daddr, val);
		}
		saddr+=sizeof(pcm_word_t);
		daddr+=sizeof(pcm_word_t);
		size-=sizeof(pcm_word_t);
	}
	if (size > 0) {
		/* Ugly hack: pcm_??_store requires a word size value. We move
		 * back a few bytes to form a word.
		 */ 
		offset = sizeof(pcm_word_t) - size;
		saddr-=offset;
		daddr-=offset;
		val = *((pcm_word_t *) saddr);
		if (use_stream_writes == 0) {
			PCM_WB_STORE(set, (volatile pcm_word_t *) daddr, val);
		} else {
			PCM_SEQSTREAM_STORE(set, (volatile pcm_word_t *) daddr, val);
		}

	}

	size = n;

	/* Now make sure data is flushed out */
	if (use_stream_writes == 0) {
		daddr=dst;
		while(size > CACHELINE_SIZE) {
			PCM_WB_FLUSH(set, (pcm_word_t *) daddr);
			daddr+=CACHELINE_SIZE;
			size-=CACHELINE_SIZE;
		}
		if (size > 0)  {
			PCM_WB_FLUSH(set, (volatile pcm_word_t *) daddr);
		}
	} else {
		PCM_SEQSTREAM_FLUSH(set);
	}
	return dst;
}


void *
pcm_memcpy(pcm_storeset_t *set, void *dst, const void *src, size_t n) 
{
	return pcm_memcpy_internal(set, dst, src, n, 1);
}
