#include <vk_utility/shader_utility.h>
#include <components/device.h>
namespace toolhub::vk {
VkShaderModule ShaderUtility::LoadShader(vstd::span<uint const> data, VkDevice device) {
	VkShaderModule shaderModule;
	VkShaderModuleCreateInfo moduleCreateInfo{};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = data.size_bytes();
	moduleCreateInfo.pCode = reinterpret_cast<uint const*>(data.data());
	ThrowIfFailed(vkCreateShaderModule(device, &moduleCreateInfo, Device::Allocator(), &shaderModule));
	return shaderModule;
}
}// namespace toolhub::vk