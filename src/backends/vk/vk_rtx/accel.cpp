#include "accel.h"
#include "mesh.h"
#include "mesh_handle.h"
#include <gpu_collection/buffer.h>
#include <vk_runtime/res_state_tracker.h>
#include <vk_runtime/command_buffer.h>
#include <vk_runtime/frame_resource.h>
namespace toolhub::vk {
Accel::Accel(Device const* device,
			 bool allowUpdate, bool allowCompact, bool fastTrace)
	: GPUCollection(device),
	  allowUpdate(allowUpdate),
	  allowCompact(allowCompact),
	  fastTrace(fastTrace) {}
Accel::~Accel() {
	for (auto&& i : accelInsts) {
		auto mesh = i.second;
		if (mesh)
			mesh->mesh->RemoveAccelRef(mesh);
	}
	if (accel) {
		device->vkDestroyAccelerationStructureKHR(device->device, accel, Device::Allocator());
	}
}
void Accel::AddUpdateMesh(uint index) {
	requireUpdateMesh.emplace(index);
}
void Accel::UpdateMesh(FrameResource* frameRes) {
	if (requireUpdateMesh.empty()) return;
	auto bfView = frameRes->AllocateUpload(sizeof(uint64) * requireUpdateMesh.size());
	auto iterator = vstd::RangeImpl(
		vstd::CacheEndRange(requireUpdateMesh) | vstd::TransformRange(
													 [&](uint index) {
														 auto newAddressValue = GetAccelAddress(device, accelInsts[index].second->mesh->GetAccel());
														 accelInsts[index].first.accelerationStructureReference = newAddressValue;
														 VkBufferCopy v;
														 bfView.buffer->CopyValueFrom(newAddressValue, bfView.offset);
														 v.srcOffset = bfView.offset;
														 bfView.offset += sizeof(uint64);
														 v.dstOffset = index * ACCEL_INST_SIZE + offsetof(VkAccelerationStructureInstanceKHR, accelerationStructureReference);
														 v.size = sizeof(uint64);
														 return v;
													 }));
	frameRes->AddCopyCmd(
		bfView.buffer,
		instanceBuffer.get(),
		&iterator);
}

VkBufferCopy Accel::SetInstance(
	BuildInfo& info,
	size_t index,
	Mesh* mesh,
	float const* matPtr,
	Buffer const* uploadBuffer,
	size_t& instByteOffset,
	bool updateTransform, bool updateMesh, Visibility visible) {
	requireUpdateMesh.erase(index);
	auto& instPair = accelInsts[index];
	auto& inst = instPair.first;
	auto& originMesh = instPair.second;
	if (updateTransform) {
		memcpy(&inst.transform, matPtr, sizeof(inst.transform));
	}
	switch (visible) {
		case Visibility::On:
			inst.mask = 0xFF;
			break;
		case Visibility::Off:
			inst.mask = 0;
			break;
	}
	inst.instanceShaderBindingTableRecordOffset = 0;
	inst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	if (updateMesh) {
		if (originMesh) {
			if (originMesh->mesh != mesh) {
				originMesh->mesh->RemoveAccelRef(originMesh);
				originMesh = mesh->AddAccelRef(this, index);
			}
		} else {
			originMesh = mesh->AddAccelRef(this, index);
		}
		info.isUpdate = false;
		inst.accelerationStructureReference = GetAccelAddress(device, mesh->GetAccel());
	}
	uploadBuffer->CopyValueFrom(
		inst,
		instByteOffset);

	auto cpy = VkBufferCopy{
		instByteOffset,
		index * ACCEL_INST_SIZE,
		ACCEL_INST_SIZE};

	instByteOffset += ACCEL_INST_SIZE;
	return cpy;
}
BuildInfo Accel::Preprocess(
	vstd::StackAllocator& stackAlloc,
	CommandBuffer* cb,
	ResStateTracker& stateTracker,
	size_t buildSize,
	bool isUpdate,
	size_t instanceUpdateCount,
	FrameResource* frameRes) {
	if (buildSize != accelInsts.size())
		accelInsts.resize(buildSize);
	isUpdate = isUpdate && (lastInstCount == buildSize) && allowUpdate;
	lastInstCount = buildSize;
	if (instanceBuffer) {
		size_t instanceCount = instanceBuffer->ByteSize() / ACCEL_INST_SIZE;
		if (instanceCount < buildSize) {
			do {
				instanceCount = instanceCount * 1.5 + 8;
			} while (instanceCount < buildSize);
			auto newBuffer = new Buffer(
				device,
				instanceCount * ACCEL_INST_SIZE,
				false,
				RWState::None);
			stateTracker.MarkBufferWrite(
				newBuffer,
				BufferWriteState::Copy);
			stateTracker.MarkBufferRead(
				instanceBuffer.get(),
				BufferReadState::ComputeOrCopy);
			stateTracker.Execute(cb);
			cb->CopyBuffer(
				instanceBuffer.get(),
				0,
				newBuffer,
				0,
				instanceBuffer->ByteSize());
			frameRes->AddDisposeEvent([v = std::move(instanceBuffer)] {});
			instanceBuffer = vstd::create_unique(newBuffer);
		}
	} else {
		instanceBuffer = vstd::create_unique(new Buffer(
			device,
			buildSize * ACCEL_INST_SIZE,
			false,
			RWState::None));
	}
	BuildInfo info{};
	VkAccelerationStructureGeometryKHR* geoInfo = stackAlloc.AllocateMemory<VkAccelerationStructureGeometryKHR>();
	GetAccelBuildInfo(info.buildInfo, true, allowUpdate, allowCompact, fastTrace, isUpdate);
	geoInfo->sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geoInfo->geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	auto&& inst = geoInfo->geometry.instances;
	inst.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	inst.arrayOfPointers = false;
	inst.data.deviceAddress = instanceBuffer->GetAddress(0);
	VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	uint instCount = buildSize;
	info.buildInfo.geometryCount = 1;
	info.buildInfo.pGeometries = geoInfo;
	device->vkGetAccelerationStructureBuildSizesKHR(
		device->device,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&info.buildInfo, &instCount,
		&buildSizeInfo);
	auto accelSize = buildSizeInfo.accelerationStructureSize;
	if (accelBuffer) {
		size_t newSize = accelBuffer->ByteSize();
		if (newSize < accelSize) {
			do {
				newSize = newSize * 1.5 + 8;
			} while (newSize < accelSize);
			newSize = CalcAlign(newSize, 256);
			frameRes->AddDisposeEvent([a = std::move(accelBuffer)] {});
			accelBuffer = vstd::create_unique(new Buffer(
				device,
				newSize,
				false,
				RWState::Accel,
				256));
		}
	} else {
		accelBuffer = vstd::create_unique(new Buffer(
			device,
			CalcAlign(accelSize, 256),
			false,
			RWState::Accel,
			256));
	}
	stateTracker.MarkBufferWrite(
		accelBuffer.get(),
		BufferWriteState::Accel);
	if (instanceUpdateCount > 0) {
		stateTracker.MarkBufferWrite(
			instanceBuffer.get(),
			BufferWriteState::Copy);
	}
	info.scratchSize = isUpdate ? buildSizeInfo.updateScratchSize : buildSizeInfo.buildScratchSize;
	if (!isUpdate) {
		if (accel) {
			frameRes->AddDisposeEvent([a = accel, device = device] {
				device->vkDestroyAccelerationStructureKHR(device->device, a, Device::Allocator());
			});
		}
		VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
		createInfo.buffer = accelBuffer->GetResource();
		createInfo.offset = 0;
		createInfo.size = buildSize;
		createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		ThrowIfFailed(device->vkCreateAccelerationStructureKHR(
			device->device,
			&createInfo,
			Device::Allocator(),
			&accel));
	}
	info.isUpdate = isUpdate;
	return info;
}
void Accel::Build(
	vstd::StackAllocator& stackAlloc,
	ResStateTracker& stateTracker,
	CommandBuffer* cb,
	BuildInfo& buildBuffer,
	size_t instanceUpdateCount,
	size_t buildSize,
	vstd::vector<VkAccelerationStructureBuildGeometryInfoKHR>& accelBuildCmd,
	vstd::vector<VkAccelerationStructureBuildRangeInfoKHR*>& accelRangeCmd) {
	if (instanceUpdateCount > 0) {
		stateTracker.MarkBufferRead(
			instanceBuffer.get(),
			BufferReadState::BuildAccel);
		stateTracker.Execute(cb);
	}
	auto&& buildInfo = buildBuffer.buildInfo;
	if (buildBuffer.isUpdate) {
		buildInfo.srcAccelerationStructure = accel;
	}
	buildInfo.scratchData.deviceAddress = buildBuffer.buffer ? buildBuffer.buffer->GetAddress(buildBuffer.scratchOffset) : 0;
	buildInfo.dstAccelerationStructure = accel;
	auto buildRange = stackAlloc.AllocateMemory<VkAccelerationStructureBuildRangeInfoKHR, false>();
	buildRange->primitiveCount = buildSize;
	buildRange->primitiveOffset = 0;
	buildRange->firstVertex = 0;
	buildRange->transformOffset = 0;
	accelBuildCmd.emplace_back(buildInfo);
	accelRangeCmd.emplace_back(buildRange);
}
}// namespace toolhub::vk