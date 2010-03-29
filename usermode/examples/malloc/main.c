#include <mnemosyne.h>
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
typedef uint64_t hrtime_t;

static inline void asm_cpuid() {
    asm volatile( "cpuid" :::"rax", "rbx", "rcx", "rdx");
}


static inline unsigned long long asm_rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
//__attribute__((tm_callable)) void* pmalloc(size_t bytes);

main()
{
	hrtime_t start;
	hrtime_t stop;

	struct dummy_s *dummy_ptr;
	struct dummy_s *d1;
	struct dummy_s *d2;
	struct dummy_s *d3;
	switch (next_step) {
		case 0:
			//__tm_atomic 
			{
				__tm_waiver {
					d1 = (struct dummy_s *) pmalloc(1024);
					d2 = (struct dummy_s *) pmalloc(1024);
					d3 = (struct dummy_s *) pmalloc(1024);
					 printf("d1=%p\n", d1);
					pfree(d1); 
					printf("d2=%p\n", d2);
					pfree(d2); 
					printf("d3=%p\n", d3);
					pfree(d3);
					start = asm_rdtsc();
				}
				dummy_ptr = dummy_ptr1 = (struct dummy_s *) pmalloc(1024);
				__tm_waiver {
					asm_cpuid();
					stop = asm_rdtsc();
				}
			}
			printf("malloc duration: %lld\n", stop-start);
			next_step = 0;
			dummy_ptr->a = 0xDEAD0000;
			dummy_ptr->b = 0xDEAD0001;
			dummy_ptr->c = 0xDEAD0002;
			dummy_ptr->d = 0xDEAD0003;
			break;
		case 1:
			dummy_ptr = dummy_ptr2 = (struct dummy_s *) pmalloc(1024);
			next_step = 2;
			dummy_ptr->a = 0xDEAD1000;
			dummy_ptr->b = 0xDEAD1001;
			dummy_ptr->c = 0xDEAD1002;
			dummy_ptr->d = 0xDEAD1003;
			break;
		case 2:
			printf("dummy_ptr1: %p\n", dummy_ptr1);
			printf("a: %x\n", dummy_ptr1->a);
			printf("b: %x\n", dummy_ptr1->b);
			printf("c: %x\n", dummy_ptr1->c);
			printf("d: %x\n", dummy_ptr1->d);
			pfree(dummy_ptr1);
			next_step = 3;
			break;
		case 3:
			printf("dummy_ptr2: %p\n", dummy_ptr2);
			printf("a: %x\n", dummy_ptr2->a);
			printf("b: %x\n", dummy_ptr2->b);
			printf("c: %x\n", dummy_ptr2->c);
			printf("d: %x\n", dummy_ptr2->d);
			pfree(dummy_ptr2);
			next_step = 0;

	}

}
