#include <libconfig.h>
#include <stdlib.h>
#include <stdarg.h>
#include "config_generic.h"

#define ENVVAR_MAX_LEN 128

static inline int 
env_setting_lookup(char *group, char *member, char **value_str)
{
	char name[ENVVAR_MAX_LEN];
	char *val;
	int  l;
	int  i;
	int  len;
	int  group_len = strlen(group);
	int  member_len = strlen(member);

	if ((group_len + member_len) > ENVVAR_MAX_LEN-1) {
		return CONFIG_FALSE;
	}
	for (len=0, i=0; i<group_len; len++, i++) {
		name[i] = toupper(group[i]);
	}
	name[len++] = '_';
	for (i=0; i<member_len; len++, i++) {
		name[len] = toupper(member[i]);
	}
	name[len++] = '\0';
	val = getenv(name);
	if (val) {
		*value_str = val;
		return CONFIG_TRUE;
	} else {	
		return CONFIG_FALSE;
	}	
}

static inline int
env_setting_lookup_int(char *group, char *member, int *value)
{
	char *value_str;

	if (env_setting_lookup(group, member, &value_str) == CONFIG_FALSE) {
		return CONFIG_FALSE;
	}

	if (value_str) {
		*value = atoi(value_str);
		return CONFIG_TRUE;
	} else {
		return CONFIG_FALSE;
	}
}

static inline int
env_setting_lookup_bool(char *group, char *member, int *value)
{
	return env_setting_lookup_int(group, member, value);
}


static inline int 
env_setting_lookup_string(char *group, char *member, char *value)
{
	return env_setting_lookup(group, member, value);	
}


int
m_config_setting_lookup_bool(config_t *cfg, 
                             char *group_name, 
                             char *member_name, 
                             int *value, 
                             int validity_check, ...)
{
	config_setting_t *group = config_lookup(cfg, group_name);
	int              ret;
	va_list          ap;
	int              val;
	int              found_val  = 0;

	if (env_setting_lookup_bool(group_name, member_name, &val) == CONFIG_TRUE) {
		found_val = 1;
	} else {	
		group = config_lookup(cfg, group_name);
	    if (group && config_setting_lookup_bool(group, member_name , &val) == CONFIG_TRUE) {
			found_val = 1;
		}
	}

	if (found_val)	{
		*value = val;
		return CONFIG_TRUE;
	}
	return CONFIG_FALSE;
}


int
m_config_setting_lookup_int(config_t *cfg, 
                            char *group_name, 
                            char *member_name, 
                            int *value, 
                            int validity_check, ...)
{
	config_setting_t *group;
	int              ret;
	int              min;
	int              max;
	int              list_length;
	int              i;
	int              val;
	va_list          ap;
	int              found_val  = 0;

	if (env_setting_lookup_int(group_name, member_name, &val) == CONFIG_TRUE) {
		found_val = 1;
	} else {	
		group = config_lookup(cfg, group_name);
	    if (group && config_setting_lookup_int(group, member_name , &val) == CONFIG_TRUE) {
			found_val = 1;
		}
	}

	if (found_val)	{
		switch (validity_check) {
			case CONFIG_NO_CHECK:
				*value = val;
				return CONFIG_TRUE;
			case CONFIG_RANGE_CHECK:
				va_start(ap, 2);
				min = va_arg(ap, int);
				max = va_arg(ap, int);
				va_end(ap);
				if (*value >= min && *value <= max) {
					*value = val;
					return CONFIG_TRUE;
				}
				break;
			case CONFIG_LIST_CHECK:
				va_start(ap, 1);
				list_length = va_arg(ap, int);
				va_end(ap);
				va_start(ap, list_length+1);
				list_length = va_arg(ap, int);
				for (i=0; i<list_length; i++) {
					if (*value == va_arg(ap, int)) {
						*value = val;
						return CONFIG_TRUE;
					}
				}
				va_end(ap);
				break;
		}
	}
	return CONFIG_FALSE;
}


int
m_config_setting_lookup_string(config_t *cfg, 
                               char *group_name, 
                               char *member_name, 
                               char **value, 
                               int validity_check, ...)
{
	config_setting_t *group = config_lookup(cfg, group_name);
	int              ret;
	int              list_length;
	int              i;
	char             *val;
	va_list          ap;
	int              found_val = 0;

	if (env_setting_lookup_string(group_name, member_name, &val) == CONFIG_TRUE) {
		found_val = 1;
	} else {	
		group = config_lookup(cfg, group_name);
	    if (group && config_setting_lookup_string(group, member_name , &val) == CONFIG_TRUE) {
			found_val = 1;
		}
	}

	if (found_val == 1)	{
		switch (validity_check) {
			case CONFIG_NO_CHECK:
				*value = val;
				return CONFIG_TRUE;
			case CONFIG_RANGE_CHECK:
				break;
			case CONFIG_LIST_CHECK:
				va_start(ap, 1);
				list_length = va_arg(ap, int);
				va_end(ap);
				va_start(ap, list_length+1);
				list_length = va_arg(ap, int);
				for (i=0; i<list_length; i++) {
					if (strcmp(*value, va_arg(ap, char *))==0) {
						*value = val;
						return CONFIG_TRUE;
					}
				}
				va_end(ap);
				break;
		}
	}
	return CONFIG_FALSE;
}
