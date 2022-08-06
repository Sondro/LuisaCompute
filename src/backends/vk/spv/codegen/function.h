#pragma once
#include "types.h"
#include <vstl/small_vector.h>
namespace toolhub::spv {
class Builder;
class Function : public Component {
public:
	Id func;
	Id funcType;
	Id returnType;
	Id funcBlockId;
	vstd::small_vector<Id> argValues;

	Function(Builder* builder, Id returnType, vstd::IRange<Id>* argType);
	// kernel
	Function(Builder* builder);
	Function(Function const&) = delete;
	Function(Function&&) = delete;
	void ReturnValue(Id returnValue);
	void Return();
	//Id GetReturnTypeBranch(Id value);
	~Function();
};
}// namespace toolhub::spv
