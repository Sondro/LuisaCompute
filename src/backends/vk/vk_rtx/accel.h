#pragma once
#include <gpu_collection/gpu_collection.h>
#include "build_info.h"
#include <vstl/functional.h>
#include <vstl/StackAllocator.h>
namespace toolhub::vk {
class Mesh;
class ResStateTracker;
class Buffer;
class CommandBuffer;
class FrameResource;
class MeshHandle;
class Accel : public GPUCollection {
	struct InitAccelInst: public VkAccelerationStructureInstanceKHR{
		InitAccelInst(){
			mask = 0xFF;
		}
	};
	friend class Mesh;
	vstd::vector<std::pair<InitAccelInst, MeshHandle*>> accelInsts;
	vstd::unique_ptr<Buffer> instanceBuffer;//VkAccelerationStructureInstanceKHR
	vstd::unique_ptr<Buffer> accelBuffer;
	VkAccelerationStructureKHR accel = nullptr;
	size_t lastInstCount = 0;
	bool allowUpdate;
	bool allowCompact;
	bool fastTrace;
	luisa::unordered_set<uint> requireUpdateMesh;

public:
	enum class Visibility : uint8_t {
		On,
		Off,
		Unchange
	};
	static constexpr auto ACCEL_INST_SIZE = sizeof(VkAccelerationStructureInstanceKHR);
	void AddUpdateMesh(uint index);
	VkAccelerationStructureKHR AccelNativeHandle() const { return accel; }
	Buffer* AccelBuffer() const { return accelBuffer.get(); }
	Buffer* InstanceBuffer() const { return instanceBuffer.get(); }
	Accel(Device const* device,
		  bool allowUpdate, bool allowCompact, bool fastTrace);
	~Accel();
	VkBufferCopy SetInstance(
		BuildInfo& info,
		size_t index,
		Mesh* mesh,
		float const* matPtr,
		Buffer const* uploadBuffer,
		size_t& instByteOffset,
		bool updateTransform, bool updateMesh, Visibility visible);
	void UpdateMesh(FrameResource* frameRes);
	BuildInfo Preprocess(
		vstd::StackAllocator& stackAlloc,
		CommandBuffer* cb,
		ResStateTracker& stateTracker,
		size_t buildSize,
		bool isUpdate,
		size_t instanceUpdateCount,
		FrameResource* frameRes);

	void Build(
		vstd::StackAllocator& stackAlloc,
		ResStateTracker& stateTracker,
		CommandBuffer* cb,
		BuildInfo& buildBuffer,
		size_t instanceUpdateCount,
		size_t buildSize,
		vstd::vector<VkAccelerationStructureBuildGeometryInfoKHR>& accelBuildCmd,
		vstd::vector<VkAccelerationStructureBuildRangeInfoKHR*>& accelRangeCmd);
	Tag GetTag() const override {
		return Tag::Accel;
	}
};
}// namespace toolhub::vk