#pragma once
#include <Shader/Shader.h>
#include <core/binary_io_visitor.h>
namespace toolhub::directx {
struct CodegenResult;
class ShaderSerializer;
class ComputeShader final : public Shader {
    friend class ShaderSerializer;

private:
    ComPtr<ID3D12PipelineState> pso;
    Device *device;
    uint3 blockSize;
    ComputeShader(
        uint3 blockSize,
        Device *device,
        vstd::vector<Property> &&prop,
        vstd::vector<SavedArgument> &&args,
        ComPtr<ID3D12RootSignature> &&rootSig,
        ComPtr<ID3D12PipelineState> &&pso);

public:
    Tag GetTag() const { return Tag::ComputeShader; }
    uint3 BlockSize() const { return blockSize; }
    static ComputeShader *CompileCompute(
        BinaryIOVisitor *fileIo,
        Device *device,
        Function kernel,
        vstd::function<CodegenResult()> const &codegen,
        vstd::optional<vstd::MD5> const &md5,
        uint3 blockSize,
        uint shaderModel,
        vstd::string_view fileName,
        bool byteCodeIsCache);
    static void SaveCompute(
        BinaryIOVisitor *fileIo,
        Function kernel,
        CodegenResult &codegen,
        uint3 blockSize,
        uint shaderModel,
        vstd::string_view fileName);
    static ComputeShader *LoadPresetCompute(
        BinaryIOVisitor *fileIo,
        Device *device,
        vstd::span<Type const *const> types,
        vstd::string_view fileName);
    ComputeShader(
        uint3 blockSize,
        vstd::vector<Property> &&properties,
        vstd::vector<SavedArgument> &&args,
        vstd::span<std::byte const> binData,
        Device *device);
    ID3D12PipelineState *Pso() const { return pso.Get(); }
    ~ComputeShader();
    KILL_COPY_CONSTRUCT(ComputeShader)
    KILL_MOVE_CONSTRUCT(ComputeShader)
};
}// namespace toolhub::directx