BuildProject({
	projectName = "luisa-compute-remote",
	projectType = "shared",
	unityBuildBatch = 4
})
add_deps("luisa-compute-vstl", "luisa-compute-runtime", "luisa-compute-ast")
add_files("*.cpp")
