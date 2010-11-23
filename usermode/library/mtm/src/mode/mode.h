#ifndef _MODE_H
#define _MODE_H


# define FOREACH_MODE(ACTION)   \
    ACTION(pwbnl)               \
    ACTION(pwbetl)               


typedef enum {
	MTM_MODE_none  = -1,
	MTM_MODE_pwbnl = 0,
	MTM_MODE_pwbetl = 1,
	MTM_NUM_MODES
} mtm_mode_t;

/* This type is private to the STM implementation.  */
struct mtm_mode_data_s;

typedef struct mtm_mode_data_s mtm_mode_data_t;

mtm_mode_t mtm_str2mode(char *str);
char *mtm_mode2str(mtm_mode_t mode);

#endif
