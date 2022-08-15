#pragma once
#include <Shader/ComputeShader.h>
#include <DXRuntime/ShaderPaths.h>
namespace toolhub::directx {
class BuiltinKernel {
public:
    static ComputeShader *LoadAccelSetKernel(Device *device, ShaderPaths const& ctx);
    static ComputeShader *LoadBC6TryModeG10CSKernel(Device *device, ShaderPaths const& ctx);
    static ComputeShader *LoadBC6TryModeLE10CSKernel(Device *device, ShaderPaths const& ctx);
    static ComputeShader *LoadBC6EncodeBlockCSKernel(Device *device, ShaderPaths const& ctx);
    static ComputeShader *LoadBC7TryMode456CSKernel(Device *device, ShaderPaths const& ctx);
    static ComputeShader *LoadBC7TryMode137CSKernel(Device *device, ShaderPaths const& ctx);
    static ComputeShader *LoadBC7TryMode02CSKernel(Device *device, ShaderPaths const& ctx);
    static ComputeShader *LoadBC7EncodeBlockCSKernel(Device *device, ShaderPaths const& ctx);
};
}// namespace toolhub::directx