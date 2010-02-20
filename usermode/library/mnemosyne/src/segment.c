#include <string.h>
#include <err.h>
#include <fcntl.h>
#include <gelf.h>
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <assert.h>
#include "mnemosyne_i.h"
#include "segment.h"

#define SEGMENTS_DIR ".segments"

typedef struct mnemosyne_segment_file_header_s mnemosyne_segment_file_header_t;
typedef struct mnemosyne_segment_s mnemosyne_segment_t;
typedef struct mnemosyne_segment_list_node_s mnemosyne_segment_list_node_t;

struct mnemosyne_segment_file_header_s {
	uint32_t num_segments;
};

struct mnemosyne_segment_s {
	void   *start;
	size_t length;
	char   *backing_store_path;
};

struct mnemosyne_segment_list_node_s {
	struct list_head    node;
	mnemosyne_segment_t segment;
};

char                     segment_table_path[256];


void 
mkdir_r(const char *dir, mode_t mode) 
{
	char   tmp[256];
	char   *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", dir);
	len = strlen(tmp);
	if(tmp[len - 1] == '/') {
		tmp[len - 1] = 0;
		for(p = tmp + 1; *p; p++)
			if(*p == '/') {
				*p = 0;
				mkdir(tmp, mode);
				*p = '/';
		}
	}	
	mkdir(tmp, mode);
}


/* TODO: this should be moved under platform specific directory */
/**
 * get_exe_name - Get the filename of the currently running executable
 *
 * The get_exe_name() function copies an absolute filename of the currently 
 * running executable to the array pointed to by buf, which is of length size.
 *
 */
static 
char* 
get_exe_name(char* buf, size_t size)
{
	char linkname[64]; /* /proc/<pid>/exe */
	pid_t pid;
	int ret;
	
	/* Get our PID and build the name of the link in /proc */
	pid = getpid();
	
	if (snprintf(linkname, sizeof(linkname), "/proc/%i/exe", pid) < 0) {
		/* 
		 * This should only happen on large word systems. I'm not sure
		 * what the proper response is here.
		 * Since it really is an assert-like condition, aborting the
		 * program seems to be in order. 
		 */
		abort();
	}

	
	/* Now read the symbolic link */
	ret = readlink(linkname, buf, size);
	
	/* In case of an error, leave the handling up to the caller */
	if (ret == -1) {
		return NULL;
	}	
	
	/* Report insufficient buffer size */
	if (ret >= size) {
		errno = ERANGE;
		return NULL;
	}
	
	/* Ensure proper NULL termination */
	buf[ret] = 0;
	
	return buf;
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


static
mnemosyne_result_t
sync_segment_table()
{
	char                          segment_table_path[256];
	char                          segment_table_path_tildes[256];
	mnemosyne_segment_list_node_t *iter;
	int                           i;
	int                           fd_segment_table;
	char                          buf[256];

	sprintf(segment_table_path, "%s/segment_table", SEGMENTS_DIR);
	sprintf(segment_table_path_tildes, "%s/segment_table~", SEGMENTS_DIR);
	mkdir_r(SEGMENTS_DIR, S_IRWXU);
	fd_segment_table = open(segment_table_path_tildes, 
	                        O_CREAT | O_TRUNC| O_WRONLY, 
	                        S_IRWXU);
	if (fd_segment_table<0) {
		return MNEMOSYNE_R_FAILURE;
	}
	i = 0;
	list_for_each_entry(iter, &segment_list.list, node) {
		sprintf(buf, "%p %llu %s/segment_file_%d\n",
		        iter->segment.start, 
		        (unsigned long long) iter->segment.length, 
		        SEGMENTS_DIR,
		        i);
		write(fd_segment_table, buf, strlen(buf));		
		i++;		
	}
	fsync(fd_segment_table);
	close(fd_segment_table);
	rename(segment_table_path_tildes, segment_table_path);
	//FIXME: do we need another sync to ensure directory 
	// metadata are synced? if yes, then what do you sync? the directory file?
	return MNEMOSYNE_R_SUCCESS;
}


/* FIXME: Currently atomicity is broken. 
 * If you have successfully acquired a persistent segment but then crash before 
 * noting it into your segment_table then the segment is lost. We could deal
 * with this by logging our intention but this should be something to consider
 * in the design of transactions. Moreover we might need to drop the usermode
 * approach of keeping the segments in a file because the kernel can't really 
 * trust this file. Since it will have to account of how much memory is given to each
 * program/process we might just implement the functionality in the kernel
 * and avoid duplication.
 */
void *
mnemosyne_segment_create(void *start, size_t length, int prot, int flags)
{
	mnemosyne_segment_list_node_t *iter;
	mnemosyne_segment_list_node_t *segment_node;
	void                          *ptr;

	/* Ensure segment does not already exist. */
	list_for_each_entry(iter, &segment_list.list, node) {
		if ((uint64_t) iter->segment.start <= (uint64_t) start &&
		    (uint64_t) iter->segment.start + (uint64_t) iter->segment.length > (uint64_t) start) 
		{
			return NULL;
		}
	}

	segment_node = (mnemosyne_segment_list_node_t *) 
	               malloc(sizeof(mnemosyne_segment_list_node_t));
	if (!segment_node) {
		return NULL;
	}
	pthread_mutex_lock(&segment_list.mutex);
	segment_node->segment.start = start;
	segment_node->segment.length = length;

	/* 
	 * FIXME: backing_store_path is currently assigned when syncing 
	 * the table but this should really be assigned by the OS when segments 
	 * are implemented by the the VM subsystem 
	 */
	/* segment_node->segment.backing_store_path = ...; */
	list_add(&segment_node->node, &segment_list.list);
	sync_segment_table();
	ptr = mmap(start, length, PROT_READ|PROT_WRITE,  MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	assert(ptr == start); /* FIXME: mmap uses start as a hint -- our interface does not */
	pthread_mutex_unlock(&segment_list.mutex);
	
	return segment_node->segment.start;
}


/* TODO: destroy just the requested region and not the whole segment */
/* assumes no segment overlapping */
int
mnemosyne_segment_destroy(void *start, size_t length)
{
	mnemosyne_segment_list_node_t *iter;
	mnemosyne_segment_list_node_t *node_to_delete = NULL;

	pthread_mutex_lock(&segment_list.mutex);
	list_for_each_entry(iter, &segment_list.list, node) {
		if ((uint64_t) iter->segment.start <= (uint64_t) start &&
		    (uint64_t) iter->segment.start + (uint64_t) iter->segment.length > (uint64_t) start) 
		{
			node_to_delete = iter;
			break;
		}
	}

	if (node_to_delete != NULL) {
		list_del(&node_to_delete->node);
		free(node_to_delete);
		sync_segment_table();
	}	
	pthread_mutex_unlock(&segment_list.mutex);
	
	return 0;
}


mnemosyne_result_t
path2file(char *path, char **file)
{
	int i;

	for (i=strlen(path); i>=0; i--) {
		if (path[i] == '/') {
			*file = &path[i+1];
			return MNEMOSYNE_R_SUCCESS;
		}
	}
	return MNEMOSYNE_R_FAILURE;
}
