BuildProject({
	projectName = "luisa-compute-vstl",
	projectType = "shared",
	unityBuildBatch = 4
})
add_deps("luisa-compute-core")
add_files("**.cpp")
add_defines("LC_VSTL_EXPORT_DLL")
if is_plat("windows") then
	add_links("Ole32")
end
