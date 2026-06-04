add_library(mtm STATIC
    src/config.c
    src/gc.c
    src/init.c
    src/gcc-abi.c
    src/local.c
    src/mode/mode.c
    src/mode/pwbnl.c
    src/mode/common/common.c
    src/mode/pwb-common/pwb.c
    src/mode/pwb-common/tmlog_base.c
    src/mode/pwb-common/tmlog_tornbit.c
    src/mode/pwbetl/beginend.c
    src/mode/pwbetl/memcpy.c
    src/mode/pwbetl/memset.c
    src/mode/pwbetl/pwbetl.c
    src/mode/pwbetl/barrier.c
    src/mtm.c
    src/stats.c
    src/txlock.c
    src/useraction.c
    src/sysdeps/linux/rwlock.c
    src/sysdeps/x86/arch.c
)

target_include_directories(mtm PRIVATE
    ${CMAKE_SOURCE_DIR}/library/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sysdeps/x86
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sysdeps/linux
)
target_link_libraries(mtm
    PUBLIC  mnemosyne_iface
    PRIVATE mcore
    PRIVATE Threads::Threads
    PRIVATE ${LIBCONFIG_LIB}
)
target_compile_options(mtm PRIVATE -fPIC)
target_compile_definitions(mtm PRIVATE
    # boolean flags (True entries from default/mtm.py)
    ROLLOVER_CLOCK
    CLOCK_IN_CACHE_LINE
    NO_DUPLICATES_IN_RW_SETS
    WAIT_YIELD
    LOCK_IDX_SWAP
    SYNC_TRUNCATION
    # enum flags
    CM=CM_SUICIDE
    TMLOG_TYPE=TMLOG_TYPE_BASE
    # numerical flags
    RW_SET_SIZE=32768
    LOCK_ARRAY_LOG_SIZE=20
    LOCK_SHIFT_EXTRA=2
    PRIVATE_LOCK_ARRAY_LOG_SIZE=8
    MIN_BACKOFF=0x04UL
    MAX_BACKOFF=0x80000000UL
    VR_THRESHOLD_DEFAULT=3
    CM_THRESHOLD_DEFAULT=0
)

set_property(SOURCE src/sysdeps/x86/arch.c
    APPEND PROPERTY COMPILE_OPTIONS -DHAVE_CONFIG_H
)
