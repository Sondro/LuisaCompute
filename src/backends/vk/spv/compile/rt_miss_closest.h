#pragma once
#include <vulkan_include.h>
namespace toolhub::spv {
class RTMissClosestData {
	vstd::vector<vbyte> data;
	size_t missSize;
	size_t closestSize;

public:
	RTMissClosestData(vstd::string const& path);
    vstd::span<uint const> MissKernel() const;
    vstd::span<uint const> ClosestKernel() const;
    ~RTMissClosestData();
};
}// namespace toolhub::spv