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
 * Collects the interfaces which allow the search for and creation of special files
 * for segment management.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef FILES_H_SSO60IIC
#define FILES_H_SSO60IIC

#include "mcore_i.h"
#include "result.h"

/*!
 * Creates a directory with the given path. This will create directories
 * to satisfy the full path if necessary.
 * 
 * e.g. mkdir_r("/a/b/c", S_IRWXU), supposing none of a b or c exists,
 *  will create a, then a/b, then a/b/c.
 * 
 * \param dir is the path of the directory to be created. This may be
 *  relative or absolute.
 * \param mode is a permission setting to fix on the new directory(s). e.g. S_IRWXU
 */
void mkdir_r(const char *dir, mode_t mode);

/*!
 * Given a full path, finds the last component of it (which is the unqualified
 * file name).
 * 
 * \param path is a path to a file, whether relative or not. This must not be NULL.
 * \param file is an output parameter, set to point to the beginning of the filename
 *  component of path. This must not be NULL.
 *
 * \return M_R_SUCCESS if the routine found a slash and set file. M_R_FAILURE
 *  if there was no slash in path; file was not set.
 */
mnemosyne_result_t path2file(char *path, char **file);

#endif /* end of include guard: FILES_H_SSO60IIC */
