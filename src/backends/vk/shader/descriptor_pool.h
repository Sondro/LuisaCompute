#pragma once
#include <vulkan_initializer.hpp>
#include <components/resource.h>
namespace toolhub::vk {
class DescriptorPool : public Resource {
protected:
	VkDescriptorPool pool;

public:
	static constexpr uint MAX_BINDLESS_SIZE = 65536;
	static constexpr uint MAX_SET = 16384;
	static constexpr uint MAX_RES = MAX_BINDLESS_SIZE + 16384;
	static constexpr uint MAX_SAMP = 16;
	static constexpr uint MAX_WRITE_TEX = MAX_SET * 2;
	DescriptorPool(Device const* device);
	~DescriptorPool();
	VkDescriptorSet Allocate(
		VkDescriptorSetLayout layout,
		VkDescriptorSetVariableDescriptorCountAllocateInfo* info = nullptr);
	void Destroy(VkDescriptorSet set);
	void Destroy(vstd::span<VkDescriptorSet> set);
};
}// namespace toolhub::vk