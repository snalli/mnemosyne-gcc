/*
    Copyright (C) 2011 Computer Sciences Department, 
    University of Wisconsin -- Madison

    ----------------------------------------------------------------------

    This file is part of Mnemosyne: Lightweight Persistent Memory, 
    originally developed at the University of Wisconsin -- Madison.

    Mnemosyne was originally developed primarily by Haris Volos
    with contributions from Andres Jaan Tack.

    ----------------------------------------------------------------------

    Mnemosyne is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, version 2
    of the License.
 
    Mnemosyne is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA.

### END HEADER ###
*/

/* System header files */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <assert.h>
#include <dirent.h> 
/* Mnemosyne common header files */
#define _M_DEBUG_BUILD
#include <debug.h>
#include <result.h>
#include <pm_instr.h>
/* Private local header files */
#include "mcore_i.h"
#include "files.h"
#include "segment.h"
#include "module.h"
#include "hal/pcm_i.h"
#include "pregionlayout.h"
#include "config.h"


/**
 * The directory where persistent segment backing stores are kept.
 */
#define SEGMENTS_DIR mcore_runtime_settings.segments_dir


#define M_DEBUG_SEGMENT 1

m_segtbl_t m_segtbl;


/* Check whether there is a hole where we can allocate memory from. */
#undef TRY_ALLOC_IN_HOLES


static inline void *segment_map(void *addr, size_t size, int prot, int flags, int segment_fd);
static m_result_t segidx_find_entry_using_index(m_segidx_t *segidx, uint32_t index, m_segidx_entry_t **entryp);


/**
 * \brief Verify backing stores 
 *
 * Cleanup any backing stores which do not have a valid entry in the 
 * segment table. 
 * 
 * For .persistent section backing stores, update the segment table index 
 * entry with the module id the .persistent section belongs to.
 *
 * Segment table must already have an index attached to it. 
 */
static
void
verify_backing_stores(m_segtbl_t *segtbl)
{
	int              n;
	DIR              *d;
    struct dirent    *dir;
	uint32_t         index;
	uint32_t         segment_id; 
	uint64_t         segment_module_id; /* This is valid for the .persistent backing stores */
	m_segidx_entry_t *ientry;
	char             complete_path[256];

	d = opendir(SEGMENTS_DIR);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			n = sscanf(dir->d_name, "%u.%lu\n", &segment_id, &segment_module_id);
			if (n == 2) {
				index = segment_id;
				M_DEBUG_PRINT(M_DEBUG_SEGMENT, "Verifying backing store: %u.%lu\n", segment_id, segment_module_id);
				/* Backing store has a valid entry in the segment table? */
				if (!(segtbl->entries[index].flags & SGTB_VALID_ENTRY)) {
					/* No valid entry; erase backing store */
					sprintf(complete_path, "%s/%s", SEGMENTS_DIR, dir->d_name);
					M_DEBUG_PRINT(M_DEBUG_SEGMENT, "Remove backing store: %s\n", complete_path);
					unlink(complete_path);
				}	
				/* If this is .persistent backing store then update the index */
				if (segment_module_id != (uint64_t) (-1LLU)) {
					if (segidx_find_entry_using_index(segtbl->idx, index, &ientry)
						!= M_R_SUCCESS) 
					{
						M_INTERNALERROR("Cannot find index entry for valid table entry.");
					}
					ientry->module_id = segment_module_id;
				}
			}	
		}
		closedir(d);
	}
}


