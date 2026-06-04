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

#ifndef _M_RESULT_H
#define _M_RESULT_H

#define M_R_SUCCESS        0    /*%< success */
#define M_R_FAILURE	       1    /*%< generic failure */
#define M_R_INVALIDFILE    2    /*%< invalid file */
#define M_R_INVALIDARG     3    /*%< invalid arg */
#define M_R_NOMEMORY       4    /*%< no memory */
#define M_R_EXISTS         5    /*%< item exists */
#define M_R_NOTEXISTS      6    /*%< item does not exist */
#define M_R_NULLITER       7    /*%< null iterator */

typedef int mnemosyne_result_t;
typedef int m_result_t;

#endif /* _M_RESULT_H */
