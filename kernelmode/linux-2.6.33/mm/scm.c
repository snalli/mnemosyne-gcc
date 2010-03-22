#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/slab.h>

#include "scm.h"

static int (*scm_update_mapping_callback)(struct page *page, struct address_space *mapping, pgoff_t offset);

int scm_update_mapping(struct page *page, struct address_space *mapping, pgoff_t offset)
{
	struct inode *inode = mapping->host;

	if (scm_update_mapping_callback) {
		return scm_update_mapping_callback(page, mapping, offset);
	}
	//printk(KERN_INFO "inode = %p, offset = %lu\n", inode, offset);
	return 0;
}

int scm_register_callback(int (*update_mapping_callback)(struct page *page, struct address_space *mapping, pgoff_t offset))
{
	scm_update_mapping_callback = update_mapping_callback;
}

EXPORT_SYMBOL(scm_register_callback);
