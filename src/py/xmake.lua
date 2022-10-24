rule("copy")
after_build(function(target)
	local bdPath = target:targetdir()
	os.cp(bdPath .. "/lcapi.dll", bdPath .. "/lcapi.pyd")
end)

BuildProject({
	projectName = "lcapi",
	projectType = "shared"
})
add_links("python3", "python310")
add_linkdirs("../ext/python/libs")
add_includedirs("../ext/pybind11/include", "../ext/python/include")
add_files("*.cpp")
add_deps("luisa-compute-runtime", "luisa-compute-dsl", "nanobind")
add_rules("copy")
