/*!
 * \file
 * Implements the routines defined in address_space.h.
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */
#include "address_space.h"
#include "files.h"
#include "segment.h"
#include <err.h>
#include <fcntl.h>
#include <gelf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
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

/*!
 * Parses a binary header named ".persistent" in a given module (e.g. library).
 *
 * \param modulename is the name of the library whose header will be gathered.
 * \param persistent_shdr Will be written with the ELF header of the module
 *  analyzed. This must not be NULL.
 *
 * \return MNEMOSYNE_SUCCESS if the header was successfully parsed. MNEMOSYNE_R_FAILURE
 *  if the persistent section was not found. MNEMOSYNE_R_INVALIDFILE if the file
 *  does not appear to be an ELF header.
 */
mnemosyne_result_t get_persistent_shdr(char *modulename, GElf_Shdr *persistent_shdr);

/*!
 * Opens a file for reading and loads its contents into a memory buffer.
 *
 * \param file is the file containing the data.
 * \param mem_start is the buffer into which data will be read. This must not be NULL.
 * \param size is the number of bytes to read into mem_start.
 *
 * \return MNEMOSYNE_R_SUCCESS if the writing was successful. MNEMOSYNE_R_INVALIDFILE
 *  if file is not readable.
 */
mnemosyne_result_t load_memory_from_file(char *file, void *mem_start, size_t size);

/*!
 * Opens a file, writes out data from a buffer, and closes the file.
 * 
 * \param file is the name of the file to write to. Writing will commence
 *  at the beginning of the file.
 * \param mem_start is the buffer which will be written out.
 * \param size is the number of bytes in mem_start to write to the file.
 *
 * \return MNEMOSYNE_R_SUCCESS if the writing was successful. MNEMOSYNE_R_INVALIDFILE
 *  if file is not writeable.
 */
mnemosyne_result_t save_memory_to_file(char *file, void *mem_start, size_t size);


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
			fscanf(fsegment_table, "%p %llu %128s\n", 
			       &segment_start, &segment_length,
			       segment_backing_store_path);
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
			fscanf(fsegment_table, "%p %llu %128s\n", 
			       &segment_start, &segment_length,
			       segment_backing_store_path);
			mnemosyne_segment_create(segment_start, segment_length, 0, 0);
			load_memory_from_file(segment_backing_store_path,
			                      segment_start, segment_length);
		}
		fclose(fsegment_table);
	}
	
	return MNEMOSYNE_R_SUCCESS;
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
		sscanf(buf, "%lx-%lx %4s %lx %5s %lu %s", &begin, &end, perm,
		       &foo, dev, &inode, mapname);
		if (strlen(mapname) > 0 && strcmp(prev_mapname, mapname) != 0)	{
			strcpy(module_list->path[i], mapname);
			module_list->base[i] = begin;
			module_list->num++;
			i++;
			strcpy(prev_mapname, mapname);
		}
		if (i>module_list->size) {
			return MNEMOSYNE_R_FAILURE;
		}
	}

	return MNEMOSYNE_R_SUCCESS;
}


mnemosyne_result_t 
get_persistent_shdr(char *modulename, GElf_Shdr *persistent_shdr)
{
	int       fd;
	Elf       *e;
	char      *name;
    Elf_Scn   *scn;
	GElf_Shdr shdr;
	size_t    shstrndx;
	GElf_Ehdr ehdr;

	if (elf_version(EV_CURRENT) == EV_NONE) {
		errx(EX_SOFTWARE, "ELF library initialization failed: %s", elf_errmsg(-1));
	}		 

	if ((fd = open(modulename, O_RDONLY, 0)) < 0) {
		return MNEMOSYNE_R_INVALIDFILE;
	}	

	if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
		errx(EX_SOFTWARE, "elf_begin() failed: %s.", elf_errmsg(-1));
	}	

	if (elf_kind(e) != ELF_K_ELF) {
		return MNEMOSYNE_R_INVALIDFILE;
	}

	gelf_getehdr(e, &ehdr);
	shstrndx = ehdr.e_shstrndx;

	scn = NULL;
	while ((scn = elf_nextscn(e, scn)) != NULL) {
		if (gelf_getshdr(scn, &shdr) != &shdr) {
			errx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));
		}	

		if ((name = elf_strptr(e, shstrndx, shdr.sh_name)) == NULL) {
			errx(EX_SOFTWARE, "elf_strptr() failed: %s.", elf_errmsg(-1));
		}

		if (strcmp(name, ".persistent") == 0) {
			memcpy((void *) persistent_shdr, &shdr, sizeof(GElf_Shdr)); 		
			return MNEMOSYNE_R_SUCCESS;		
		}			
	}

	(void) elf_end(e);
	(void) close(fd);
	return MNEMOSYNE_R_FAILURE;
}


mnemosyne_result_t
load_memory_from_file(char *file, void *mem_start, size_t size)
{
	FILE     *fp;
	uint64_t i;
	char     byte;
	char     *start = (char *) mem_start;

	if ((fp = fopen(file, "r")) == NULL) {
		return MNEMOSYNE_R_INVALIDFILE;
	}
	for (i=0; i<size; i++) {
		byte = fgetc(fp);
		start[i] = byte;
	}
	fclose(fp);

	return MNEMOSYNE_R_SUCCESS;
}


mnemosyne_result_t
save_memory_to_file(char *file, void *mem_start, size_t size)
{
	FILE     *fp;
	uint64_t i;
	char     byte;
	char     *start = (char *) mem_start;

	if ((fp = fopen(file, "w")) == NULL) {
		return MNEMOSYNE_R_INVALIDFILE;
	}
	for (i=0; i<size; i++) {
		byte = start[i];
		fputc(byte, fp);
	}
	fclose(fp);

	return MNEMOSYNE_R_SUCCESS;
}
