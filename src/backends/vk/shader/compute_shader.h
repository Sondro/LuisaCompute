#pragma once
#include "pipeline_layout.h"
namespace toolhub::vk {
class PipelineCache;
class CommandBuffer;

class ComputeShader : public Resource {
	friend class CommandBuffer;
	PipelineLayout pipeLayout;
	VkPipeline pipeline;
	uint3 threadGroupSize;

public:
	PipelineLayout const& Layout() const { return pipeLayout; }
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