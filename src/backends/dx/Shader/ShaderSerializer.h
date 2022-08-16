#pragma once
#include <d3dx12.h>
#include <Shader/Shader.h>
namespace toolhub::directx {
class ComputeShader;
class RTShader;
struct ShaderBuildData {
	vstd::vector<vbyte> binData;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
};

class ShaderSerializer {
	static size_t SerializeRootSig(
		vstd::span<Property const> properties,
		vstd::vector<vbyte>& result);
	static ComPtr<ID3D12RootSignature> DeSerializeRootSig(
		ID3D12Device* device,
		vstd::span<vbyte const> bytes);

public:
	static ComPtr<ID3DBlob> SerializeRootSig(
		vstd::span<Property const> properties);
	struct ReadResult {
		vbyte const* fileData;
		size_t fileSize;
		vbyte const* psoData;
		size_t psoSize;
	};
	class Visitor {
	protected:
		~Visitor() = default;

	public:
		virtual vbyte const* ReadFile(size_t size) = 0;
		virtual void ReadFile(void* ptr, size_t size) = 0;
		virtual ReadResult ReadFileAndPSO(
			size_t fileSize) = 0;
		virtual void DeletePSOFile() = 0;
	};
	static vstd::vector<vbyte>
	Serialize(
		vstd::span<Property const> properties,
		vstd::span<SavedArgument const> kernelArgs,
		vstd::span<vbyte> binByte,
		uint bindlessCount,
		uint3 blockSize);
	static ComputeShader* DeSerialize(
		Device* device,
		Visitor& streamFunc);
    static vstd::vector<SavedArgument> SerializeKernel(Function kernel);

	ShaderSerializer() = delete;
	~ShaderSerializer() = delete;
};
}// namespace toolhub::directx