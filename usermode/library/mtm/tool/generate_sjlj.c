/* 
 * DO NOT RELEASE THIS
 */

/** \file generate_sjlj.c
 *
 * \brief Generate the appropriate set of setjmp and longjmp functions.
 */

#define GENERATING_SAVES
#include "mtm_i.h"
#include <stdio.h>

#ifndef offsetof
# define offsetof(type, field) ((uintptr_t)&(((type *)0)->field))
#endif
#define InnermostOffset 3

static const char *longjmp_name = "mtm_longjmp";
#define OuterFunctionIndex 2
static const char *entry_names[] = {
    "beginTransaction"
};
static const char *entry_init_name = "begin_transaction_initialize";

#ifndef _BUILD64
# if (defined (__x86_64__))
#  define _BUILD64 1
# else
#  define _BUILD64 0
# endif
#endif


# define FUNCTION_SUFFIX ""
# if (_BUILD64)

#  define PTR_SIZE    8
#  define WORD_LENGTH "q"
#  define R_ACC       "%rax"      /* A scratch register */
#  define R_SCRATCH   "%r10"      /* Another scratch register */
#  define R_SP        "%rsp"
#  define R_ARG1      "%rdi"
#  define R_ARG2      "%rsi"
#  define R_ARG3      "%rdx"
#  define R_RESULT    "%rax"

#  define FOREACH_REGISTER(MACRO)   \
    MACRO ("%rbx", rbx)             \
    MACRO ("%rbp", rbp)             \
    MACRO ("%r12", r12)             \
    MACRO ("%r13", r13)             \
    MACRO ("%r14", r14)             \
    MACRO ("%r15", r15)
# else

#  define PTR_SIZE    4
#  define WORD_LENGTH "l"
#  define R_ACC       "%ecx"
#  define R_SP        "%esp"
#  define R_ARG1      "%eax"
#  define R_ARG2      "%edx"
#  define R_ARG3      "4(%esp)"
#  define R_RESULT    "%eax"

#  define FOREACH_REGISTER(MACRO)   \
    MACRO ("%ebx", ebx)             \
    MACRO ("%ebp", ebp)             \
    MACRO ("%esi", esi)             \
    MACRO ("%edi", edi)
# endif

#  define BUFFER_REG R_ACC

static void generate_setjmp (const char *prefix, const char *entry, int glock)
{
    char * insert = "";
    if (glock)
    {
       insert = "abortable_";
       printf ("\n\t.hidden\t%s%s%s_internal\n", prefix, insert, entry);
    }
    printf ("\n\t.hidden\t%s%s_internal\n", prefix, entry);
    //printf ("\n\t.hidden\t%s%s%s\n", prefix, insert, entry_init_name);
    printf ("\t.type\t%s%s,@function\n", prefix, entry);
    printf ("\t.align\t2,0x90\n"
            "\t.global\t%s%s\n" "\t.hidden\t%s%s\n", prefix, entry, prefix, entry);
    printf ("%s%s:\n", prefix, entry);

    if (glock)
    {
        printf("\tmov" WORD_LENGTH "\t%s, %s\n", R_ARG3, R_ACC);
        printf("\ttest" WORD_LENGTH "\t$%i, %s\n", pr_instrumentedCode, R_ACC);
        printf ("\tjz\t%s%s_internal\t# Irrevocable transaction has no need to save registers\n", prefix, entry);
        printf("\tand" WORD_LENGTH "\t$%i, %s\n", pr_hasNoAbort | pr_hasNoRetry, R_ACC);
        printf("\tcmp" WORD_LENGTH "\t$%i, %s\n", pr_hasNoAbort | pr_hasNoRetry, R_ACC);
        printf ("\tje\t%s%s_internal\t# Irrevocable transaction has no need to save registers\n", prefix, entry);
    }

    printf ("\tmov\t%lu(%s),%s\n", offsetof (mtm_tx_t, tmp_jb_ptr), R_ARG1, BUFFER_REG);

	/* Save the registers. */
# define saveReg(regName, field) \
    printf ("\tmov\t%s,%lu(%s)\n",regName, offsetof(mtm_jmpbuf_t,field), BUFFER_REG);
    FOREACH_REGISTER (saveReg);

    /* And now the few remaining special values. */
# if (_BUILD64)
    printf ("\tlea\t%d(%%rsp),%s\n", PTR_SIZE,R_SCRATCH);
    printf ("\tmov\t%s,%lu(%s)\n", R_SCRATCH,offsetof (mtm_jmpbuf_t, sp), BUFFER_REG);
    printf ("\tmov\t(%%rsp),%s\n",R_SCRATCH);
    printf ("\tmov\t%s,%lu(%s)\n", R_SCRATCH,offsetof (mtm_jmpbuf_t, abendPC), BUFFER_REG);
# else
    printf ("\tlea\t%d(%%esp),%%ebx\n", PTR_SIZE);
    printf ("\tmov\t%%ebx,%d(%s)\n", offsetof (mtm_jmpbuf_t, sp), BUFFER_REG);
    printf ("\tmov\t(%%esp),%%ebx\n");
    printf ("\tmov\t%%ebx,%d(%s)\n", offsetof (mtm_jmpbuf_t, abendPC), BUFFER_REG);
    printf ("\tmov\t%d(%s),%%ebx\n", offsetof (mtm_jmpbuf_t, ebx), BUFFER_REG);
# endif

    /* SSE */
    printf ("\tstmxcsr %lu(%s)\n", offsetof (mtm_jmpbuf_t, mxcsr), BUFFER_REG);
    printf ("\tfnstcw  %lu(%s)\n", offsetof (mtm_jmpbuf_t, fpcsr), BUFFER_REG);
    printf ("\tfclex\n");

    //printf ("\tjmp\t%s%s%s\t# Vector to the target routine.\n\n", prefix, insert, entry_init_name);
    printf ("\tjmp\t%s%s%s_internal\t# Vector to the target routine.\n\n", prefix, insert, entry);
}

