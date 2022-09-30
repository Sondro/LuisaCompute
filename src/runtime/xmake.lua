BuildProject({
    projectName = "luisa-compute-runtime",
    projectType = "shared",
    unityBuildBatch = 4
})
add_deps("luisa-compute-ast")
add_defines("LC_RUNTIME_EXPORT_DLL")
add_files("**.cpp")