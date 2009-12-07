#include <common/mnemosyne_i.h>
#include <common/list.h>
#include <common/segment.h>

#include <err.h>
#include <fcntl.h>
#include <gelf.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>


char spring_path[256];


struct mnemosyne_spring_s {
	uint32_t num_segments;
};


struct mnemosyne_segment_s {
	void                              *offset;
	size_t                            size;
	mnemosyne_segment_backing_store_t *backing_store;
};


struct mnemosyne_segment_list_node_s {
	struct list_head node;
	struct mnemosyne_segment_s segment;
};

struct mnemosyne_segment_list_s {
	pthread_mutex_t *mutex;
	struct list_head list;
};

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

static
void
sync_spring()
{


}

/* TODO: this should be moved under platform specific directory */

/**
 * getexename - Get the filename of the currently running executable
 *
 * The getexename() function copies an absolute filename of the currently 
 * running executable to the array pointed to by buf, which is of length size.
 *
 * If the filename would require a buffer longer than size elements, NULL is
 * returned, and errno is set to ERANGE; an application should check for this
 * error, and allocate a larger buffer if necessary.
 *
 * Return value:
 * NULL on failure, with errno set accordingly, and buf on success. The 
 * contents of the array pointed to by buf is undefined on error.
 *
 * Notes:
 * This function is tested on Linux only. It relies on information supplied by
 * the /proc file system.
 * The returned filename points to the final executable loaded by the execve()
 * system call. In the case of scripts, the filename points to the script 
 * handler, not to the script.
 * The filename returned points to the actual exectuable and not a symlink.
 *
 */
static 
char* 
getexename(char* buf, size_t size)
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


int
get_static_segment(char *exename)
{
	int fd;
	Elf *e;
	char *name;
	char *p;
	char pc[4*sizeof(char)];
    Elf_Scn *scn;
	Elf_Data *data;
	GElf_Shdr shdr;
	size_t n, shstrndx, sz;
	GElf_Ehdr *ehdr;

	if (elf_version(EV_CURRENT) == EV_NONE) {
		errx(EX_SOFTWARE, "ELF library initialization failed: %s", elf_errmsg(-1));
	}		 

	if ((fd = open(exename, O_RDONLY, 0)) < 0) {
		err(EX_NOINPUT, "open \%s\" failed", exename);
	}	

	if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
		errx(EX_SOFTWARE, "elf_begin() failed: %s.", elf_errmsg(-1));
	}	

	if (elf_kind(e) != ELF_K_ELF) {
		errx(EX_DATAERR, "%s is not an ELF object.", exename);
	}	

	ehdr = elf64_getehdr(e);
	//ehdr = elf32_getehdr(e);
	//if (elf_getshstrndx(e, &shstrndx) == 0)
	//        errx(EX_SOFTWARE, "getshstrndx() failed: %s.", elf_errmsg(-1));
	shstrndx = ehdr->e_shstrndx;

	scn = NULL;
	while ((scn = elf_nextscn(e, scn)) != NULL) {
		if (gelf_getshdr(scn, &shdr) != &shdr) {
			errx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));
		}	

		if ((name = elf_strptr(e, shstrndx, shdr.sh_name)) == NULL) {
			errx(EX_SOFTWARE, "elf_strptr() failed: %s.", elf_errmsg(-1));
		}	

		printf("Section %-4.4jd %s %x %d\n", (uintmax_t) elf_ndxscn(scn),
		        name, shdr.sh_addr, shdr.sh_size);
	}

	if ((scn = elf_getscn(e, shstrndx)) == NULL) {
		errx(EX_SOFTWARE, "getscn() failed: %s.", elf_errmsg(-1));
	}	

	if (gelf_getshdr(scn, &shdr) != &shdr) {
		errx(EX_SOFTWARE, "getshdr(shstrndx) failed: %s.", elf_errmsg(-1));
	}	

	printf(".shstrab: size=%jd\n", (uintmax_t) shdr.sh_size);

	data = NULL; n = 0;
	while (n < shdr.sh_size && (data = elf_getdata(scn, data)) != NULL) {
		p = (char *) data->d_buf;
		while (p < (char *) data->d_buf + data->d_size) {
			n++; p++;
			(void) putchar((n % 16) ? ' ' : '\n');
		}
	}
	(void) putchar('\n');

	(void) elf_end(e);
	(void) close(fd);
}

void
mnemosyne_segment_reincarnate()
{
	char segment[256];
	char segment_path[256];
	char exename[128];
	FILE *fspring;
	FILE *fsegment;

	/* Reincarnate the static segment */
	sprintf(segment_path, ".spring/segment_static");
	fsegment = fopen(segment_path, "r");

		getexename(exename, 128);
		printf("exename: %s\n", exename);
		get_static_segment(exename);
	if (fsegment) {
		getexename(exename, 128);
		printf("exename: %s\n", exename);
	}
	

	/* Reincarnate the dynamic segments */
	sprintf(spring_path, ".spring/spring_descriptor");

	fspring = fopen(spring_path, "r");

	if (fspring) {

		sprintf(segment_path, ".spring/%s", segment);
	}
}


void
mnemosyne_segment_load()
{


}


void *
mnemosyne_segment_create(void *start, size_t length, int prot, int flags)
{

	sync_spring();

}

int
mnemosyne_segment_destroy(void *start, size_t length)
{
	
}
