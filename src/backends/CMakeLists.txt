add_library(luisa-compute-backends INTERFACE)
function(luisa_compute_add_backend name)
    cmake_parse_arguments(BACKEND "" "" "SOURCES" ${ARGN})
    add_library(luisa-compute-backend-${name} MODULE ${BACKEND_SOURCES})
    target_link_libraries(luisa-compute-backend-${name} PRIVATE
            luisa-compute-ast
            luisa-compute-dsl
            luisa-compute-runtime
            luisa-compute-compile
            luisa-compute-gui
            luisa-compute-vstl)
    add_dependencies(luisa-compute-backends luisa-compute-backend-${name})
    set_target_properties(luisa-compute-backend-${name} PROPERTIES
            UNITY_BUILD ${LUISA_COMPUTE_ENABLE_UNITY_BUILD}
            DEBUG_POSTFIX "")
endfunction()

