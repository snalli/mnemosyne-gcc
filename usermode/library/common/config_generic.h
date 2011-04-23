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

#ifndef _CONFIG_GENERIC_H
#define _CONFIG_GENERIC_H

#include <stdio.h>
#include <libconfig.h>

#define CONFIG_NO_CHECK    0
#define CONFIG_RANGE_CHECK 1
#define CONFIG_LIST_CHECK  2

/* 
 * The lookup functions return the value of a configuration variable based on 
 * the following order: 
 *  1) value of environment variable
 *  2) value in configuration file
 *  3) default value
 */


#define CONFIG_SETTING_LOOKUP(config, values, group, member, type_name, type,  \
                              default_val, validitycheck, args...)             \
  (values)->member = default_val;                                              \
  m_config_setting_lookup_##type_name(config, #group, #member,                 \
                                      &((values)->member),                     \
                                      validitycheck, args);

#define CONFIG_SETTING_ENTRY(config, values, group, member, type_name, type,    \
                             default_val, validitycheck, args...)               \
  type member;

#define CONFIG_GROUP_STRUCT(group)                                             \
  struct {                                                                     \
    FOREACH_RUNTIME_CONFIG_SETTING(CONFIG_SETTING_ENTRY, group, NULL, NULL)    \
  }

static inline void
config_setting_print_bool(FILE * stream, char *group, char *member, int val) {
    fprintf(stream, "%s.%s = %s\n", group, member, val==1 ? "true": "false");
}

static inline void
config_setting_print_int(FILE * stream, char *group, char *member, int val) {
    fprintf(stream, "%s.%s = %d\n", group, member, val);
}

static inline void
config_setting_print_string(FILE * stream, char *group, char *member, char *val) {
    fprintf(stream, "%s.%s = %s\n", group, member, val);
}


#define CONFIG_SETTING_PRINT(config, values, group, member, type_name, type,   \
                             default_val, validitycheck, args...)              \
 config_setting_print_##type_name(__stream, #group, #member, (values)->member);


#define CONFIG_GROUP_PRINT(stream, group, values)                              \
do {                                                                           \
  FILE *__stream = stream;                                                     \
  FOREACH_RUNTIME_CONFIG_SETTING(CONFIG_SETTING_PRINT, group, NULL, values)    \
} while(0);

int m_config_setting_lookup_string(config_t *cfg, char *group_name, char *member_name, char **value, int validity_check, ...);
int m_config_setting_lookup_int(config_t *cfg, char *group_name, char *member_name, int *value, int validity_check, ...);
int m_config_setting_lookup_bool(config_t *cfg, char *group_name, char *member_name, int *value, int validity_check, ...);

#endif 
