/**
 * \file main_kernel.c
 *
 * \brief The main file of the SCM mappings subsystem. 
 *
 * A usermode front-end controls this subsystem through the procfs 
 * interface.
 *
 * The VM subsystem calls us whenever there is a mapping of an SCM page frame
 * on a mmaped persistent region.
 *
 */

#include <linux/module.h> 
#include <linux/kernel.h>

#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/exportfs.h>
#include <linux/pagemap.h>

#include <asm/uaccess.h>

#include "scmmap.h"
#include "scmmap-userkernel.h"
#include "index.h"
#include "hrtime.h"

#define MEASURE_TIME


static struct proc_dir_entry *procfs_dir_scm;
static struct proc_dir_entry *procfs_file_mapping;
static struct proc_dir_entry *procfs_file_control;

static struct scmpage_mapping        null_mapping = { .dev = 0, 
                                                      .ino = 0, 
                                                      .ino_gen = 0, 
                                                      .offset = 0, 
                                                      .pfn = 0, 
                                                      .page = (void *) 0, };

int scm_register_callback(int (*update_mapping_callback)(struct page *page, struct address_space *mapping, pgoff_t offset));


/**
 * \brief Read an SCM mapping.
 *
 * \param file Ignored.
 * \param buffer The user space buffer the mapping will be read into.
 * \param count Ignored.
 * \param off The mapping offset (index).
 */
static ssize_t procfs_read_scmmap_mapping(struct file *file, char *buffer, size_t count, loff_t *off)
{
	int                    len=0;
	struct scmpage_mapping *mapping;
	unsigned long          index = *off;

	if (index >= NUM_PAGES) {
		len = -EFAULT;
		goto out;
	}

	scmmap_index_get_mapping(index, &mapping);
	if (mapping == NULL) {
		/* return the special NULL mapping instead of error*/
		mapping = &null_mapping;
	}
	if (copy_to_user(buffer, mapping, sizeof(struct scmpage_mapping)) > 0) {
		len = -EFAULT;
		goto out;
	}
	len = count;

out:	
	return len;	
}


/**
 * \brief Update an SCM mapping.
 *
 * \param file Ignored.
 * \param buffer The user space buffer the mapping will be read from.
 * \param count Ignored.
 * \param off The mapping offset (index). Ignored if equal to -1.
 */
static ssize_t procfs_write_scmmap_mapping(struct file *file, const char *buffer, size_t count, loff_t *off)
{
	int                    rv;
	struct scmpage_mapping *mapping;
	unsigned long          index = *off;

	if (index == 0xFFFFFFFF) {
		rv = scmmap_index_alloc_mapping(&mapping);
		procfs_file_mapping->size++;
		if (rv < 0) {
			goto out;
		}
	} else {
		rv = scmmap_index_get_mapping(index, &mapping);
		if (rv < 0) {
			goto out;
		}
		if (mapping == NULL) {
			rv = -EFAULT;
			goto out;
		}
	}	
	if (copy_from_user(mapping, buffer, sizeof(struct scmpage_mapping)) > 0) {
		rv = -EFAULT;
		goto out;
	}
	rv = count;
out:	
	return rv;	
}


static struct file_operations procfs_scmmap_mapping = {
	.read    = procfs_read_scmmap_mapping,
	.write   = procfs_write_scmmap_mapping,
};

/** 
 * \file It creates a file handle given an inode number and generation number, 
 */
static int encode_fh(unsigned long i_ino, __u32 i_generation, struct fid *fid)
{
	int type = FILEID_INO32_GEN;

	fid->i32.ino = i_ino;
	fid->i32.gen = i_generation;
	return type;
}


static int load_page_into_cache(struct address_space *mapping, pgoff_t offset)
{
	struct page *page; 
	int         ret;

	do {
		page = page_cache_alloc_cold(mapping);
		if (!page)
			return -ENOMEM;

		ret = add_to_page_cache_lru(page, mapping, offset, GFP_KERNEL);
		if (ret == -EEXIST) {
			ret = 0; /* losing race to add is OK */
		}	
		page_cache_release(page);
		unlock_page(page);

	} while (ret == AOP_TRUNCATED_PAGE);
		
	return ret;
}

/**
 * For each mapping, it creates the VFS inode and address_space object 
 * if not already created and brings the page into the page cache.
 * Later on, on an address space fault the VM subsystem will find the 
 * page in the cache and handle it as a minor page fault.
 *
 */
