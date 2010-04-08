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
