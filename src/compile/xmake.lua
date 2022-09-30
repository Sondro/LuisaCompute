BuildProject({
    projectName = "luisa-compute-compile",
    projectType = "shared"
})
add_deps("luisa-compute-ast")
add_defines("LC_COMPILE_EXPORT_DLL")
add_files("**.cpp")