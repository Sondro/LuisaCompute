#pragma once
#include "types.h"
#include "internal_type.h"
namespace toolhub::spv {
class TypeCaster {
	TypeCaster() = delete;

public:
	enum class CompareResult {
		ToLeft,
		ToRight,
		UnChanged
	};
	static CompareResult Compare(
		InternalType const& srcType,
		InternalType const& dstType);
	static Id TryCast(
		Builder* bd,
		InternalType const& srcType,
		InternalType const& dstType,
		Id value);
};
}// namespace toolhub::spv