BuildProject({
    projectName = "luisa-compute-dsl",
    projectType = "shared"
})
add_deps("luisa-compute-ast", "luisa-compute-runtime")
add_files("**.cpp")