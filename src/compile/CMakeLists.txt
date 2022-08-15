set(LUISA_COMPUTE_COMPILE_SOURCES
        codegen.cpp codegen.h
        definition_analysis.cpp definition_analysis.h
        hlsl/codegen_stack_data.h
        hlsl/codegen_stack_data.cpp
        hlsl/dx_codegen.h
        hlsl/dx_codegen.cpp
        hlsl/dx_codegen_util.cpp
        hlsl/struct_generator.h
        hlsl/struct_generator.cpp
        hlsl/shader_property.h
        hlsl/shader_header.h
        hlsl/shader_header.cpp)

add_library(luisa-compute-compile SHARED ${LUISA_COMPUTE_COMPILE_SOURCES})
target_link_libraries(luisa-compute-compile PUBLIC luisa-compute-ast)
set_target_properties(luisa-compute-compile PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        UNITY_BUILD ${LUISA_COMPUTE_ENABLE_UNITY_BUILD})
