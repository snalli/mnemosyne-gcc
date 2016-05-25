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

/*!
 * \file
 * Implements the routines defined in module.h.
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */
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
#include <sys/mman.h>
#include <unistd.h>
#include "module.h"
#include "files.h"
#include "list.h"
#include "debug.h"
#include <pm_instr.h>

static m_result_t persistent_shdr_open(char *module_path, uint64_t module_inode, uintptr_t module_start, module_dsr_t *module_dsr);

struct list_head module_dsr_list;

/**
 * \brief Relocates symbols that fall in the range of addresses by 
 * offsetting their addresses stored in the global offset table (GOT).
 *
 * Note: The actual data are not transferred. This function only 
 * hijacks the global offset table.
 */
void
m_module_relocate_symbols(uintptr_t got_start,
                          uintptr_t got_end,
                          uintptr_t old_start, 
                          uintptr_t old_end, 
                          uintptr_t new_start, 
                          uintptr_t new_end)
{
	uintptr_t entry;
	uintptr_t symbol_old_addr;
	uintptr_t symbol_new_addr;

	M_DEBUG_PRINT(M_DEBUG_SEGMENT, "relocate.got_start = %lx\n", got_start);
	M_DEBUG_PRINT(M_DEBUG_SEGMENT, "relocate.got_end   = %lx\n", got_end);
	M_DEBUG_PRINT(M_DEBUG_SEGMENT, "relocate.old_start = %lx\n", old_start);
	M_DEBUG_PRINT(M_DEBUG_SEGMENT, "relocate.old_end   = %lx\n", old_end);
	M_DEBUG_PRINT(M_DEBUG_SEGMENT, "relocate.new_start = %lx\n", new_start);
	M_DEBUG_PRINT(M_DEBUG_SEGMENT, "relocate.new_end   = %lx\n", new_end);

	mprotect(got_start - (got_start % 4096), got_end - (got_start - (got_start % 4096)), PROT_READ | PROT_WRITE | PROT_EXEC);
	
	for (entry = got_start; entry < got_end; entry+=sizeof(uintptr_t)) {
		symbol_old_addr = *((uintptr_t *) entry);
		/* Symbol in range? */
		if (symbol_old_addr >= old_start && symbol_old_addr < old_end) {
			symbol_new_addr = symbol_old_addr - old_start + new_start;
			*((uintptr_t *) entry) = symbol_new_addr;

			M_DEBUG_PRINT(M_DEBUG_SEGMENT, "relocate_symbol: %lx ==> %lx\n", symbol_old_addr, symbol_new_addr );
		}
	}
}


/*!
 * \brief Parses the .persistent and .got sections of a given module (e.g. library)
 * if a .persistent section exists.
 *
 * \param module_path The name of the library whose header will be gathered.
 * \param module_inode The inode number of the module's binary file.
 * \param module_start The virtual address where the module is loaded in the process 
 *                     address space
 * \param module_dsr Gets written with information about the .persistent and .got sections.
 *                This must not be NULL.
 *
 * \return MNEMOSYNE_SUCCESS if the header was successfully parsed. M_R_FAILURE
 *  if the persistent section was not found. M_R_INVALIDFILE if the file
 *  does not appear to be an ELF header.
 */
