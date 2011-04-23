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

#ifndef _MNEMOSYNE_MODULE_H
#define _MNEMOSYNE_MODULE_H

#include <stdint.h>
#include <gelf.h>
#include "result.h"
#include "list.h"

/*! The maximum path length for a module. */
#define M_MODULE_PATH_MAX 256


/*!
 * \brief Module descriptor (for modules that have a .persistent section)
 *
 * Holds information about the .persistent and .got sections of a loaded module.
 * By term module we refer to the main app static module and to any dynamically 
 * loaded piece of code such as libraries.
 */
typedef struct module_dsr_s module_dsr_t;
struct module_dsr_s {
	char             module_path[M_MODULE_PATH_MAX]; /**< pathname of the module owning this section */
	uint64_t         module_inode;          /**< the inode number of the module file */
	uintptr_t        module_start;          /**< address where the module is loaded */
	int              fd;                    /**< file descriptor of the open elf object */
	Elf              *elf;                  /**< the elf in-mem object that corresponds to this section */
	GElf_Shdr        GOT_shdr;              /**< the .got (global offset table) section header descriptor */
	GElf_Shdr        persistent_shdr;       /**< the .persistent section header descriptor */
    Elf_Scn          *persistent_scn;       /**< the in-mem .persistent section object */
	struct list_head list;                  /**< list connector */
};

extern struct list_head module_dsr_list;

m_result_t m_module_create_module_dsr_list(struct list_head *module_dsr_listp);
void m_module_relocate_symbols(uintptr_t got_start, uintptr_t got_end, uintptr_t old_start, uintptr_t old_end, uintptr_t new_start, uintptr_t new_end);

#endif /* _MNEMOSYNE_MODULE_H */
