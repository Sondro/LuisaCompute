#pragma once
#include <vulkan_include.h>
#include <components/device.h>
#include <vstl/small_vector.h>
namespace toolhub::vk {

class Resource : public vstd::IOperatorNewBase {
protected:
	Device const* device;

public:
	Device const* GetDevice() const{return device;};
	Resource(Device const* device) : device(device) {}
	virtual ~Resource() = default;
};
}// namespace toolhub::vk