static 
m_result_t 
persistent_shdr_open(char *module_path, 
                     uint64_t module_inode, 
                     uintptr_t module_start, 
                     module_dsr_t *module_dsr)
{
	m_result_t result = M_R_FAILURE;
	int                fd;
	Elf                *e;
	char               *name;
    Elf_Scn            *scn;
	GElf_Shdr          shdr;
	size_t             shstrndx;
	GElf_Ehdr          ehdr;
	int                have_persistent = 0;

	if (module_dsr == NULL) {
		result = M_R_INVALIDARG;
		goto out;
	}

	if (elf_version(EV_CURRENT) == EV_NONE) {
		errx(EX_SOFTWARE, "ELF library initialization failed: %s", elf_errmsg(-1));
	}

	if ((fd = open(module_path, O_RDONLY, 0)) < 0) {
		result = M_R_INVALIDFILE;
		goto out;
	}	

	if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
		errx(EX_SOFTWARE, "elf_begin() failed: %s.", elf_errmsg(-1));
	}	

	if (elf_kind(e) != ELF_K_ELF) {
		result = M_R_INVALIDFILE;
		goto err_end;
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
			module_dsr->persistent_scn = scn;
			PM_MEMCPY((void *) &module_dsr->persistent_shdr, &shdr, sizeof(GElf_Shdr));
			have_persistent = 1;
			break;
		}			
	}

	if (have_persistent) {
		PM_STRCPY(module_dsr->module_path, module_path);
		module_dsr->module_start = module_start;
		module_dsr->module_inode = module_inode;
		module_dsr->fd = fd;
		module_dsr->elf = e;

		/* Find the global offset table */
		scn = NULL;
		while ((scn = elf_nextscn(e, scn)) != NULL) {
			if (gelf_getshdr(scn, &shdr) != &shdr) {
				errx(EX_SOFTWARE, "getshdr() failed: %s.", elf_errmsg(-1));
			}	
			if ((name = elf_strptr(e, shstrndx, shdr.sh_name)) == NULL) {
				errx(EX_SOFTWARE, "elf_strptr() failed: %s.", elf_errmsg(-1));
			}
			if (strcmp(name, ".got") == 0) {
				PM_MEMCPY((void *) &module_dsr->GOT_shdr, &shdr, sizeof(GElf_Shdr));
				result = M_R_SUCCESS;
				goto out;
			}
		}
		assert(0 && "PANIC: No GOT section\n");
	}
err_end:
	(void) elf_end(e);
	(void) close(fd);
out:	
	return result;
}


static 
void
persistent_shdr_close(module_dsr_t *module_dsr)
{
	(void) elf_end(module_dsr->elf);
	(void) close(module_dsr->fd);
}


/*!
 * Retrieve the list of loaded modules (e.g. libraries) that need to have their
 * persistent data section mapped into this process.
 *
 * \param module_dsr_listp should point to the head of the list. This structure will
 *  be written with the list of modules which should be loaded in this address space.
 */
m_result_t 
m_module_create_module_dsr_list(struct list_head *module_dsr_listp)
{
	char      fname[64]; /* /proc/<pid>/exe */
	pid_t     pid;
	FILE      *f;
	char      buf[128+M_MODULE_PATH_MAX];
	char      perm[5], dev[6], mapname[M_MODULE_PATH_MAX];
	uint64_t  start, end, inode, foo;
	char      prev_mapname[M_MODULE_PATH_MAX];
	module_dsr_t *module_dsr = NULL;

	
	/* Get our PID and build the name of the link in /proc */
	pid = getpid();
	
	if (snprintf(fname, sizeof(fname), "/proc/%i/maps", pid) < 0) {
		return M_R_FAILURE;
	}
	if ((f = fopen(fname, "r")) == NULL) {
		return M_R_FAILURE;
	}
	INIT_LIST_HEAD(module_dsr_listp);
	
	while (!feof(f)) {
		if(fgets(buf, sizeof(buf), f) == 0)
			break;
		mapname[0] = '\0';
		sscanf(buf, "%lx-%lx %4s %lx %5s %lu %s", &start, &end, perm,
		       &foo, dev, &inode, mapname);
		if (module_dsr == NULL) {	   
			if (!(module_dsr = (module_dsr_t *) malloc(sizeof(module_dsr_t)))) {
				/* FIXME: cleanup allocated memory */
				return M_R_FAILURE;
			}
		}
		if (strlen(mapname) > 0 && strcmp(prev_mapname, mapname) != 0) {
			if (persistent_shdr_open(mapname, inode, start, module_dsr)
		    	== M_R_SUCCESS)
			{
				list_add(&module_dsr->list, module_dsr_listp);
				module_dsr = NULL; /* to allocate a new module_dsr object */
				PM_STRCPY(prev_mapname, mapname);
			}
		}
	}

	/* Free the last module_dsr allocated object if it has not been used */
	if (module_dsr) {
		free(module_dsr);
	}
	
	return M_R_SUCCESS;
}
