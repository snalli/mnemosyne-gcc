#include <boost/program_options/variables_map.hpp>
namespace po = boost::program_options;

/*!
 * Get a description set of the options used to run this example program.
 */
po::variables_map parsed_options(int argc, const char* argv[]);
