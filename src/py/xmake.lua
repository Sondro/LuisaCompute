BuildProject({
	projectName = "lcapi",
	projectType = "shared"
})
add_links("python3", "python310")
add_linkdirs("../ext/python/libs")
add_includedirs("../ext/pybind11/include", "../ext/python/include")
add_files("*.cpp")
add_deps("lc-runtime", "lc-dsl")
after_build(function(target)
	local bdPath = target:targetdir()
	os.cp(bdPath .. "/lcapi.dll", bdPath .. "/lcapi.pyd")
end)