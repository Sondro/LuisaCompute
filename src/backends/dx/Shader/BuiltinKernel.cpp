
#include <Shader/BuiltinKernel.h>
#include <compile/hlsl/dx_codegen.h>
#include <compile/hlsl/shader_header.h>
namespace toolhub::directx {
ComputeShader* BuiltinKernel::LoadAccelSetKernel(Device* device, ShaderPaths const& ctx) {
	auto func = [&] {
		CodegenResult code;
		code.bdlsBufferCount = 0;
		code.result = ReadInternalHLSLFile("accel_process", ctx.dataFolder);
		code.properties.resize(3);
		auto& Global = code.properties[0];
		Global.arrSize = 0;
		Global.registerIndex = 0;
		Global.spaceIndex = 0;
		Global.type = ShaderVariableType::ConstantBuffer;
		auto& SetBuffer = code.properties[1];
		SetBuffer.arrSize = 0;
		SetBuffer.registerIndex = 0;
		SetBuffer.spaceIndex = 0;
		SetBuffer.type = ShaderVariableType::StructuredBuffer;
		auto& InstBuffer = code.properties[2];
		InstBuffer.arrSize = 0;
		InstBuffer.registerIndex = 0;
		InstBuffer.spaceIndex = 0;
		InstBuffer.type = ShaderVariableType::RWStructuredBuffer;
		return code;
	};
	return ComputeShader::CompileCompute(
		device,
		{},
		func,
		uint3(64, 1, 1),
		60,
		ctx.shaderCacheFolder,
		ctx.psoFolder,
		"set_accel_kernel"sv, true);
}
namespace detail {
static ComputeShader* LoadBCKernel(
	Device* device,
	vstd::function<vstd::string_view()> const& includeCode,
	vstd::function<vstd::string_view()> const& kernelCode,
	vstd::string&& codePath,
	ShaderPaths const& ctx) {
	auto func = [&] {
		CodegenResult code;
		auto incCode = includeCode();
		auto kerCode = kernelCode();
		code.result.reserve(incCode.size() + kerCode.size());
		code.result << incCode << kerCode;
		code.bdlsBufferCount = 0;
		code.properties.resize(4);
		auto& globalBuffer = code.properties[0];
		globalBuffer.arrSize = 0;
		globalBuffer.registerIndex = 0;
		globalBuffer.spaceIndex = 0;
		globalBuffer.type = ShaderVariableType::ConstantBuffer;

		auto& gInput = code.properties[1];
		gInput.arrSize = 0;
		gInput.registerIndex = 0;
		gInput.spaceIndex = 0;
		gInput.type = ShaderVariableType::SRVDescriptorHeap;

		auto& gInBuff = code.properties[2];
		gInBuff.arrSize = 0;
		gInBuff.registerIndex = 1;
		gInBuff.spaceIndex = 0;
		gInBuff.type = ShaderVariableType::StructuredBuffer;

		auto& gOutBuff = code.properties[3];
		gOutBuff.arrSize = 0;
		gOutBuff.registerIndex = 0;
		gOutBuff.spaceIndex = 0;
		gOutBuff.type = ShaderVariableType::RWStructuredBuffer;
		return code;
	};
	return ComputeShader::CompileCompute(
		device,
		{},
		func,
		uint3(1, 1, 1),
		60,
		ctx.shaderCacheFolder,
		ctx.psoFolder,
		codePath, true);
}
static vstd::string const& Bc6Header(ShaderPaths const& ctx) {
	static vstd::string bc6Header = ReadInternalHLSLFile("bc6_header", ctx.dataFolder);
	return bc6Header;
}
static vstd::string const& Bc7Header(ShaderPaths const& ctx) {
	static vstd::string bc7Header = ReadInternalHLSLFile("bc7_header", ctx.dataFolder);
	return bc7Header;
}

static vstd::string bc7Header;
}// namespace detail
ComputeShader* BuiltinKernel::LoadBC6TryModeG10CSKernel(Device* device, ShaderPaths const& ctx) {
	return detail::LoadBCKernel(
		device,
		[&] { return detail::Bc6Header(ctx); },
		[&] { return ReadInternalHLSLFile("bc6_trymode_g10cs", ctx.dataFolder); },
		vstd::string("bc6_trymodeg10"sv),
		ctx);
}
ComputeShader* BuiltinKernel::LoadBC6TryModeLE10CSKernel(Device* device, ShaderPaths const& ctx) {
	return detail::LoadBCKernel(
		device,
		[&] { return detail::Bc6Header(ctx); },
		[&] { return ReadInternalHLSLFile("bc6_trymode_le10cs", ctx.dataFolder); },
		vstd::string("bc6_trymodele10"sv),
		ctx);
}
ComputeShader* BuiltinKernel::LoadBC6EncodeBlockCSKernel(Device* device, ShaderPaths const& ctx) {
	return detail::LoadBCKernel(
		device,
		[&] { return detail::Bc6Header(ctx); },
		[&] { return ReadInternalHLSLFile("bc6_encode_block", ctx.dataFolder); },
		vstd::string("bc6_encodeblock"sv),
		ctx);
}
ComputeShader* BuiltinKernel::LoadBC7TryMode456CSKernel(Device* device, ShaderPaths const& ctx) {
	return detail::LoadBCKernel(
		device,
		[&] { return detail::Bc7Header(ctx); },
		[&] { return ReadInternalHLSLFile("bc7_trymode_456cs", ctx.dataFolder); },
		vstd::string("bc7_trymode456"sv),
		ctx);
}
ComputeShader* BuiltinKernel::LoadBC7TryMode137CSKernel(Device* device, ShaderPaths const& ctx) {
	return detail::LoadBCKernel(
		device,
		[&] { return detail::Bc7Header(ctx); },
		[&] { return ReadInternalHLSLFile("bc7_trymode_137cs", ctx.dataFolder); },
		vstd::string("bc7_trymode137"sv),
		ctx);
}
ComputeShader* BuiltinKernel::LoadBC7TryMode02CSKernel(Device* device, ShaderPaths const& ctx) {
	return detail::LoadBCKernel(
		device,
		[&] { return detail::Bc7Header(ctx); },
		[&] { return ReadInternalHLSLFile("bc7_trymode_02cs", ctx.dataFolder); },
		vstd::string("bc7_trymode02"sv),
		ctx);
}
ComputeShader* BuiltinKernel::LoadBC7EncodeBlockCSKernel(Device* device, ShaderPaths const& ctx) {
	return detail::LoadBCKernel(
		device,
		[&] { return detail::Bc7Header(ctx); },
		[&] { return ReadInternalHLSLFile("bc7_encode_block", ctx.dataFolder); },
		vstd::string("bc7_encodeblock"sv),
		ctx);
}
}// namespace toolhub::directx