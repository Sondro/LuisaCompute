#include "rt_shader.h"
#include <vulkan_initializer.hpp>
#include <vk_utility/shader_utility.h>
#include <shader/pipeline_cache.h>
namespace toolhub::vk {
RTShader::RTShader(
	Device const* device,
	vstd::span<uint const> raygen,
	vstd::span<uint const> miss,
	vstd::span<uint const> closestHit,
	PipelineCache const* cache,
	vstd::span<VkDescriptorType> properties)
	: Resource(device) {
	vstd::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	vstd::vector<VkShaderModule> shaderModule;
	auto disp = vstd::create_disposer([&] {
		for (auto&& i : shaderModule) {
			vkDestroyShaderModule(device->device, i, Device::Allocator());
		}
	});
	vstd::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

	auto loadShader = [&](VkShaderStageFlagBits stage, vstd::span<uint const> code) {
		VkPipelineShaderStageCreateInfo shaderStage = {};
		shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStage.stage = stage;
		shaderStage.module = ShaderUtility::LoadShader(code, device->device);
		shaderModule.push_back(shaderStage.module);
		shaderStage.pName = "main";
		return shaderStage;
	};
	VkShaderStageFlags bits = 0;
	///////////////////////// Raygen group
	if (!raygen.empty()) {
		shaderStages.push_back(loadShader(VK_SHADER_STAGE_RAYGEN_BIT_KHR, raygen));
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
		sbtOffsets.emplace(VK_SHADER_STAGE_RAYGEN_BIT_KHR, sbtOffsets.size());
		bits |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	}

	///////////////////////// Miss group
	if (!miss.empty()) {
		shaderStages.push_back(loadShader(VK_SHADER_STAGE_MISS_BIT_KHR, miss));
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
		sbtOffsets.emplace(VK_SHADER_STAGE_MISS_BIT_KHR, sbtOffsets.size());
		bits |= VK_SHADER_STAGE_MISS_BIT_KHR;
	}

	///////////////////////// Closest hit group
	if (!closestHit.empty()) {
		shaderStages.push_back(loadShader(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, closestHit));
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.closestHitShader = static_cast<uint>(shaderStages.size()) - 1;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
		sbtOffsets.emplace(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, sbtOffsets.size());
		bits |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	}
	///////////////////////// Create RT Pipeline
	pipeLayout.New(device, bits, properties);
	{
		VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI{};
		rayTracingPipelineCI.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		rayTracingPipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		rayTracingPipelineCI.pStages = shaderStages.data();
		rayTracingPipelineCI.groupCount = static_cast<uint32_t>(shaderGroups.size());
		rayTracingPipelineCI.pGroups = shaderGroups.data();
		rayTracingPipelineCI.maxPipelineRayRecursionDepth = 1;
		rayTracingPipelineCI.layout = pipeLayout->Layout();
		ThrowIfFailed(device->vkCreateRayTracingPipelinesKHR(device->device, VK_NULL_HANDLE, cache ? cache->Cache() : nullptr, 1, &rayTracingPipelineCI, Device::Allocator(), &pipeline));
	}

	///////////////////////// Create SBT
	{
		auto&& rayTracingProperties = device->rayTracingProperties;
		const uint handleSize = rayTracingProperties.shaderGroupHandleSize;
		const uint handleSizeAligned = CalcAlign(rayTracingProperties.shaderGroupHandleSize, std::max(rayTracingProperties.shaderGroupHandleAlignment, rayTracingProperties.shaderGroupBaseAlignment));
		const uint groupCount = static_cast<uint>(shaderGroups.size());
		const uint sbtSize = groupCount * handleSizeAligned;
		vstd::vector<vbyte> shaderHandleStorage(sbtSize);
		ThrowIfFailed(device->vkGetRayTracingShaderGroupHandlesKHR(device->device, pipeline, 0, groupCount, sbtSize, shaderHandleStorage.data()));
		const VkBufferUsageFlagBits bufferUsageFlags = static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
		sbtBuffer.New(device, shaderHandleStorage.byte_size(), bufferUsageFlags, false, RWState::Upload);
		sbtBuffer->CopyFrom(shaderHandleStorage, 0);
	}
}
RTShader::~RTShader() {
	vkDestroyPipeline(device->device, pipeline, Device::Allocator());
	sbtBuffer.Delete();
	pipeLayout.Delete();
}
VkStridedDeviceAddressRegionKHR RTShader::GetSBTAddress(VkShaderStageFlags flag) const {
	VkStridedDeviceAddressRegionKHR address;
	auto ite = sbtOffsets.find(flag);
	if (ite == sbtOffsets.end()) {
		memset(&address, 0, sizeof(address));
		return address;
	};
	auto&& rayTracingProperties = device->rayTracingProperties;
	auto handleAlignedSize = CalcAlign(rayTracingProperties.shaderGroupHandleSize, std::max(rayTracingProperties.shaderGroupHandleAlignment, rayTracingProperties.shaderGroupBaseAlignment));
	address.deviceAddress = sbtBuffer->GetAddress(ite->second * handleAlignedSize);
	address.size = handleAlignedSize;
	address.stride = handleAlignedSize;
	return address;
}
}// namespace toolhub::vk