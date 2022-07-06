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
	uint dimension = 1;
	static vstd::optional<InternalType> GetType(Type const* type);
	Id TypeId() const;
};
}// namespace toolhub::spv