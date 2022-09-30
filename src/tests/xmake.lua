BuildProject({
    projectName = "lc_test",
    projectType = "binary"
})
add_files("test_rtx.cpp")
add_deps("luisa-compute-dsl", "luisa-compute-rtx", "luisa-compute-vstl")
add_links("stb")