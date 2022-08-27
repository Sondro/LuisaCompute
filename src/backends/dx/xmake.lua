set_policy("build.ccache", false)
add_rules("mode.release", "mode.debug")
function GetValue(funcOrValue)
    if type(funcOrValue) == 'function' then
        return funcOrValue()
    else
        return funcOrValue
    end
end

function SetException(enableException)
    local value = GetValue(enableException)
    if (value ~= nil) and value then
        add_cxflags("/EHsc", {
            force = true
        })
    end
end
function BuildProject(config)
    local projectName = GetValue(config.projectName)
    if projectName == nil then
        return
    end
    target(projectName)
    set_languages("clatest")
    set_languages("cxx20")
    local projectType = GetValue(config.projectType)
    if projectType ~= nil then
        set_kind(projectType)
    end
    local unityBuildBatch = GetValue(config.unityBuildBatch)
    if (unityBuildBatch == nil) then
        unityBuildBatch = 1;
    end
    add_rules("c.unity_build", {
        batchsize = unityBuildBatch
    })
    add_rules("c++.unity_build", {
        batchsize = unityBuildBatch
    })
    SetException(config.exception)
    if is_mode("release") then
        set_optimize("aggressive")
        if is_plat("windows") then
            set_runtimes("MD")
        end
        add_cxflags("/Zi", "/W0", "/Ob0", "/Oi", "/Ot", "/Oy", "/GT", "/GF", "/GS-", "/Gy", "/arch:AVX2", "/Gd",
            "/sdl-", "/GL", "/Zc:preprocessor", "/TP", {
                force = true
            })
        local event = GetValue(config.releaseEvent)
        if (type(event) == "function") then
            event()
        end
    else
        set_optimize("none")
        if is_plat("windows") then
            set_runtimes("MDd")
        end
        add_cxflags("/Zi", "/W0", "/Ob0", "/Oy-", "/GF", "/GS", "/arch:AVX2", "/TP", "/Gd", "/Zc:preprocessor", {
            force = true
        })
        local event = GetValue(config.debugEvent)
        if (type(event) == "function") then
            event()
        end
    end
    local event = GetValue(config.event)
    if (type(event) == "function") then
        event()
    end
end

rule("copy_to_build")
after_build(function(target)
    if is_mode("release") then
        os.cp("bin/release/*.dll", "$(buildir)/windows/x64/release/")
    else
        os.cp("bin/debug/*.dll", "$(buildir)/windows/x64/debug/")
    end
end)

rule("copy_dll")
after_build(function(target)
    local build_path = nil
    if is_mode("release") then
        build_path = "$(buildir)/windows/x64/release/"
        os.cp("../../../out/build/x64-Release/bin/*.dll", build_path)
    else
        build_path = "$(buildir)/windows/x64/debug/"
        os.cp("../../../out/build/x64-Debug/bin/*.dll", build_path)
    end
    os.cp("../../compile/hlsl/internal_text/*", build_path .. ".data/")
end)
IncludePath = {"./", "../../", "../../ext/abseil-cpp/", "../../ext/EASTL/include/", "../../ext/json/include/",
               "../../ext/EASTL/packages/EABase/include/Common", "../../ext/fmt/include/", "../../ext/spdlog/include/",
               "../../ext/xxHash/", "../../ext/stb/"}
Defines = {"VENGINE_DIRECTX_PROJECT", "UNICODE", "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS", "NOMINMAX", "EASTL_DLL=1",
           "ABSL_CONSUME_DLL=1", "FMT_CONSTEVAL=constexpr", "FMT_USE_CONSTEXPR=1", "FMT_HEADER_ONLY=1",
           "_ENABLE_EXTENDED_ALIGNED_STORAGE"}

BuildProject({
    projectName = "luisa-compute-backend-dx",
    projectType = "shared",
    event = function()
        add_files("Api/**.cpp", "DXRuntime/**.cpp", "Resource/**.cpp", "serialize/**.cpp", "Shader/**.cpp")
        add_defines(Defines)
        add_includedirs(IncludePath)
        add_rules("copy_dll", "copy_to_build")
        add_links("D3D12", "dxgi")
    end,
    debugEvent = function()
        add_links("../../../out/build/x64-Debug/lib/*", "lib/debug/*")
        add_defines("_DEBUG", "DEBUG")
    end,
    releaseEvent = function()
        add_links("../../../out/build/x64-Release/lib/*", "lib/release/*")
        add_defines("NDEBUG")
    end,
    exception = true,
    unityBuildBatch = 4
})
BuildProject({
    projectName = "lc_test",
    projectType = "shared",
    event = function()
        add_files("build/test/test_dsl.cpp")
        add_defines(Defines)
        add_includedirs(IncludePath)
    end,
    debugEvent = function()
        add_links("../../../out/build/x64-Debug/lib/*", "lib/debug/*")
        add_defines("_DEBUG", "DEBUG")
    end,
    releaseEvent = function()
        add_links("../../../out/build/x64-Release/lib/*", "lib/release/*")
        add_defines("NDEBUG")
    end,
    exception = true
})