static
int 
create_backing_store(char *file, unsigned long long size)
{
	int      fd;
	unsigned long long  roundup_size;
	char     buf[1]; 
	
	fd = open(file, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		return fd;
	}
	/* TRICK: We could create an empty file by seeking to the end of the file 
	 * and writing a single zero byte there. However this would cause a page 
	 * fault at the last page of the file and bring the page into the OS page 
	 * cache. This would prevent us from observing the virtual to 
	 * physical persistent frame mapping later after the file is mmaped to a persistent
	 * memory region. This is an artifact of our implementation of the persistent 
	 * mapping subsystem in the kernel. To avoid this problem we round up the file
	 * of the size to N * PAGE_SIZE, where N=NUM_PAGES(size), and write a single 
	 * byte at location N*PAGE_SIZE+1. 
	 *
	 * Another way would be to bypass the file cache and use DIRECT I/O.
	 */
	printf("file = %s, size = %llu, size_of_pages = %llu \n", file, size, SIZEOF_PAGES(size));
	roundup_size = SIZEOF_PAGES(size);
	assert(lseek64(fd, roundup_size, SEEK_SET) !=  (off_t) -1);
	write(fd, buf, 1);
	fsync(fd); /* make sure the file metadata is synced */
	/* FIXME: sync directory as well to reflect the new file entry. */

	return fd;
}

m_result_t
m_check_backing_store(char *path, ssize_t size)
{
	m_result_t  rv = M_R_FAILURE;
	struct stat stat_buf;
	ssize_t     roundup_size;

	if (stat(path, &stat_buf)==0) {
		roundup_size = SIZEOF_PAGES(size);
		if (stat_buf.st_size > roundup_size) {
			rv = M_R_SUCCESS;
		} else {
			rv = M_R_FAILURE; 
		}
	}
	return rv;
}

static 
m_result_t
segidx_insert_entry_ordered(m_segidx_t *segidx, m_segidx_entry_t *new_entry, int lock)
{
	m_segidx_entry_t *ientry;

	if (lock) {
		pthread_mutex_lock(&(segidx->mutex));
	}
	/* 
	 * Find the right location of the entry so that the list is ordered
	 * incrementally by start address. 
	 */
	list_for_each_entry(ientry, &(segidx->mapped_entries.list), list) {
		if (ientry->segtbl_entry->start > new_entry->segtbl_entry->start) {
			list_add_tail(&(new_entry->list), &(ientry->list));
			goto out;
		}
	}
	list_add_tail(&(new_entry->list), &(segidx->mapped_entries.list));

out:
	if (lock) {
		pthread_mutex_unlock(&(segidx->mutex));
	}
	return M_R_SUCCESS;
}


m_result_t
segidx_create(m_segtbl_t *_segtbl, m_segidx_t **_segidxp)
{
	m_result_t       rv;
	m_segidx_t       *_segidx;
	m_segidx_entry_t *entries;
	m_segtbl_entry_t *segtbl_entry;
	int              i;

	if (!(_segidx = (m_segidx_t *) malloc(sizeof(m_segidx_t)))) {
		rv = M_R_NOMEMORY;
		goto out;
	}
	
	if (!(entries = (m_segidx_entry_t *) calloc(SEGMENT_TABLE_NUM_ENTRIES,
	                                            sizeof(m_segidx_entry_t))))
	{
		rv = M_R_NOMEMORY;
		goto err_calloc;
	}
	pthread_mutex_init(&(_segidx->mutex), NULL);
	_segidx->all_entries = entries;
	INIT_LIST_HEAD(&(_segidx->mapped_entries.list));
	INIT_LIST_HEAD(&(_segidx->free_entries.list));
	for (i=0; i < SEGMENT_TABLE_NUM_ENTRIES; i++) {
		segtbl_entry = entries[i].segtbl_entry = &_segtbl->entries[i];
		entries[i].index = i;
		entries[i].module_id = (uint64_t) (-1ULL);
		if (segtbl_entry->flags & SGTB_VALID_ENTRY) {
			segidx_insert_entry_ordered(_segidx, &(entries[i]), 0);
		} else {
			list_add_tail(&(entries[i].list), &(_segidx->free_entries.list));
		}
	}
	*_segidxp = _segidx;
	rv = M_R_SUCCESS;
	goto out;
err_calloc:
	free(_segidx);
out:
	return rv;
}


