#include <mnemosyne.h>
struct dummy_s {
	int a;
	int b;
	int c;
	int d;
};

MNEMOSYNE_PERSISTENT int reincarnated = 0;
MNEMOSYNE_PERSISTENT struct dummy_s *dummy_ptr;

main()
{
	if (reincarnated == 0) {
		dummy_ptr = (struct dummy_s *) mnemosyne_malloc(1024);
		reincarnated = 1;
		dummy_ptr->a = 0xDEAD0000;
		dummy_ptr->b = 0xDEAD0001;
		dummy_ptr->c = 0xDEAD0002;
		dummy_ptr->d = 0xDEAD0003;
	} else {
		printf("dummy_ptr: %p\n", dummy_ptr);
		printf("a: %x\n", dummy_ptr->a);
		printf("b: %x\n", dummy_ptr->b);
		printf("c: %x\n", dummy_ptr->c);
		printf("d: %x\n", dummy_ptr->d);
		mnemosyne_free(dummy_ptr);
		reincarnated = 0;
	}

}