int reconstruct_pages(void)
{
	int                    error;
	int                    i;
	struct dentry          *de;
	struct super_block     *sb;
	struct fid             fid;
	struct scmpage_mapping *scm_mapping;
	struct address_space   *address_space;
	struct page            *page;
	struct inode           *inode;
	pgoff_t                offset;
	hrtime_t               start_time;
	hrtime_t               stop_time;
	hrtime_t               stop_time2;

	for (i=0; i<scmpage_mapping_index->mappings_max_n; i++) {
		if ((scm_mapping = scmpage_mapping_index->array[i])) {
			start_time = hrtime_cycles();
			sb = user_get_super((dev_t) scm_mapping->dev);
			if (IS_ERR_OR_NULL(sb)) {
				printk(KERN_INFO "[%s] no superblock for device 0x%lx\n", MODULE_NAME, scm_mapping->dev);
				continue;
			}
			encode_fh(scm_mapping->ino, (__u32) scm_mapping->ino_gen, &fid);
			de = sb->s_export_op->fh_to_dentry(sb, &fid, 2, FILEID_INO32_GEN);
			if (IS_ERR_OR_NULL(de)) {
				printk(KERN_INFO "[%s] no dentry for inode number 0x%lx\n", MODULE_NAME, scm_mapping->ino);
				continue;
			}
			inode = de->d_inode;
			address_space = inode->i_mapping;
			offset = scm_mapping->offset;
			scm_mapping->page = NULL;

#if DEBUG_BUILD
			printk(KERN_INFO "RECONSTRUCT: inode        = %p\n", inode);
			printk(KERN_INFO "RECONSTRUCT: inode->i_ino = 0x%lx\n", inode->i_ino);
			printk(KERN_INFO "RECONSTRUCT: inode->i_generation = %u\n", inode->i_generation);
			printk(KERN_INFO "RECONSTRUCT: inode->i_mapping = %p\n", inode->i_mapping);
#endif
			stop_time2 = hrtime_cycles();
retry_find:		
			page = find_lock_page (address_space, offset);
			if (!page) {
				error = load_page_into_cache(address_space, offset);
				if (error >= 0)
					goto retry_find;
				if (error == -ENOMEM) {
					printk(KERN_INFO "RECONSTRUCT: ERROR\n");
				}
			} else {	
				scm_mapping->page = page;
#if DEBUG_BUILD
				printk(KERN_INFO "RECONSTRUCT: page         = %p\n", page);
#endif				
			}	
			stop_time = hrtime_cycles();
#ifdef MEASURE_TIME			
			printk(KERN_INFO "RECONSTRUCT: page read  = %llu\n", HRTIME_CYCLE2NS(stop_time2 - start_time));
			printk(KERN_INFO "RECONSTRUCT: page load latency  = %llu\n", HRTIME_CYCLE2NS(stop_time - stop_time2));
			printk(KERN_INFO "RECONSTRUCT: page total latency  = %llu\n", HRTIME_CYCLE2NS(stop_time - start_time));
#endif
		}
	}

	/* Now read the contents of the pages from disk into memory
	 * This step would be unnecessary if we had real SCM since the 
	 * contents would be already there. 
	 */
	/* FIXME: What if there are multiple mappings which point to a page already loaded in the cache? */
	for (i=0; i<scmpage_mapping_index->mappings_max_n; i++) {
		scm_mapping = scmpage_mapping_index->array[i];
		if (scm_mapping && scm_mapping->page) {
			page = scm_mapping->page;
			address_space = page->mapping;
			ClearPageError(page);
			error = address_space->a_ops->readpage(NULL, page);
			if (!error) {
				wait_on_page_locked(page);
				if (!PageUptodate(page))
					error = -EIO;
			} else {
				/* If readpage failed then page is left locked -- unlock it */
				unlock_page(page);
			}
			page_cache_release(page);
		}
	}

	return 0;
}



int execute_control_command(char *cmd, unsigned int payload_size, void *payload)
{
	int rv = 0;

	if (strcmp(cmd, SCMMAP_CMD_RECONSTRUCT_PAGES) == 0) {
		printk(KERN_INFO "SCMMAP_CMD_RECONSTRUCT_PAGES\n");
		rv = reconstruct_pages();
	}
	return rv;
}


/**
 * \brief Control SCM mapping subsystem.
 *
 * \param file Ignored.
 * \param buffer Contains the control instruction.
 * \param count Ignored.
 * \param off Ignored.
 */
static ssize_t procfs_write_control(struct file *file, const char *buffer, size_t count, loff_t *off)
{
	int                       rv = 0;
	char                      *cmd;
	struct scmmap_control_msg control_msg;
	unsigned int              payload_size;
	void                      *payload_buf;

	if (copy_from_user(&control_msg, 
	                   buffer, 
	                   sizeof(struct scmmap_control_msg)) > 0) 
	{
		rv = -EFAULT;
		goto out;
	}

	cmd = control_msg.cmd;
	if ((payload_size = control_msg.payload_size) > 0 && control_msg.payload_buf) 
	{
		payload_buf = vmalloc(control_msg.payload_size);
		if (payload_buf == NULL) {
			rv = -ENOMEM;
			goto out;
		}
		if (copy_from_user(payload_buf, 
		                   control_msg.payload_buf, 
		                   control_msg.payload_size) > 0) 
		{
			rv = -EFAULT;
			goto err_cpy_payload;
		}
	} else {
		payload_size = 0;
		payload_buf = NULL;
	}
	rv = execute_control_command(cmd, payload_size, payload_buf);
	vfree(payload_buf);
	goto out;

err_cpy_payload:
	vfree(payload_buf);
	return rv;
out:	
	return rv;	
}

