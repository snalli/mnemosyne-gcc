#include <mnemosyne.h>
#include <pmalloc.h>

struct dummy_s {
	int a;
	int b;
	int c;
	int d;
};

MNEMOSYNE_PERSISTENT int next_step = 0;
MNEMOSYNE_PERSISTENT struct dummy_s *dummy_ptr1;
MNEMOSYNE_PERSISTENT struct dummy_s *dummy_ptr2;

#include <stdint.h>

main()
{
	struct dummy_s *d1;
	struct dummy_s *d2;

    __tm_atomic {
	    d1 = (struct dummy_s *) pmalloc(1024);
	}
    printf("%p\n", d1);

    __tm_atomic {
	    pfree(d1);
        //__tm_abort;
	}
}
