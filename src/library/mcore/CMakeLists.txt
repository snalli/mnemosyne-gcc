add_library(mcore STATIC
    src/config.c
    src/module.c
    src/files.c
    src/init.c
    src/reincarnation_callback.c
    src/segment.c
    src/hal/pcm.c
    src/log/mgr.c
    src/log/phlog_base.c
    src/log/phlog_tornbit.c
    src/log/logtrunc.c
    # shared common sources compiled once here (single definition in libmnemosyne)
    ${CMAKE_SOURCE_DIR}/library/common/chhash.c
    ${CMAKE_SOURCE_DIR}/library/common/config_generic.c
    ${CMAKE_SOURCE_DIR}/library/common/debug.c
)

target_include_directories(mcore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/library/common
)
target_link_libraries(mcore
    PUBLIC  mnemosyne_iface
    PRIVATE Threads::Threads
    PRIVATE ${LIBCONFIG_LIB}
    PRIVATE ${LIBELF_LIB}
)
set_target_properties(mcore PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_options(mcore PRIVATE -pthread)
target_compile_definitions(mcore PRIVATE
    MCORE_KERNEL_PMAP
)
