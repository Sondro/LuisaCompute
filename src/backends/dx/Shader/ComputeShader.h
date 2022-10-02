#pragma once
#include <Shader/Shader.h>
#include <vstl/VGuid.h>
#include <core/binary_io.h>
namespace toolhub::directx {
struct CodegenResult;
class ShaderSerializer;
class ComputeShader final : public Shader {
	friend class ShaderSerializer;

protected:
	ComPtr<ID3D12PipelineState> pso;
	Device* device;
	uint3 blockSize;
	ComputeShader(
		uint3 blockSize,
		Device* device,
		vstd::vector<Property>&& prop,
		vstd::vector<SavedArgument>&& args,
		ComPtr<ID3D12RootSignature>&& rootSig,
		ComPtr<ID3D12PipelineState>&& pso);

public:
	Tag GetTag() const { return Tag::ComputeShader; }
	uint3 BlockSize() const { return blockSize; }
	static ComputeShader* CompileCompute(
		BinaryIO* fileIo,
		Device* device,
		Function kernel,
		vstd::function<CodegenResult()> const& codegen,
		uint3 blockSize,
		uint shaderModel,
		vstd::string_view fileName,
		vstd::optional<vstd::MD5> const& checkMD5);
	static ComputeShader* LoadPresetCompute(
		BinaryIO* fileIo,
		Device* device,
		vstd::span<Type const* const> types,
		vstd::string_view fileName);
	ComputeShader(
		uint3 blockSize,
		vstd::vector<Property>&& properties,
		vstd::vector<SavedArgument>&& args,
		vstd::span<vbyte const> binData,
		Device* device);
	ID3D12PipelineState* Pso() const { return pso.Get(); }
	~ComputeShader();
	KILL_COPY_CONSTRUCT(ComputeShader)
	KILL_MOVE_CONSTRUCT(ComputeShader)
};
}// namespace toolhub::directx