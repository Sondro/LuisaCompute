#pragma once
#include <Shader/Shader.h>
#include <vstl/VGuid.h>
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
		vstd::span<Property const> prop,
		ComPtr<ID3D12RootSignature>&& rootSig,
		ComPtr<ID3D12PipelineState>&& pso);

public:

	Tag GetTag() const { return Tag::ComputeShader; }
	uint3 BlockSize() const { return blockSize; }
	static ComputeShader* CompileCompute(
		Device* device,
		CodegenResult const& str,
		uint3 blockSize,
		uint shaderModel,
		vstd::string_view cacheFolder,
		vstd::string_view psoFolder,
		vstd::optional<vstd::string>&& fileName);
	static ComputeShader* LoadPresetCompute(
		Device* device,
		uint3 blockSize,
		vstd::string_view cacheFolder,
		vstd::string_view psoFolder,
		vstd::string_view fileName);
	ComputeShader(
		uint3 blockSize,
		vstd::span<Property const> properties,
		vstd::span<vbyte const> binData,
		Device* device);
	ID3D12PipelineState* Pso() const { return pso.Get(); }
	~ComputeShader();
	KILL_COPY_CONSTRUCT(ComputeShader)
	KILL_MOVE_CONSTRUCT(ComputeShader)
};
}// namespace toolhub::directx