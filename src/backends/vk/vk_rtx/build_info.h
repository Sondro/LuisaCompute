#pragma once
#include <vk_types/buffer_view.h>
#include <components/device.h>
namespace toolhub::vk {
struct BuildInfo {
	VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
	VkAccelerationStructureGeometryKHR geoInfo{};
	size_t scratchSize;
	bool isUpdate;
	Buffer const* buffer;
	size_t scratchOffset;
};
void GetAccelBuildInfo(
	VkAccelerationStructureBuildGeometryInfoKHR& asBuildGeoInfo,
	bool isTopLevel,
	bool allowUpdate, bool allowCompact, bool fastTrace,
	bool isUpdate);
VkDeviceAddress GetAccelAddress(
	Device const* device,
	VkAccelerationStructureKHR accel);
}// namespace toolhub::vk