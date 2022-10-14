#include <shader/compute_shader.h>
#include <vstl/small_vector.h>
#include <vulkan_initializer.hpp>
#include <vk_utility/shader_utility.h>
#include <shader/pipeline_cache.h>
namespace toolhub::vk {
ComputeShader::ComputeShader(
	Device const* device,
	vstd::span<uint const> spirvCode,
	PipelineCache const* cache,
	vstd::span<VkDescriptorType> properties,
	uint3 threadGroupSize)
	: threadGroupSize(threadGroupSize),
	  pipeLayout(device, VK_SHADER_STAGE_COMPUTE_BIT, properties),
	  Resource(device) {

	VkComputePipelineCreateInfo computePipelineCreateInfo =
		vks::initializers::computePipelineCreateInfo(pipeLayout.Layout(), 0);
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStage.module = ShaderUtility::LoadShader(spirvCode, device->device);
	shaderStage.pName = "main";
	computePipelineCreateInfo.stage = shaderStage;
	auto d = vstd::scope_exit([&] {
		vkDestroyShaderModule(device->device, shaderStage.module, Device::Allocator());
	});
	auto CreatePipeline = [&](VkPipelineCache cache) {
		ThrowIfFailed(vkCreateComputePipelines(device->device, cache, 1, &computePipelineCreateInfo, Device::Allocator(), &pipeline));
	};
	if (cache) {
		auto lck = cache->lock();
		CreatePipeline(cache->Cache());
	} else {
		CreatePipeline(nullptr);
	}
}

ComputeShader::~ComputeShader() {
	vkDestroyPipeline(device->device, pipeline, Device::Allocator());
}
}// namespace toolhub::vk