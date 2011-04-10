#include "experiment_options.hxx"
#include <boost/program_options.hpp>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <map>
using std::map;
#include <mnemosyne.h>
#include <sys/time.h>

class BigObject
{
private:
	char itsData[128];
};

/*! The subject of the experiment. */
MNEMOSYNE_PERSISTENT
map<size_t, BigObject>* theMap;

void time_operations(const size_t operations_to_perform);

int main (int argc, char const *argv[])
{
	// Prepare!
	std::srand(time(NULL));
	po::variables_map options = parsed_options(argc, argv);
	
	// Initialize this shit! :D
	if (theMap == NULL) {
		std::cerr << "Creating map... ";
		theMap = new map<size_t, BigObject>;
		std::cerr << "done! (" << theMap << ')' << std::endl;
	} else {
		std::cout << "Map " << std::hex << theMap << " has " << std::dec << theMap->size() << " elements." << std::endl;
		std::cout << "... first is (" << theMap->begin()->first << ", ...)" << std::endl;
		return 0;
	}
	
	// Build the tree!
	const size_t required_tree_size = options["size-of-tree"].as<size_t>();
	std::cerr << "Building a tree of " << required_tree_size << " elements." << std::endl;
	for(size_t size = 0; size < required_tree_size; ++size)
		theMap->insert(std::pair<size_t, BigObject>(rand(), BigObject()));
	
	// Run the experiment body!
	std::cerr << "Running update experiments." << std::endl;
	std::vector<size_t> operation_count = options["operations-to-perform"].as< std::vector<size_t> >();
	std::for_each(operation_count.begin(), operation_count.end(), time_operations);
	
	return 0;
}


void time_operations(const size_t operations_to_perform) {
	struct timeval begin;
	gettimeofday(&begin, NULL);
	bool insert = true;
	for (size_t operationNumber = 0; operationNumber < operations_to_perform; ++operationNumber) {
		static std::pair<size_t, BigObject> last_pair;
		if (insert) {
			size_t first = rand();     BigObject second = BigObject();
			last_pair = std::make_pair(first, second);
			__tm_atomic theMap->insert(last_pair);
		} else {
			__tm_atomic theMap->erase(last_pair.first);
		}
		
		insert = !insert;
	}
	struct timeval end;
	gettimeofday(&end, NULL);
	
	// Print the time it took!
	std::cout << std::dec << operations_to_perform << " updates: " << 1000000 * (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) << std::endl;
}


po::variables_map parsed_options(int argc, const char* argv[])
{
	po::options_description descriptions("Experiment Options");
	descriptions.add_options()
		("size-of-tree,s", po::value<size_t>())
		("operations-to-perform,n", po::value< std::vector<size_t> >()->multitoken())
		;
	
	po::variables_map result;
	po::store(po::parse_command_line(argc, const_cast<char**>(argv), descriptions), result);
	po::notify(result);
	return result;
}
