/* See items.c */
void item_init(void);
/*@null@*/
__attribute__((tm_callable)) item *do_item_alloc(char *key, const size_t nkey, const int flags, const rel_time_t exptime, const int nbytes);
__attribute__((tm_callable)) void item_free(item *it);
bool item_size_ok(const size_t nkey, const int flags, const int nbytes);

__attribute__((tm_callable)) int  do_item_link(item *it);     /** may fail if transgresses limits */
__attribute__((tm_callable)) void do_item_unlink(item *it);
__attribute__((tm_callable)) void do_item_remove(item *it);
__attribute__((tm_callable)) void do_item_update(item *it);   /** update LRU time to current and reposition */
__attribute__((tm_callable)) int  do_item_replace(item *it, item *new_it);

/*@null@*/
__attribute__((tm_callable)) char *do_item_cachedump(const unsigned int slabs_clsid, const unsigned int limit, unsigned int *bytes);
char *do_item_stats(int *bytes);

/*@null@*/
char *do_item_stats_sizes(int *bytes);
__attribute__((tm_callable)) void do_item_flush_expired(void);
item *item_get(const char *key, const size_t nkey);

__attribute__((tm_callable)) item *do_item_get_notedeleted(const char *key, const size_t nkey, bool *delete_locked);
__attribute__((tm_callable)) item *do_item_get_nocheck(const char *key, const size_t nkey);
