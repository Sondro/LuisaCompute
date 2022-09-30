BuildProject({
    projectName = "luisa-compute-core",
    projectType = "shared",
    debugEvent = function()
        add_links("mimalloc-static-debug", "spdlogd", {
            public = true
        })
        if is_plat("windows") then
            add_links("Dbghelp", {
                public = true
            })
        end
    end,
    releaseEvent = function()
        add_links("mimalloc-static", "spdlog", {
            public = true
        })
    end,
    unityBuildBatch = 4
})
add_includedirs("../", "../ext/abseil-cpp/", "../ext/EASTL/include/", "../ext/json/include/",
    "../ext/EASTL/packages/EABase/include/Common", "../ext/fmt/include/", "../ext/spdlog/include/", "../ext/xxHash/",
    "../ext/stb/", {
        public = true
    })
add_files("**.cpp")
add_defines("UNICODE", "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS", "NOMINMAX", "EASTL_DLL=1", "ABSL_CONSUME_DLL=1",
    "FMT_CONSTEVAL=constexpr", "FMT_USE_CONSTEXPR=1", "FMT_HEADER_ONLY=1", "FMT_EXCEPTIONS=0", "SPDLOG_NO_EXCEPTIONS=1",
    "_ENABLE_EXTENDED_ALIGNED_STORAGE", {
        public = true
    })
add_defines("LC_CORE_EXPORT_DLL")
add_links("abseil_dll", "EASTL", "luisa-compute-ext", {
    public = true
})
