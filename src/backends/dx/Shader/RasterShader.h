#pragma once
#include <Shader/Shader.h>
#include <raster/raster_state.h>
#include <runtime/pixel.h>
#include <core/binary_io_visitor.h>
namespace toolhub::directx {
struct CodegenResult;
class ShaderSerializer;
class RasterShader final : public Shader {
    friend class ShaderSerializer;

private:
    ComPtr<ID3D12PipelineState> pso;
    Device *device;
    RasterShader(
        Device *device,
        vstd::vector<Property> &&prop,
        vstd::vector<SavedArgument> &&args,
        ComPtr<ID3D12RootSignature> &&rootSig,
        ComPtr<ID3D12PipelineState> &&pso);

public:
    ID3D12PipelineState *Pso() const { return pso.Get(); }
    Tag GetTag() const noexcept override { return Tag::RasterShader; }
    static vstd::MD5 GenMD5(
        vstd::MD5 const &codeMD5,
        MeshFormat const &meshFormat,
        RasterState const &state,
        vstd::span<PixelFormat const> rtv,
        DepthFormat dsv);
    static void GetMeshFormatState(
        vstd::vector<D3D12_INPUT_ELEMENT_DESC> &inputLayout,
        MeshFormat const &meshFormat);
    static D3D12_GRAPHICS_PIPELINE_STATE_DESC GetState(
        vstd::vector<D3D12_INPUT_ELEMENT_DESC> &inputLayout,
        MeshFormat const &meshFormat,
        RasterState const &state,
        vstd::span<PixelFormat const> rtv,
        DepthFormat dsv);
    RasterShader(
        Device *device,
        vstd::vector<Property> &&prop,
        vstd::vector<SavedArgument> &&args,
        MeshFormat const &meshFormat,
        RasterState const &state,
        vstd::span<PixelFormat const> rtv,
        DepthFormat dsv,
        vstd::span<std::byte const> vertBinData,
        vstd::span<std::byte const> pixelBinData);

    ~RasterShader();

    static RasterShader *CompileRaster(
        BinaryIOVisitor *fileIo,
        Device *device,
        Function vertexKernel,
        Function pixelKernel,
        vstd::function<CodegenResult()> const &codegen,
        vstd::MD5 const &md5,
        uint shaderModel,
        MeshFormat const &meshFormat,
        RasterState const &state,
        vstd::span<PixelFormat const> rtv,
        DepthFormat dsv,
        vstd::string_view fileName,
        bool byteCodeIsCache);
};
}// namespace toolhub::directx