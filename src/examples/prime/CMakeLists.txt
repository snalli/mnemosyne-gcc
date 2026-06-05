add_executable(prime main.c)
target_include_directories(prime PRIVATE
    ${CMAKE_SOURCE_DIR}/library/mcore/include
    ${CMAKE_SOURCE_DIR}/library/mtm/include
    ${CMAKE_SOURCE_DIR}/library/pmalloc/include
    ${CMAKE_SOURCE_DIR}/library/common
)
target_compile_options(prime PRIVATE -fgnu-tm)
target_link_options(prime PRIVATE
    -T${CMAKE_SOURCE_DIR}/tool/linker/linker_script_persistent_segment
    -no-pie
)
target_link_libraries(prime mnemosyne Threads::Threads)
