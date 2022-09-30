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
        add_cxflags("/EHsc", "-fexceptions")
    else
        add_cxflags("-fno-exceptions")
    end
end
--[[
    BuildProject({
        projectName = xxx,
        projectType = xxx,
        unityBuildBatch = n,
        exception = true/false,
        releaseEvent = function()
        end,
        debugEvent = function()
        end
    })
]]

if is_mode("debug") then
    set_targetdir("bin/debug")
    add_linkdirs("lib/debug")
else
    set_targetdir("bin/release")
    add_linkdirs("lib/release")
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
    if (unityBuildBatch ~= nil) and (unityBuildBatch > 1) then
        add_rules("c.unity_build", {
            batchsize = unityBuildBatch
        })
        add_rules("c++.unity_build", {
            batchsize = unityBuildBatch
        })
    end
    SetException(config.exception)
    set_warnings("none")
    add_vectorexts("avx", "avx2")
    if is_mode("debug") then
        set_optimize("none")
        if is_plat("windows") then
            set_runtimes("MDd")
        end
        add_cxflags("/GS", "/Gd")
        local event = GetValue(config.debugEvent)
        if (type(event) == "function") then
            event()
        end
    else
        set_optimize("fastest")
        if is_plat("windows") then
            set_runtimes("MD")
        end
        add_cxflags("/Oy", "/GS-", "/Gd", "/Oi", "/Ot", "/GT")
        local event = GetValue(config.releaseEvent)
        if (type(event) == "function") then
            event()
        end
    end
end
add_subdirs("src")