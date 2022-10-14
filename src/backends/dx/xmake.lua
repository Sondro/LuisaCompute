BuildProject({
	projectName = "luisa-compute-backend-dx",
	projectType = "shared",
	unityBuildBatch = 8,
	debugEvent = function()
		add_defines("SHADER_COMPILER_TEST")
	end
})
add_deps("luisa-compute-runtime", "luisa-compute-vstl")
add_files("Api/**.cpp", "DXRuntime/**.cpp", "Resource/**.cpp", "Shader/**.cpp", "HLSL/**.cpp")
add_includedirs("./")
add_links("D3D12", "dxgi")
after_build(function(target)
	local binDir = nil
	if is_mode("release") then
		binDir = "bin/release/"
	else
		binDir = "bin/debug/"
	end
	os.cp("src/backends/dx/dx_builtin", binDir .. ".data/")
	os.cp("src/backends/dx/dx_support/*.dll", binDir)
end)
