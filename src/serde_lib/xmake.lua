BuildProject({
    projectName = "VEngine_Database",
    projectType = "shared",
    unityBuildBatch = 4
})
add_files("**.cpp")
add_deps("luisa-compute-vstl")
add_defines("LC_SERDE_LIB_EXPORT_DLL", "VENGINE_CSHARP_SUPPORT")