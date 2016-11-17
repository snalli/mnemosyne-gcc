#ifndef _HELPER_H_ASA121
#define _HELPER_H_ASA121

TM_ATTR void* txc_libc_memset (void*, int, const int);
TM_ATTR void* txc_libc_memcpy (char*, const char*, const int);
TM_ATTR int txc_libc_memcmp (const void*, const void*, const int);
TM_ATTR char *txc_libc_strcpy (char*, const char*);
TM_ATTR char *txc_libc_strncpy (char*, const char*, const int);
TM_ATTR int txc_libc_strcmp (const char*, const char*);
TM_ATTR int txc_libc_strncmp (const char*, const char*, const int);

#endif
