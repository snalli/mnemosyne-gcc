#include <stdio.h>
#include <iostream>

#define GLOBAL_PERSISTENT __attribute__ ((section("PERSISTENT")))

using namespace std;

extern "C" void * pmalloc(size_t);
extern "C" void pfree(void *);

GLOBAL_PERSISTENT int state = 0;
GLOBAL_PERSISTENT void *globalptr = 0;

main()
{
	void *ptr;
	switch(state) {
		case 0:
			ptr = pmalloc(100);
			cout << "pmalloc: " << ptr << endl;
			globalptr = ptr;
			state = 1;
			break;
		case 1:
			cout << "pfree: " << globalptr << endl;
			pfree(globalptr);
			state = 0;
			break;
	}		
}