static
m_result_t
segidx_destroy(m_segidx_t *segidx)
{
	M_INTERNALERROR("Unimplemented functionality: segidx_destroy\n");
	return M_R_SUCCESS;
}	


static
m_result_t 
segidx_alloc_entry(m_segidx_t *segidx, m_segidx_entry_t **entryp)
{
	m_result_t       rv = M_R_FAILURE;
	m_segidx_entry_t *entry;
	struct list_head *free_head;
	

	pthread_mutex_lock(&(segidx->mutex));
	if ((free_head = segidx->free_entries.list.next)) {
		entry = list_entry(free_head, m_segidx_entry_t, list);
		list_del_init(&(entry->list));
	} else {
		rv = M_R_NOMEMORY;
		goto out;
	}
	*entryp = entry;
	rv = M_R_SUCCESS;
	goto out;

out:
	pthread_mutex_unlock(&(segidx->mutex));
	return rv;
}


static
m_result_t 
segidx_free_entry(m_segidx_t *segidx, m_segidx_entry_t *entry)
{
	m_result_t       rv = M_R_FAILURE;

	if (!entry) {
		rv = M_R_INVALIDARG;
		return rv;
	}	

	pthread_mutex_lock(&(segidx->mutex));
	list_del_init(&(entry->list));
	list_add(&(entry->list), &(segidx->free_entries.list));
	rv = M_R_SUCCESS;
	goto unlock;

unlock:
	pthread_mutex_unlock(&(segidx->mutex));
	return rv;
}


/* Not thread safe */
static
m_result_t 
segidx_find_entry_using_addr(m_segidx_t *segidx, void *addr, m_segidx_entry_t **entryp)
{
	m_segidx_entry_t *ientry;
	m_segtbl_entry_t *tentry;
	uintptr_t        start;
	uint32_t         size;

	list_for_each_entry(ientry, &segidx->mapped_entries.list, list) {
		tentry = ientry->segtbl_entry;
		start = tentry->start;
		size = tentry->size;
		if ((uintptr_t) addr >= start && (uintptr_t) addr < start+size) {
			*entryp = ientry;
			return M_R_SUCCESS;
		}
	}
	return M_R_FAILURE;
}



/* Not thread safe */
static
m_result_t 
segidx_find_entry_using_index(m_segidx_t *segidx, uint32_t index, m_segidx_entry_t **entryp)
{
	if (index >= SEGMENT_TABLE_NUM_ENTRIES) {
		return M_R_INVALIDARG;
	}
	*entryp = &segidx->all_entries[index];
	return M_R_SUCCESS;
}


/* Not thread safe */
static
m_result_t 
segidx_find_entry_using_module_id(m_segidx_t *segidx, uint64_t module_id, m_segidx_entry_t **entryp)
{
	m_segidx_entry_t *ientry;

	list_for_each_entry(ientry, &segidx->mapped_entries.list, list) {
		if (ientry->module_id == module_id) {
			*entryp = ientry;
			return M_R_SUCCESS;
		}
	}
	return M_R_FAILURE;
}

static
uintptr_t
segidx_find_free_region(m_segidx_t *segidx, uintptr_t start_addr, size_t length)
{
	struct list_head *tmp_list;
	m_segidx_entry_t *max_ientry;

	pthread_mutex_lock(&segidx->mutex); 
	if ((tmp_list = segidx->mapped_entries.list.prev) !=  &segidx->mapped_entries.list) {
		max_ientry = list_entry(tmp_list, m_segidx_entry_t, list);
		if (start_addr < (max_ientry->segtbl_entry->start +	max_ientry->segtbl_entry->size)) {
			start_addr = 0x0;
#ifdef TRY_ALLOC_IN_HOLES
			/* Check whether there is a hole where we can allocate memory */
			prev_end_addr = SEGMENT_MAP_START;
			list_for_each_entry(ientry, &(segidx->mapped_entries.list), list) {
				if (ientry->segtbl_entry->start - prev_end_addr > length) {
					start_addr = prev_end_addr;
					break;
				}
				prev_end_addr = ientry->segtbl_entry->start + ientry->segtbl_entry->size;
			}
#endif			
			/* If not found a hole then start from the maximum allocated address so far */
			if (!start_addr) {
				start_addr = max_ientry->segtbl_entry->start +	max_ientry->segtbl_entry->size;
			}	
		}
	}
	pthread_mutex_unlock(&segidx->mutex);
	return start_addr;
}


