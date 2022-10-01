set(LC_BACKEND_COMMON_SOURCES
        command_reorder_visitor.h command_reorder_visitor.hpp)

add_library(luisa-compute-backend-common INTERFACE ${LC_BACKEND_COMMON_SOURCES})
target_link_libraries(luisa-compute-backend-common INTERFACE
        luisa-compute-ast
        luisa-compute-dsl
        luisa-compute-runtime
        luisa-compute-compile
        luisa-compute-vstl)
