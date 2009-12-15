#ifndef _LOCAL_H
#define _LOCAL_H

# define DECLARE_LOG_BARRIER(name, type, encoding)                             \
void _ITM_CALL_CONVENTION name##L##encoding (mtm_thread_t *, const type *);

FOR_ALL_TYPES(DECLARE_LOG_BARRIER, mtm_local_)
void _ITM_CALL_CONVENTION mtm_local_LB (mtm_thread_t *, const void *, size_t); 

#endif /* _LOCAL_H */
