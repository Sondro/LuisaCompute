#pragma once
#include <vulkan_include.h>
#include <vstl/small_vector.h>
#include <shader/descriptor_pool.h>
#include <vk_types/bind_desriptor.h>
#include <shared_mutex>
#include <vstl/StackAllocator.h>
#include <vstl/LockFreeArrayQueue.h>
namespace toolhub::vk {
class DescriptorSetManager;
class CommandBuffer;
class DescriptorSetManager : public Resource {
	friend class Device;

public:
	struct DescriptorSets {
		vstd::small_vector<VkDescriptorSet> sets;
		vstd::spin_mutex mtx;
		~DescriptorSets();
	};

private:
	size_t glbIndex = 0;
	using DescSetMap = vstd::HashMap<VkDescriptorSetLayout, DescriptorSets>;
	DescSetMap descSets;
	vstd::LockFreeArrayQueue<VkDescriptorSetLayout> removeList;
	vstd::vector<std::pair<DescSetMap::Index, VkDescriptorSet>> allocatedSets;
	vstd::vector<VkWriteDescriptorSet> computeWriteRes;
	vstd::StackAllocator stackAlloc;
	DescriptorSetManager(DescriptorSetManager&&) = delete;
	DescriptorSetManager(DescriptorSetManager const&) = delete;

public:
	static void DestroyPipelineLayout(VkDescriptorSetLayout layout);
	DescriptorSetManager(Device const* device);
	~DescriptorSetManager();
	VkDescriptorSet Allocate(
		VkDescriptorSetLayout layout,
		vstd::span<VkDescriptorType const> descTypes,
		vstd::span<BindResource const> descriptors);
	void EndFrame();
};
}// namespace toolhub::vk