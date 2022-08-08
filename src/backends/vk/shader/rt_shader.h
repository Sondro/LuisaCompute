#pragma once
#include "pipeline_layout.h"
#include <gpu_collection/buffer.h>
namespace toolhub::vk {
class PipelineCache;
class CommandBuffer;

class RTShader : public Resource {
	friend class CommandBuffer;
	PipelineLayout pipeLayout;
	VkPipeline pipeline;
	vstd::StackObject<Buffer> sbtBuffer;
    luisa::unordered_map<VkShaderStageFlags, size_t> sbtOffsets;

public:
	PipelineLayout const& Layout() const { return pipeLayout; }
    Buffer const* SBTBuffer() const{return sbtBuffer;}
	RTShader(
		Device const* device,
		vstd::span<uint const> raygen,
		vstd::span<uint const> miss,
		vstd::span<uint const> closestHit,
		PipelineCache const* cache,
		vstd::span<VkDescriptorType> properties);
	~RTShader();
    VkStridedDeviceAddressRegionKHR GetSBTAddress(VkShaderStageFlags flag) const;
};
}// namespace toolhub::vk