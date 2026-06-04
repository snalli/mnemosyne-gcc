cmake_minimum_required(VERSION 3.12)
project(mnemosyne C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)
find_library(LIBCONFIG_LIB NAMES config REQUIRED)
find_library(LIBELF_LIB    NAMES elf    REQUIRED)

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tool/linker/linker_script_persistent_segment")

# Interface target carries shared include paths and compiler options.
# The linker script is NOT propagated here (it applies only to shared libs
# and executables, not to the intermediate static libraries).
add_library(mnemosyne_iface INTERFACE)
target_include_directories(mnemosyne_iface INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/library/atomic_ops
    ${CMAKE_CURRENT_SOURCE_DIR}/library/common
    ${CMAKE_CURRENT_SOURCE_DIR}/library/mtm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/library/mtm/include/mode
    ${CMAKE_CURRENT_SOURCE_DIR}/library/mtm/include/mode/common
    ${CMAKE_CURRENT_SOURCE_DIR}/library/mtm/include/mode/pwb-common
    ${CMAKE_CURRENT_SOURCE_DIR}/library/mtm/include/mode/pwbetl
    ${CMAKE_CURRENT_SOURCE_DIR}/library/mtm/include/sysdeps/linux
    ${CMAKE_CURRENT_SOURCE_DIR}/library/mtm/include/sysdeps/x86
    ${CMAKE_CURRENT_SOURCE_DIR}/library/mcore/include
    ${CMAKE_CURRENT_SOURCE_DIR}/library/mcore/include/hal
    ${CMAKE_CURRENT_SOURCE_DIR}/library/mcore/include/log
    ${CMAKE_CURRENT_SOURCE_DIR}/library/pmalloc/include
)
target_compile_options(mnemosyne_iface INTERFACE
    -fgnu-tm
    -fno-omit-frame-pointer
    -Wall
    -Wextra
    -Wno-unused-parameter
    -g
    -O0
)
target_compile_definitions(mnemosyne_iface INTERFACE
    _MNEMOSYNE_BUILD_DEBUG
    _GNU_SOURCE
    HAVE_CONFIG_H
    M_PCM_CPUFREQ=2500
    M_PCM_LATENCY_WRITE=150
    M_PCM_BANDWIDTH_MB=1200
)

add_subdirectory(library/common)
add_subdirectory(library/mcore)
add_subdirectory(library/mtm)
add_subdirectory(library/pmalloc)

# -------------------------------------------------------------------------
# Combined shared library: libmnemosyne.so
# All three static libs are pulled in with --whole-archive so every object
# file ends up in the shared library, avoiding "undefined symbol" surprises.
# -------------------------------------------------------------------------

# Static analysis target (cppcheck)
find_program(CPPCHECK cppcheck)
if(CPPCHECK)
    add_custom_target(analysis
        COMMAND ${CPPCHECK} --enable=warning,style --quiet
            -I${CMAKE_SOURCE_DIR}/library/mcore/include
            -I${CMAKE_SOURCE_DIR}/library/mtm/include
            -I${CMAKE_SOURCE_DIR}/library/common
            ${CMAKE_SOURCE_DIR}/library/mcore/src
            ${CMAKE_SOURCE_DIR}/library/mtm/src
            ${CMAKE_SOURCE_DIR}/library/common
        COMMENT "Running cppcheck static analysis"
    )
endif()

add_library(mnemosyne SHARED dummy.c)
target_compile_options(mnemosyne PRIVATE -fPIC)
target_link_options(mnemosyne PRIVATE
    -T${LINKER_SCRIPT}
)
target_link_libraries(mnemosyne PRIVATE
    -Wl,--whole-archive
    mcore
    mtm
    pmalloc
    -Wl,--no-whole-archive
    ${LIBCONFIG_LIB}
    ${LIBELF_LIB}
    alps
    Threads::Threads
)

add_subdirectory(examples/simple)

enable_testing()
set(MNEMOSYNE_SRC ${CMAKE_CURRENT_SOURCE_DIR})
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../tests/unit
                 ${CMAKE_CURRENT_BINARY_DIR}/tests/unit)