static struct file_operations procfs_scmmap_control = {
	.write   = procfs_write_control,
};


static int init_procfs(void) 
{
	int rv = 0;

	procfs_dir_scm = proc_mkdir(MODULE_NAME, NULL);
	if (procfs_dir_scm == NULL) {
		rv = -ENOMEM;
		goto out;
	}

	procfs_file_mapping = create_proc_entry("mapping", 00666, procfs_dir_scm);
	if (procfs_file_mapping == NULL) {
		rv = -ENOMEM;
		goto no_mapping;
	}
	procfs_file_mapping->proc_fops = &procfs_scmmap_mapping;
	procfs_file_mapping->size = 0;

	procfs_file_control = create_proc_entry("control", 00666, procfs_dir_scm);
	if (procfs_file_control == NULL) {
		rv = -ENOMEM;
		goto no_control;
	}
	procfs_file_control->proc_fops = &procfs_scmmap_control;


	printk(KERN_INFO "[%s] initialised procfs\n", MODULE_NAME);
	goto out;

no_control:
	remove_proc_entry("mapping", procfs_dir_scm);
no_mapping:
	remove_proc_entry(MODULE_NAME, NULL);
out:
	return rv;
}


static int cleanup_procfs(void)
{
	remove_proc_entry("mapping", procfs_dir_scm);
	remove_proc_entry("control", procfs_dir_scm);
	remove_proc_entry(MODULE_NAME, NULL);

	return 0;
}

/**
 * \briefs Updates the mapping index.
 *
 * Called by the VM subsystem whenever there is a mapping of a SCM page to
 * a mmaped persistent region.
 */
int scmmap_update_mapping(struct page *page, struct address_space *address_space, pgoff_t offset)
{
	struct inode           *inode;
	struct super_block     *sb;
	struct scmpage_mapping *mapping;
	int                    rv = 0;

	printk(KERN_INFO "scmmap_update_mapping\n");

	/* FIXME: RACE CONDITION: You may race with another instance of this function or with function reconstruct_pages (Bug: 0001) */
	if (!address_space) {
		rv = 0;
		goto out;
	}
	inode = address_space->host;
	if (!inode) {
		rv = 0;
		goto out;
	}
	mapping = (struct scmpage_mapping *) page->scmpage_mapping;
	if (mapping == NULL) {
		rv = scmmap_index_alloc_mapping(&mapping);
		procfs_file_mapping->size++;
		if (rv < 0) {
			rv = 0;
			goto out;
		}
	}

	sb = inode->i_sb;

	/* Updating a mapping should be atomic */
	/* FIXME: Issue a PCM fence/flush (Bug: 0002) */
	/* FIXME: Have a flag to indicate whether the updates were atomic or do the updates 
	in a way you can later find out whether all were done */
	mapping->dev = (unsigned long) sb->s_dev;
	mapping->ino = (unsigned long) inode->i_ino;
	mapping->ino_gen = (unsigned long) inode->i_generation;
	mapping->offset = (unsigned long) offset;
	mapping->pfn = (unsigned long) 0; /* FIXME: what is this? does it apply? */

	/* Volatile fields -- used for emulation */
	mapping->page = (void *) page;

#if DEBUG_BUILD
	printk(KERN_INFO "UPDATE_MAPPING: page = %p\n", page);
	printk(KERN_INFO "UPDATE_MAPPING: page->address_space = %p\n", page->mapping);
	printk(KERN_INFO "UPDATE_MAPPING: dev    = 0x%lx\n", mapping->dev);
	printk(KERN_INFO "UPDATE_MAPPING: ino    = 0x%lx\n", mapping->ino);
	printk(KERN_INFO "UPDATE_MAPPING: ino_gen= %u\n", mapping->ino_gen);
	printk(KERN_INFO "UPDATE_MAPPING: offset = %lu\n", mapping->offset);
	printk(KERN_INFO "UPDATE_MAPPING: pfn = %lu\n", mapping->pfn);
#endif

	rv = 0;
out:
	return rv;
}



int init_module()
{
	int rv = 0;

	rv = scmmap_index_create(NUM_PAGES);
	if (rv < 0) {
		goto out;
	}
	rv = init_procfs();
	if (rv < 0) {
		goto no_procfs;
	}

	scm_register_callback(scmmap_update_mapping);

	printk(KERN_INFO "[%s] initialised\n", MODULE_NAME);
	goto out;

no_procfs:
	scmmap_index_destroy();
out:
	return rv;
}


void cleanup_module()
{
	scmmap_index_destroy();
	cleanup_procfs();

	scm_register_callback(NULL); /* unregister */

	printk(KERN_INFO "[%s] removed\n", MODULE_NAME);
}
