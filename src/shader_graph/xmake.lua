BuildProject({
    projectName = "lc-shadergraph",
    projectType = "shared"
})
add_deps("lc-vstl", "lc-runtime", "lc-ast")
add_files("**.cpp")
add_defines("LC_SHADERGRAPH_LIB_EXPORT_DLL")
