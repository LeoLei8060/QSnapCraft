add_library(hooks SHARED
    hooks.cpp
    hooks.h
)

target_compile_definitions(hooks PRIVATE HOOKS_EXPORTS)
