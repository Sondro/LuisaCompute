set(LUISA_COMPUTE_RASTER_SOURCES
raster_shader.cpp
depth_buffer.cpp)

add_library(luisa-compute-raster SHARED ${LUISA_COMPUTE_RASTER_SOURCES})
target_link_libraries(luisa-compute-raster PUBLIC luisa-compute-runtime luisa-compute-dsl)
target_compile_definitions(luisa-compute-raster PRIVATE LC_RASTER_EXPORT_DLL=1)
set_target_properties(luisa-compute-raster PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        UNITY_BUILD ${LUISA_COMPUTE_ENABLE_UNITY_BUILD})
