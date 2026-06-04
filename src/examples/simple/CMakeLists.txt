add_library(pvar_simple STATIC pvar.c)
target_include_directories(pvar_simple PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pvar_simple PUBLIC mnemosyne_iface)
target_compile_options(pvar_simple PRIVATE -fgnu-tm)
set_target_properties(pvar_simple PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_executable(simple main.c)
target_include_directories(simple PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/library/mcore/include
    ${CMAKE_SOURCE_DIR}/library/mtm/include
    ${CMAKE_SOURCE_DIR}/library/pmalloc/include
    ${CMAKE_SOURCE_DIR}/library/common
    ${CMAKE_SOURCE_DIR}/library/mtm/include/sysdeps/x86
)
target_compile_options(simple PRIVATE -fgnu-tm)
target_link_options(simple PRIVATE
    -T${CMAKE_SOURCE_DIR}/tool/linker/linker_script_persistent_segment
    -no-pie   # ASLR would randomize the PERSISTENT section address; mnemosyne needs it fixed
)
target_link_libraries(simple
    mnemosyne
    pvar_simple
    Threads::Threads
)