static inline
m_result_t
segment_table_map(m_segtbl_t *segtbl)
{
	char              segtbl_path[256];
	int               segtbl_fd;

	sprintf(segtbl_path, "%s/segment_table", SEGMENTS_DIR);
	if (m_check_backing_store(segtbl_path, SEGMENT_TABLE_SIZE) != M_R_SUCCESS) {
		mkdir_r(SEGMENTS_DIR, S_IRWXU);
		segtbl_fd = create_backing_store(segtbl_path, SEGMENT_TABLE_SIZE);
	} else {
		segtbl_fd = open(segtbl_path, O_RDWR);
	}
	segtbl->entries = segment_map((void *) SEGMENT_TABLE_START, 
	                              SEGMENT_TABLE_SIZE, 
	                              PROT_READ|PROT_WRITE,
	                              MAP_PERSISTENT | MAP_SHARED,
		                          segtbl_fd);
	if (segtbl->entries == MAP_FAILED) {
		assert(0 && "Going crazy...couldn't map the segment table\n");
		return M_R_FAILURE;
	}
	return M_R_SUCCESS;
}


/**
 * \brief (Re)incarnates the segment table.
 *
 * It maps the persistent segment table, fleshes out an index on top of it
 * and verifies existing backing stores.
 *
 * It does not map any other segments.
 */
static 
m_result_t
segment_table_incarnate()
{
	m_result_t rv;

	if ((rv = segment_table_map(&m_segtbl)) != M_R_SUCCESS) {
		return rv;
	}
	if ((rv = segidx_create(&m_segtbl, &(m_segtbl.idx))) != M_R_SUCCESS) {

	}
	verify_backing_stores(&m_segtbl);

	return M_R_SUCCESS;
}


static inline
void
segment_table_print(m_segtbl_t *segtbl)
{
	m_segidx_entry_t *ientry;
	m_segtbl_entry_t *tentry;
	uintptr_t        start;
	uintptr_t        end;

	M_DEBUG_PRINT(M_DEBUG_SEGMENT, "PERSISTENT SEGMENT TABLE\n");
	M_DEBUG_PRINT(M_DEBUG_SEGMENT, "========================\n");
	M_DEBUG_PRINT(M_DEBUG_SEGMENT, "%16s   %16s %17s %10s\n", "start", "end", "size", "flags");
	list_for_each_entry(ientry, &segtbl->idx->mapped_entries.list, list) {
		tentry = ientry->segtbl_entry;
		start = (uintptr_t) tentry->start;
		end   = (uintptr_t) tentry->start + (uintptr_t) tentry->size;
		M_DEBUG_PRINT(M_DEBUG_SEGMENT, "0x%016lx - 0x%016lx %16luK", start, end, (long unsigned int) tentry->size/1024);
		M_DEBUG_PRINT(M_DEBUG_SEGMENT, " %7c%c%c%c\n", ' ',
		              (tentry->flags & SGTB_VALID_ENTRY)? 'V': '-',
		              (tentry->flags & SGTB_VALID_DATA)? 'D': '-',
		              (tentry->flags & SGTB_TYPE_SECTION)? 'S': '-'
			         );
	}
}


void 
m_segment_table_print()
{
	segment_table_print(&m_segtbl);
}


