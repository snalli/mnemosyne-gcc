#include <iostream>
#include <mnemosyne.h>

MNEMOSYNE_PERSISTENT unsigned int my_counter = 0xDEAD0000;
MNEMOSYNE_PERSISTENT void         *my_segment = NULL;

int main (int argc, char const *argv[])
{
	int *segment_cnt;  //FIXME use persistent type to aid static analysis?

	if (!my_segment) {
		mnemosyne_segment_create((void *) 0xa0000000, 1024, 0, 0);
		my_segment = (void *) 0xa0000000;
	}	

	my_counter += 1;

	segment_cnt = (int *) 0xa0000000;
	(*segment_cnt)++;

	std::cout << "my_counter : " << std::hex << my_counter << std::dec << std::endl;
	std::cout << "*segment_cnt: " << *segment_cnt << std::endl;
	return 0;
	
}
