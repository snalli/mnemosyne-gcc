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

/* 
 * \file scmmap-userkernel.h
 *
 * \brief Common definitions used by both the user and kernel module 
 */

#ifndef _SCMMAP_USERKERNEL_H
#define _SCMMAP_USERKERNEL_H

/**
 * Each physical storage class memory (SCM) page in the system has a 
 * struct scmpage associated with it. Information kept in scmpage must
 * be persistent, so those structs are kept in SCM at some known locations. 
 */
struct scmpage_mapping {
	/* NON-VOLATILE fields */
	unsigned long dev;      /* Device identifier */
	unsigned long ino;      /* File inode number */
	unsigned int  ino_gen;  /* Inode's generation number */
	unsigned long offset;	/* Our offset within mapping */
	unsigned long pfn;      /* Physical frame number */

	/* VOLATILE fields -- these are used for emulating SCM and 
	 * wouldn't be necessary if we had real SCM. 
	 */
	void          *page;	 
};

struct scmmap_control_msg {
	char          cmd[32];
	unsigned int  payload_size;
	void          *payload_buf;
};

#define	SCMMAP_CMD_RECONSTRUCT_PAGES "rcp"

#endif
