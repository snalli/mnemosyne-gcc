enable_testing()
find_package(GTest REQUIRED)

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

# Helper: register a test and optionally a valgrind variant
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

# ---- tests ----

add_unit_test(test_atomics       SOURCES test_atomics.cpp)
add_unit_test(test_hrtime        SOURCES test_hrtime.cpp)
# Concurrent test excluded from valgrind — no heap allocs, and spinlocks
# under valgrind's instrumentation are orders of magnitude slower.
add_executable(test_spinlock test_spinlock.cpp)
target_include_directories(test_spinlock PRIVATE ${UNIT_INCLUDES})
target_compile_options(test_spinlock PRIVATE ${UNIT_FLAGS})
target_link_libraries(test_spinlock PRIVATE GTest::gtest_main Threads::Threads)
gtest_discover_tests(test_spinlock)
add_unit_test(test_simd_types    SOURCES test_simd_types.cpp)
add_unit_test(test_memory_barriers SOURCES test_memory_barriers.cpp)

# chhash compiles chhash.c directly into the test binary
add_unit_test(test_chhash
    SOURCES test_chhash.cpp ${MNEMOSYNE_SRC}/library/common/chhash.c
)
