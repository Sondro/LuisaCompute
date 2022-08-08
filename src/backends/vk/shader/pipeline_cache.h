#pragma once
#include <components/resource.h>
namespace toolhub::vk {
class PipelineCache : public Resource {
	VkPipelineCache pipelineCache;
	bool cacheAvailable;

public:
	bool CacheAvailable() const { return cacheAvailable; }
	VkPipelineCache Cache() const { return pipelineCache; }
	vstd::vector<vbyte> GetPSOData() const;
	PipelineCache(
		Device const* device,
		vstd::span<vbyte const> psoCache);
	~PipelineCache();
};
}// namespace toolhub::vk