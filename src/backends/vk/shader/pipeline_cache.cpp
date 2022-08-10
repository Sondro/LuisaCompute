#include <shader/pipeline_cache.h>
#include <vstl/MD5.h>
namespace toolhub::vk {
PipelineCache::PipelineCache(
	Device const* device,
	vstd::span<vbyte const> psoCache)
	: Resource(device) {
	VkPipelineCacheCreateInfo createInfo{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
	auto ptr = reinterpret_cast<VkPipelineCacheHeaderVersionOne const*>(psoCache.data());
	if (ptr && (memcmp(ptr, &device->psoCache, sizeof(VkPipelineCacheHeaderVersionOne)) == 0)) {
		createInfo.initialDataSize = psoCache.size();
		createInfo.pInitialData = psoCache.data();
		cacheAvailable = true;
		createInfo.flags = VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
	} else {
		cacheAvailable = false;
		createInfo.initialDataSize = 0;
	}
	ThrowIfFailed(vkCreatePipelineCache(device->device, &createInfo, Device::Allocator(), &pipelineCache));
}
vstd::vector<vbyte> PipelineCache::GetPSOData() const {
	vstd::vector<vbyte> data;
	size_t dataSize;
	vkGetPipelineCacheData(device->device, pipelineCache, &dataSize, nullptr);
	data.resize(dataSize);
	vkGetPipelineCacheData(device->device, pipelineCache, &dataSize, data.data());
	data.resize(dataSize);
	return data;
}
PipelineCache::~PipelineCache() {
	vkDestroyPipelineCache(device->device, pipelineCache, Device::Allocator());
}
}// namespace toolhub::vk