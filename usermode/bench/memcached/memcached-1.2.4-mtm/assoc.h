/* associative array */
void assoc_init(void);
__attribute__((tm_callable)) item *assoc_find(const char *key, const size_t nkey);
__attribute__((tm_callable)) int assoc_insert(item *item);
__attribute__((tm_callable)) void assoc_delete(const char *key, const size_t nkey);
__attribute__((tm_callable)) void do_assoc_move_next_bucket(void);
__attribute__((tm_pure)) uint32_t hash( const void *key, size_t length, const uint32_t initval);
