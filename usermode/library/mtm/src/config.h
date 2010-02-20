#ifndef _CONFIG_H
#define _CONFIG_H

#include "config_generic.h"


#define FOREACH_RUNTIME_CONFIG_SETTING(ACTION, group, config, values)                        \
  ACTION(config, values, group, stats, bool, int, 0, CONFIG_NO_CHECK, 0)                     \
  ACTION(config, values, group, force_mode, string, char *, "pwb", CONFIG_NO_CHECK, 0)       \
  ACTION(config, values, group, stats_file, string, char *, "mtm.stats", CONFIG_NO_CHECK, 0)


typedef CONFIG_GROUP_STRUCT(mtm) mtm_config_t;

extern mtm_config_t mtm_runtime_settings;

#endif
