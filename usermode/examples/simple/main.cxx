/*!
 * \file
 * This example program, if implemented correctly, will generate the following output across
 * two runs.
 *
 * First Run:
 *   There were 0 messages and 0 people stored here.
 *   Adding two new messages and a person...
 *
 * Second Run:
 *   There were 2 messages and 1 people stored here.
 *   Adding two new messages and a person...
 */
#include <iostream>
#include <mnemosyne.h>
#include "singly_linked_list.hxx"

// Imagine we have two custom classes, that we wrote as the programmer...
typedef unsigned int Message;
typedef unsigned int Person;

// Some random structure of things that I want to remember between
// executions of the program. For all we care, the contents could
// be pointers to Fizzbat classes.
typedef struct	{
	SinglyLinkedList<Message>* messages;
	SinglyLinkedList<Person>* people;
} my_objects_t;

// This is our always-persistent data. As the programmer, I don't care how this works.
MNEMOSYNE_PERSISTENT my_objects_t my_objects;

/*!
 * Initializes our persistent structures on the _first_ run of the program.
 * See definition below...
 *
 * Notice that any "new" calls are implictly persistent, because they are being
 * pointed-to by a persistent pointer (a member of a persistent structure).
 */
static
void InitializeFirstTime();

int main (int argc, char const *argv[])
{
	InitializeFirstTime();
	
	std::cout << "There were " <<
		my_objects.messages->length() << " messages and " <<
		my_objects.people->length() << " people stored here." << std::endl;
		
	std::cerr << "Adding two new messages and a person..." << std::endl;
	MNEMOSYNE_ATOMIC	{
		my_objects.messages->append(Message(42));
		my_objects.messages->append(Message(89));
		my_objects.people->append(Person(2114));
	}
	
	return 0;
}


static
void InitializeFirstTime()	{
	MNEMOSYNE_ATOMIC	{
		if (my_objects.messages == NULL)
			my_objects.messages = new SinglyLinkedList<Message>;   // This is automatically a persistent allocation,
			                                             // and it's automatically recovered when we run
			                                             // the process!

		if (my_objects.people == NULL)
			my_objects.people = new SinglyLinkedList<Person>;        // As above, automatically persistent allocation.
	}
	mnemosyne_segment_create((void *) 0xa0000000, 1024, 0, 0);
}
