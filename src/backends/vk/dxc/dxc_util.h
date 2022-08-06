#pragma once
#include <vstl/Common.h>
#include <dxc/dxcapi.h>
#include <core/dynamic_module.h>
namespace toolhub::directx {
using Microsoft::WRL::ComPtr;
class DXByteBlob final : public vstd::IOperatorNewBase {
private:
	ComPtr<IDxcBlob> blob;
	ComPtr<IDxcResult> comRes;

public:
	DXByteBlob(
		ComPtr<IDxcBlob>&& b,
		ComPtr<IDxcResult>&& rr);
	~DXByteBlob();
	vbyte* GetBufferPtr() const;
	size_t GetBufferSize() const;
};
using CompileResult = vstd::variant<
	vstd::unique_ptr<DXByteBlob>,
	vstd::string>;
class DXShaderCompiler final : public vstd::IOperatorNewBase {
private:
	ComPtr<IDxcCompiler3> comp;
	std::optional<luisa::DynamicModule> dxcCompiler;

	CompileResult Compile(
		vstd::string_view code,
		vstd::span<LPCWSTR> args);

public:
	DXShaderCompiler();
	~DXShaderCompiler();
	CompileResult CompileCompute(
		vstd::string_view code,
		bool optimize,
		uint shaderModel = 63,
		bool isRTPipeline = false);

	/*CompileResult CompileRayTracing(
        vstd::string_view code,
        bool optimize,
        uint shaderModel = 63);*/
};
}// namespace toolhub::directx