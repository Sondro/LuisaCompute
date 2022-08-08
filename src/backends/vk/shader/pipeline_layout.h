#pragma once
#include <vk_types/bind_desriptor.h>
#include <components/resource.h>
#include <vstl/small_vector.h>
namespace toolhub::vk {
class PipelineLayout : public Resource {
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	vstd::small_vector<VkDescriptorType> propertiesTypes;

public:
	vstd::span<VkDescriptorType const> PropertiesTypes() const { return propertiesTypes; }
	VkPipelineLayout Layout() const { return pipelineLayout; }
	VkDescriptorSetLayout DescSetLayout() const { return descriptorSetLayout; }
	PipelineLayout(
		Device const* device,
		VkShaderStageFlags shaderStage,
		vstd::span<VkDescriptorType> properties);
	~PipelineLayout();
};
}// namespace toolhub::vk