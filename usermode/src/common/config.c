/**
 * \file config.c
 *
 * \brief Runtime configuration.
 *
 * It parses a configuration file (mnemosyne.ini) and sets runtime 
 * parameters.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <common/result.h>
#include <common/config.h>
#include <common/debug.h>


#define CONFIG_OPTION_DEFVALUE(name,                                        \
                               typename,                                    \
                               store_type,                                  \
                               parse_type,                                  \
                               defvalue,                                    \
                               validvalues,	                                \
                               num_validvalues)                             \
  defvalue,


#define DEFVALUES                                                           \
{                                                                           \
  FOREACH_RUNTIME_CONFIG_OPTION(CONFIG_OPTION_DEFVALUE)                     \
}


#define CONFIG_OPTION_VALID_VALUES_ENTRY(name,                              \
                                         typename,                          \
                                         store_type,                        \
                                         parse_type,                        \
                                         defvalue,                          \
                                         validvalues,                       \
                                         num_validvalues)                   \
  parse_type name[num_validvalues]; 


#define CONFIG_OPTION_VALID_VALUES(name,                                    \
                                   typename,                                \
                                   store_type,                              \
                                   parse_type,                              \
                                   defvalue,                                \
                                   validvalues,                             \
                                   num_validvalues)                         \
  validvalues,


#define VALIDVALUES                                                         \
{                                                                           \
  FOREACH_RUNTIME_CONFIG_OPTION(CONFIG_OPTION_VALID_VALUES)                 \
}


#define CONFIG_OPTION_KEY(name,                                             \
                          typename,                                         \
                          store_type,                                       \
                          parse_type,                                       \
                          defvalue,                                         \
                          validvalues,                                      \
                          num_validvalues)                                  \
{                                                                           \
  #name,                                                                    \
  mnemosyne_config_##typename##_data,                                             \
  &(mnemosyne_runtime_settings.name),                                             \
  &(valid_values.name),                                                     \
  num_validvalues                                                           \
},


mnemosyne_runtime_settings_t mnemosyne_runtime_settings = DEFVALUES;
static char *mnemosyne_init_filename = "mnemosyne.ini";


typedef enum {
	mnemosyne_config_integer_data, 
	mnemosyne_config_string_data,	
	mnemosyne_config_boolean_data	
} option_valuetype_t;

typedef enum {
	parse_result_pair,      /* Found (option, value) pair */ 
	parse_result_EOF,       /* End of file */ 
	parse_result_error,     /* Parsing error */
	parse_result_comment    /* Comment */
} parse_result_t;


typedef struct option_s option_t;

struct option_s {
	char               *name;
	option_valuetype_t type;
	void               *value_ptr;
	void               *validvalues_ptr;
	int                num_validvalues;
};

static struct {
	FOREACH_RUNTIME_CONFIG_OPTION(CONFIG_OPTION_VALID_VALUES_ENTRY)
} valid_values = VALIDVALUES;  

static void 
strtrim_whitespace(char **str)
{
	char *p, *end;
	int len;

	p = *str;
	
	len = strlen(p);
	while ( *p && len) {
		end = p + len-1;
		if( ' ' == *end) {
			*end = 0;
		} else if ( '\t' == *end) {
			*end = 0;
		} else {
			break;
		}
		len = strlen( p);
	}

	while ( *p && *p != 0 && len) {
		if( ' ' == *p) {
			*p = 0;
		} else if ( '\t' == *p) {
			*p = 0;
		} else {
			break;
		}
		p++;
		len = strlen( p);
	}
	*str = p;

	return;
}

static 
parse_result_t
parse(FILE *fin, char *option, char *value) 
{
	char buf1[1024];
	char buf2[1024];
	char *subtoken, *ret, *trimmed;

	ret = fgets(buf1, 1024, fin);
	if (ret == NULL) {
		return parse_result_EOF;	
	}
	subtoken = strtok(buf1, "=");
	if (subtoken == NULL) {
		return parse_result_error;
	}
	strcpy(buf2, subtoken);
	trimmed = buf2;
	strtrim_whitespace(&trimmed);
	if (trimmed[0] == '#') {
		return parse_result_comment;
	}
	strcpy(option, trimmed);

	subtoken = strtok(NULL, "\n");
	if (subtoken == NULL) {
		return parse_result_error;
	}
	strcpy(trimmed, subtoken);
	strtrim_whitespace(&trimmed);
	strcpy(value, trimmed);

	return parse_result_pair;
}  


static void 
config_print()
{
	int i;

	option_t options[] = {
		FOREACH_RUNTIME_CONFIG_OPTION(CONFIG_OPTION_KEY)
	};

	fprintf(MNEMOSYNE_DEBUG_OUT, "Configuration parameters\n");
	fprintf(MNEMOSYNE_DEBUG_OUT, "========================\n");
	for (i=0; i < sizeof (options) / sizeof(option_t); i++) {
		fprintf(MNEMOSYNE_DEBUG_OUT, "%s=", options[i].name);
		switch(options[i].type) {
			case mnemosyne_config_integer_data:
				fprintf(MNEMOSYNE_DEBUG_OUT, "%d\n", *((int *) options[i].value_ptr));
				break;
			case mnemosyne_config_string_data:
				fprintf(MNEMOSYNE_DEBUG_OUT, "%s\n", *((char **) options[i].value_ptr));
				break;
			case mnemosyne_config_boolean_data:
				fprintf(MNEMOSYNE_DEBUG_OUT, "%s\n", 
				        *((bool *) options[i].value_ptr) == true ?
				        "enable": "disable");
				break;
			default:
				MNEMOSYNE_INTERNALERROR("Not supported configuration parameter type");
		} 
	}
}


