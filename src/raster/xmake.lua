BuildProject({
    projectName = "luisa-compute-raster",
    projectType = "shared"
})
add_deps("luisa-compute-runtime","luisa-compute-dsl")
add_files("**.cpp")
add_defines("LC_RASTER_EXPORT_DLL")
