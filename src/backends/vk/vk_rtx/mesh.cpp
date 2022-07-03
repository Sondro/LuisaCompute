#include "mesh.h"
#include "accel.h"
#include <gpu_collection/buffer.h>
#include <vk_runtime/res_state_tracker.h>
#include "query.h"
#include <vk_runtime/command_buffer.h>
#include <vk_runtime/frame_resource.h>
#include "mesh_handle.h"
namespace toolhub::vk {
Mesh::Mesh(Device const* device,
		   bool allowUpdate, bool allowCompact, bool fastTrace)
	: GPUCollection(device),
	  allowUpdate(allowUpdate),
	  allowCompact(allowCompact),
	  fastTrace(fastTrace) {
}
void GetAccelBuildInfo(
	VkAccelerationStructureBuildGeometryInfoKHR& asBuildGeoInfo,
	bool isTopLevel,
	bool allowUpdate, bool allowCompact, bool fastTrace,
	bool isUpdate) {
	asBuildGeoInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	asBuildGeoInfo.type =
		isTopLevel ? VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR : VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	if (allowUpdate) {
		asBuildGeoInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
	}
	if (allowCompact) {
		asBuildGeoInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
	}
	asBuildGeoInfo.flags |= fastTrace ? VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR : VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
	asBuildGeoInfo.mode = isUpdate ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
}
VkDeviceAddress GetAccelAddress(
	Device const* device,
	VkAccelerationStructureKHR accel) {
	VkAccelerationStructureDeviceAddressInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
	createInfo.accelerationStructure = accel;
	return device->vkGetAccelerationStructureDeviceAddressKHR(
		device->device,
		&createInfo);
}
VkAccelerationStructureBuildGeometryInfoKHR Mesh::GetBuildInfo(
	bool isUpdate, VkAccelerationStructureGeometryKHR& geoData) {
	VkAccelerationStructureBuildGeometryInfoKHR asBuildGeoInfo{};
	GetAccelBuildInfo(
		asBuildGeoInfo,
		false,
		allowUpdate, allowCompact, fastTrace, isUpdate);

	return asBuildGeoInfo;
}
VkAccelerationStructureGeometryKHR Mesh::GetGeoInfo(
	Buffer const* vertexBuffer,
	size_t vertexStride,
	size_t vertexBufferOffset,
	size_t vertexBufferSize,
	Buffer const* triangleBuffer,
	size_t triangleBufferOffset) {
	VkAccelerationStructureGeometryKHR geo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
	geo.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geo.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	auto&& tri = geo.geometry.triangles;
	tri.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	tri.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	tri.vertexData.deviceAddress = vertexBuffer->GetAddress(vertexBufferOffset);
	tri.vertexStride = vertexStride;
	tri.maxVertex = vertexBufferSize / vertexStride;
	tri.indexData.deviceAddress = triangleBuffer->GetAddress(triangleBufferOffset);
	tri.indexType = VK_INDEX_TYPE_UINT32;
	return geo;
}
BuildInfo Mesh::Preprocess(
	ResStateTracker& stateTracker,
	Buffer const* vertexBuffer,
	size_t vertexStride,
	size_t vertexBufferOffset,
	size_t vertexBufferSize,
	Buffer const* triangleBuffer,
	size_t triangleBufferOffset,
	size_t triangleBufferSize,
	bool isUpdate,
	FrameResource* frameRes) {
	bool canUpdate = (lastTriSize == triangleBufferSize) && (lastVertSize == vertexBufferSize);
	lastTriSize = triangleBufferSize;
	lastVertSize = vertexBufferSize;
	isUpdate = isUpdate && canUpdate && allowUpdate;
	stateTracker.MarkBufferRead(
		BufferView(vertexBuffer, vertexBufferOffset, vertexBufferSize),
		BufferReadState::BuildAccel);
	stateTracker.MarkBufferRead(
		BufferView(triangleBuffer, triangleBufferOffset, triangleBufferSize),
		BufferReadState::BuildAccel);
	auto geoInfo = GetGeoInfo(
		vertexBuffer,
		vertexStride, vertexBufferOffset,
		vertexBufferSize,
		triangleBuffer,
		triangleBufferOffset);
	auto asBuildInfo = GetBuildInfo(isUpdate, geoInfo);
	uint triangleCount = triangleBufferSize / (3 * sizeof(uint));
	asBuildInfo.geometryCount = 1;
	asBuildInfo.pGeometries = &geoInfo;

	VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	device->vkGetAccelerationStructureBuildSizesKHR(
		device->device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&asBuildInfo, &triangleCount,
		&buildSizeInfo);
	auto bfSize = buildSizeInfo.accelerationStructureSize;
	if (accelBuffer && accelBuffer->ByteSize() < bfSize) {
		frameRes->AddDisposeEvent([bf = std::move(accelBuffer)] {});
	}
	if (!accelBuffer) {
		accelBuffer = vstd::create_unique(new Buffer(
			device,
			bfSize,
			false,
			RWState::Accel,
			256));
	}
	if (!isUpdate) {
		if (accel) {
			frameRes->AddDisposeEvent([this, a = accel] {
				device->vkDestroyAccelerationStructureKHR(device->device, a, Device::Allocator());
			});
		}
		VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
		createInfo.buffer = accelBuffer->GetResource();
		createInfo.offset = 0;
		createInfo.size = bfSize;
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		ThrowIfFailed(device->vkCreateAccelerationStructureKHR(
			device->device,
			&createInfo,
			Device::Allocator(),
			&accel));
		UpdateAccel();
	}
	stateTracker.MarkBufferWrite(
		accelBuffer.get(),
		BufferWriteState::Accel);

	return {asBuildInfo, geoInfo, isUpdate ? buildSizeInfo.updateScratchSize : buildSizeInfo.buildScratchSize, isUpdate};
}
void Mesh::Build(
	VkCommandBuffer cb,
	BuildInfo& buildBuffer,
	size_t triangleBufferSize) {
	auto&& buildInfo = buildBuffer.buildInfo;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &buildBuffer.geoInfo;
	if (buildBuffer.isUpdate) {
		buildInfo.srcAccelerationStructure = accel;
	}
	buildInfo.scratchData.deviceAddress = buildBuffer.buffer ? buildBuffer.buffer->GetAddress(buildBuffer.scratchOffset) : 0;

	buildInfo.dstAccelerationStructure = accel;
	VkAccelerationStructureBuildRangeInfoKHR buildRange;
	buildRange.primitiveCount = triangleBufferSize / (3 * sizeof(uint));
	buildRange.primitiveOffset = 0;
	buildRange.firstVertex = 0;
	buildRange.transformOffset = 0;
	auto ptr = &buildRange;
	device->vkCmdBuildAccelerationStructuresKHR(
		cb,
		1,
		&buildInfo,
		&ptr);
}
Mesh::~Mesh() {
	for(auto&& i : handles){
		i->accel->accelInsts[i->accelIndex].second = nullptr;
		MeshHandle::DestroyHandle(i);
	}
	if (accel) {
		device->vkDestroyAccelerationStructureKHR(device->device, accel, Device::Allocator());
	}
}
VkAccelerationStructureKHR Mesh::PreprocessLoadCompactSize(
	ResStateTracker& stateTracker) {
	stateTracker.MarkBufferRead(
		accelBuffer.get(),
		BufferReadState::BuildAccel);
	return accel;
}
void Mesh::LoadCompactSize(
	Device const* device,
	VkCommandBuffer cb,
	Query* query,
	vstd::span<VkAccelerationStructureKHR> accels) {
	query->Reset(accels.size());
	device->vkCmdWriteAccelerationStructuresPropertiesKHR(
		cb,
		accels.size(),
		accels.data(),
		VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
		query->Pool(),
		0);
}
void Mesh::Compact(
	CommandBuffer* cb,
	size_t afterCompactSize,
	ResStateTracker& stateTracker,
	FrameResource* frameRes) {
	stateTracker.MarkBufferRead(
		accelBuffer.get(),
		BufferReadState::ComputeOrCopy);
	auto newBuffer = new Buffer(
		device,
		afterCompactSize,
		false,
		RWState::Accel,
		256);
	stateTracker.MarkBufferWrite(
		newBuffer,
		BufferWriteState::Copy);
	stateTracker.Execute(cb);
	VkAccelerationStructureKHR newAccel;
	{
		VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
		createInfo.buffer = newBuffer->GetResource();
		createInfo.offset = 0;
		createInfo.size = afterCompactSize;
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		ThrowIfFailed(device->vkCreateAccelerationStructureKHR(
			device->device,
			&createInfo,
			Device::Allocator(),
			&newAccel));
	}
	VkCopyAccelerationStructureInfoKHR createInfo{VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
	createInfo.src = accel;
	createInfo.dst = newAccel;
	createInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
	device->vkCmdCopyAccelerationStructureKHR(
		cb->CmdBuffer(),
		&createInfo);
	frameRes->AddDisposeEvent([a = accel, device = this->device, b = std::move(accelBuffer)] {
		device->vkDestroyAccelerationStructureKHR(device->device, a, Device::Allocator());
	});
	accel = newAccel;
	accelBuffer = vstd::create_unique(newBuffer);
	UpdateAccel();
}
void Mesh::UpdateAccel() {
	std::lock_guard lck(handleMtx);
	for (auto&& i : handles) {
		i->accel->AddUpdateMesh(i->accelIndex);
	}
}

MeshHandle* Mesh::AddAccelRef(Accel* accel, uint index) {
	auto meshHandle = MeshHandle::AllocateHandle();
	meshHandle->mesh = this;
	meshHandle->accel = accel;
	meshHandle->accelIndex = index;
	std::lock_guard lck(handleMtx);
	meshHandle->meshIndex = handles.size();
	handles.emplace_back(meshHandle);
	return meshHandle;
}
void Mesh::RemoveAccelRef(MeshHandle* handle) {
	assert(handle->mesh == this);
	{
		std::lock_guard lck(handleMtx);
		auto last = handles.erase_last();
		if (last != handle) {
			last->meshIndex = handle->meshIndex;
			handles[handle->meshIndex] = last;
		}
	}
	MeshHandle::DestroyHandle(handle);
}
}// namespace toolhub::vk