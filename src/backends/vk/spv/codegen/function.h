#pragma once
#include "types.h"
#include <vstl/small_vector.h>
namespace toolhub::spv {
class Builder;
class Function : public Component {
	vstd::small_vector<std::pair<Id, Id>> returnValues;

public:
	Id func;
	Id funcType;
	Id returnType;
	Id funcBlockId;
	vstd::small_vector<Id> argValues;

	Function(Builder* builder, Id returnType, vstd::span<Id const> argType);
	// kernel
	Function(Builder* builder);
	Id GetReturnTypeBranch(Id value);
	~Function();
};
}// namespace toolhub::spv
