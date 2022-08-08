#include <shader/pipeline_cache.h>
#include <vstl/MD5.h>
namespace toolhub::vk {
PipelineCache::PipelineCache(
	Device const* device,
	vstd::span<vbyte const> psoCache)
	: Resource(device) {
	VkPipelineCacheCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	PipelineCachePrefixHeader const* ptr = reinterpret_cast<PipelineCachePrefixHeader const*>(psoCache.data());
	if (ptr && (psoCache.size() > sizeof(PipelineCachePrefixHeader)) && (*ptr == device->psoHeader)) {
		createInfo.initialDataSize = psoCache.size() - sizeof(PipelineCachePrefixHeader);
		createInfo.pInitialData = psoCache.data() + sizeof(PipelineCachePrefixHeader);
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