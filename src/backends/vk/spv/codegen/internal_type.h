#pragma once
#include "types.h"
using namespace luisa;
using namespace luisa::compute;
namespace toolhub::spv {
class InternalType {
public:
	enum struct Tag : uint32_t {
		BOOL,
		FLOAT,
		INT,
		UINT,
		MATRIX
	};
	Tag tag;
	uint dimension;
	InternalType() {}
	InternalType(Tag tag, uint dim)
		: tag(tag), dimension(dim) {}
	static vstd::optional<InternalType> GetType(Type const* type);
	Id TypeId() const;
	size_t Align() const;
	size_t Size() const;
};
}// namespace toolhub::spv