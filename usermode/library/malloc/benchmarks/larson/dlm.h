
#ifdef __cplusplus
extern "C" {
#endif

/* SVID2/XPG mallinfo structure */

struct mallinfo {
  int arena;    /* total space allocated from system */
  int ordblks;  /* number of non-inuse chunks */
  int smblks;   /* unused -- always zero */
  int hblks;    /* number of mmapped regions */
  int hblkhd;   /* total space in mmapped regions */
  int usmblks;  /* unused -- always zero */
  int fsmblks;  /* unused -- always zero */
  int uordblks; /* total allocated space */
  int fordblks; /* total non-inuse space */
  int keepcost; /* top-most, releasable (via malloc_trim) space */
};	
	
/* Public routines */

void    dl_malloc_init( void );
void*   dl_malloc(size_t, PerfCounters *pfc);
void    dl_free(void*, PerfCounters *pfc);
int     dl_malloc_trim(size_t);
void    dl_malloc_stats();
int     dl_mallopt(int, int);
struct mallinfo dl_mallinfo(void);
void    dl_clear_lock_counts( void ) ;
void    dl_print_lock_counts( void ) ;



#ifdef __cplusplus
};  /* end of extern "C" */
#endif