m_result_t
m_segment_table_alloc_entry(m_segtbl_t *segtbl, m_segtbl_entry_t **entryp)
{
	m_result_t       rv;
	m_segidx_entry_t *ientry;

	rv = segidx_alloc_entry(segtbl->idx, &ientry);
	if (rv != M_R_SUCCESS) {
		goto out;
	}
	*entryp = ientry->segtbl_entry;

out:
	return rv;
}
	

/**
 * Assumes segment_fd points to a valid segment backing store. 
 */
static inline
void *
segment_map(void *addr, size_t size, int prot, int flags, int segment_fd)
{
	void      *segmentp;
	uintptr_t start;
	uintptr_t end;


	if (segment_fd < 0) {
		return ((void *) -1);
	}
	segmentp = mmap(addr, size, prot, 
	                flags | MAP_PERSISTENT| MAP_SHARED, 
		            segment_fd,
		            0);
				   
	if (segmentp == MAP_FAILED) {
		return segmentp;
	}
	/* 
	 * Ensure mapped segment falls into the address space region reserved 
	 * for persistent segments.
	 */
	start = (uintptr_t) segmentp;
	end = start + size;
	if (start < PSEGMENT_RESERVED_REGION_START || 
	    start > PSEGMENT_RESERVED_REGION_END ||
	    end > PSEGMENT_RESERVED_REGION_END) 
	{
		/* FIXME: unmap the segment */
		M_DEBUG_PRINT(M_DEBUG_SEGMENT, "   limits : %016lx - %016lx\n", PSEGMENT_RESERVED_REGION_START, PSEGMENT_RESERVED_REGION_END);
		M_DEBUG_PRINT(M_DEBUG_SEGMENT, "asked for : %016lx - %016lx\n", (uintptr_t) addr, (uintptr_t) addr + size);
		M_DEBUG_PRINT(M_DEBUG_SEGMENT, "      got : %016lx - %016lx\n", start, end);
		M_INTERNALERROR("Persistent segment not in the reserved address space region.\n");
		return MAP_FAILED;
	}

	/* we don't want page prefetching on the persistent segment */
	if (madvise(segmentp, size, MADV_RANDOM) < 0) {
		return MAP_FAILED;
	}
	return segmentp;
}


/**
 * Assumes segment_path points to a valid segment backing store. 
 */
static 
void *
segment_map2(void *addr, size_t size, int prot, int flags, char *segment_path)
{
	int  segment_fd;
	void *segmentp;

	segment_fd = open(segment_path, O_RDWR);
	if (segment_fd < 0) {
		return ((void *) -1);
	}

	segmentp = segment_map(addr, size, prot, flags, segment_fd);
				   
	if (segmentp == MAP_FAILED) {
		close (segment_fd);
		return segmentp;
	}
	return segmentp;
}


/**
 * \brief Reincarnates valid segments
 *
 * Assumes segment table already has an index attached to it.
 */
void
segment_reincarnate_segments(m_segtbl_t *segtbl)
{
	m_segidx_entry_t *ientry;
	m_segtbl_entry_t *tentry;
	uintptr_t        start;
	uintptr_t        end;
	char             path[256];
	void             *map_addr;

	list_for_each_entry(ientry, &segtbl->idx->mapped_entries.list, list) {
		tentry = ientry->segtbl_entry;
		if (tentry->flags & SGTB_TYPE_PMAP) {
			sprintf(path, "%s/%d.0", SEGMENTS_DIR, ientry->index);
		} else if (tentry->flags & SGTB_TYPE_SECTION) {
			sprintf(path, "%s/%d.%lu", SEGMENTS_DIR, ientry->index, (long unsigned int) ientry->module_id);
		} else {
			M_INTERNALERROR("Unknown persistent segment type.\n");
		}
		start = (uintptr_t) tentry->start;
		end   = (uintptr_t) tentry->start + (uintptr_t) tentry->size;
		/* 
		 * We pass MAP_FIXED to force the segment be mapped in its previous 
		 * address space region.
		 */
		/* FIXME: protection flags should be stored in the segment table */
		map_addr = segment_map2((void *) start, (size_t) tentry->size, 
								PROT_READ|PROT_WRITE,
								MAP_FIXED,
								path);
		if (map_addr == MAP_FAILED) {
			M_INTERNALERROR("Cannot reincarnate persistent segment.\n");
		}
	}
}


