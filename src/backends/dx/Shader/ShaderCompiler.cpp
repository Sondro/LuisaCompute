#include <Shader/ShaderCompiler.h>
#include <core/dynamic_module.h>
#include <vstl/StringUtility.h>
#ifdef DEBUG
#include <vstl/BinaryReader.h>
#endif
namespace toolhub::directx {
DXByteBlob::DXByteBlob(
    ComPtr<IDxcBlob> &&b,
    ComPtr<IDxcResult> &&rr)
    : blob(std::move(b)),
      comRes(std::move(rr)) {}
std::byte *DXByteBlob::GetBufferPtr() const {
    return reinterpret_cast<std::byte *>(blob->GetBufferPointer());
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
ShaderCompiler::~ShaderCompiler() {
    comp = nullptr;
}
ShaderCompiler::ShaderCompiler() {
    dxcCompiler = luisa::DynamicModule::load("dxcompiler");
    if (dxcCompiler.has_value()) {
        luisa::DynamicModule &m = dxcCompiler.value();
        auto voidPtr = m.address("DxcCreateInstance"sv);
        HRESULT(__stdcall * DxcCreateInstance)
        (const IID &, const IID &, LPVOID *) =
            reinterpret_cast<HRESULT(__stdcall *)(const IID &, const IID &, LPVOID *)>(voidPtr);
        DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(comp.GetAddressOf()));
    } else {
        comp = nullptr;
    }
}
CompileResult ShaderCompiler::Compile(
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
        return vstd::create_unique(new DXByteBlob(std::move(resultBlob), std::move(compileResult)));
    } else {
        ComPtr<IDxcBlobEncoding> errBuffer;
        ThrowIfFailed(compileResult->GetErrorBuffer(errBuffer.GetAddressOf()));
        auto errStr = vstd::string_view(
            reinterpret_cast<char const *>(errBuffer->GetBufferPointer()),
            errBuffer->GetBufferSize());
        return vstd::string(errStr);
    }
}

CompileResult ShaderCompiler::CompileCompute(
    vstd::string_view code,
    bool optimize,
    uint shaderModel) {
    if (shaderModel < 10) {
        return "Illegal shader model!"_sv;
    }
    vstd::vector<LPCWSTR, VEngine_AllocType::VEngine, 32> args;
    vstd::wstring smStr;
    smStr << L"cs_" << GetSM(shaderModel);
    args.push_back(L"/T");
    args.push_back(smStr.c_str());
    args.push_back_all(
        {L"/Qstrip_debug",
         L"/Qstrip_reflect",
         L"/Qstrip_priv",
         L"/enable_unbounded_descriptor_tables",
         L"/Qstrip_rootsignature",
         L"/Gfa",
         L"/all-resources-bound",
         L"/quiet",
         L"/q",
         L"/debug_level=none",
         L"-HV 2021"});
    if (optimize) {
        args.push_back(L"/O3");
    }
    return Compile(code, args);
}
#ifdef SHADER_COMPILER_TEST

CompileResult ShaderCompiler::CustomCompile(
    vstd::string_view code,
    vstd::span<vstd::string const> args) {
    vstd::vector<vstd::wstring> wstrArgs;
    wstrArgs.push_back_func(
        args.size(),
        [&](size_t i) {
            auto &&strv = args[i];
            vstd::wstring wstr;
            wstr.resize(strv.size());
            for (auto ite : vstd::range(strv.size())) {
                wstr[ite] = strv[ite];
            }
            return wstr;
        });
    vstd::vector<LPCWSTR> argPtr;
    argPtr.push_back_func(
        args.size(),
        [&](size_t i) {
            return wstrArgs[i].data();
        });

    return Compile(code, argPtr);
}

extern "C" __declspec(dllexport) void ConsoleTest() {
    ShaderPaths paths;
    auto &&c = *Device::Compiler();
    vstd::string fileName = "shader.hlsl";
    if (fileName == "exit") return;
    BinaryReader read(fileName);
    if (!read) {
        std::cout << "illegal file path!\n";
    }
    auto vec = read.ReadToString();
    //TODO: read file
    vstd::vector<vstd::string> args;
    args.reserve(32);
    {
        BinaryReader reader("compile_args.txt");
        if (reader) {
            auto argsStr = reader.ReadToString();
            auto lines = vstd::StringUtil::Split(argsStr, "\r\n");
            for (auto &&i : lines) {
                args.emplace_back(i);
            }
        }
    }
    auto size = args.size();
    args.emplace_back("/Tvs_6_5");
    args.emplace_back("/DVS");
    {
        auto compileResult = c.CustomCompile(vec, args);
        if (compileResult.index() == 1) {
            std::cout << compileResult.get<1>() << '\n';
        }
    }
    args.resize(size);
    args.emplace_back("/Tps_6_5");
    args.emplace_back("/DPS");
    {
        auto compileResult = c.CustomCompile(vec, args);
        if (compileResult.index() == 1) {
            std::cout << compileResult.get<1>() << '\n';
        }
    }
}
#endif
/*
CompileResult ShaderCompiler::CompileRayTracing(
    vstd::string_view code,
    bool optimize,
    uint shaderModel) {
    if (shaderModel < 10) {
        return "Illegal shader model!"_sv;
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