#pragma once
#include <components/resource.h>
namespace toolhub::vk {
class PipelineCache : public Resource {
	VkPipelineCache pipelineCache;
	mutable std::mutex mtx;
	bool cacheAvailable;

public:
	std::lock_guard<std::mutex> lock() const { return  std::lock_guard{mtx}; }
	bool CacheAvailable() const { return cacheAvailable; }
	VkPipelineCache Cache() const { return pipelineCache; }
	vstd::vector<vbyte> GetPSOData() const;
	PipelineCache(
		Device const* device,
		vstd::span<vbyte const> psoCache);
	~PipelineCache();
};
}// namespace toolhub::vk