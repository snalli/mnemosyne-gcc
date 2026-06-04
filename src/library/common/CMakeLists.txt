add_library(mnemosyne_common STATIC
    debug.c
)
target_link_libraries(mnemosyne_common PUBLIC mnemosyne_iface)
target_compile_options(mnemosyne_common PRIVATE -fPIC)
