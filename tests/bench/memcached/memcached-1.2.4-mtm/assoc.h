/* associative array */
void assoc_init(void);
TM_ATTR item *assoc_find(const char *key, const size_t nkey);
TM_ATTR int assoc_insert(item *item);
TM_ATTR void assoc_delete(const char *key, const size_t nkey);
TM_ATTR void do_assoc_move_next_bucket(void);
TM_ATTR uint32_t hash( const void *key, size_t length, const uint32_t initval);
