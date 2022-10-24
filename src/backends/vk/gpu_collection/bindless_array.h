#pragma once
#include "buffer.h"
#include "texture.h"
#include <vk_types/buffer_view.h>

namespace toolhub::vk {
class GPUCollection;
class FrameResource;
class CommandBuffer;
class ResStateTracker;
class BindlessArray : public GPUCollection {
	struct Instance {
		uint index = std::numeric_limits<uint>::max();
	};
	enum class UpdateFlag : uint8_t {
		None = 0,
		BUFFER = 1,
		TEX2D = 2,
		TEX3D = 4
	};
	struct Ref {
		Buffer const* buffer = nullptr;
		Texture const* tex2D = nullptr;
		Texture const* tex3D = nullptr;
		uint refCount = 0;
	};

	struct RefOption {
		uint64 bufferOffset;
		uint8_t tex2DOffset;
		uint8_t tex3DOffset;
		UpdateFlag flag = UpdateFlag::None;
	};
	Buffer instanceBuffer;
	vstd::HashMap<uint64, size_t> refMap;
	vstd::vector<std::pair<Instance, Ref>> instances;
	vstd::HashMap<uint, RefOption> updateList;
	vstd::vector<VkBufferCopy> copyCmds;
	template <typename T>
	requires(std::is_base_of_v<GPUCollection, T>)
	void AddRef(T const* v, uint& refCount);
	template <typename T>
	requires(std::is_base_of_v<GPUCollection, T>)
	void RemoveRef(T const* v, uint& refCount);

public:
	//host set
	void Bind(uint index, Buffer const* buffer, uint64 offset);
	void Bind(uint index, Texture const* tex, uint8_t mipOffset);
	void UnBindBuffer(uint index);
	void UnBindTex2D(uint index);
	void UnBindTex3D(uint index);
	//rendering
	void Preprocess(
		FrameResource* res,
		ResStateTracker& stateTracker,
		CommandBuffer* cb);
	Buffer const* InstanceBuffer() const { return &instanceBuffer; }

	bool IsPtrInRes(uint64 handle) const;
	Tag GetTag() const {
		return Tag::BindlessArray;
	}
	BindlessArray(
		Device const* device,
		uint arrSize);
	~BindlessArray();
};
}// namespace toolhub::vk