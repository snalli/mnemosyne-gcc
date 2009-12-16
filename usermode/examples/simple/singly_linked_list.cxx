/*!
 * \file
 * Implements the template-parameterized functions of the SinglyLinkedList class.
 *
 * \note This is not a compilation unit. It is included with the matching header.
 *
 * \author Andres Jaan Tack
 */
#include <mnemosyne.h>


template <typename DataType>
void SinglyLinkedList<DataType>::append (DataType data)
{
	Node* new_node_area = reinterpret_cast<Node*>(mnemosyne_malloc(sizeof(Node)));
	if (itsTailNode != NULL)
		itsTailNode->itsNextNode = new(new_node_area) Node(data);
	else
		itsHeadNode = itsTailNode = new(new_node_area) Node(data);
	
	++itsLength;
}


template <class T>
std::ostream& operator<< (std::ostream &output, SinglyLinkedList<T>& list)
{
	typename SinglyLinkedList<T>::Node* current;
	for (current = list.itsFirstNode; current != NULL; current = current->nextNode)
		output << "[" << current->itsData << "] -> ";
		
	output << "[/]";
}