static
void *
pmap_internal_abs(void *start, unsigned long long length, int prot, int flags, 
                  m_segidx_entry_t **entryp, uint32_t segtbl_entry_flags, uint64_t module_id)
{
	char             path[256];
	uintptr_t        start_addr = (uintptr_t) start;
	void             *map_addr;
	int              fd;
	void             *rv = MAP_FAILED;
	m_segidx_entry_t *new_ientry;
	m_segtbl_entry_t *tentry;
	uint32_t         flags_val;
	

	if ((segidx_alloc_entry(m_segtbl.idx, &new_ientry)) != M_R_SUCCESS) {
		rv = MAP_FAILED;
		goto out;
	}	
	sprintf(path, "%s/%d.%lu", SEGMENTS_DIR, new_ientry->index, module_id);

	/* 
	 * Round-up the size of the segment to be an integer multiple of 4K pages.
	 * This doesn't add any extra memory overhead because the kernel already 
	 * round-ups and makes segment management simpler.
	 */
	length = SIZEOF_PAGES(length);

	if ((fd = create_backing_store(path, length)) < 0) {
		rv = MAP_FAILED;
		goto err_create_backing_store;
	}
	
	/* 
	 * Passing a start_addr that overlaps with a region will cause segment_map 
	 * (which in turn calls mmap) to return an address that is not always the 
	 * first address starting from start_addr to be found free. 
	 * I don't understand why so I make sure that I pass an address that 
	 * is guaranteed not to overlap with any other persistent segment.
	 */
	M_DEBUG_PRINT(M_DEBUG_SEGMENT, "start_addr = %p\n", (void *) start_addr);

	if ((flags & MAP_FIXED) != MAP_FIXED) {
		start_addr = segidx_find_free_region(m_segtbl.idx, start_addr, length);
	}	
	map_addr = segment_map((void *)start_addr, length, prot, flags, fd);
	M_DEBUG_PRINT(M_DEBUG_SEGMENT, "new_start_addr = %p\n", (void *) start_addr);
	M_DEBUG_PRINT(M_DEBUG_SEGMENT, "map_addr = %p\n", map_addr);
	if (map_addr == MAP_FAILED) {
		rv = MAP_FAILED;
		goto err_segment_map;
	}

	/* Now update the segment table with the necessary segment information. */
	tentry = new_ientry->segtbl_entry;
	PM_EQU(tentry->start, map_addr); /* PCM STORE */ 
	PM_EQU(tentry->size, length);  /* PCM STORE */
	PCM_WB_FENCE(NULL);
	PCM_WB_FLUSH(NULL, &(tentry->start));
	PCM_WB_FLUSH(NULL, &(tentry->size));
	flags_val = segtbl_entry_flags;
	PM_EQU(tentry->flags, flags_val);  /* PCM STORE */
	PCM_WB_FENCE(NULL);
	PCM_WB_FLUSH(NULL, &(tentry->flags));

	/* Insert the entry into the ordered index */
	segidx_insert_entry_ordered(m_segtbl.idx, new_ientry, 1);

	*entryp = new_ientry;
	rv = map_addr;
	goto out;

err_segment_map:
err_create_backing_store:
	segidx_free_entry(m_segtbl.idx, new_ientry);
out:
	return rv;
}


static
void *
pmap_internal(void *start, unsigned long long length, int prot, int flags, 
              m_segidx_entry_t **entryp, uint32_t segtbl_entry_flags, uint64_t module_id)
{
	uintptr_t        start_addr = SEGMENT_MAP_START + (uintptr_t) start;

	segment_table_print(&m_segtbl);
	return pmap_internal_abs((void *) start_addr, length, prot, flags, 
	                         entryp, segtbl_entry_flags, module_id);
}



