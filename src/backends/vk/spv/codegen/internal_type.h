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
	bool operator==(InternalType const& t) const {
		return tag == t.tag && dimension == t.dimension;
	}
	bool operator!=(InternalType const& t) const {return !operator==(t);};
	InternalType() {}
	InternalType(Tag tag, uint dim)
		: tag(tag), dimension(dim) {}
	static vstd::optional<InternalType> GetType(Type const* type);
	Id TypeId() const;
	size_t Align() const;
	size_t Size() const;
	ConstValue LiteralValue(vstd::variant<bool, int32, uint, float> var);
};
}// namespace toolhub::spv