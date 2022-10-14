#pragma once
#include <d3dx12.h>
#include <Shader/Shader.h>
#include <core/binary_io_visitor.h>
namespace toolhub::directx {
class ComputeShader;
class RTShader;
struct ShaderBuildData {
    vstd::vector<std::byte> binData;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
};

class ShaderSerializer {
    static size_t SerializeRootSig(
        vstd::span<Property const> properties,
        vstd::vector<std::byte> &result);
    static ComPtr<ID3D12RootSignature> DeSerializeRootSig(
        ID3D12Device *device,
        vstd::span<std::byte const> bytes);

public:
    static ComPtr<ID3DBlob> SerializeRootSig(
        vstd::span<Property const> properties);
    static vstd::vector<std::byte>
    Serialize(
        vstd::span<Property const> properties,
        vstd::span<SavedArgument const> kernelArgs,
        vstd::span<std::byte const> binByte,
        vstd::MD5 const &checkMD5,
        uint bindlessCount,
        uint3 blockSize);
    static ComputeShader *DeSerialize(
        luisa::string_view fileName,
        bool byteCodeIsCache,
        Device *device,
        BinaryIOVisitor &streamFunc,
        vstd::optional<vstd::MD5> const &checkMD5,
        bool &clearCache,
        vstd::MD5 *lastMD5 = nullptr);
    static bool CheckMD5(
        vstd::string_view fileName,
        vstd::MD5 const &checkMD5,
        BinaryIOVisitor &streamFunc);
    static vstd::vector<SavedArgument> SerializeKernel(Function kernel);
    ShaderSerializer() = delete;
    ~ShaderSerializer() = delete;
};
}// namespace toolhub::directx