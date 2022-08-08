#pragma once
#include "pipeline_layout.h"
#include "i_shader.h"
namespace toolhub::vk {
class PipelineCache;
class CommandBuffer;

class ComputeShader : public Resource, public IShader {
	friend class CommandBuffer;
	PipelineLayout pipeLayout;
	VkPipeline pipeline;
	uint3 threadGroupSize;

public:
	Tag GetTag() const { return Tag::Compute; }
	PipelineLayout const& GetLayout() const override { return pipeLayout; }
	VkPipeline GetPipeline() const override { return pipeline; }
	bool useConstBuffer = false;
	uint3 ThreadGroupSize() const { return threadGroupSize; }
	ComputeShader(
		Device const* device,
		vstd::span<uint const> spirvCode,
		PipelineCache const* cache,
		vstd::span<VkDescriptorType> properties,
		uint3 threadGroupSize);
	~ComputeShader();
};
}// namespace toolhub::vk