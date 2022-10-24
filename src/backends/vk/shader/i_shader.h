#pragma once
#include <vulkan_include.h>
namespace toolhub::vk {
class PipelineLayout;
class IShader {
public:
	enum class Tag : uint8_t {
		Compute,
		RayTracing
	};
	virtual Tag GetTag() const = 0;
	virtual PipelineLayout const& GetLayout() const = 0;
	virtual VkPipeline GetPipeline() const { return nullptr; }
	virtual ~IShader() = default;
};
}// namespace toolhub::vk