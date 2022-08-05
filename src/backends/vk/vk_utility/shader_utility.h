#pragma once
#include <vulkan_include.h>
#include <vstl/functional.h>
namespace toolhub::vk {
class ShaderUtility {
	ShaderUtility(ShaderUtility const&) = delete;
	ShaderUtility(ShaderUtility&&) = delete;

public:
	static VkShaderModule LoadShader(vstd::span<uint const> data, VkDevice device);
};
}// namespace toolhub::vk