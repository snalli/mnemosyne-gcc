# Persistent-memory memcached (1.2.4) built on mnemosyne.
#
# This is the MTM variant: hash-table state lives in persistent memory and is
# updated transactionally (-fgnu-tm).  It links the combined libmnemosyne.so
# and, like the examples, must keep its PERSISTENT section at a fixed ELF
# address (-no-pie + the persistent-segment linker script).

find_package(Threads REQUIRED)

# libevent (memcached's event loop)
find_library(LIBEVENT_LIB NAMES event libevent)
if(NOT LIBEVENT_LIB)
    message(WARNING "libevent not found; skipping memcached benchmark build")
    return()
endif()

set(MC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/memcached-1.2.4-mtm)

add_executable(memcached_mtm
    ${MC_DIR}/memcached.c
    ${MC_DIR}/assoc.c
    ${MC_DIR}/helper.c
    ${MC_DIR}/items.c
    ${MC_DIR}/slabs.c
    ${MC_DIR}/stats.c
    ${MC_DIR}/thread.c
    ${MC_DIR}/pvar.c
)

target_include_directories(memcached_mtm PRIVATE
    ${MC_DIR}
    ${MNEMOSYNE_SRC}/library/mcore/include
    ${MNEMOSYNE_SRC}/library/mtm/include
    ${MNEMOSYNE_SRC}/library/pmalloc/include
    ${MNEMOSYNE_SRC}/library/common
)

target_compile_options(memcached_mtm PRIVATE
    -fgnu-tm
    -fno-omit-frame-pointer
    -O2
    # tm_def.h redeclares fprintf/snprintf as transaction_pure, which clashes
    # with glibc's _FORTIFY_SOURCE always_inline wrappers (auto-enabled at -O2).
    -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0
    -D_GNU_SOURCE -DHAVE_CONFIG_H
    # This is decade-old C; keep its own warnings non-fatal.
    -Wno-deprecated-declarations
    -Wno-implicit-function-declaration
)

target_link_options(memcached_mtm PRIVATE
    -T${MNEMOSYNE_SRC}/tool/linker/linker_script_persistent_segment
    -no-pie
)

target_link_libraries(memcached_mtm
    mnemosyne
    ${LIBEVENT_LIB}
    Threads::Threads
)
