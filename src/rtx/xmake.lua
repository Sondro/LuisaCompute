BuildProject({
	projectName = "luisa-compute-rtx",
	projectType = "shared",
	unityBuildBatch = 4
})
add_deps("luisa-compute-runtime", "luisa-compute-dsl")
add_files("**.cpp")
add_defines("LC_RTX_EXPORT_DLL")
