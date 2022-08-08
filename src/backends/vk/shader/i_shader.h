#pragma once
#include <vulkan_include.h>
namespace toolhub::vk {
class PipelineLayout;
class IShader {
public:
	enum class Tag : vbyte {
		Compute,
		RayTracing
	};
	virtual Tag GetTag() const = 0;
	virtual PipelineLayout const& GetLayout() const = 0;
	virtual VkPipeline GetPipeline() const { return nullptr; }
	virtual ~IShader() = default;
};
}// namespace toolhub::vk