#ifndef _CONFIG_GENERIC_H
#define _CONFIG_GENERIC_H

#include <stdio.h>
#include <libconfig.h>

#define CONFIG_NO_CHECK    0
#define CONFIG_RANGE_CHECK 1
#define CONFIG_LIST_CHECK  2


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
