BuildProject({
	projectName = "lc-dsl",
	projectType = "shared"
})
add_defines("LC_DSL_EXPORT_DLL")
add_deps("lc-ast", "lc-runtime")
add_files("**.cpp")
