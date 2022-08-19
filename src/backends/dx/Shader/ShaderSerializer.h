#pragma once
#include <d3dx12.h>
#include <Shader/Shader.h>
#include <core/binary_io_visitor.h>
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
	static vstd::vector<vbyte>
	Serialize(
		vstd::span<Property const> properties,
		vstd::span<SavedArgument const> kernelArgs,
		vstd::span<vbyte const> binByte,
		vstd::optional<vstd::MD5> const& checkMD5,
		uint bindlessCount,
		uint3 blockSize);
	static ComputeShader* DeSerialize(
		luisa::string_view fileName,
		Device* device,
		BinaryIOVisitor& streamFunc,
		vstd::optional<vstd::MD5> const& checkMD5,
		bool& clearCache);
	static vstd::vector<SavedArgument> SerializeKernel(Function kernel);

	ShaderSerializer() = delete;
	~ShaderSerializer() = delete;
};
}// namespace toolhub::directx