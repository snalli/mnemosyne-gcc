#ifndef _M_DEBUG_H
#define _M_DEBUG_H

#include <stdio.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Debugging levels per module */

#define M_DEBUG_MODULEX     m_runtime_settings.debug_moduleX

#define M_DEBUG_OUT      stderr


/* Operations on timevals */
#define timevalclear(tvp)      ((tvp)->tv_sec = (tvp)->tv_usec = 0)
#define timevaladd(vvp, uvp)                                                \
    do {                                                                    \
        (vvp)->tv_sec += (uvp)->tv_sec;                                     \
        (vvp)->tv_usec += (uvp)->tv_usec;                                   \
        if ((vvp)->tv_usec >= 1000000) {                                    \
            (vvp)->tv_sec++;                                                \
            (vvp)->tv_usec -= 1000000;                                      \
        }                                                                   \
    } while (0)

#define timevalsub(vvp, uvp)                                                \
    do {                                                                    \
        (vvp)->tv_sec -= (uvp)->tv_sec;                                     \
        (vvp)->tv_usec -= (uvp)->tv_usec;                                   \
        if ((vvp)->tv_usec < 0) {                                           \
            (vvp)->tv_sec--;                                                \
            (vvp)->tv_usec += 1000000;                                      \
        }                                                                   \
    } while (0)


#define M_INTERNALERROR(msg, ...)                                           \
    m_debug_print(__FILE__, __LINE__, 1, "Internal Error: ",                \
                  msg, ##__VA_ARGS__ )

#define M_ERROR(msg, ...)                                                   \
    m_debug_print(NULL, 0, 1, "Error",	msg, ##__VA_ARGS__ )


#ifdef _M_BUILD_DEBUG

# define M_WARNING(msg, ...)                                                \
    m_debug_print(NULL, 0, 0, "Warning",	msg, ##__VA_ARGS__ )


# define M_DEBUG_PRINT(debug_level, msg, ...)                               \
    m_debug_print_L(debug_level,	msg, ##__VA_ARGS__ )

# define M_ASSERT(condition) assert(condition) 

#else /* !_M_BUILD_DEBUG */

# define M_WARNING(msg, ...)                   ((void) 0)
# define M_DEBUG_PRINT(debug_level, msg, ...)  ((void) 0)
# define M_ASSERT(condition)                   ((void) 0)

#endif /* !_M_BUILD_DEBUG */


/* Interface functions */

void m_debug_print(char *file, int line, int fatal, const char *prefix, const char *strformat, ...); 
void m_debug_print_L(int debug_level, const char *strformat, ...); 
void m_print_trace (void);

#ifdef __cplusplus
}
#endif

#endif /* _M_DEBUG_H */