/**
 * \brief Checks if there are any new .persistent sections and creates them
 *
 */
static
m_result_t 
segment_create_sections(m_segtbl_t *segtbl)
{
	void             *mapped_addr;
	size_t           length;
	uintptr_t        persistent_section_absolute_addr;
	uintptr_t        GOT_section_absolute_addr;
	m_result_t       rv;
	module_dsr_t     *module_dsr;
	m_segtbl_entry_t *tentry;
	m_segidx_entry_t *ientry;
	uint32_t         flags_val;
	Elf_Data         *elfdata;

	rv = m_module_create_module_dsr_list(&module_dsr_list);

	list_for_each_entry(module_dsr, &module_dsr_list, list) {
		M_DEBUG_PRINT(M_DEBUG_SEGMENT, "module_path = %s\n", module_dsr->module_path);
		M_DEBUG_PRINT(M_DEBUG_SEGMENT, "module_id   = %lu\n", module_dsr->module_inode);

		/* 
		 * If there is no valid entry for the persistent section of this module 
		 * then create it and map the segment.
		 */
		if (segidx_find_entry_using_module_id(segtbl->idx, module_dsr->module_inode, &ientry) 
		    != M_R_SUCCESS)
		{
			length = module_dsr->persistent_shdr.sh_size;
			mapped_addr = pmap_internal(0, length, PROT_READ|PROT_WRITE, 0, &ientry, 
	                                    SGTB_TYPE_SECTION | SGTB_VALID_ENTRY, module_dsr->module_inode);
			if (mapped_addr == MAP_FAILED) {
				M_INTERNALERROR("Cannot map .persistent section's segment.\n");
			}
		} else {
			mapped_addr = (void *) ientry->segtbl_entry->start;
			length = ientry->segtbl_entry->size;
		}

		/* 
		 * Our linker script ensures that relocation for modules that have 
		 * a .persistent section will always happen at offset 0x400000.
		 */

		persistent_section_absolute_addr = (uintptr_t) (module_dsr->persistent_shdr.sh_addr+module_dsr->module_start-0x400000);
		GOT_section_absolute_addr = (uintptr_t) (module_dsr->GOT_shdr.sh_addr+module_dsr->module_start-0x400000);

		M_DEBUG_PRINT(M_DEBUG_SEGMENT, "persistent_section.start         = %p\n", 
		              (void *) persistent_section_absolute_addr);
		M_DEBUG_PRINT(M_DEBUG_SEGMENT, "persistent_section.end           = %p\n", 
		              (void *) (persistent_section_absolute_addr + 
		              module_dsr->persistent_shdr.sh_size));
		M_DEBUG_PRINT(M_DEBUG_SEGMENT, "persistent_section.sh_size       = %d\n", 
		              (int) module_dsr->persistent_shdr.sh_size);
		M_DEBUG_PRINT(M_DEBUG_SEGMENT, "persistent_section.sh_size_pages = %d\n", 
		              (int) SIZEOF_PAGES(module_dsr->persistent_shdr.sh_size));
		M_DEBUG_PRINT(M_DEBUG_SEGMENT, "GOT_section.start                = %p\n", 
		              (void *) GOT_section_absolute_addr);
		M_DEBUG_PRINT(M_DEBUG_SEGMENT, "GOT_section.end                  = %p\n", 
		              (void *) GOT_section_absolute_addr+module_dsr->GOT_shdr.sh_size);
		M_DEBUG_PRINT(M_DEBUG_SEGMENT, "GOT_section.sh_size              = %d\n", 
		              (int) module_dsr->GOT_shdr.sh_size);


		m_module_relocate_symbols(GOT_section_absolute_addr,
		                          GOT_section_absolute_addr + module_dsr->GOT_shdr.sh_size,
		                          persistent_section_absolute_addr, 
		                          persistent_section_absolute_addr + module_dsr->persistent_shdr.sh_size, 
		                          (uintptr_t) mapped_addr,
		                          (uintptr_t) mapped_addr+length
		                         );

		/* If data not valid then load them using the data found in the .persistent section */
		if (!(ientry->segtbl_entry->flags & SGTB_VALID_DATA))
		{
			elfdata = NULL;
			while ((elfdata = elf_getdata(module_dsr->persistent_scn, elfdata)))
			{
				M_DEBUG_PRINT(M_DEBUG_SEGMENT, "data.d_size = %d\n", (int) elfdata->d_size);	
				M_DEBUG_PRINT(M_DEBUG_SEGMENT, "data.d_buf = %p\n", elfdata->d_buf);	
				M_DEBUG_PRINT(M_DEBUG_SEGMENT, "start_addr = %lx\n", (uintptr_t) mapped_addr);	
				M_DEBUG_PRINT(M_DEBUG_SEGMENT, "end_addr = %lx\n", (uintptr_t) mapped_addr + length);	
				M_DEBUG_PRINT(M_DEBUG_SEGMENT, "length     = %u\n", (unsigned int) length);	
				PM_MEMCPY(mapped_addr, elfdata->d_buf, elfdata->d_size);
			}

			tentry = ientry->segtbl_entry;
			flags_val = tentry->flags | SGTB_VALID_DATA; // PM_LOAD
			PM_EQU(tentry->flags, flags_val); /* PCM STORE */
			PCM_WB_FENCE(NULL);
			PCM_WB_FLUSH(NULL, &(tentry->flags));
		}

	} /* end of list_for_each_entry */

	return M_R_SUCCESS;
}



