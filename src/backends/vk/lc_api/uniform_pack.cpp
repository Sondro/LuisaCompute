#include "uniform_pack.h"
#include <vulkan_include.h>
namespace toolhub::vk {
void UniformPack::AlignAs(size_t align) {
	auto sz = CalcAlign(data.size(), align);
	data.resize(sz);
	structAlign = std::max(structAlign, align);
}
void* UniformPack::AlignAs(size_t align, size_t nextSize) {
	auto sz = CalcAlign(data.size(), align);
	data.resize(sz + nextSize);
	structAlign = std::max(structAlign, align);
	return data.data() + sz;
}

void UniformPack::Reset() {
	data.clear();
	structAlign = 1;
}
void UniformPack::AddValue(size_t size, size_t align, vstd::span<std::byte const> data) {
	auto ptr = AlignAs(align, size);
	memcpy(ptr, data.data(), data.size());
}

void UniformPack::AddValue(Type const* type, vstd::span<std::byte const> data) {
	AddValue(type->size(), type->alignment(), data);
}
}// namespace toolhub::vk