static void generate_longjmp (const char *name)
{
    printf ("\n\t.type\t%s,@function\n", name);
    printf ("\t.align\t2,0x90\n" "\t.globl\t%s\n" "\t.hidden\t%s\n", name, name);
    printf ("%s:\n", name);
# define restoreReg(regName, field) \
    printf ("\tmov\t%lu(%s),%s\n",offsetof(mtm_jmpbuf_t,field), R_ARG1,regName);
    FOREACH_REGISTER (restoreReg);
    printf ("\tmov\t%lu(%s),%s\n", offsetof (mtm_jmpbuf_t, sp), R_ARG1, R_SP);

    /* SSE */
    printf ("\tldmxcsr\t%lu(%s)\n", offsetof (mtm_jmpbuf_t, mxcsr), R_ARG1);
    printf ("\tfnclex\n");
    printf ("\tfldcw\t%lu(%s)\n", offsetof (mtm_jmpbuf_t, fpcsr), R_ARG1);
    printf ("\tcld\n" "\temms\n");

    /* The register shuffling here is somewhat intricate, and not very amenable to parameterization,
     * because we have to worry about the cases where R_ARG1 == R_RESULT and so on.
     */
# if (_BUILD64)
    printf ("\tmov\t%s,%s\n", R_ARG2, R_RESULT);
    printf ("\tmov\t%lu(%s),%s\n", offsetof (mtm_jmpbuf_t, abendPC), R_ARG1, R_ARG2);
    printf ("\tjmp\t*%s\n", R_ARG2);
# else
    printf ("\tmov\t%d(%s),%s\n", offsetof (mtm_jmpbuf_t, abendPC), R_ARG1, R_ACC);
    printf ("\tmov\t%s,%s\n", R_ARG2, R_RESULT);
    printf ("\tjmp\t*%s\n", R_ACC);
# endif
}


static void file_prefix (void)
{
}


static void file_suffix (void)
{
}


int main (int argc, char **argv)
{
#if 0
    if (offsetof (mtm_tx_t, tx) != 3 * sizeof (void *))
    {
        fprintf (stderr, "***tx field has moved in mtm_tx_t, fix generate_sjlj.c\n");
        exit (1);
    }
#endif

    file_prefix ();

    if (argc == 1)
    {
        generate_longjmp (longjmp_name);
    }
    else
    {
        const char *prefix = argv[1];
        int i;
        int glock = 0;

        if (!strcmp(prefix, "glock_"))
        {
           glock = 1;
        }
        for (i = 0; i < sizeof (entry_names) / sizeof (entry_names[0]); i++)
        {
                generate_setjmp (prefix, entry_names[i], glock);
        }
    }

    file_suffix ();
}
