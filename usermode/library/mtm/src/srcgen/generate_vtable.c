/* 
 * DO NOT RELEASE THIS
 */

/** 
 * \file generate_vtable.c
 *
 * \brief Generate an assembly file which contains the function call stubs.
 *
 * Using the same vtable indices as are used in the C structure declaration.
 *
 */

#include <stdio.h>
#include <string.h>
#include "mode/vtable_macros.h"

#ifndef _BUILD64
# if (defined (__x86_64__))
#  define _BUILD64 1
# else
#  define _BUILD64 0
# endif
#endif

# if (_BUILD64)
#  define R_ACC "%rax"
#  define ARG1  "%rdi"
#  define ARG2  "%rsi"
#  define ExtendedTypes "E"
# else
#  define R_ACC "%ecx"
#  define ARG1  "%eax"
#  define ARG2  "%edx"
#  define ExtendedTypes "DE"
# endif


static void header (void)
{ 
# if (_BUILD64)
    printf ("\t.equ PTR_SIZE,8\n");
# else
    printf ("\t.equ PTR_SIZE,4\n");
# endif
    printf ("\n"                /*  */
            "\t.equ VTABLE_OFFSET, 2*PTR_SIZE\n");
}


static void trailer (void)
{
}


/* Compute the number of bytes used by the arguments, which we need
 * for the fastcall name mangling. 
 * Not fully general, but good enough for what we need.
 */
static int arg_bytes (const char *args)
{
    /* Count up the number of arguments, deduced from the number of separating commas. 
     * We know we don't have any void functions, since only functions which take txndesc_t *
     * as a first argument are ever put in the vtable.
     */
    int argcount = 1;
    int extra_stack = 0;
    const char *last_arg = args + 1;
    const char *p = args;

    do
    {
        if ((*p) == ',')
        {
            argcount += 1;
            last_arg = p + 1;
        }
    }
    while (*p++);

    if (!strstr (last_arg, "*"))
    {
        argcount--;             /* We're going to account for this one here. */

        if (strstr (last_arg, "double"))
            extra_stack = strstr (last_arg, "long") ? 16 : 8;
        else if (strstr (last_arg, "m128"))
            extra_stack += 16;
        else if (strstr (last_arg, "m64"))
            extra_stack += 8;
        else if (strstr (last_arg, "uint64_t"))
            extra_stack += 8;
        else
            extra_stack += 4;

        if (strstr (last_arg, "_Complex"))
            extra_stack *= 2;
    }

    return argcount * 4 + extra_stack;
}


static const char *generate_name (const char *src, const char *args, const char *function_prefix)
{
    static char buffer[128];    
    snprintf (buffer, sizeof (buffer), "%s%s", function_prefix, src);
    return &buffer[0];
}


static void generate_stub (unsigned int offset, const char *function, const char *args,
                           int idx, const char *function_prefix)
{
    const char *fn = generate_name (function, args, function_prefix);

    printf ("\n"                /*  */
            "\t.type   %s,@function\n"  /*  */
            "\t.align  2,0x90\n"        /*  */
            "\t.globl  %s\n"    /*  */
            "%s:\n", fn, fn, fn);

    /* N.B. The 32 bit Linux calling convention for functions returning large structs
     * (such as double _Complex) passes a pointer to the space for the returned value
     * as the first argument. The transaction descriptor is therefore the SECOND
     * argument for such functions.
     */

    /*
     * However the 64 bit Linux convention returns these by value in %st0, %st1.
     * (According to http://www.x86-64.org/documentation/abi.pdf). Unfortunately the
     * Intel compiler at least up to 10.1.012 got this wrong and treated them as
     * large structs passing the additional argument. Since we're only expecting to
     * be called by the STM compiler, which has now been fixed, we adopt the correct
     * (published ABI) calling convention, and do not expect an extra argument in 64
     * bit mode.
     *
     * ***EXCEPT*** the STM compiler (and intel 10.0 compilers in general) have a bug
     * whereby they mis-generate the code for long double _Complex and treat it as a
     * struct, rather than using the FP stack. So, for now we continue to generate
     * the "wrong" code.
     */
//#  define NON_ABI_COMPLIANT_COMPILER 1	/* Until the compiler is fixed. */
#  define NON_ABI_COMPLIANT_COMPILER 0          /* FIXME : Intel 11.0 is a conforming compiler? */


#  if ((!_BUILD64) || (_BUILD64 && NON_ABI_COMPLIANT_COMPILER))
    /* We just look for these by name; there's no need to be too subtle here. */
    if (strstr (function, "RC") == function || strstr (function, "RaRC") == function ||
        strstr (function, "RaWC") == function || strstr (function, "RfWC") == function)
    {
        char type = function[strlen (function) - 1];

        if (strchr (ExtendedTypes, type) != 0)
        {
            printf ("\tmov     VTABLE_OFFSET(%s),%s\n"  /*  */
                    "\tjmp     * %d*PTR_SIZE(%s)\n", ARG2, R_ACC, idx, R_ACC);
            return;
        }
    }
#  endif

    printf ("\tmov     VTABLE_OFFSET(%s),%s\n"  /*  */
            "\tjmp     * %d*PTR_SIZE(%s)\n", ARG1, R_ACC, idx, R_ACC);
}


int main (int argc, char **argv)
{
    int          i = 0;
    unsigned int offset = 0;

    header ();
#define GENERATE_FUNCTION(result,function,args,ARG)        \
    generate_stub (offset, #function, #args, i++, ARG);

    FOREACH_VTABLE_ENTRY (GENERATE_FUNCTION, "_ITM_");
    trailer ();
}
