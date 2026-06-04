find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

set(INTEG_INCLUDES
    ${MNEMOSYNE_SRC}/library/mcore/include
    ${MNEMOSYNE_SRC}/library/mcore/include/hal
    ${MNEMOSYNE_SRC}/library/mcore/include/log
    ${MNEMOSYNE_SRC}/library/mtm/include
    ${MNEMOSYNE_SRC}/library/mtm/include/mode
    ${MNEMOSYNE_SRC}/library/mtm/include/mode/common
    ${MNEMOSYNE_SRC}/library/mtm/include/mode/pwb-common
    ${MNEMOSYNE_SRC}/library/mtm/include/mode/pwbetl
    ${MNEMOSYNE_SRC}/library/mtm/include/sysdeps/x86
    ${MNEMOSYNE_SRC}/library/mtm/include/sysdeps/linux
    ${MNEMOSYNE_SRC}/library/pmalloc/include
    ${MNEMOSYNE_SRC}/library/common
    ${MNEMOSYNE_SRC}/library/atomic_ops
)

set(INTEG_FLAGS
    -fgnu-tm
    -fno-omit-frame-pointer
    -Wall -Wextra -Wno-unused-parameter
    -g
    -D_GNU_SOURCE -DHAVE_CONFIG_H
    -DM_PCM_CPUFREQ=2500 -DM_PCM_LATENCY_WRITE=150 -DM_PCM_BANDWIDTH_MB=1200
    -DMCORE_KERNEL_PMAP
    -DROLLOVER_CLOCK -DCLOCK_IN_CACHE_LINE -DNO_DUPLICATES_IN_RW_SETS
    -DWAIT_YIELD -DLOCK_IDX_SWAP -DSYNC_TRUNCATION
    -DCM=CM_SUICIDE -DTMLOG_TYPE=TMLOG_TYPE_BASE
    -DRW_SET_SIZE=32768 -DLOCK_ARRAY_LOG_SIZE=20 -DLOCK_SHIFT_EXTRA=2
    -DPRIVATE_LOCK_ARRAY_LOG_SIZE=8
)

set(LINKER_SCRIPT "${MNEMOSYNE_SRC}/tool/linker/linker_script_persistent_segment")

function(add_integration_test name src)
    add_executable(${name} ${src})
    target_include_directories(${name} PRIVATE ${INTEG_INCLUDES})
    target_compile_options(${name} PRIVATE ${INTEG_FLAGS})
    target_link_libraries(${name} PRIVATE
        GTest::gtest_main
        Threads::Threads
        mnemosyne          # the combined shared library
    )
    target_link_options(${name} PRIVATE -T${LINKER_SCRIPT})
    add_test(NAME ${name}
        COMMAND ${name}
        WORKING_DIRECTORY ${MNEMOSYNE_SRC}
    )
    set_tests_properties(${name} PROPERTIES TIMEOUT 30)
endfunction()

add_integration_test(itest_pmap    test_pmap.cpp)
add_integration_test(itest_malloc  test_malloc.cpp)
add_integration_test(itest_rawlog  test_rawlog.cpp)
