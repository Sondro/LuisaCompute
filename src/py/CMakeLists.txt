SET(LUISA_PYTHON_SOURCES
        lcapi.cpp
        export_op.cpp
        export_vector2.cpp
        export_vector3.cpp
        export_vector4.cpp
        export_matrix.cpp)
pybind11_add_module(lcapi ${LUISA_PYTHON_SOURCES})
if (WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(lcapi PRIVATE -Wa,-mbig-obj)
endif ()
target_link_libraries(lcapi PRIVATE luisa::compute)
add_custom_target(lcapi-copy ALL)
add_custom_command(TARGET lcapi-copy
        DEPENDS test.py
        COMMENT "Copying Python test"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/test.py"
        "$<TARGET_FILE_DIR:luisa-compute-core>/test.py"
        VERBATIM)
add_custom_command(TARGET lcapi-copy
        DEPENDS test_sdf_render.py
        COMMENT "Copying Python test"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/test_sdf_render.py"
        "$<TARGET_FILE_DIR:luisa-compute-core>/test_sdf_render.py"
        VERBATIM)
add_custom_command(TARGET lcapi-copy
        DEPENDS test_sdf_render.py
        COMMENT "Copying Python test"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/test_path_tracing.py"
        "$<TARGET_FILE_DIR:luisa-compute-core>/test_path_tracing.py"
        VERBATIM)
add_custom_command(TARGET lcapi-copy
        DEPENDS test_sdf_render.py
        COMMENT "Copying Python test"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/test_rtx.py"
        "$<TARGET_FILE_DIR:luisa-compute-core>/test_rtx.py"
        VERBATIM)
add_custom_command(TARGET lcapi-copy
        DEPENDS test_sdf_render.py
        COMMENT "Copying Python test"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/cornell_box.py"
        "$<TARGET_FILE_DIR:luisa-compute-core>/cornell_box.py"
        VERBATIM)
add_custom_command(TARGET lcapi-copy
        DEPENDS luisa
        COMMENT "Copying Python package"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/luisa"
        "$<TARGET_FILE_DIR:luisa-compute-core>/luisa"
        VERBATIM)
add_dependencies(lcapi lcapi-copy)
