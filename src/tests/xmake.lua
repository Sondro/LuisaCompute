BuildProject({
	projectName = "lc_test",
	projectType = "binary"
})
add_includedirs("../ext/stb/")
-- add_files("test_dsl.cpp")
add_files("test_rtx.cpp", "../ext/stb/stb.c")
-- add_files("test_raster.cpp", "../ext/stb/stb.c")
-- add_files("test_dispatch_indirect.cpp")
add_deps("lc-dsl", "lc-runtime", "lc-vstl")
