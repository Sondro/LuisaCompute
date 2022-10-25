BuildProject({
	projectName = "lc-runtime",
	projectType = "shared",
	unityBuildBatch = 4
})
add_deps("lc-ast")
add_defines("LC_RUNTIME_EXPORT_DLL")
add_files("**.cpp", "../raster/**.cpp", "../rtx/**.cpp")
