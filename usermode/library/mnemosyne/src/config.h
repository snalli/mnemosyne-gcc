#ifndef _CONFIG_H
#define _CONFIG_H

#include "config_generic.h"


#define FOREACH_RUNTIME_CONFIG_SETTING(ACTION, group, config, values)          \
  ACTION(config, values, group, reset_segments, bool, int, 0,                  \
         CONFIG_NO_CHECK, 0)                                                   \
  ACTION(config, values, group, segments_dir, string, char *, "/tmp/segments", \
         CONFIG_NO_CHECK, 0)                                                   \
  ACTION(config, values, group, stats, bool, int, 0, CONFIG_NO_CHECK, 0)       \
  ACTION(config, values, group, stats_file, string, char *, "mcore.stats",     \
         CONFIG_NO_CHECK, 0)


typedef CONFIG_GROUP_STRUCT(mcore) mcore_config_t;

extern mcore_config_t mcore_runtime_settings;

void mcore_config_init();

#endif
