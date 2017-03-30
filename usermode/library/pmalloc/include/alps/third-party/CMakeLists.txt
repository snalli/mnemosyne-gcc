# these are third party code, we don't care warning.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

set(BUILD_TESTING OFF) # Disable testcases in these third party libraries

add_subdirectory(gtest)

add_subdirectory(libbacktrace)

set(BUILD_SHARED_LIBS OFF) # we use gflags as static library only
set(BUILD_STATIC_LIBS ON)
#add_subdirectory(gflags)

# we use the most recent release from github as the l4tm package brings a 
# broken header file
# we disable yaml-cpp testing (in yaml-cpp-0.5.2/CMakeLists.txt) as it
# builds gtest which conflicts with our gtest target
add_subdirectory(yaml-cpp-0.5.2)
