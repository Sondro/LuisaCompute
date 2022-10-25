BuildProject({
	projectName = "lc-dsl",
	projectType = "shared"
})
add_deps("lc-ast", "lc-runtime")
add_files("**.cpp")
