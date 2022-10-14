set(LC_BACKEND_COMMON_SOURCES
        command_reorder_visitor.cpp command_reorder_visitor.h)

add_library(luisa-compute-backend-common STATIC ${LC_BACKEND_COMMON_SOURCES})
target_link_libraries(luisa-compute-backend-common PUBLIC
        luisa-compute-ast
        luisa-compute-dsl
        luisa-compute-runtime
        luisa-compute-compile
        luisa-compute-vstl)
