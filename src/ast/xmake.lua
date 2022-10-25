BuildProject({
	projectName = "lc-ast",
	projectType = "shared",
	unityBuildBatch = 4
})
add_deps("lc-core", "lc-vstl")
add_files("**.cpp")
add_defines("LC_AST_EXPORT_DLL")
