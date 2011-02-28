#include "tm.h"
#include "helper.h"

char *
txc_libc_strcpy (char *dest, const char *src)
{
	const char *p;
	char *q;

	TM_ATOMIC 
	{
		for(p = src, q = dest; *p != '\0'; p++, q++)
			*q = *p;

		*q = '\0';
	}

	return dest;
}

	
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


int
txc_libc_memcmp (const void * s1, const void * s2, int n)
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

