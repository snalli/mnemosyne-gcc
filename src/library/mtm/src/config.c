/*
    Copyright (C) 2011 Computer Sciences Department, 
    University of Wisconsin -- Madison

    ----------------------------------------------------------------------

    This file is part of Mnemosyne: Lightweight Persistent Memory, 
    originally developed at the University of Wisconsin -- Madison.

    Mnemosyne was originally developed primarily by Haris Volos
    with contributions from Andres Jaan Tack.

    ----------------------------------------------------------------------

    Mnemosyne is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, version 2
    of the License.
 
    Mnemosyne is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA.

### END HEADER ###
*/

#include <stdio.h>
#include <stdlib.h>
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
	char buf[128];
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
