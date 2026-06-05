enable_testing()
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# =============================================================================
# Unit tests
#   Self-contained tests of individual components (atomics, hashing, timing,
#   SIMD shims, spinlocks). No persistent runtime required; each also gets a
#   valgrind variant.
# =============================================================================

set(UNIT_INCLUDES
    ${MNEMOSYNE_SRC}/library/common
    ${MNEMOSYNE_SRC}/library/mtm/include
    ${MNEMOSYNE_SRC}/library/mtm/include/sysdeps/x86
    ${MNEMOSYNE_SRC}/library/mcore/include
    ${MNEMOSYNE_SRC}/library/mcore/include/hal
    ${MNEMOSYNE_SRC}/library/atomic_ops
)

set(UNIT_FLAGS
    -fgnu-tm
    -fno-omit-frame-pointer
    -Wall -Wextra -Wno-unused-parameter
    -g
    -D_GNU_SOURCE -DHAVE_CONFIG_H
    -DM_PCM_CPUFREQ=2500 -DM_PCM_LATENCY_WRITE=150 -DM_PCM_BANDWIDTH_MB=1200
    -DMCORE_KERNEL_PMAP
)

find_program(VALGRIND valgrind)

# Register a unit test and (when valgrind is available) a memcheck variant.
function(add_unit_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;EXTRA_LIBS" ${ARGN})
    add_executable(${name} ${ARG_SOURCES})
    target_include_directories(${name} PRIVATE ${UNIT_INCLUDES})
    target_compile_options(${name} PRIVATE ${UNIT_FLAGS})
    target_link_libraries(${name} PRIVATE GTest::gtest_main Threads::Threads ${ARG_EXTRA_LIBS})
    gtest_discover_tests(${name})

    if(VALGRIND)
        add_test(NAME ${name}_valgrind
            COMMAND ${VALGRIND}
                --error-exitcode=1
                --leak-check=full
                --show-leak-kinds=definite,indirect
                --errors-for-leak-kinds=definite,indirect
                $<TARGET_FILE:${name}>
        )
        set_tests_properties(${name}_valgrind PROPERTIES TIMEOUT 120)
    endif()
endfunction()

add_unit_test(test_atomics         SOURCES test_atomics.cpp)
add_unit_test(test_hrtime          SOURCES test_hrtime.cpp)
add_unit_test(test_simd_types      SOURCES test_simd_types.cpp)
add_unit_test(test_memory_barriers SOURCES test_memory_barriers.cpp)

# chhash compiles chhash.c directly into the test binary
add_unit_test(test_chhash
    SOURCES test_chhash.cpp ${MNEMOSYNE_SRC}/library/common/chhash.c
)

# Concurrent test excluded from valgrind — no heap allocs, and spinlocks
# under valgrind's instrumentation are orders of magnitude slower.
add_executable(test_spinlock test_spinlock.cpp)
target_include_directories(test_spinlock PRIVATE ${UNIT_INCLUDES})
target_compile_options(test_spinlock PRIVATE ${UNIT_FLAGS})
target_link_libraries(test_spinlock PRIVATE GTest::gtest_main Threads::Threads)
gtest_discover_tests(test_spinlock PROPERTIES TIMEOUT 30)

# =============================================================================
# Integration tests
#   Exercise the live mnemosyne runtime: persistent mapping, persistent malloc,
#   and the raw physical log. These link the combined libmnemosyne.so and place
#   their PERSISTENT globals at a fixed ELF address (-no-pie + linker script).
#   They share /dev/shm segment files, so they run serially with a small heap.
# =============================================================================

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
    target_link_options(${name} PRIVATE
        -T${LINKER_SCRIPT}
        -no-pie
    )
    add_test(NAME ${name}
        COMMAND ${name}
        WORKING_DIRECTORY ${MNEMOSYNE_SRC}
    )
    # Integration tests share /dev/shm/psegments segment files; mnemosyne cannot
    # be initialised by two processes concurrently, so run them one at a time.
    # Use a small persistent-heap region so the backing store fits comfortably
    # in a modest /dev/shm (the default heap region is 8 GiB).
    set_tests_properties(${name} PROPERTIES
        TIMEOUT 30
        RUN_SERIAL YES
        ENVIRONMENT "MNEMOSYNE_PHEAP_SIZE_MB=32"
    )
endfunction()

add_integration_test(itest_pmap    test_pmap.cpp)
add_integration_test(itest_malloc  test_malloc.cpp)
add_integration_test(itest_rawlog  test_rawlog.cpp)
