#pragma once
#include <gpu_collection/buffer.h>
#include "build_info.h"
#include <vstl/functional.h>
#include <vstl/StackAllocator.h>
namespace toolhub::vk {
class ResStateTracker;
class Buffer;
class Query;
class CommandBuffer;
class FrameResource;
class Accel;
class MeshHandle;
class Mesh : public GPUCollection {
	VkAccelerationStructureKHR accel = nullptr;
	vstd::unique_ptr<Buffer> accelBuffer;
	vstd::fixedvector<MeshHandle*, 2> handles;
	vstd::spin_mutex handleMtx;
	size_t lastVertSize = 0;
	size_t lastTriSize = 0;
	bool allowUpdate;
	bool allowCompact;
	bool fastTrace;
	void GetBuildInfo(
		VkAccelerationStructureBuildGeometryInfoKHR& asBuildInfo,
		bool isUpdate);
	void GetGeoInfo(
		VkAccelerationStructureGeometryKHR& geoInfo,
		Buffer const* vertexBuffer,
		size_t vertexStride,
		size_t vertexBufferOffset,
		size_t vertexBufferSize,
		Buffer const* triangleBuffer,
		size_t triangleBufferOffset);
	size_t TryInit(
		VkAccelerationStructureBuildGeometryInfoKHR& geometryData,
		size_t triangleBufferSize);
	void UpdateAccel();

public:
	MeshHandle* AddAccelRef(Accel* accel, uint index);
	void RemoveAccelRef(MeshHandle* handle);

	bool AllowUpdate() const { return allowUpdate; }
	bool AllowCompact() const { return allowCompact; }
	bool FastTrace() const { return fastTrace; }
	VkAccelerationStructureKHR GetAccel() const { return accel; }
	Tag GetTag() const override {
		return Tag::Mesh;
	}
	Mesh(Device const* device,
		 bool allowUpdate, bool allowCompact, bool fastTrace);
	BuildInfo Preprocess(
		vstd::StackAllocator& stackAlloc,
		ResStateTracker& stateTracker,
		Buffer const* vertexBuffer,
		size_t vertexStride,
		size_t vertexBufferOffset,
		size_t vertexBufferSize,
		Buffer const* triangleBuffer,
		size_t triangleBufferOffset,
		size_t triangleBufferSize,
		bool isUpdate,
		FrameResource* frameRes);
	void Build(
		vstd::StackAllocator& stackAlloc,
		BuildInfo& buildBuffer,
		size_t triangleBufferSize,
		vstd::vector<VkAccelerationStructureBuildGeometryInfoKHR>& accelBuildCmd,
		vstd::vector<VkAccelerationStructureBuildRangeInfoKHR*>& accelRangeCmd);
	// compact
	VkAccelerationStructureKHR PreprocessLoadCompactSize(
		ResStateTracker& stateTracker);
	static void LoadCompactSize(
		Device const* device,
		VkCommandBuffer cb,
		Query* query,
		vstd::span<VkAccelerationStructureKHR> accels);
	void Compact(
		CommandBuffer* cb,
		size_t afterCompactSize,
		ResStateTracker& stateTracker,
		FrameResource* frameRes);

	~Mesh();
};
}// namespace toolhub::vk