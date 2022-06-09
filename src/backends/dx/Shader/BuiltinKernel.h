#pragma once
#include <Shader/ComputeShader.h>
#include <runtime/context.h>
using namespace luisa::compute;
namespace toolhub::directx {
class BuiltinKernel {
public:
    static ComputeShader *LoadAccelSetKernel(Device *device, Context const& ctx);
    static ComputeShader *LoadBC6TryModeG10CSKernel(Device *device, Context const& ctx);
    static ComputeShader *LoadBC6TryModeLE10CSKernel(Device *device, Context const& ctx);
    static ComputeShader *LoadBC6EncodeBlockCSKernel(Device *device, Context const& ctx);
    static ComputeShader *LoadBC7TryMode456CSKernel(Device *device, Context const& ctx);
    static ComputeShader *LoadBC7TryMode137CSKernel(Device *device, Context const& ctx);
    static ComputeShader *LoadBC7TryMode02CSKernel(Device *device, Context const& ctx);
    static ComputeShader *LoadBC7EncodeBlockCSKernel(Device *device, Context const& ctx);
};
}// namespace toolhub::directx