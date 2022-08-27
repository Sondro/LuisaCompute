#pragma once
#include <core/dynamic_module.h>
#include <DXRuntime/Device.h>
#include <dxc/dxcapi.h>
namespace toolhub::directx {

class DXByteBlob final : public vstd::IOperatorNewBase {
private:
	ComPtr<IDxcBlob> blob;
	ComPtr<IDxcResult> comRes;

public:
	DXByteBlob(
		ComPtr<IDxcBlob>&& b,
		ComPtr<IDxcResult>&& rr);
	vbyte* GetBufferPtr() const;
	size_t GetBufferSize() const;
};
using CompileResult = vstd::variant<
	vstd::unique_ptr<DXByteBlob>,
	vstd::string>;
class ShaderCompiler final : public vstd::IOperatorNewBase {
private:
	ComPtr<IDxcCompiler3> comp;
	luisa::optional<luisa::DynamicModule> dxcCompiler;
	CompileResult Compile(
		vstd::string_view code,
		vstd::span<LPCWSTR> args);

public:
	ShaderCompiler();
	~ShaderCompiler();
	CompileResult CompileCompute(
		vstd::string_view code,
		bool optimize,
		uint shaderModel = 63);

	CompileResult CustomCompile(
		vstd::string_view code,
		vstd::span<vstd::string const> args);

	/*CompileResult CompileRayTracing(
        vstd::string_view code,
        bool optimize,
        uint shaderModel = 63);*/
};
}// namespace toolhub::directx