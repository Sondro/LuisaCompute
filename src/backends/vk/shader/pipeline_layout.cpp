#include "pipeline_layout.h"
#include <vulkan_initializer.hpp>
#include <shader/descriptorset_manager.h>
namespace toolhub::vk {
PipelineLayout::PipelineLayout(
	Device const* device,
	vstd::span<VkDescriptorType> properties)
	: Resource(device),
	  propertiesTypes(properties) {
	vstd::small_vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
	setLayoutBindings.push_back_func(properties.size(), [&](size_t i) {
		return vks::initializers::descriptorSetLayoutBinding(properties[i], VK_SHADER_STAGE_COMPUTE_BIT, i);
	});

	VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
	ThrowIfFailed(vkCreateDescriptorSetLayout(device->device, &descriptorLayout, Device::Allocator(), &descriptorSetLayout));
	//TODO: add bindless array descset layout
	// bindless descset should be in Device class
	VkDescriptorSetLayout layouts[] = {
		descriptorSetLayout,
		device->bindlessBufferSetLayout,
		device->bindlessTex2DSetLayout,
		device->bindlessTex3DSetLayout,
		device->samplerSetLayout};
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo =
		vks::initializers::pipelineLayoutCreateInfo(layouts, vstd::array_count(layouts));

	ThrowIfFailed(vkCreatePipelineLayout(device->device, &pPipelineLayoutCreateInfo, Device::Allocator(), &pipelineLayout));
}
PipelineLayout::~PipelineLayout() {
    DescriptorSetManager::DestroyPipelineLayout(descriptorSetLayout);
	vkDestroyPipelineLayout(device->device, pipelineLayout, Device::Allocator());
	vkDestroyDescriptorSetLayout(device->device, descriptorSetLayout, Device::Allocator());
}
}// namespace toolhub::vk