static 
void 
option_set(option_t *option, char *value)
{
	int j, value_is_valid;
	int int_value;

	switch (option->type) {
		case mnemosyne_config_integer_data: 
			if (option->num_validvalues != 2) {
				MNEMOSYNE_INTERNALERROR("config.h has wrong range definition");
			}
			int_value = atoi(value);
			value_is_valid = 1;
			if ((int_value < ((int *) option->validvalues_ptr)[0])
			    || (int_value > ((int *) option->validvalues_ptr)[1])) 
			{
			    value_is_valid = 0;
				MNEMOSYNE_WARNING("Value given for parameter '%s' is out of range. Using default value.", 
				            option->name); 	
			}
			if (value_is_valid) {
				*((int *) option->value_ptr) = int_value;
			}
			break;
		case mnemosyne_config_string_data: 
			if (option->num_validvalues > 0) {
				value_is_valid = 0;
				for (j=0; j < option->num_validvalues; j++) {
					if (strcmp(value, ((char **) option->validvalues_ptr)[j]) == 0) {
						value_is_valid = 1; 
					}
				} 
			} else {
				value_is_valid = 1;
			}
			if (value_is_valid == 0) {
				MNEMOSYNE_WARNING("Value given for parameter '%s' is not recognized. Using default value.", 
				            option->name); 	
			} else {
				char *bufp;
				bufp = (char *) malloc(strlen(value)+1);
				strcpy(bufp, value);
				*( (char **) option->value_ptr) = bufp;
			}
			break;
		case mnemosyne_config_boolean_data: 
			if (option->num_validvalues > 0) {
				value_is_valid = 0;
				for (j=0; j < option->num_validvalues; j++) {
					if (strcmp(value, ((char **) option->validvalues_ptr)[j]) == 0) {
						value_is_valid = 1; 
					}
				} 
			} else {
				value_is_valid = 1;
			}
			if (value_is_valid == 0) {
				MNEMOSYNE_WARNING("Value given for parameter '%s' is not recognized. Using default value.", 
				            option->name); 	
			} else {
				if (strcmp(value, "enable") == 0) {
					*((bool *) option->value_ptr) = true;
				} else {
					*((bool *) option->value_ptr) = false;
				}
			}
			break;
		default:
			MNEMOSYNE_INTERNALERROR("Not supported configuration parameter type");
	}	
}


static 
mnemosyne_result_t
config_init_use_env()
{
	int i, j;
	char buf_option[128];
	char *env_value;
	int len;

	option_t options[] = {
		FOREACH_RUNTIME_CONFIG_OPTION(CONFIG_OPTION_KEY)
	};

	strcpy(buf_option, "MNEMOSYNE_");

	for (i=0; i < sizeof (options) / sizeof(option_t); i++) {
		len = strlen(options[i].name);
		for (j=0; j < len; j++) { 
			buf_option[4+j] = (char) toupper(options[i].name[j]);
		}
		buf_option[4+len] = '\0';
		if ((env_value = getenv(buf_option)) != NULL) {
			option_set(&(options[i]), env_value);
		}
	}

	return MNEMOSYNE_R_SUCCESS;
}


static 
mnemosyne_result_t
config_init_use_file(char *filename)
{
	int i;
	int known_parameter;
	FILE *fin;
	char buf_option[128];
	char buf_value[128];
	int done;

	option_t options[] = {
		FOREACH_RUNTIME_CONFIG_OPTION(CONFIG_OPTION_KEY)
	};


	if ((fin = fopen(filename, "r")) == NULL) {
		return MNEMOSYNE_R_INVALIDFILE;
	} 
	done = 0;

	/* Parse each line of the configuration file */
	while (!done) {
		switch(parse(fin, buf_option, buf_value)) {
			case parse_result_EOF:
				done = 1;
				continue;
			case parse_result_error: 
				MNEMOSYNE_ERROR("Error while parsing file %s.", mnemosyne_init_filename);
				MNEMOSYNE_ASSERT(0); /* never returns */
			case parse_result_comment:
				continue;
		}
		known_parameter = 0;
		for (i=0; i < sizeof (options) / sizeof(option_t); i++) {
			if (strcmp(options[i].name, buf_option) == 0) {
				option_set(&options[i], buf_value); 
				known_parameter = 1;
				break;
			}  
		}		
		if (known_parameter == 0) {
			MNEMOSYNE_WARNING("Ignoring unknown parameter '%s'.", buf_option);
		} 
	}

	return MNEMOSYNE_R_SUCCESS;
}	

mnemosyne_result_t
mnemosyne_config_init ()
{
	config_init_use_file(mnemosyne_init_filename);
	config_init_use_env();

	if (mnemosyne_runtime_settings.printconf == true) {
		config_print();
	}	
		
	return MNEMOSYNE_R_SUCCESS; 	
}


mnemosyne_result_t 
mnemosyne_config_set_option(char *option, char *value)
{
	int i;
	int known_parameter;

	option_t options[] = {
		FOREACH_RUNTIME_CONFIG_OPTION(CONFIG_OPTION_KEY)
	};

	known_parameter = 0;
	for (i=0; i < sizeof (options) / sizeof(option_t); i++) {
		if (strcmp(options[i].name, option) == 0) {
			option_set(&options[i], value); 
			known_parameter = 1;
			break;
		}  
	}		
	if (known_parameter == 0) {
		MNEMOSYNE_WARNING("Ignoring unknown parameter '%s'.", option);
	} 

	return MNEMOSYNE_R_SUCCESS;
}
