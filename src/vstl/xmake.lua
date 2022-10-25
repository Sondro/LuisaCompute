BuildProject({
	projectName = "lc-vstl",
	projectType = "shared",
	unityBuildBatch = 4
})
add_deps("lc-core")
add_files("**.cpp")
add_defines("LC_VSTL_EXPORT_DLL")
if is_plat("windows") then
	add_syslinks("Ole32")
end
