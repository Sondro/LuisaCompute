#pragma once
#include "types.h"
namespace toolhub::spv {
class Builder;
class IfBranch : public Component {
public:
	struct Branch {
		IfBranch* self;
		Id id;
		Branch(IfBranch* self, Id id);
		Branch(Branch const&) = delete;
		Branch(Branch&&) = delete;
		~Branch();
	};
	Id mergeBlock;
	Id trueBranch;
	Id falseBranch;
	Branch TrueBranchScope();
	Branch FalseBranchScoe();
	IfBranch(Builder* builder, Id condition);
	~IfBranch();
};
}// namespace toolhub::spv