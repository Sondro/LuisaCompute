#pragma once
#include <vulkan_include.h>
#include <vstl/StackAllocator.h>
namespace toolhub::vk {
static constexpr uint PSO_MAGIC_NUM = 2595790981u;
class Device;
class DescriptorSetManager;
class DescriptorPool;
class GPUAllocator;
class TexView;
class BufferView;
class Device : public vstd::IOperatorNewBase {
	Device();
	void Init();
	void InitBindless();
	mutable vstd::spin_mutex updateBindlessMtx;
	mutable vstd::spin_mutex allocIdxMtx;
	mutable vstd::vector<uint> bindlessIdx;
	mutable vstd::vector<VkWriteDescriptorSet> bindlessWriteRes;
	mutable vstd::StackAllocator bindlessStackAlloc;

public:
	VkPipelineCacheHeaderVersionOne psoCache;
	VkPhysicalDeviceLimits limits;
	static VkInstance InitVkInstance();
	uint AllocateBindlessIdx() const;
	void DeAllocateBindlessIdx(uint index) const;
	mutable vstd::DefaultMallocVisitor mallocVisitor;
	vstd::unique_ptr<DescriptorSetManager> manager;
	vstd::unique_ptr<GPUAllocator> gpuAllocator;
	VkQueue computeQueue;
	VkQueue presentQueue;
	uint computeFamily;
	vstd::optional<uint> presentFamily;
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties;
	static VkAllocationCallbacks* Allocator();
	static Device* CreateDevice(
		VkInstance instance,
		VkSurfaceKHR surface,
		uint physicalDeviceIndex,
		void* placedMemory = nullptr);
	~Device();
	//KHR functions
	PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
	PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
	PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
	PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
	PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
	PFN_vkCmdWriteAccelerationStructuresPropertiesKHR vkCmdWriteAccelerationStructuresPropertiesKHR;
	PFN_vkCmdCopyAccelerationStructureKHR vkCmdCopyAccelerationStructureKHR;
	PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
	PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
	PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

	vstd::unique_ptr<DescriptorPool> pool;
	VkDescriptorSetLayout samplerSetLayout;
	VkDescriptorSet samplerSet;
	VkDescriptorSetLayout bindlessTex2DSetLayout;
	VkDescriptorSet bindlessTex2DSet;
	VkDescriptorSetLayout bindlessTex3DSetLayout;
	VkDescriptorSet bindlessTex3DSet;
	VkDescriptorSetLayout bindlessBufferSetLayout;
	VkDescriptorSet bindlessBufferSet;
	std::array<VkSampler, 16> samplers;
	void AddBindlessBufferUpdateCmd(size_t index, BufferView const& buffer) const;
	void AddBindlessTex2DUpdateCmd(size_t index, TexView const& tex) const;
	void AddBindlessTex3DUpdateCmd(size_t index, TexView const& tex) const;
	void UpdateBindless() const;
};
}// namespace toolhub::vk