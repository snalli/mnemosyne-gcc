#ifndef _MNEMOSYNE_DEBUG_H
#define _MNEMOSYNE_DEBUG_H

#include <stdio.h>
#include <assert.h>

/* Debugging levels per module */

#define MNEMOSYNE_DEBUG_MODULEX     mnemosyne_runtime_settings.debug_moduleX

#define MNEMOSYNE_DEBUG_OUT      stderr


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


#define MNEMOSYNE_INTERNALERROR(msg, ...)                                         \
    mnemosyne_debug_printmsg(__FILE__, __LINE__, 1, "Internal Error: ",           \
                       msg, ##__VA_ARGS__ )

#define MNEMOSYNE_ERROR(msg, ...)                                                 \
    mnemosyne_debug_printmsg(NULL, 0, 1, "Error",	msg, ##__VA_ARGS__ )


#ifdef _MNEMOSYNE_BUILD_DEBUG

# define MNEMOSYNE_WARNING(msg, ...)                                              \
    mnemosyne_debug_printmsg(NULL, 0, 0, "Warning",	msg, ##__VA_ARGS__ )


# define MNEMOSYNE_DEBUG_PRINT(debug_level, msg, ...)                             \
    mnemosyne_debug_printmsg_L(debug_level,	msg, ##__VA_ARGS__ )

# define MNEMOSYNE_ASSERT(condition) assert(condition) 

#else /* !_MNEMOSYNE_BUILD_DEBUG */

# define MNEMOSYNE_WARNING(msg, ...)                   ((void) 0)
# define MNEMOSYNE_DEBUG_PRINT(debug_level, msg, ...)  ((void) 0)
# define MNEMOSYNE_ASSERT(condition)                   ((void) 0)

#endif /* !_MNEMOSYNE_BUILD_DEBUG */


/* Interface functions */

void mnemosyne_debug_printmsg(char *file, int line, int fatal, const char *prefix, const char *strformat, ...); 
void mnemosyne_debug_printmsg_L(int debug_level, const char *strformat, ...); 

#endif /* _MNEMOSYNE_DEBUG_H */
