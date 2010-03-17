/*!
 * \file
 * Implements the template-parameterized functions of the SinglyLinkedList class.
 *
 * \note This is not a compilation unit. It is included with the matching header.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#include <mnemosyne.h>


template <typename DataType>
__attribute__((tm_callable))
void SinglyLinkedList<DataType>::append (DataType data)
{
	Node* new_node_area = reinterpret_cast<Node*>(pmalloc(sizeof(Node)));
	if (itsTailNode != NULL) {
		itsTailNode->itsNextNode = new(new_node_area) Node(data);
		itsTailNode = itsTailNode->itsNextNode;
	}
	else
		itsHeadNode = itsTailNode = new(new_node_area) Node(data);
	
	++itsLength;
}


template <class T>
std::ostream& operator<< (std::ostream &output, SinglyLinkedList<T>& list)
{
	typename SinglyLinkedList<T>::Node* current;
	for (current = list.itsHeadNode; current != NULL; current = current->itsNextNode)
		output << "[" << current->itsData << "] -> ";
	
	output << "[/]";
	
	return output;
}
