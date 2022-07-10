#pragma once
#include "types.h"
#include <vstl/small_vector.h>
namespace toolhub::spv {
class Builder;
class Function : public Component {
	vstd::small_vector<std::pair<Id, Id>> returnValues;
	Id returnVoidCmd;

public:
	Id func;
	Id funcType;
	Id returnType;
	Id funcBlockId;
	vstd::small_vector<Id> argValues;
	vstd::string varStr;
	vstd::string bodyStr;

	Function(Builder* builder, Id returnType, vstd::span<Id const> argType);
	// kernel
	Function(Builder* builder);
	Function(Function const&) = delete;
	Function(Function&&) = delete;
	Id GetReturnTypeBranch(Id value);
	Id GetReturnBranch();
	~Function();
};
}// namespace toolhub::spv
