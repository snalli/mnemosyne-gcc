/**
 * \file config.h
 *
 * \brief Runtime configuration.
 *
 * Defines the runtime parameters.
 */

#ifndef _MNEMOSYNE_CONFIG_H
#define _MNEMOSYNE_CONFIG_H

#include <result.h>
#include <stdbool.h>

/*
 ****************************************************************************
 ***                     RUNTIME DYNAMIC CONFIGURATION                    ***
 ****************************************************************************
 */


#define VALIDVAL0	{} 
#define VALIDVAL2(val1, val2)	{val1, val2} 

#define FOREACH_RUNTIME_CONFIG_OPTION(ACTION)                                \
  ACTION(debug_all, boolean, bool, char *, false,                            \
         VALIDVAL2("enable", "disable"), 2)                                  \
  ACTION(debug_module, boolean, bool, char *, false,                         \
         VALIDVAL2("enable", "disable"), 2)                                  \
  ACTION(printconf, boolean, bool, char *, false,                            \
         VALIDVAL2("enable", "disable"), 2)                                  \
  ACTION(statistics, boolean, bool, char *, true,                            \
         VALIDVAL2("enable", "disable"), 2)                                  \
  ACTION(statistics_file, string, char *, char *, "mnemosyne.stats",         \
         VALIDVAL0, 0)                                                       \


#define CONFIG_OPTION_ENTRY(name,                                            \
                            type_name,                                       \
                            store_type,                                      \
                            parse_type,                                      \
                            defvalue,                                        \
                            validvalues,                                     \
                            num_validvalues)                                 \
  store_type name;

typedef struct mnemosyne_runtime_settings_s mnemosyne_runtime_settings_t;

struct mnemosyne_runtime_settings_s {
	FOREACH_RUNTIME_CONFIG_OPTION(CONFIG_OPTION_ENTRY)  
};
 
extern mnemosyne_runtime_settings_t mnemosyne_runtime_settings;


/* Interface functions */
mnemosyne_result_t mnemosyne_config_init ();
mnemosyne_result_t mnemosyne_config_set_option(char *option, char *value);



/*
 ****************************************************************************
 ***                    BUILD TIME STATIC CONFIGURATION                   ***
 ****************************************************************************
 */

/* Maximum number of threads/descriptors. */
#define MNEMOSYNE_MAX_NUM_THREADS 32

/* Initial sizes of the per descriptor commit and undo action lists. */
#define MNEMOSYNE_ACTION_LIST_SIZE 32

/* Size of the per thread stat hash table */
#define MNEMOSYNE_STATS_THREADSTAT_HASHTABLE_SIZE 512

/* Maximum length of pathname */
#define MNEMOSYNE_MAX_LEN_PATHNAME                128


#endif /* _MNEMOSYNE_CONFIG_H */
