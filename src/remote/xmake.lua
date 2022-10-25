BuildProject({
	projectName = "lc-remote",
	projectType = "shared",
	unityBuildBatch = 4
})
add_deps("lc-vstl", "lc-runtime", "lc-ast")
add_files("*.cpp")
