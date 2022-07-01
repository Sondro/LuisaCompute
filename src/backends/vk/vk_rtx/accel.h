#pragma once
#include <gpu_collection/gpu_collection.h>
#include "build_info.h"
#include <vstl/functional.h>
namespace toolhub::vk {
class Mesh;
class ResStateTracker;
class Buffer;
class CommandBuffer;
class FrameResource;
class Accel : public GPUCollection {
	vstd::vector<VkAccelerationStructureInstanceKHR> accelInsts;
	vstd::unique_ptr<Buffer> instanceBuffer;//VkAccelerationStructureInstanceKHR
	vstd::unique_ptr<Buffer> accelBuffer;
	VkAccelerationStructureKHR accel = nullptr;
	size_t lastInstCount = 0;
	bool allowUpdate;
	bool allowCompact;
	bool fastTrace;

public:
	enum class Visibility : vbyte {
		On,
		Off,
		Unchange
	};
	static constexpr auto ACCEL_INST_SIZE = sizeof(VkAccelerationStructureInstanceKHR);

	VkAccelerationStructureKHR AccelNativeHandle() const { return accel; }
	Buffer* AccelBuffer() const { return accelBuffer.get(); }
	Buffer* InstanceBuffer() const { return instanceBuffer.get(); }
	Accel(Device const* device,
		  bool allowUpdate, bool allowCompact, bool fastTrace);
	~Accel();
	VkBufferCopy SetInstance(
		BuildInfo& info,
		size_t index,
		Mesh const* mesh,
		float const* matPtr,
		Buffer const* uploadBuffer,
		size_t& instByteOffset,
		bool updateTransform, bool updateMesh, Visibility visible);
	BuildInfo Preprocess(
		CommandBuffer* cb,
		ResStateTracker& stateTracker,
		size_t buildSize,
		bool isUpdate,
		size_t instanceUpdateCount,
		FrameResource* frameRes);

	void Build(
		ResStateTracker& stateTracker,
		CommandBuffer* cb,
		BuildInfo& buildBuffer,
		size_t instanceUpdateCount,
		size_t buildSize);
	Tag GetTag() const override {
		return Tag::Accel;
	}
};
}// namespace toolhub::vk