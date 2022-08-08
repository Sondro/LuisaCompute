#include <dxc/dxc_util.h>
#include <vstl/small_vector.h>
namespace toolhub::directx {
inline void ThrowIfFailed(HRESULT result) {
#ifdef DEBUG
	if (!SUCCEEDED(result)) {
		VEngine_Log("DXC Failed!");
		VENGINE_EXIT;
	}
#endif
}
DXByteBlob::~DXByteBlob() {}

DXByteBlob::DXByteBlob(
	ComPtr<IDxcBlob>&& b,
	ComPtr<IDxcResult>&& rr)
	: blob(std::move(b)),
	  comRes(std::move(rr)) {}
vbyte* DXByteBlob::GetBufferPtr() const {
	return reinterpret_cast<vbyte*>(blob->GetBufferPointer());
}
size_t DXByteBlob::GetBufferSize() const {
	return blob->GetBufferSize();
}
static vstd::wstring GetSM(uint shaderModel) {
	vstd::string smStr;
	smStr << vstd::to_string(shaderModel / 10) << '_' << vstd::to_string(shaderModel % 10);
	vstd::wstring wstr;
	wstr.resize(smStr.size());
	for (auto i : vstd::range(smStr.size())) {
		wstr[i] = smStr[i];
	}
	return wstr;
}
DXShaderCompiler::~DXShaderCompiler() {
	comp = nullptr;
}
DXShaderCompiler::DXShaderCompiler() {
	dxcCompiler = luisa::DynamicModule::load("dxcompiler");
	if (dxcCompiler.has_value()) {
		auto voidPtr = dxcCompiler->address("DxcCreateInstance"sv);
		HRESULT(__stdcall * DxcCreateInstance)
		(const IID&, const IID&, LPVOID*) =
			reinterpret_cast<HRESULT(__stdcall*)(const IID&, const IID&, LPVOID*)>(voidPtr);
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(comp.GetAddressOf()));
	} else {
		comp = nullptr;
	}
}
CompileResult DXShaderCompiler::Compile(
	vstd::string_view code,
	vstd::span<LPCWSTR> args) {
	DxcBuffer buffer{
		code.data(),
		code.size(),
		CP_UTF8};
	ComPtr<IDxcResult> compileResult;

	ThrowIfFailed(comp->Compile(
		&buffer,
		args.data(),
		args.size(),
		nullptr,
		IID_PPV_ARGS(compileResult.GetAddressOf())));
	HRESULT status;
	ThrowIfFailed(compileResult->GetStatus(&status));
	if (status == 0) {
		ComPtr<IDxcBlob> resultBlob;
		ThrowIfFailed(compileResult->GetResult(resultBlob.GetAddressOf()));
		return vstd::unique_ptr<DXByteBlob>(new DXByteBlob(std::move(resultBlob), std::move(compileResult)));
	} else {
		ComPtr<IDxcBlobEncoding> errBuffer;
		ThrowIfFailed(compileResult->GetErrorBuffer(errBuffer.GetAddressOf()));
		auto errStr = vstd::string_view(
			reinterpret_cast<char const*>(errBuffer->GetBufferPointer()),
			errBuffer->GetBufferSize());
		return vstd::string(errStr);
	}
}

CompileResult DXShaderCompiler::CompileCompute(
	vstd::string_view code,
	bool optimize,
	uint shaderModel,
	bool isRTPipeline) {
	if (shaderModel < 10) {
		return "Illegal shader model!"sv;
	}
	vstd::vector<LPCWSTR, VEngine_AllocType::VEngine, 32> args;
	vstd::wstring smStr;
	smStr << (isRTPipeline ? L"lib_" : L"cs_") << GetSM(shaderModel);
	args.push_back(L"/T");
	args.push_back(smStr.c_str());
	args.push_back_all(
		{L"/Qstrip_rootsignature",
		 L"/quiet",
		 L"/q",
		 L"-HV 2021",
		 L"-spirv",
		 L"-fspv-extension=SPV_KHR_ray_tracing",
		 L"-fspv-extension=SPV_KHR_ray_query",
		 L"-fspv-target-env=vulkan1.3"});
	if (optimize) {
		args.push_back(L"/O3");
	}
	return Compile(code, args);
}
/*
CompileResult DXShaderCompiler::CompileRayTracing(
    vstd::string_view code,
    bool optimize,
    uint shaderModel) {
    if (shaderModel < 10) {
        return "Illegal shader model!"sv;
    }
    vstd::vector<LPCWSTR, VEngine_AllocType::VEngine, 32> args;
    vstd::wstring smStr;
    smStr << L"lib_" << GetSM(shaderModel);
    args.push_back(L"/T");
    args.push_back(smStr.c_str());
    args.push_back_all(
        {L"-Qstrip_debug",
         L"-Qstrip_reflect",
         L"/enable_unbounded_descriptor_tables",
         L"-HV 2021"});
    if (optimize) {
        args.push_back(L"/O3");
    }
    return Compile(code, args);
}*/

}// namespace toolhub::directx