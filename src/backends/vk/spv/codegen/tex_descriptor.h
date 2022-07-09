#pragma once
#include "internal_type.h"
namespace toolhub::spv {
class TexDescriptor {
public:
	enum class TexType : uint {
		SampleTex,
		StorageTex
	};
	InternalType vectorType;
	uint dimension;
	TexType texType;
	TexDescriptor(
		InternalType vectorType,
		uint dimension,
		TexType texType);
};
}// namespace toolhub::spv