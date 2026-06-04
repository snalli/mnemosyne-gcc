# Build ALPS first
set(TARGET_ARCH_MEM "CC-NUMA" CACHE STRING "ALPS target memory architecture")
add_subdirectory(include/alps)

add_library(pmalloc STATIC
    src/heap.cc
    src/wrapper.cc
)
target_include_directories(pmalloc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/alps/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/alps/include/alps
    ${CMAKE_CURRENT_SOURCE_DIR}/include/alps/include/alps/common
    ${CMAKE_CURRENT_SOURCE_DIR}/include/alps/include/alps/layers
    ${CMAKE_CURRENT_SOURCE_DIR}/include/alps/include/alps/pegasus
    ${CMAKE_CURRENT_SOURCE_DIR}/include/alps
)
target_link_libraries(pmalloc
    PUBLIC  mnemosyne_iface
    PRIVATE mcore
    PRIVATE alps
    PRIVATE Threads::Threads
)
target_compile_options(pmalloc PRIVATE
    -std=gnu++11
    -finline-functions
    -fpic

)
target_compile_definitions(pmalloc PRIVATE _M_BUILD_DEBUG)
