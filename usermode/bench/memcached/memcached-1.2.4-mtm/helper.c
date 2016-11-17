#include <memcached.h>

TM_ATTR
void *
txc_libc_memset (void *dest, int i, const int bytes)
{
	char *q, c = i;
	int count;

	{
		for(q = dest, count = 0; count < bytes; ++count)
			*(q+count) = c;
	}

	return dest;
}

TM_ATTR
void *
txc_libc_memcpy (char *dest, const char *src, const int bytes)
{
	const char *p;
	char *q;
	int count;

	{
		for(p = src, q = dest, count = 0; count < bytes; p++, q++, count++)
			*q = *p;
	}

	return dest;
}

TM_ATTR
int
txc_libc_memcmp (const void * s1, const void * s2, const int n)
{
	char *a = (char *) s1;	
	char *b = (char *) s2;	
	char c1;
	char c2;
	int i=0;

  	do
    {
	    int delta = (c1=*a++)-(c2=*b++);
        if (delta)
            return delta;
		i++;
    } while (i < n);

	return 0;
}

TM_ATTR
char *
txc_libc_strncpy (char *dest, const char *src, const int bytes)
{
	const char *p;
	char *q;
	int n;

	{
		for(p = src, q = dest, n = 0; *p != '\0' && n < bytes; p += n, q += n)
		{
			*q = *p;
			++n;
		}

		*q = '\0';
	}

	return dest;
}

TM_ATTR
int
txc_libc_strlen (const char *src)
{
	int count = 0;
	while(*(src+count)) {
		++count;
	}
	return count;
}

TM_ATTR
char *
txc_libc_strcpy (char *dest, const char *src)
{
	const char *p;
	char *q;

	{
		for(p = src, q = dest; *p != '\0'; p++, q++)
			*q = *p;

		*q = '\0';
	}

	return dest;
}
	
TM_ATTR
int
txc_libc_strcmp (const char * a, const char * b)
{
	char c1;
	char c2;

  	do
    {
	    int delta = (c1=*a++)-(c2=*b++);
        if (delta)
            return delta;
    } while (c1 && c2);

	return 0;
}

TM_ATTR
int
txc_libc_strncmp (const char * a, const char * b, const int bytes)
{
	char c1;
	char c2;
	int n = 0;

  	do
    	{
	    int delta = (c1=*a)-(c2=*b);
        	if (delta)
           		return delta;
	    ++n;
	    a += n;
	    b += n;
	    
    	} while ((c1 && c2) && (n < bytes));

	return 0;
}