/**
 * \brief Creates the segment manager and reincarnates any previous life
 * persistent segments.
 */
m_result_t 
m_segmentmgr_init()
{
	char buf[256];

	/* Clear previous life segments? */
	if (mcore_runtime_settings.reset_segments) {
		/* what if buffer overflow attack -- who cares, this is a prototype */
		sprintf(buf, "rm -rf %s", SEGMENTS_DIR);
		system(buf);
	}

	segment_table_incarnate();
	segment_reincarnate_segments(&m_segtbl);
	segment_create_sections(&m_segtbl);

	segment_table_print(&m_segtbl);

	return M_R_SUCCESS;
}


/**
 * \brief Shutdowns the segment manager.
 */
m_result_t 
m_segmentmgr_fini()
{
	/* Nothing really to do here. */
	return M_R_SUCCESS;
}


/* Not thread safe */
m_result_t 
m_segment_find_using_addr(void *addr, m_segidx_entry_t **entryp)
{
	return segidx_find_entry_using_addr(m_segtbl.idx, addr, entryp);
}



/**
 * \brief Maps an address space region into persistent memory.
 *
 * Start address is offset by SEGMENT_MAP_START.
 */
void *
m_pmap(void *start, unsigned long long length, int prot, int flags)
{
	m_segidx_entry_t *ientry;
	void             *rv;
	rv = pmap_internal(start, length, prot, flags, &ientry, 
	                   SGTB_TYPE_PMAP | SGTB_VALID_ENTRY | SGTB_VALID_DATA, 0);
	return rv;
}


/**
 * \brief Maps an address space region into persistent memory.
 *
 */
void *
m_pmap2(void *start, unsigned long long length, int prot, int flags)
{
	m_segidx_entry_t *ientry;
	void             *rv;

	rv = pmap_internal_abs(start, length, prot, flags, &ientry, 
	                       SGTB_TYPE_PMAP | SGTB_VALID_ENTRY | SGTB_VALID_DATA, 0);
	return rv;
}



int 
m_punmap(void *start, size_t length)
{
	M_INTERNALERROR("Unimplemented functionality: m_punmap\n");
	return 0;
}
