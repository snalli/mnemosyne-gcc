#include "experiment_options.hxx"
#include <algorithm>
#include <cstdlib>
#include <map>
using std::map;
#include <fstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/program_options.hpp>
#include <sys/time.h>
#include <string>
#include <vector>

/*
 * COMMAND LINE OPTIONS
 *
 * -s, --size-of-tree     Size of the tree
 * -f, --output-file      File location of the serialized tree
 */

std::string output_filename = "/mnt/pcmfs/tmp";

class BigObject
{
public:
	template<class Archive>
    inline void serialize(Archive & ar, const unsigned int version)
    {
		ar & itsData;
    }
private:
	char itsData[128];
};

/*! The subject of the experiment. */
static map<size_t, BigObject>* theMap;

int main (int argc, char const *argv[])
{
	// Prepare!
	std::srand(time(NULL));
	po::variables_map options = parsed_options(argc, argv);
	
	// Initialize this shit! :D
	std::cerr << "Creating map... ";
	theMap = new map<size_t, BigObject>;
	std::cerr << "done! (" << theMap << ')' << std::endl;
	
	if (options.count("output-file")) {
		output_filename = options["output-file"].as< std::string >();
	}	
	// Build the tree!
	const size_t required_tree_size = options["size-of-tree"].as<size_t>();
	std::cerr << "Building a tree of " << required_tree_size << " elements." << std::endl;
	for(size_t size = 0; size < required_tree_size; ++size)
		theMap->insert(std::pair<size_t, BigObject>(rand(), BigObject()));
	
	// Serialize the tree.
	std::cerr << "Serializing the tree." << std::endl;
	struct timeval begin;
	gettimeofday(&begin, NULL);
	
	{
		std::ofstream archive_file(output_filename.c_str());
		boost::archive::binary_oarchive archive(archive_file);
		archive << *theMap;
		archive_file.rdbuf()->pubsync();

		// FIXME: Haris: I don't understant why Tack removed and renamed 
		//        the file, and why he didn't synced. 
		//        He had the following:
		//        
		// std::ofstream archive_file("tmp");
		// boost::archive::binary_oarchive archive(archive_file);
		// archive << *theMap;
		// boost::filesystem::remove("archive");
		// boost::filesystem::rename("tmp", "archive");

	}
	
	struct timeval end;
	gettimeofday(&end, NULL);

	// Print the time it took!
	std::cout << "Output: " << output_filename << std::endl;
	std::cout << "Serialization Time: " << 1000000 * (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) << std::endl;

	return 0;
}


po::variables_map parsed_options(int argc, const char* argv[])
{
	po::options_description descriptions("Experiment Options");
	descriptions.add_options()
		("size-of-tree,s", po::value<size_t>(), "")
		("output-file,f", po::value< std::string >(), "")
		;

	po::variables_map result;
	po::store(po::parse_command_line(argc, const_cast<char**>(argv), descriptions), result);
	po::notify(result);
	return result;
}
