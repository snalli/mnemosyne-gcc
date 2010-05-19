#include <stdio.h>
#include "config.h"

mtm_config_t mtm_runtime_settings;
config_t     mtm_cfg;


static void
config_init_internal(char *config_file)
{
	config_init(&mtm_cfg);
	config_read_file(&mtm_cfg, config_file);
	FOREACH_RUNTIME_CONFIG_SETTING(CONFIG_SETTING_LOOKUP, mtm, &mtm_cfg, &mtm_runtime_settings);
}


void
mtm_config_init()
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
mtm_config_fini()
{
	config_destroy(&mtm_cfg);
}


void
mtm_config_print(FILE *stream) 
{
	CONFIG_GROUP_PRINT(stream, mtm, &mtm_runtime_settings);
}
