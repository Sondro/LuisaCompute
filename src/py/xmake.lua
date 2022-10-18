BuildProject({
	projectName = "luisa-compute-py",
	projectType = "shared",
	debugEvent = function()
		add_links("python3_d", "python39_d")
	end,
	releaseEvent = function()
		add_links("python3", "python39")
	end
})
add_linkdirs("../ext/python/libs")
add_includedirs("../ext/pybind11/include", "../ext/python/include")
add_files("*.cpp")
add_deps("luisa-compute-runtime", "luisa-compute-dsl")
