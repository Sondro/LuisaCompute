BuildProject({
	projectName = "lc_test",
	projectType = "binary"
})
add_includedirs("../ext/stb/")
add_files("test_rtx.cpp", "../ext/stb/stb.c")
add_deps("luisa-compute-dsl", "luisa-compute-runtime", "luisa-compute-vstl")
