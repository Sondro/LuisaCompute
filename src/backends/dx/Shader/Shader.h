#pragma once
#include <vstl/Common.h>
#include <Windows.h>
#include <d3dx12.h>
#include <Shader/ShaderVariableType.h>
#include <Resource/Buffer.h>
#include <Resource/DescriptorHeap.h>
#include <ast/function.h>
using namespace luisa::compute;
namespace toolhub::directx {
struct SavedArgument {
	Type::Tag tag;
	Usage varUsage;
	uint structSize;
	SavedArgument() {}
	SavedArgument(Function kernel, Variable const& var);
	SavedArgument(Usage usage, Variable const& var);
	SavedArgument(Type const* type);
};
class TopAccel;
class CommandBufferBuilder;
class Shader : public vstd::IOperatorNewBase {
public:
	enum class Tag : vbyte {
		ComputeShader,
		RayTracingShader,
		RasterShader
	};
	virtual Tag GetTag() const = 0;

protected:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig;
	vstd::vector<Property> properties;
	vstd::vector<SavedArgument> kernelArguments;
	uint bindlessCount;

public:
	virtual ~Shader() noexcept = default;
	uint BindlessCount() const { return bindlessCount; }
	vstd::span<Property const> Properties() const { return properties; }
	vstd::span<SavedArgument const> Args() const { return kernelArguments; }
	Shader(
		vstd::vector<Property>&& properties,
		vstd::vector<SavedArgument>&& args,
		ID3D12Device* device,
		bool isRaster);
	Shader(
		vstd::vector<Property>&& properties,
		vstd::vector<SavedArgument>&& args,
		ComPtr<ID3D12RootSignature>&& rootSig);
	ID3D12RootSignature* RootSig() const { return rootSig.Get(); }

	void SetComputeResource(
		uint propertyName,
		CommandBufferBuilder* cmdList,
		BufferView buffer) const;
	void SetComputeResource(
		uint propertyName,
		CommandBufferBuilder* cmdList,
		DescriptorHeapView view) const;
	void SetComputeResource(
		uint propertyName,
		CommandBufferBuilder* cmdList,
		TopAccel const* bAccel) const;
	void SetComputeResource(
		uint propertyName,
		CommandBufferBuilder* cmdList,
		std::pair<uint, uint4> const& constValue) const;

	void SetRasterResource(
		uint propertyName,
		CommandBufferBuilder* cmdList,
		BufferView buffer) const;
	void SetRasterResource(
		uint propertyName,
		CommandBufferBuilder* cmdList,
		DescriptorHeapView view) const;
	void SetRasterResource(
		uint propertyName,
		CommandBufferBuilder* cmdList,
		TopAccel const* bAccel) const;
	void SetRasterResource(
		uint propertyName,
		CommandBufferBuilder* cmdList,
		std::pair<uint, uint4> const& constValue) const;

	KILL_COPY_CONSTRUCT(Shader)
	KILL_MOVE_CONSTRUCT(Shader)
};
}// namespace toolhub::directx