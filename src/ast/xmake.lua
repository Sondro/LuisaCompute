BuildProject({
    projectName = "luisa-compute-ast",
    projectType = "shared"
})
add_deps("luisa-compute-core", "luisa-compute-vstl")
add_files("**.cpp")
add_defines("LC_AST_EXPORT_DLL")
