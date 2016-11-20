/* See items.c */
void item_init(void);
/*@null@*/
TM_ATTR item *do_item_alloc(char *key, const size_t nkey, const int flags, const rel_time_t exptime, const int nbytes);
TM_ATTR void item_free(item *it);
bool item_size_ok(const size_t nkey, const int flags, const int nbytes);

TM_ATTR int  do_item_link(item *it);     /** may fail if transgresses limits */
TM_ATTR void do_item_unlink(item *it);
TM_ATTR void do_item_remove(item *it);
TM_ATTR void do_item_update(item *it);   /** update LRU time to current and reposition */
TM_ATTR int  do_item_replace(item *it, item *new_it);

/*@null@*/
TM_ATTR char *do_item_cachedump(const unsigned int slabs_clsid, const unsigned int limit, unsigned int *bytes);
TM_ATTR char *do_item_stats(int *bytes);

/*@null@*/
TM_ATTR char *do_item_stats_sizes(int *bytes);
TM_ATTR void do_item_flush_expired(void);
item *item_get(const char *key, const size_t nkey);

TM_ATTR item *do_item_get_notedeleted(const char *key, const size_t nkey, bool *delete_locked);
TM_ATTR item *do_item_get_nocheck(const char *key, const size_t nkey);
TM_ATTR bool item_delete_lock_over (item *it);

