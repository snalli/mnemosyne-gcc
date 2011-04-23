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
