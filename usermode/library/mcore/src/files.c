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
