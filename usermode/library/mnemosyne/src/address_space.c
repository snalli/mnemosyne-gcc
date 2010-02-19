/*!
 * \file
 * Implements the routines defined in address_space.h.
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */
#include "address_space.h"
#include "segment.h"
#include <gelf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*! The maximum path length for any module name. */
#define PATH_MAX 256

/*!
 * A static-size set of module address space descriptors. A module is a dynamically
 * loaded piece of code that may have its own persistent segments. Libraries are
 * the usual suspects.
 */
struct module_list_s {
	char     path[128][PATH_MAX];
	uint64_t base[128];
	int      size;
	int      num;
};
typedef struct module_list_s module_list_t;

/*!
 * Retrieve the list of dynamically loaded modules (e.g. libraries) that need to have
 * their persistent data mapped into this process.
 *
 * \param module_list should point to a module_list_t structure. This structure will
 *  be written with the list of modules which should be loaded in this address space.
 */
static
mnemosyne_result_t get_module_list(module_list_t *module_list);


mnemosyne_result_t mnemosyne_segment_address_space_checkpoint()
{
	char               *modulename;
	FILE               *fsegment_table;
	void               *segment_start;
	unsigned long long segment_length;
	char               segment_backing_store_path[128];
	module_list_t      module_list;
	GElf_Shdr          persistent_shdr;
	uint64_t           persistent_shdr_absolute_addr;
	int                i;
	
	mkdir_r(SEGMENTS_DIR, S_IRWXU);
	
	/* Checkpoint the static persistent segment of each loaded module (ELF shdr) */
	module_list.size = 128;
	get_module_list(&module_list);
	for (i=0; i<module_list.num; i++) {
		if (get_persistent_shdr(module_list.path[i], &persistent_shdr)
		    == MNEMOSYNE_R_SUCCESS) 
		{	
			//FIXME: assuming relocation happens always at 0x400000
			persistent_shdr_absolute_addr = (uint64_t) (persistent_shdr.sh_addr+module_list.base[i]-0x400000);
			path2file(module_list.path[i], &modulename);
			sprintf(segment_backing_store_path, "%s/segment_%s", SEGMENTS_DIR, modulename);
			save_memory_to_file(segment_backing_store_path, 
			                    (void *) persistent_shdr_absolute_addr, 
			                    persistent_shdr.sh_size);
		}	
	}
	
	/* Checkpoint the dynamic segments */
	pthread_mutex_init(&segment_list.mutex, NULL);
	sprintf(segment_table_path, "%s/segment_table", SEGMENTS_DIR);
	fsegment_table = fopen(segment_table_path, "r");
	if (fsegment_table) {
		while(!feof(fsegment_table)) {
			fscanf(fsegment_table, "%p %llu %s\n", 
			       &segment_start, &segment_length,
			       &segment_backing_store_path);
			save_memory_to_file(segment_backing_store_path,
			                    segment_start, segment_length);
		}
		fclose(fsegment_table);
	}
	
	return MNEMOSYNE_R_SUCCESS;
}


mnemosyne_result_t mnemosyne_segment_address_space_reincarnate()
{
	char               *modulename;
	FILE               *fsegment_table;
	void               *segment_start;
	unsigned long long segment_length;
	char               segment_backing_store_path[128];
	module_list_t      module_list;
	GElf_Shdr          persistent_shdr;
	uint64_t           persistent_shdr_absolute_addr;
	int i;
	
	/* Reincarnate the static persistent segment of each loaded module (ELF shdr) */
	module_list.size = 128;
	get_module_list(&module_list);
	for (i=0; i<module_list.num; i++) {
		if (get_persistent_shdr(module_list.path[i], &persistent_shdr)
		    == MNEMOSYNE_R_SUCCESS)
		{
			//FIXME: assuming relocation happens always at 0x400000
			persistent_shdr_absolute_addr = (uint64_t) (persistent_shdr.sh_addr+module_list.base[i]-0x400000);
			path2file(module_list.path[i], &modulename);
			sprintf(segment_backing_store_path, "%s/segment_%s", SEGMENTS_DIR, modulename);
			load_memory_from_file(segment_backing_store_path, 
			                      (void *) persistent_shdr_absolute_addr, 
			                      persistent_shdr.sh_size);
		}
	}
	
	/* Reincarnate the dynamic segments */
	pthread_mutex_init(&segment_list.mutex, NULL);
	INIT_LIST_HEAD(&segment_list.list);
	sprintf(segment_table_path, "%s/segment_table", SEGMENTS_DIR);
	fsegment_table = fopen(segment_table_path, "r");
	if (fsegment_table) {
		while(!feof(fsegment_table)) {
			fscanf(fsegment_table, "%p %llu %s\n", 
			       &segment_start, &segment_length,
			       &segment_backing_store_path);
			mnemosyne_segment_create(segment_start, segment_length, 0, 0);
			load_memory_from_file(segment_backing_store_path,
			                      segment_start, segment_length);
		}
		fclose(fsegment_table);
	}
}


mnemosyne_result_t get_module_list(module_list_t *module_list)
{
	char fname[64]; /* /proc/<pid>/exe */
	pid_t pid;
	FILE *f;
	char buf[128+PATH_MAX];
	char perm[5], dev[6], mapname[PATH_MAX];
	uint64_t begin, end, inode, foo;
	char prev_mapname[PATH_MAX];
	int i;

	
	/* Get our PID and build the name of the link in /proc */
	pid = getpid();
	
	if (snprintf(fname, sizeof(fname), "/proc/%i/maps", pid) < 0) {
		return MNEMOSYNE_R_FAILURE;
	}
	if ((f = fopen(fname, "r")) == NULL) {
		return MNEMOSYNE_R_FAILURE;
	}
	module_list->num = i = 0;
	while (!feof(f)) {
		if(fgets(buf, sizeof(buf), f) == 0)
			break;
		mapname[0] = '\0';
		sscanf(buf, "%llx-%llx %4s %llx %5s %llu %s", &begin, &end, perm,
		       &foo, dev, &inode, mapname);
		if (strlen(mapname) > 0 && strcmp(prev_mapname, mapname) != 0)	{
			strcpy(module_list->path[i], mapname);
			module_list->base[i] = begin;
			module_list->num++;
			i++;
		}
		strcpy(prev_mapname, mapname);
		if (i>module_list->size) {
			return MNEMOSYNE_R_FAILURE;
		}
	}

	return MNEMOSYNE_R_SUCCESS;
}
