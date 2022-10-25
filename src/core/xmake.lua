BuildProject({
	projectName = "lc-core",
	projectType = "shared",
	debugEvent = function()
		if is_plat("windows") then
			add_syslinks("Dbghelp", {
				public = true
			})
		end
	end
})
add_deps("EASTL", "abseil_dll", "spdlog")
add_includedirs("../", "../ext/xxHash/", {
	public = true
})
add_files("**.cpp")
add_defines("UNICODE", "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS", "_ENABLE_EXTENDED_ALIGNED_STORAGE", {
	public = true
})
add_defines("LC_CORE_EXPORT_DLL")