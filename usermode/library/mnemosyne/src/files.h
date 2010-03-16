/*!
 * \file
 * Collects the interfaces which allow the search for and creation of special files
 * for segment management.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef FILES_H_SSO60IIC
#define FILES_H_SSO60IIC

#include "mnemosyne_i.h"
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
 * \return MNEMOSYNE_R_SUCCESS if the routine found a slash and set file. MNEMOSYNE_R_FAILURE
 *  if there was no slash in path; file was not set.
 */
mnemosyne_result_t path2file(char *path, char **file);

#endif /* end of include guard: FILES_H_SSO60IIC */
