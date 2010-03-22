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
