BuildProject({
	projectName = "lc-compile",
	projectType = "shared"
})
add_deps("lc-ast")
add_defines("LC_COMPILE_EXPORT_DLL")
add_files("**.cpp")
