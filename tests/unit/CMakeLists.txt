enable_testing()

# Include paths needed by the portability-layer tests.
# These tests do NOT link against libmnemosyne — they only test headers.
set(PORTABILITY_INCLUDES
    ${MNEMOSYNE_SRC}/library/common
    ${MNEMOSYNE_SRC}/library/mtm/include
    ${MNEMOSYNE_SRC}/library/mtm/include/sysdeps/x86
    ${MNEMOSYNE_SRC}/library/mcore/include
    ${MNEMOSYNE_SRC}/library/mcore/include/hal
    ${MNEMOSYNE_SRC}/library/atomic_ops
)

# Shared compile flags matching the library build
set(PORTABILITY_FLAGS
    -fgnu-tm
    -fno-omit-frame-pointer
    -w
    -g
    -D_GNU_SOURCE
    -DHAVE_CONFIG_H
    -DM_PCM_CPUFREQ=2500
    -DM_PCM_LATENCY_WRITE=150
    -DM_PCM_BANDWIDTH_MB=1200
    -DMCORE_KERNEL_PMAP
)

function(add_portability_test name src)
    add_executable(${name} ${src})
    target_include_directories(${name} PRIVATE ${PORTABILITY_INCLUDES})
    target_compile_options(${name} PRIVATE ${PORTABILITY_FLAGS})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_portability_test(test_portable_atomics  test_portable_atomics.c)
add_portability_test(test_portable_hrtime   test_portable_hrtime.c)
add_portability_test(test_portable_spinlock test_portable_spinlock.c)
add_portability_test(test_simd_typesizes    test_simd_typesizes.c)
