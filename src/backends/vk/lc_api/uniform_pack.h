#pragma once
#include <ast/function.h>
#include <runtime/command.h>
#include <vstl/Common.h>
using namespace luisa;
using namespace luisa::compute;
namespace toolhub::vk {
class UniformPack : public vstd::IOperatorNewBase {
	vstd::vector<std::byte> data;
    size_t structAlign = 1;
    void AlignAs(size_t align);
    void* AlignAs(size_t align, size_t nextSize);
public:
	vstd::span<std::byte const> Data() const {
		return data;
	}
	void Reset();
	void AddValue(Type const* type, vstd::span<std::byte const> data);
	void AddValue(size_t size, size_t align, vstd::span<std::byte const> data);
};
}// namespace toolhub::vk