#pragma once
#include <vulkan/vulkan.h>
#include <vstl/Common.h>
#include <core/mathematics.h>
using namespace luisa;
using namespace luisa::compute;
namespace toolhub::vk {
static constexpr uint32_t VulkanApiVersion = VK_API_VERSION_1_3;
//0: texture
//1: buffer
//2: sampler
inline void ThrowIfFailed(VkResult result) {
#ifdef DEBUG
	if (result != VK_SUCCESS) {
		VEngine_Log("Vulkan Failed!");
		VENGINE_EXIT;
	}
#endif
}

inline uint64 CalcAlign(uint64 value, uint64 align) {
	return (value + (align - 1)) & ~(align - 1);
}
using uint16 = uint16_t;
}// namespace toolhub::vk