#pragma once
#include <d3dx12.h>
#include <Shader/Shader.h>
#include <core/binary_io_visitor.h>
namespace toolhub::directx {
class ComputeShader;
class RasterShader;
class RTShader;
struct ShaderBuildData {
    vstd::vector<std::byte> binData;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
};
struct RasterHeaderData {
    D3D12_BLEND_DESC blendState;
    D3D12_RASTERIZER_DESC rasterizerState;
    D3D12_DEPTH_STENCIL_DESC depthStencilState;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType;
    uint numRtv;
    uint inputLayoutCount;
    DXGI_FORMAT RTVFormats[8];
    DXGI_FORMAT DSVFormat;
};
class ShaderSerializer {
    static size_t SerializeRootSig(
        vstd::span<Property const> properties,
        vstd::vector<std::byte> &result,
        bool isRasterShader);
    static ComPtr<ID3D12RootSignature> DeSerializeRootSig(
        ID3D12Device *device,
        vstd::span<std::byte const> bytes);

public:
    static ComPtr<ID3DBlob> SerializeRootSig(vstd::span<Property const> properties, bool isRasterShader);
    static vstd::vector<std::byte> Serialize(
        vstd::span<Property const> properties,
        vstd::span<SavedArgument const> kernelArgs,
        vstd::span<std::byte const> binByte,
        vstd::MD5 const &checkMD5,
        uint bindlessCount,
        uint3 blockSize);
    static vstd::vector<std::byte> RasterSerialize(
        vstd::span<Property const> properties,
        vstd::span<SavedArgument const> kernelArgs,
        vstd::span<std::byte const> vertBin,
        vstd::span<std::byte const> pixelBin,
        vstd::MD5 const &checkMD5,
        uint bindlessCount,
        RasterHeaderData const &headerData,
        D3D12_INPUT_ELEMENT_DESC const *inputLayouts);
    static ComputeShader *DeSerialize(
        luisa::string_view fileName,
        bool byteCodeIsCache,
        Device *device,
        BinaryIOVisitor &streamFunc,
        vstd::optional<vstd::MD5> const &checkMD5,
        bool &clearCache,
        vstd::MD5 *lastMD5 = nullptr);
    static RasterShader *RasterDeSerialize(
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
    static vstd::vector<SavedArgument> SerializeKernel(
        vstd::IRange<std::pair<Variable, Usage>> &arguments);
    ShaderSerializer() = delete;
    ~ShaderSerializer() = delete;
};
}// namespace toolhub::directx