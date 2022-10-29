#pragma once
#include <DXRuntime/Device.h>
namespace toolhub::directx {
class IGpuAllocator : public vstd::ISelfPtr {
public:
	enum class Tag : uint8_t {
		None,
		DefaultAllocator
	};
	static IGpuAllocator* CreateAllocator(
		Device* device,
		Tag tag);
	virtual uint64 AllocateBufferHeap(
		Device* device,
		uint64_t targetSizeInBytes,
		D3D12_HEAP_TYPE heapType,
		ID3D12Heap** heap, uint64_t* offset) = 0;
	virtual uint64 AllocateTextureHeap(
		Device* device,
		size_t sizeBytes,
		ID3D12Heap** heap, uint64_t* offset,
		bool isRenderTexture) = 0;
	virtual void Release(uint64 handle) = 0;
	virtual ~IGpuAllocator() = default;
};
}// namespace toolhub::directx