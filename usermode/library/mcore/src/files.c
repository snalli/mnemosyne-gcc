/*!
 * \file
 * Implements the utilities in files.h.
 * 
 * \author Andres Jaan Tack
 */
#include "files.h"
#include <string.h>
#include <sys/stat.h>


void 
mkdir_r(const char *dir, mode_t mode) 
{
	char   tmp[256];
	char   *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", dir);
	len = strlen(tmp);
	if(tmp[len - 1] == '/') {
		tmp[len - 1] = 0;
		for(p = tmp + 1; *p; p++)
			if(*p == '/') {
				*p = 0;
				mkdir(tmp, mode);
				*p = '/';
		}
	}	
	mkdir(tmp, mode);
}


m_result_t
path2file(char *path, char **file)
{
	int i;

	for (i=strlen(path); i>=0; i--) {
		if (path[i] == '/') {
			*file = &path[i+1];
			return M_R_SUCCESS;
		}
	}
	return M_R_FAILURE;
}
