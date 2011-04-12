#include <stdlib.h>
#include <stdio.h>
#include "config.h"

mcore_config_t mcore_runtime_settings;
config_t       mcore_cfg;


static void
config_init_internal(char *config_file)
{
	config_init(&mcore_cfg);
	config_read_file(&mcore_cfg, config_file);
	FOREACH_RUNTIME_CONFIG_SETTING(CONFIG_SETTING_LOOKUP, mcore, &mcore_cfg, &mcore_runtime_settings);
}


void
mcore_config_init()
{
	char *config_file;
	config_file = getenv("MNEMOSYNE_CONFIG");
	if (config_file) {
		config_init_internal(config_file);
	} else {
		config_init_internal("mnemosyne.ini");
	}
}

void
mcore_config_fini()
{

	config_destroy(&mcore_cfg);
}

void
mcore_config_print(FILE *stream) 
{
	CONFIG_GROUP_PRINT(stream, mcore, &mcore_runtime_settings);
}
