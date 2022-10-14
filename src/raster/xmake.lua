BuildProject({
	projectName = "luisa-compute-raster",
	projectType = "shared"
})
add_deps("luisa-compute-runtime", "luisa-compute-dsl", "luisa-compute-vstl")
add_files("**.cpp")
add_defines("LC_RASTER_EXPORT_DLL")
