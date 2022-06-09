
#include <Shader/BuiltinKernel.h>
#include <compile/hlsl/dx_codegen.h>
#include <compile/hlsl/shader_header.h>
namespace toolhub::directx {
ComputeShader *BuiltinKernel::LoadAccelSetKernel(Device *device, Context const &ctx) {
    CodegenResult code;
    code.bdlsBufferCount = 0;
    code.result = vstd::string(ReadInternalHLSLFile("accel_process", ctx.data_directory()));
    code.properties.resize(3);
    code.md5 = vstd::MD5(code.result);
    auto &InstBuffer = code.properties[0];
    InstBuffer.first = "_InstBuffer"sv;
    InstBuffer.second.arrSize = 0;
    InstBuffer.second.registerIndex = 0;
    InstBuffer.second.spaceIndex = 0;
    InstBuffer.second.type = ShaderVariableType::RWStructuredBuffer;
    auto &Global = code.properties[1];
    Global.first = "_Global"sv;
    Global.second.arrSize = 0;
    Global.second.registerIndex = 0;
    Global.second.spaceIndex = 0;
    Global.second.type = ShaderVariableType::ConstantBuffer;
    auto &SetBuffer = code.properties[2];
    SetBuffer.first = "_SetBuffer"sv;
    SetBuffer.second.arrSize = 0;
    SetBuffer.second.registerIndex = 0;
    SetBuffer.second.spaceIndex = 0;
    SetBuffer.second.type = ShaderVariableType::StructuredBuffer;
    return ComputeShader::CompileCompute(
        device,
        code,
        uint3(64, 1, 1),
        60,
        ctx.cache_directory(),
        {
            "set_accel_kernel"sv,
        });
}
namespace detail {
static ComputeShader *LoadBCKernel(
    Device *device,
    vstd::string_view includeCode,
    vstd::string_view kernelCode,
    vstd::string &&codePath,
    Context const &ctx) {
    CodegenResult code;
    code.result.reserve(includeCode.size() + kernelCode.size());
    code.result << includeCode << kernelCode;
    code.bdlsBufferCount = 0;
    code.properties.resize(4);
    code.md5 = vstd::MD5(code.result);
    auto &globalBuffer = code.properties[0];
    globalBuffer.first = "_Global"sv;
    globalBuffer.second.arrSize = 0;
    globalBuffer.second.registerIndex = 0;
    globalBuffer.second.spaceIndex = 0;
    globalBuffer.second.type = ShaderVariableType::ConstantBuffer;

    auto &gInput = code.properties[1];
    gInput.first = "g_Input"sv;
    gInput.second.arrSize = 0;
    gInput.second.registerIndex = 0;
    gInput.second.spaceIndex = 0;
    gInput.second.type = ShaderVariableType::SRVDescriptorHeap;

    auto &gInBuff = code.properties[2];
    gInBuff.first = "g_InBuff"sv;
    gInBuff.second.arrSize = 0;
    gInBuff.second.registerIndex = 1;
    gInBuff.second.spaceIndex = 0;
    gInBuff.second.type = ShaderVariableType::StructuredBuffer;

    auto &gOutBuff = code.properties[3];
    gOutBuff.first = "g_OutBuff"sv;
    gOutBuff.second.arrSize = 0;
    gOutBuff.second.registerIndex = 0;
    gOutBuff.second.spaceIndex = 0;
    gOutBuff.second.type = ShaderVariableType::RWStructuredBuffer;
    return ComputeShader::CompileCompute(
        device,
        code,
        uint3(1, 1, 1),
        60,
        ctx.cache_directory(), {std::move(codePath)});
}
static vstd::string const &Bc6Header(Context const& ctx) {
    static vstd::string bc6Header = ReadInternalHLSLFile("bc6_header", ctx.data_directory());
    return bc6Header;
}
static vstd::string const &Bc7Header(Context const& ctx) {
    static vstd::string bc7Header = ReadInternalHLSLFile("bc7_header", ctx.data_directory());
}

static vstd::string bc7Header;
}// namespace detail
ComputeShader *BuiltinKernel::LoadBC6TryModeG10CSKernel(Device *device, Context const &ctx) {
    return detail::LoadBCKernel(
        device,
        detail::Bc6Header(ctx),
        ReadInternalHLSLFile("bc6_trymode_g10cs", ctx.data_directory()),
        vstd::string("bc6_trymodeg10"sv),
        ctx);
}
ComputeShader *BuiltinKernel::LoadBC6TryModeLE10CSKernel(Device *device, Context const &ctx) {
    return detail::LoadBCKernel(
        device,
        detail::Bc6Header(ctx),
        ReadInternalHLSLFile("bc6_trymode_le10cs", ctx.data_directory()),
        vstd::string("bc6_trymodele10"sv),
        ctx);
}
ComputeShader *BuiltinKernel::LoadBC6EncodeBlockCSKernel(Device *device, Context const &ctx) {
    return detail::LoadBCKernel(
        device,
        detail::Bc6Header(ctx),
        ReadInternalHLSLFile("bc6_encode_block", ctx.data_directory()),
        vstd::string("bc6_encodeblock"sv),
        ctx);
}
ComputeShader *BuiltinKernel::LoadBC7TryMode456CSKernel(Device *device, Context const &ctx) {
    return detail::LoadBCKernel(
        device,
        detail::Bc7Header(ctx),
        ReadInternalHLSLFile("bc7_trymode_456cs", ctx.data_directory()),
        vstd::string("bc7_trymode456"sv),
        ctx);
}
ComputeShader *BuiltinKernel::LoadBC7TryMode137CSKernel(Device *device, Context const &ctx) {
    return detail::LoadBCKernel(
        device,
        detail::Bc7Header(ctx),
        ReadInternalHLSLFile("bc7_trymode_137cs", ctx.data_directory()),
        vstd::string("bc7_trymode137"sv),
        ctx);
}
ComputeShader *BuiltinKernel::LoadBC7TryMode02CSKernel(Device *device, Context const &ctx) {
    return detail::LoadBCKernel(
        device,
        detail::Bc7Header(ctx),
        ReadInternalHLSLFile("bc7_trymode_02cs", ctx.data_directory()),
        vstd::string("bc7_trymode02"sv),
        ctx);
}
ComputeShader *BuiltinKernel::LoadBC7EncodeBlockCSKernel(Device *device, Context const &ctx) {
    return detail::LoadBCKernel(
        device,
        detail::Bc7Header(ctx),
        ReadInternalHLSLFile("bc7_encode_block", ctx.data_directory()),
        vstd::string("bc7_encodeblock"sv),
        ctx);
}
}// namespace toolhub::directx