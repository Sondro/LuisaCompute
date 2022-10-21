#pragma once
#include <d3dx12.h>
#include <vstl/PoolAllocator.h>
#include <Resource/BufferView.h>
#include <vstl/VGuid.h>
#include <dxgi1_4.h>
#include <DXRuntime/ShaderPaths.h>
#include <core/binary_io_visitor.h>
#include <vstl/BinaryReader.h>
namespace luisa::compute {
class BinaryIOVisitor;
}
class ElementAllocator;
using Microsoft::WRL::ComPtr;
namespace toolhub::directx {
class IGpuAllocator;
class DescriptorHeap;
class ComputeShader;
class PipelineLibrary;
class ShaderCompiler;
class BinaryStream : public luisa::compute::IBinaryStream, public vstd::IOperatorNewBase {
public:
	BinaryReader reader;
	BinaryStream(vstd::string const& path);
	size_t length() const override;
	size_t pos() const override;
	void read(luisa::span<std::byte> dst) override;
	~BinaryStream();
};
struct SerializeVisitor : public luisa::compute::BinaryIOVisitor {
	ShaderPaths const& path;
	SerializeVisitor(
		ShaderPaths const& path);
	luisa::unique_ptr<luisa::compute::IBinaryStream> Read(vstd::string const& filePath);
	void Write(vstd::string const& filePath, luisa::span<std::byte const> data);
//	static vstd::string_view FileNameFilter(vstd::string_view path);
	luisa::unique_ptr<luisa::compute::IBinaryStream> read_bytecode(luisa::string_view name) override;
	luisa::unique_ptr<luisa::compute::IBinaryStream> read_cache(luisa::string_view name) override;
	void write_bytecode(luisa::string_view name, luisa::span<std::byte const> data) override;
	void write_cache(luisa::string_view name, luisa::span<std::byte const> data) override;
};
class Device {
public:
    size_t maxAllocatorCount = 2;
	ShaderPaths const& path;
	std::atomic<luisa::compute::BinaryIOVisitor*> fileIo = nullptr;
	mutable SerializeVisitor serVisitor;
	struct LazyLoadShader {
	public:
		using LoadFunc = vstd::funcPtr_t<ComputeShader*(Device*, ShaderPaths const&)>;

	private:
		vstd::unique_ptr<ComputeShader> shader;
		LoadFunc loadFunc;

	public:
		LazyLoadShader(LoadFunc loadFunc);
		ComputeShader* Get(Device* self);
		bool Check(Device* self);
		~LazyLoadShader();
	};
	bool SupportMeshShader() const;

	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
	Microsoft::WRL::ComPtr<ID3D12Device5> device;
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	vstd::unique_ptr<IGpuAllocator> defaultAllocator;
	vstd::unique_ptr<DescriptorHeap> globalHeap;
	vstd::unique_ptr<DescriptorHeap> samplerHeap;

	LazyLoadShader setAccelKernel;

	LazyLoadShader bc6TryModeG10;
	LazyLoadShader bc6TryModeLE10;
	LazyLoadShader bc6EncodeBlock;

	LazyLoadShader bc7TryMode456;
	LazyLoadShader bc7TryMode137;
	LazyLoadShader bc7TryMode02;
	LazyLoadShader bc7EncodeBlock;
	
	/*vstd::unique_ptr<ComputeShader> bc6_0;
    vstd::unique_ptr<ComputeShader> bc6_1;
    vstd::unique_ptr<ComputeShader> bc6_2;
    vstd::unique_ptr<ComputeShader> bc7_0;
    vstd::unique_ptr<ComputeShader> bc7_1;
    vstd::unique_ptr<ComputeShader> bc7_2;
    vstd::unique_ptr<ComputeShader> bc7_3;*/
	Device(ShaderPaths const& path, uint index);
	Device(Device const&) = delete;
	Device(Device&&) = delete;
	~Device();
	void WaitFence(ID3D12Fence* fence, uint64 fenceIndex);
	static ShaderCompiler* Compiler();
};
}// namespace toolhub::directx