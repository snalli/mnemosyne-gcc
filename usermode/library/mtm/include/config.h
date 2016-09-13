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

#ifndef _CONFIG_H
#define _CONFIG_H

#include "config_generic.h"


#define FOREACH_RUNTIME_CONFIG_SETTING(ACTION, group, config, values)                        \
  ACTION(config, values, group, stats, bool, int, 0, CONFIG_NO_CHECK, 0)                     \
  ACTION(config, values, group, force_mode, string, char *, "pwbetl", CONFIG_NO_CHECK, 0)     \
  ACTION(config, values, group, stats_file, string, char *, "mtm.stats", CONFIG_NO_CHECK, 0)


typedef CONFIG_GROUP_STRUCT(mtm) mtm_config_t;

extern mtm_config_t mtm_runtime_settings;

#endif
