#ifndef _MODE_H
#define _MODE_H


# define FOREACH_MODE(ACTION)   \
    ACTION(wbetl)           



typedef enum {
	MTM_MODE_none  = -1,
	MTM_MODE_wbetl = 0,
	MTM_NUM_MODES
} mtm_mode_t;


/* This type is private to the STM implementation.  */
struct mtm_mode_data_s;

typedef struct mtm_mode_data_s mtm_mode_data_t;
#endif
