#include <sys/mman.h>
#include <iostream>
#include <mnemosyne.h>

MNEMOSYNE_PERSISTENT unsigned int my_counter = 0xDEAD0000;
MNEMOSYNE_PERSISTENT void         *my_segment = NULL;

int main (int argc, char const *argv[])
{
	int *segment_cnt;  //FIXME use persistent type to aid static analysis?

	std::cout << "&my_counter : " << std::hex << &my_counter << std::dec << std::endl;
	std::cout << "&my_segment : " << std::hex << &my_segment << std::dec << std::endl;

	if (!my_segment) {
		my_segment = m_pmap((void *) 0xb0000000, 1024, PROT_READ|PROT_WRITE, 0);
		my_segment = m_pmap((void *) 0xa000000, 1024, PROT_READ|PROT_WRITE, 0);
		my_segment = m_pmap((void *) 0xc0000000, 1024, PROT_READ|PROT_WRITE, 0);
	}	

	my_counter += 1;

	segment_cnt = (int *) my_segment;
	(*segment_cnt)++;

	std::cout << "my_counter : " << std::hex << my_counter << std::dec << std::endl;
	std::cout << "*segment_cnt: " << *segment_cnt << std::endl;
	return 0;
	
}
